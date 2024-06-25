// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package control

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
	"github.com/alibaba/ilogtail/test/testhub/setup/subscriber"
)

const iLogtailLocalConfigDir = "/etc/ilogtail/config/local"

func AddLocalConfig(ctx context.Context, configName, c string) (context.Context, error) {
	c = completeConfigWithFlusher(c)
	fmt.Println(config.ConfigDir)
	if setup.Env.GetType() == "docker-compose" {
		// write local file
		if _, err := os.Stat(config.ConfigDir); os.IsNotExist(err) {
			if err := os.MkdirAll(config.ConfigDir, 0750); err != nil {
				return ctx, err
			}
		} else if err != nil {
			return ctx, err
		}
		filePath := fmt.Sprintf("%s/%s.yaml", config.ConfigDir, configName)
		err := os.WriteFile(filePath, []byte(c), 0644)
		if err != nil {
			return ctx, err
		}
	} else {
		command := fmt.Sprintf(`cd %s && cat << 'EOF' > %s.yaml
	%s`, iLogtailLocalConfigDir, configName, c)
		if err := setup.Env.ExecOnLogtail(command); err != nil {
			return ctx, err
		}
	}
	return ctx, nil
}

func RemoveAllLocalConfig(ctx context.Context) (context.Context, error) {
	command := fmt.Sprintf("cd %s && rm -rf *.yaml", iLogtailLocalConfigDir)
	if err := setup.Env.ExecOnLogtail(command); err != nil {
		return ctx, err
	}
	return ctx, nil
}

func completeConfigWithFlusher(c string) string {
	if strings.Index(c, "flushers") != -1 {
		return c
	}
	return c + subscriber.TestSubscriber.FlusherConfig()
}
