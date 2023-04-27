package limiter

import "time"

type RateLimiter interface {
	Allow() (bool, DoneFunc)
}

type DoneFunc func(err error) time.Duration
