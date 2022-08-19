package store

import (
	"sync"

	"github.com/alibaba/ilogtail/config_server/config_server_service/setting"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store/database"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store/leveldb"
)

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
