package store

import (
	"testing"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
)

func TestLoadStore(t *testing.T) {
	setting.SetSettingPath("./../example/setting.json")

	t.Log(GetStore().GetMode())
}

func TestConfigStore(t *testing.T) {
	setting.SetSettingPath("./../example/setting.json")

	s := GetStore()

	value1, _ := s.GetAll(common.LABEL_CONFIG)
	t.Log("ALL CONFIGS:", value1)

	config := model.NewConfig("111", "1111", 1, "111")
	s.Add(common.LABEL_CONFIG, config.Name, config)

	value2, _ := s.Get(common.LABEL_CONFIG, "111")
	t.Log("CONFIG 111:", value2)

	config.Description = "test"
	s.Update(common.LABEL_CONFIG, config.Name, config)
	value2, _ = s.Get(common.LABEL_CONFIG, "111")
	t.Log("CONFIG 111:", value2)

	value1, _ = s.GetAll(common.LABEL_CONFIG)
	t.Log("ALL CONFIGS:", value1)

	//	s.Delete(common.LABEL_CONFIG, "111")

	//	value1, _ = s.GetAll(common.LABEL_CONFIG)
	//	t.Log("ALL CONFIGS:", value1)

}

func TestAll(t *testing.T) {
	setting.SetSettingPath("./../example/setting.json")

	s := GetStore()

	s.CheckAll()
}
