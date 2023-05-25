package limiter

import "time"

type RateLimiter interface {
	Allow() (bool, DoneFunc)
	Stop()
}

type DoneFunc func(err error) time.Duration
