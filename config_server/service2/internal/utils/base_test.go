package utils

import (
	"fmt"
	"testing"
)

func TestSlice(t *testing.T) {
	var arr []int

	s := arr
	s = append(s, 1)
	s = append(s, 2)
	AddSlice(s)
	fmt.Println(s)
	//fmt.Println(cap(arr))
	//fmt.Println(cap(s))
}

func AddSlice(arr []int) {
	arr = append(arr, 123)
}
