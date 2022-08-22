package main

import (
	"github.com/alibaba/ilogtail/config_server/service/router"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func main() {
	defer store.GetStore().Close()
	router.InitRouter()
}
