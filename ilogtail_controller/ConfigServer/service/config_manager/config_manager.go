package configmanager

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store"
)

func CreateMachineGroup(name string, tag string, description string) (bool, error) {
	myMachineGroups := store.GetStore().MachineGroup()
	ok, err := myMachineGroups.Has(name)
	if err != nil {
		return true, err
	} else if ok {
		return true, nil
	} else {
		machineGroup := model.NewMachineGroup(name, description, tag)
		myMachineGroups.Add(machineGroup)
		return false, nil
	}
}
