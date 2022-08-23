package store

import (
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

/*
Create a singleton of store
*/
func GetStore() database.Database {
	return myStore
}

// batch

func CreateBacth() *database.Batch {
	return new(database.Batch)
}
