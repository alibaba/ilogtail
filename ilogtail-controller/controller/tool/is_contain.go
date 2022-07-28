package tool

func IsContain(items []interface{}, item interface{}) bool {
	for _, eachItem := range items {
		if eachItem == item {
			return true
		}
	}
	return false
}
