package main

import (
	"github.com/alibaba/ilogtail/config_server/config_server_service/router"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store"
)

func main() {
	defer store.GetStore().Close()
	router.InitRouter()
}
