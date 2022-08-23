package model

type Machine struct {
	MachineId string            `json:"instance_id"`
	Ip        string            `json:"ip"`
	State     string            `json:"state"`
	Heartbeat string            `json:"heartbeat"`
	Tag       map[string]string `json:"tags"`
	Status    map[string]string `json:"status"`
}

type AgentAlarm struct {
	MachineId    string `json:"instance_id"`
	Time         string `json:"time"`
	AlarmType    string `json:"alarm_type"`
	AlarmMessage string `json:"alarm_message"`
}

type AgentStatus struct {
	MachineId string            `json:"instance_id"`
	Status    map[string]string `json:"status"`
}
