package manager

import "strings"

// Dichotomous lookup
func findPre(arr []string, name string) (int, bool) {
	var l = 0
	var r = len(arr)
	for l < r {
		mid := (l + r) / 2
		f := strings.Compare(arr[mid], name)
		if f == 0 {
			return mid, true
		}
		if f > 0 {
			r = mid
		}
		if f < 0 {
			l = mid + 1
		}
	}
	return l, false
}

func IsContain(items []interface{}, item interface{}) bool {
	for _, eachItem := range items {
		if eachItem == item {
			return true
		}
	}
	return false
}
