package cleanup

import (
	"context"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

func CleanupAll() {
	ctx := context.TODO()
	control.RemoveAllLocalConfig(ctx)
	CleanupAllGeneratedLog(ctx)
	CleanupGoTestCache(ctx)
}
