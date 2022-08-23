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

	value1, _ := s.GetAll(common.TYPE_COLLECTION_CONFIG)
	t.Log("ALL COLLECTION_CONFIGS:", value1)

	config := model.NewConfig("111", "1111", 1, "111")
	s.Add(common.TYPE_COLLECTION_CONFIG, config.Name, config)

	value2, _ := s.Get(common.TYPE_COLLECTION_CONFIG, "111")
	t.Log("COLLECTION_CONFIG 111:", value2)

	config.Description = "test"
	s.Update(common.TYPE_COLLECTION_CONFIG, config.Name, config)
	value2, _ = s.Get(common.TYPE_COLLECTION_CONFIG, "111")
	t.Log("COLLECTION_CONFIG 111:", value2)

	value1, _ = s.GetAll(common.TYPE_COLLECTION_CONFIG)
	t.Log("ALL COLLECTION_CONFIGS:", value1)

	//	s.Delete(common.TYPE_COLLECTION_CONFIG, "111")

	//	value1, _ = s.GetAll(common.TYPE_COLLECTION_CONFIG)
	//	t.Log("ALL COLLECTION_CONFIGS:", value1)

}
