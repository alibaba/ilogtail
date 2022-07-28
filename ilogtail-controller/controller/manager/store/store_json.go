package store

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager/structure"
)

type StoreData struct {
	Configs       []structure.Config       `json:"configs"`
	Machines      []structure.Machine      `json:"machines"`
	ConfigGroups  []structure.ConfigGroup  `json:"json_groups"`
	MachineGroups []structure.MachineGroup `json:"machine_groups"`
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

func (j *jsonStore) GetData() *StoreData {
	return &j.Data
}

func (j *jsonStore) ReadData() {
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
		structure.AddConfig(&j.Data.Configs[i])
	}
	for i := range j.Data.Machines {
		structure.AddMachine(&j.Data.Machines[i])
	}
	for i := range j.Data.ConfigGroups {
		structure.AddConfigGroup(&j.Data.ConfigGroups[i])
	}
	for i := range j.Data.MachineGroups {
		structure.AddMachineGroup(&j.Data.MachineGroups[i])
	}
}

func (j *jsonStore) UpdateData() {
	j.Data.Configs = structure.GetConfigs()
	j.Data.ConfigGroups = structure.GetConfigGroups()
	j.Data.Machines = structure.GetMachines()
	j.Data.MachineGroups = structure.GetMachineGroups()

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

func (j *jsonStore) ShowData() {
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
