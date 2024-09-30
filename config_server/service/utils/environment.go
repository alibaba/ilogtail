package utils

import (
	"log"
	"path/filepath"
)

func GetEnvConfig(path string) (map[string]any, error) {
	envMap := make(map[string]any)
	err := ReadJson(path, &envMap)
	if err != nil {
		return nil, err
	}
	return envMap, err
}

func GetEnvName() (string, error) {
	path, err := filepath.Abs("cmd/env.json")
	if err != nil {
		panic("can not find env file, it should be placed in cmd/env.json")
	}
	config, err := GetEnvConfig(path)
	if err != nil {
		return "", err
	}
	name := config["env"].(string)
	log.Printf("the environment is %s", name)
	return name, nil
}
