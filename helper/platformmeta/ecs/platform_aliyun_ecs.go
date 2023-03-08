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
	logger.Debug(context.Background(), "api", r.URL.Path)
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

type tag struct {
	k string
	v string
}
type Data struct {
	// unchanged meta
	id           string
	region       string
	zone         string
	instanceType string
	imageID      string

	// dynamic changed meta
	name          string
	tags          map[string]string
	maxNetEngress int64
	maxNetIngress int64
	vpcID         string
	vswitchID     string
}

type Manager struct {
	mutex                   sync.RWMutex
	data                    Data
	ecsToken                string
	ecsLastFetchTime        time.Time
	ecsLastFetchTokenTime   time.Time
	ecsMinimumFetchInterval time.Duration
	ecsTokenExpireTime      int
	fetchRes                bool
	once                    sync.Once
	unchangedAlreadyRead    bool
	resChan                 chan bool
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

	asyncReadMetaFunc := func(api string, key string, configFunc func(key, val string)) {
		go func() {
			val, err := AlibabaCloudEcsPlatformReadMetaVal(api, m.ecsToken)
			if err != nil && err != error404 {
				logger.Error(context.Background(), "ECS_ALARM", "read meta error", err)
				m.resChan <- false
				return
			}
			configFunc(key, val)
			m.resChan <- true
			return
		}()

	}
	success := true
	if !m.unchangedAlreadyRead {
		asyncReadMetaFunc("/meta-data/instance-id", "", func(key, val string) { m.data.id = val })
		asyncReadMetaFunc("/meta-data/region-id", "", func(key, val string) { m.data.region = val })
		asyncReadMetaFunc("/meta-data/zone-id", "", func(key, val string) { m.data.zone = val })
		asyncReadMetaFunc("/meta-data/image-id", "", func(key, val string) { m.data.imageID = val })
		asyncReadMetaFunc("/meta-data/instance/instance-type", "", func(key, val string) { m.data.instanceType = val })
		for i := 0; i < 5; i++ {
			ok := <-m.resChan
			success = success && ok
		}
	}
	if !success {
		return
	}
	var tags string
	asyncReadMetaFunc("/meta-data/instance/max-netbw-egress", "", func(key, val string) { m.data.maxNetEngress, _ = strconv.ParseInt(val, 10, 64) })
	asyncReadMetaFunc("/meta-data/instance/max-netbw-ingress", "", func(key, val string) { m.data.maxNetIngress, _ = strconv.ParseInt(val, 10, 64) })
	asyncReadMetaFunc("/meta-data/instance/instance-name", "", func(key, val string) { m.data.name = val })
	asyncReadMetaFunc("/meta-data/vswitch-id", "", func(key, val string) { m.data.vswitchID = val })
	asyncReadMetaFunc("/meta-data/vpc-id", "", func(key, val string) { m.data.vpcID = val })
	asyncReadMetaFunc("/meta-data/tags/instance/", "", func(key, val string) { tags = val })
	for i := 0; i < 6; i++ {
		ok := <-m.resChan
		success = success && ok
	}
	if success && tags != "" {
		keys := strings.Split(tags, "\n")
		num := 0
		for i, key := range keys {
			key = strings.TrimSpace(key)
			if key == "" {
				continue
			}
			keys[num] = keys[i]
			num++
		}
		keys = keys[:num]
		res := make(chan *tag, len(keys))
		for _, key := range keys {
			asyncReadMetaFunc("/meta-data/tags/instance/"+key, key, func(key, val string) {
				res <- &tag{
					k: key,
					v: val,
				}
			})
		}
		for i := 0; i < len(keys); i++ {
			<-m.resChan
		}
		for i := 0; i < len(res); i++ {
			t := <-res
			m.data.tags[t.k] = t.v
		}
	}
	m.fetchRes = true
}

func init() {
	manager = new(Manager)
	manager.data.tags = make(map[string]string)
	manager.resChan = make(chan bool, 30) // max support qps 30
	var val int
	_ = util.InitFromEnvInt("ALIYUN_ECS_MINIMUM_REFLUSH_INTERVAL", &val, 30)
	manager.ecsMinimumFetchInterval = time.Second * time.Duration(val)
	_ = util.InitFromEnvInt("ALIYUN_ECS_TOKEN_EXPIRE_TIME", &manager.ecsTokenExpireTime, 300)
}
