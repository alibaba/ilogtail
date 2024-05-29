package cleanup

import (
	"context"

	"github.com/alibaba/ilogtail/test/testhub/setup"
)

func CleanupGoTestCache(ctx context.Context) (context.Context, error) {
	command := "/usr/local/go/bin/go clean -testcache"
	if err := setup.Env.ExecOnSource(command); err != nil {
		return ctx, err
	}
	return ctx, nil
}
