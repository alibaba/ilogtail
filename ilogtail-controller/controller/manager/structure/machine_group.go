package structure

import (
	"errors"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/tool"
)

type MachineGroup struct {
	Name        string   `json:"name"`
	Description string   `json:"description"`
	State       int      `json:"state"`
	Machines    []string `json:"machine_ids"`
	Configs     []string `json:"config_names"`
}

// Create a machine group.
func NewMachineGroup(name string) *MachineGroup {
	return &MachineGroup{name, "", 0, make([]string, 0), make([]string, 0)}
}

// Get machine number in a group.
func (mg *MachineGroup) GetMachineNumber() int {
	return len(mg.Machines)
}

func (mg *MachineGroup) GetConfigNumber() int {
	return len(mg.Configs)
}

// Add a machine to a group.
func (mg *MachineGroup) AddMachine(newMachine *Machine) error {
	id, flag := tool.FindPre(mg.Machines, newMachine.Id)
	if id > MaxMachineNum {
		return errors.New("Exceed the maximum number limit, add failed.")
	}
	if flag == true {
		return errors.New("A configuration with the same name existed, add failed.")
	}
	mg.Machines = append(mg.Machines[:id], append([]string{newMachine.Id}, mg.Machines[id:]...)...)
	return nil
}

// Add a config to a group.
func (mg *MachineGroup) AddConfig(newConfig *Config) error {
	id, flag := tool.FindPre(mg.Configs, newConfig.Name)
	if id > MaxConfigNum {
		return errors.New("Exceed the maximum number limit, add failed.")
	}
	if flag == true {
		return errors.New("A configuration with the same name existed, add failed.")
	}
	mg.Configs = append(mg.Configs[:id], append([]string{newConfig.Name}, mg.Configs[id:]...)...)
	return nil
}

// Delete a machine from a group.
func (mg *MachineGroup) DelMachine(machineId string) error {
	id, flag := tool.FindPre(mg.Machines, machineId)
	if id >= mg.GetMachineNumber() || flag == false {
		return errors.New("The specified target was not found, delete failed.")
	}
	mg.Machines = append(mg.Machines[0:id], mg.Machines[id+1:]...)
	return nil
}

// Delete a config from a group.
func (mg *MachineGroup) DelConfig(configName string) error {
	id, flag := tool.FindPre(mg.Configs, configName)
	if id >= mg.GetConfigNumber() || flag == false {
		return errors.New("The specified target was not found, delete failed.")
	}
	mg.Configs = append(mg.Configs[0:id], mg.Configs[id+1:]...)
	return nil
}

// Get all machines' information in a group.
func (mg *MachineGroup) GetMachineList() []Machine {
	var tmp []Machine
	for _, v := range mg.Machines {
		tmp = append(tmp, *MachineList[v])
	}
	return tmp
}

// Get all configs' information in a group.
func (mg *MachineGroup) GetConfigList() []Config {
	var tmp []Config
	for _, v := range mg.Configs {
		tmp = append(tmp, *ConfigList[v])
	}
	return tmp
}

// Change a machine's name in a group.
func (mg *MachineGroup) ChangeMachineName(machineName string, newName string) error {
	// Find if the target exists.
	id1, flag1 := tool.FindPre(mg.Machines, machineName)
	if id1 >= mg.GetMachineNumber() || flag1 == false {
		return errors.New("The specified target was not found, change failed.")
	}
	tmp := mg.Machines[id1]
	mg.Machines = append(mg.Machines[0:id1], mg.Machines[id1+1:]...)

	// Find if the new name has been used.
	id2, flag2 := tool.FindPre(mg.Machines, newName)
	if flag2 == true {
		mg.Machines = append(mg.Machines[:id1], append([]string{tmp}, mg.Machines[id1:]...)...)
		return errors.New("The new name has been used, change failed")
	}
	MachineList[tmp].Id = newName
	mg.Machines = append(mg.Machines[:id2], append([]string{tmp}, mg.Machines[id2:]...)...)

	return nil
}

// Change a machine's description in a group.
func (mg *MachineGroup) ChangeMachineDescription(machineName string, newDescription string) error {
	id1, flag1 := tool.FindPre(mg.Machines, machineName)
	if id1 == 0 || flag1 == false {
		return errors.New("The specified target was not found, change failed.")
	}
	MachineList[mg.Machines[id1]].Description = newDescription
	return nil
}

// Max machine number allowed in a machine group.
const MaxMachineGroupNum int = 10

var MachineGroupList map[string]*MachineGroup

func AddMachineGroup(newMachineGroup *MachineGroup) error {
	_, ok := MachineGroupList[newMachineGroup.Name]
	if ok {
		return errors.New("This machineGroup already existed.")
	}
	if len(MachineGroupList) >= MaxMachineGroupNum {
		return errors.New("The number of machineGroups has reached the upper limit, adding failed.")
	}
	MachineGroupList[newMachineGroup.Name] = newMachineGroup

	//	GetMyStore().sendMessage("U", "MG", newMachineGroup.Name)
	return nil
}

func DelMachineGroup(machineGroupName string) error {
	_, ok := MachineGroupList[machineGroupName]
	if ok {
		delete(MachineGroupList, machineGroupName)
		//		GetMyStore().sendMessage("D", "MG", machineGroupName)
		return nil
	}
	return errors.New("This machineGroup do not exist.")
}

func GetMachineGroups() []MachineGroup {
	var ans []MachineGroup
	for _, mg := range MachineGroupList {
		ans = append(ans, *mg)
	}
	return ans
}
