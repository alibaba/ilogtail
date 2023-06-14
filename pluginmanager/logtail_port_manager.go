package pluginmanager

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
)

var exportLogtailPortsRunning = false

var exportLogtailPortsInterval = 30 * time.Second

func getLogtailLitsenPorts() ([]int, error) {
	portsMap := map[int]int{}
	pid := os.Getpid()
	cmd1 := exec.Command("netstat", "-lnp")
	cmd2 := exec.Command("grep", strconv.Itoa(pid))

	stdout1, err := cmd1.StdoutPipe()
	if err != nil {
		return nil, err
	}
	defer stdout1.Close()
	err = cmd1.Start()
	if err != nil {
		return nil, err
	}

	stdout2, err := cmd2.StdoutPipe()
	if err != nil {
		return nil, err
	}
	defer stdout2.Close()
	cmd2.Stdin = stdout1
	err = cmd2.Start()
	if err != nil {
		return nil, err
	}

	output, err := ioutil.ReadAll(stdout2)
	if err != nil {
		return nil, err
	}

	lines := strings.Split(string(output), "\n")
	for _, line := range lines {
		fields := strings.Fields(line)
		if len(fields) < 7 {
			continue
		}
		port, err := strconv.Atoi(strings.Split(fields[3], ":")[3])
		if err != nil {
			return nil, err
		}
		portsMap[port]++
	}
	ports := []int{}
	for port := range portsMap {
		ports = append(ports, port)
	}
	return ports, nil
}

func exportLogtailLitsenPorts(ports []int) error {
	param := &struct {
		Status string `json:"status"`
		Ports  []int  `json:"ports"`
	}{}
	param.Status = "success"
	param.Ports = ports
	jsonBytes, err := json.Marshal(param)
	if err != nil {
		return err
	}
	client := &http.Client{}
	req, err := http.NewRequest("POST", "http://127.0.0.1:18689/export/port", bytes.NewBuffer(jsonBytes))
	if err != nil {
		return err
	}
	res, err := client.Do(req)
	if err != nil {
		return err
	}
	if res.StatusCode != 200 {
		return errors.New(res.Status)
	}
	return nil
}

func ExportLogtailPorts() {
	exportLogtailPorts := func() {
		exportLogtailPortsTicker := time.NewTicker(exportLogtailPortsInterval)
		for range exportLogtailPortsTicker.C {
			ports, err := getLogtailLitsenPorts()
			if err != nil {
				logger.Error(context.Background(), "get logtail's listen ports failed", err.Error())
				continue
			}
			logger.Info(context.Background(), "get logtail's listen ports success", ports)

			err = exportLogtailLitsenPorts(ports)
			if err != nil {
				logger.Error(context.Background(), "export logtail's listen ports failed", err.Error())
				continue
			}
			logger.Info(context.Background(), "export logtail's listen ports success", ports)
		}
	}
	if !exportLogtailPortsRunning {
		go exportLogtailPorts()
		exportLogtailPortsRunning = true
	}
}
