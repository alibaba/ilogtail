package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IMachine interface {
	Get(entityKey string) *model.Machine
	Add(entity *model.Machine)
	Mod(entity *model.Machine)
	Delete(entityKey string)
	GetAll() []*model.Machine
}
