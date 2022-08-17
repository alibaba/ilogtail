package store

import "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"

type Data struct {
	Configs       []model.Config
	Machines      []model.Machine
	MachineGroups []model.MachineGroup
	Connections   []model.Connection
}
