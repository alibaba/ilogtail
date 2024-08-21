package utils

func AllEmpty(objs ...any) bool {
	if objs == nil {
		return true
	}
	for _, obj := range objs {
		if obj != nil {
			return false
		}
	}
	return true

}
