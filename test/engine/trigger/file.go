package trigger

import (
	"context"
	"os"
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
	file, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0600)

	limiter := rate.NewLimiter(rate.Limit(speed*1024*1024), len(templateStr))

	timeout := time.After(time.Minute * time.Duration(totalTime))

	for {
		select {
		// context is done
		case <-ctx.Done():
			// clear file
			_ = file.Close()
			return ctx, ctx.Err()
		// all time is done
		case <-timeout:
			// clear file
			_ = file.Close()
			return ctx, nil
		default:
			if limiter.AllowN(time.Now(), len(templateStr)) {
				_, _ = file.WriteString(templateStr + "\n")
			}
		}
	}
}
