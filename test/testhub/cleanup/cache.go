package cleanup

import "github.com/alibaba/ilogtail/test/testhub/setup"

func CleanupGoTestCache() {
	command := "/usr/local/go/bin/go clean -testcache"
	if err := setup.Env.Exec(command); err != nil {
		panic(err)
	}
}
