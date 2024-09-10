package utils

import "sync"

func ParallelProcessSlice[T any](arr []T, fun func(int, T)) {
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
