package main

import (
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/service"
)

func main() {
	manager.InitManager()
	service.MessageMain()
	//	manager.Jsontest()
	//  manager.LeveldbTest()
	//	config.ConfigGroupTest()
	//	fmt.Println(*config.GetMyBaseSetting())
}
