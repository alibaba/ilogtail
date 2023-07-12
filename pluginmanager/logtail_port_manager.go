package pluginmanager

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
)

var exportLogtailPortsRunning = false

var exportLogtailPortsPort = 18689

func getExcludePorts() []int {
	ports := []int{}
	// logtail service port
	ports = append(ports, exportLogtailPortsPort)
	// env "HTTP_PROBE_PORT"
	checkAlivePort := os.Getenv("HTTP_PROBE_PORT")
	if len(checkAlivePort) != 0 {
		port, err := strconv.Atoi(checkAlivePort)
		if err == nil {
			ports = append(ports, port)
		}
	}
	return ports
}

func getListenPortsFromFile(pid int, protocol string) ([]int, error) {
	ports := []int{}
	filepath.Join()
	file := fmt.Sprintf("/proc/%d/net/%s", pid, protocol)
	data, err := ioutil.ReadFile(filepath.Clean(file))
	if err != nil {
		return ports, err
	}

	lines := strings.Split(string(data), "\n")
	for _, line := range lines[1:] {
		fields := strings.Fields(line)
		if len(fields) < 10 {
			continue
		}
		if strings.HasPrefix(protocol, "tcp") && fields[3] != "0A" {
			continue
		}
		if strings.HasPrefix(protocol, "udp") && fields[3] != "07" {
			continue
		}
		port, err := strconv.ParseUint((strings.Split(fields[1], ":")[1]), 16, 32)
		if err != nil {
			logger.Error(context.Background(), "parse port fail", err.Error())
			continue
		}
		ports = append(ports, int(port))
	}
	return ports, nil
}

func getLogtailLitsenPorts() ([]int, []int) {
	portsTCPMap := map[int]int{}
	portsUDPMap := map[int]int{}
	pid := os.Getpid()
	portsTCP := []int{}
	portsUDP := []int{}
	// get tcp ports
	tcpPorts, err := getListenPortsFromFile(pid, "tcp")
	if err != nil {
		logger.Error(context.Background(), "get tcp port fail", err.Error())
	}
	for _, port := range tcpPorts {
		portsTCPMap[port]++
	}
	// get tcp6 ports
	tcp6Ports, err := getListenPortsFromFile(pid, "tcp6")
	if err != nil {
		logger.Error(context.Background(), "get tcp6 port fail", err.Error())
	}
	for _, port := range tcp6Ports {
		portsTCPMap[port]++
	}
	// get udp ports
	udpPorts, err := getListenPortsFromFile(pid, "udp")
	if err != nil {
		logger.Error(context.Background(), "get udp port fail", err.Error())
	}
	for _, port := range udpPorts {
		portsUDPMap[port]++
	}
	// get udp6 ports
	udp6Ports, err := getListenPortsFromFile(pid, "udp6")
	if err != nil {
		logger.Error(context.Background(), "get udp6 port fail", err.Error())
	}
	for _, port := range udp6Ports {
		portsUDPMap[port]++
	}

	excludePorts := getExcludePorts()
	for port := range excludePorts {
		delete(portsTCPMap, port)
	}

	for port := range portsTCPMap {
		portsTCP = append(portsTCP, port)
	}
	for port := range portsUDPMap {
		portsUDP = append(portsUDP, port)
	}
	return portsTCP, portsUDP
}

func processPort(res http.ResponseWriter, req *http.Request) {
	portsTCP, portsUDP := getLogtailLitsenPorts()
	logger.Info(context.Background(), "get logtail's listen ports tcp", portsTCP, "udp", portsUDP)

	param := &struct {
		PortsTCP []int `json:"ports_tcp"`
		PortsUDP []int `json:"ports_udp"`
	}{}
	param.PortsTCP = portsTCP
	param.PortsUDP = portsUDP
	jsonBytes, err := json.Marshal(param)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		_, WriteErr := res.Write([]byte(err.Error()))
		if WriteErr != nil {
			logger.Error(context.Background(), "write response err", WriteErr.Error())
		}
		return
	}
	res.WriteHeader(http.StatusOK)
	_, WriteErr := res.Write(jsonBytes)
	if WriteErr != nil {
		logger.Error(context.Background(), "write response err", WriteErr.Error())
	}
}

func ExportLogtailPorts() {
	if !exportLogtailPortsRunning {
		go func() {
			mux := http.NewServeMux()
			mux.HandleFunc("/export/port", processPort)
			server := &http.Server{
				Addr:              ":" + strconv.Itoa(exportLogtailPortsPort),
				Handler:           mux,
				ReadHeaderTimeout: 10 * time.Second,
			}
			err := server.ListenAndServe()
			defer server.Close()
			if err != nil && err != http.ErrServerClosed {
				logger.Error(context.Background(), "export logtail's ports failed", err.Error())
				return
			}
		}()
		exportLogtailPortsRunning = true
	}
}
