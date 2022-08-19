package configmanager

import (
	"github.com/alibaba/ilogtail/config_server/config_server_service/common"
	"github.com/alibaba/ilogtail/config_server/config_server_service/model"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store"
)

func CreateConfig(name string, info string, description string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.LABEL_CONFIG, name)
	if err != nil { // has err
		return true, err
	} else if ok {
		value, err := s.Get(common.LABEL_CONFIG, name)
		if err != nil { // has err
			return true, err
		}
		config := value.(*model.Config)
		if config.DelTag == false { // exsit
			return true, nil
		} else { // exist but has delete tag
			config.Content = info
			config.Version++
			config.Description = description
			config.DelTag = false
			config.AppliedMachineGroup = []string{}
			err = s.Mod(common.LABEL_CONFIG, config.Name, config)
			if err != nil { // has err
				return false, err
			}
			return false, nil
		}
	} else { // doesn't exist
		config := new(model.Config)
		config.Name = name
		config.Content = info
		config.Version = 1
		config.Description = description
		config.DelTag = false
		config.AppliedMachineGroup = []string{}

		err = s.Add(common.LABEL_CONFIG, config.Name, config)
		if err != nil { // has err
			return false, err
		}
		return false, nil
	}
}

func UpdateConfig(name string, info string, description string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.LABEL_CONFIG, name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.LABEL_CONFIG, name)
		if err != nil {
			return true, err
		}
		config := value.(*model.Config)

		if config.DelTag == true {
			return false, nil
		} else {
			config.Content = info
			config.Version++
			config.Description = description
			err = s.Mod(common.LABEL_CONFIG, name, config)
			if err != nil {
				return true, err
			}
			return true, nil
		}
	}
}

func DeleteConfig(name string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.LABEL_CONFIG, name)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.LABEL_CONFIG, name)
		if err != nil {
			return true, err
		}
		config := value.(*model.Config)

		if config.DelTag == true {
			return false, nil
		} else {
			config.DelTag = true
			config.Version++
			err = s.Mod(common.LABEL_CONFIG, name, config)
			if err != nil {
				return true, err
			}
			return true, nil
		}
	}
}

func GetConfig(name string) (*model.Config, error) {
	s := store.GetStore()
	ok, err := s.Has(common.LABEL_CONFIG, name)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		config, err := s.Get(common.LABEL_CONFIG, name)
		if config.(*model.Config).DelTag == true {
			return nil, nil
		}
		if err != nil {
			return nil, err
		}
		return config.(*model.Config), nil
	}
}

func ListAllConfigs() ([]model.Config, error) {
	s := store.GetStore()
	configList, err := s.GetAll(common.LABEL_CONFIG)
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.Config, 0)
		for _, config := range configList {
			if config.(*model.Config).DelTag == false {
				ans = append(ans, *config.(*model.Config))
			}
		}
		return ans, nil
	}
}
