// Copyright 2021 iLogtail Authors
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

package httpserver

import (
	"bytes"
	"context"
	"encoding/binary"
	"encoding/json"
	"io"
	"net"
	"net/http"
	"net/url"
	"os"
	"path"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/decoder"
	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	v1 = iota
	v2
)

type dumpData struct {
	body   []byte
	url    []byte
	header []byte
}

// ServiceHTTP ...
type ServiceHTTP struct {
	context           ilogtail.Context
	collector         ilogtail.Collector
	decoder           decoder.Decoder
	server            *http.Server
	listener          net.Listener
	wg                sync.WaitGroup
	collectorV2       ilogtail.PipelineCollector
	version           int8
	paramCount        int
	dumpDataChan      chan *dumpData
	stopChan          chan struct{}
	dumpDataKeepFiles []string

	DumpDataKeepFiles  int
	DumpData           bool // would dump the received data to a local file, which is only used to valid data by the developers. And the maximum size of file is 100M.
	Format             string
	Address            string
	Endpoint           string
	ReadTimeoutSec     int
	ShutdownTimeoutSec int
	MaxBodySize        int64
	UnlinkUnixSock     bool
	FieldsExtend       bool
	DisableUncompress  bool

	// params below works only for version v2
	QueryParams       []string
	HeaderParams      []string
	QueryParamPrefix  string
	HeaderParamPrefix string
}

// Init ...
func (s *ServiceHTTP) Init(context ilogtail.Context) (int, error) {
	s.context = context
	var err error
	if s.decoder, err = decoder.GetDecoderWithOptions(s.Format, decoder.Option{FieldsExtend: s.FieldsExtend, DisableUncompress: s.DisableUncompress}); err != nil {
		return 0, err
	}
	if s.Endpoint == "" {
		switch s.Format {
		case common.ProtocolOTLPLogV1:
			s.Endpoint = "/v1/logs"
		case common.ProtocolPyroscope:
			s.Endpoint = "/ingest"
		}
	}
	s.Address += s.Endpoint
	if strings.Contains(s.Address, "SELF_ADDR") {
		s.Address = strings.ReplaceAll(s.Address, "SELF_ADDR", util.GetIPAddress())
	}
	logger.Infof(context.GetRuntimeContext(), "addr", s.Address, "format", s.Format)

	s.paramCount = len(s.QueryParams) + len(s.HeaderParams)

	if s.DumpData {
		s.dumpDataChan = make(chan *dumpData, 10)
		prefix := strings.Join([]string{s.context.GetProject(), s.context.GetLogstore(), s.context.GetConfigName()}, "_")
		files, err := helper.GetFileListByPrefix(util.GetCurrentBinaryPath(), prefix, true, 0)
		if err != nil {
			logger.Warning(context.GetRuntimeContext(), "LIST_HISTORY_DUMP_ALARM", "err", err)
		} else {
			s.dumpDataKeepFiles = files
		}
	}
	s.stopChan = make(chan struct{})
	return 0, nil
}

// Description ...
func (s *ServiceHTTP) Description() string {
	return "HTTP service input plugin for logtail"
}

// Collect takes in an accumulator and adds the metrics that the Input
// gathers. This is called every "interval"
func (s *ServiceHTTP) Collect(ilogtail.Collector) error {
	return nil
}

func (s *ServiceHTTP) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.ContentLength > s.MaxBodySize {
		tooLarge(w)
		return
	}
	data, statusCode, err := s.decoder.ParseRequest(w, r, s.MaxBodySize)
	logger.Debugf(s.context.GetRuntimeContext(), "request [method] %v; [header] %v; [url] %v; [body len] %d", r.Method, r.Header, r.URL, len(data))
	switch statusCode {
	case http.StatusBadRequest:
		badRequest(w)
	case http.StatusRequestEntityTooLarge:
		tooLarge(w)
	case http.StatusInternalServerError:
		internalServerError(w)
	case http.StatusMethodNotAllowed:
		methodNotAllowed(w)
	}
	if err != nil {
		logger.Warning(s.context.GetRuntimeContext(), "READ_BODY_FAIL_ALARM", "read body failed", err, "request", r.URL.String())
		return
	}

	if s.DumpData {
		b, _ := json.Marshal(r.Header)
		s.dumpDataChan <- &dumpData{
			body:   data,
			url:    []byte(r.URL.String()),
			header: b,
		}
	}
	switch s.version {
	case v1:
		logs, err := s.decoder.Decode(data, r)
		if err != nil {
			logger.Warning(s.context.GetRuntimeContext(), "DECODE_BODY_FAIL_ALARM", "decode body failed", err, "request", r.URL.String())
			badRequest(w)
			return
		}
		for _, log := range logs {
			s.collector.AddRawLog(log)
		}
	case v2:
		groups, err := s.decoder.DecodeV2(data, r)
		if err != nil {
			logger.Warning(s.context.GetRuntimeContext(), "DECODE_BODY_FAIL_ALARM", "decode body failed", err, "request", r.URL.String())
			badRequest(w)
			return
		}
		if reqParams := s.extractRequestParams(r); len(reqParams) != 0 {
			for _, g := range groups {
				g.Group.Metadata.Merge(models.NewMetadataWithMap(reqParams))
			}
		}
		s.collectorV2.CollectList(groups...)
	}

	if s.Format == "sls" {
		w.Header().Set("x-log-requestid", "1234567890abcde")
		w.WriteHeader(http.StatusOK)
	} else if s.Format != common.ProtocolPyroscope {
		w.WriteHeader(http.StatusNoContent)
	}
}

func tooLarge(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusRequestEntityTooLarge)
	_, _ = res.Write([]byte(`{"error":"http: request body too large"}`))
}

func methodNotAllowed(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusMethodNotAllowed)
	_, _ = res.Write([]byte(`{"error":"http: method not allowed"}`))
}

func internalServerError(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusInternalServerError)
}

func badRequest(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusBadRequest)
	_, _ = res.Write([]byte(`{"error":"http: bad request"}`))
}

// Start starts the ServiceInput's service, whatever that may be
func (s *ServiceHTTP) Start(c ilogtail.Collector) error {
	s.collector = c
	s.version = v1
	return s.start()
}

// StartService start the ServiceInput's service by plugin runner v2
func (s *ServiceHTTP) StartService(context ilogtail.PipelineContext) error {
	s.collectorV2 = context.Collector()
	s.version = v2

	return s.start()
}

func (s *ServiceHTTP) start() error {
	s.wg.Add(2)

	server := &http.Server{
		Addr:        s.Address,
		Handler:     s,
		ReadTimeout: time.Duration(s.ReadTimeoutSec) * time.Second,
	}
	var listener net.Listener
	var err error
	switch {
	case strings.HasPrefix(s.Address, "unix"):
		sockPath := strings.Replace(s.Address, "unix://", "", 1)
		if s.UnlinkUnixSock {
			_ = syscall.Unlink(sockPath)
		}
		listener, err = net.Listen("unix", sockPath)
	case strings.HasPrefix(s.Address, "http") ||
		strings.HasPrefix(s.Address, "https") ||
		strings.HasPrefix(s.Address, "tcp"):
		configURL, errAddr := url.Parse(s.Address)
		if errAddr != nil {
			return errAddr
		}
		listener, err = net.Listen("tcp", configURL.Host)
	default:
		listener, err = net.Listen("tcp", s.Address)
	}
	if err != nil {
		return err
	}
	s.listener = listener
	s.server = server
	go func() {
		logger.Info(s.context.GetRuntimeContext(), "http server start", s.Address)
		_ = server.Serve(listener)
		ctx, cancel := context.WithTimeout(context.Background(), time.Duration(s.ShutdownTimeoutSec)*time.Second)
		defer cancel()
		_ = server.Shutdown(ctx)
		logger.Info(s.context.GetRuntimeContext(), "http server shutdown", s.Address)
		s.wg.Done()
	}()
	go func() {
		defer s.wg.Done()
		if s.DumpData {
			s.doDumpFile()
		}
	}()

	return nil
}

func (s *ServiceHTTP) doDumpFile() {
	var err error
	fileName := strings.Join([]string{s.context.GetProject(), s.context.GetLogstore(), s.context.GetConfigName()}, "_") + ".dump"
	logger.Info(s.context.GetRuntimeContext(), "http server start dump data", fileName)
	var f *os.File
	closeFile := func() {
		if f != nil {
			_ = f.Close()
		}
	}
	cutFile := func() (f *os.File, err error) {
		nFile := path.Join(util.GetCurrentBinaryPath(), fileName+"_"+time.Now().Format("2006-01-02_15"))
		if len(s.dumpDataKeepFiles) == 0 || s.dumpDataKeepFiles[len(s.dumpDataKeepFiles)-1] != nFile {
			s.dumpDataKeepFiles = append(s.dumpDataKeepFiles, nFile)
		}
		if len(s.dumpDataKeepFiles) > s.DumpDataKeepFiles {
			_ = os.Remove(s.dumpDataKeepFiles[0])
			s.dumpDataKeepFiles = s.dumpDataKeepFiles[1:]
		}
		closeFile()
		return os.OpenFile(s.dumpDataKeepFiles[len(s.dumpDataKeepFiles)-1], os.O_CREATE|os.O_WRONLY, 0777)
	}
	lastHour := 0
	offset := int64(0)
	for {
		select {
		case d := <-s.dumpDataChan:
			if time.Now().Hour() != lastHour {
				file, err := cutFile()
				if err != nil {
					logger.Error(s.context.GetRuntimeContext(), "DUMP_FILE_ALARM", "cut new file error", err)
				} else {
					offset, _ = file.Seek(0, io.SeekEnd)
					f = file
				}
			}
			if f != nil {
				buffer := bytes.NewBuffer([]byte{})
				// 4Byte url len, 4Byte body len,4 byte header len,  url, body, header(serialized by json)
				if err = binary.Write(buffer, binary.BigEndian, uint32(len(d.url))); err != nil {
					continue
				}
				if err = binary.Write(buffer, binary.BigEndian, uint32(len(d.body))); err != nil {
					continue
				}
				if err = binary.Write(buffer, binary.BigEndian, uint32(len(d.header))); err != nil {
					continue
				}
				total := 12 + len(d.url) + len(d.body) + len(d.header)
				if _, err = f.WriteAt(buffer.Bytes(), offset); err != nil {
					continue
				}
				if _, err = f.WriteAt(d.url, offset+12); err != nil {
					continue
				}
				if _, err = f.WriteAt(d.body, offset+12+int64(len(d.url))); err != nil {
					continue
				}
				if _, err = f.WriteAt(d.header, offset+12+int64(len(d.url)+len(d.body))); err != nil {
					continue
				}
				offset += int64(total)
			}
		case <-s.stopChan:
			closeFile()
			return
		}
	}
}

func (s *ServiceHTTP) extractRequestParams(req *http.Request) map[string]string {
	keyValues := make(map[string]string, s.paramCount)
	for _, key := range s.QueryParams {
		value := req.FormValue(key)
		if len(value) == 0 {
			continue
		}
		if len(s.QueryParamPrefix) > 0 {
			builder := strings.Builder{}
			builder.WriteString(s.QueryParamPrefix)
			builder.WriteString(key)
			key = builder.String()
		}
		keyValues[key] = value
	}
	for _, key := range s.HeaderParams {
		value := req.Header.Get(key)
		if len(value) == 0 {
			continue
		}
		if len(s.HeaderParamPrefix) > 0 {
			builder := strings.Builder{}
			builder.WriteString(s.HeaderParamPrefix)
			builder.WriteString(key)
			key = builder.String()
		}
		keyValues[key] = value
	}
	return keyValues
}

// Stop stops the services and closes any necessary channels and connections
func (s *ServiceHTTP) Stop() error {
	if s.listener != nil {
		close(s.stopChan)
		_ = s.listener.Close()
		logger.Info(s.context.GetRuntimeContext(), "http server stop", s.Address)
		s.wg.Wait()
	}
	return nil
}

func init() {
	ilogtail.ServiceInputs["service_http_server"] = func() ilogtail.ServiceInput {
		return &ServiceHTTP{
			ReadTimeoutSec:     10,
			ShutdownTimeoutSec: 5,
			MaxBodySize:        64 * 1024 * 1024,
			UnlinkUnixSock:     true,
			DumpDataKeepFiles:  5,
		}
	}
}
