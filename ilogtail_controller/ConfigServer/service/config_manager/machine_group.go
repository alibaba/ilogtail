package configmanager

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store"
)

func CreateMachineGroup(name string, tag string, description string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	myMachineGroups := store.GetStore().MachineGroup()
	ok, err := myMachineGroups.Has(name)
	if err != nil {
		return true, err
	} else if ok {
		return true, nil
	} else {
		machineGroup := model.NewMachineGroup(name, description, tag, []string{})

		err = myMachineGroups.Add(machineGroup)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func UpdateMachineGroup(name string, tag string, description string, configs []string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	myMachineGroups := store.GetStore().MachineGroup()
	ok, err := myMachineGroups.Has(name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		machineGroup := model.NewMachineGroup(name, description, tag, configs)

		err = myMachineGroups.Mod(machineGroup)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func DeleteMachineGroup(name string) (bool, error) {
	myMachineGroups := store.GetStore().MachineGroup()
	ok, err := myMachineGroups.Has(name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		err = myMachineGroups.Delete(name)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func GetMachineGroup(name string) (*model.MachineGroup, error) {
	myMachineGroups := store.GetStore().MachineGroup()
	ok, err := myMachineGroups.Has(name)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		machineGroup, err := myMachineGroups.Get(name)
		if err != nil {
			return nil, err
		}
		return machineGroup, nil
	}
}

func GetAllMachineGroup() ([]model.MachineGroup, error) {
	myMachineGroups := store.GetStore().MachineGroup()
	machineGroupList, err := myMachineGroups.GetAll()
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.MachineGroup, 0)
		for _, machineGroup := range machineGroupList {
			ans = append(ans, *machineGroup)
		}
		return ans, nil
	}
}
