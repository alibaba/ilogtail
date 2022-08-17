package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IMachineGroup interface {
	Get(entityKey string) (*model.MachineGroup, error)
	Add(entity *model.MachineGroup) error
	Mod(entity *model.MachineGroup) error
	Has(entityKey string) (bool, error)
	Delete(entityKey string) error
	GetAll() ([]*model.MachineGroup, error)
}
