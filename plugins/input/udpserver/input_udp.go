// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package udpserver

import (
	"net"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper/decoder"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type UDPServer struct {
	Format         string
	Address        string
	MaxBufferSize  int
	ReadTimeoutSec int

	context   ilogtail.Context
	decoder   decoder.Decoder
	addr      *net.UDPAddr
	conn      *net.UDPConn
	collector ilogtail.Collector
}

func (u *UDPServer) Init(context ilogtail.Context) (int, error) {
	u.context = context
	var err error
	if u.decoder, err = decoder.GetDecoder(u.Format); err != nil {
		return 0, err
	}

	host, portStr, err := net.SplitHostPort(u.Address)
	if err != nil {
		logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "illegal udp listening addr", u.Address, "err", err)
		return 0, err
	}
	if host == "" {
		host = "0.0.0.0"
	}

	ip, err := net.ResolveIPAddr("ip", host)
	if err != nil {
		logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "unable resolve addr", u.Address, "err", err)
		return 0, err
	}

	port, err := strconv.Atoi(portStr)
	if err != nil || port < 0 || port > 65535 {
		logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "illegal port", portStr, "err", err)
	}

	u.addr = &net.UDPAddr{
		IP:   ip.IP,
		Port: port,
		Zone: ip.Zone,
	}
	return 0, nil
}

func (u *UDPServer) Description() string {
	return "this is an udp listening server"
}

func (u *UDPServer) Start(collector ilogtail.Collector) error {
	u.collector = collector
	err := u.doStart(u.dispatcher)
	logger.Infof(u.context.GetRuntimeContext(), "start udp server, status", err == nil)
	return err
}

func (u *UDPServer) doStart(dispatchFunc func(logs []*protocol.Log)) error {
	var err error
	u.conn, err = net.ListenUDP("udp", u.addr)
	if err != nil {
		logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "start udp server err", err)
		return err
	}
	go func() {
		buf := make([]byte, u.MaxBufferSize)
		defer func() {
			logger.Debug(u.context.GetRuntimeContext(), "release udp read goroutine")
		}()
		for {
			n, _, err := u.conn.ReadFromUDP(buf)
			if err != nil {
				// https://github.com/golang/go/issues/4373
				// ignore net: errClosing error as it will occur during shutdown
				if strings.HasSuffix(err.Error(), "use of closed network connection") {
					return
				}
				logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "read record err", err)
				return
			}
			logs, err := u.decoder.Decode(buf[:n], nil)
			if err != nil {
				logger.Error(u.context.GetRuntimeContext(), "UDP_SERVER_ALARM", "decode record err", err)
			}
			dispatchFunc(logs)
		}
	}()
	return nil
}

func (u *UDPServer) Stop() error {
	_ = u.conn.Close()
	u.conn = nil
	logger.Infof(u.context.GetRuntimeContext(), "stop udp server, success")
	return nil
}

func (u *UDPServer) dispatcher(logs []*protocol.Log) {
	for _, log := range logs {
		u.collector.AddRawLog(log)
	}
}

func init() {
	ilogtail.ServiceInputs["service_udp_server"] = func() ilogtail.ServiceInput {
		return &UDPServer{
			MaxBufferSize:  65535,
			ReadTimeoutSec: 2,
		}
	}
}
