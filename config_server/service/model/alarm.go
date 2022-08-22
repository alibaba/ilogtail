package model

type Alarm struct {
	MachineId    string `json:"machine_id"`
	Time         string `json:"time"`
	AlarmType    string `json:"alarm_type"`
	AlarmMessage string `json:"alarm_message"`
}
