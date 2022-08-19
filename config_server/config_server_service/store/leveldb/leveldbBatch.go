package leveldb

import (
	"github.com/alibaba/ilogtail/config_server/config_server_service/setting"
	"github.com/syndtr/goleveldb/leveldb"
)

type LeveldbBatch struct {
	batch *leveldb.Batch
}

func (b *LeveldbBatch) Add(table string, entityKey string, entity interface{}) error {
	key := generateKey(table, entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}
	b.batch.Put(key, value)
	return nil
}

func (b *LeveldbBatch) Mod(table string, entityKey string, entity interface{}) error {
	key := generateKey(table, entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}
	b.batch.Put(key, value)
	return nil
}

func (b *LeveldbBatch) Delete(table string, entityKey string) error {
	key := generateKey(table, entityKey)
	b.batch.Delete(key)
	return nil
}

func (b *LeveldbBatch) Write() error {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return err
	}
	return db.Write(b.batch, nil)
}
