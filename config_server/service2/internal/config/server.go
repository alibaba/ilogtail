package config

import (
	"config-server2/internal/common"
	"config-server2/internal/utils"
	"log"
	"path/filepath"
)

type ServerConfig struct {
	Address       string          `json:"address"`
	Capabilities  map[string]bool `json:"capabilities"`
	ResponseFlags map[string]bool `json:"responseFlags"`
	TimeLimit     int64           `json:"timeLimit"`
}

var ServerConfigInstance = new(ServerConfig)

func GetServerConfiguration() error {
	var err error
	serverConfigPath, err := filepath.Abs("cmd/config/serverConfig.json")
	log.Println(serverConfigPath)
	err = utils.ReadJson(serverConfigPath, ServerConfigInstance)
	if err != nil {
		return common.SystemError(err)
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
