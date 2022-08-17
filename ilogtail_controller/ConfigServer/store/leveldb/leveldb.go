package leveldb

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store/istore"

type LeveldbStore struct {
	DataPath string
}

func (l *LeveldbStore) GetMode() string {
	return "leveldb"
}

func (l *LeveldbStore) GetConfigs() istore.IConfig {
	return new(LeveldbConfig)
}
