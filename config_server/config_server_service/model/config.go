package model

type Config struct {
	Name                string   `json:"name"`
	Content             string   `json:"content"`
	Version             int      `json:"version"`
	Description         string   `json:"description"`
	DelTag              bool     `json:"delete_tag"`
	AppliedMachineGroup []string `json:"applied_machines"`
}

func NewConfig(name string, content string, version int, description string) *Config {
	return &Config{name, content, version, description, false, []string{}}
}
