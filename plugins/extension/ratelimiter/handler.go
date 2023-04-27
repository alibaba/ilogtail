package ratelimiter

import (
	"net/http"
	"time"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/limiter"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

type httpServerHandler struct {
	errorCode int
	limiter   limiter.RateLimiter
	rttFeed   trigger.FeedTrigger
	next      http.Handler
}

func (h *httpServerHandler) ServeHTTP(writer http.ResponseWriter, request *http.Request) {
	allow, done := h.limiter.Allow()
	if !allow {
		http.Error(writer, "too many requests", h.errorCode)
		return
	}
	defer done(nil)
	start := time.Now().UnixNano()
	h.next.ServeHTTP(writer, request)
	if h.rttFeed != nil {
		h.rttFeed.Feed(float64(time.Now().UnixNano() - start))
	}
}
