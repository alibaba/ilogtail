package manager

import (
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager/store"
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager/structure"
	"github.com/gin-gonic/gin"
)

func InitManager() {
	structure.MachineList = make(map[string]*structure.Machine)
	structure.ConfigList = make(map[string]*structure.Config)
	structure.MachineGroupList = make(map[string]*structure.MachineGroup)
	structure.ConfigGroupList = make(map[string]*structure.ConfigGroup)

	myStore := store.GetMyStore()
	fmt.Println("Store Mode:", myStore.GetName())
	myStore.ReadData()
}

func AddLocalConfig(configName string, configPath string, configDescription string) {
	newConfig := structure.NewConfig(configName, configPath, configDescription)
	structure.AddConfig(newConfig)
	store.GetMyStore().SendMessage("U", "CO", newConfig.Name)
	store.Update()
}

func DelLocalConfig(configName string) {
	structure.DelConfig(configName)
	store.GetMyStore().SendMessage("D", "CO", configName)
	store.Update()
}

func RegisterMachine(id string, heart string, ip string) bool {
	_, ok := structure.MachineList[id]
	if ok {
		structure.MachineList[id].Heartbeat = heart
		store.GetMyStore().SendMessage("M", "MA", id)
		return false

	} else {
		structure.AddMachine(structure.NewMachine(id, "", ip, "", "good", heart))
		store.GetMyStore().SendMessage("U", "MA", id)
		store.Update()
		return true
	}
}

// Incomplete
func AddConfigToMachine(machineId string, configName string) (string, int) {
	ip := structure.MachineList[machineId].Ip
	body := strings.NewReader(configName)
	response, err := http.Post(ip+"/", "application/json; charset=utf-8", body)
	if err != nil || response.StatusCode != http.StatusOK {
		return ip, http.StatusServiceUnavailable
	} else {
		return ip, http.StatusOK
	}
}

// Incomplete
func DelConfigFromMachine(machineId string, configName string) (string, int) {
	ip := structure.MachineList[machineId].Ip
	body := strings.NewReader(configName)
	response, err := http.Post(ip+"/", "application/json; charset=utf-8", body)
	if err != nil || response.StatusCode != http.StatusOK {
		return ip, http.StatusServiceUnavailable
	} else {
		return ip, http.StatusOK
	}
}

func GetMachineList(nowTime string) gin.H {
	data := gin.H{}
	for k := range structure.MachineList {
		NowTime, _ := time.Parse("2006-01-02 15:04:05", nowTime)
		preTime, _ := time.Parse("2006-01-02 15:04:05", structure.MachineList[k].Heartbeat)
		preHeart := NowTime.Sub(preTime)
		fmt.Println(preHeart)
		if preHeart.Seconds() < 15 {
			structure.MachineList[k].State = "good"
		} else if preHeart.Seconds() < 60 {
			structure.MachineList[k].State = "bad"
		} else {
			structure.MachineList[k].State = "lost"
		}
		data[k] = structure.MachineList[k]
	}
	return data
}
