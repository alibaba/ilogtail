package pluginmanager

import (
	"encoding/json"
	"net"
	"net/http"
	"testing"
	"time"
)

func TestExportLogtailLitsenPorts(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("/export/port", func(res http.ResponseWriter, req *http.Request) {
		if req.Method == "POST" {
			param := &struct {
				Status string `json:"status"`
				Ports  []int  `json:"ports"`
			}{}
			err := json.NewDecoder(req.Body).Decode(param)
			if err != nil {
				t.Log("Decode logtail's ports err", err.Error())
			} else {
				t.Log("Receive logtail's ports success", param.Ports)
			}
		}
	})
	server := &http.Server{
		Addr:    ":18689",
		Handler: mux,
	}
	go server.ListenAndServe()
	defer server.Close()

	listener1, _ := net.Listen("tcp", ":8080")
	defer listener1.Close()
	addr, err := net.ResolveUDPAddr("udp", ":80")
	if err != nil {
		t.Log(err.Error())
	}
	listener2, _ := net.ListenUDP("udp", addr)
	defer listener2.Close()

	ticker := time.NewTicker(2 * time.Second)
	count := 0
	for range ticker.C {
		count++

		ports, err := getLogtailLitsenPorts()
		if err != nil {
			t.Log(err.Error())
		} else {
			err = exportLogtailLitsenPorts(ports)
			if err != nil {
				t.Log(err.Error())
			}
		}

		if count == 3 {
			return
		}
	}
}
