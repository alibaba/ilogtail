package manager

import "errors"

type Machine struct {
	Uid         string `json:"uid"`
	Description string `json:"description"`
	Ip          string `json:"ip"`
	Tag         string `json:"tag"`
	State       string `json:"state"`
	Heartbeat   string `json:"heartbeat"`
}

func NewMachine(uid string, description string, ip string, port string, state string, heartbeat string) *Machine {
	return &Machine{uid, description, ip, port, state, heartbeat}
}

const MaxMachineNum int = 100

var MachineList map[string]*Machine

func AddMachine(newMachine *Machine) error {
	_, ok := MachineList[newMachine.Uid]
	if ok {
		return errors.New("This machine already existed.")
	}
	if len(MachineList) >= MaxMachineNum {
		return errors.New("The number of machines has reached the upper limit, adding failed.")
	}
	MachineList[newMachine.Uid] = newMachine
	GetMyStore().sendMessage("U", "MA", newMachine.Uid)
	return nil
}

func DelMachine(machineUid string) error {
	_, ok := MachineList[machineUid]
	if ok {
		delete(MachineList, machineUid)
		GetMyStore().sendMessage("D", "MA", machineUid)
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
