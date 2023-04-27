package limiter

import (
	"math"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

type config struct {
	maxInflight int
	trigger     trigger.Trigger
}

type Option func(c *config)

func WithTrigger(trigger trigger.Trigger) Option {
	return func(c *config) {
		c.trigger = trigger
	}
}

func WithMaxInflight(max int) Option {
	return func(c *config) {
		if max <= 0 {
			max = math.MaxInt
		}
		c.maxInflight = max
	}
}
