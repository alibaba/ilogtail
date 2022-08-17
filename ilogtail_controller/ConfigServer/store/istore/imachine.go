package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IMachine interface {
	Get(entityKey string) (*model.Machine, error)
	Add(entity *model.Machine) error
	Mod(entity *model.Machine) error
	Has(entityKey string) (bool, error)
	Delete(entityKey string) error
	GetAll() ([]*model.Machine, error)
}
