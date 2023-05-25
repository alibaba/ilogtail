package ratelimiter

import (
	"fmt"
	"net/http"
	"time"

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/limiter"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

const (
	AlgorithmBBR       Algorithm = "bbr"
	AlgorithmGradient2 Algorithm = "gradient2"
)

type Algorithm string

type ExtensionRateLimiter struct {
	Algorithm            Algorithm // the algorithm to use, BBR by default
	MaxInflight          int       // fixed max limit of requests processing in concurrency, 2000 by default
	CPUUsageThreshold    int       // CPU usage, percent in range of (0,100], 0 means no limit, 80 by default
	MemUsedThresholdInMB int       // process rss memory threshold in MB, 0 means no limit
	RTTThresholdInMillis int64     // request processing time in milliseconds, 0 means no limit
	HTTPCodeOnLimit      int       // http status code to return when limit, 503 by default

	context pipeline.Context
	trigger trigger.Trigger
	rttFeed trigger.FeedTrigger

	limiters []limiter.RateLimiter
}

func (e *ExtensionRateLimiter) Description() string {
	return "rate limiter for http server handler"
}

func (e *ExtensionRateLimiter) Init(context pipeline.Context) error {
	e.context = context
	if e.HTTPCodeOnLimit <= 0 {
		return fmt.Errorf("invalid HTTPCodeOnLimit value: %v", e.HTTPCodeOnLimit)
	}
	switch e.Algorithm {
	case AlgorithmBBR:
	case AlgorithmGradient2:
	default:
		return fmt.Errorf("not supported algorithm: %v", e.Algorithm)
	}
	e.createTrigger()
	return nil
}

func (e *ExtensionRateLimiter) Stop() error {
	for _, rateLimiter := range e.limiters {
		rateLimiter.Stop()
	}
	if e.trigger != nil {
		e.trigger.Stop()
	}
	return nil
}

func (e *ExtensionRateLimiter) Handler(handler http.Handler) http.Handler {
	// create new RateLimiter for each handler, while sharing with same trigger instance
	var l limiter.RateLimiter
	switch e.Algorithm {
	case AlgorithmBBR:
		l = limiter.NewBBRRateLimiter(
			limiter.WithMaxLimit(e.MaxInflight),
			limiter.WithTrigger(e.trigger),
		)
	case AlgorithmGradient2:
		l = limiter.NewGradient2RateLimiter(
			limiter.WithMaxLimit(e.MaxInflight),
			limiter.WithTrigger(e.trigger),
		)
	default:
		panic(fmt.Sprintf("not supported algorithm: %v", e.Algorithm))
	}

	e.limiters = append(e.limiters, l)
	h := &httpServerHandler{
		limiter:   l,
		rttFeed:   e.rttFeed,
		next:      handler,
		errorCode: e.HTTPCodeOnLimit,
	}
	return h
}

func (e *ExtensionRateLimiter) createTrigger() {
	var triggers []trigger.Trigger
	var rttTrigger trigger.FeedTrigger
	if e.CPUUsageThreshold > 0 {
		triggers = append(triggers, trigger.NewCPUUsageTrigger(e.CPUUsageThreshold))
	}
	if e.MemUsedThresholdInMB > 0 {
		triggers = append(triggers, trigger.NewMemUsedTrigger(float64(e.MemUsedThresholdInMB)))
	}
	if e.RTTThresholdInMillis > 0 {
		rttTrigger = trigger.NewRTTTrigger(time.Duration(e.RTTThresholdInMillis) * time.Millisecond)
		triggers = append(triggers, rttTrigger)
	}

	e.trigger = trigger.NewCompositeTrigger(triggers...)
	e.rttFeed = rttTrigger
}

func init() {
	pipeline.AddExtensionCreator("ext_ratelimiter", func() pipeline.Extension {
		return &ExtensionRateLimiter{
			Algorithm:         AlgorithmGradient2,
			HTTPCodeOnLimit:   503,
			MaxInflight:       2000,
			CPUUsageThreshold: 80,
		}
	})
}
