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
