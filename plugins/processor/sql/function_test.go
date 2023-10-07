// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package sql

import (
	"fmt"
	"testing"
)

func TestCoalesce(t *testing.T) {
	coalesce := &Coalesce{}
	err := coalesce.Init("NULL", "NULL", "test")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	result, err := coalesce.Process("NULL", "NULL", "test")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	if result != "test" {
		t.Errorf("expected 'test', but got '%s'", result)
	}
}

func TestConcat(t *testing.T) {
	concat := &Concat{}
	err := concat.Init("test1", "test2", "test3")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	result, err := concat.Process("test1", "test2", "test3")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	if result != "test1test2test3" {
		t.Errorf("expected 'test1test2test3', but got '%s'", result)
	}
}

func TestConcat_ws(t *testing.T) {
	concatWs := &ConcatWs{}
	err := concatWs.Init(",", "test1", "test2", "test3")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	result, err := concatWs.Process(",", "test1", "test2", "test3")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	if result != "test1,test2,test3" {
		t.Errorf("expected 'test1,test2,test3', but got '%s'", result)
	}
}
func TestLtrim(t *testing.T) {
	input := "   hello world   "

	ltrim := &Ltrim{}
	err := ltrim.Init(input)
	if err != nil {
		t.Errorf("Error initializing ltrim: %v", err)
	}

	expectedOutput := "hello world   "
	output, err := ltrim.Process(input)
	if err != nil {
		t.Errorf("Error processing ltrim: %v", err)
	}
	if output != expectedOutput {
		t.Errorf("Unexpected output from ltrim. Expected: %v, but got: %v", expectedOutput, output)
	}
}

func TestRtrim(t *testing.T) {
	rtrim := &Rtrim{}
	input := "   hello world   "
	err := rtrim.Init(input)
	if err != nil {
		t.Errorf("Error initializing rtrim: %v", err)
	}

	expectedOutput := "   hello world"
	output, err := rtrim.Process(input)
	if err != nil {
		t.Errorf("Error processing rtrim: %v", err)
	}
	if output != expectedOutput {
		t.Errorf("Unexpected output from rtrim. Expected: %v, but got: %v", expectedOutput, output)
	}
}

func TestTrim(t *testing.T) {
	trim := &Trim{}
	input := "   hello world   "
	err := trim.Init(input)
	if err != nil {
		t.Errorf("Error initializing trim: %v", err)
	}

	expectedOutput := "hello world"
	output, err := trim.Process(input)
	if err != nil {
		t.Errorf("Error processing trim: %v", err)
	}
	if output != expectedOutput {
		t.Errorf("Unexpected output from trim. Expected: %v, but got: %v", expectedOutput, output)
	}
}

func TestLower(t *testing.T) {
	lower := &Lower{}
	input := "Hello World"
	err := lower.Init(input)
	if err != nil {
		t.Errorf("Error initializing lower: %v", err)
	}

	expectedOutput := "hello world"
	output, err := lower.Process(input)
	if err != nil {
		t.Errorf("Error processing lower: %v", err)
	}
	if output != expectedOutput {
		t.Errorf("Unexpected output from lower. Expected: %v, but got: %v", expectedOutput, output)
	}
}

func TestUpper(t *testing.T) {
	upper := &Upper{}
	input := "Hello World"
	err := upper.Init(input)
	if err != nil {
		t.Errorf("Error initializing upper: %v", err)
	}
	expectedOutput := "HELLO WORLD"
	output, err := upper.Process(input)
	if err != nil {
		t.Errorf("Error processing upper: %v", err)
	}
	if output != expectedOutput {
		t.Errorf("Unexpected output from upper. Expected: %v, but got: %v", expectedOutput, output)
	}
}

func TestSubstring(t *testing.T) {
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"hello world", "1", "5"}, "hello", nil},
		{[]string{"hello world", "7", "5"}, "world", nil},
		{[]string{"hello world", "7"}, "world", nil},
		{[]string{"hello world", "0", "5"}, "", nil},
		{[]string{"hello world", "-5", "5"}, "world", nil},
		{[]string{"hello world", "7", "50"}, "world", nil},
		{[]string{"hello world", "7", "-5"}, "", nil},
		{[]string{"hello world", "7", "0"}, "", nil},
		{[]string{"NULL", "7", "5"}, "NULL", nil},
		{[]string{"hello world"}, "", fmt.Errorf("substring: need at least 2 parameters")},
		{[]string{"hello world", "7", "five"}, "", fmt.Errorf("substring: length parameter should be a number")},
		{[]string{"hello world", "five", "5"}, "", fmt.Errorf("substring: position parameter should be a number")},
	}
	for _, tc := range testCases {
		substr := &Substring{}
		err := substr.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substr.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestSubstringIndex(t *testing.T) {

	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"www.example.com", ".", "2"}, "www.example", nil},
		{[]string{"www.example.com", ".", "-2"}, "example.com", nil},
		{[]string{"www.example.com", ".", "0"}, "", nil},
		{[]string{"www.example.com", ".", "5"}, "www.example.com", nil},
		{[]string{"www.example.com", ".", "-10"}, "www.example.com", nil},
		{[]string{"www.example.com", ",", "2"}, "www.example.com", nil},
		{[]string{"www.ex.am.ple.co.m", ".", "5"}, "www.ex.am.ple.co", nil},
		{[]string{"www.ex.am.ple.co.m", ".", "-5"}, "ex.am.ple.co.m", nil},
		{[]string{"www.example.com", ".", "100"}, "www.example.com", nil},
		{[]string{"NULL", ".", "2"}, "NULL", nil},
		{[]string{"www.example.com", "2"}, "", fmt.Errorf("substrindex: need at least 3 parameters")},
		// {[]string{"www.example.com", ".", "two"}, "", fmt.Errorf("substrindex: count parameter should be a number")},
	}
	for _, tc := range testCases {
		substrIndex := &SubstringIndex{}
		err := substrIndex.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substrIndex.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("SubstringIndex.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestLength(t *testing.T) {
	length := &Length{}
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"hello world"}, "11", nil},
		{[]string{"NULL"}, "NULL", nil},
		{[]string{}, "", fmt.Errorf("length: need at least 1 parameter")},
	}
	for _, tc := range testCases {
		actual, err := length.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("Length.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestLeft(t *testing.T) {
	f := &Left{}
	// Test normal case
	result, err := f.Process("hello world", "5")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "hello" {
		t.Errorf("Expected 'hello', but got '%s'", result)
	}
	// Test when length is greater than string length
	result, err = f.Process("hello", "10")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "hello" {
		t.Errorf("Expected 'hello', but got '%s'", result)
	}
	// Test when length is zero
	result, err = f.Process("hello", "0")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "" {
		t.Errorf("Expected '', but got '%s'", result)
	}
	// Test when string is NULL
	result, err = f.Process("NULL", "5")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "NULL" {
		t.Errorf("Expected 'NULL', but got '%s'", result)
	}
	// Test when length is not a number
	_, err = f.Process("hello", "abc")
	if err == nil {
		t.Errorf("Expected error, but got nil")
	}
}

func TestRight(t *testing.T) {
	f := &Right{}
	// Test normal case
	result, err := f.Process("hello world", "5")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "world" {
		t.Errorf("Expected 'world', but got '%s'", result)
	}
	// Test when length is greater than string length
	result, err = f.Process("hello", "10")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "hello" {
		t.Errorf("Expected 'hello', but got '%s'", result)
	}
	// Test when length is zero
	result, err = f.Process("hello", "0")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "" {
		t.Errorf("Expected '', but got '%s'", result)
	}
	// Test when string is NULL
	result, err = f.Process("NULL", "5")
	if err != nil {
		t.Errorf("Unexpected error: %v", err)
	}
	if result != "NULL" {
		t.Errorf("Expected 'NULL', but got '%s'", result)
	}
	// Test when length is not a number
	_, err = f.Process("hello", "abc")
	if err == nil {
		t.Errorf("Expected error, but got nil")
	}
}

func TestLocate(t *testing.T) {
	locate := &Locate{}
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"or", "hello world"}, "8", nil},
		{[]string{"or", "hello world", "5"}, "8", nil},
		{[]string{"or", "hello world", "0"}, "0", nil},
		{[]string{"or", "hello world", "-5"}, "0", nil},
		{[]string{"NULL", "hello world"}, "NULL", nil},
		{[]string{"or", "NULL"}, "NULL", nil},
		{[]string{"or"}, "", fmt.Errorf("locate: need at least 2 parameters")},
		{[]string{"or", "hello world", "five"}, "", fmt.Errorf("locate: position parameter should be a number")},
	}
	for _, tc := range testCases {
		actual, err := locate.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("Locate.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestRegexp_instr(t *testing.T) {
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"www.example.com", "e"}, "5", nil},
		{[]string{"www.example.com", "k"}, "NULL", nil},
		{[]string{"www.example.com", "e", "2"}, "5", nil},
		{[]string{"www.example.com", "e", "2", "2"}, "11", nil},
		{[]string{"www.example.com", "E", "2", "2"}, "NULL", nil},
		{[]string{"www.example.com", "e.", "2", "2", "1"}, "12", nil},
		{[]string{"www.example.com", "e.*", "2", "2", "1"}, "NULL", nil},
	}
	for _, tc := range testCases {
		substrIndex := &RegexpInstr{}
		err := substrIndex.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substrIndex.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("SubstringIndex.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestRegexp_like(t *testing.T) {
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"www.example.com", "e"}, "1", nil},
		{[]string{"www.example.com", "k"}, "0", nil},
		{[]string{"www.example.com", "e.*"}, "1", nil},
		{[]string{"www.example.com", "com."}, "0", nil},
	}
	for _, tc := range testCases {
		substrIndex := &RegexpLike{}
		err := substrIndex.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substrIndex.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("SubstringIndex.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestRegexp_replace(t *testing.T) {
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"www.example.com", "e", "h"}, "www.hxamplh.com", nil},
		{[]string{"www.example.com", "k", "h"}, "www.example.com", nil},
		{[]string{"www.example.com", "e", "h", "6"}, "www.examplh.com", nil},
		// now support param[0] ~ param[3], not support param[4]: occurrence, param[5]: mode
		// {[]string{"www.example.com", "e", "h", "2", "1"}, "www.hxample.com", nil},
		// {[]string{"www.example.com", "e", "h", "2", "2"}, "www.examplh.com", nil},
	}
	for _, tc := range testCases {
		substrIndex := &RegexpReplace{}
		err := substrIndex.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substrIndex.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("SubstringIndex.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestRegexp_substr(t *testing.T) {
	testCases := []struct {
		input    []string
		expected string
		err      error
	}{
		{[]string{"www.example.com", "\\..*\\."}, ".example.", nil},
		{[]string{"www.example.com", "k"}, "NULL", nil},
		{[]string{"www.example.com", "e..", "7"}, "e.c", nil},
		{[]string{"www.example.com", ".e", "2", "2"}, "le", nil},
		{[]string{"www.example.com", "E", "2", "2"}, "NULL", nil},
		{[]string{"www.example.com", "e.", "14", "2"}, "NULL", nil},

		// {[]string{"www.example.com", "e.", "2", "2", "1"}, "12", nil},
		// {[]string{"www.example.com", "e.*", "2", "2", "1"}, "NULL", nil},
	}
	for _, tc := range testCases {
		substrIndex := &RegexpSubstr{}
		err := substrIndex.Init(tc.input...)
		if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Unexpected error: %v", err)
			}
		}
		actual, err := substrIndex.Process(tc.input...)
		if actual != tc.expected {
			t.Errorf("SubstringIndex.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
		} else if err != nil {
			if err.Error() != tc.err.Error() {
				t.Errorf("Substring.Process(%v) = (%v, %v), expected (%v, %v)", tc.input, actual, err, tc.expected, tc.err)
			}
		}
	}
}

func TestReplace(t *testing.T) {
	r := &Replace{}
	cases := []struct {
		param      []string
		expected   string
		expectNull bool
	}{
		{[]string{"hello world", "world", "everyone"}, "hello everyone", false},
		{[]string{"hello world", "invalid regex", "everyone"}, "hello world", false},
		{[]string{"NULL", "world", "everyone"}, "NULL", true},
	}
	for _, c := range cases {
		actual, err := r.Process(c.param...)
		if c.expectNull {
			if actual != "NULL" {
				t.Errorf("Replace(%v) = %v, expected NULL", c.param, actual)
			}
		} else {
			if err != nil {
				t.Errorf("Replace(%v) returned error: %v", c.param, err)
			}
			if actual != c.expected {
				t.Errorf("Replace(%v) = %v, expected %v", c.param, actual, c.expected)
			}
		}
	}
}

func TestMd5(t *testing.T) {
	m := &Md5{}
	cases := []struct {
		param      []string
		expected   string
		expectNull bool
	}{
		{[]string{"hello world"}, "5eb63bbbe01eeed093cb22bb8f5acdc3", false},
		{[]string{"NULL"}, "NULL", true},
	}
	for _, c := range cases {
		actual, err := m.Process(c.param...)
		if c.expectNull {
			if actual != "NULL" {
				t.Errorf("Md5(%v) = %v, expected NULL", c.param, actual)
			}
		} else {
			if err != nil {
				t.Errorf("Md5(%v) returned error: %v", c.param, err)
			}
			if actual != c.expected {
				t.Errorf("Md5(%v) = %v, expected %v", c.param, actual, c.expected)
			}
		}
	}
}

func TestSha1(t *testing.T) {
	s := &Sha1{}
	cases := []struct {
		param      []string
		expected   string
		expectNull bool
	}{
		{[]string{"hello world"}, "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed", false},
		{[]string{"NULL"}, "NULL", true},
	}
	for _, c := range cases {
		actual, err := s.Process(c.param...)
		if c.expectNull {
			if actual != "NULL" {
				t.Errorf("Sha1(%v) = %v, expected NULL", c.param, actual)
			}
		} else {
			if err != nil {
				t.Errorf("Sha1(%v) returned error: %v", c.param, err)
			}
			if actual != c.expected {
				t.Errorf("Sha1(%v) = %v, expected %v", c.param, actual, c.expected)
			}
		}
	}
}

func TestSha2(t *testing.T) {
	cases := []struct {
		param      []string
		expected   string
		expectNull bool
	}{
		{[]string{"hello world", "256"}, "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", false},
		{[]string{"hello world", "224"}, "2f05477fc24bb4faefd86517156dafdecec45b8ad3cf2522a563582b", false},
		{[]string{"hello world", "384"}, "fdbd8e75a67f29f701a4e040385e2e23986303ea10239211af907fcbb83578b3e417cb71ce646efd0819dd8c088de1bd", false},
		{[]string{"hello world", "512"}, "309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f", false},
		{[]string{"NULL", "256"}, "NULL", true},
	}
	for _, c := range cases {
		s := &Sha2{}
		s.Init(c.param[0], c.param[1])
		actual, err := s.Process(c.param[0], c.param[1])
		if c.expectNull {
			if actual != "NULL" {
				t.Errorf("Sha2(%v) = %v, expected NULL", c.param, actual)
			}
		} else {
			if err != nil {
				t.Errorf("Sha2(%v) returned error: %v", c.param, err)
			}
			if actual != c.expected {
				t.Errorf("Sha2(%v) = %v, expected %v", c.param, actual, c.expected)
			}
		}
	}
}

func TestTo_base64(t *testing.T) {
	tob := &ToBase64{}
	cases := []struct {
		param      []string
		expected   string
		expectNull bool
	}{
		{[]string{"hello world"}, "aGVsbG8gd29ybGQ=", false},
		{[]string{"NULL"}, "NULL", true},
	}
	for _, c := range cases {
		actual, err := tob.Process(c.param...)
		if c.expectNull {
			if actual != "NULL" {
				t.Errorf("To_base64(%v) = %v, expected NULL", c.param, actual)
			}
		} else {
			if err != nil {
				t.Errorf("To_base64(%v) returned error: %v", c.param, err)
			}
			if actual != c.expected {
				t.Errorf("To_base64(%v) = %v, expected %v", c.param, actual, c.expected)
			}
		}
	}
}
func TestIf(t *testing.T) {
	f := &If{}
	result, err := f.Process("true", "true", "false")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	if result != "true" {
		t.Errorf("unexpected result: got %s, want true", result)
	}
	result, err = f.Process("false", "true", "false")
	if err != nil {
		t.Errorf("unexpected error: %v", err)
	}
	if result != "false" {
		t.Errorf("unexpected result: got %s, want false", result)
	}
}
