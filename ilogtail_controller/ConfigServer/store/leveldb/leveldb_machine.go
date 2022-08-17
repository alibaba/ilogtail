package leveldb

import (
	"encoding/json"
	"fmt"
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
	defer func() {
		fmt.Println(recover())
	}()

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
		}
	}
}

func (l *LeveldbMachine) Get(machineId string) (*model.Machine, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return nil, err
	}
	defer db.Close()

	data, err := db.Get([]byte(l.generateKey(machineId)), nil)
	if err != nil {
		return nil, err
	}
	return l.parseValue(data), nil
}

func (l *LeveldbMachine) Add(machine *model.Machine) error {
	key := l.generateKey(machine.Id)
	value, err := l.generateValue(machine)
	if err != nil {
		return err
	}
	l.batch.Put(key, value)
	return nil
}

func (l *LeveldbMachine) Mod(machine *model.Machine) error {
	key := l.generateKey(machine.Id)
	value, err := l.generateValue(machine)
	if err != nil {
		return err
	}
	l.batch.Put(key, value)
	return nil
}

func (l *LeveldbMachine) Has(machineId string) (bool, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return false, err
	}
	defer db.Close()

	key := l.generateKey(machineId)
	ok, err := db.Has(key, nil)
	if err != nil {
		return false, err
	}
	return ok, nil
}

func (l *LeveldbMachine) Delete(machineId string) error {
	key := l.generateKey(machineId)
	l.batch.Delete(key)
	return nil
}

func (l *LeveldbMachine) GetAll() ([]*model.Machine, error) {
	db, err := leveldb.OpenFile(setting.GetSetting().LeveldbStorePath, nil)
	if err != nil {
		return nil, err
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
		return nil, err
	}
	return ans, nil
}

func (l *LeveldbMachine) generateKey(machineId string) []byte {
	return []byte("MACHINE:" + machineId)
}

func (l *LeveldbMachine) generateValue(machine *model.Machine) ([]byte, error) {
	value, err := json.Marshal(machine)
	if err != nil {
		return nil, err
	}
	return value, nil
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
