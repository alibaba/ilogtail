package setting

import (
	"reflect"
	"sync"

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

var setOnce sync.Once

var settingFile string = "./setting/setting.json"

/*
Change setting file's path
*/
func SetSettingPath(path string) {
	settingFile = path
}

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
	t := reflect.TypeOf(mySetting)
	v := reflect.ValueOf(mySetting).Elem()
	t = v.Type()

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
	common.ReadJson(settingFile, mySetting)
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
