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

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/decoder"
	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	v1 = iota
	v2
)

type dumpDataReq struct {
	Body   []byte
	URL    string
	Header map[string][]string
}
type dumpDataResp struct {
	Body   []byte
	Header map[string]string
}

type dumpData struct {
	Req  dumpDataReq
	Resp dumpDataResp
}

// ServiceHTTP ...
type ServiceHTTP struct {
	context           pipeline.Context
	collector         pipeline.Collector
	decoder           decoder.Decoder
	server            *http.Server
	listener          net.Listener
	wg                sync.WaitGroup
	collectorV2       pipeline.PipelineCollector
	version           int8
	paramCount        int
	dumpDataChan      chan *dumpData
	stopChan          chan struct{}
	dumpDataKeepFiles []string

	DumpDataKeepFiles  int
	DumpData           bool // would dump the received data to a local file, which is only used to valid data by the developers.
	Format             string
	Address            string
	Path               string
	ReadTimeoutSec     int
	ShutdownTimeoutSec int
	MaxBodySize        int64
	UnlinkUnixSock     bool
	FieldsExtend       bool
	DisableUncompress  bool
	Tags               map[string]string // todo for v2

	// params below works only for version v2
	QueryParams       []string
	HeaderParams      []string
	QueryParamPrefix  string
	HeaderParamPrefix string
}

// Init ...
func (s *ServiceHTTP) Init(context pipeline.Context) (int, error) {
	s.context = context
	var err error
	if s.decoder, err = decoder.GetDecoderWithOptions(s.Format, decoder.Option{FieldsExtend: s.FieldsExtend, DisableUncompress: s.DisableUncompress}); err != nil {
		return 0, err
	}

	switch s.Format {
	case common.ProtocolOTLPLogV1:
		s.Path = "/v1/logs"
	case common.ProtocolOTLPMetricV1:
		s.Path = "/v1/metrics"
	case common.ProtocolOTLPTraceV1:
		s.Path = "/v1/traces"
	}
	s.Address += s.Path
	logger.Infof(context.GetRuntimeContext(), "addr", s.Address, "format", s.Format)

	s.paramCount = len(s.QueryParams) + len(s.HeaderParams)

	if s.DumpData {
		_ = os.MkdirAll(path.Join(util.GetCurrentBinaryPath(), "dump"), 0750)
		s.dumpDataChan = make(chan *dumpData, 10)
		prefix := strings.Join([]string{s.context.GetProject(), s.context.GetLogstore(), s.context.GetConfigName()}, "_")
		files, err := helper.GetFileListByPrefix(path.Join(util.GetCurrentBinaryPath(), "dump"), prefix, true, 0)
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
func (s *ServiceHTTP) Collect(pipeline.Collector) error {
	return nil
}

func (s *ServiceHTTP) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.ContentLength > s.MaxBodySize {
		TooLarge(w)
		return
	}
	data, statusCode, err := s.decoder.ParseRequest(w, r, s.MaxBodySize)
	logger.Debugf(s.context.GetRuntimeContext(), "request [method] %v; [header] %v; [url] %v; [body len] %d", r.Method, r.Header, r.URL, len(data))
	switch statusCode {
	case http.StatusBadRequest:
		BadRequest(w)
	case http.StatusRequestEntityTooLarge:
		TooLarge(w)
	case http.StatusInternalServerError:
		InternalServerError(w)
	case http.StatusMethodNotAllowed:
		MethodNotAllowed(w)
	}
	if err != nil {
		logger.Warning(s.context.GetRuntimeContext(), "READ_BODY_FAIL_ALARM", "read body failed", err, "request", r.URL.String())
		return
	}

	if s.DumpData {
		s.dumpDataChan <- &dumpData{
			Req: dumpDataReq{
				Body:   data,
				URL:    r.URL.String(),
				Header: r.Header,
			},
		}
	}
	switch s.version {
	case v1:
		logs, err := s.decoder.Decode(data, r, s.Tags)
		if err != nil {
			logger.Warning(s.context.GetRuntimeContext(), "DECODE_BODY_FAIL_ALARM", "decode body failed", err, "request", r.URL.String())
			BadRequest(w)
			return
		}
		for _, log := range logs {
			s.collector.AddRawLog(log)
		}
	case v2:
		groups, err := s.decoder.DecodeV2(data, r)
		if err != nil {
			logger.Warning(s.context.GetRuntimeContext(), "DECODE_BODY_FAIL_ALARM", "decode body failed", err, "request", r.URL.String())
			BadRequest(w)
			return
		}
		if reqParams := s.extractRequestParams(r); len(reqParams) != 0 {
			for _, g := range groups {
				g.Group.Metadata.Merge(models.NewMetadataWithMap(reqParams))
			}
		}
		s.collectorV2.CollectList(groups...)
	}

	switch s.Format {
	case common.ProtocolSLS:
		w.Header().Set("x-log-requestid", "1234567890abcde")
		w.WriteHeader(http.StatusOK)
	case common.ProtocolPyroscope:
		// do nothing
	default:
		w.WriteHeader(http.StatusNoContent)
	}
}

func TooLarge(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusRequestEntityTooLarge)
	_, _ = res.Write([]byte(`{"error":"http: request body too large"}`))
}

func MethodNotAllowed(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusMethodNotAllowed)
	_, _ = res.Write([]byte(`{"error":"http: method not allowed"}`))
}

func InternalServerError(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusInternalServerError)
}

func BadRequest(res http.ResponseWriter) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusBadRequest)
	_, _ = res.Write([]byte(`{"error":"http: bad request"}`))
}

// Start starts the ServiceInput's service, whatever that may be
func (s *ServiceHTTP) Start(c pipeline.Collector) error {
	s.collector = c
	s.version = v1
	return s.start()
}

// StartService start the ServiceInput's service by plugin runner v2
func (s *ServiceHTTP) StartService(context pipeline.PipelineContext) error {
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
		nFile := path.Join(path.Join(util.GetCurrentBinaryPath(), "dump"), fileName+"_"+time.Now().Format("2006-01-02_15"))
		if len(s.dumpDataKeepFiles) == 0 || s.dumpDataKeepFiles[len(s.dumpDataKeepFiles)-1] != nFile {
			s.dumpDataKeepFiles = append(s.dumpDataKeepFiles, nFile)
		}
		if len(s.dumpDataKeepFiles) > s.DumpDataKeepFiles {
			_ = os.Remove(s.dumpDataKeepFiles[0])
			s.dumpDataKeepFiles = s.dumpDataKeepFiles[1:]
		}
		closeFile()
		return os.OpenFile(s.dumpDataKeepFiles[len(s.dumpDataKeepFiles)-1], os.O_CREATE|os.O_WRONLY, 0600)
	}
	lastHour := 0
	offset := int64(0)
	for {
		select {
		case d := <-s.dumpDataChan:
			if time.Now().Hour() != lastHour {
				file, cerr := cutFile()
				if cerr != nil {
					logger.Error(s.context.GetRuntimeContext(), "DUMP_FILE_ALARM", "cut new file error", err)
				} else {
					offset, _ = file.Seek(0, io.SeekEnd)
					f = file
				}
			}
			if f != nil {
				buffer := bytes.NewBuffer([]byte{})
				b, _ := json.Marshal(d)
				if err = binary.Write(buffer, binary.BigEndian, uint32(len(b))); err != nil {
					continue
				}
				if _, err = f.WriteAt(buffer.Bytes(), offset); err != nil {
					continue
				}
				if _, err = f.WriteAt(b, offset+4); err != nil {
					continue
				}
				offset += int64(4 + len(b))
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
	pipeline.ServiceInputs["service_http_server"] = func() pipeline.ServiceInput {
		return &ServiceHTTP{
			ReadTimeoutSec:     10,
			ShutdownTimeoutSec: 5,
			MaxBodySize:        64 * 1024 * 1024,
			UnlinkUnixSock:     true,
			DumpDataKeepFiles:  5,
			Tags:               map[string]string{},
		}
	}
}
