package cleanup

import "github.com/alibaba/ilogtail/test/testhub/control"

func CleanupAll() {
	control.RemoveAllLocalConfig()
	CleanupAllGeneratedLog()
	CleanupGoTestCache()
}
