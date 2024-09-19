package utils

import (
	"encoding/json"
	"log"
	"os"
	"path/filepath"
)

func ReadJson[T any](path string, obj *T) error {
	file, openErr := os.Open(filepath.Clean(path))
	if openErr != nil {
		return openErr
	}
	defer func() {
		if closeErr := file.Close(); closeErr != nil {
			log.Println("Error closing file: " + closeErr.Error())
		}
	}()

	decoder := json.NewDecoder(file)
	err := decoder.Decode(obj)
	if err != nil {
		return err
	}
	return nil

}
