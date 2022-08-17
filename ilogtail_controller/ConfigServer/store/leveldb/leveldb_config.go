package leveldb

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type LeveldbConfig struct {
}

func (l *LeveldbConfig) Get(entityName string) *model.Config {
	return nil
}

func (l *LeveldbConfig) Add(entity *model.Config) bool {
	return true
}

func (l *LeveldbConfig) Edit(entity *model.Config) bool {
	return true
}

func (l *LeveldbConfig) Delete(entityName string) bool {
	return true
}
