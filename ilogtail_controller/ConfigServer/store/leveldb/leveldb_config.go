package leveldb

import (
	"encoding/json"
	"strings"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/syndtr/goleveldb/leveldb"
)

type LeveldbConfig struct {
}

func (l *LeveldbConfig) Get(configName string) *model.Config {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	data, err := db.Get([]byte(l.generateKey(configName)), nil)
	if err != nil {
		panic(err)
	}
	return l.parseValue(data)
}

func (l *LeveldbConfig) Add(config *model.Config) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(config.Name)
	value := l.generateValue(config)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbConfig) Mod(config *model.Config) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(config.Name)
	value := l.generateValue(config)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbConfig) Delete(configName string) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(configName)
	err = db.Delete(key, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbConfig) GetAll() []*model.Config {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	ans := []*model.Config{}
	iter := db.NewIterator(nil, nil)
	for iter.Next() {
		key := l.parseKey(iter.Key())
		keyMap := strings.Split(string(key), ":")
		if strings.Compare(keyMap[0], "CONFIG") == 0 {
			value := l.parseValue(iter.Value())
			ans = append(ans, value)
		}
	}
	iter.Release()
	err = iter.Error()
	if err != nil {
		panic(err)
	}
	return ans
}

func (l *LeveldbConfig) generateKey(configName string) []byte {
	return []byte("CONFIG:" + configName)
}

func (l *LeveldbConfig) generateValue(config *model.Config) []byte {
	value, err := json.Marshal(config)
	if err != nil {
		panic(err)
	}
	return value
}

func (l *LeveldbConfig) parseKey(key []byte) string {
	keyMap := strings.Split(string(key), ":")
	return keyMap[1]
}

func (l *LeveldbConfig) parseValue(data []byte) *model.Config {
	config := new(model.Config)
	json.Unmarshal(data, config)
	return config
}
