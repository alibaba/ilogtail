package istore

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type IConfig interface {
	Get(entityName string) *model.Config
	Add(entity *model.Config) bool
	Edit(entity *model.Config) bool
	Delete(entityName string) bool
}
