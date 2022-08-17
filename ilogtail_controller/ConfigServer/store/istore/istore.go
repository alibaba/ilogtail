package istore

/*
Interface of store
*/

type IStore interface {
	GetMode() string             // store mode
	Config() IConfig             // Read or Write Config
	MachineGroup() IMachineGroup // Read or Write MachineGroup
	Machine() IMachine           // Read or Write Machine
}
