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

package prometheus

import (
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"sync"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"

	"github.com/VictoriaMetrics/VictoriaMetrics/lib/prompbmarshal"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/promscrape"
)

type ServiceStaticPrometheus struct {
	Yaml           string `comment:"the prometheus configuration content, more details please see [here](https://prometheus.io/docs/prometheus/latest/configuration/configuration/)"`
	ConfigFilePath string `comment:"the prometheus configuration path, and the param would be ignored when Yaml param is configured."`

	sctaper   *promscrape.Scraper
	shutdown  chan struct{}
	waitGroup sync.WaitGroup
	context   ilogtail.Context
}

func (p *ServiceStaticPrometheus) Init(context ilogtail.Context) (int, error) {
	p.context = context
	var configPath string
	switch {
	case p.Yaml != "":
		name := strings.Join([]string{p.context.GetProject(), p.context.GetLogstore(), p.context.GetConfigName(), "scrape.yml"}, "_")
		_ = ioutil.WriteFile(name, []byte(p.Yaml), os.FileMode(0744))
		configPath = name
	case p.ConfigFilePath != "":
		if _, err := os.Stat(p.ConfigFilePath); err != nil {
			return 0, fmt.Errorf("read scrape configuration fail: %v", err)
		}
		configPath = p.ConfigFilePath
	default:
		return 0, errors.New("the scrape configuration is required")
	}
	name := strings.Join([]string{context.GetProject(), context.GetLogstore(), context.GetConfigName()}, "_")
	p.sctaper = promscrape.NewScraper(configPath, name)
	if err := p.sctaper.CheckConfig(); err != nil {
		return 0, fmt.Errorf("illegal prometheus configuration file %s: %v", configPath, err)
	}
	if scrapeFlag := flag.Lookup("promscrape.configCheckInterval"); scrapeFlag != nil {
		_ = scrapeFlag.Value.Set("15s")
		logger.Info(p.context.GetRuntimeContext(), "set prometheus scrape config file reload interval", "15s")
	}
	return 0, nil
}

func (p *ServiceStaticPrometheus) Description() string {
	return "prometheus scrape plugin for logtail, use vmagent lib"
}

// Start starts the ServiceInput's service, whatever that may be
func (p *ServiceStaticPrometheus) Start(c ilogtail.Collector) error {
	p.shutdown = make(chan struct{})
	p.waitGroup.Add(1)
	defer p.waitGroup.Done()
	p.sctaper.Init(func(wr *prompbmarshal.WriteRequest) {
		appendTSDataToSlsLog(c, wr)
	})
	<-p.shutdown
	return nil
}

// Stop stops the services and closes any necessary channels and connections
func (p *ServiceStaticPrometheus) Stop() error {
	close(p.shutdown)
	p.waitGroup.Wait()
	return nil
}

func init() {
	ilogtail.ServiceInputs["service_prometheus"] = func() ilogtail.ServiceInput {
		return &ServiceStaticPrometheus{
			ConfigFilePath: "prometheus_scrape_config_default.yml",
		}
	}
}
