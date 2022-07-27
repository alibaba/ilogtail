package manager

import (
	"errors"
)

// Definition of class config group.
type ConfigGroup struct {
	Name        string   `json:"name"`
	Description string   `json:"description"`
	Configs     []string `json:"config_names"`
}

// Create a config group.
func NewConfigGroup(name string) *ConfigGroup {
	return &ConfigGroup{name, "", make([]string, 0)}
}

// Get config number in a group.
func (cg *ConfigGroup) GetConfigNumber() int {
	return len(cg.Configs)
}

// Add a config to a group.
func (cg *ConfigGroup) AddConfig(newConfig *Config) error {
	id, flag := findPre(cg.Configs, newConfig.Name)
	if id >= MaxConfigNum {
		return errors.New("Exceed the maximum number limit, add failed.")
	}
	if flag == true {
		return errors.New("A configuration with the same name existed, add failed.")
	}
	cg.Configs = append(cg.Configs[:id], append([]string{newConfig.Name}, cg.Configs[id:]...)...)

	GetMyStore().sendMessage("M", "CG", cg.Name)
	return nil
}

// Delete a config from a group.
func (cg *ConfigGroup) DelConfig(configName string) error {
	id, flag := findPre(cg.Configs, configName)
	if id >= cg.GetConfigNumber() || flag == false {
		return errors.New("The specified target was not found, delete failed.")
	}
	cg.Configs = append(cg.Configs[0:id], cg.Configs[id+1:]...)

	GetMyStore().sendMessage("M", "CG", cg.Name)
	return nil
}

// Get all configs' information in a group.
func (cg *ConfigGroup) GetConfigList() []Config {
	var tmp []Config
	for _, v := range cg.Configs {
		tmp = append(tmp, *ConfigList[v])
	}
	return tmp
}

// Change a config's name in a group.
func (cg *ConfigGroup) ChangeConfigName(configName string, newName string) error {
	// Find if the target exists.
	id1, flag1 := findPre(cg.Configs, configName)
	if id1 >= cg.GetConfigNumber() || flag1 == false {
		return errors.New("The specified target was not found, change failed.")
	}
	tmp := cg.Configs[id1]
	cg.Configs = append(cg.Configs[0:id1], cg.Configs[id1+1:]...)

	// Find if the new name has been used.
	id2, flag2 := findPre(cg.Configs, newName)
	if flag2 == true {
		cg.Configs = append(cg.Configs[:id1], append([]string{tmp}, cg.Configs[id1:]...)...)
		return errors.New("The new name has been used, change failed")
	}
	ConfigList[tmp].Name = newName
	cg.Configs = append(cg.Configs[:id2], append([]string{tmp}, cg.Configs[id2:]...)...)

	GetMyStore().sendMessage("M", "CG", cg.Name)
	return nil
}

// Change a config's file path in a group.
func (cg *ConfigGroup) ChangeConfigPath(configName string, newPath string) error {
	id1, flag1 := findPre(cg.Configs, configName)
	if id1 == 0 || flag1 == false {
		return errors.New("The specified target was not found, change failed.")
	}
	ConfigList[cg.Configs[id1]].Path = newPath

	GetMyStore().sendMessage("M", "CG", cg.Name)
	return nil
}

// Change a config's description in a group.
func (cg *ConfigGroup) ChangeConfigDescription(configName string, newDescription string) error {
	id1, flag1 := findPre(cg.Configs, configName)
	if id1 == 0 || flag1 == false {
		return errors.New("The specified target was not found, change failed.")
	}
	ConfigList[cg.Configs[id1]].Description = newDescription

	GetMyStore().sendMessage("M", "CG", cg.Name)
	return nil
}

// Max machine number allowed in a machine group.
const MaxConfigGroupNum int = 10

var ConfigGroupList map[string]*ConfigGroup

func AddConfigGroup(newConfigGroup *ConfigGroup) error {
	_, ok := ConfigGroupList[newConfigGroup.Name]
	if ok {
		return errors.New("This configGroup already existed.")
	}
	if len(ConfigGroupList) >= MaxConfigGroupNum {
		return errors.New("The number of configGroups has reached the upper limit, adding failed.")
	}
	ConfigGroupList[newConfigGroup.Name] = newConfigGroup

	GetMyStore().sendMessage("U", "CG", newConfigGroup.Name)
	return nil
}

func DelConfigGroup(configGroupName string) error {
	_, ok := ConfigGroupList[configGroupName]
	if ok {
		delete(ConfigGroupList, configGroupName)
		GetMyStore().sendMessage("D", "CG", configGroupName)
		return nil
	}
	return errors.New("This configGroup do not exist.")
}

func GetConfigGroups() []ConfigGroup {
	var ans []ConfigGroup
	for _, cg := range ConfigGroupList {
		ans = append(ans, *cg)
	}
	return ans
}
