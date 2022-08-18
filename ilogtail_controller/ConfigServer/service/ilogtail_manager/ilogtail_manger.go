package ilogtailmanager

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store"
)

func GetAllMachine() ([]model.Machine, error) {
	myMachines := store.GetStore().Machine()
	machineList, err := myMachines.GetAll()
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.Machine, 0)
		for _, machine := range machineList {
			ans = append(ans, *machine)
		}
		return ans, nil
	}
}
