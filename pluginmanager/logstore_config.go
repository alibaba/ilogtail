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

package pluginmanager

import (
	"bytes"
	"context"
	"crypto/md5" //nolint:gosec
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"

	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/input"
)

var maxFlushOutTime = 5
var embeddedNamingCnt = int64(0)

const mixProcessModeFlag = "mix_process_mode"

type mixProcessMode int

const (
	normal mixProcessMode = iota
	file
	observer
)

// checkMixProcessMode
// When inputs plugins not exist, it means this LogConfig is a mixed process mode config.
// And the default mix process mode is the file mode.
func checkMixProcessMode(pluginCfg map[string]interface{}) mixProcessMode {
	config, exists := pluginCfg["inputs"]
	inputs, ok := config.([]interface{})
	if exists && ok && len(inputs) > 0 {
		return normal
	}
	mixModeFlag, mixModeFlagOk := pluginCfg[mixProcessModeFlag]
	if !mixModeFlagOk {
		return file
	}
	s := mixModeFlag.(string)
	switch {
	case strings.EqualFold(s, "observer"):
		return observer
	default:
		return file
	}
}

type LogstoreStatistics struct {
	CollecLatencytMetric pipeline.LatencyMetric
	RawLogMetric         pipeline.CounterMetric
	SplitLogMetric       pipeline.CounterMetric
	FlushLogMetric       pipeline.CounterMetric
	FlushLogGroupMetric  pipeline.CounterMetric
	FlushReadyMetric     pipeline.CounterMetric
	FlushLatencyMetric   pipeline.LatencyMetric
}

type ConfigVersion string

var (
	v1 ConfigVersion = "v1"
	v2 ConfigVersion = "v2"
)

type LogstoreConfig struct {
	// common fields
	ProjectName  string
	LogstoreName string
	ConfigName   string
	LogstoreKey  int64
	FlushOutFlag bool
	// Each LogstoreConfig can have its independent GlobalConfig if the "global" field
	//   is offered in configuration, see build-in StatisticsConfig and AlarmConfig.
	GlobalConfig *config.GlobalConfig

	Version      ConfigVersion
	Context      pipeline.Context
	Statistics   LogstoreStatistics
	PluginRunner PluginRunner
	// private fields
	alreadyStarted   bool // if this flag is true, do not start it when config Resume
	configDetailHash string
	// processShutdown  chan struct{}
	// flushShutdown    chan struct{}
	pauseChan  chan struct{}
	resumeChan chan struct{}
	// processWaitSema  sync.WaitGroup
	// flushWaitSema    sync.WaitGroup
	pauseOrResumeWg sync.WaitGroup

	K8sLabelSet           map[string]struct{}
	ContainerLabelSet     map[string]struct{}
	EnvSet                map[string]struct{}
	CollectContainersFlag bool
}

func (p *LogstoreStatistics) Init(context pipeline.Context) {
	p.CollecLatencytMetric = helper.NewLatencyMetric("collect_latency")
	p.RawLogMetric = helper.NewCounterMetric("raw_log")
	p.SplitLogMetric = helper.NewCounterMetric("processed_log")
	p.FlushLogMetric = helper.NewCounterMetric("flush_log")
	p.FlushLogGroupMetric = helper.NewCounterMetric("flush_loggroup")
	p.FlushReadyMetric = helper.NewAverageMetric("flush_ready")
	p.FlushLatencyMetric = helper.NewLatencyMetric("flush_latency")

	context.RegisterLatencyMetric(p.CollecLatencytMetric)
	context.RegisterCounterMetric(p.RawLogMetric)
	context.RegisterCounterMetric(p.SplitLogMetric)
	context.RegisterCounterMetric(p.FlushLogMetric)
	context.RegisterCounterMetric(p.FlushLogGroupMetric)
	context.RegisterCounterMetric(p.FlushReadyMetric)
	context.RegisterLatencyMetric(p.FlushLatencyMetric)
}

// Start initializes plugin instances in config and starts them.
// Procedures:
//  1. Start flusher goroutine and push FlushOutLogGroups inherited from last config
//     instance to LogGroupsChan, so that they can be flushed to flushers.
//  2. Start aggregators, allocate new goroutine for each one.
//  3. Start processor goroutine to process logs from LogsChan.
//  4. Start inputs (including metrics and services), just like aggregator, each input
//     has its own goroutine.
func (lc *LogstoreConfig) Start() {
	lc.FlushOutFlag = false
	logger.Info(lc.Context.GetRuntimeContext(), "config start", "begin")

	lc.pauseChan = make(chan struct{}, 1)
	lc.resumeChan = make(chan struct{}, 1)

	lc.PluginRunner.Run()

	logger.Info(lc.Context.GetRuntimeContext(), "config start", "success")
}

// Stop stops plugin instances and corresponding goroutines of config.
// @exitFlag passed from Logtail, indicates that if Logtail will quit after this.
// Procedures:
// 1. SetUrgent to all flushers to indicate them current state.
// 2. Stop all input plugins, stop generating logs.
// 3. Stop processor goroutine, pass all existing logs to aggregator.
// 4. Stop all aggregator plugins, make all logs to LogGroups.
// 5. Set stopping flag, stop flusher goroutine.
// 6. If Logtail is exiting and there are remaining data, try to flush once.
// 7. Stop flusher plugins.
func (lc *LogstoreConfig) Stop(exitFlag bool) error {
	logger.Info(lc.Context.GetRuntimeContext(), "config stop", "begin", "exit", exitFlag)
	if err := lc.PluginRunner.Stop(exitFlag); err != nil {
		return err
	}
	logger.Info(lc.Context.GetRuntimeContext(), "Plugin Runner stop", "done")
	close(lc.pauseChan)
	close(lc.resumeChan)
	logger.Info(lc.Context.GetRuntimeContext(), "config stop", "success")
	return nil
}

func (lc *LogstoreConfig) pause() {
	lc.pauseOrResumeWg.Add(1)
	lc.pauseChan <- struct{}{}
	lc.pauseOrResumeWg.Wait()
}

func (lc *LogstoreConfig) waitForResume() {
	lc.pauseOrResumeWg.Done()
	select {
	case <-lc.resumeChan:
		lc.pauseOrResumeWg.Done()
	case <-GetFlushCancelToken(lc.PluginRunner):
	}
}

func (lc *LogstoreConfig) resume() {
	lc.pauseOrResumeWg.Add(1)
	lc.resumeChan <- struct{}{}
	lc.pauseOrResumeWg.Wait()
}

const (
	rawStringKey     = "content"
	defaultTagPrefix = "__tag__:__prefix__"
)

var (
	tagDelimiter = []byte("^^^")
	tagSeparator = []byte("~=~")
)

func (lc *LogstoreConfig) ProcessRawLog(rawLog []byte, packID string, topic string) int {
	log := &protocol.Log{}
	log.Contents = append(log.Contents, &protocol.Log_Content{Key: rawStringKey, Value: string(rawLog)})
	logger.Debug(context.Background(), "Process raw log ", packID, topic, len(rawLog))
	lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic}})
	return 0
}

// extractTags extracts tags from rawTags and append them into log.
// Rule: k1~=~v1^^^k2~=~v2
// rawTags
func extractTags(rawTags []byte, log *protocol.Log) {
	defaultPrefixIndex := 0
	for len(rawTags) != 0 {
		idx := bytes.Index(rawTags, tagDelimiter)
		var part []byte
		if idx < 0 {
			part = rawTags
			rawTags = rawTags[len(rawTags):]
		} else {
			part = rawTags[:idx]
			rawTags = rawTags[idx+len(tagDelimiter):]
		}
		if len(part) > 0 {
			pos := bytes.Index(part, tagSeparator)
			if pos > 0 {
				log.Contents = append(log.Contents, &protocol.Log_Content{
					Key:   string(part[:pos]),
					Value: string(part[pos+len(tagSeparator):]),
				})
			} else {
				log.Contents = append(log.Contents, &protocol.Log_Content{
					Key:   defaultTagPrefix + strconv.Itoa(defaultPrefixIndex),
					Value: string(part),
				})
			}
			defaultPrefixIndex++
		}
	}
}

// extractTagsToLogTags extracts tags from rawTags and append them into []*protocol.LogTag.
// Rule: k1~=~v1^^^k2~=~v2
// rawTags
func extractTagsToLogTags(rawTags []byte) []*protocol.LogTag {
	logTags := []*protocol.LogTag{}
	defaultPrefixIndex := 0
	for len(rawTags) != 0 {
		idx := bytes.Index(rawTags, tagDelimiter)
		var part []byte
		if idx < 0 {
			part = rawTags
			rawTags = rawTags[len(rawTags):]
		} else {
			part = rawTags[:idx]
			rawTags = rawTags[idx+len(tagDelimiter):]
		}
		if len(part) > 0 {
			pos := bytes.Index(part, tagSeparator)
			if pos > 0 {
				logTags = append(logTags, &protocol.LogTag{
					Key:   string(part[:pos]),
					Value: string(part[pos+len(tagSeparator):]),
				})
			} else {
				logTags = append(logTags, &protocol.LogTag{
					Key:   defaultTagPrefix + strconv.Itoa(defaultPrefixIndex),
					Value: string(part),
				})
			}
			defaultPrefixIndex++
		}
	}
	return logTags
}

// ProcessRawLogV2 ...
// V1 -> V2: enable topic field, and use tags field to pass more tags.
// unsafe parameter: rawLog,packID and tags
// safe parameter:  topic
func (lc *LogstoreConfig) ProcessRawLogV2(rawLog []byte, packID string, topic string, tags []byte) int {
	log := &protocol.Log{
		Contents: make([]*protocol.Log_Content, 0, 16),
	}
	log.Contents = append(log.Contents, &protocol.Log_Content{Key: rawStringKey, Value: string(rawLog)})
	if len(topic) > 0 {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__log_topic__", Value: topic})
	}
	// When UsingOldContentTag is set to false, the tag is now put into the context during cgo.
	if !lc.GlobalConfig.UsingOldContentTag {
		logTags := extractTagsToLogTags(tags)
		lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic, "tags": logTags}})
	} else {
		extractTags(tags, log)
		lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic}})
	}
	return 0
}

func (lc *LogstoreConfig) ProcessLog(logByte []byte, packID string, topic string, tags []byte) int {
	log := &protocol.Log{}
	err := log.Unmarshal(logByte)
	if err != nil {
		logger.Error(lc.Context.GetRuntimeContext(), "WRONG_PROTOBUF_ALARM",
			"cannot process logs passed by core, err", err)
		return -1
	}
	if len(topic) > 0 {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__log_topic__", Value: topic})
	}
	// When UsingOldContentTag is set to false, the tag is now put into the context during cgo.
	if !lc.GlobalConfig.UsingOldContentTag {
		logTags := extractTagsToLogTags(tags)
		lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic, "tags": logTags}})
	} else {
		extractTags(tags, log)
		lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic}})
	}
	return 0
}

func (lc *LogstoreConfig) ProcessLogGroup(logByte []byte, packID string) int {
	logGroup := &protocol.LogGroup{}
	err := logGroup.Unmarshal(logByte)
	if err != nil {
		logger.Error(lc.Context.GetRuntimeContext(), "WRONG_PROTOBUF_ALARM",
			"cannot process log group passed by core, err", err)
		return -1
	}
	topic := logGroup.GetTopic()
	// TODO: use v2 pipeline
	for _, log := range logGroup.Logs {
		if len(topic) > 0 {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__log_topic__", Value: topic})
		}
		// When UsingOldContentTag is set to false, the tag is now put into the context during cgo.
		if !lc.GlobalConfig.UsingOldContentTag {
			lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic, "tags": logGroup.LogTags}})
		} else {
			for _, tag := range logGroup.LogTags {
				log.Contents = append(log.Contents, &protocol.Log_Content{
					Key:   tagPrefix + tag.GetKey(),
					Value: tag.GetValue(),
				})
			}
			lc.PluginRunner.ReceiveRawLog(&pipeline.LogWithContext{Log: log, Context: map[string]interface{}{"source": packID, "topic": topic}})
		}
	}
	return 0
}

func hasDockerStdoutInput(plugins map[string]interface{}) bool {
	inputs, exists := plugins["inputs"]
	if !exists {
		return false
	}

	inputList, valid := inputs.([]interface{})
	if !valid {
		return false
	}

	for _, detail := range inputList {
		cfg, valid := detail.(map[string]interface{})
		if !valid {
			continue
		}
		typeName, valid := cfg["type"]
		if !valid {
			continue
		}
		if val, valid := typeName.(string); valid && val == input.ServiceDockerStdoutPluginName {
			return true
		}
	}
	return false
}

var enableAlwaysOnlineForStdout = true

func createLogstoreConfig(project string, logstore string, configName string, logstoreKey int64, jsonStr string) (*LogstoreConfig, error) {
	var err error
	contextImp := &ContextImp{}
	contextImp.InitContext(project, logstore, configName)
	logstoreC := &LogstoreConfig{
		ProjectName:      project,
		LogstoreName:     logstore,
		ConfigName:       configName,
		LogstoreKey:      logstoreKey,
		Context:          contextImp,
		configDetailHash: fmt.Sprintf("%x", md5.Sum([]byte(jsonStr))), //nolint:gosec
	}
	contextImp.logstoreC = logstoreC

	// Check if the config has been disabled (keep disabled if config detail is unchanged).
	DisabledLogtailConfigLock.Lock()
	if disabledConfig, hasDisabled := DisabledLogtailConfig[configName]; hasDisabled {
		if disabledConfig.configDetailHash == logstoreC.configDetailHash {
			DisabledLogtailConfigLock.Unlock()
			return nil, fmt.Errorf("failed to create config because timeout "+
				"stop has happened on it: %v", configName)
		}
		delete(DisabledLogtailConfig, configName)
		DisabledLogtailConfigLock.Unlock()
		logger.Info(contextImp.GetRuntimeContext(), "retry timeout config because config detail has changed")
	} else {
		DisabledLogtailConfigLock.Unlock()
	}

	var plugins = make(map[string]interface{})
	if err = json.Unmarshal([]byte(jsonStr), &plugins); err != nil {
		return nil, err
	}

	logstoreC.Version = fetchPluginVersion(plugins)
	if logstoreC.PluginRunner, err = initPluginRunner(logstoreC); err != nil {
		return nil, err
	}

	// check AlwaysOnlineManager
	if oldConfig, ok := GetAlwaysOnlineManager().GetCachedConfig(configName); ok {
		logger.Info(contextImp.GetRuntimeContext(), "find alwaysOnline config", oldConfig.ConfigName, "config compare", oldConfig.configDetailHash == logstoreC.configDetailHash,
			"new config hash", logstoreC.configDetailHash, "old config hash", oldConfig.configDetailHash)
		if oldConfig.configDetailHash == logstoreC.configDetailHash {
			logstoreC = oldConfig
			logstoreC.alreadyStarted = true
			logger.Info(contextImp.GetRuntimeContext(), "config is same after reload, use it again", GetFlushStoreLen(logstoreC.PluginRunner))
			return logstoreC, nil
		}
		oldConfig.resume()
		_ = oldConfig.Stop(false)
		logstoreC.PluginRunner.Merge(oldConfig.PluginRunner)
		logger.Info(contextImp.GetRuntimeContext(), "config is changed after reload", "stop and create a new one")
	} else if lastConfig, hasLastConfig := LastLogtailConfig[configName]; hasLastConfig {
		// Move unsent LogGroups from last config to new config.
		logstoreC.PluginRunner.Merge(lastConfig.PluginRunner)
	}

	enableAlwaysOnline := enableAlwaysOnlineForStdout && hasDockerStdoutInput(plugins)
	logstoreC.ContainerLabelSet = make(map[string]struct{})
	logstoreC.EnvSet = make(map[string]struct{})
	logstoreC.K8sLabelSet = make(map[string]struct{})
	// add env and label set to logstore config
	inputs, exists := plugins["inputs"]
	if exists {
		inputList, valid := inputs.([]interface{})
		if valid {
			for _, detail := range inputList {
				cfg, valid := detail.(map[string]interface{})
				if !valid {
					continue
				}
				typeName, valid := cfg["type"]
				if !valid {
					continue
				}
				if val, valid := typeName.(string); valid && (val == input.ServiceDockerStdoutPluginName || val == input.MetricDocierFilePluginName) {
					configDetail, valid := cfg["detail"]
					if !valid {
						continue
					}
					detailMap, valid := configDetail.(map[string]interface{})
					if !valid {
						continue
					}
					for key, value := range detailMap {
						lowerKey := strings.ToLower(key)
						if strings.Contains(lowerKey, "include") || strings.Contains(lowerKey, "exclude") {
							conditionMap, valid := value.(map[string]interface{})
							if !valid {
								continue
							}
							if strings.Contains(lowerKey, "k8slabel") {
								for key := range conditionMap {
									logstoreC.K8sLabelSet[key] = struct{}{}
								}
							} else if strings.Contains(lowerKey, "label") {
								for key := range conditionMap {
									logstoreC.ContainerLabelSet[key] = struct{}{}
								}
							}
							if strings.Contains(lowerKey, "env") {
								for key := range conditionMap {
									logstoreC.EnvSet[key] = struct{}{}
								}
							}
						}
						if strings.Contains(lowerKey, "collectcontainersflag") {
							collectContainersFlag, valid := value.(bool)
							if !valid {
								continue
							}
							logstoreC.CollectContainersFlag = collectContainersFlag
						}
					}
				}
			}
		}
	}

	logstoreC.GlobalConfig = &config.LogtailGlobalConfig
	// If plugins config has "global" field, then override the logstoreC.GlobalConfig
	if pluginConfigInterface, flag := plugins["global"]; flag || enableAlwaysOnline {
		pluginConfig := &config.GlobalConfig{}
		*pluginConfig = config.LogtailGlobalConfig
		if flag {
			configJSONStr, err := json.Marshal(pluginConfigInterface) //nolint:govet
			if err != nil {
				return nil, err
			}
			err = json.Unmarshal(configJSONStr, &pluginConfig)
			if err != nil {
				return nil, err
			}
		}
		if enableAlwaysOnline {
			pluginConfig.AlwaysOnline = true
		}
		logstoreC.GlobalConfig = pluginConfig
		logger.Debug(contextImp.GetRuntimeContext(), "load plugin config", *logstoreC.GlobalConfig)
	}

	logQueueSize := logstoreC.GlobalConfig.DefaultLogQueueSize
	// Because the transferred data of the file MixProcessMode is quite large, we have to limit queue size to control memory usage here.
	if checkMixProcessMode(plugins) == file {
		logger.Infof(contextImp.GetRuntimeContext(), "no inputs in config %v, maybe file input, limit queue size", configName)
		logQueueSize = 10
	}

	logGroupSize := logstoreC.GlobalConfig.DefaultLogGroupQueueSize

	if err = logstoreC.PluginRunner.Init(logQueueSize, logGroupSize); err != nil {
		return nil, err
	}

	logstoreC.Statistics.Init(logstoreC.Context)

	// extensions should be initialized first
	pluginConfig, ok := plugins["extensions"]
	if ok {
		extensions, ok := pluginConfig.([]interface{})
		if !ok {
			return nil, fmt.Errorf("invalid extension type: %s, not json array", "extensions")
		}
		for _, extensionInterface := range extensions {
			extension, ok := extensionInterface.(map[string]interface{})
			if !ok {
				return nil, fmt.Errorf("invalid extension type")
			}
			typeName, ok := extension["type"].(string)
			if !ok {
				return nil, fmt.Errorf("invalid extension type")
			}
			logger.Debug(contextImp.GetRuntimeContext(), "add extension", typeName)
			err = loadExtension(getPluginTypeWithID(typeName), logstoreC, extension["detail"])
			if err != nil {
				return nil, err
			}
			contextImp.AddPlugin(typeName)
		}
	}

	for pluginType, pluginConfig := range plugins {

		if pluginType == "extensions" {
			continue
		}

		if pluginType == "inputs" {
			inputs, ok := pluginConfig.([]interface{})
			if ok {
				for _, inputInterface := range inputs {
					input, ok := inputInterface.(map[string]interface{})
					if ok {
						if typeName, ok := input["type"]; ok {
							if typeNameStr, ok := typeName.(string); ok {
								if _, isMetricInput := pipeline.MetricInputs[typeNameStr]; isMetricInput {
									// Load MetricInput plugin defined in pipeline.MetricInputs
									// pipeline.MetricInputs will be renamed in a future version
									err = loadMetric(getPluginTypeWithID(typeNameStr), logstoreC, input["detail"])
								} else if _, isServiceInput := pipeline.ServiceInputs[typeNameStr]; isServiceInput {
									// Load ServiceInput plugin defined in pipeline.ServiceInputs
									err = loadService(getPluginTypeWithID(typeNameStr), logstoreC, input["detail"])
								}
								if err != nil {
									return nil, err
								}
								contextImp.AddPlugin(typeNameStr)
								continue
							}

						}
					}
					return nil, fmt.Errorf("invalid input type")
				}
			} else {
				return nil, fmt.Errorf("invalid inputs type : %s, not json array", pluginType)
			}
			continue
		}

		if pluginType == "processors" {
			processors, ok := pluginConfig.([]interface{})
			if ok {
				for i, processorInterface := range processors {
					processor, ok := processorInterface.(map[string]interface{})
					if ok {
						if typeName, ok := processor["type"]; ok {
							if typeNameStr, ok := typeName.(string); ok {
								logger.Debug(contextImp.GetRuntimeContext(), "add processor", typeNameStr)
								err = loadProcessor(getPluginTypeWithID(typeNameStr), i, logstoreC, processor["detail"])
								if err != nil {
									return nil, err
								}
								contextImp.AddPlugin(typeNameStr)
								continue
							}
						}
					}
					return nil, fmt.Errorf("invalid processor type")
				}
			} else {
				return nil, fmt.Errorf("invalid processors type : %s, not json array", pluginType)
			}
			continue
		}

		if pluginType == "aggregators" {
			aggregators, ok := pluginConfig.([]interface{})
			if ok {
				for _, aggregatorInterface := range aggregators {
					aggregator, ok := aggregatorInterface.(map[string]interface{})
					if ok {
						if typeName, ok := aggregator["type"]; ok {
							if typeNameStr, ok := typeName.(string); ok {
								logger.Debug(contextImp.GetRuntimeContext(), "add aggregator", typeNameStr)
								err = loadAggregator(getPluginTypeWithID(typeNameStr), logstoreC, aggregator["detail"])
								if err != nil {
									return nil, err
								}
								contextImp.AddPlugin(typeNameStr)
								continue
							}
						}
					}
					return nil, fmt.Errorf("invalid aggregator type")
				}
			} else {
				return nil, fmt.Errorf("invalid aggregator type : %s, not json array", pluginType)
			}
			continue
		}

		if pluginType == "flushers" {
			flushers, ok := pluginConfig.([]interface{})
			if ok {
				for _, flusherInterface := range flushers {
					flusher, ok := flusherInterface.(map[string]interface{})
					if ok {
						if typeName, ok := flusher["type"]; ok {
							if typeNameStr, ok := typeName.(string); ok {
								logger.Debug(contextImp.GetRuntimeContext(), "add flusher", typeNameStr)
								err = loadFlusher(getPluginTypeWithID(typeNameStr), logstoreC, flusher["detail"])
								if err != nil {
									return nil, err
								}
								contextImp.AddPlugin(typeNameStr)
								continue
							}
						}
					}
					return nil, fmt.Errorf("invalid flusher type")
				}
			} else {
				return nil, fmt.Errorf("invalid flusher type : %s, not json array", pluginType)
			}
			continue
		}

		if pluginType != "global" && pluginType != "version" && pluginType != mixProcessModeFlag {
			return nil, fmt.Errorf("error plugin name \"%s\"", pluginType)
		}
	}
	// Perform operations after pluginrunner initialization, such as adding default aggregators and flushers
	if err := logstoreC.PluginRunner.Initialized(); err != nil {
		return nil, err
	}
	return logstoreC, nil
}

func fetchPluginVersion(config map[string]interface{}) ConfigVersion {
	if v, ok := config["version"]; ok {
		if s, ok := v.(string); ok {
			return ConfigVersion(strings.ToLower(s))
		}
	}
	return v1
}

func initPluginRunner(lc *LogstoreConfig) (PluginRunner, error) {
	switch lc.Version {
	case v1:
		return &pluginv1Runner{
			LogstoreConfig: lc,
			FlushOutStore:  NewFlushOutStore[protocol.LogGroup](),
		}, nil
	case v2:
		return &pluginv2Runner{
			LogstoreConfig: lc,
			FlushOutStore:  NewFlushOutStore[models.PipelineGroupEvents](),
		}, nil
	default:
		return nil, fmt.Errorf("undefined config version %s", lc.Version)
	}
}

func LoadLogstoreConfig(project string, logstore string, configName string, logstoreKey int64, jsonStr string) error {
	if len(jsonStr) == 0 {
		logger.Info(context.Background(), "delete config", configName, "logstore", logstore)
		delete(LogtailConfig, configName)
		return nil
	}
	logger.Info(context.Background(), "load config", configName, "logstore", logstore)
	logstoreC, err := createLogstoreConfig(project, logstore, configName, logstoreKey, jsonStr)
	if err != nil {
		return err
	}
	LogtailConfig[configName] = logstoreC
	return nil
}

func loadBuiltinConfig(name string, project string, logstore string,
	configName string, cfgStr string) (*LogstoreConfig, error) {
	logger.Infof(context.Background(), "load built-in config %v, config name: %v, logstore: %v", name, configName, logstore)
	return createLogstoreConfig(project, logstore, configName, 0, cfgStr)
}

// loadMetric creates a metric plugin object and append to logstoreConfig.MetricPlugins.
// @pluginType: the type of metric plugin.
// @logstoreConfig: where to store the created metric plugin object.
// It returns any error encountered.
func loadMetric(pluginType string, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.MetricInputs[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		return fmt.Errorf("can't find plugin %s", pluginType)
	}
	metric := creator()
	if err = applyPluginConfig(metric, configInterface); err != nil {
		return err
	}
	interval, err := metric.Init(logstoreConfig.Context)
	if err != nil {
		return err
	}
	if interval == 0 {
		interval = logstoreConfig.GlobalConfig.InputIntervalMs
		configMapI, convertSuc := configInterface.(map[string]interface{})
		if convertSuc {
			valI, keyExist := configMapI["IntervalMs"]
			if keyExist {
				if val, convSuc := valI.(float64); convSuc {
					interval = int(val)
				}
			}
		}
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginMetricInput, metric, map[string]interface{}{"interval": interval})
}

// loadService creates a service plugin object and append to logstoreConfig.ServicePlugins.
// @pluginType: the type of service plugin.
// @logstoreConfig: where to store the created service plugin object.
// It returns any error encountered.
func loadService(pluginType string, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.ServiceInputs[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		return fmt.Errorf("can't find plugin %s", pluginType)
	}
	service := creator()
	if err = applyPluginConfig(service, configInterface); err != nil {
		return err
	}
	if _, err = service.Init(logstoreConfig.Context); err != nil {
		return err
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginServiceInput, service, map[string]interface{}{})
}

func loadProcessor(pluginType string, priority int, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.Processors[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		logger.Error(logstoreConfig.Context.GetRuntimeContext(), "INVALID_PROCESSOR_TYPE", "invalid processor type, maybe type is wrong or logtail version is too old", pluginType)
		return nil
	}
	processor := creator()
	if err = applyPluginConfig(processor, configInterface); err != nil {
		return err
	}
	if err = processor.Init(logstoreConfig.Context); err != nil {
		return err
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginProcessor, processor, map[string]interface{}{"priority": priority})
}

func loadAggregator(pluginType string, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.Aggregators[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		logger.Error(logstoreConfig.Context.GetRuntimeContext(), "INVALID_AGGREGATOR_TYPE", "invalid aggregator type, maybe type is wrong or logtail version is too old", pluginType)
		return nil
	}
	aggregator := creator()
	if err = applyPluginConfig(aggregator, configInterface); err != nil {
		return err
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginAggregator, aggregator, map[string]interface{}{})
}

func loadFlusher(pluginType string, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.Flushers[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		return fmt.Errorf("can't find plugin %s", pluginType)
	}
	flusher := creator()
	if err = applyPluginConfig(flusher, configInterface); err != nil {
		return err
	}
	if err = flusher.Init(logstoreConfig.Context); err != nil {
		return err
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginFlusher, flusher, map[string]interface{}{})
}

func loadExtension(pluginType string, logstoreConfig *LogstoreConfig, configInterface interface{}) (err error) {
	creator, existFlag := pipeline.Extensions[getPluginType(pluginType)]
	if !existFlag || creator == nil {
		return fmt.Errorf("can't find plugin %s", pluginType)
	}
	extension := creator()
	if err = applyPluginConfig(extension, configInterface); err != nil {
		return err
	}
	if err = extension.Init(logstoreConfig.Context); err != nil {
		return err
	}
	return logstoreConfig.PluginRunner.AddPlugin(pluginType, pluginExtension, extension, map[string]interface{}{})
}

func applyPluginConfig(plugin interface{}, pluginConfig interface{}) error {
	config, err := json.Marshal(pluginConfig)
	if err != nil {
		return err
	}
	err = json.Unmarshal(config, plugin)
	return err
}

// getPluginTypeWithID extracts plugin type with pluginID from full pluginName.
// Rule: pluginName=pluginType/pluginID#pluginPriority.
// It returns the plugin type with pluginID.
func getPluginTypeWithID(pluginName string) string {
	if idx := strings.IndexByte(pluginName, '#'); idx != -1 {
		return pluginName[:idx]
	}
	return pluginName
}

func getPluginType(pluginName string) string {
	if idx := strings.IndexByte(pluginName, '/'); idx != -1 {
		return pluginName[:idx]
	}
	return pluginName
}

func GetPluginPriority(pluginName string) int {
	if idx := strings.IndexByte(pluginName, '#'); idx != -1 {
		val, err := strconv.Atoi(pluginName[idx+1:])
		if err != nil {
			return 0
		}
		return val
	}
	return 0
}

func genEmbeddedPluginName(pluginType string) string {
	id := atomic.AddInt64(&embeddedNamingCnt, 1)
	return fmt.Sprintf("%s/_gen_embedded_%v", pluginType, id)
}

func init() {
	LogtailConfig = make(map[string]*LogstoreConfig)
	_ = util.InitFromEnvBool("ALIYUN_LOGTAIL_ENABLE_ALWAYS_ONLINE_FOR_STDOUT", &enableAlwaysOnlineForStdout, true)
}
