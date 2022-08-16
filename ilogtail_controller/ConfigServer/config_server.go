package main

import (
	"fmt"

	conf "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
)

func main() {
	fmt.Println(conf.GetSetting())
}
