package envconfig

import (
	"context"
	"errors"
	"github.com/alibaba/ilogtail/pkg/logger"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	aliyunlog "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/alibabacloud-go/tea/tea"
	"sync"
	"time"
)

type UpdateTokenFunc func() (accessKeyID, accessKeySecret, securityToken string, expireTime time.Time, err error)

var errSTSFetchHighFrequency = errors.New("sts token fetch frequency is too high")

type TokenAutoUpdateClient struct {
	*aliyunlog.Client
	shutdown               <-chan struct{}
	closeFlag              bool
	tokenUpdateFunc        UpdateTokenFunc
	maxTryTimes            int
	waitIntervalMin        time.Duration
	waitIntervalMax        time.Duration
	updateTokenIntervalMin time.Duration
	nextExpire             time.Time

	lock               sync.Mutex
	lastFetch          time.Time
	lastRetryFailCount int
	lastRetryInterval  time.Duration
	ctx                context.Context
}

func (c *TokenAutoUpdateClient) flushSTSToken() {
	for {
		nowTime := time.Now()
		c.lock.Lock()
		sleepTime := c.nextExpire.Sub(nowTime)
		if sleepTime < time.Duration(time.Minute) {
			sleepTime = time.Duration(time.Second * 30)

		} else if sleepTime < time.Duration(time.Minute*10) {
			sleepTime = sleepTime / 10 * 7
		} else if sleepTime < time.Duration(time.Hour) {
			sleepTime = sleepTime / 10 * 6
		} else {
			sleepTime = sleepTime / 10 * 5
		}
		c.lock.Unlock()
		logger.Debug(c.ctx, "msg", "next fetch sleep interval : ", sleepTime.String())
		trigger := time.After(sleepTime)
		select {
		case <-trigger:
			err := c.fetchSTSToken()
			if err != nil {
				logger.Error(c.ctx, "msg", "fetch sts token done, error : ", err)
			}
		case <-c.shutdown:
			logger.Debug(c.ctx, "msg", "receive shutdown signal, exit flushSTSToken")
			return
		}
		if c.closeFlag {
			logger.Debug(c.ctx, "msg", "close flag is true, exit flushSTSToken")
			return
		}
	}
}

func (c *TokenAutoUpdateClient) fetchSTSToken() error {
	nowTime := time.Now()
	skip := false
	sleepTime := time.Duration(0)
	c.lock.Lock()
	if nowTime.Sub(c.lastFetch) < c.updateTokenIntervalMin {
		skip = true
	} else {
		c.lastFetch = nowTime
		if c.lastRetryFailCount == 0 {
			sleepTime = 0
		} else {
			c.lastRetryInterval *= 2
			if c.lastRetryInterval < c.waitIntervalMin {
				c.lastRetryInterval = c.waitIntervalMin
			}
			if c.lastRetryInterval >= c.waitIntervalMax {
				c.lastRetryInterval = c.waitIntervalMax
			}
			sleepTime = c.lastRetryInterval
		}
	}
	c.lock.Unlock()
	if skip {
		return errSTSFetchHighFrequency
	}
	if sleepTime > time.Duration(0) {
		time.Sleep(sleepTime)
	}

	accessKeyID, accessKeySecret, securityToken, expireTime, err := c.tokenUpdateFunc()
	if err == nil {
		c.lock.Lock()
		c.lastRetryFailCount = 0
		c.lastRetryInterval = time.Duration(0)
		c.nextExpire = expireTime
		c.lock.Unlock()
		logClient, err := CreateNormalInterface(*c.Client.Endpoint, accessKeyID, accessKeySecret, securityToken, *c.Client.UserAgent)
		if err != nil {
			return err
		}
		c.Client = *logClient
		logger.Debug(c.ctx, "msg", "fetch sts token success id : ", accessKeyID)
	} else {
		c.lock.Lock()
		c.lastRetryFailCount++
		c.lock.Unlock()
		logger.Warning(c.ctx, "msg", "fetch sts token error : ", err.Error())
	}
	return err
}

// CreateNormalInterface create a normal client
func CreateNormalInterface(endpoint, accessKeyID, accessKeySecret, securityToken string, userAgent string) (**aliyunlog.Client, error) {
	openapiConfig := &openapi.Config{
		AccessKeyId:     tea.String(accessKeyID),
		AccessKeySecret: tea.String(accessKeySecret),
		SecurityToken:   tea.String(securityToken),
		UserAgent:       tea.String(userAgent),
	}
	openapiConfig.Endpoint = tea.String(endpoint)
	logClient := &aliyunlog.Client{}
	logClient, err := aliyunlog.NewClient(openapiConfig)
	return &logClient, err
}

func CreateTokenAutoUpdateClient(endpoint string, tokenUpdateFunc UpdateTokenFunc, shutdown <-chan struct{}, userAgent string) (**aliyunlog.Client, error) {
	accessKeyID, accessKeySecret, securityToken, expireTime, err := tokenUpdateFunc()
	if err != nil {
		return nil, err
	}

	logClient, err := CreateNormalInterface(endpoint, accessKeyID, accessKeySecret, securityToken, userAgent)
	if err != nil {
		return nil, err
	}
	tauc := &TokenAutoUpdateClient{
		Client:                 *logClient,
		shutdown:               shutdown,
		tokenUpdateFunc:        tokenUpdateFunc,
		maxTryTimes:            3,
		waitIntervalMin:        time.Duration(time.Second * 1),
		waitIntervalMax:        time.Duration(time.Second * 60),
		updateTokenIntervalMin: time.Duration(time.Second * 1),
		nextExpire:             expireTime,
	}
	go tauc.flushSTSToken()
	return &tauc.Client, nil
}
