package utils

import (
	"encoding/json"
	"log"
	"os"
	"path/filepath"
)

func ReadJson(path string, obj any) error {
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
	decoderErr := decoder.Decode(&obj)
	if decoderErr != nil {
		return decoderErr
	}
	return nil

}
