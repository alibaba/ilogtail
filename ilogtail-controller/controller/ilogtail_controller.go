package main

import (
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/message"
)

func main() {
	manager.InitManager()
	message.MessageMain()
	//	manager.Jsontest()
	//  manager.LeveldbTest()
	//	config.ConfigGroupTest()
	//	fmt.Println(*config.GetMyBaseSetting())
}
