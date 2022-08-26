package leveldb

import (
	"encoding/json"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	database "github.com/alibaba/ilogtail/config_server/service/store/interface_database"
	"github.com/syndtr/goleveldb/leveldb"
)

var dbPath = []string{
	common.TYPE_AGENT_ALARM,
	common.TYPE_AGENT_STATUS,
	common.TYPE_COLLECTION_CONFIG,
	common.TYPE_MACHINE,
	common.TYPE_MACHINEGROUP,
}

type LeveldbStore struct {
	db map[string]*leveldb.DB
}

func (l *LeveldbStore) Connect() error {
	l.db = make(map[string]*leveldb.DB)

	var err error
	for _, c := range dbPath {
		l.db[c], err = leveldb.OpenFile(setting.GetSetting().DbPath+"/"+c, nil)
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

func (l *LeveldbStore) Count(table string) (int, error) {
	var ans int = 0

	iter := l.db[table].NewIterator(nil, nil)
	for iter.Next() {
		ans = ans + 1
	}

	iter.Release()
	err := iter.Error()
	if err != nil {
		return 0, err
	}
	return ans, nil
}

func (l *LeveldbStore) WriteBatch(batch *database.Batch) error {
	batchTemp := *batch
	var leveldbBatch map[string]*leveldb.Batch = make(map[string]*leveldb.Batch)

	for !batch.Empty() {
		data := batch.Pop()
		key := generateKey(data.Key)

		if _, ok := leveldbBatch[data.Table]; !ok {
			leveldbBatch[data.Table] = new(leveldb.Batch)
		}

		if data.Opt == database.OPT_DELETE {
			leveldbBatch[data.Table].Delete(key)
		} else if data.Opt == database.OPT_ADD || data.Opt == database.OPT_UPDATE {
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
	case common.TYPE_COLLECTION_CONFIG:
		ans = new(model.Config)
		break
	case common.TYPE_MACHINE:
		ans = new(model.Machine)
		break
	case common.TYPE_MACHINEGROUP:
		ans = new(model.MachineGroup)
		break
	case common.TYPE_AGENT_ALARM:
		ans = new(model.AgentAlarm)
		break
	case common.TYPE_AGENT_STATUS:
		ans = new(model.AgentStatus)
		break
	}
	json.Unmarshal(data, ans)
	return ans
}
