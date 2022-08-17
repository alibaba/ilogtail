package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IConfig interface {
	Get(entityKey string) *model.Config
	Add(entity *model.Config)
	Mod(entity *model.Config)
	Delete(entityKey string)
	GetAll() []*model.Config
}
