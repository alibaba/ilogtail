package structure

import "errors"

type Machine struct {
	Id          string `json:"id"`
	Description string `json:"description"`
	Ip          string `json:"ip"`
	Tag         string `json:"tag"`
	State       string `json:"state"`
	Heartbeat   string `json:"heartbeat"`
}

func NewMachine(id string, description string, ip string, port string, state string, heartbeat string) *Machine {
	return &Machine{id, description, ip, port, state, heartbeat}
}

const MaxMachineNum int = 100

var MachineList map[string]*Machine

func AddMachine(newMachine *Machine) error {
	_, ok := MachineList[newMachine.Id]
	if ok {
		return errors.New("This machine already existed.")
	}
	if len(MachineList) >= MaxMachineNum {
		return errors.New("The number of machines has reached the upper limit, adding failed.")
	}
	MachineList[newMachine.Id] = newMachine
	//	GetMyStore().sendMessage("U", "MA", newMachine.Id)
	return nil
}

func DelMachine(machineId string) error {
	_, ok := MachineList[machineId]
	if ok {
		delete(MachineList, machineId)
		//		GetMyStore().sendMessage("D", "MA", machineId)
		return nil
	}
	return errors.New("This machine do not exist.")
}

func GetMachines() []Machine {
	var ans []Machine
	for _, m := range MachineList {
		ans = append(ans, *m)
	}
	return ans
}
