package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IMachineGroup interface {
	Get(entityKey string) *model.MachineGroup
	Add(entity *model.MachineGroup)
	Mod(entity *model.MachineGroup)
	Has(entityKey string) bool
	Delete(entityKey string)
	GetAll() []*model.MachineGroup
}
