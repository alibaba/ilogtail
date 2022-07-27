package manager

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
)

type StoreData struct {
	Configs       []Config       `json:"configs"`
	Machines      []Machine      `json:"machines"`
	ConfigGroups  []ConfigGroup  `json:"json_groups"`
	MachineGroups []MachineGroup `json:"machine_groups"`
}

const dataStoreFile string = "./data.json"

type jsonStore struct {
	store
	Data     StoreData
	filePath string
}

func newJsonStore() iStore {
	return &jsonStore{
		store: store{
			name: "Json",
		},
		filePath: dataStoreFile,
	}
}

func (j *jsonStore) getData() *StoreData {
	return &j.Data
}

func (j *jsonStore) readData() {
	file, err := os.Open(j.filePath)
	if err != nil {
		info := "{\"machines\":[], \"configs\":[], \"machine_groups\":[], \"config_groups\":[]}"
		filePtr, createErr := os.Create(j.filePath)
		if createErr != nil {
			panic(createErr)
		}
		filePtr.WriteString(info)
		defer filePtr.Close()
		return
	}
	defer file.Close()

	byteValue, err := ioutil.ReadAll(file)
	if err != nil {
		panic(err)
	}

	json.Unmarshal([]byte(byteValue), &j.Data)
	for i := range j.Data.Configs {
		AddConfig(&j.Data.Configs[i])
	}
	for i := range j.Data.Machines {
		AddMachine(&j.Data.Machines[i])
	}
	for i := range j.Data.ConfigGroups {
		AddConfigGroup(&j.Data.ConfigGroups[i])
	}
	for i := range j.Data.MachineGroups {
		AddMachineGroup(&j.Data.MachineGroups[i])
	}
}

func (j *jsonStore) updateData() {
	j.Data.Configs = GetConfigs()
	j.Data.ConfigGroups = GetConfigGroups()
	j.Data.Machines = GetMachines()
	j.Data.MachineGroups = GetMachineGroups()

	j.messageQueue.Clear()

	dataString, _ := json.Marshal(j.Data)
	file, err := os.OpenFile(j.filePath, os.O_RDWR|os.O_TRUNC, os.ModeAppend)
	if err != nil {
		panic(err)
	}
	_, err = file.WriteString(string(dataString))
	if err != nil {
		panic(err)
	}
	defer file.Close()
}

func (j *jsonStore) showData() {
	file, err := os.OpenFile(j.filePath, os.O_RDWR, os.ModeAppend)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	byteValue, err := ioutil.ReadAll(file)
	if err != nil {
		panic(err)
	}

	fmt.Println(string(byteValue))
}
