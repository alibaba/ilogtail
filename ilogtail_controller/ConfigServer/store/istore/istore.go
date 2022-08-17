package istore

type IStore interface {
	GetMode() string // store mode
	Config() IConfig
	MachineGroup() IMachineGroup
	Machine() IMachine
}
