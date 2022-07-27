package tool

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
