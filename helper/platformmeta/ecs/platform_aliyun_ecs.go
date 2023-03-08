package ecs

import (
	"context"
	"errors"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
)

// global var
var (
	error404 = errors.New("404")
	manager  *Manager
)

func AlibabaCloudEcsPlatformRequest(api string, method string, f func(header *http.Header)) (string, error) {
	r, _ := http.NewRequest(http.MethodPut, "http://100.100.100.200/latest"+api, nil)
	r.Method = method
	f(&r.Header)
	resp, err := new(http.Client).Do(r)
	if err != nil {
		return "", err
	}
	defer func() {
		_ = resp.Body.Close()
	}()
	logger.Info(context.Background(), "api", r.URL.Path)
	if resp.StatusCode == 404 {
		return "", error404
	}
	bytes, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	return string(bytes), nil
}

func AlibabaCloudEcsPlatformReadMetaVal(api string, token string) (val string, err error) {
	for i := 0; i < 2; i++ {
		val, err = AlibabaCloudEcsPlatformRequest(api, http.MethodGet, func(header *http.Header) {
			(*header)["X-aliyun-ecs-metadata-token"] = []string{token}
		})
		if err == nil || err == error404 {
			return
		}
	}
	return
}

type Data struct {
	id            string
	name          string
	tags          map[string]string
	region        string
	zone          string
	instanceType  string
	maxNetEngress int64
	maxNetIngress int64
	vpcID         string
	vswitchID     string
	imageID       string
}

type Manager struct {
	mutex                   sync.Mutex
	data                    Data
	ecsToken                string
	ecsLastFetchTime        time.Time
	ecsLastFetchTokenTime   time.Time
	ecsMinimumFetchInterval time.Duration
	ecsTokenExpireTime      int
	fetchRes                bool
	once                    sync.Once
}

func (m *Manager) fetchToken() (err error) {
	var val string
	for i := 0; i < 2; i++ {
		val, err = AlibabaCloudEcsPlatformRequest("/api/token", http.MethodPut, func(header *http.Header) {
			(*header)["X-aliyun-ecs-metadata-token-ttl-seconds"] = []string{strconv.Itoa(m.ecsTokenExpireTime)}
		})
		if err == nil {
			break
		}
	}
	if err != nil {
		return err
	}
	m.ecsToken = val
	return nil
}

func (m *Manager) startFetch() {
	m.once.Do(func() {
		m.fetchAPI()
		go func() {
			for range time.NewTicker(m.ecsMinimumFetchInterval).C {
				m.mutex.Lock()
				m.fetchAPI()
				m.mutex.Unlock()
			}
		}()
	})
}

func (m *Manager) fetchAPI() {
	defer func() {
		logger.Debug(context.Background(), "fetch ecs meta api res", m.fetchRes)
	}()
	now := time.Now()
	if now.Sub(m.ecsLastFetchTime) < m.ecsMinimumFetchInterval {
		return
	}
	m.fetchRes = false // reset
	if now.Sub(m.ecsLastFetchTokenTime).Seconds() > float64(m.ecsTokenExpireTime)*3/4 {
		if err := m.fetchToken(); err != nil {
			logger.Error(context.Background(), "ECS_ALARM", "read token error", err)
			return
		}
		m.ecsLastFetchTokenTime = now
	}

	for k := range m.data.tags {
		delete(m.data.tags, k)
	}

	readMetaFunc := func(api string, configFunc func(val string)) bool {
		val, err := AlibabaCloudEcsPlatformReadMetaVal(api, m.ecsToken)
		if err != nil {
			logger.Error(context.Background(), "ECS_ALARM", "read meta error", err)
			return false
		}
		configFunc(val)
		return true
	}

	if success := readMetaFunc("/meta-data/instance-id", func(val string) { m.data.id = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/instance/max-netbw-egress", func(val string) { m.data.maxNetEngress, _ = strconv.ParseInt(val, 10, 64) }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/instance/max-netbw-ingress", func(val string) { m.data.maxNetIngress, _ = strconv.ParseInt(val, 10, 64) }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/instance/instance-name", func(val string) { m.data.name = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/region-id", func(val string) { m.data.region = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/zone-id", func(val string) { m.data.zone = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/vswitch-id", func(val string) { m.data.vswitchID = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/vpc-id", func(val string) { m.data.vpcID = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/image-id", func(val string) { m.data.imageID = val }); !success {
		return
	}
	if success := readMetaFunc("/meta-data/instance/instance-type", func(val string) { m.data.instanceType = val }); !success {
		return
	}
	// tags is optional feature for ecs meta, so response code 404 is legal
	val, err := AlibabaCloudEcsPlatformReadMetaVal("/meta-data/tags/instance/", m.ecsToken)
	if err != nil {
		if err == error404 {
			m.fetchRes = true
			return
		}
		logger.Error(context.Background(), "ECS_ALARM", "tags api error", err)
		return
	}
	keys := strings.Split(val, "\n")
	for _, key := range keys {
		if key == "" {
			continue
		}
		key = strings.TrimSpace(key)
		if kval, err := AlibabaCloudEcsPlatformReadMetaVal("/meta-data/tags/instance/"+key, m.ecsToken); err != nil {
			logger.Errorf(context.Background(), "ECS_ALARM", "key: %s not found val", key)
			continue
		} else {
			m.data.tags[key] = kval
		}
	}
	m.fetchRes = true
}

func init() {
	manager = new(Manager)
	manager.data.tags = make(map[string]string)
	var val int
	_ = util.InitFromEnvInt("ALIYUN_ECS_MINIMUM_REFLUSH_INTERVAL", &val, 30)
	manager.ecsMinimumFetchInterval = time.Second * time.Duration(val)
	_ = util.InitFromEnvInt("ALIYUN_ECS_TOKEN_EXPIRE_TIME", &manager.ecsTokenExpireTime, 300)
}
