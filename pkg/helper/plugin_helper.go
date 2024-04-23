package helper

import "strings"

const delimiter = '|'

func ConvertCreatorMapToString[T any](buffer *strings.Builder, s map[string]T) {
	for k := range s {
		if buffer.Len() > 0 {
			buffer.WriteRune(delimiter)
		}
		buffer.WriteString(k)
	}
}
