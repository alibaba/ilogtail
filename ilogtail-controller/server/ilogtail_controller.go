package main

import (
	"ilogtail-controller/manager"
	"ilogtail-controller/message"
)

func main() {
	manager.InitManager()
	message.MessageMain()
	//	manager.Jsontest()
	//  manager.LeveldbTest()
	//	config.ConfigGroupTest()
	//	fmt.Println(*config.GetMyBaseSetting())
}
