package configmanager

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func CreateMachineGroup(name string, tag string, description string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, name)
	if err != nil {
		return true, err
	} else if ok {
		return true, nil
	} else {
		machineGroup := new(model.MachineGroup)
		machineGroup.Name = name
		machineGroup.Tag = tag
		machineGroup.Description = description
		machineGroup.AppliedConfigs = []string{}

		err = s.Add(common.TYPE_MACHINEGROUP, machineGroup.Name, machineGroup)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func UpdateMachineGroup(name string, tag string, description string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_MACHINEGROUP, name)
		if err != nil {
			return true, err
		}
		machineGroup := value.(*model.MachineGroup)

		machineGroup.Tag = tag
		machineGroup.Description = description
		machineGroup.Version++

		err = s.Update(common.TYPE_MACHINEGROUP, name, machineGroup)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func DeleteMachineGroup(name string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		err = s.Delete(common.TYPE_MACHINEGROUP, name)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func GetMachineGroup(name string) (*model.MachineGroup, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, name)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		machineGroup, err := s.Get(common.TYPE_MACHINEGROUP, name)
		if err != nil {
			return nil, err
		}
		return machineGroup.(*model.MachineGroup), nil
	}
}

func GetAllMachineGroup() ([]model.MachineGroup, error) {
	s := store.GetStore()
	machineGroupList, err := s.GetAll(common.TYPE_MACHINEGROUP)
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.MachineGroup, 0)
		for _, machineGroup := range machineGroupList {
			ans = append(ans, *machineGroup.(*model.MachineGroup))
		}
		return ans, nil
	}
}
