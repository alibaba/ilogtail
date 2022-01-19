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
	"path/filepath"
	"strings"
	"sync"

	liblogger "github.com/VictoriaMetrics/VictoriaMetrics/lib/logger"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/prompbmarshal"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/promscrape"

	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail"
)

type ServiceStaticPrometheus struct {
	Yaml              string `comment:"the prometheus configuration content, more details please see [here](https://prometheus.io/docs/prometheus/latest/configuration/configuration/)"`
	ConfigFilePath    string `comment:"the prometheus configuration path, and the param would be ignored when Yaml param is configured."`
	AuthorizationPath string `comment:"the prometheus authorization path, only using in authorization files. When Yaml param is configured, the default value is the current binary path. However, the default value is the ConfigFilePath directory when ConfigFilePath is working."`

	scraper       *promscrape.Scraper //nolint:typecheck
	shutdown      chan struct{}
	waitGroup     sync.WaitGroup
	context       ilogtail.Context
	libLoggerOnce sync.Once
}

func (p *ServiceStaticPrometheus) Init(context ilogtail.Context) (int, error) {
	p.libLoggerOnce.Do(func() {
		if f := flag.Lookup("loggerOutput"); f != nil {
			_ = f.Value.Set("stdout")
		}
		liblogger.Init()
	})
	p.context = context
	var detail []byte
	switch {
	case p.Yaml != "":
		detail = []byte(p.Yaml)
		if p.AuthorizationPath == "" {
			p.AuthorizationPath = util.GetCurrentBinaryPath()
		}
	case p.ConfigFilePath != "":
		f, err := os.Open(p.ConfigFilePath)
		if err != nil {
			return 0, fmt.Errorf("cannot find prometheus configuration file")
		}
		defer func(f *os.File) {
			_ = f.Close()
		}(f)
		bytes, err := ioutil.ReadAll(f)
		if err != nil {
			return 0, fmt.Errorf("cannot read prometheus configuration file")
		}
		detail = bytes
		if p.AuthorizationPath == "" {
			p.AuthorizationPath = filepath.Dir(p.ConfigFilePath)
		}
	default:
		return 0, errors.New("the scrape configuration is required")
	}
	var err error
	if p.AuthorizationPath, err = filepath.Abs(p.AuthorizationPath); err != nil {
		return 0, fmt.Errorf("cannot find the abs authorization path: %v", err)
	}

	name := strings.Join([]string{context.GetProject(), context.GetLogstore(), context.GetConfigName()}, "_")
	p.scraper = promscrape.NewScraper(detail, name, p.AuthorizationPath) //nolint:typecheck
	if err := p.scraper.CheckConfig(); err != nil {
		return 0, fmt.Errorf("illegal prometheus configuration file %s: %v", name, err)
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
	p.scraper.Init(func(wr *prompbmarshal.WriteRequest) {
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
		return &ServiceStaticPrometheus{}
	}
}
