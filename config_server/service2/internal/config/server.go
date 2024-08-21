package config

import (
	"config-server2/internal/utils"
	"log"
	"path/filepath"
	"runtime"
)

type ServerConfig struct {
	Capabilities  map[string]bool `json:"capabilities"`
	ResponseFlags map[string]bool `json:"responseFlags"`
	TimeLimit     int64           `json:"timeLimit"`
}

var ServerConfigInstance = new(ServerConfig)

func GetServerConfiguration() error {
	var err error
	_, currentFilePath, _, _ := runtime.Caller(0)
	err = utils.ReadJson(filepath.Join(filepath.Dir(currentFilePath), "./serverConfig.json"),
		ServerConfigInstance)
	if err != nil {
		return err
	}
	log.Print("server config init...")
	return nil
}

func init() {
	err := GetServerConfiguration()
	if err != nil {
		return
	}
}
