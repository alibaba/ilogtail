package leveldb

import (
	"encoding/json"
	"strings"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/syndtr/goleveldb/leveldb"
)

type LeveldbMachineGroup struct {
}

func (l *LeveldbMachineGroup) Get(machineGroupName string) *model.MachineGroup {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	data, err := db.Get([]byte(l.generateKey(machineGroupName)), nil)
	if err != nil {
		panic(err)
	}
	return l.parseValue(data)
}

func (l *LeveldbMachineGroup) Add(machineGroup *model.MachineGroup) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineGroup.Name)
	value := l.generateValue(machineGroup)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachineGroup) Mod(machineGroup *model.MachineGroup) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineGroup.Name)
	value := l.generateValue(machineGroup)
	err = db.Put(key, value, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachineGroup) Has(machineGroupName string) bool {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineGroupName)
	ok, err := db.Has(key, nil)
	if err != nil {
		panic(err)
	}
	return ok
}

func (l *LeveldbMachineGroup) Delete(machineGroupName string) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineGroupName)
	err = db.Delete(key, nil)
	if err != nil {
		panic(err)
	}
}

func (l *LeveldbMachineGroup) GetAll() []*model.MachineGroup {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	ans := []*model.MachineGroup{}
	iter := db.NewIterator(nil, nil)
	for iter.Next() {
		key := l.parseKey(iter.Key())
		keyMap := strings.Split(string(key), ":")
		if strings.Compare(keyMap[0], "MACHINEGROUP") == 0 {
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

func (l *LeveldbMachineGroup) generateKey(machineGroupName string) []byte {
	return []byte("MACHINEGROUP:" + machineGroupName)
}

func (l *LeveldbMachineGroup) generateValue(machineGroup *model.MachineGroup) []byte {
	value, err := json.Marshal(machineGroup)
	if err != nil {
		panic(err)
	}
	return value
}

func (l *LeveldbMachineGroup) parseKey(key []byte) string {
	keyMap := strings.Split(string(key), ":")
	return keyMap[1]
}

func (l *LeveldbMachineGroup) parseValue(data []byte) *model.MachineGroup {
	machineGroup := new(model.MachineGroup)
	json.Unmarshal(data, machineGroup)
	return machineGroup
}
