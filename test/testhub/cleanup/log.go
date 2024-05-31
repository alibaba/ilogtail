package cleanup

import (
	"context"
	"fmt"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
)

func AllGeneratedLog(ctx context.Context) (context.Context, error) {
	command := fmt.Sprintf("rm -rf %s/*", config.TestConfig.GeneratedLogDir)
	if err := setup.Env.ExecOnSource(command); err != nil {
		return ctx, err
	}
	return ctx, nil
}
