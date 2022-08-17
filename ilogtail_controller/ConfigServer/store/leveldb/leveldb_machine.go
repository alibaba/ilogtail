package leveldb

import (
	"encoding/json"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/syndtr/goleveldb/leveldb"
)

type LeveldbMachine struct {
	batch *leveldb.Batch
}

/*
	Batch refresh ilogtail data per 3 seconds
*/
func (l *LeveldbMachine) update() {
	ticker := time.NewTicker(3 * time.Second)

	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	l.batch = new(leveldb.Batch)
	for {
		select {
		case <-ticker.C:
			err = db.Write(l.batch, nil)
			if err != nil {
				panic(err)
			}
			break
		default:

		}
	}
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
	key := l.generateKey(machine.Id)
	value := l.generateValue(machine)
	l.batch.Put(key, value)
}

func (l *LeveldbMachine) Mod(machine *model.Machine) {
	key := l.generateKey(machine.Id)
	value := l.generateValue(machine)
	l.batch.Put(key, value)
}

func (l *LeveldbMachine) Has(machineId string) bool {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	key := l.generateKey(machineId)
	ok, err := db.Has(key, nil)
	if err != nil {
		panic(err)
	}
	return ok
}

func (l *LeveldbMachine) Delete(machineId string) {
	key := l.generateKey(machineId)
	l.batch.Delete(key)
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
