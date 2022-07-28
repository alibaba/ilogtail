package store

import (
	"sync"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/tool"
)

const BaseSettingFile string = "./base_setting.json"

type baseSetting struct {
	RunMode   string `json:"run_mode"`   // "cluster" cluster mode, "alone" stand-alone mode
	StoreMode string `json:"store_mode"` // "file" file mode, "leveldb" leveldb mode
	Port      string `json:"port"`       //
}

var myBaseSetting *baseSetting

var setOnce sync.Once

func GetMyBaseSetting() *baseSetting {
	setOnce.Do(func() {
		myBaseSetting = new(baseSetting)
		tool.ReadJson(BaseSettingFile, myBaseSetting)
	})
	return myBaseSetting
}

func UpdateMyBasesetting() {
	tool.ReadJson(BaseSettingFile, myBaseSetting)
}
