package manager

import (
	"fmt"
	"sync"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/tool"
)

const BaseSettingFile string = "./base_setting.json"

type baseSetting struct {
	RunMode   int `json:"run_mode,string"`   // 0 cluster mode, 1 stand-alone mode
	StoreMode int `json:"store_mode,string"` // 0 file mode, 1 leveldb mode
}

var myBaseSetting *baseSetting

var setOnce sync.Once

func GetMyBaseSetting() *baseSetting {
	setOnce.Do(func() {
		myBaseSetting = new(baseSetting)
		tool.ReadJson(BaseSettingFile, myBaseSetting)
	})
	return myBaseSetting
}

func UpdateMyBasesetting() {
	tool.ReadJson(BaseSettingFile, myBaseSetting)
}

func InitManager() {
	MachineList = make(map[string]*Machine)
	ConfigList = make(map[string]*Config)
	MachineGroupList = make(map[string]*MachineGroup)
	ConfigGroupList = make(map[string]*ConfigGroup)

	store := GetMyStore()
	fmt.Println("Store Mode:", store.getName())
	store.readData()
}

func LeveldbTest() {
	fmt.Println("Leveldbtest")

	config1 := NewConfig("config1", "111", "")
	AddConfig(config1)
	config2 := NewConfig("config2", "111", "")
	AddConfig(config2)
	config3 := NewConfig("config3", "111", "")
	AddConfig(config3)
	config4 := NewConfig("config4", "111", "")
	AddConfig(config4)
	config5 := NewConfig("config5", "111", "")
	AddConfig(config5)
	config6 := NewConfig("config6", "111", "")
	AddConfig(config6)
	config7 := NewConfig("config7", "111", "")
	AddConfig(config7)
	config8 := NewConfig("config1", "111", "")
	AddConfig(config8)

	configGroup1 := NewConfigGroup("default")
	AddConfigGroup(configGroup1)

	ConfigGroupList["default"].Description = "test"
	err := ConfigGroupList["default"].DelConfig(config1.Name)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config1)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config7)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config3)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config4)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config8)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config2)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config6)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].AddConfig(config5)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].DelConfig(config5.Name)
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].DelConfig("config8")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)

	err = ConfigGroupList["default"].ChangeConfigName(config1.Name, "config8")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].ChangeConfigName(config3.Name, "config6")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].ChangeConfigPath(config1.Name, "config8")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].ChangeConfigDescription(config1.Name, "config8")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].ChangeConfigName("config9", "config8")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)
	err = ConfigGroupList["default"].ChangeConfigName(config1.Name, "config1")
	fmt.Println(ConfigGroupList["default"].GetConfigNumber(), ConfigGroupList["default"].GetConfigList(), err)

	GetMyStore().updateData()

	GetMyStore().showData()

	machineGroup := NewMachineGroup("default")
	AddMachineGroup(machineGroup)

	MachineGroupList["default"].AddConfig(ConfigList["config3"])
	MachineGroupList["default"].AddConfig(ConfigList["config4"])

	GetMyStore().updateData()

	GetMyStore().showData()

	fmt.Println("===================================")
	fmt.Println()
}

func Jsontest() {
	fmt.Println("Jsontest")

	fmt.Println(MachineGroupList["default"].GetConfigList())

	if MachineGroupList["default"].AddConfig(ConfigList["config4"]) == nil {
		fmt.Println(MachineGroupList["default"].GetConfigList())
	}

	if MachineGroupList["default"].DelConfig("config3") == nil {
		fmt.Println(MachineGroupList["default"].GetConfigList())
	}

	GetMyStore().updateData()

	fmt.Println("===================================")
	fmt.Println()
}

func ConfigGroupTest() {
	fmt.Println("ConfigGrouptest")

	config1 := NewConfig("config1", "111", "")
	AddConfig(config1)
	config2 := NewConfig("config2", "111", "")
	AddConfig(config2)
	config3 := NewConfig("config3", "111", "")
	AddConfig(config3)
	config4 := NewConfig("config4", "111", "")
	AddConfig(config4)
	config5 := NewConfig("config5", "111", "")
	AddConfig(config5)
	config6 := NewConfig("config6", "111", "")
	AddConfig(config6)
	config7 := NewConfig("config7", "111", "")
	AddConfig(config7)
	config8 := NewConfig("config1", "111", "")
	AddConfig(config8)

	configGroup1 := NewConfigGroup("default")

	err := configGroup1.DelConfig(config1.Name)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config1)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config7)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config3)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config4)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config8)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config2)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config6)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.AddConfig(config5)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.DelConfig(config5.Name)
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.DelConfig("config8")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)

	err = configGroup1.ChangeConfigName(config1.Name, "config8")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.ChangeConfigName(config3.Name, "config6")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.ChangeConfigPath(config1.Name, "config8")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.ChangeConfigDescription(config1.Name, "config8")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.ChangeConfigName("config9", "config8")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)
	err = configGroup1.ChangeConfigName(config1.Name, "config1")
	fmt.Println(configGroup1.GetConfigNumber(), configGroup1.GetConfigList(), err)

	fmt.Println("===================================")
	fmt.Println()
}
