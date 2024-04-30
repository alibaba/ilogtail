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
	"testing"
)

func TestIsNumber(t *testing.T) {
	tests := []struct {
		input string
		want  bool
	}{
		{"123", true},
		{"-123", true},
		{"123.456", true},
		{"-123.456", true},
		{"0b1010", true},
		{"-0b1010", true},
		{"0o777", true},
		{"-0o777", true},
		{"0xdeadbeef", true},
		{"-0xdeadbeef", true},
		{"abc", false},
		{"", false},
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			got := isNumber(tt.input)
			if got != tt.want {
				t.Errorf("isNumber(%q) = %v, want %v", tt.input, got, tt.want)
			}
		})
	}
}

func TestIsZeroNumber(t *testing.T) {
	tests := []struct {
		input string
		want  bool
	}{
		{"0", true},
		{"-0", true},
		{"0.0", true},
		{"-0.0", true},
		{"0b0", true},
		{"-0b0", true},
		{"0o0", true},
		{"-0o0", true},
		{"0x0", true},
		{"-0x0", true},
		{"123", false},
		{"-123", false},
		{"123.456", false},
		{"-123.456", false},
		{"0b1010", false},
		{"-0b1010", false},
		{"0o777", false},
		{"-0o777", false},
		{"0xdeadbeef", false},
		{"-0xdeadbeef", false},
		{"abc", false},
		{"", false},
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			got := isZeroNumber(tt.input)
			if got != tt.want {
				t.Errorf("isZeroNumber(%q) = %v, want %v", tt.input, got, tt.want)
			}
		})
	}
}

func TestPreEvaluateLike(t *testing.T) {
	tests := []struct {
		pattern string
		input   string
		want    bool
	}{
		{"abc", "abc", true},
		{"abc", "abcd", false},
		{"abc%", "abc", true},
		{"abc%", "abcd", true},
		{"abc_", "abc", false},
		{"abc_", "abcd", true},
		{"%abc", "abc", true},
		{"%abc", "abcd", false},
		{"_abc", "abc", false},
		{"abc_", "abcd", true},
	}

	for _, tt := range tests {
		t.Run(tt.pattern, func(t *testing.T) {
			got, err := preEvaluateLike(tt.pattern)
			if err != nil {
				t.Errorf("preEvaluateLike(%q) returned error: %v", tt.pattern, err)
			}
			if got.MatchString(tt.input) != tt.want {
				t.Errorf("preEvaluateLike(%q).MatchString(%q) = %v, want %v", tt.pattern, tt.input, !tt.want, tt.want)
			}
		})
	}
}

func TestPreEvaluateRegex(t *testing.T) {
	tests := []struct {
		pattern string
		input   string
		want    bool
	}{
		{"abc", "abc", true},
		{"abc", "abcd", true},
		{"a.c", "abc", true},
		{"a.c", "abcd", true},
		{"a.*c", "abc", true},
		{"a.*c", "abcd", true},
		{"a.c", "abdc", false},
	}

	for _, tt := range tests {
		t.Run(tt.pattern, func(t *testing.T) {
			got, err := preEvaluateRegex(tt.pattern)
			if err != nil {
				t.Errorf("preEvaluateRegex(%q) returned error: %v", tt.pattern, err)
			}
			if got.MatchString(tt.input) != tt.want {
				t.Errorf("preEvaluateRegex(%q).MatchString(%q) = %v, want %v", tt.pattern, tt.input, !tt.want, tt.want)
			}
		})
	}
}

func TestEvaluateLike(t *testing.T) {
	tests := []struct {
		pattern string
		left    string
		right   string
		want    bool
	}{
		{"abc", "abc", "", true},
		{"abc", "abcd", "", false},
		{"abc%", "abc", "", true},
		{"abc%", "abcd", "", true},
		{"abc_", "abc", "", false},
		{"abc_", "abcd", "", true},
		{"%abc", "abc", "", true},
		{"%abc", "abcd", "", false},
		{"_abc", "abc", "", false},
		{"_abc", "abcd", "", false},
		{"", "abc", "abc", true},
		{"", "abc", "abcd", false},
		{"os is%", "os ism", "os is%", true},
	}

	for _, tt := range tests {
		t.Run(tt.pattern, func(t *testing.T) {
			p, err := preEvaluateLike(tt.pattern)
			if err != nil {
				t.Errorf("preEvaluateLike(%q) returned error: %v", tt.pattern, err)
			}
			got, _ := evaluateLike(tt.left, tt.right, p)
			if got != tt.want {
				t.Errorf("evaluateLike(%q, %q, %v) = %v, want %v", tt.left, tt.right, p, got, tt.want)
			}
		})
	}
}

func TestEvaluateRegex(t *testing.T) {
	tests := []struct {
		pattern string
		left    string
		right   string
		want    bool
	}{
		{"abc", "abc", "", true},
		{"abc", "abcd", "", true},
		{"a.c", "abc", "", true},
		{"a.c", "abcd", "", true},
		{"a.*c", "abc", "", true},
		{"a.*c", "abcd", "", true},
		{"a.c", "abdc", "", false},
		{"", "abc", "abc", true},
		{"", "abc", "abcd", false},
	}

	for _, tt := range tests {
		t.Run(tt.pattern, func(t *testing.T) {
			p, err := preEvaluateRegex(tt.pattern)
			if err != nil {
				t.Errorf("preEvaluateRegex(%q) returned error: %v", tt.pattern, err)
			}
			got, _ := evaluateRegex(tt.left, tt.right, p)
			if got != tt.want {
				t.Errorf("evaluateRegex(%q, %q, %v) = %v, want %v", tt.left, tt.right, p, got, tt.want)
			}
		})
	}
}
