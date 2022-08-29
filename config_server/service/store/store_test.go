// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
