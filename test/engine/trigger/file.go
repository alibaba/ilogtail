package trigger

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"golang.org/x/time/rate"
)

func GenerateLogToFile(ctx context.Context, speed, totalTime int, path string, templateStr string) (context.Context, error) {
	// 打开或创建文件
	if !filepath.IsAbs(path) {
		path = filepath.Join(config.CaseHome, path)
	}
	file, err := os.OpenFile(path, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0600)
	if err != nil {
		return ctx, err
	}

	// interval = templateLen / speed
	interval := time.Microsecond * time.Duration(len(templateStr)/speed)

	limiter := rate.NewLimiter(rate.Every(interval), 1)

	// 总时间控制
	timeout := time.After(time.Minute * time.Duration(totalTime))

	for {
		select {
		case <-ctx.Done(): // 上下文取消
			if err := file.Close(); err != nil {
				return ctx, err
			}
			return ctx, ctx.Err()
		case <-timeout: // 总时间到
			if err := file.Close(); err != nil {
				return ctx, err
			}
			return ctx, nil
		default:
			if err := limiter.Wait(ctx); err != nil {
				return ctx, err
			}
			if _, err := fmt.Fprintln(file, templateStr); err != nil {
				return ctx, err
			}
		}
	}
}
