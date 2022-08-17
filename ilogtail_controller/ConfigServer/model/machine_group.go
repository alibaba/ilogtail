package model

type MachineGroup struct {
	Name           string   `json:"name"`
	Description    string   `json:"description"`
	Tag            string   `json:"tag"`
	AppliedConfigs []string `json:"applied_configs"`
}

func NewMachineGroup(name string) *MachineGroup {
	return &MachineGroup{name, "", "default", []string{}}
}
