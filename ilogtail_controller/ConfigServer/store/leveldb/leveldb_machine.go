package leveldb

import (
	"encoding/json"
	"strings"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/syndtr/goleveldb/leveldb"
)

type LeveldbMachine struct {
}

func (l *LeveldbMachine) Get(machineId string) *model.Machine {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	data, err := db.Get([]byte(l.generateKey(machineId)), nil)
	if err != nil {
		panic(err)
	}
	return l.parseValue(data)
}

func (l *LeveldbMachine) Add(machine *model.Machine) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machine.Id)
	value := l.generateValue(machine)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachine) Mod(machine *model.Machine) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machine.Id)
	value := l.generateValue(machine)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachine) Delete(machineId string) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineId)
	err = db.Delete(key, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachine) GetAll() []*model.Machine {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	ans := []*model.Machine{}
	iter := db.NewIterator(nil, nil)
	for iter.Next() {
		key := l.parseKey(iter.Key())
		keyMap := strings.Split(string(key), ":")
		if strings.Compare(keyMap[0], "MACHINE") == 0 {
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

func (l *LeveldbMachine) generateKey(machineId string) []byte {
	return []byte("MACHINE:" + machineId)
}

func (l *LeveldbMachine) generateValue(machine *model.Machine) []byte {
	value, err := json.Marshal(machine)
	if err != nil {
		panic(err)
	}
	return value
}

func (l *LeveldbMachine) parseKey(key []byte) string {
	keyMap := strings.Split(string(key), ":")
	return keyMap[1]
}

func (l *LeveldbMachine) parseValue(data []byte) *model.Machine {
	machine := new(model.Machine)
	json.Unmarshal(data, machine)
	return machine
}
