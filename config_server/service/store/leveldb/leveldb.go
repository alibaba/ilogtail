package leveldb

import (
	"encoding/json"
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	database "github.com/alibaba/ilogtail/config_server/service/store/interface_database"
	"github.com/syndtr/goleveldb/leveldb"
)

var content = []string{
	common.LABEL_ALARM,
	common.LABEL_CONFIG,
	common.LABEL_MACHINE,
	common.LABEL_MACHINEGROUP,
}

type LeveldbStore struct {
	db map[string]*leveldb.DB
}

func (l *LeveldbStore) Connect() error {
	var err error
	for _, c := range content {
		l.db[c], err = leveldb.OpenFile(setting.GetSetting().LeveldbStorePath+"/"+c, nil)
		if err != nil {
			return err
		}
	}

	return nil
}

func (l *LeveldbStore) GetMode() string {
	return "leveldb"
}

func (l *LeveldbStore) Close() error {
	for _, db := range l.db {
		err := db.Close()
		if err != nil {
			return err
		}
	}
	return nil
}

func (l *LeveldbStore) Get(table string, entityKey string) (interface{}, error) {
	value, err := l.db[table].Get([]byte(generateKey(entityKey)), nil)
	if err != nil {
		return nil, err
	}
	return parseValue(table, value), nil
}

func (l *LeveldbStore) Add(table string, entityKey string, entity interface{}) error {
	key := generateKey(entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}

	err = l.db[table].Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) Update(table string, entityKey string, entity interface{}) error {
	key := generateKey(entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}

	err = l.db[table].Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) Has(table string, entityKey string) (bool, error) {
	key := generateKey(entityKey)
	ok, err := l.db[table].Has(key, nil)
	if err != nil {
		return false, err
	}
	return ok, nil
}

func (l *LeveldbStore) Delete(table string, entityKey string) error {
	key := generateKey(entityKey)
	err := l.db[table].Delete(key, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) GetAll(table string) ([]interface{}, error) {
	ans := make([]interface{}, 0)

	iter := l.db[table].NewIterator(nil, nil)
	iter.Next()
	for iter.Next() {
		ans = append(ans, parseValue(table, iter.Value()))
	}

	iter.Release()
	err := iter.Error()
	if err != nil {
		return nil, err
	}
	return ans, nil
}

func (l *LeveldbStore) WriteBatch(batch *database.Batch) error {
	batchTemp := *batch
	var leveldbBatch map[string]*leveldb.Batch

	for !batch.Empty() {
		data := batch.Pop()
		key := generateKey(data.Key)

		if _, ok := leveldbBatch[data.Table]; !ok {
			leveldbBatch[data.Table] = new(leveldb.Batch)
		}

		if data.Opt == "D" {
			leveldbBatch[data.Table].Delete(key)
		} else {
			value, err := generateValue(data.Value)
			if err != nil {
				*batch = batchTemp
				return err
			}
			leveldbBatch[data.Table].Put(key, value)
		}
	}

	for t, b := range leveldbBatch {
		err := l.db[t].Write(b, nil)
		if err != nil {
			*batch = batchTemp
			return err
		}
	}

	return nil
}

func generateKey(key string) []byte {
	return []byte(key)
}

func generateValue(entity interface{}) ([]byte, error) {
	value, err := json.Marshal(entity)
	if err != nil {
		return nil, err
	}
	return value, nil
}

func parseKey(key []byte) string {
	return string(key)
}

func parseValue(table string, data []byte) interface{} {
	var ans interface{}
	switch table {
	case common.LABEL_CONFIG:
		ans = new(model.Config)
		break
	case common.LABEL_MACHINE:
		ans = new(model.Machine)
		break
	case common.LABEL_MACHINEGROUP:
		ans = new(model.MachineGroup)
		break
	}
	json.Unmarshal(data, ans)
	fmt.Println(ans)
	return ans
}

func (l *LeveldbStore) CheckAll() {
	for t, db := range l.db {
		fmt.Println("Table:", t)
		iter := db.NewIterator(nil, nil)
		for iter.Next() {
			fmt.Println(string(iter.Key()), ":", string(iter.Value()))
		}
		iter.Release()
	}
}
