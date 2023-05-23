package limiter

import (
	"math"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

type config struct {
	maxLimit     int
	minLimit     int
	initialLimit int
	trigger      trigger.Trigger
}

type Option func(c *config)

func WithTrigger(trigger trigger.Trigger) Option {
	return func(c *config) {
		c.trigger = trigger
	}
}

func WithMaxLimit(max int) Option {
	return func(c *config) {
		if max <= 0 {
			max = math.MaxInt
		}
		c.maxLimit = max
	}
}

func WithMinLimit(min int) Option {
	return func(c *config) {
		c.minLimit = min
	}
}

func WithInitialLimit(init int) Option {
	return func(c *config) {
		if init <= 0 {
			return
		}
		c.initialLimit = init
	}
}
