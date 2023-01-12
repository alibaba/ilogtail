package helper

import (
	"github.com/stretchr/testify/require"
	"testing"
)

func TestReverseStringSlice(t *testing.T) {
	ss := []string{"a", "b"}
	ReverseStringSlice(ss)
	require.Equal(t, ss, []string{"b", "a"})
}
