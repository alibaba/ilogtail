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

package setting

import (
	"reflect"

	"github.com/alibaba/ilogtail/config_server/service/common"
)

type setting struct {
	Ip                  string `json:"ip"`                    // default: "127.0.0.1"
	StoreMode           string `json:"store_mode"`            // support "leveldb", "mysql"
	Port                string `json:"port"`                  // default: "8899"
	DbPath              string `json:"db_path"`               // default: "./DB"
	AgentUpdateInterval int    `json:"agent_update_interval"` // default: 1s
	ConfigSyncInterval  int    `json:"config_sync_interval"`  // default: 3s
}

var mySetting *setting

var settingFile string = "./setting/setting.json"

/*
Create a singleton of setting
*/
func GetSetting() setting {
	return *mySetting
}

/*
Use map to update setting.
For example, if the value of map is {store_mode:"mysql"},
this function will change "mySetting's StoreMode" to "mysql".
*/
func UpdateSetting(tagMap map[string]interface{}) {
	v := reflect.ValueOf(mySetting).Elem()
	t := v.Type()

	fieldNum := v.NumField()
	for i := 0; i < fieldNum; i++ {
		fieldInfo := t.Field(i)
		tag := fieldInfo.Tag.Get("json")
		if tag == "" {
			continue
		}
		if value, ok := tagMap[tag]; ok {
			if reflect.ValueOf(value).Type() == v.FieldByName(fieldInfo.Name).Type() {
				v.FieldByName(fieldInfo.Name).Set(reflect.ValueOf(value))
			}
		}
	}
	common.WriteJson(settingFile, mySetting)
}

func init() {
	mySetting = new(setting)
	err := common.ReadJson(settingFile, mySetting)
	if err != nil {
		panic(err)
	}
	if mySetting.Ip == "" {
		mySetting.Ip = "127.0.0.1"
	}
	if mySetting.Port == "" {
		mySetting.Port = "8899"
	}
	if mySetting.StoreMode == "" {
		panic("Please set store mode")
	}
	if mySetting.AgentUpdateInterval == 0 {
		mySetting.AgentUpdateInterval = 1
	}
	if mySetting.ConfigSyncInterval == 0 {
		mySetting.ConfigSyncInterval = 3
	}
}
