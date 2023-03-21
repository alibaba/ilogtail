package requestbreaker

import (
	"fmt"
	"net/http"
	"time"

	"github.com/snakorse/handy/breaker"

	"github.com/alibaba/ilogtail/pkg/pipeline"
)

type ExtensionRequestBreaker struct {
	FailureRatio    float64
	WindowInSeconds int

	breaker breaker.Breaker
}

func (b *ExtensionRequestBreaker) Description() string {
	return "http client request breaker extension that enable fail-fast on unavailable endpoints"
}

func (b *ExtensionRequestBreaker) Init(context pipeline.Context) error {
	var ops []breaker.BreakerOption
	ops = append(ops, breaker.WithFailureRatio(b.FailureRatio))
	if b.WindowInSeconds > 0 {
		ops = append(ops, breaker.WithWindow(time.Duration(b.WindowInSeconds)*time.Second))
	}
	b.breaker = breaker.NewBreaker(ops...)
	return nil
}

func (b *ExtensionRequestBreaker) Stop() error {
	return nil
}

func (b *ExtensionRequestBreaker) RoundTripper(base http.RoundTripper) (http.RoundTripper, error) {
	if b.breaker == nil {
		return nil, fmt.Errorf("extension not initialized")
	}
	transport := breaker.Transport(b.breaker, breaker.DefaultResponseValidator, base)
	return transport, nil
}

func init() {
	pipeline.AddExtensionCreator("ext_request_breaker", func() pipeline.Extension {
		return &ExtensionRequestBreaker{
			FailureRatio:    0.10,
			WindowInSeconds: 10,
		}
	})
}
