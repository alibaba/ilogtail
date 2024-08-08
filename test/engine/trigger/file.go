package trigger

import (
	"context"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	"golang.org/x/time/rate"

	"github.com/alibaba/ilogtail/test/config"
)

func GenerateLogToFile(ctx context.Context, speed, totalTime int, path string, templateStr string) (context.Context, error) {
	// clear file
	path = filepath.Join(config.CaseHome, path)
	path = filepath.Clean(path)
	_ = os.WriteFile(path, []byte{}, 0600)

	command := fmt.Sprintf("echo '%s' >> %s", templateStr, path)

	// interval = templateLen / speed
	interval := time.Microsecond * time.Duration(len(templateStr)/speed)

	limiter := rate.NewLimiter(rate.Every(interval), 1)

	timeout := time.After(time.Minute * time.Duration(totalTime))

	for {
		select {
		// context is done
		case <-ctx.Done():
			// clear file
			_ = os.WriteFile(path, []byte{}, 0600)
			return ctx, ctx.Err()
		// all time is done
		case <-timeout:
			// clear file
			_ = os.WriteFile(path, []byte{}, 0600)
			return ctx, nil
		default:
			if err := limiter.Wait(ctx); err != nil {
				return ctx, err
			}
			cmd := exec.Command("sh", "-c", command)
			_ = cmd.Run()
		}
	}
}
