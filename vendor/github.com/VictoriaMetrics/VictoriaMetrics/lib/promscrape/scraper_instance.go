package promscrape

import (
	"bytes"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/logger"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/procutil"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/prompbmarshal"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/promscrape/discovery/consul"
	"sync"
	"time"
)

type Scraper struct {
	globalStopCh chan struct{}
	scraperWG    sync.WaitGroup
	configFile   string
	name         string
	pushData     func(wr *prompbmarshal.WriteRequest)
}

func NewScraper(file, name string) *Scraper {
	return &Scraper{
		configFile: file,
		name:       name,
	}
}

func (s *Scraper) Init(pushData func(wr *prompbmarshal.WriteRequest)) {
	s.globalStopCh = make(chan struct{})
	s.scraperWG.Add(1)
	s.pushData = pushData
	go func() {
		defer s.scraperWG.Done()
		s.runScraper()
	}()
}

func (s *Scraper) Stop() {
	close(s.globalStopCh)
	s.scraperWG.Wait()
}

func (s *Scraper) CheckConfig() error {
	_, _, err := loadConfig(s.configFile)
	return err
}

func (s *Scraper) runScraper() {
	if s.configFile == "" {
		// Nothing to scrape.
		return
	}
	logger.Infof("reading Prometheus configs from %q", s.configFile)
	cfg, data, err := loadConfig(s.configFile)
	if err != nil {
		logger.Fatalf("cannot read %q: %s", s.configFile, err)
	}
	cfg.mustStart()
	scs := newScrapeConfigs(s.pushData)
	scs.add(s.name+"_static_configs", 0, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getStaticScrapeWork() })
	scs.add(s.name+"_file_sd_configs", *fileSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getFileSDScrapeWork(swsPrev) })
	scs.add(s.name+"_kubernetes_sd_configs", *kubernetesSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getKubernetesSDScrapeWork(swsPrev) })
	scs.add(s.name+"_openstack_sd_configs", *openstackSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getOpenStackSDScrapeWork(swsPrev) })
	scs.add(s.name+"_consul_sd_configs", *consul.SDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getConsulSDScrapeWork(swsPrev) })
	scs.add(s.name+"_eureka_sd_configs", *eurekaSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getEurekaSDScrapeWork(swsPrev) })
	scs.add(s.name+"_dns_sd_configs", *dnsSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getDNSSDScrapeWork(swsPrev) })
	scs.add(s.name+"_ec2_sd_configs", *ec2SDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getEC2SDScrapeWork(swsPrev) })
	scs.add(s.name+"_gce_sd_configs", *gceSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getGCESDScrapeWork(swsPrev) })
	scs.add(s.name+"_dockerswarm_sd_configs", *dockerswarmSDCheckInterval, func(cfg *Config, swsPrev []*ScrapeWork) []*ScrapeWork { return cfg.getDockerSwarmSDScrapeWork(swsPrev) })

	sighupCh := procutil.NewSighupChan()

	var tickerCh <-chan time.Time
	if *configCheckInterval > 0 {
		ticker := time.NewTicker(*configCheckInterval)
		tickerCh = ticker.C
		defer ticker.Stop()
	}
	for {
		scs.updateConfig(cfg)
	waitForChans:
		select {
		case <-sighupCh:
			logger.Infof("SIGHUP received; reloading Prometheus configs from %q", s.configFile)
			cfgNew, dataNew, err := loadConfig(s.configFile)
			if err != nil {
				logger.Errorf("cannot read %q on SIGHUP: %s; continuing with the previous config", s.configFile, err)
				goto waitForChans
			}
			if bytes.Equal(data, dataNew) {
				logger.Infof("nothing changed in %q", s.configFile)
				goto waitForChans
			}
			cfg.mustStop()
			cfgNew.mustStart()
			cfg = cfgNew
			data = dataNew
		case <-tickerCh:
			cfgNew, dataNew, err := loadConfig(s.configFile)
			if err != nil {
				logger.Errorf("cannot read %q: %s; continuing with the previous config", s.configFile, err)
				goto waitForChans
			}
			if bytes.Equal(data, dataNew) {
				// Nothing changed since the previous loadConfig
				goto waitForChans
			}
			cfg.mustStop()
			cfgNew.mustStart()
			cfg = cfgNew
			data = dataNew
		case <-s.globalStopCh:
			cfg.mustStop()
			logger.Infof("stopping Prometheus scrapers")
			startTime := time.Now()
			scs.stop()
			logger.Infof("stopped Prometheus scrapers in %.3f seconds", time.Since(startTime).Seconds())
			return
		}
		logger.Infof("found changes in %q; applying these changes", s.configFile)
		configReloads.Inc()
	}
}
