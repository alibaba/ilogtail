package model

type Machine struct {
	Id        string            `json:"instance_id"`
	Ip        string            `json:"ip"`
	Tag       map[string]string `json:"tags"`
	State     string            `json:"state"`
	Heartbeat string            `json:"heartbeat"`
	Version   int               `json:"version"`
}
