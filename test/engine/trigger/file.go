package trigger

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"time"

	"github.com/alibaba/ilogtail/test/config"
)

func GenerateLogToFile(ctx context.Context, cnt, interval, totalTime int, path string, templateStr string) (context.Context, error) {
	// 打开或创建文件
	if !filepath.IsAbs(path) {
		path = filepath.Join(config.CaseHome, path)
	}
	file, err := os.OpenFile(path, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644)
	if err != nil {
		return ctx, err
	}
	defer file.Close()

	// 创建定时器
	ticker := time.NewTicker(time.Millisecond * time.Duration(interval))
	defer ticker.Stop()

	// 总时间控制
	timeout := time.After(time.Minute * time.Duration(totalTime))

	for {
		select {
		case <-ctx.Done(): // 上下文取消
			return ctx, ctx.Err()
		case <-timeout: // 总时间到
			return ctx, nil
		case <-ticker.C: // 定时器触发
			for i := 0; i < cnt; i++ {
				if _, err := fmt.Fprintln(file, templateStr); err != nil {
					return ctx, err
				}
			}
		}
	}
}
