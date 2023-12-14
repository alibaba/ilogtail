// Copyright 2021 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package envconfig

import (
	"context"
	"errors"
	"fmt"
	"github.com/alibabacloud-go/tea/tea"
	//"k8s.io/client-go/openapi"
	"strings"
	"sync"
	"time"

	aliyunlog "github.com/alibabacloud-go/sls-20201230/v5/client"
	_ "github.com/alibabacloud-go/tea/tea"

	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/flags"
	k8s_event "github.com/alibaba/ilogtail/pkg/helper/eventrecorder"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
)

// const MachineIDTypes
const (
	MachineIDTypeIP          = "ip"
	MachineIDTypeUserDefined = "userdefined"
)

// nolint:unused
// 定义一个结构体,用于检查配置
type configCheckPoint struct {
	ProjectName string // project名称
	ConfigName  string // 配置名称
}

// 定义一个操作包装器结构体
type operationWrapper struct {
	logClient        **aliyunlog.Client   // 日志客户端接口
	lock             sync.RWMutex         // 读写锁
	project          string               // project名称
	logstoreCacheMap map[string]time.Time // logStore缓存映射
	configCacheMap   map[string]time.Time // 配置缓存映射
}

// createDefaultK8SIndex 创建默认的K8S索引
// logstoreMode 是一个字符串,用于确定文档值是否为标准模式
func createDefaultK8SIndex(logstoreMode string) *aliyunlog.CreateIndexRequest {
	// 判断文档值是否为标准模式
	docValue := logstoreMode == StandardMode
	// 定义一个普通索引键
	normalIndexKey := aliyunlog.KeysValue{
		CaseSensitive: tea.Bool(false), // 不区分大小写
		Chn:           nil,
		Type:          tea.String("text"), // 类型为文本
		Alias:         nil,
		// 定义分词符
		Token:    tea.StringSlice([]string{" ", "\n", "\t", "\r", ",", ";", "[", "]", "{", "}", "(", ")", "&", "^", "*", "#", "@", "~", "=", "<", ">", "/", "\\", "?", ":", "'", "\""}),
		DocValue: &docValue, // 文档值
	}
	return &aliyunlog.CreateIndexRequest{
		// 索引键
		Keys: map[string]*aliyunlog.KeysValue{
			"__tag__:__hostname__":     &normalIndexKey,
			"__tag__:__path__":         &normalIndexKey,
			"__tag__:_container_ip_":   &normalIndexKey,
			"__tag__:_container_name_": &normalIndexKey,
			"__tag__:_image_name_":     &normalIndexKey,
			"__tag__:_namespace_":      &normalIndexKey,
			"__tag__:_pod_name_":       &normalIndexKey,
			"__tag__:_pod_uid_":        &normalIndexKey,
			"_container_ip_":           &normalIndexKey,
			"_container_name_":         &normalIndexKey,
			"_image_name_":             &normalIndexKey,
			"_namespace_":              &normalIndexKey,
			"_pod_name_":               &normalIndexKey,
			"_pod_uid_":                &normalIndexKey,
			"_source_":                 &normalIndexKey,
		}, Line: &aliyunlog.CreateIndexRequestLine{ // 索引行
			CaseSensitive: tea.Bool(false),
			Chn:           nil,
			ExcludeKeys:   nil,
			IncludeKeys:   nil,
			Token:         tea.StringSlice([]string{" ", "\n", "\t", "\r", ",", ";", "[", "]", "{", "}", "(", ")", "&", "^", "*", "#", "@", "~", "=", "<", ">", "/", "\\", "?", ":", "'", "\""}),
		},
		LogReduce:          nil,
		LogReduceBlackList: nil,
		LogReduceWhiteList: nil,
		MaxTextLen:         nil,
		Ttl:                nil,
	}
}

// createClientInterface 创建客户端接口
// endpoint是服务端点,accessKeyID和accessKeySecret是访问密钥,stsToken是安全令牌服务令牌,shutdown是关闭通道
func createClientInterface(endpoint, accessKeyID, accessKeySecret, stsToken string, shutdown <-chan struct{}) (**aliyunlog.Client, error) {
	// 定义客户端接口
	var logClient **aliyunlog.Client
	// 定义错误
	var err error
	if *flags.AliCloudECSFlag {
		// 如果是阿里云ECS, 创建一个自动更新令牌的客户端
		logClient, err = CreateTokenAutoUpdateClient(endpoint, UpdateTokenFunction, shutdown, config.UserAgent)
		// 如果有错误, 返回nil和错误
		if err != nil {
			return nil, err
		}
	} else {
		// 如果不是阿里云ECS, 创建一个普通的客户端接口
		logClient, err = CreateNormalInterface(endpoint, accessKeyID, accessKeySecret, stsToken, config.UserAgent)
		if err != nil {
			return nil, err
		}
	}
	return logClient, err
}

// createAliyunLogOperationWrapper 创建阿里云日志操作包装器
// project是project名称,clientInterface是客户端接口
func createAliyunLogOperationWrapper(project string, logClient **aliyunlog.Client) (*operationWrapper, error) {
	// 定义错误
	var err error
	// 创建一个新的操作包装器
	wrapper := &operationWrapper{
		logClient: logClient, // 设置日志客户端接口
		project:   project,   // 设置project名称
	}
	logger.Info(context.Background(), "init aliyun log operation wrapper", "begin")
	// 重试当创建project失败时
	for i := 0; i < 1; i++ {
		err = wrapper.makesureProjectExist(nil, project) // 确保project存在
		if err == nil {                                  // 如果没有错误
			break // 跳出循环
		}
		logger.Warning(context.Background(), "CREATE_PROJECT_ALARM", "create project error, project", project, "error", err)
		// 等待30秒
		time.Sleep(time.Second * time.Duration(30))
	}

	// 重试当创建机器组失败时
	for i := 0; i < 3; i++ {
		// 确保机器组存在
		err = wrapper.makesureMachineGroupExist(project, *flags.DefaultLogMachineGroup)
		if err == nil { // 如果没有错误
			break // 跳出循环
		}
		logger.Warning(context.Background(), "CREATE_MACHINEGROUP_ALARM", "create machine group error, project", project, "error", err)
		time.Sleep(time.Second * time.Duration(30))
	}
	if err != nil { // 如果有错误
		return nil, err // 返回nil和错误
	}
	// 创建logStore缓存映射
	wrapper.logstoreCacheMap = make(map[string]time.Time)
	// 创建配置缓存映射
	wrapper.configCacheMap = make(map[string]time.Time)
	logger.Info(context.Background(), "init aliyun log operation wrapper", "done")
	return wrapper, nil
}

// logstoreCacheExists 检查给定的project和logStore是否在缓存中存在
// 并且其最后更新时间距离现在的时间小于缓存过期时间。
func (o *operationWrapper) logstoreCacheExists(project, logstore string) bool {
	// 读锁
	o.lock.RLock()
	// 解锁
	defer o.lock.RUnlock()
	// 检查project和logStore是否在缓存中
	if lastUpdateTime, ok := o.logstoreCacheMap[project+"@@"+logstore]; ok {
		// 检查最后更新时间是否在缓存过期时间内
		if time.Since(lastUpdateTime) < time.Second*time.Duration(*flags.LogResourceCacheExpireSec) {
			return true
		}
	}
	return false
}

// addLogstoreCache 将给定的project和logStore添加到缓存中。
func (o *operationWrapper) addLogstoreCache(project, logstore string) {
	// 写锁
	o.lock.Lock()
	// 解锁
	defer o.lock.Unlock()
	// 将project和logStore添加到缓存中
	o.logstoreCacheMap[project+"@@"+logstore] = time.Now()
}

// configCacheExists 检查给定的project和配置是否在缓存中存在
// 并且其最后更新时间距离现在的时间小于缓存过期时间。
func (o *operationWrapper) configCacheExists(project, config string) bool {
	// 读锁
	o.lock.RLock()
	// 解锁
	defer o.lock.RUnlock()
	// 检查project和配置是否在缓存中
	if lastUpdateTime, ok := o.configCacheMap[project+"@@"+config]; ok {
		// 检查最后更新时间是否在缓存过期时间内
		if time.Since(lastUpdateTime) < time.Second*time.Duration(*flags.LogResourceCacheExpireSec) {
			return true
		}
	}
	return false
}

// addConfigCache 将给定的project和配置添加到缓存中。
func (o *operationWrapper) addConfigCache(project, config string) {
	// 写锁
	o.lock.Lock()
	// 解锁
	defer o.lock.Unlock()
	// 将project和配置添加到缓存中
	o.configCacheMap[project+"@@"+config] = time.Now()
}

// removeConfigCache 从缓存中删除给定的project和配置。
func (o *operationWrapper) removeConfigCache(project, config string) {
	// 写锁
	o.lock.Lock()
	// 解锁
	defer o.lock.Unlock()
	// 从缓存中删除project和配置
	delete(o.configCacheMap, project+"@@"+config)
}

// retryCreateIndex 重试创建索引,如果索引已存在,则返回。
func (o *operationWrapper) retryCreateIndex(project, logstore, logstoremode string) {
	// 等待10秒
	time.Sleep(time.Second * 10)
	// 创建默认的K8S索引
	index := createDefaultK8SIndex(logstoremode)
	// 创建索引,创建索引不返回错误, 重试10次
	for i := 0; i < 10; i++ {
		// 创建索引
		_, err := (*o.logClient).CreateIndex(&project, &logstore, index)
		if err != nil {
			// 如果索引已存在,就返回
			var clientError *tea.SDKError
			if errors.As(err, &clientError) && tea.StringValue(clientError.Code) == "IndexAlreadyExist" {
				return
			}
			time.Sleep(time.Second * 10) // 等待10秒
		} else {
			break
		}
	}
}

// createProductLogstore 创建logStore
func (o *operationWrapper) createProductLogstore(config *AliyunLogConfigSpec, project, logstore, product, lang string, hotTTL int) error {
	logger.Info(context.Background(), "begin to create product logstore, project", project, "logstore", logstore, "product", product, "lang", lang)
	// 调用 CreateProductLogstore 函数创建logStore
	err := CreateProductLogstore(*flags.DefaultRegion, project, logstore, product, lang, hotTTL)

	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, product, tea.StringValue(config.LogtailConfig.ConfigName), false)

	// 如果创建过程中出现错误
	if err != nil {
		// 如果事件记录器存在
		if k8s_event.GetEventRecorder() != nil {
			// 从 PopError 中获取自定义错误
			customErr := CustomErrorFromPopError(err)
			// 发送带有注解的错误事件
			k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.CreateProductLogStore, "", fmt.Sprintf("create product log failed, error: %s", err.Error()))
		}
		// 记录创建产品错误的警告日志
		logger.Warning(context.Background(), "CREATE_PRODUCT_ALARM", "create product error, error", err)
		// 返回错误
		return err
	} else if k8s_event.GetEventRecorder() != nil { // 如果事件记录器存在
		// 发送正常的事件,表示产品日志创建成功
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateProductLogStore, "create product log success")
	}

	// 将logStore添加到缓存中
	o.addLogstoreCache(project, logstore)
	// 返回 nil,表示没有错误
	return nil
}

// makesureLogstoreExist 是一个方法,用于确保logStore存在
func (o *operationWrapper) makesureLogstoreExist(config *AliyunLogConfigSpec) error {
	// 获取project名称
	project := o.project
	// 如果配置中的project名称不为空,则使用配置中的project名称
	if len(config.Project) != 0 {
		project = config.Project
	}
	// 获取logStore名称
	logstore := config.Logstore
	// 初始化分片数量为 0
	shardCount := 0
	// 如果配置中的分片数量不为空,则使用配置中的分片数量
	if config.ShardCount != nil {
		shardCount = int(*config.ShardCount)
	}
	// 初始化生命周期为 0
	lifeCycle := 0
	// 如果配置中的生命周期不为空,则使用配置中的生命周期
	if config.LifeCycle != nil {
		lifeCycle = int(*config.LifeCycle)
	}
	// 初始化模式为 StandardMode
	mode := StandardMode
	// 如果配置中的logStore模式不为空
	if len(config.LogstoreMode) != 0 {
		// 如果配置中的logStore模式为 QueryMode,则将模式设置为 QueryMode
		if config.LogstoreMode == QueryMode {
			mode = QueryMode
		}
	}

	// 获取产品代码和产品语言
	product := config.ProductCode
	lang := config.ProductLang
	// 初始化 hotTTL 为 0
	hotTTL := 0
	// 如果配置中的 LogstoreHotTTL 不为空,则使用配置中的 LogstoreHotTTL
	if config.LogstoreHotTTL != nil {
		hotTTL = int(*config.LogstoreHotTTL)
	}

	// 如果logStore已经存在于缓存中,则返回 nil
	if o.logstoreCacheExists(project, logstore) {
		return nil
	}

	// 如果产品代码长度大于 0
	if len(product) > 0 {
		// 如果产品语言长度为 0,则将产品语言设置为 "cn"
		if len(lang) == 0 {
			lang = "cn"
		}
		// 调用 createProductLogstore 方法创建产品logStore
		return o.createProductLogstore(config, project, logstore, product, lang, hotTTL)
	}

	// 如果logStore名称以 "audit-" 开头并且长度为 39
	if strings.HasPrefix(logstore, "audit-") && len(logstore) == 39 {
		// 调用 createProductLogstore 方法创建产品logStore,产品为 "k8s-audit",语言为 "cn",hotTTL 为 0
		return o.createProductLogstore(config, project, logstore, "k8s-audit", "cn", 0)
	}

	// 如果project名称不等于 o.project
	if project != o.project {
		// 确保project存在
		if err := o.makesureProjectExist(config, project); err != nil {
			return err
		}
	}
	// 初始化 ok 为 nil
	var ok *aliyunlog.GetLogStoreResponse
	var err error
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 检查logStore是否存在
		if ok, err = (*o.logClient).GetLogStore(&project, &logstore); err != nil {
			// 如果存在错误,则等待 100 毫秒
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误,则跳出循环
			break
		}
	}
	// 如果logStore已经存在
	if ok != nil && ok.Body != nil && err == nil {
		// 在后台记录日志,表示logStore已经存在
		logger.Info(context.Background(), "logstore already exist, logstore", logstore)
		// 将logStore添加到缓存中
		o.addLogstoreCache(project, logstore)
		// 返回 nil,表示没有错误
		return nil
	}
	// 设置默认的生命周期为 90 天
	ttl := 90
	// 如果生命周期大于 0,那么使用用户设置的生命周期
	if lifeCycle > 0 {
		ttl = lifeCycle
	}
	// 如果分片数量小于等于 0,那么设置默认的分片数量为 2
	if shardCount <= 0 {
		shardCount = 2
	}
	// 如果分片数量大于 10,那么设置分片数量为 10
	// 这是因为初始化的最大分片数量限制为 10
	if shardCount > 10 {
		shardCount = 10
	}
	// 创建一个新的logStore对象
	logStore := &aliyunlog.CreateLogStoreRequest{
		LogstoreName:  tea.String(logstore),         // logStore的名称
		Ttl:           tea.Int32(int32(ttl)),        // logStore的生命周期
		ShardCount:    tea.Int32(int32(shardCount)), // 分片数量
		AutoSplit:     tea.Bool(true),               // 是否自动分片
		MaxSplitShard: tea.Int32(32),                // 最大分片数量
		Mode:          tea.String(mode),             // 模式
	}
	// 如果设置了热存储的生命周期,那么设置logStore的热存储生命周期
	if config.LogstoreHotTTL != nil {
		logStore.HotTtl = tea.Int32(*config.LogstoreHotTTL)
	}
	// 如果设置了遥测类型,那么设置logStore的遥测类型
	if len(config.LogstoreTelemetryType) > 0 {
		if MetricsTelemetryType == config.LogstoreTelemetryType {
			logStore.TelemetryType = tea.String(config.LogstoreTelemetryType)
		}
	}
	// 如果设置了追加元数据,那么设置logStore的追加元数据
	if config.LogstoreAppendMeta {
		logStore.AppendMeta = tea.Bool(config.LogstoreAppendMeta)
	}
	// 如果设置了启用跟踪,那么设置logStore的跟踪
	if config.LogstoreEnableTracking {
		logStore.EnableTracking = tea.Bool(config.LogstoreEnableTracking)
	}
	// 如果设置了自动分片,那么设置logStore的自动分片
	if config.LogstoreAutoSplit {
		logStore.AutoSplit = tea.Bool(config.LogstoreAutoSplit)
	}
	// 如果设置了最大分片数量,那么设置logStore的最大分片数量
	if config.LogstoreMaxSplitShard != nil {
		logStore.MaxSplitShard = tea.Int32(*config.LogstoreMaxSplitShard)
	}
	// 如果启用了加密,那么设置logStore的加密配置
	if *config.LogstoreEncryptConf.Enable {
		logStore.EncryptConf = &config.LogstoreEncryptConf
	}
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 尝试创建logStore
		_, err = (*o.logClient).CreateLogStore(&project, logStore)
		// 如果创建失败,那么等待 100 毫秒后重试
		if err != nil {
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果创建成功,那么将logStore添加到缓存中,并记录日志,然后退出循环
			o.addLogstoreCache(project, logstore)
			logger.Info(context.Background(), "create logstore success, logstore", logstore)
			break
		}
	}
	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, "", tea.StringValue(config.LogtailConfig.ConfigName), false)
	// 如果创建logStore失败,那么发送错误事件
	if err != nil {
		if k8s_event.GetEventRecorder() != nil {
			customErr := CustomErrorFromSlsSDKError(err)
			k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.CreateLogstore, "", fmt.Sprintf("create logstore failed, error: %s", err.Error()))
		}
		// 返回错误
		return err
	} else if k8s_event.GetEventRecorder() != nil {
		// 如果创建logStore成功,那么发送正常事件
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateLogstore, "create logstore success")
	}
	// 创建logStore成功后,等待 1 秒
	time.Sleep(time.Second)
	// 使用默认的 k8s 索引
	index := createDefaultK8SIndex(mode)
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 尝试创建索引
		_, err = (*o.logClient).CreateIndex(&project, &logstore, index)
		// 如果创建失败,那么等待 100 毫秒后重试
		if err != nil {
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果创建成功,那么退出循环
			break
		}
	}
	// 如果创建索引成功,那么记录日志
	if err == nil {
		logger.Info(context.Background(), "create index done, logstore", logstore)
	} else {
		// 如果创建索引失败,那么记录警告日志
		logger.Warning(context.Background(), "CREATE_INDEX_ALARM", "create index done, logstore", logstore, "error", err)
	}
	return nil
}

// makesureProjectExist 函数确保在阿里云日志服务中存在指定的project。
// 如果project不存在,该函数将尝试创建它。
// config 参数是阿里云日志配置的规格,project 是要检查或创建的project的名称。
func (o *operationWrapper) makesureProjectExist(config *AliyunLogConfigSpec, project string) error {
	var ok *aliyunlog.GetProjectResponse // 初始化 ok 为 false,表示project是否存在的标志
	var err error                        // 初始化 err 为 nil,用于存储错误信息

	// 尝试最大重试次数来检查project是否存在
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CheckProjectExist 方法检查project是否存在
		if ok, err = (*o.logClient).GetProject(&project); err != nil {
			// 如果出现错误,暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 1000)
		} else {
			// 如果没有错误,跳出循环
			break
		}
	}
	// 如果project存在,返回 nil
	if ok != nil && ok.Body != nil && err == nil {
		return nil
	}
	// 如果project不存在,尝试最大重试次数来创建project
	createProjectRequest := aliyunlog.CreateProjectRequest{
		DataRedundancyType: tea.String(""),
		Description:        tea.String("k8s log project, created by alibaba cloud log controller"),
		ProjectName:        tea.String(project),
		ResourceGroupId:    nil,
	}
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CreateProject 方法创建project
		_, err = (*o.logClient).CreateProject(&createProjectRequest)
		if err != nil {
			// 如果出现错误,暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 1000)
		} else {
			// 如果没有错误,跳出循环
			break
		}
	}
	// 初始化配置名称和logStore变量
	configName := ""
	logstore := ""
	// 如果配置不为 nil,获取配置名称和logStore
	if config != nil {
		configName = tea.StringValue(config.LogtailConfig.ConfigName)
		logstore = config.Logstore
	}
	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, "", configName, false)
	// 如果创建project过程中出现错误,发送错误事件
	if err != nil {
		if k8s_event.GetEventRecorder() != nil {
			customErr := CustomErrorFromSlsSDKError(err)
			k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.CreateProject, "", fmt.Sprintf("create project failed, error: %s", err.Error()))
		}
	} else if k8s_event.GetEventRecorder() != nil {
		// 如果创建project成功,发送正常事件
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateProject, "create project success")
	}
	// 返回错误信息
	return err
}

// makesureMachineGroupExist 函数确保在阿里云日志服务中存在指定的机器组。
// 如果机器组不存在,该函数将尝试创建它。
// project 是project的名称,machineGroup 是要检查或创建的机器组的名称。
func (o *operationWrapper) makesureMachineGroupExist(project, machineGroup string) error {
	var ok *aliyunlog.GetMachineGroupResponse // 初始化 ok 为 nil,表示机器组是否存在的标志
	var err error                             // 初始化 err 为 nil,用于存储错误信息

	// 尝试最大重试次数来检查机器组是否存在
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CheckMachineGroupExist 方法检查机器组是否存在
		if ok, err = (*o.logClient).GetMachineGroup(&project, &machineGroup); err != nil {
			// 如果出现错误,暂停100Ms后再次尝试
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误,跳出循环
			break
		}
	}
	if err != nil {
		return err
	}
	// 如果机器组存在,返回 nil
	if ok != nil && ok.Body != nil {
		return nil
	}
	// 如果机器组不存在,创建一个新的机器组
	createMachineGroupRequest := aliyunlog.CreateMachineGroupRequest{
		GroupAttribute:      nil,
		GroupName:           tea.String(machineGroup),
		GroupType:           tea.String(""),
		MachineIdentifyType: tea.String(MachineIDTypeUserDefined),
		MachineList:         tea.StringSlice([]string{machineGroup}),
	}
	// 尝试最大重试次数来创建机器组
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CreateMachineGroup 方法创建机器组
		_, err = (*o.logClient).CreateMachineGroup(&project, &createMachineGroupRequest)
		if err != nil {
			// 如果出现错误,暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误,尝试最大重试次数来标记机器组
			for j := 0; j < *flags.LogOperationMaxRetryTimes; j++ {
				// 调用 TagMachineGroup 方法标记机器组
				err = o.TagMachineGroup(project, machineGroup, SlsMachinegroupDeployModeKey, SlsMachinegroupDeployModeDeamonset)
				if err != nil {
					// 如果出现错误,暂停一秒后再次尝试
					time.Sleep(time.Millisecond * 100)
				} else {
					// 如果没有错误,跳出循环
					break
				}
			}
			// 跳出创建机器组的循环
			break
		}
	}
	// 返回错误信息
	return err
}

// nolint:unused
// deleteConfig 函数用于删除配置
func (o *operationWrapper) deleteConfig(checkpoint *configCheckPoint) error {
	var err error
	// 循环尝试删除配置,最大尝试次数为 LogOperationMaxRetryTimes
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 DeleteConfig 方法来删除配置
		_, err = (*o.logClient).DeleteConfig(&checkpoint.ProjectName, &checkpoint.ConfigName)
		// 如果没有错误,那么从缓存中移除配置并返回 nil
		if err == nil {
			o.removeConfigCache(checkpoint.ProjectName, checkpoint.ConfigName)
			return nil
		}
		// 如果错误是 tea.SDKError 类型
		var sdkError *tea.SDKError
		if errors.As(err, &sdkError) {
			// 如果 HTTP 状态码是 404,那么从缓存中移除配置并返回 nil
			if tea.IntValue(sdkError.StatusCode) == 404 {
				o.removeConfigCache(checkpoint.ProjectName, checkpoint.ConfigName)
				return nil
			}
		}
		// 等待 100 毫秒
		time.Sleep(time.Millisecond * 100)
	}
	// 如果有错误,那么返回错误信息
	if err != nil {
		return fmt.Errorf("DeleteConfig error, config : %s, error : %s", checkpoint.ConfigName, err.Error())
	}
	// 返回错误
	return err
}

// updateConfig 函数用于更新配置
func (o *operationWrapper) updateConfig(config *AliyunLogConfigSpec) error {
	// 调用 updateConfigInner 方法来更新配置
	return o.updateConfigInner(config)
}

// checkFileConfigChanged 函数用于检查文件配置是否发生了变化
func checkFileConfigChanged(filePaths, includeEnv, includeLabel string, inputs map[string]interface{}) bool {
	// 判断FilePaths是否存在
	if _, ok := inputs["FilePaths"]; !ok {
		return false
	}
	serverFilePaths, _ := util.InterfaceToString(inputs["logPath"])
	// 判断ContainerFilters是否存在
	if _, ok := inputs["ContainerFilters"]; !ok {
		return false
	}
	var containerFilters map[string]interface{}
	var ok bool
	// 判断ContainerFilters类型是否正确
	if containerFilters, ok = inputs["ContainerFilters"].(map[string]interface{}); !ok {
		return false
	}
	// 判断IncludeEnv是否存在
	if _, ok = containerFilters["IncludeEnv"]; !ok {
		return false
	}
	serverIncludeEnv, _ := util.InterfaceToJSONString(containerFilters["IncludeEnv"])
	// 判断IncludeContainerLabel是否存在
	if _, ok = containerFilters["IncludeContainerLabel"]; !ok {
		return false
	}
	serverIncludeLabel, _ := util.InterfaceToJSONString(containerFilters["IncludeContainerLabel"])
	// 检查各项配置是否发生了变化
	return filePaths != serverFilePaths ||
		includeEnv != serverIncludeEnv ||
		includeLabel != serverIncludeLabel
}

// UnTagLogtailConfig operationWrapper 结构体的 UnTagLogtailConfig 方法,用于移除 Logtail 配置所有的标签
func (o *operationWrapper) UnTagLogtailConfig(project string, logtailConfig string) error {
	var err error // 定义一个错误变量

	untagResourcesRequest := aliyunlog.UntagResourcesRequest{
		All:          tea.Bool(true),
		ResourceId:   tea.StringSlice([]string{project + "#" + logtailConfig}),
		ResourceType: tea.String(TagLogtailConfig),
	}
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		_, err = (*o.logClient).UntagResources(&untagResourcesRequest) // 移除标签
		if err == nil {                                                // 如果没有错误,返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// TagLogtailConfig operationWrapper 结构体的 TagLogtailConfig 方法,用于给 Logtail 配置添加标签
func (o *operationWrapper) TagLogtailConfig(project string, logtailConfig string, tags map[string]string) error {
	var err error // 定义一个错误变量

	// 在创建之前删除所有标签
	err = o.UnTagLogtailConfig(project, logtailConfig)
	if err != nil { // 如果有错误,返回错误
		return err
	}

	// 定义一个用于添加标签的资源
	tagResourcesRequest := aliyunlog.TagResourcesRequest{
		ResourceId:   tea.StringSlice([]string{project + "#" + logtailConfig}),
		ResourceType: tea.String(TagLogtailConfig),
		Tags:         []*aliyunlog.TagResourcesRequestTags{},
	}
	for k, v := range tags { // 遍历所有标签
		tag := aliyunlog.TagResourcesRequestTags{Key: tea.String(k), Value: tea.String(v)} // 创建一个新的标签
		tagResourcesRequest.Tags = append(tagResourcesRequest.Tags, &tag)                  // 添加到资源中
	}
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		_, err = (*o.logClient).TagResources(&tagResourcesRequest) // 添加标签
		if err == nil {                                            // 如果没有错误,返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// TagMachineGroup operationWrapper 结构体的 TagMachineGroup 方法,用于给机器组添加标签
func (o *operationWrapper) TagMachineGroup(project, machineGroup, tagKey, tagValue string) error {
	// 定义一个用于添加标签的资源
	tagResourcesRequest := aliyunlog.TagResourcesRequest{
		ResourceId:   tea.StringSlice([]string{project + "#" + machineGroup}),
		ResourceType: tea.String(TagMachinegroup),
		Tags:         []*aliyunlog.TagResourcesRequestTags{{Key: tea.String(tagKey), Value: tea.String(tagValue)}},
	}
	var err error                                           // 定义一个错误变量
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		_, err = (*o.logClient).TagResources(&tagResourcesRequest) // 添加标签
		if err == nil {                                            // 如果没有错误,返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// nolint:govet,ineffassign
// 定义一个名为updateConfigInner的方法,该方法接收一个AliyunLogConfigSpec类型的指针作为参数,并返回一个错误类型的值
func (o *operationWrapper) updateConfigInner(config *AliyunLogConfigSpec) error {
	project := o.project
	if len(config.Project) != 0 {
		project = config.Project
	}
	logstore := config.Logstore
	// 调用makesureLogstoreExist方法,确保logStore存在
	err := o.makesureLogstoreExist(config)
	if err != nil {
		return fmt.Errorf("Create logconfig error when update config, config : %s, error : %s", config.LogtailConfig.ConfigName, err.Error())
	}

	logger.Info(context.Background(), "create or update config", config.LogtailConfig.ConfigName, "detail", config.LogtailConfig.GoString())

	ok := false

	// 服务端配置
	var serverConfig *aliyunlog.LogtailPipelineConfig
	var getLogtailPipelineConfigResponse *aliyunlog.GetLogtailPipelineConfigResponse
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		getLogtailPipelineConfigResponse, err = (*o.logClient).GetLogtailPipelineConfig(&project, config.LogtailConfig.ConfigName)
		if err != nil {
			var slsErr *tea.SDKError
			if errors.As(err, &slsErr) {
				if tea.StringValue(slsErr.Code) == "ConfigNotExist" {
					ok = false
					break
				}
			}
		} else {
			ok = true
			serverConfig = getLogtailPipelineConfigResponse.Body
			break
		}
	}

	logger.Info(context.Background(), "get config", config.LogtailConfig.ConfigName, "result", ok)

	// 如果服务端配置存在
	if ok && serverConfig != nil {
		// 如果配置为简单配置
		if config.SimpleConfig {
			if config.LogtailConfig.Inputs == nil || len(config.LogtailConfig.Inputs) == 0 {
				updateLogtailPipelineConfigRequest := aliyunlog.UpdateLogtailPipelineConfigRequest{
					Aggregators: config.LogtailConfig.Aggregators,
					ConfigName:  config.LogtailConfig.ConfigName,
					Flushers: []map[string]interface{}{
						{
							"Logstore": logstore,
							"Type":     "flusher_sls",
						},
					},
					Global:     config.LogtailConfig.Global,
					Inputs:     config.LogtailConfig.Inputs,
					LogSample:  config.LogtailConfig.LogSample,
					Processors: config.LogtailConfig.Processors,
				}
				for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
					_, err = (*o.logClient).UpdateLogtailPipelineConfig(&project, config.LogtailConfig.ConfigName, &updateLogtailPipelineConfigRequest)
					if err == nil {
						break
					}
				}
			} else if config.LogtailConfig.Inputs[0]["Type"].(string) == "input_file" && serverConfig.Inputs[0]["Type"].(string) == "input_file" {
				// 如果配置的输入类型为"input_file",并且serverConfig的输入类型也为"input_file"
				// 则检查env配置的FilePaths、includeEnv、includeLabel是否和服务端配置相同, 如果不同则更新服务端配置

				// 获取FilePaths、includeEnv、includeLabel的值
				filePaths, _ := util.InterfaceToString(config.LogtailConfig.Inputs[0]["FilePaths"])
				includeEnv, _ := util.InterfaceToJSONString(config.LogtailConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeEnv"])
				includeLabel, _ := util.InterfaceToJSONString(config.LogtailConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeContainerLabel"])

				if len(filePaths) > 0 {

					if checkFileConfigChanged(filePaths, includeEnv, includeLabel, serverConfig.Inputs[0]) {
						logger.Info(context.Background(), "file config changed, old", serverConfig.GoString(), "new", config.LogtailConfig.GoString())
						serverConfig.Inputs[0]["FilePaths"] = filePaths
						if _, ok = serverConfig.Inputs[0]["ContainerFilters"]; !ok {
							serverConfig.Inputs[0]["ContainerFilters"] = map[string]interface{}{}
						}
						serverConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeEnv"] = config.LogtailConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeEnv"]
						serverConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeContainerLabel"] = config.LogtailConfig.Inputs[0]["ContainerFilters"].(map[string]interface{})["IncludeContainerLabel"]
						updateLogtailPipelineConfigRequest := aliyunlog.UpdateLogtailPipelineConfigRequest{
							Aggregators: serverConfig.Aggregators,
							ConfigName:  serverConfig.ConfigName,
							Flushers:    serverConfig.Flushers,
							Global:      serverConfig.Global,
							Inputs:      serverConfig.Inputs,
							LogSample:   serverConfig.LogSample,
							Processors:  serverConfig.Processors,
						}
						// 循环尝试更新配置,直到成功或达到最大重试次数
						for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
							_, err = (*o.logClient).UpdateLogtailPipelineConfig(&project, config.LogtailConfig.ConfigName, &updateLogtailPipelineConfigRequest)
							if err == nil {
								break
							}
						}

						// 拼装事件, 记录到k8s_event中
						annotations := GetAnnotationByObject(config, project, logstore, "", tea.StringValue(config.LogtailConfig.ConfigName), true)
						if err != nil {
							if k8s_event.GetEventRecorder() != nil {
								customErr := CustomErrorFromSlsSDKError(err)
								k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.UpdateConfig, "", fmt.Sprintf("update config failed, error: %s", err.Error()))
							}
						} else {
							if k8s_event.GetEventRecorder() != nil {
								k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.UpdateConfig, "update config success")
							}
						}

					} else {
						logger.Info(context.Background(), "file config not changed", "skip update")
					}
				}
			} else if config.LogtailConfig.Inputs[0]["Type"].(string) != serverConfig.Inputs[0]["Type"].(string) && ok {
				// 如果配置的输入类型和serverConfig的输入类型不一致, 强制更新
				logger.Info(context.Background(), "config input type change from", serverConfig.Inputs[0]["Type"].(string),
					"to", config.LogtailConfig.Inputs[0]["Type"].(string), "force update")
				updateLogtailPipelineConfigRequest := aliyunlog.UpdateLogtailPipelineConfigRequest{
					Aggregators: config.LogtailConfig.Aggregators,
					ConfigName:  config.LogtailConfig.ConfigName,
					Flushers: []map[string]interface{}{
						{
							"Logstore": logstore,
							"Type":     "flusher_sls",
						},
					},
					Global:     config.LogtailConfig.Global,
					Inputs:     config.LogtailConfig.Inputs,
					LogSample:  config.LogtailConfig.LogSample,
					Processors: config.LogtailConfig.Processors,
				}
				for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
					_, err = (*o.logClient).UpdateLogtailPipelineConfig(&project, config.LogtailConfig.ConfigName, &updateLogtailPipelineConfigRequest)
					if err == nil {
						break
					}
				}
			}
			logger.Info(context.Background(), "config updated, server config", *serverConfig, "local config", *config)
		}

	} else {
		// 如果配置不存在, 创建新的配置
		createLogtailPipelineConfigRequest := aliyunlog.CreateLogtailPipelineConfigRequest{
			Aggregators: config.LogtailConfig.Aggregators,
			ConfigName:  config.LogtailConfig.ConfigName,
			Flushers: []map[string]interface{}{
				{
					"Logstore": logstore,
					"Type":     "flusher_sls",
				},
			},
			Global:     config.LogtailConfig.Global,
			Inputs:     config.LogtailConfig.Inputs,
			LogSample:  config.LogtailConfig.LogSample,
			Processors: config.LogtailConfig.Processors,
		}
		for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
			_, err = (*o.logClient).CreateLogtailPipelineConfig(&project, &createLogtailPipelineConfigRequest)
			if err == nil {
				break
			}
		}
		// 拼装事件, 记录到k8s_event中
		annotations := GetAnnotationByObject(config, project, logstore, "", tea.StringValue(config.LogtailConfig.ConfigName), true)
		if err != nil {
			if k8s_event.GetEventRecorder() != nil {
				customErr := CustomErrorFromSlsSDKError(err)
				k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.UpdateConfig, "", fmt.Sprintf("update config failed, error: %s", err.Error()))
			}
		} else if k8s_event.GetEventRecorder() != nil {
			k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.UpdateConfig, "update config success")
		}
	}
	if err != nil {
		return fmt.Errorf("UpdateConfig error, config : %s, error : %s", config.LogtailConfig.ConfigName, err.Error())
	}
	logger.Info(context.Background(), "create or update config success", config.LogtailConfig.ConfigName)

	// 为Logtail配置添加标签
	logtailConfigTags := map[string]string{}
	for k, v := range config.ConfigTags {
		logtailConfigTags[k] = v
	}
	logtailConfigTags[SlsLogtailChannalKey] = SlsLogtailChannalEnv
	err = o.TagLogtailConfig(project, tea.StringValue(config.LogtailConfig.ConfigName), logtailConfigTags)

	// 拼装事件, 记录到k8s_event中
	annotations := GetAnnotationByObject(config, project, logstore, "", tea.StringValue(config.LogtailConfig.ConfigName), true)
	if err != nil {
		k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, CustomErrorFromSlsSDKError(err)), k8s_event.CreateTag, "", fmt.Sprintf("tag config %s error :%s", config.LogtailConfig.ConfigName, err.Error()))
	} else {
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateTag, fmt.Sprintf("tag config %s success", config.LogtailConfig.ConfigName))
	}

	// 检查配置是否在机器组中, 只在创建配置时检查
	var machineGroup string
	// 如果配置中有机器组,取第一个机器组
	if len(config.MachineGroups) > 0 {
		machineGroup = config.MachineGroups[0]
	} else {
		// 如果配置中没有机器组,使用默认的机器组
		machineGroup = *flags.DefaultLogMachineGroup
	}
	// 确保机器组存在
	err = o.makesureMachineGroupExist(project, machineGroup)
	if err != nil {
		return fmt.Errorf("makesureMachineGroupExist error, config : %s, machineGroup : %s, error : %s", tea.StringValue(config.LogtailConfig.ConfigName), machineGroup, err.Error())
	}

	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 将配置应用到机器组
		_, err = (*o.logClient).ApplyConfigToMachineGroup(&project, &machineGroup, config.LogtailConfig.ConfigName)
		if err == nil {
			break
		}
	}
	if err != nil {
		return fmt.Errorf("ApplyConfigToMachineGroup error, config : %s, machine group : %s, error : %s", config.LogtailConfig.ConfigName, machineGroup, err.Error())
	}
	logger.Info(context.Background(), "apply config to machine group success", config.LogtailConfig.ConfigName, "group", machineGroup)
	// 将配置添加到缓存中
	o.addConfigCache(project, tea.StringValue(config.LogtailConfig.ConfigName))
	return nil
}
