package trigger

import (
	"context"
	"math/rand"
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
	file, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0600) // #nosec G304

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

func GenerateRandomJsonLogToFile(ctx context.Context, speed, totalTime int, path string) (context.Context, error) {
	// json template
	jsonTemplates := []string{
		`{"url": "POST /PutData?Category=paskdnkwja HTTP/1.1", "ip": "10.200.98.220", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "18"}, "time": "12/Sep/2024:11:30:02"}`,
		`{"url": "GET /PutData?Category=dwds HTTP/1.1", "ip": "10.7.159.1", "user-agent": "aliyun-sdk-java", "request": {"status": "404", "latency": "22024"}, "time": "06/Jun/2001:09:35:59"}`,
		`{"url": "GET /PutData?Category=ubkjbkjgiuiu HTTP/1.1", "ip": "172.130.98.250", "user-agent": "aliyun-sdk-java", "request": {"301": "200", "latency": "12334"}, "time": "08/Aug/2018:10:30:28"}`,
		`{"url": "POST /PutData?Category=oasjdwbjkbjkg HTTP/1.1", "ip": "1.20.9.220", "user-agent": "aliyun-sdk-java", "request": {"401": "200", "latency": "112"}, "time": "09/Sep/2024:15:25:28"}`,
		`{"url": "POST /PutData?Category=asdjhoiasjdoOpLog HTTP/1.1", "ip": "172.168.0.1", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "8815"}, "time": "01/Jan/2022:10:30:28"}`,
	}
	maxLen := 0
	for i := 0; i < len(jsonTemplates); i++ {
		if len(jsonTemplates[i]) > maxLen {
			maxLen = len(jsonTemplates[i])
		}
	}

	// clear file
	path = filepath.Join(config.CaseHome, path)
	path = filepath.Clean(path)
	_ = os.WriteFile(path, []byte{}, 0600)
	file, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0600) // #nosec G304

	limiter := rate.NewLimiter(rate.Limit(speed*1024*1024), maxLen)

	timeout := time.After(time.Minute * time.Duration(totalTime))

	rand.Seed(time.Now().UnixNano())

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
			if limiter.AllowN(time.Now(), maxLen) {
				randomIndex := rand.Intn(len(jsonTemplates))
				_, _ = file.WriteString(jsonTemplates[randomIndex] + "\n")
			}
		}
	}
}
