package leveldb

import (
	"bytes"
	"encoding/json"
	"fmt"
	"strings"

	"github.com/alibaba/ilogtail/config_server/config_server_service/common"
	"github.com/alibaba/ilogtail/config_server/config_server_service/model"
	"github.com/alibaba/ilogtail/config_server/config_server_service/setting"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store/database"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/util"
)

var content = map[string]int{
	common.LABEL_CONFIG:       0,
	common.LABEL_MACHINE:      1,
	common.LABEL_MACHINEGROUP: 2,
}

var contentValue = [][]byte{
	[]byte(common.LABEL_CONFIG),
	[]byte(common.LABEL_MACHINE),
	[]byte(common.LABEL_MACHINEGROUP),
	[]byte("~"),
}

type LeveldbStore struct {
	db *leveldb.DB
}

func (l *LeveldbStore) Connect() error {
	var err error
	l.db, err = leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return err
	}

	for _, i := range contentValue {
		ok, err := l.db.Has(i, nil)
		if err != nil {
			return err
		}
		if !ok {
			err = l.db.Put(i, bytes.Join([][]byte{[]byte("content_"), i}, []byte("")), nil)
			if err != nil {
				return err
			}
		}
	}

	return nil
}

func (l *LeveldbStore) GetMode() string {
	return "leveldb"
}

func (l *LeveldbStore) Close() error {
	err := l.db.Close()
	return err
}

func (l *LeveldbStore) Get(table string, entityKey string) (interface{}, error) {
	value, err := l.db.Get([]byte(generateKey(table, entityKey)), nil)
	if err != nil {
		return nil, err
	}
	return parseValue(table, value), nil
}

func (l *LeveldbStore) Add(table string, entityKey string, entity interface{}) error {
	key := generateKey(table, entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}

	err = l.db.Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) Mod(table string, entityKey string, entity interface{}) error {
	key := generateKey(table, entityKey)
	value, err := generateValue(entity)
	if err != nil {
		return err
	}

	err = l.db.Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) Has(table string, entityKey string) (bool, error) {
	key := generateKey(table, entityKey)
	ok, err := l.db.Has(key, nil)
	if err != nil {
		return false, err
	}
	return ok, nil
}

func (l *LeveldbStore) Delete(table string, entityKey string) error {
	key := generateKey(table, entityKey)
	err := l.db.Delete(key, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbStore) GetAll(table string) ([]interface{}, error) {
	ans := make([]interface{}, 0)

	iter := l.db.NewIterator(&util.Range{Start: contentValue[content[table]], Limit: contentValue[content[table]+1]}, nil)
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

func (l *LeveldbStore) CreateBatch() database.Batch {
	batch := new(LeveldbBatch)
	batch.batch = new(leveldb.Batch)
	return batch
}

func generateKey(table string, key string) []byte {
	return []byte(table + ":" + key)
}

func generateValue(entity interface{}) ([]byte, error) {
	value, err := json.Marshal(entity)
	if err != nil {
		return nil, err
	}
	return value, nil
}

func parseTable(key []byte) string {
	keyMap := strings.Split(string(key), ":")
	return keyMap[0]
}

func parseKey(key []byte) string {
	keyMap := strings.Split(string(key), ":")
	return keyMap[1]
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
	iter := l.db.NewIterator(nil, nil)
	for iter.Next() {
		fmt.Println(string(iter.Key()), ":", string(iter.Value()))
	}

	iter.Release()
}
