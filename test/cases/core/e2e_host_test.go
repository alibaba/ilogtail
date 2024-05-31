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

func TestE2EOnHost(t *testing.T) {
	defer cleanup.All()
	suite := godog.TestSuite{
		Name: "E2EOnHost",
		ScenarioInitializer: func(ctx *godog.ScenarioContext) {
			ctx.Given(`^\{(\S+)\} environment$`, setup.InitEnv)
			ctx.Given(`^\{(\S+)\} config as below`, control.AddLocalConfig)
			ctx.When(`^generate \{(\d+)\} regex logs, with interval \{(\d+)\}ms$`, trigger.RegexSingle)
			ctx.Then(`^there is \{(\d+)\} logs$`, verify.LogCount)
			ctx.Then(`^the log contents match regex single`, verify.RegexSingle)
		},
		Options: &godog.Options{
			Format:   "pretty",
			Paths:    []string{"scenarios"},
			Tags:     "@e2e && @host",
			TestingT: t,
		},
	}
	if suite.Run() != 0 {
		t.Fail()
	}
}
