package helper

import (
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestConvertCreatorMapToString(t *testing.T) {
	var buffer strings.Builder

	testcases := []struct {
		input    map[string]interface{}
		expected string
	}{
		{map[string]interface{}{}, ""},
		{map[string]interface{}{"a": 1}, "a"},
		{map[string]interface{}{"a": 1, "b": 2}, "a|b"},
		{map[string]interface{}{"a": 1, "b": 2, "c": 3}, "a|b|c"},
	}

	for _, test := range testcases {
		ConvertCreatorMapToString(&buffer, test.input)
		assert.Equal(t, test.expected, buffer.String())
		buffer.Reset()
	}
}
