package common

import (
	"encoding/json"
	"os"
)

func ReadJson(filePath string, ptr interface{}) error {
	file, err := os.Open(filePath)
	if err != nil {
		return err
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	err = decoder.Decode(&ptr)
	if err != nil {
		return err
	}
	return nil
}

func WriteJson(filePath string, data interface{}) error {
	dataString, err := json.Marshal(data)
	if err != nil {
		return err
	}

	file, err := os.OpenFile(filePath, os.O_RDWR|os.O_TRUNC, os.ModeAppend)
	if err != nil {
		return err
	}

	_, err = file.WriteString(string(dataString))
	if err != nil {
		return err
	}
	defer file.Close()
	return nil
}
