package leveldb

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store/istore"

type LeveldbStore struct {
}

func (l *LeveldbStore) GetMode() string {
	return "leveldb"
}

func (l *LeveldbStore) Config() istore.IConfig {
	return new(LeveldbConfig)
}

func (l *LeveldbStore) MachineGroup() istore.IMachineGroup {
	return new(LeveldbMachineGroup)
}

func (l *LeveldbStore) Machine() istore.IMachine {
	return new(LeveldbMachine)
}
