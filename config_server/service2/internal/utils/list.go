package utils

func ContainElement[T comparable](arr []T, val T, eqCondition func(T, T) bool) bool {
	if arr == nil || len(arr) == 0 {
		return false
	}
	for _, element := range arr {
		if element == val || eqCondition != nil && eqCondition(element, val) {
			return true
		}
	}
	return false
}

func RemoveElement[T comparable](arr []T, val T, eqCondition func(T, T) bool) (bool, []T) {
	if arr == nil || len(arr) == 0 {
		return false, arr
	}
	for i, element := range arr {
		if element == val || eqCondition != nil && eqCondition(element, val) {
			return true, append(arr[:i], arr[i+1:]...)
		}
	}
	return false, arr
}
