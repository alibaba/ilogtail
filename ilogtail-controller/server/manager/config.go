package manager

import "errors"

// Definition of class config.
type Config struct {
	Name        string `json:"name"`
	Path        string `json:"path"`
	Description string `json:"description"`
}

func NewConfig(name string, path string, description string) *Config {
	return &Config{name, path, description}
}

// Max config number allowed in a config group.
const MaxConfigNum int = 1000

var ConfigList map[string]*Config

func AddConfig(newConfig *Config) error {
	_, ok := ConfigList[newConfig.Name]
	if ok {
		return errors.New("This config already existed.")
	}
	if len(ConfigList) >= MaxConfigNum {
		return errors.New("The number of configs has reached the upper limit, adding failed.")
	}
	ConfigList[newConfig.Name] = newConfig
	GetMyStore().sendMessage("U", "CO", newConfig.Name)
	return nil
}

func DelConfig(configName string) error {
	_, ok := ConfigList[configName]
	if ok {
		delete(ConfigList, configName)
		GetMyStore().sendMessage("D", "CO", configName)
		return nil
	}
	return errors.New("This config do not exist.")
}

func GetConfigs() []Config {
	var ans []Config
	for _, c := range ConfigList {
		ans = append(ans, *c)
	}
	return ans
}
