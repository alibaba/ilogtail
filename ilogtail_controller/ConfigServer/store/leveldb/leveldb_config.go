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

func (l *LeveldbConfig) Get(configName string) (*model.Config, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return nil, err
	}
	defer db.Close()

	data, err := db.Get([]byte(l.generateKey(configName)), nil)
	if err != nil {
		return nil, err
	}
	return l.parseValue(data), nil
}

func (l *LeveldbConfig) Add(config *model.Config) error {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return err
	}
	defer db.Close()

	key := l.generateKey(config.Name)
	value, err := l.generateValue(config)
	if err != nil {
		return err
	}

	err = db.Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbConfig) Mod(config *model.Config) error {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return err
	}
	defer db.Close()

	key := l.generateKey(config.Name)
	value, err := l.generateValue(config)
	if err != nil {
		return err
	}

	err = db.Put(key, value, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbConfig) Has(configName string) (bool, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return false, err
	}
	defer db.Close()

	key := l.generateKey(configName)
	ok, err := db.Has(key, nil)
	if err != nil {
		return false, err
	}
	return ok, nil
}

func (l *LeveldbConfig) Delete(configName string) error {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return err
	}
	defer db.Close()

	key := l.generateKey(configName)
	err = db.Delete(key, nil)
	if err != nil {
		return err
	}
	return nil
}

func (l *LeveldbConfig) GetAll() ([]*model.Config, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return nil, err
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
		return nil, err
	}
	return ans, nil
}

func (l *LeveldbConfig) generateKey(configName string) []byte {
	return []byte("CONFIG:" + configName)
}

func (l *LeveldbConfig) generateValue(config *model.Config) ([]byte, error) {
	value, err := json.Marshal(config)
	if err != nil {
		return nil, err
	}
	return value, nil
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
