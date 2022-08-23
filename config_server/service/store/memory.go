package store

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
)

// Data in memory

type Memory struct {
	ConfigList        map[string]*model.Config
	AgentMessageQueue common.Queue
}

var myMemory *Memory

func GetMemory() *Memory {
	return myMemory
}

// init

func init() {
	myStore = newStore(setting.GetSetting().StoreMode)
	err := myStore.Connect()
	if err != nil {
		panic(err)
	}

	myMemory = new(Memory)
	myMemory.ConfigList = make(map[string]*model.Config)
}
