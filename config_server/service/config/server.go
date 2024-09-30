package config

import (
	"config-server/common"
	"config-server/utils"
	"fmt"
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
	envName, err := utils.GetEnvName()
	if err != nil {
		return err
	}
	serverConfigPath, err := filepath.Abs(fmt.Sprintf("cmd/config/%s/serverConfig.json", envName))
	if err != nil {
		return common.SystemError(err)
	}
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
