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

	value1, _ := myConfigs.GetAll()
	t.Log("ALL CONFIGS:", value1)

	config := model.NewConfig("111", "1111", 1, "111")
	myConfigs.Add(config)

	value2, _ := myConfigs.Get("111")
	t.Log("CONFIG 111:", value2)

	config.Description = "test"
	myConfigs.Mod(config)
	value2, _ = myConfigs.Get("111")
	t.Log("CONFIG 111:", value2)

	value1, _ = myConfigs.GetAll()
	t.Log("ALL CONFIGS:", value1)

	myConfigs.Delete("111")

	value1, _ = myConfigs.GetAll()
	t.Log("ALL CONFIGS:", value1)

}
