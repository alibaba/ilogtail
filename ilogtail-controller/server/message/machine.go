package message

import (
	"fmt"
	"ilogtail-controller/manager"
	"net/http"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
)

type MACO struct {
	Machines []string `form:"machines" json:"machines" uri:"machines" xml:"machines" binding:"required"`
	Configs  []string `form:"configs" json:"configs" uri:"configs" xml:"configs" binding:"required"`
}

func machineRegister(c *gin.Context) {
	uid := c.Param("uid")
	heart := time.Now().Format("2006-01-02 15:04:05")
	ip := c.ClientIP()
	_, ok := manager.MachineList[uid]
	if ok {
		manager.MachineList[uid].Heartbeat = heart
		c.String(http.StatusOK, "Heart Beat successfully!")
	} else {
		manager.AddMachine(manager.NewMachine(uid, "", ip, "", "good", heart))
		manager.Update()
		c.String(http.StatusOK, "Register successfully!")
	}
}

func addConfigToMachine(c *gin.Context) {
	// 声明接收的变量
	var mc MACO
	// Bind()默认解析并绑定form格式
	// 根据请求头中content-type自动推断
	if err := c.Bind(&mc); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	for _, m := range mc.Machines {
		ip := manager.MachineList[m].Ip
		for _, co := range mc.Configs {
			body := strings.NewReader(co)
			response, err := http.Post(ip+"/", "application/json; charset=utf-8", body)
			if err != nil || response.StatusCode != http.StatusOK {
				c.Status(http.StatusServiceUnavailable)
			}
		}
	}
	c.JSON(http.StatusOK, gin.H{"status": "200"})
}

func delConfigFromMachine(c *gin.Context) {
	// 声明接收的变量
	var mc MACO
	// Bind()默认解析并绑定form格式
	// 根据请求头中content-type自动推断
	if err := c.Bind(&mc); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	for _, m := range mc.Machines {
		ip := manager.MachineList[m].Ip
		for _, co := range mc.Configs {
			body := strings.NewReader(co)
			response, err := http.Post(ip+"/", "application/json; charset=utf-8", body)
			if err != nil || response.StatusCode != http.StatusOK {
				c.Status(http.StatusServiceUnavailable)
			}
		}
	}
	c.JSON(http.StatusOK, gin.H{"status": "200"})
}

func getMachineList(c *gin.Context) {
	data := gin.H{}
	queryTime := time.Now().Format("2006-01-02 15:04:05")
	for k := range manager.MachineList {
		NowTime, _ := time.Parse("2006-01-02 15:04:05", queryTime)
		preTime, _ := time.Parse("2006-01-02 15:04:05", manager.MachineList[k].Heartbeat)
		preHeart := NowTime.Sub(preTime)
		fmt.Println(preHeart)
		if preHeart.Seconds() < 15 {
			manager.MachineList[k].State = "good"
		} else if preHeart.Seconds() < 60 {
			manager.MachineList[k].State = "bad"
		} else {
			manager.MachineList[k].State = "lost"
		}
		data[k] = manager.MachineList[k]
	}
	c.JSON(http.StatusOK, data)
}
