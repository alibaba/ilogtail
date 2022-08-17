package store

import (
	"sync"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store/istore"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store/leveldb"
)

func newStore(storeType string) istore.IStore {
	switch storeType {
	case "leveldb":
		return new(leveldb.LeveldbStore)
	default:
		panic("Wrong store type.")
	}
}

var myStore istore.IStore

var storeOnce sync.Once

func GetStore() istore.IStore {
	storeOnce.Do(func() {
		myStore = newStore(setting.GetSetting().StoreMode)
	})
	return myStore
}
