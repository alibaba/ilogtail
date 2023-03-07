package platformmeta

import (
	"context"
	"errors"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
)

// global var
var (
	ecsToken                string
	ecsLastFetchTime        time.Time
	ecsLastFetchTokenTime   time.Time
	ecsMutex                sync.Mutex
	ecsMeta                 map[string]string
	ecsMinimumFetchInterval time.Duration
	ecsTokenExpireTime      = 300
	error404                = errors.New("404")
)

func AlibabaCloudEcsPlatformMetaCollect(meta map[string]string, minimize bool) map[string]string {
	ecsMutex.Lock()
	defer ecsMutex.Unlock()
	var err error
	now := time.Now()
	if now.Sub(ecsLastFetchTime) < ecsMinimumFetchInterval {
		for k, v := range ecsMeta {
			meta[k] = v
		}
		return meta
	}
	if now.Sub(ecsLastFetchTokenTime).Seconds() > float64(ecsTokenExpireTime)/2 {
		if ecsToken, err = AlibabaCloudEcsPlatformReadToken(); err != nil {
			logger.Error(context.Background(), "ECS_ALARM", "read token error", err)
			return meta
		} else {
			ecsLastFetchTokenTime = now
		}
	}
	m, err := AlibabaCloudEcsPlatformReadMeta(minimize)
	if err != nil {
		logger.Error(context.Background(), "ECS_ALARM", "read meta error", err)
		return meta
	}
	ecsMeta = m
	for k, v := range ecsMeta {
		meta[k] = v
	}
	ecsLastFetchTime = now
	return meta
}

func AlibabaCloudEcsPlatformReadToken() (val string, err error) {
	for i := 0; i < 2; i++ {
		val, err = AlibabaCloudEcsPlatformRequest("/api/token", http.MethodPut, func(header *http.Header) {
			(*header)["X-aliyun-ecs-metadata-token-ttl-seconds"] = []string{strconv.Itoa(ecsTokenExpireTime)}
		})
		if err == nil || err == error404 {
			return
		}
	}
	return
}

func AlibabaCloudEcsPlatformReadMetaVal(api string) (val string, err error) {
	for i := 0; i < 2; i++ {
		val, err = AlibabaCloudEcsPlatformRequest(api, http.MethodGet, func(header *http.Header) {
			(*header)["X-aliyun-ecs-metadata-token"] = []string{ecsToken}
		})
		if err == nil || err == error404 {
			return
		}
	}
	return
}

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
	if resp.StatusCode == 404 {
		return "", error404
	}
	if bytes, err := ioutil.ReadAll(resp.Body); err != nil {
		return "", err
	} else {
		return string(bytes), nil
	}
}

func AlibabaCloudEcsPlatformReadMeta(minimize bool) (map[string]string, error) {
	meta := make(map[string]string)
	var err error
	configFunc := func(api, name string) error {
		if val, err := AlibabaCloudEcsPlatformReadMetaVal(api); err != nil {
			return err
		} else {
			meta[name] = val
		}
		return nil
	}
	if err = configFunc("/meta-data/instance-id", "ecs_instance_id"); err != nil {
		return nil, err
	}
	if err = configFunc("/meta-data/instance/instance-name", "ecs_instance_name"); err != nil {
		return nil, err
	}
	if err = configFunc("/meta-data/region-id", "ecs_region_id"); err != nil {
		return nil, err
	}
	if err = configFunc("/meta-data/zone-id", "ecs_zone_id"); err != nil {
		return nil, err
	}

	if !minimize {
		if err = configFunc("/meta-data/instance/instance-type", "ecs_instance_type"); err != nil {
			return nil, err
		}
		if err = configFunc("/meta-data/instance/max-netbw-egress", "ecs_instance_max_net_engress"); err != nil {
			return nil, err
		}
		if err = configFunc("/meta-data/instance/max-netbw-ingress", "ecs_instance_max_net_ingress"); err != nil {
			return nil, err
		}
		if err = configFunc("/meta-data/vswitch-id", "ecs_vswitch_id"); err != nil {
			return nil, err
		}
		if err = configFunc("/meta-data/vpc-id", "ecs_vpc_id"); err != nil {
			return nil, err
		}
		if err = configFunc("/meta-data/image-id", "ecs_image_id"); err != nil {
			return nil, err
		}
		if val, err := AlibabaCloudEcsPlatformReadMetaVal("/meta-data/tags/instance/"); err != nil {
			if err == error404 {
				logger.Error(context.Background(), "ECS_ALARM", "tags api error code 404")
				return meta, nil
			}
			return nil, err
		} else {
			keys := strings.Split(val, "\n")
			for _, key := range keys {
				if key == "" {
					continue
				}
				key = strings.TrimSpace(key)
				if kval, err := AlibabaCloudEcsPlatformReadMetaVal("/meta-data/tags/instance/" + key); err != nil {
					logger.Errorf(context.Background(), "ECS_ALARM", "key: %s not found val", key)
					continue
				} else {
					helper.ReplaceInvalidChars(&key)
					meta["ecs_tag_"+key] = kval
				}
			}
		}
	}
	return meta, nil
}

func init() {
	val := os.Getenv("ECS_MINIMUM_REFLUSH_INTERVAL")
	if val == "" {
		ecsMinimumFetchInterval = time.Second * 10
	}
	if d, err := time.ParseDuration(val); err != nil {
		ecsMinimumFetchInterval = time.Second * 10
	} else {
		ecsMinimumFetchInterval = d
	}

}
