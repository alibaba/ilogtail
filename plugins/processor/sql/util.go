// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package sql

import (
	"fmt"
	"regexp"
	"strings"
)

func preEvaluateRegex(pattern string) (*regexp.Regexp, error) {
	if pattern == "" {
		return nil, nil
	}
	reg, err := regexp.Compile(pattern)
	if err != nil {
		return nil, fmt.Errorf("FILTER_INIT_ALARM init filter error, regex %s, error %v", pattern, err)
	}
	return reg, nil
}

func preEvaluateLike(pattern string) (*regexp.Regexp, error) {
	if pattern == "" {
		return nil, nil
	}
	pattern = strings.ReplaceAll(pattern, "%", ".*")
	pattern = strings.ReplaceAll(pattern, "_", ".")
	pattern = "^" + pattern + "$"
	reg, err := regexp.Compile(pattern)
	if err != nil {
		return nil, fmt.Errorf("FILTER_INIT_ALARM init filter error, like %s, error %v", pattern, err)
	}
	return reg, nil
}

func evaluateRegex(left string, right string, pattern *regexp.Regexp) (bool, error) {
	if pattern != nil {
		return pattern.MatchString(left), nil
	}
	if right == "" {
		return false, nil
	}
	found, err := regexp.MatchString(right, left)
	if err != nil {
		return false, err
	}
	return found, nil
}

func evaluateLike(left string, right string, pattern *regexp.Regexp) (bool, error) {
	if pattern != nil {
		return pattern.MatchString(left), nil
	}
	if right == "" {
		return false, nil
	}
	right = strings.ReplaceAll(right, "%", ".*")
	right = strings.ReplaceAll(right, "_", ".")
	right = "^" + right + "$"
	found, err := regexp.MatchString(right, left)
	if err != nil {
		return false, err
	}
	return found, nil
}

// decide whether a string is a number, including 2, 8, 10, 16 hexadecimal integers and floating point numbers
func isNumber(str string) bool {
	// 10 binary
	if matched, _ := regexp.MatchString("^-?\\d+$", str); matched {
		return true
	}
	// floating point number
	if matched, _ := regexp.MatchString("^-?\\d+\\.\\d+$", str); matched {
		return true
	}
	// 2 binary
	if matched, _ := regexp.MatchString("^-?0[bB][01]+$", str); matched {
		return true
	}
	// 8 binary
	if matched, _ := regexp.MatchString("^-?0[oO][0-7]+$", str); matched {
		return true
	}
	// 16 binary
	if matched, _ := regexp.MatchString("^-?0[xX][0-9a-fA-F]+$", str); matched {
		return true
	}
	return false
}

// decide whether a string is a number, including 2, 8, 10, 16 hexadecimal integers and floating point numbers. if this str is a number, decide whether it is zero
func isZeroNumber(str string) bool {
	// 10 binary
	if matched, _ := regexp.MatchString("^-?0+$", str); matched {
		return true
	}
	// floating point number
	if matched, _ := regexp.MatchString("^-?0+\\.0+$", str); matched {
		return true
	}
	// 2 binary 0
	if matched, _ := regexp.MatchString("^-?0[bB][0]+$", str); matched {
		return true
	}
	// 8 binary 0
	if matched, _ := regexp.MatchString("^-?0[oO][0]+$", str); matched {
		return true
	}
	// 16 binary 0
	if matched, _ := regexp.MatchString("^-?0[xX][0]+$", str); matched {
		return true
	}
	return false
}
