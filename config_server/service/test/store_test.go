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

package test

import (
	"testing"

	. "github.com/smartystreets/goconvey/convey"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func TestStore(t *testing.T) {
	Convey("Test load store, check it's type.", t, func() {
		So(store.GetStore().GetMode(), ShouldEqual, "leveldb")
	})

	Convey("Test store's CURD, take config as example.", t, func() {
		s := store.GetStore()

		Convey("First, there is no config in store.", func() {
			configs, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)
			So(err, ShouldBeNil)
			So(configs, ShouldBeEmpty)
		})

		Convey("Second, add a config named config-1 to store.", func() {
			config := new(model.Config)
			config.Name = "config-1"
			config.AgentType = "iLogtail"
			config.Content = "test"
			config.Version = 1
			config.Description = ""
			config.DelTag = false
			s.Add(common.TYPE_COLLECTION_CONFIG, config.Name, config)

			value, err := s.Get(common.TYPE_COLLECTION_CONFIG, "config-1")
			So(err, ShouldBeNil)
			So(value.(*model.Config), ShouldResemble, &model.Config{Name: "config-1", AgentType: "iLogtail", Content: "test", Version: 1, Description: "", DelTag: false})
		})

		Convey("Third, update config-1's content.", func() {
			value, err := s.Get(common.TYPE_COLLECTION_CONFIG, "config-1")
			So(err, ShouldBeNil)
			config := value.(*model.Config)
			config.Description = "test"
			s.Update(common.TYPE_COLLECTION_CONFIG, config.Name, config)

			value, err = s.Get(common.TYPE_COLLECTION_CONFIG, "config-1")
			So(err, ShouldBeNil)
			So(value.(*model.Config), ShouldResemble, &model.Config{Name: "config-1", AgentType: "iLogtail", Content: "test", Version: 1, Description: "test", DelTag: false})
		})

		Convey("Fourth, delete config-1.", func() {
			s.Delete(common.TYPE_COLLECTION_CONFIG, "config-1")
			_, err := s.Get(common.TYPE_COLLECTION_CONFIG, "config-1")
			So(err, ShouldNotBeNil)
		})

		Convey("Fifth, there is no config in store.", func() {
			configs, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)
			So(err, ShouldBeNil)
			So(configs, ShouldBeEmpty)
		})
	})
}
