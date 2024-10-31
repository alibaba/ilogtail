package utils

import (
	"sync"
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

func ParallelProcessSlice[T any](arr []T, fun func(int, T)) {
	if arr == nil || len(arr) == 0 {
		return
	}
	var wg sync.WaitGroup

	for i, v := range arr {
		wg.Add(1)
		go func(index int, value T) {
			defer wg.Done()
			fun(index, value)
		}(i, v)
	}

	wg.Wait()
}

func ParallelProcessTask[T any](parameter T, tasks ...func(T)) {
	if tasks == nil || len(tasks) == 0 {
		return
	}

	var wg sync.WaitGroup
	for i, task := range tasks {
		wg.Add(1)
		go func(index int, parameter T, taskFunc func(T)) {
			defer wg.Done()
			taskFunc(parameter)
		}(i, parameter, task)
	}
	wg.Wait()
}
