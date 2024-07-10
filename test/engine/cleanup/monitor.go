package cleanup

import (
	"context"

	"github.com/alibaba/ilogtail/test/engine/setup/monitor"
)

func StopMonitor(ctx context.Context) (context.Context, error) {
	return monitor.StopMonitor(ctx)
}
