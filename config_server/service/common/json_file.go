// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package common

import (
	"encoding/json"
	"os"
)

func ReadJson(filePath string, ptr interface{}) error {
	file, err := os.Open(filePath)
	if err != nil {
		return err
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	err = decoder.Decode(&ptr)
	if err != nil {
		return err
	}
	return nil
}

func WriteJson(filePath string, data interface{}) error {
	dataString, err := json.Marshal(data)
	if err != nil {
		return err
	}

	file, err := os.OpenFile(filePath, os.O_RDWR|os.O_TRUNC, os.ModeAppend)
	if err != nil {
		return err
	}

	_, err = file.WriteString(string(dataString))
	if err != nil {
		return err
	}
	defer file.Close()
	return nil
}
