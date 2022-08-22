package store

import (
	"sync"

	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	database "github.com/alibaba/ilogtail/config_server/service/store/interface_database"
	"github.com/alibaba/ilogtail/config_server/service/store/leveldb"
)

// Data in database

/*
Store Factory
*/
func newStore(storeType string) database.Database {
	switch storeType {
	case "leveldb":
		return new(leveldb.LeveldbStore)
	default:
		panic("Wrong store type.")
	}
}

var myStore database.Database

var storeOnce sync.Once

/*
Create a singleton of store
*/
func GetStore() database.Database {
	storeOnce.Do(func() {
		myStore = newStore(setting.GetSetting().StoreMode)
		err := myStore.Connect()
		if err != nil {
			panic(err)
		}
	})
	return myStore
}

// Data in memory

type Memory struct {
	ConfigList map[string]*model.Config
}

var myMemory *Memory

var memoryOnce sync.Once

func GetMemory() *Memory {
	storeOnce.Do(func() {
		myMemory = new(Memory)
		myMemory.Init()
		err := myStore.Connect()
		if err != nil {
			panic(err)
		}
	})
	return myMemory
}

func (m *Memory) Init() {
	go m.Update()
}

func (m *Memory) Update() {

}
