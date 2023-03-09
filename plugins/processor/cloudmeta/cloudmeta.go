package cloudmeta

import (
	"encoding/json"
	"errors"
	"fmt"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/processor/cloudmeta/platformmeta"
)

type Mode string

const (
	contentMode     Mode = "content"
	contentJSONMode Mode = "json"
)

type ProcessorCloudMeta struct {
	Platform platformmeta.Platform
	Mode     Mode
	JSONKey  string
	JSONPath string
	AddMetas map[string]string

	keyPath     []string
	m           platformmeta.Manager
	context     pipeline.Context
	meta        map[string]string
	logcontents []*protocol.Log_Content
}

func (c *ProcessorCloudMeta) readMeta() {
	for k := range c.meta {
		delete(c.meta, k)
	}
	for k, name := range c.AddMetas {
		switch k {
		case platformmeta.FlagInstanceID:
			c.meta[name] = c.m.GetInstanceID()
		case platformmeta.FlagInstanceName:
			c.meta[name] = c.m.GetInstanceName()
		case platformmeta.FlagInstanceZone:
			c.meta[name] = c.m.GetInstanceZone()
		case platformmeta.FlagInstanceRegion:
			c.meta[name] = c.m.GetInstanceRegion()
		case platformmeta.FlagInstanceType:
			c.meta[name] = c.m.GetInstanceType()
		case platformmeta.FlagInstanceVswitchID:
			c.meta[name] = c.m.GetInstanceVswitchID()
		case platformmeta.FlagInstanceVpcID:
			c.meta[name] = c.m.GetInstanceVpcID()
		case platformmeta.FlagInstanceImageID:
			c.meta[name] = c.m.GetInstanceImageID()
		case platformmeta.FlagInstanceMaxIngress:
			c.meta[name] = strconv.FormatInt(c.m.GetInstanceMaxNetIngress(), 10)
		case platformmeta.FlagInstanceMaxEgress:
			c.meta[name] = strconv.FormatInt(c.m.GetInstanceMaxNetEgress(), 10)
		case platformmeta.FlagInstanceTags:
			for k, v := range c.m.GetInstanceTags() {
				c.meta[name+"_"+k] = v
			}
		}
	}

}

func (c *ProcessorCloudMeta) Init(context pipeline.Context) error {
	c.context = context
	c.meta = make(map[string]string)
	m := platformmeta.GetManager(c.Platform)
	if m == nil {
		logger.Error(c.context.GetRuntimeContext(), "CLOUD_META_ALARM", "not support platform", c.Platform)
		return fmt.Errorf("not support collect %s metadata", c.Platform)
	}
	m.StartCollect()
	c.m = m
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
	if c.Mode == contentMode {
		for k, v := range c.meta {
			c.logcontents = append(c.logcontents, &protocol.Log_Content{
				Key:   k,
				Value: v,
			})
		}
	}
	return nil
}

func (c *ProcessorCloudMeta) Description() string {
	return "add cloud meta for log content or json log content"
}

func (c *ProcessorCloudMeta) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
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
			Platform: platformmeta.Mock,
		}
	}
}
