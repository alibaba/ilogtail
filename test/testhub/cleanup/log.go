package cleanup

import (
	"fmt"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
)

func CleanupAllGeneratedLog() {
	command := fmt.Sprintf("rm -rf %s/*", config.TestConfig.GeneratedLogDir)
	if err := setup.Env.Exec(command); err != nil {
		panic(err)
	}
}
