package utils

import (
	"time"
)

func TimedTask(timeLimit int64, task func(int64)) {
	if timeLimit <= 0 {
		panic("timeLimit 不能小于等于0")
	}
	timeLimitNano := timeLimit * int64(time.Second)

	ticker := time.NewTicker(time.Duration(timeLimitNano))
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			task(timeLimit)
		}
	}
}
