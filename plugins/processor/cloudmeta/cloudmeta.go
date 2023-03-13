package cloudmeta

import (
	"encoding/json"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"github.com/cespare/xxhash/v2"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/processor/cloudmeta/platformmeta"
)

type Mode string

const (
	contentMode     Mode = "add_fields"
	contentJSONMode Mode = "modify_json"
)

type ProcessorCloudMeta struct {
	Platform platformmeta.Platform
	Mode     Mode
	JSONKey  string
	JSONPath string
	AddMetas map[string]string
	ReadOnce bool

	keyPath      []string
	m            platformmeta.Manager
	context      pipeline.Context
	meta         map[string]string
	logcontents  []*protocol.Log_Content
	metaHash     uint64
	lastReadTime time.Time
}

func (c *ProcessorCloudMeta) readMeta() {
	now := time.Now()
	if c.m == nil || now.Sub(c.lastReadTime).Milliseconds() < 1000 || (c.ReadOnce && !c.lastReadTime.IsZero()) {
		return
	}
	var strBuilder strings.Builder
	newMeta := make(map[string]string)
	wrapperFunc := func(name, val string) {
		strBuilder.WriteString(name)
		strBuilder.WriteString(val)
		newMeta[name] = val
	}
	for k, name := range c.AddMetas {
		switch k {
		case platformmeta.FlagInstanceIDWrapper:
			wrapperFunc(name, c.m.GetInstanceID())
		case platformmeta.FlagInstanceNameWrapper:
			wrapperFunc(name, c.m.GetInstanceName())
		case platformmeta.FlagInstanceZoneWrapper:
			wrapperFunc(name, c.m.GetInstanceZone())
		case platformmeta.FlagInstanceRegionWrapper:
			wrapperFunc(name, c.m.GetInstanceRegion())
		case platformmeta.FlagInstanceTypeWrapper:
			wrapperFunc(name, c.m.GetInstanceType())
		case platformmeta.FlagInstanceVswitchIDWrapper:
			wrapperFunc(name, c.m.GetInstanceVswitchID())
		case platformmeta.FlagInstanceVpcIDWrapper:
			wrapperFunc(name, c.m.GetInstanceVpcID())
		case platformmeta.FlagInstanceImageIDWrapper:
			wrapperFunc(name, c.m.GetInstanceImageID())
		case platformmeta.FlagInstanceMaxIngressWrapper:
			wrapperFunc(name, strconv.FormatInt(c.m.GetInstanceMaxNetIngress(), 10))
		case platformmeta.FlagInstanceMaxEgressWrapper:
			wrapperFunc(name, strconv.FormatInt(c.m.GetInstanceMaxNetEgress(), 10))
		case platformmeta.FlagInstanceTagsWrapper:
			for k, v := range c.m.GetInstanceTags() {
				newK := name + "_" + k
				newMeta[newK] = v
				strBuilder.WriteString(newK)
				strBuilder.WriteString(v)
			}
		}
	}
	newHash := xxhash.Sum64(util.ZeroCopyStringToBytes(strBuilder.String()))
	if newHash != c.metaHash {
		c.metaHash = newHash
		c.meta = newMeta
		if c.Mode == contentMode {
			c.logcontents = c.logcontents[:0]
			for k, v := range c.meta {
				c.logcontents = append(c.logcontents, &protocol.Log_Content{
					Key:   k,
					Value: v,
				})
			}
		}
	}
	c.lastReadTime = now
}

func (c *ProcessorCloudMeta) Init(context pipeline.Context) error {
	c.context = context
	c.meta = make(map[string]string)
	m := platformmeta.GetManager(c.Platform)
	if m == nil {
		logger.Error(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "not support platform", c.Platform)
	} else {
		c.m = m
		c.m.StartCollect()
	}
	if c.Mode != contentMode && c.Mode != contentJSONMode {
		logger.Error(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "not support mode", c.Mode)
		return fmt.Errorf("not support mode %s", c.Mode)
	}
	c.JSONKey = strings.TrimSpace(c.JSONKey)
	if c.Mode == contentJSONMode && c.JSONKey == "" {
		logger.Error(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "json mode key is required")
		return errors.New("json mode key is required")
	}
	if c.JSONPath != "" {
		c.keyPath = strings.Split(c.JSONPath, ".")
	}
	c.readMeta()
	return nil
}

func (c *ProcessorCloudMeta) Description() string {
	return "add cloud meta for log content or json log content"
}

func (c *ProcessorCloudMeta) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	c.readMeta()
	if len(c.meta) == 0 {
		return logArray
	}
	switch c.Mode {
	case contentMode:
		for _, log := range logArray {
			log.Contents = append(log.Contents, c.logcontents...)
		}
	case contentJSONMode:
		for _, log := range logArray {
			for _, con := range log.Contents {
				if con.Key != c.JSONKey {
					continue
				}
				data := make(map[string]interface{})
				if err := json.Unmarshal(util.ZeroCopyStringToBytes(con.Value), &data); err != nil {
					logger.Warning(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "json deserialize err", err)
					continue
				}
				res := findMapByPath(data, c.keyPath)
				if res == nil {
					logger.Warning(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "json path not exist")
					continue
				}
				switch t := res.(type) {
				case map[string]string:
					for k, v := range c.meta {
						t[k] = v
					}
				case map[string]interface{}:
					for k, v := range c.meta {
						t[k] = v
					}
				}
				added, _ := json.Marshal(data)
				con.Value = string(added)
			}
		}
	}
	return logArray
}

func findMapByPath(data map[string]interface{}, path []string) interface{} {
	if len(path) == 0 {
		return data
	}
	k := path[0]
	d, ok := data[k]
	if !ok {
		return nil
	}
	switch t := d.(type) {
	case map[string]string:
		if len(path) == 1 {
			return d
		}
		return nil
	case map[string]interface{}:
		if len(path) == 1 {
			return d
		}
		return findMapByPath(t, path[1:])
	default:
		return nil
	}
}

func init() {
	pipeline.Processors["processor_cloudmeta"] = func() pipeline.Processor {
		return &ProcessorCloudMeta{
			Mode:     contentMode,
			Platform: platformmeta.Auto,
		}
	}
}
