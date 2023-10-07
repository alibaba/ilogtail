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
	"crypto/md5"  // #nosec
	"crypto/sha1" // #nosec
	"crypto/sha256"
	"crypto/sha512"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"regexp"
	"strconv"
	"strings"
)

/** scalar functions**/

// FunctionMap is a map of all scalar functions
var FunctionMap = map[string]func() Function{}

// coalesce
type Coalesce struct{}

func (f *Coalesce) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("coalesce: need at least 1 parameter")
	}
	return nil
}

func (f *Coalesce) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("coalesce: need at least 1 parameter")
	}
	for _, v := range param {
		if strings.ToUpper(v) != "NULL" {
			return v, nil
		}
	}
	return "NULL", nil
}

func (f *Coalesce) Name() string {
	return "coalesce"
}

func init() {
	FunctionMap["coalesce"] = func() Function {
		return &Coalesce{}
	}
}

// concat
type Concat struct{}

func (f *Concat) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("concat: need at least 1 parameter")
	}
	return nil
}

// if one of the parameters is NULL, return NULL.
func (f *Concat) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("concat: need at least 1 parameter")
	}
	var result string
	for _, arg := range param {
		if arg == "NULL" {
			return "NULL", nil
		}
		result += arg
	}
	return result, nil
}

func (f *Concat) Name() string {
	return "concat"
}

func init() {
	FunctionMap["concat"] = func() Function {
		return &Concat{}
	}
}

// concat_ws
type ConcatWs struct{}

func (f *ConcatWs) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("concat_ws: need at least 2 parameters")
	}
	return nil
}

// if separator is NULL, return NULL. if string1, string2, ..., stringN has NULL, ignore it.
func (f *ConcatWs) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("concat_ws: need at least 2 parameters")
	}
	sep := param[0]
	if sep == "NULL" {
		return "NULL", nil
	}
	var result string
	for _, arg := range param[1:] {
		if arg == "NULL" {
			continue
		}
		if result == "" {
			result += arg
		} else {
			result += sep + arg
		}
	}
	return result, nil
}

func (f *ConcatWs) Name() string {
	return "concat_ws"
}

func init() {
	FunctionMap["concat_ws"] = func() Function {
		return &ConcatWs{}
	}
}

// ltrim
type Ltrim struct{}

func (f *Ltrim) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("ltrim: need at least 1 parameter")
	}
	return nil
}

func (f *Ltrim) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("ltrim: need at least 1 parameter")
	}
	return strings.TrimLeft(param[0], " "), nil
}

func (f *Ltrim) Name() string {
	return "ltrim"
}

func init() {
	FunctionMap["ltrim"] = func() Function {
		return &Ltrim{}
	}
}

// rtrim
type Rtrim struct{}

func (f *Rtrim) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("rtrim: need at least 1 parameter")
	}
	return nil
}

func (f *Rtrim) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("rtrim: need at least 1 parameter")
	}
	return strings.TrimRight(param[0], " "), nil
}

func (f *Rtrim) Name() string {
	return "rtrim"
}

func init() {
	FunctionMap["rtrim"] = func() Function {
		return &Rtrim{}
	}
}

// trim
type Trim struct{}

func (f *Trim) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("trim: need at least 1 parameter")
	}
	return nil
}

// now only support trim(string)
func (f *Trim) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("trim: need at least 1 parameter")
	}
	return strings.Trim(param[0], " "), nil
}

func (f *Trim) Name() string {
	return "trim"
}

func init() {
	FunctionMap["trim"] = func() Function {
		return &Trim{}
	}
}

// lower
type Lower struct{}

func (f *Lower) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("lower: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Lower) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("lower: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	return strings.ToLower(param[0]), nil
}

func (f *Lower) Name() string {
	return "lower"
}

func init() {
	FunctionMap["lower"] = func() Function {
		return &Lower{}
	}
}

// upper
type Upper struct{}

func (f *Upper) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("upper: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Upper) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("upper: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	return strings.ToUpper(param[0]), nil
}

func (f *Upper) Name() string {
	return "upper"
}

func init() {
	FunctionMap["upper"] = func() Function {
		return &Upper{}
	}
}

// substring
type Substring struct{}

func (f *Substring) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("substring: need at least 2 parameters")
	}
	return nil
}

// now only support substring(string, start [,length]), not support substring(string FROM start [FOR length])
// if pos is 0 or greater than the length of the original string, SUBSTRING() returns an empty string.
// if pos is negative, count from the end of the string.
// if pos + len is greater than the length of the original string, SUBSTRING() returns the portion of the string that begins with pos.
// if str is NULL, return NULL.
func (f *Substring) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("substring: need at least 2 parameters")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	// param[1]: start search position;  default: 1
	// position := 1
	// var err error
	position, err := strconv.Atoi(param[1])
	if err != nil {
		return "", fmt.Errorf("substring: position parameter should be a number")
	}
	if position == 0 || position > len(param[0]) {
		return "", nil
	}
	// param[2]: length;  default: the rest of the string
	length := len(param[0])
	if len(param) >= 3 {
		if param[2] == "NULL" {
			return "NULL", nil
		}
		var err error
		length, err = strconv.Atoi(param[2])
		if err != nil {
			return "", fmt.Errorf("substring: length parameter should be a number")
		}
		if length <= 0 {
			return "", nil
		}
	}
	if position < 0 {
		position = len(param[0]) + position + 1
	}
	if position+length > len(param[0]) {
		return param[0][position-1:], nil // to do: check whether the result is right
	}
	return param[0][position-1 : position+length-1], nil
}

func (f *Substring) Name() string {
	return "substring"
}

func init() {
	FunctionMap["substring"] = func() Function {
		return &Substring{}
	}
}

// substring_index
type SubstringIndex struct{}

func (f *SubstringIndex) Init(param ...string) error {
	if len(param) < 3 {
		return fmt.Errorf("substrindex: need at least 3 parameters")
	}
	return nil
}

// substring_index(str,delim,count) return the substring from 'str' before 'count' occurrences of the delimiter delim.
func (f *SubstringIndex) Process(param ...string) (string, error) {
	if len(param) < 3 {
		return "", fmt.Errorf("substrindex: need at least 3 parameters")
	}
	// param[0]: str;  necessary
	str := param[0]
	if str == "NULL" {
		return "NULL", nil
	}
	// param[2]: count;  necessary
	count, err := strconv.Atoi(param[2])
	if err != nil {
		return "", err
	}
	switch {
	case count == 0:
		return "", nil
	case count > 0:
		// param[1]: delim;  necessary
		delim := param[1]
		pos := 0
		for i := 0; i < count; i++ {
			posIn := strings.Index(str[pos:], delim)
			if posIn == -1 {
				return str, nil
			}
			pos += posIn + 1
		}
		return str[:pos-1], nil
	case count < 0:
		count = -count
		// param[1]: delim;  necessary
		delim := param[1]
		pos := len(str)
		for i := 0; i < count; i++ {
			pos = strings.LastIndex(str[:pos], delim)
			if pos == -1 {
				return str, nil
			}
		}
		return str[pos+1:], nil
	}
	return "", fmt.Errorf("substrindex: count parameter should be a number")
}

func (f *SubstringIndex) Name() string {
	return "substring_index"
}

func init() {
	FunctionMap["substring_index"] = func() Function {
		return &SubstringIndex{}
	}
}

// length
type Length struct{}

func (f *Length) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("length: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Length) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("length: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	return strconv.Itoa(len(param[0])), nil // to do: check whether the semantics is right
}

func (f *Length) Name() string {
	return "length"
}

func init() {
	FunctionMap["length"] = func() Function {
		return &Length{}
	}
}

// locate
type Locate struct{}

func (f *Locate) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("locate: need at least 2 parameters")
	}
	return nil
}

// if str or substr is NULL, return NULL.
func (f *Locate) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("locate: need at least 2 parameters")
	}
	if param[0] == "NULL" || param[1] == "NULL" {
		return "NULL", nil
	}
	// param[2]: start search position;  default: 1
	position := 1
	if len(param) >= 3 {
		var err error
		position, err = strconv.Atoi(param[2])
		if err != nil {
			return "", fmt.Errorf("locate: position parameter should be a number")
		}
	}
	if position <= 0 {
		return "0", nil // to do: check whether the semantics is right
	} else if position > len(param[1]) {
		return "0", nil
	}
	return strconv.Itoa(strings.Index(param[1][position-1:], param[0]) + position), nil
}

func (f *Locate) Name() string {
	return "locate"
}

func init() {
	FunctionMap["locate"] = func() Function {
		return &Locate{}
	}
}

// left
type Left struct{}

func (f *Left) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("left: need at least 2 parameters")
	}
	return nil
}

// left() returns the leftmost length characters from the string str, or NULL if any argument is NULL.
func (f *Left) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("left: need at least 2 parameters")
	}
	if param[0] == "NULL" || param[1] == "NULL" {
		return "NULL", nil
	}
	// param[1]: length;  necessary
	length, err := strconv.Atoi(param[1])
	if err != nil {
		return "", fmt.Errorf("left: length parameter should be a number")
	}
	if length <= 0 {
		return "", nil
	} else if length > len(param[0]) {
		return param[0], nil
	}
	return param[0][:length], nil
}

func (f *Left) Name() string {
	return "left"
}

func init() {
	FunctionMap["left"] = func() Function {
		return &Left{}
	}
}

// right
type Right struct{}

func (f *Right) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("right: need at least 2 parameters")
	}
	return nil
}

// right() returns the rightmost length characters from the string str, or NULL if any argument is NULL.
func (f *Right) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("right: need at least 2 parameters")
	}
	if param[0] == "NULL" || param[1] == "NULL" {
		return "NULL", nil
	}
	// param[1]: length;  necessary
	length, err := strconv.Atoi(param[1])
	if err != nil {
		return "", fmt.Errorf("right: length parameter should be a number")
	}
	if length <= 0 {
		return "", nil
	} else if length > len(param[0]) {
		return param[0], nil
	}
	return param[0][len(param[0])-length:], nil
}

func (f *Right) Name() string {
	return "right"
}

func init() {
	FunctionMap["right"] = func() Function {
		return &Right{}
	}
}

// regexp_instr
type RegexpInstr struct {
	Pattern *regexp.Regexp
}

func (f *RegexpInstr) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("regexp_instr: need at least 2 parameters")
	}
	// `(?s)` change the meaning of `.` in Golang to match the every character, and the default meaning is not match a newline.
	reg, err := regexp.Compile("(?s)" + param[1])
	if err != nil {
		return fmt.Errorf("regexp_instr error, regex %s, error %v", param[1], err)
	}
	f.Pattern = reg
	return nil
}

// now support param[0] ~ param[4], not support param[5]: mode
func (f *RegexpInstr) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("regexp_instr: need at least 2 parameters")
	}
	if f.Pattern != nil {
		if param[0] == "NULL" || param[1] == "NULL" {
			return "NULL", nil
		}
		returnOption := 0
		if len(param) >= 5 {
			var err error
			returnOption, err = strconv.Atoi(param[4])
			if err != nil {
				return "", fmt.Errorf("regexp_instr: returnOption parameter should be a number")
			}
		}
		// param[2]: start search position;  default: 1
		position := 1
		if len(param) >= 3 {
			var err error
			position, err = strconv.Atoi(param[2])
			if err != nil {
				return "", fmt.Errorf("regexp_instr: position parameter should be a number")
			}
		}
		// param[3]: occurrence;  default: 1
		occurrence := 1
		if len(param) >= 4 {
			var err error
			occurrence, err = strconv.Atoi(param[3])
			if err != nil {
				return "", fmt.Errorf("regexp_instr: occurrence parameter should be a number")
			}
		}
		if occurrence == 1 {
			res := f.Pattern.FindStringIndex(param[0][position-1:])
			if res == nil {
				return "NULL", nil
			}
			return strconv.Itoa(res[returnOption] + position - returnOption), nil
		}
		res := f.Pattern.FindAllStringIndex(param[0][position-1:], -1)
		if res == nil || len(res) < occurrence {
			return "NULL", nil
		}
		return strconv.Itoa(res[occurrence-1][returnOption] + position - returnOption), nil
	}
	return "", fmt.Errorf("regexp_instr: compile pattern error when Init")
}

func (f *RegexpInstr) Name() string {
	return "regexp_instr"
}

func init() {
	FunctionMap["regexp_instr"] = func() Function {
		return &RegexpInstr{}
	}
}

// regexp_like
type RegexpLike struct {
	Pattern *regexp.Regexp
}

func (f *RegexpLike) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("regexp_like: need at least 2 parameters")
	}
	// `(?s)` change the meaning of `.` in Golang to match the every character, and the default meaning is not match a newline.
	reg, err := regexp.Compile("(?s)" + param[1])
	if err != nil {
		return fmt.Errorf("regexp_like error, regex %s, error %v", param[1], err)
	}
	f.Pattern = reg
	return nil
}

// now support param[0] ~ param[1], not support param[2]: mode
// regexp_like(str, regexp) return 1 if str matches regexp; otherwise it returns 0. If either argument is NULL, the result is NULL.
func (f *RegexpLike) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("regexp_like: need at least 2 parameters")
	}
	if f.Pattern != nil {
		if param[0] == "NULL" || param[1] == "NULL" {
			return "NULL", nil
		}
		if f.Pattern.MatchString(param[0]) {
			return "1", nil
		}
		return "0", nil
	}
	return "", fmt.Errorf("regexp_like: compile pattern error when Init")
}

func (f *RegexpLike) Name() string {
	return "regexp_like"
}

func init() {
	FunctionMap["regexp_like"] = func() Function {
		return &RegexpLike{}
	}
}

// regexp_replace
type RegexpReplace struct {
	Pattern *regexp.Regexp
}

func (f *RegexpReplace) Init(param ...string) error {
	if len(param) < 3 {
		return fmt.Errorf("regexp_replace: need at least 3 parameters")
	}
	// `(?s)` change the meaning of `.` in Golang to match the every character, and the default meaning is not match a newline.
	reg, err := regexp.Compile("(?s)" + param[1])
	if err != nil {
		return fmt.Errorf("regexp_replace error, regex %s, error %v", param[1], err)
	}
	f.Pattern = reg
	return nil
}

// now support param[0] ~ param[3], not support param[4]: occurrence, param[5]: mode
func (f *RegexpReplace) Process(param ...string) (string, error) {
	if len(param) < 3 {
		return "", fmt.Errorf("regexp_replace: need at least 3 parameters")
	}
	if f.Pattern != nil {
		if param[0] == "NULL" || param[1] == "NULL" || param[2] == "NULL" {
			return "NULL", nil
		}
		// param[3]: start search position;  default: 1
		position := 1
		if len(param) >= 4 {
			var err error
			position, err = strconv.Atoi(param[3])
			if err != nil {
				return "", fmt.Errorf("regexp_substr: position parameter should be a number")
			}
		}
		return param[0][:position-1] + f.Pattern.ReplaceAllString(param[0][position-1:], param[2]), nil
	}
	return "", fmt.Errorf("regexp_replace: compile pattern error when Init")
}

func (f *RegexpReplace) Name() string {
	return "regexp_replace"
}

func init() {
	FunctionMap["regexp_replace"] = func() Function {
		return &RegexpReplace{}
	}
}

// regexp_substr
type RegexpSubstr struct {
	Pattern *regexp.Regexp
}

func (f *RegexpSubstr) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("regexp_substr: need at least 2 parameters")
	}
	// `(?s)` change the meaning of `.` in Golang to match the every character, and the default meaning is not match a newline.
	reg, err := regexp.Compile("(?s)" + param[1])
	if err != nil {
		return fmt.Errorf("regexp_substr error, regex %s, error %v", param[1], err)
	}
	f.Pattern = reg
	return nil
}

// now support param[0] ~ param[3], not support param[4]: mode
func (f *RegexpSubstr) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("regexp_substr: need at least 2 parameters")
	}
	if f.Pattern != nil {
		if param[0] == "NULL" || param[1] == "NULL" {
			return "NULL", nil
		}
		// param[2]: start search position;  default: 1
		position := 1
		if len(param) >= 3 {
			var err error
			position, err = strconv.Atoi(param[2])
			if err != nil {
				return "", fmt.Errorf("regexp_substr: position parameter should be a number")
			}
		}
		// param[3]: occurrence;  default: 1
		occurrence := 1
		if len(param) >= 4 {
			var err error
			occurrence, err = strconv.Atoi(param[3])
			if err != nil {
				return "", fmt.Errorf("regexp_substr: occurrence parameter should be a number")
			}
		}
		if occurrence == 1 {
			res := f.Pattern.FindString(param[0][position-1:])
			if res == "" {
				return "NULL", nil
			}
			return res, nil
		}
		res := f.Pattern.FindAllString(param[0][position-1:], -1)
		if res == nil || len(res) < occurrence {
			return "NULL", nil
		}
		return res[occurrence-1], nil
	}
	return "", fmt.Errorf("regexp_substr: compile pattern error when Init")
}

func (f *RegexpSubstr) Name() string {
	return "regexp_substr"
}

func init() {
	FunctionMap["regexp_substr"] = func() Function {
		return &RegexpSubstr{}
	}
}

// replace
type Replace struct{}

func (f *Replace) Init(param ...string) error {
	if len(param) < 3 {
		return fmt.Errorf("replace: need at least 3 parameters")
	}
	return nil
}

// if str, from_str or to_str is NULL, return NULL.
func (f *Replace) Process(param ...string) (string, error) {
	if len(param) < 3 {
		return "", fmt.Errorf("replace: need at least 3 parameters")
	}
	if param[0] == "NULL" || param[1] == "NULL" || param[2] == "NULL" {
		return "NULL", nil
	}
	return strings.ReplaceAll(param[0], param[1], param[2]), nil
}

func (f *Replace) Name() string {
	return "replace"
}

func init() {
	FunctionMap["replace"] = func() Function {
		return &Replace{}
	}
}

// md5
type Md5 struct{}

func (f *Md5) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("md5: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Md5) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("md5: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	hash := md5.Sum([]byte(param[0])) // #nosec
	return hex.EncodeToString(hash[:]), nil
}

func (f *Md5) Name() string {
	return "md5"
}

func init() {
	FunctionMap["md5"] = func() Function {
		return &Md5{}
	}
}

// sha1
type Sha1 struct{}

func (f *Sha1) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("sha1: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Sha1) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("sha1: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	hash := sha1.Sum([]byte(param[0])) // #nosec
	return hex.EncodeToString(hash[:]), nil
}

func (f *Sha1) Name() string {
	return "sha1"
}

func init() {
	FunctionMap["sha1"] = func() Function {
		return &Sha1{}
	}
}

// sha2
type Sha2 struct {
	ShaLength int
}

func (f *Sha2) Init(param ...string) error {
	if len(param) < 2 {
		return fmt.Errorf("sha2: need at least 2 parameters")
	}
	// param[1]: length;  necessary
	var err error
	f.ShaLength, err = strconv.Atoi(param[1])
	if err != nil {
		return fmt.Errorf("sha2: length parameter should be a number")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *Sha2) Process(param ...string) (string, error) {
	if len(param) < 2 {
		return "", fmt.Errorf("sha2: need at least 2 parameters")
	}
	if param[0] == "NULL" || param[1] == "NULL" {
		return "NULL", nil
	}
	switch f.ShaLength {
	case 224:
		{
			hash := sha256.Sum224([]byte(param[0]))
			return hex.EncodeToString(hash[:]), nil
		}
	case 256, 0:
		{
			hash := sha256.Sum256([]byte(param[0]))
			return hex.EncodeToString(hash[:]), nil
		}
	case 384:
		{
			hash := sha512.Sum384([]byte(param[0]))
			return hex.EncodeToString(hash[:]), nil
		}
	case 512:
		{
			hash := sha512.Sum512([]byte(param[0]))
			return hex.EncodeToString(hash[:]), nil
		}
	default:
		return "", fmt.Errorf("sha2: length parameter should be 224 or 256 or 384 or 512 or 0(represents 256)")
	}
}

func (f *Sha2) Name() string {
	return "sha2"
}

func init() {
	FunctionMap["sha2"] = func() Function {
		return &Sha2{}
	}
}

// to_base64
type ToBase64 struct{}

func (f *ToBase64) Init(param ...string) error {
	if len(param) < 1 {
		return fmt.Errorf("to_base64: need at least 1 parameter")
	}
	return nil
}

// if str is NULL, return NULL.
func (f *ToBase64) Process(param ...string) (string, error) {
	if len(param) < 1 {
		return "", fmt.Errorf("to_base64: need at least 1 parameter")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	return base64.StdEncoding.EncodeToString([]byte(param[0])), nil
}

func (f *ToBase64) Name() string {
	return "to_base64"
}

func init() {
	FunctionMap["to_base64"] = func() Function {
		return &ToBase64{}
	}
}

// if
type If struct{}

func (f *If) Init(param ...string) error {
	if len(param) < 3 {
		return fmt.Errorf("if: need at least 3 parameters")
	}
	return nil
}

// if expr is true (expr <> 0 and expr <> NULL) then if() returns if_true, otherwise it returns if_false.
func (f *If) Process(param ...string) (string, error) {
	if len(param) < 3 {
		return "", fmt.Errorf("if: need at least 3 parameters")
	}
	if param[0] == "NULL" {
		return "NULL", nil
	}
	// 根据字符串param[0],判断这个字符串所代表的表达式的值是否为true
	switch param[0] {
	case "true":
		return param[1], nil
	case "false", "0", "":
		return param[2], nil
	default:
		return param[1], nil
		// return "", fmt.Errorf("if: condition expression error")
	}
}

func (f *If) Name() string {
	return "if"
}

func init() {
	FunctionMap["if"] = func() Function {
		return &If{}
	}
}
