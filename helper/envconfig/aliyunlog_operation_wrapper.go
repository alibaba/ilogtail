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
	"fmt"
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

// nolint:unused
// 定义一个结构体，用于检查配置
type configCheckPoint struct {
	ProjectName string // 项目名称
	ConfigName  string // 配置名称
}

// 定义一个操作包装器结构体
type operationWrapper struct {
	logClient        aliyunlog.ClientInterface // 日志客户端接口
	lock             sync.RWMutex              // 读写锁
	project          string                    // 项目名称
	logstoreCacheMap map[string]time.Time      // 日志存储缓存映射
	configCacheMap   map[string]time.Time      // 配置缓存映射
}

// 创建默认的K8S索引
// logstoremode是一个字符串，用于确定文档值是否为标准模式
func createDefaultK8SIndex(logstoremode string) *aliyunlog.Index {
	docvalue := logstoremode == StandardMode // 判断文档值是否为标准模式
	normalIndexKey := aliyunlog.IndexKey{    // 定义一个普通索引键
		// 定义分词符
		Token:         []string{" ", "\n", "\t", "\r", ",", ";", "[", "]", "{", "}", "(", ")", "&", "^", "*", "#", "@", "~", "=", "<", ">", "/", "\\", "?", ":", "'", "\""},
		CaseSensitive: false,    // 不区分大小写
		Type:          "text",   // 类型为文本
		DocValue:      docvalue, // 文档值
	}
	// 返回一个新的索引
	return &aliyunlog.Index{
		Line: &aliyunlog.IndexLine{ // 索引行
			Token:         []string{" ", "\n", "\t", "\r", ",", ";", "[", "]", "{", "}", "(", ")", "&", "^", "*", "#", "@", "~", "=", "<", ">", "/", "\\", "?", ":", "'", "\""},
			CaseSensitive: false,
		},
		// 索引键
		Keys: map[string]aliyunlog.IndexKey{
			"__tag__:__hostname__":     normalIndexKey,
			"__tag__:__path__":         normalIndexKey,
			"__tag__:_container_ip_":   normalIndexKey,
			"__tag__:_container_name_": normalIndexKey,
			"__tag__:_image_name_":     normalIndexKey,
			"__tag__:_namespace_":      normalIndexKey,
			"__tag__:_pod_name_":       normalIndexKey,
			"__tag__:_pod_uid_":        normalIndexKey,
			"_container_ip_":           normalIndexKey,
			"_container_name_":         normalIndexKey,
			"_image_name_":             normalIndexKey,
			"_namespace_":              normalIndexKey,
			"_pod_name_":               normalIndexKey,
			"_pod_uid_":                normalIndexKey,
			"_source_":                 normalIndexKey,
		},
	}
}

// 添加必要的输入配置字段
// inputConfigDetail是一个映射，包含输入配置的详细信息
func addNecessaryInputConfigField(inputConfigDetail map[string]interface{}) map[string]interface{} {
	inputConfigDetailCopy := util.DeepCopy(&inputConfigDetail)                                       // 深度复制输入配置详细信息
	aliyunlog.AddNecessaryInputConfigField(*inputConfigDetailCopy)                                   // 添加必要的输入配置字段
	logger.Debug(context.Background(), "before", inputConfigDetail, "after", *inputConfigDetailCopy) // 记录调试信息
	return *inputConfigDetailCopy                                                                    // 返回添加了必要字段的输入配置详细信息
}

// 创建客户端接口
// endpoint是服务端点，accessKeyID和accessKeySecret是访问密钥，stsToken是安全令牌服务令牌，shutdown是关闭通道
func createClientInterface(endpoint, accessKeyID, accessKeySecret, stsToken string, shutdown <-chan struct{}) (aliyunlog.ClientInterface, error) {
	var clientInterface aliyunlog.ClientInterface // 定义客户端接口
	var err error                                 // 定义错误
	if *flags.AliCloudECSFlag {                   // 如果是阿里云ECS
		clientInterface, err = aliyunlog.CreateTokenAutoUpdateClient(endpoint, UpdateTokenFunction, shutdown) // 创建一个自动更新令牌的客户端
		if err != nil {                                                                                       // 如果有错误
			return nil, err // 返回nil和错误
		}
	} else { // 如果不是阿里云ECS
		clientInterface = aliyunlog.CreateNormalInterface(endpoint, accessKeyID, accessKeySecret, stsToken) // 创建一个普通的客户端接口
	}
	clientInterface.SetUserAgent(config.UserAgent) // 设置用户代理
	return clientInterface, err                    // 返回客户端接口和错误
}

// 创建阿里云日志操作包装器
// project是项目名称，clientInterface是客户端接口
func createAliyunLogOperationWrapper(project string, clientInterface aliyunlog.ClientInterface) (*operationWrapper, error) {
	var err error                 // 定义错误
	wrapper := &operationWrapper{ // 创建一个新的操作包装器
		logClient: clientInterface, // 设置日志客户端接口
		project:   project,         // 设置项目名称
	}
	logger.Info(context.Background(), "init aliyun log operation wrapper", "begin") // 记录信息
	// 重试当创建项目失败时
	for i := 0; i < 1; i++ {
		err = wrapper.makesureProjectExist(nil, project) // 确保项目存在
		if err == nil {                                  // 如果没有错误
			break // 跳出循环
		}
		logger.Warning(context.Background(), "CREATE_PROJECT_ALARM", "create project error, project", project, "error", err) // 记录警告信息
		time.Sleep(time.Second * time.Duration(30))                                                                          // 等待30秒
	}

	// 重试当创建机器组失败时
	for i := 0; i < 3; i++ {
		err = wrapper.makesureMachineGroupExist(project, *flags.DefaultLogMachineGroup) // 确保机器组存在
		if err == nil {                                                                 // 如果没有错误
			break // 跳出循环
		}
		logger.Warning(context.Background(), "CREATE_MACHINEGROUP_ALARM", "create machine group error, project", project, "error", err) // 记录警告信息
		time.Sleep(time.Second * time.Duration(30))                                                                                     // 等待30秒
	}
	if err != nil { // 如果有错误
		return nil, err // 返回nil和错误
	}
	wrapper.logstoreCacheMap = make(map[string]time.Time)                          // 创建日志存储缓存映射
	wrapper.configCacheMap = make(map[string]time.Time)                            // 创建配置缓存映射
	logger.Info(context.Background(), "init aliyun log operation wrapper", "done") // 记录信息
	return wrapper, nil                                                            // 返回操作包装器和nil
}

// logstoreCacheExists 检查给定的项目和日志存储是否在缓存中存在，并且其最后更新时间距离现在的时间小于缓存过期时间。
func (o *operationWrapper) logstoreCacheExists(project, logstore string) bool {
	o.lock.RLock()                                                           // 读锁
	defer o.lock.RUnlock()                                                   // 解锁
	if lastUpdateTime, ok := o.logstoreCacheMap[project+"@@"+logstore]; ok { // 检查项目和日志存储是否在缓存中
		if time.Since(lastUpdateTime) < time.Second*time.Duration(*flags.LogResourceCacheExpireSec) { // 检查最后更新时间是否在缓存过期时间内
			return true
		}
	}
	return false
}

// addLogstoreCache 将给定的项目和日志存储添加到缓存中。
func (o *operationWrapper) addLogstoreCache(project, logstore string) {
	o.lock.Lock()                                          // 写锁
	defer o.lock.Unlock()                                  // 解锁
	o.logstoreCacheMap[project+"@@"+logstore] = time.Now() // 将项目和日志存储添加到缓存中
}

// configCacheExists 检查给定的项目和配置是否在缓存中存在，并且其最后更新时间距离现在的时间小于缓存过期时间。
func (o *operationWrapper) configCacheExists(project, config string) bool {
	o.lock.RLock()                                                       // 读锁
	defer o.lock.RUnlock()                                               // 解锁
	if lastUpdateTime, ok := o.configCacheMap[project+"@@"+config]; ok { // 检查项目和配置是否在缓存中
		if time.Since(lastUpdateTime) < time.Second*time.Duration(*flags.LogResourceCacheExpireSec) { // 检查最后更新时间是否在缓存过期时间内
			return true
		}
	}
	return false
}

// addConfigCache 将给定的项目和配置添加到缓存中。
func (o *operationWrapper) addConfigCache(project, config string) {
	o.lock.Lock()                                      // 写锁
	defer o.lock.Unlock()                              // 解锁
	o.configCacheMap[project+"@@"+config] = time.Now() // 将项目和配置添加到缓存中
}

// removeConfigCache 从缓存中删除给定的项目和配置。
func (o *operationWrapper) removeConfigCache(project, config string) {
	o.lock.Lock()                                 // 写锁
	defer o.lock.Unlock()                         // 解锁
	delete(o.configCacheMap, project+"@@"+config) // 从缓存中删除项目和配置
}

// retryCreateIndex 重试创建索引，如果索引已存在，则返回。
func (o *operationWrapper) retryCreateIndex(project, logstore, logstoremode string) {
	time.Sleep(time.Second * 10)                 // 等待10秒
	index := createDefaultK8SIndex(logstoremode) // 创建默认的K8S索引
	// 创建索引，创建索引不返回错误
	for i := 0; i < 10; i++ { // 重试10次
		err := o.logClient.CreateIndex(project, logstore, *index) // 创建索引
		if err != nil {
			// 如果索引已存在，就返回
			if clientError, ok := err.(*aliyunlog.Error); ok && clientError.Code == "IndexAlreadyExist" {
				return
			}
			time.Sleep(time.Second * 10) // 等待10秒
		} else {
			break
		}
	}
}

// createProductLogstore 是一个方法，用于创建产品日志存储
func (o *operationWrapper) createProductLogstore(config *AliyunLogConfigSpec, project, logstore, product, lang string, hotTTL int) error {
	// 记录开始创建产品日志存储的日志
	logger.Info(context.Background(), "begin to create product logstore, project", project, "logstore", logstore, "product", product, "lang", lang)
	// 调用 CreateProductLogstore 函数创建产品日志存储
	err := CreateProductLogstore(*flags.DefaultRegion, project, logstore, product, lang, hotTTL)

	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, product, config.LogtailConfig.ConfigName, false)

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
		// 发送正常的事件，表示产品日志创建成功
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateProductLogStore, "create product log success")
	}

	// 将日志存储添加到缓存中
	o.addLogstoreCache(project, logstore)
	// 返回 nil，表示没有错误
	return nil
}

// makesureLogstoreExist 是一个方法，用于确保日志存储存在
func (o *operationWrapper) makesureLogstoreExist(config *AliyunLogConfigSpec) error {
	// 获取项目名称
	project := o.project
	// 如果配置中的项目名称不为空，则使用配置中的项目名称
	if len(config.Project) != 0 {
		project = config.Project
	}
	// 获取日志存储名称
	logstore := config.Logstore
	// 初始化分片数量为 0
	shardCount := 0
	// 如果配置中的分片数量不为空，则使用配置中的分片数量
	if config.ShardCount != nil {
		shardCount = int(*config.ShardCount)
	}
	// 初始化生命周期为 0
	lifeCycle := 0
	// 如果配置中的生命周期不为空，则使用配置中的生命周期
	if config.LifeCycle != nil {
		lifeCycle = int(*config.LifeCycle)
	}
	// 初始化模式为 StandardMode
	mode := StandardMode
	// 如果配置中的日志存储模式不为空
	if len(config.LogstoreMode) != 0 {
		// 如果配置中的日志存储模式为 QueryMode，则将模式设置为 QueryMode
		if config.LogstoreMode == QueryMode {
			mode = QueryMode
		}
	}

	// 获取产品代码和产品语言
	product := config.ProductCode
	lang := config.ProductLang
	// 初始化 hotTTL 为 0
	hotTTL := 0
	// 如果配置中的 LogstoreHotTTL 不为空，则使用配置中的 LogstoreHotTTL
	if config.LogstoreHotTTL != nil {
		hotTTL = int(*config.LogstoreHotTTL)
	}

	// 如果日志存储已经存在于缓存中，则返回 nil
	if o.logstoreCacheExists(project, logstore) {
		return nil
	}

	// 如果产品代码长度大于 0
	if len(product) > 0 {
		// 如果产品语言长度为 0，则将产品语言设置为 "cn"
		if len(lang) == 0 {
			lang = "cn"
		}
		// 调用 createProductLogstore 方法创建产品日志存储
		return o.createProductLogstore(config, project, logstore, product, lang, hotTTL)
	}

	// 如果日志存储名称以 "audit-" 开头并且长度为 39
	if strings.HasPrefix(logstore, "audit-") && len(logstore) == 39 {
		// 调用 createProductLogstore 方法创建产品日志存储，产品为 "k8s-audit"，语言为 "cn"，hotTTL 为 0
		return o.createProductLogstore(config, project, logstore, "k8s-audit", "cn", 0)
	}

	// 如果项目名称不等于 o.project
	if project != o.project {
		// 确保项目存在
		if err := o.makesureProjectExist(config, project); err != nil {
			return err
		}
	}
	// 初始化 ok 为 false
	ok := false
	var err error
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 检查日志存储是否存在
		if ok, err = o.logClient.CheckLogstoreExist(project, logstore); err != nil {
			// 如果存在错误，则等待 100 毫秒
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误，则跳出循环
			break
		}
	}
	// 如果日志存储已经存在
	if ok {
		// 在后台记录日志，表示日志存储已经存在
		logger.Info(context.Background(), "logstore already exist, logstore", logstore)
		// 将日志存储添加到缓存中
		o.addLogstoreCache(project, logstore)
		// 返回 nil，表示没有错误
		return nil
	}
	// 设置默认的生命周期为 90 天
	ttl := 90
	// 如果生命周期大于 0，那么使用用户设置的生命周期
	if lifeCycle > 0 {
		ttl = lifeCycle
	}
	// 如果分片数量小于等于 0，那么设置默认的分片数量为 2
	if shardCount <= 0 {
		shardCount = 2
	}
	// 如果分片数量大于 10，那么设置分片数量为 10
	// 这是因为初始化的最大分片数量限制为 10
	if shardCount > 10 {
		shardCount = 10
	}
	// 创建一个新的日志存储对象
	logStore := &aliyunlog.LogStore{
		Name:          logstore,   // 日志存储的名称
		TTL:           ttl,        // 日志存储的生命周期
		ShardCount:    shardCount, // 分片数量
		AutoSplit:     true,       // 是否自动分片
		MaxSplitShard: 32,         // 最大分片数量
		Mode:          mode,       // 模式
	}
	// 如果设置了热存储的生命周期，那么设置日志存储的热存储生命周期
	if config.LogstoreHotTTL != nil {
		logStore.HotTTL = uint32(*config.LogstoreHotTTL)
	}
	// 如果设置了遥测类型，那么设置日志存储的遥测类型
	if len(config.LogstoreTelemetryType) > 0 {
		if MetricsTelemetryType == config.LogstoreTelemetryType {
			logStore.TelemetryType = config.LogstoreTelemetryType
		}
	}
	// 如果设置了追加元数据，那么设置日志存储的追加元数据
	if config.LogstoreAppendMeta {
		logStore.AppendMeta = config.LogstoreAppendMeta
	}
	// 如果设置了启用跟踪，那么设置日志存储的跟踪
	if config.LogstoreEnableTracking {
		logStore.WebTracking = config.LogstoreEnableTracking
	}
	// 如果设置了自动分片，那么设置日志存储的自动分片
	if config.LogstoreAutoSplit {
		logStore.AutoSplit = config.LogstoreAutoSplit
	}
	// 如果设置了最大分片数量，那么设置日志存储的最大分片数量
	if config.LogstoreMaxSplitShard != nil {
		logStore.MaxSplitShard = int(*config.LogstoreMaxSplitShard)
	}
	// 如果启用了加密，那么设置日志存储的加密配置
	if config.LogstoreEncryptConf.Enable {
		logStore.EncryptConf = &config.LogstoreEncryptConf
	}
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 尝试创建日志存储
		err = o.logClient.CreateLogStoreV2(project, logStore)
		// 如果创建失败，那么等待 100 毫秒后重试
		if err != nil {
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果创建成功，那么将日志存储添加到缓存中，并记录日志，然后退出循环
			o.addLogstoreCache(project, logstore)
			logger.Info(context.Background(), "create logstore success, logstore", logstore)
			break
		}
	}
	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, "", config.LogtailConfig.ConfigName, false)
	// 如果创建日志存储失败，那么发送错误事件
	if err != nil {
		if k8s_event.GetEventRecorder() != nil {
			customErr := CustomErrorFromSlsSDKError(err)
			k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.CreateLogstore, "", fmt.Sprintf("create logstore failed, error: %s", err.Error()))
		}
		// 返回错误
		return err
	} else if k8s_event.GetEventRecorder() != nil {
		// 如果创建日志存储成功，那么发送正常事件
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateLogstore, "create logstore success")
	}
	// 创建日志存储成功后，等待 1 秒
	time.Sleep(time.Second)
	// 使用默认的 k8s 索引
	index := createDefaultK8SIndex(mode)
	// 尝试最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 尝试创建索引
		err = o.logClient.CreateIndex(project, logstore, *index)
		// 如果创建失败，那么等待 100 毫秒后重试
		if err != nil {
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果创建成功，那么退出循环
			break
		}
	}
	// 如果创建索引成功，那么记录日志
	if err == nil {
		logger.Info(context.Background(), "create index done, logstore", logstore)
	} else {
		// 如果创建索引失败，那么记录警告日志
		logger.Warning(context.Background(), "CREATE_INDEX_ALARM", "create index done, logstore", logstore, "error", err)
	}
	return nil
}

// makesureProjectExist 函数确保在阿里云日志服务中存在指定的项目。
// 如果项目不存在，该函数将尝试创建它。
// config 参数是阿里云日志配置的规格，project 是要检查或创建的项目的名称。
func (o *operationWrapper) makesureProjectExist(config *AliyunLogConfigSpec, project string) error {
	ok := false   // 初始化 ok 为 false，表示项目是否存在的标志
	var err error // 初始化 err 为 nil，用于存储错误信息

	// 尝试最大重试次数来检查项目是否存在
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CheckProjectExist 方法检查项目是否存在
		if ok, err = o.logClient.CheckProjectExist(project); err != nil {
			// 如果出现错误，暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 1000)
		} else {
			// 如果没有错误，跳出循环
			break
		}
	}
	// 如果项目存在，返回 nil
	if ok {
		return nil
	}
	// 如果项目不存在，尝试最大重试次数来创建项目
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CreateProject 方法创建项目
		_, err = o.logClient.CreateProject(project, "k8s log project, created by alibaba cloud log controller")
		if err != nil {
			// 如果出现错误，暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 1000)
		} else {
			// 如果没有错误，跳出循环
			break
		}
	}
	// 初始化配置名称和日志存储变量
	configName := ""
	logstore := ""
	// 如果配置不为 nil，获取配置名称和日志存储
	if config != nil {
		configName = config.LogtailConfig.ConfigName
		logstore = config.Logstore
	}
	// 获取注解
	annotations := GetAnnotationByObject(config, project, logstore, "", configName, false)
	// 如果创建项目过程中出现错误，发送错误事件
	if err != nil {
		if k8s_event.GetEventRecorder() != nil {
			customErr := CustomErrorFromSlsSDKError(err)
			k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.CreateProject, "", fmt.Sprintf("create project failed, error: %s", err.Error()))
		}
	} else if k8s_event.GetEventRecorder() != nil {
		// 如果创建项目成功，发送正常事件
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateProject, "create project success")
	}
	// 返回错误信息
	return err
}

// makesureMachineGroupExist 函数确保在阿里云日志服务中存在指定的机器组。
// 如果机器组不存在，该函数将尝试创建它。
// project 是项目的名称，machineGroup 是要检查或创建的机器组的名称。
func (o *operationWrapper) makesureMachineGroupExist(project, machineGroup string) error {
	ok := false   // 初始化 ok 为 false，表示机器组是否存在的标志
	var err error // 初始化 err 为 nil，用于存储错误信息

	// 尝试最大重试次数来检查机器组是否存在
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CheckMachineGroupExist 方法检查机器组是否存在
		if ok, err = o.logClient.CheckMachineGroupExist(project, machineGroup); err != nil {
			// 如果出现错误，暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误，跳出循环
			break
		}
	}
	// 如果机器组存在，返回 nil
	if ok {
		return nil
	}
	// 如果机器组不存在，创建一个新的机器组
	m := &aliyunlog.MachineGroup{
		Name:          machineGroup,
		MachineIDType: aliyunlog.MachineIDTypeUserDefined,
		MachineIDList: []string{machineGroup},
	}
	// 尝试最大重试次数来创建机器组
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 CreateMachineGroup 方法创建机器组
		err = o.logClient.CreateMachineGroup(project, m)
		if err != nil {
			// 如果出现错误，暂停一秒后再次尝试
			time.Sleep(time.Millisecond * 100)
		} else {
			// 如果没有错误，尝试最大重试次数来标记机器组
			for j := 0; j < *flags.LogOperationMaxRetryTimes; j++ {
				// 调用 TagMachineGroup 方法标记机器组
				err = o.TagMachineGroup(project, machineGroup, SlsMachinegroupDeployModeKey, SlsMachinegroupDeployModeDeamonset)
				if err != nil {
					// 如果出现错误，暂停一秒后再次尝试
					time.Sleep(time.Millisecond * 100)
				} else {
					// 如果没有错误，跳出循环
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
// resetToken 函数用于重置访问令牌
func (o *operationWrapper) resetToken(accessKeyID, accessKeySecret, stsToken string) {
	// 调用 logClient 的 ResetAccessKeyToken 方法来重置访问令牌
	o.logClient.ResetAccessKeyToken(accessKeyID, accessKeySecret, stsToken)
}

// nolint:unused
// deleteConfig 函数用于删除配置
func (o *operationWrapper) deleteConfig(checkpoint *configCheckPoint) error {
	var err error
	// 循环尝试删除配置，最大尝试次数为 LogOperationMaxRetryTimes
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用 logClient 的 DeleteConfig 方法来删除配置
		err = o.logClient.DeleteConfig(checkpoint.ProjectName, checkpoint.ConfigName)
		// 如果没有错误，那么从缓存中移除配置并返回 nil
		if err == nil {
			o.removeConfigCache(checkpoint.ProjectName, checkpoint.ConfigName)
			return nil
		}
		// 如果错误是 aliyunlog.Error 类型
		if sdkError, ok := err.(*aliyunlog.Error); ok {
			// 如果 HTTP 状态码是 404，那么从缓存中移除配置并返回 nil
			if sdkError.HTTPCode == 404 {
				o.removeConfigCache(checkpoint.ProjectName, checkpoint.ConfigName)
				return nil
			}
		}
		// 等待 100 毫秒
		time.Sleep(time.Millisecond * 100)
	}
	// 如果有错误，那么返回错误信息
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

// nolint:unused
// createConfig 函数用于创建配置
func (o *operationWrapper) createConfig(config *AliyunLogConfigSpec) error {
	// 调用 updateConfigInner 方法来创建配置
	return o.updateConfigInner(config)
}

// checkFileConfigChanged 函数用于检查文件配置是否发生了变化
func checkFileConfigChanged(filePath, filePattern, includeEnv, includeLabel string, serverConfigDetail map[string]interface{}) bool {
	// 从 serverConfigDetail 中获取各项配置信息
	serverFilePath, _ := util.InterfaceToString(serverConfigDetail["logPath"])
	serverFilePattern, _ := util.InterfaceToString(serverConfigDetail["filePattern"])
	serverIncludeEnv, _ := util.InterfaceToJSONString(serverConfigDetail["dockerIncludeEnv"])
	serverIncludeLabel, _ := util.InterfaceToJSONString(serverConfigDetail["dockerIncludeLabel"])
	// 检查各项配置是否发生了变化
	return filePath != serverFilePath ||
		filePattern != serverFilePattern ||
		includeEnv != serverIncludeEnv ||
		includeLabel != serverIncludeLabel
}

// operationWrapper 结构体的 UnTagLogtailConfig 方法，用于移除 Logtail 配置的标签
func (o *operationWrapper) UnTagLogtailConfig(project string, logtailConfig string) error {
	var err error // 定义一个错误变量

	// "github.com/aliyun/aliyun-log-go-sdk" 不支持一次性移除所有标签，我们需要先列出所有标签
	var ResourceTags []*aliyunlog.ResourceTagResponse
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		// 列出所有标签资源
		ResourceTags, _, err = o.logClient.ListTagResources(project, TagLogtailConfig, []string{project + "#" + logtailConfig}, []aliyunlog.ResourceFilterTag{}, "")
		if err == nil { // 如果没有错误，跳出循环
			break
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	if err != nil { // 如果有错误，返回错误
		return err
	}

	// 定义一个用于移除标签的资源
	ResourceUnTags := aliyunlog.ResourceUnTags{ResourceType: TagLogtailConfig,
		ResourceID: []string{project + "#" + logtailConfig},
		Tags:       []string{},
	}
	for _, tags := range ResourceTags { // 遍历所有标签
		ResourceUnTags.Tags = append(ResourceUnTags.Tags, tags.TagKey) // 添加到移除标签的资源中
	}
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		err = o.logClient.UnTagResources(project, &ResourceUnTags) // 移除标签
		if err == nil {                                            // 如果没有错误，返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// operationWrapper 结构体的 TagLogtailConfig 方法，用于给 Logtail 配置添加标签
func (o *operationWrapper) TagLogtailConfig(project string, logtailConfig string, tags map[string]string) error {
	var err error // 定义一个错误变量

	// 在创建之前删除所有标签
	err = o.UnTagLogtailConfig(project, logtailConfig)
	if err != nil { // 如果有错误，返回错误
		return err
	}

	// 定义一个用于添加标签的资源
	ResourceTags := aliyunlog.ResourceTags{ResourceType: TagLogtailConfig,
		ResourceID: []string{project + "#" + logtailConfig},
		Tags:       []aliyunlog.ResourceTag{},
	}
	for k, v := range tags { // 遍历所有标签
		tag := aliyunlog.ResourceTag{Key: k, Value: v}     // 创建一个新的标签
		ResourceTags.Tags = append(ResourceTags.Tags, tag) // 添加到资源中
	}

	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		err = o.logClient.TagResources(project, &ResourceTags) // 添加标签
		if err == nil {                                        // 如果没有错误，返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// operationWrapper 结构体的 TagMachineGroup 方法，用于给机器组添加标签
func (o *operationWrapper) TagMachineGroup(project, machineGroup, tagKey, tagValue string) error {
	// 定义一个用于添加标签的资源
	ResourceTags := aliyunlog.ResourceTags{
		ResourceType: TagMachinegroup,
		ResourceID:   []string{project + "#" + machineGroup},
		Tags:         []aliyunlog.ResourceTag{{Key: tagKey, Value: tagValue}},
	}
	var err error                                           // 定义一个错误变量
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ { // 最大重试次数
		err = o.logClient.TagResources(project, &ResourceTags) // 添加标签
		if err == nil {                                        // 如果没有错误，返回 nil
			return nil
		}
		time.Sleep(time.Millisecond * 100) // 等待100毫秒
	}
	return err // 返回错误
}

// nolint:govet,ineffassign
// 定义一个名为updateConfigInner的方法，该方法接收一个AliyunLogConfigSpec类型的指针作为参数，并返回一个错误类型的值
func (o *operationWrapper) updateConfigInner(config *AliyunLogConfigSpec) error {
	// 获取operationWrapper的project属性
	project := o.project
	// 如果config的Project属性的长度不为0，即Project属性不为空，则将其值赋给project
	if len(config.Project) != 0 {
		project = config.Project
	}
	// 获取config的Logstore属性
	logstore := config.Logstore
	// 调用makesureLogstoreExist方法，确保日志存储存在
	err := o.makesureLogstoreExist(config)
	// 如果上述方法返回错误，则返回一个新的错误，错误信息包含config的LogtailConfig的ConfigName属性和错误信息
	if err != nil {
		return fmt.Errorf("Create logconfig error when update config, config : %s, error : %s", config.LogtailConfig.ConfigName, err.Error())
	}

	// 获取config的LogtailConfig的InputType属性
	inputType := config.LogtailConfig.InputType
	// 如果inputType的长度为0，即inputType为空，则将aliyunlog.InputTypeFile赋给inputType
	if len(inputType) == 0 {
		inputType = aliyunlog.InputTypeFile
	}
	// 创建一个新的aliyunlog.LogConfig类型的指针，并初始化其属性
	aliyunLogConfig := &aliyunlog.LogConfig{
		Name:        config.LogtailConfig.ConfigName,                                  // Name属性为config的LogtailConfig的ConfigName属性
		InputDetail: addNecessaryInputConfigField(config.LogtailConfig.LogtailConfig), // InputDetail属性为调用addNecessaryInputConfigField方法返回的值
		InputType:   inputType,                                                        // InputType属性为inputType
		OutputType:  aliyunlog.OutputTypeLogService,                                   // OutputType属性为aliyunlog.OutputTypeLogService
		OutputDetail: aliyunlog.OutputDetail{ // OutputDetail属性为一个新的aliyunlog.OutputDetail类型的值
			ProjectName:  project,  // ProjectName属性为project
			LogStoreName: logstore, // LogStoreName属性为logstore
		},
	}

	// 使用logger的Info方法，记录一条信息，信息内容为"create or update config"，并附带config的LogtailConfig的ConfigName属性和aliyunLogConfig的详细信息
	logger.Info(context.Background(), "create or update config", config.LogtailConfig.ConfigName, "detail", *aliyunLogConfig)

	// 初始化一个布尔变量ok为false
	ok := false
	// if o.configCacheExists(project, config.LogtailConfig.ConfigName) {
	// 	ok = true
	// } else {
	// 	// 检查配置是否存在
	// 	for i := 0; i < *LogOperationMaxRetryTimes; i++ {
	// 		ok, err = o.logClient.CheckConfigExist(project, config.LogtailConfig.ConfigName)
	// 		if err == nil {
	// 			break
	// 		}
	// 	}
	// }

	// 初始化一个指向aliyunlog.LogConfig的指针变量serverConfig
	var serverConfig *aliyunlog.LogConfig
	// 循环尝试获取配置，直到成功或达到最大重试次数
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用logClient的GetConfig方法获取配置
		serverConfig, err = o.logClient.GetConfig(project, config.LogtailConfig.ConfigName)
		// 如果获取配置出错
		if err != nil {
			// 判断错误是否为aliyunlog.Error类型
			if slsErr, ok := err.(*aliyunlog.Error); ok {
				// 如果错误代码为"ConfigNotExist"，则设置ok为false并跳出循环
				if slsErr.Code == "ConfigNotExist" {
					ok = false
					break
				}
			}
		} else {
			// 如果获取配置成功，则设置ok为true并跳出循环
			ok = true
			break
		}
	}

	// 记录日志，输出获取配置的结果
	logger.Info(context.Background(), "get config", config.LogtailConfig.ConfigName, "result", ok)

	// 创建配置或更新配置
	// 如果获取配置成功并且serverConfig不为nil
	if ok && serverConfig != nil {
		// 如果配置为简单配置
		if config.SimpleConfig {
			// 将serverConfig.InputDetail转换为map[string]interface{}类型
			configDetail, ok := serverConfig.InputDetail.(map[string]interface{})
			// 只更新文件类型的filePattern和logPath
			// 如果配置的输入类型为"file"，并且serverConfig的输入类型也为"file"，并且转换类型成功
			if config.LogtailConfig.InputType == "file" && serverConfig.InputType == "file" && ok {
				// 获取filePattern、logPath、includeEnv、includeLabel的值
				filePattern, _ := util.InterfaceToString(config.LogtailConfig.LogtailConfig["filePattern"])
				logPath, _ := util.InterfaceToString(config.LogtailConfig.LogtailConfig["logPath"])
				includeEnv, _ := util.InterfaceToJSONString(config.LogtailConfig.LogtailConfig["dockerIncludeEnv"])
				includeLabel, _ := util.InterfaceToJSONString(config.LogtailConfig.LogtailConfig["dockerIncludeLabel"])

				// 如果filePattern和logPath的长度都大于0
				if len(filePattern) > 0 && len(logPath) > 0 {

					// 如果文件配置发生了变化
					if checkFileConfigChanged(logPath, filePattern, includeEnv, includeLabel, configDetail) {
						// 记录日志，输出旧的配置和新的配置
						logger.Info(context.Background(), "file config changed, old", configDetail, "new", config.LogtailConfig.LogtailConfig)
						// 更新配置
						configDetail["logPath"] = logPath
						configDetail["filePattern"] = filePattern
						configDetail["dockerIncludeEnv"] = config.LogtailConfig.LogtailConfig["dockerIncludeEnv"]
						// 如果configDetail中包含"dockerIncludeLabel"键
						if _, hasLabel := configDetail["dockerIncludeLabel"]; hasLabel {
							// 如果config.LogtailConfig.LogtailConfig中也包含"dockerIncludeLabel"键
							if _, hasLocalLabel := config.LogtailConfig.LogtailConfig["dockerIncludeLabel"]; hasLocalLabel {
								// 更新"dockerIncludeLabel"的值
								configDetail["dockerIncludeLabel"] = config.LogtailConfig.LogtailConfig["dockerIncludeLabel"]
							}
						}
						// 更新serverConfig.InputDetail的值
						serverConfig.InputDetail = configDetail
						// 更新配置
						// 循环尝试更新配置，直到成功或达到最大重试次数
						for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
							err = o.logClient.UpdateConfig(project, serverConfig)
							if err == nil {
								break
							}
						}

						// 获取注解
						annotations := GetAnnotationByObject(config, project, logstore, "", config.LogtailConfig.ConfigName, true)
						// 如果更新配置出错
						if err != nil {
							// 如果事件记录器存在
							if k8s_event.GetEventRecorder() != nil {
								// 获取自定义错误
								customErr := CustomErrorFromSlsSDKError(err)
								// 发送错误事件
								k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.UpdateConfig, "", fmt.Sprintf("update config failed, error: %s", err.Error()))
							}
						} else {
							// 如果事件记录器存在
							if k8s_event.GetEventRecorder() != nil {
								// 发送正常事件
								k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.UpdateConfig, "update config success")
							}
						}

					} else {
						// 如果文件配置没有发生变化，记录日志，跳过更新
						logger.Info(context.Background(), "file config not changed", "skip update")
					}
				}
			}
			// 如果配置的输入类型和serverConfig的输入类型不一致，并且转换类型成功
			// 如果当前配置的输入类型与服务器配置的输入类型不同，并且类型转换成功
			if config.LogtailConfig.InputType != serverConfig.InputType && ok {
				// 强制更新
				// 记录日志，输出配置输入类型的变化
				// 记录一条信息级别的日志，表示配置输入类型的变化，并强制更新
				logger.Info(context.Background(), "config input type change from", serverConfig.InputType,
					"to", config.LogtailConfig.InputType, "force update")
				// update config
				// 更新配置
				for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
					// 调用阿里云日志服务的客户端更新配置
					err = o.logClient.UpdateConfig(project, aliyunLogConfig)
					// 如果没有错误，跳出循环
					if err == nil {
						break
					}
				}
			}
			// 记录一条信息级别的日志，表示配置已更新
			logger.Info(context.Background(), "config updated, server config", *serverConfig, "local config", *config)
		}

	} else {
		// 如果配置不存在
		for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
			// 创建新的配置
			err = o.logClient.CreateConfig(project, aliyunLogConfig)
			// 如果没有错误，跳出循环
			if err == nil {
				break
			}
		}
		// 获取注解
		annotations := GetAnnotationByObject(config, project, logstore, "", config.LogtailConfig.ConfigName, true)
		// 如果有错误
		if err != nil {
			// 如果事件记录器存在
			if k8s_event.GetEventRecorder() != nil {
				// 从SlsSDKError创建自定义错误
				customErr := CustomErrorFromSlsSDKError(err)
				// 发送带有注解的错误事件
				k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, customErr), k8s_event.UpdateConfig, "", fmt.Sprintf("update config failed, error: %s", err.Error()))
			}
		} else if k8s_event.GetEventRecorder() != nil {
			// 如果事件记录器存在，并且没有错误，发送带有注解的正常事件
			k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.UpdateConfig, "update config success")
		}
	}
	// 如果有错误，返回错误
	if err != nil {
		return fmt.Errorf("UpdateConfig error, config : %s, error : %s", config.LogtailConfig.ConfigName, err.Error())
	}
	// 记录一条信息级别的日志，表示创建或更新配置成功
	logger.Info(context.Background(), "create or update config success", config.LogtailConfig.ConfigName) // Tag config

	// Tag config
	// 创建一个空的map，用于存储日志配置的标签
	logtailConfigTags := map[string]string{}
	// 遍历配置中的标签，将它们添加到新创建的map中
	for k, v := range config.ConfigTags {
		logtailConfigTags[k] = v
	}
	// 添加一个特定的标签到map中
	logtailConfigTags[SlsLogtailChannalKey] = SlsLogtailChannalEnv
	// 调用TagLogtailConfig函数，为日志配置添加标签
	err = o.TagLogtailConfig(project, config.LogtailConfig.ConfigName, logtailConfigTags)
	// 调用GetAnnotationByObject函数，获取注解
	annotations := GetAnnotationByObject(config, project, logstore, "", config.LogtailConfig.ConfigName, true)
	// 检查是否有错误
	if err != nil {
		// 如果有错误，发送一个错误事件
		k8s_event.GetEventRecorder().SendErrorEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), GetAnnotationByError(annotations, CustomErrorFromSlsSDKError(err)), k8s_event.CreateTag, "", fmt.Sprintf("tag config %s error :%s", config.LogtailConfig.ConfigName, err.Error()))
	} else {
		// 如果没有错误，发送一个正常事件
		k8s_event.GetEventRecorder().SendNormalEventWithAnnotation(k8s_event.GetEventRecorder().GetObject(), annotations, k8s_event.CreateTag, fmt.Sprintf("tag config %s success", config.LogtailConfig.ConfigName))
	}

	// 检查配置是否在机器组中
	// 只在创建配置时检查
	var machineGroup string
	// 如果配置中有机器组，取第一个机器组
	if len(config.MachineGroups) > 0 {
		machineGroup = config.MachineGroups[0]
	} else {
		// 如果配置中没有机器组，使用默认的机器组
		machineGroup = *flags.DefaultLogMachineGroup
	}
	// 确保机器组存在
	_ = o.makesureMachineGroupExist(project, machineGroup)
	// 如果ok为true，执行以下操作
	if ok {
		var machineGroups []string
		// 重试获取应用的机器组
		for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
			machineGroups, err = o.logClient.GetAppliedMachineGroups(project, config.LogtailConfig.ConfigName)
			// 如果没有错误，跳出循环
			if err == nil {
				break
			}
		}
		// 如果有错误，返回错误
		if err != nil {
			return fmt.Errorf("GetAppliedMachineGroups error, config : %s, error : %s", config.LogtailConfig.ConfigName, err.Error())
		}
		// 设置ok为false
		ok = false
		// 遍历机器组
		for _, key := range machineGroups {
			// 如果找到了匹配的机器组，设置ok为true并跳出循环
			if key == machineGroup {
				ok = true
				break
			}
		}
	}
	// 将配置应用到机器组
	for i := 0; i < *flags.LogOperationMaxRetryTimes; i++ {
		// 调用ApplyConfigToMachineGroup函数，将配置应用到机器组
		err = o.logClient.ApplyConfigToMachineGroup(project, config.LogtailConfig.ConfigName, machineGroup)
		// 如果没有错误，跳出循环
		if err == nil {
			break
		}
	}
	// 如果有错误，返回错误
	if err != nil {
		return fmt.Errorf("ApplyConfigToMachineGroup error, config : %s, machine group : %s, error : %s", config.LogtailConfig.ConfigName, machineGroup, err.Error())
	}
	// 记录一条信息日志，表示配置已成功应用到机器组
	logger.Info(context.Background(), "apply config to machine group success", config.LogtailConfig.ConfigName, "group", machineGroup)
	// 将配置添加到缓存中
	o.addConfigCache(project, config.LogtailConfig.ConfigName)
	// 返回nil，表示没有错误
	return nil
}
