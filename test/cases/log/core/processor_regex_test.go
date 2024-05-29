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

func TestRegexSingle(t *testing.T) {
	defer cleanup.CleanupAll()
	suite := godog.TestSuite{
		Name: "TestRegexSingle",
		ScenarioInitializer: func(ctx *godog.ScenarioContext) {
			ctx.Given(`^(\S+) environment$`, setup.InitEnv)
			ctx.Given(`^(\S+) config as below`, control.AddLocalConfig)
			ctx.When(`^generate (\d+) regex logs, with interval (\d+)ms$`, trigger.TriggerRegexSingle)
			ctx.Then(`^there is (\d+) logs$`, verify.VerifyLogCount)
			ctx.Then(`^the log contents match regex single`, verify.VerifyRegexSingle)
		},
		Options: &godog.Options{
			Format:   "pretty",
			Paths:    []string{"scenarios"},
			Tags:     "@e2e",
			TestingT: t,
		},
	}
	if suite.Run() != 0 {
		t.Fail()
	}
}
