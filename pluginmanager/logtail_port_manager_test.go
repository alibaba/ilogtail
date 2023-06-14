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

	listener1, err := net.Listen("tcp", ":18688")
	if err != nil {
		t.Log(err.Error())
	} else {
		defer listener1.Close()
	}
	addr, err := net.ResolveUDPAddr("udp", ":18687")
	if err != nil {
		t.Log(err.Error())
	}
	listener2, err := net.ListenUDP("udp", addr)
	if err != nil {
		t.Log(err.Error())
	} else {
		defer listener2.Close()
	}

	ticker := time.NewTicker(2 * time.Second)
	count := 0
	for range ticker.C {
		count++

		ports := getLogtailLitsenPorts()

		err := exportLogtailLitsenPorts(ports)
		if err != nil {
			t.Log(err.Error())
		}

		if count == 3 {
			return
		}
	}
}
