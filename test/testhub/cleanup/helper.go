package cleanup

import (
	"context"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

func All() {
	ctx := context.TODO()
	_, _ = control.RemoveAllLocalConfig(ctx)
	_, _ = AllGeneratedLog(ctx)
	_, _ = GoTestCache(ctx)
}
