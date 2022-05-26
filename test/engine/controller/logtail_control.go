package controller

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strconv"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
)

type LogtailController struct {
	chain  *CancelChain
	config map[string]map[string]interface{}
}

func (l *LogtailController) Init(parent *CancelChain, fullCfg *config.Case) error {
	logger.Info(context.Background(), "ilogtail controller is initializing....")
	l.chain = WithCancelChain(parent)
	logstoreCfg := make(map[string]interface{})
	// todo currently only support one fixed config with cgo
	if len(fullCfg.Ilogtail.Config) == 1 {
		cfg := fullCfg.Ilogtail.Config[0]
		for idx, con := range cfg.Content {
			name := cfg.Name + "_" + strconv.Itoa(idx)
			content := make(map[string]interface{})
			if err := json.Unmarshal([]byte(con), &content); err != nil {
				return fmt.Errorf("cannot deserizlize %s config by json, content:%s", name, con)
			}
			if cfg.MixedMode {
				logstoreCfg[name] = content
			} else {
				pluginCfg := make(map[string]interface{})
				pluginCfg["enable"] = true
				pluginCfg["log_type"] = "plugin"
				pluginCfg["category"] = E2ELogstoreName
				pluginCfg["project_name"] = E2EProjectName
				pluginCfg["plugin"] = content
				logstoreCfg[name] = pluginCfg
			}
		}
		fullCfg.Ilogtail.Config = fullCfg.Ilogtail.Config[:0]
	}

	bytes, _ := json.Marshal(map[string]interface{}{
		"metrics": logstoreCfg,
	})
	return ioutil.WriteFile(config.ConfigFile, bytes, 0600)
}

func (l *LogtailController) Start() error {
	go func() {
		<-l.chain.Done()
		logger.Info(context.Background(), "httpcase controller is closing....")
		l.Clean()
		l.chain.CancelChild()
	}()
	return nil
}

func (l *LogtailController) Clean() {
	logger.Info(context.Background(), "ilogtail controller is cleaning....")
}

func (l *LogtailController) CancelChain() *CancelChain {
	return l.chain
}
