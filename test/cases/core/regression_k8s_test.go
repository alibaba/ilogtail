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

	"github.com/alibaba/ilogtail/test/testhub/cleanup"
	"github.com/alibaba/ilogtail/test/testhub/control"
	"github.com/alibaba/ilogtail/test/testhub/setup"
	"github.com/alibaba/ilogtail/test/testhub/trigger"
	"github.com/alibaba/ilogtail/test/testhub/verify"
	"github.com/cucumber/godog"
)

func TestRegressionOnK8s(t *testing.T) {
	defer cleanup.All()
	suite := godog.TestSuite{
		Name: "RegressionOnK8s",
		ScenarioInitializer: func(ctx *godog.ScenarioContext) {
			ctx.Given(`^\{(\S+)\} environment$`, setup.InitEnv)
			ctx.Given(`^\{(\S+)\} config as below`, control.AddLocalConfig)
			ctx.When(`add k8s label \{(.*)\}`, control.AddLabel)
			ctx.When(`remove k8s label \{(.*)\}`, control.RemoveLabel)
			ctx.When(`^generate \{(\d+)\} regex logs, with interval \{(\d+)\}ms$`, trigger.RegexSingle)
			ctx.Then(`^there is \{(\d+)\} logs$`, verify.LogCount)
		},
		Options: &godog.Options{
			Format:   "pretty",
			Paths:    []string{"scenarios"},
			Tags:     "@regression && @k8s",
			TestingT: t,
		},
	}
	if suite.Run() != 0 {
		t.Fail()
	}
}
