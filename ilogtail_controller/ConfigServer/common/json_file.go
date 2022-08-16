package common

import (
	"encoding/json"
	"os"
)

func ReadJson(filePath string, ptr interface{}) {
	file, err := os.Open(filePath)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	err = decoder.Decode(&ptr)
	if err != nil {
		panic(err)
	}
}

func WriteJson(filePath string, data interface{}) {
	dataString, err := json.Marshal(data)
	if err != nil {
		panic(err)
	}

	file, err := os.OpenFile(filePath, os.O_RDWR|os.O_TRUNC, os.ModeAppend)
	if err != nil {
		panic(err)
	}

	_, err = file.WriteString(string(dataString))
	if err != nil {
		panic(err)
	}
	defer file.Close()
}
