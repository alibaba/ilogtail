package store

import (
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager/structure"
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/tool"

	"github.com/syndtr/goleveldb/leveldb"
)

type leveldbStore struct {
	store
	dataPath string
}

const dbFile string = "./dataDB"

func newLeveldbStore() iStore {
	return &leveldbStore{
		store: store{
			name:         "Leveldb",
			messageQueue: tool.NewQueue(),
		},
		dataPath: dbFile,
	}
}

func (j *leveldbStore) ReadData() {
	db, err := leveldb.OpenFile(dbFile, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	iter := db.NewIterator(nil, nil)
	for iter.Next() {
		key := iter.Key()
		value := iter.Value()

		keyMap := strings.Split(string(key), ":")

		if strings.Compare(keyMap[0], "CONFIG") == 0 {
			config := new(structure.Config)
			json.Unmarshal(value, config)
			structure.AddConfig(config)
		}
		if strings.Compare(keyMap[0], "MACHINE") == 0 {
			machine := new(structure.Machine)
			json.Unmarshal(value, machine)
			structure.AddMachine(machine)
		}
		if strings.Compare(keyMap[0], "CONFIGGROUP") == 0 {
			configGroup := new(structure.ConfigGroup)
			json.Unmarshal(value, configGroup)
			sort.Strings(configGroup.Configs)
			structure.AddConfigGroup(configGroup)
		}
		if strings.Compare(keyMap[0], "MACHINEGROUP") == 0 {
			machineGroup := new(structure.MachineGroup)
			json.Unmarshal(value, machineGroup)
			structure.AddMachineGroup(machineGroup)
		}
	}
	iter.Release()
	err = iter.Error()
	if err != nil {
		panic(err)
	}
}

func (j *leveldbStore) UpdateData() {
	db, err := leveldb.OpenFile(dbFile, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	for !j.messageQueue.Empty() {
		msg := j.messageQueue.Pop()
		//		fmt.Println(msg)
		id := msg.(StoreMessage).id
		var value []byte
		switch msg.(StoreMessage).opt {
		case "U":
			switch msg.(StoreMessage).trg {
			case "CO":
				value, err = json.Marshal(*structure.ConfigList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("CONFIG:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "MA":
				value, err = json.Marshal(*structure.MachineList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("MACHINE:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "CG":
				value, err = json.Marshal(*structure.ConfigGroupList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("CONFIGGROUP:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "MG":
				value, err = json.Marshal(*structure.MachineGroupList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("MACHINEGROUP:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			default:
				panic("Order error")
			}
			break
		case "D":
			switch msg.(StoreMessage).trg {
			case "CO":
				err = db.Delete([]byte("CONFIG:"+id), nil)
				if err != nil {
					panic(err)
				}
				break
			case "MA":
				err = db.Delete([]byte("MACHINE:"+id), nil)
				if err != nil {
					panic(err)
				}
				break
			case "CG":
				err = db.Delete([]byte("CONFIGGROUP:"+id), nil)
				if err != nil {
					panic(err)
				}
				break
			case "MG":
				err = db.Delete([]byte("MACHINEGROUP:"+id), nil)
				if err != nil {
					panic(err)
				}
				break
			default:
				panic("Order error")
			}
			break
		case "M":
			switch msg.(StoreMessage).trg {
			case "CO":
				value, err = json.Marshal(*structure.ConfigList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("CONFIG:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "MA":
				value, err = json.Marshal(*structure.MachineList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("MACHINE:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "CG":
				value, err = json.Marshal(*structure.ConfigGroupList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("CONFIGGROUP:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			case "MG":
				value, err = json.Marshal(*structure.MachineGroupList[id])
				if err != nil {
					panic(err)
				}
				err = db.Put([]byte("MACHINEGROUP:"+id), value, nil)
				if err != nil {
					panic(err)
				}
				break
			default:
				panic("Order error")
			}
			break
		default:
			panic("Order error")
		}
	}
}

func (j *leveldbStore) ShowData() {
	db, err := leveldb.OpenFile(dbFile, nil)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	iter := db.NewIterator(nil, nil)
	for iter.Next() {
		key := iter.Key()
		value := iter.Value()

		fmt.Println(string(key), string(value))
	}
	iter.Release()
	err = iter.Error()
	if err != nil {
		panic(err)
	}
}
