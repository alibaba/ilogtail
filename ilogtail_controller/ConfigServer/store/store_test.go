package store

import (
	"testing"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
)

func TestLoadStore(t *testing.T) {
	setting.SetSettingPath("./../setting/setting.json")

	t.Log(GetStore().GetMode())
}

func TestConfigStore(t *testing.T) {
	setting.SetSettingPath("./../setting/setting.json")

	myConfigs := GetStore().Config()

	t.Log("ALL CONFIGS:", myConfigs.GetAll())

	config := model.NewConfig("111", "1111", 1, "111")
	myConfigs.Add(config)

	t.Log("CONFIG 111:", myConfigs.Get("111"))

	config.Description = "test"
	myConfigs.Mod(config)
	t.Log("CONFIG 111:", myConfigs.Get("111"))

	t.Log("ALL CONFIGS:", myConfigs.GetAll())

	myConfigs.Delete("111")

	t.Log("ALL CONFIGS:", myConfigs.GetAll())

}
