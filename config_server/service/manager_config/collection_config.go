package configmanager

import (
	"log"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func CreateConfig(configName string, info string, description string) (bool, error) {
	config, ok := store.GetMemory().ConfigList[configName]
	if ok {
		if config.DelTag == false { // exsit
			return true, nil
		} else { // exist but has delete tag
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description
			config.DelTag = false

			store.GetMemory().ConfigList[configName] = config
			return false, nil
		}
	} else { // doesn't exist
		config := new(model.Config)
		config.Name = configName
		config.Content = info
		config.Version = 0
		config.Description = description
		config.DelTag = false

		store.GetMemory().ConfigList[configName] = config
		return false, nil
	}
}

func UpdateConfig(configName string, info string, description string) (bool, error) {
	config, ok := store.GetMemory().ConfigList[configName]
	if !ok {
		return false, nil
	} else {
		if config.DelTag == true {
			return false, nil
		} else {
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description

			store.GetMemory().ConfigList[configName] = config
			return true, nil
		}
	}
}

func DeleteConfig(configName string) (bool, error) {
	config, ok := store.GetMemory().ConfigList[configName]
	if !ok {
		return false, nil
	} else {
		if config.DelTag == true {
			return false, nil
		} else {
			config.DelTag = true
			config.Version = config.Version + 1

			store.GetMemory().ConfigList[configName] = config
			return true, nil
		}
	}
}

func GetConfig(configName string) (*model.Config, error) {
	config, ok := store.GetMemory().ConfigList[configName]
	if !ok {
		return nil, nil
	} else {
		if config.DelTag == true {
			return nil, nil
		}
		return config, nil
	}
}

func ListAllConfigs() ([]model.Config, error) {
	configList := store.GetMemory().ConfigList
	ans := make([]model.Config, 0)
	for _, config := range configList {
		if config.DelTag == false {
			ans = append(ans, *config)
		}
	}
	return ans, nil
}

func updateConfig() {
	ticker := time.NewTicker(3 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		b := store.CreateBacth()

		// push
		for k, v := range store.GetMemory().ConfigList {
			ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, k)
			if err != nil {
				log.Println(err)
				continue
			} else if ok {
				value, err := s.Get(common.TYPE_COLLECTION_CONFIG, k)
				if err != nil {
					log.Println(err)
					continue
				}
				config := value.(*model.Config)

				if config.Version < v.Version {
					b.Update(common.TYPE_COLLECTION_CONFIG, k, v)
				}
			} else {
				b.Add(common.TYPE_COLLECTION_CONFIG, k, v)
			}
		}

		err := s.WriteBatch(b)
		if err != nil {
			log.Println(err)
			continue
		}

		// pull
		configList, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)
		if err != nil {
			log.Println(err)
			continue
		} else {
			for k := range store.GetMemory().ConfigList {
				delete(store.GetMemory().ConfigList, k)
			}

			for _, config := range configList {
				store.GetMemory().ConfigList[config.(*model.Config).Name] = config.(*model.Config)
			}
		}
	}
}

func init() {
	go updateConfig()
}
