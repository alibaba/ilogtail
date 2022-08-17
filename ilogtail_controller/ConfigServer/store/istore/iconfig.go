package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IConfig interface {
	Get(entityKey string) (*model.Config, error)
	Add(entity *model.Config) error
	Mod(entity *model.Config) error
	Has(entityKey string) (bool, error)
	Delete(entityKey string) error
	GetAll() ([]*model.Config, error)
}
