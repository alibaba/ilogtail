package utils

import (
	"log"
	"os"
)

func GetEnvName() (string, error) {
	name := os.Getenv("GO_ENV")
	if name == "" {
		name = "dev"
	}
	log.Printf("the environment is %s", name)
	return name, nil
}
