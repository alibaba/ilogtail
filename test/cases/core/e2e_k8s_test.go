// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package core

import (
	"testing"

	"github.com/cucumber/godog"

	"github.com/alibaba/ilogtail/test/testhub/cleanup"
)

func TestE2EOnK8s(t *testing.T) {
	defer cleanup.All()
	suite := godog.TestSuite{
		Name:                "E2EOnK8s",
		ScenarioInitializer: scenarioInitializer,
		Options: &godog.Options{
			Format:   "pretty",
			Paths:    []string{"scenarios"},
			Tags:     "@e2e && @k8s",
			TestingT: t,
		},
	}
	if suite.Run() != 0 {
		t.Fail()
	}
}
