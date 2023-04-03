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

package rawstdout

import (
	"bufio"
	"context"
	"io"
	"regexp"
	"strings"
	"sync"
	"time"

	"github.com/docker/docker/api/types"
	docker "github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

func logDriverSupported(container types.ContainerJSON) bool {
	// containerd has no hostConfig, return true
	if container.HostConfig == nil {
		return true
	}
	switch container.HostConfig.LogConfig.Type {
	case "json-file", "journald":
		return true
	default:
		return false
	}
}

type StdoutCheckPoint struct {
	lock          sync.Mutex
	checkpointMap map[string]string
}

func (sc *StdoutCheckPoint) DeleteCheckPoint(id string) {
	sc.lock.Lock()
	defer sc.lock.Unlock()
	delete(sc.checkpointMap, id)
}

func (sc *StdoutCheckPoint) SaveCheckPoint(id, checkpoint string) {
	sc.lock.Lock()
	defer sc.lock.Unlock()
	sc.checkpointMap[id] = checkpoint
}

func (sc *StdoutCheckPoint) GetAllCheckPoint() map[string]string {
	rst := make(map[string]string)
	sc.lock.Lock()
	defer sc.lock.Unlock()
	for key, val := range sc.checkpointMap {
		rst[key] = val
	}
	return rst
}

func (sc *StdoutCheckPoint) GetCheckPoint(id string) string {
	sc.lock.Lock()
	defer sc.lock.Unlock()
	return sc.checkpointMap[id]
}

type stdoutSyner struct {
	ExternalEnvTag      map[string]string
	ExternalK8sLabelTag map[string]string

	info                 *helper.DockerInfoDetail
	client               *docker.Client
	startCheckPoint      string
	lock                 sync.Mutex
	stdoutCheckPoint     *StdoutCheckPoint
	context              pipeline.Context
	runtimeContext       context.Context
	cancelFun            context.CancelFunc
	wg                   sync.WaitGroup
	beginLineReg         *regexp.Regexp
	beginLineTimeout     time.Duration
	beginLineCheckLength int
	maxLogSize           int
	stdout               bool
	stderr               bool
}

func (ss *stdoutSyner) newContainerPump(c pipeline.Collector, stdout, stderr *io.PipeReader) {
	pump := func(source string, tags map[string]string, input io.Reader) {
		ss.wg.Add(1)
		defer ss.wg.Done()
		keys := []string{"_time_", "_source_", "content"}
		values := []string{"", source, ""}

		if ss.beginLineReg == nil {
			buf := bufio.NewReader(input)
			for {
				line, err := buf.ReadString('\n')
				if err != nil {
					if err != io.EOF && err != io.ErrClosedPipe {
						logger.Warning(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "stdoutSyner done, id", ss.info.IDPrefix(),
							"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status(), "source", source, "error", err)
					}
					logger.Debug(ss.context.GetRuntimeContext(), "docker source stop", source, "id", ss.info.IDPrefix(),
						"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status(), "error", err)
					return
				}
				if index := strings.IndexByte(line, ' '); index > 1 && index < len(line)-1 {
					values[0] = line[0:index]
					values[2] = line[index+1 : len(line)-1]
					ss.lock.Lock()
					ss.startCheckPoint = values[0] //TODO: this is not correct, stdout and stderr should have separate checkpoint
					ss.lock.Unlock()
				} else {
					values[0] = ""
					values[2] = line
				}
				c.AddDataArray(tags, keys, values)
			}
		} else {
			buf := bufio.NewReader(util.NewTimeoutReader(input, ss.beginLineTimeout))
			for {
				line, err := buf.ReadString('\n')
				if err == util.ErrReaderTimeout {
					// process timeout
					if len(values[2]) > 0 {
						logger.Debug(ss.context.GetRuntimeContext(), "log time out, treat as full log", util.CutString(values[2], 512))
						c.AddDataArray(tags, keys, values)
						values[2] = line
					} else {
						values[2] += line
					}
					continue
				}
				if err != nil {
					if err != io.EOF && err != io.ErrClosedPipe {
						logger.Warning(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "stdoutSyner done, id", ss.info.IDPrefix(),
							"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status(), "source", source, "error", err)
					}
					logger.Debug(ss.context.GetRuntimeContext(), "docker source stop", source, "name", ss.info.ContainerInfo.Name, "error", err)
					if len(values[2]) > 0 {
						logger.Info(ss.context.GetRuntimeContext(), "flush out last line", util.CutString(values[2], 512))
						c.AddDataArray(tags, keys, values)
					}
					return
				}
				// full line, check regex match result
				var logLine string
				if index := strings.IndexByte(line, ' '); index > 1 && index < len(line)-1 {
					values[0] = line[0:index]
					logLine = line[index+1 : len(line)-1]
					ss.lock.Lock()
					ss.startCheckPoint = values[0] //TODO: checkpoint before regex check, too early
					ss.lock.Unlock()
				} else {
					values[0] = ""
					logLine = line
				}
				var checkLine string
				if len(logLine) > ss.beginLineCheckLength {
					checkLine = logLine[0:ss.beginLineCheckLength]
				} else {
					checkLine = logLine
				}
				if matchRst := ss.beginLineReg.FindStringIndex(checkLine); len(matchRst) >= 2 && matchRst[0] == 0 && matchRst[1] == len(checkLine) {
					// match begin line
					if len(values[2]) > 0 {
						// new line
						c.AddDataArray(tags, keys, values)
					}
					values[2] = logLine
				} else {
					// not math begin line, wait for next line
					values[2] += "\n"
					values[2] += logLine
					// check very big line
					if len(values[2]) >= ss.maxLogSize {
						logger.Warning(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "log line is too long, force flush out", len(values[2]), "log prefix", util.CutString(values[2], 4096))
						c.AddDataArray(tags, keys, values)
						values[2] = ""
					}
				}
			}
		}
	}
	tags := ss.info.GetExternalTags(ss.ExternalEnvTag, ss.ExternalK8sLabelTag)
	if stdout != nil {
		go pump("stdout", tags, stdout)
	}
	if stderr != nil {
		go pump("stderr", tags, stderr)
	}
}

func (ss *stdoutSyner) Start(c pipeline.Collector) {
	if ss.beginLineCheckLength <= 0 {
		ss.beginLineCheckLength = 10 * 1024
	}
	if ss.maxLogSize <= 0 {
		ss.maxLogSize = 512 * 1024
	}
	ss.wg.Add(1)
	defer ss.wg.Done()
	for {
		var cpTime time.Time
		ss.lock.Lock()
		if len(ss.startCheckPoint) > 0 {
			var err error
			if cpTime, err = time.Parse(helper.DockerTimeFormat, ss.startCheckPoint); err != nil {
				logger.Warning(ss.context.GetRuntimeContext(), "CHECKPOINT_ALARM", "docker stdout raw parse start time error", ss.startCheckPoint,
					"id", ss.info.IDPrefix(),
					"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
			} else {
				logger.Info(ss.context.GetRuntimeContext(), "docker stdout raw recover since", ss.startCheckPoint,
					"id", ss.info.IDPrefix(),
					"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
			}
		}
		if cpTime.IsZero() {
			// if first start, skip 10 second
			cpTime = time.Now().Add(time.Second * time.Duration(-10))
			logger.Info(ss.context.GetRuntimeContext(), "docker stdout raw first read since", cpTime.Format(helper.DockerTimeFormat),
				"id", ss.info.IDPrefix(),
				"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
		}
		ss.lock.Unlock()
		options := types.ContainerLogsOptions{
			ShowStdout: ss.stdout,
			ShowStderr: ss.stderr,
			Since:      cpTime.Format(helper.DockerTimeFormat),
			Timestamps: true,
			Follow:     true,
		}
		var outrd, errrd *io.PipeReader
		var outwr, errwr *io.PipeWriter
		outrd, outwr = io.Pipe()
		if ss.info.ContainerInfo.Config.Tty {
			options.ShowStderr = false
		}
		if options.ShowStdout {
			outrd, outwr = io.Pipe()
		}
		if options.ShowStderr {
			errrd, errwr = io.Pipe()
		}

		logger.Info(ss.context.GetRuntimeContext(), "docker stdout raw", "begin", "id", ss.info.IDPrefix(),
			"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
		// start pump logs go routines
		ss.newContainerPump(c, outrd, errrd)
		// loop to copy logs to parser
		logReader, err := ss.client.ContainerLogs(ss.runtimeContext, ss.info.ContainerInfo.ID, options)
		if err != nil {
			logger.Errorf(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "open container log error=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
				err.Error(), ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
			break
		}
		var written int64
		if ss.info.ContainerInfo.Config.Tty {
			written, err = io.Copy(outwr, logReader)
			logger.Debugf(ss.context.GetRuntimeContext(), "read container log bytes=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
				written, ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
			if err != nil && err != context.Canceled {
				logger.Errorf(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "read container log error=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
					err.Error(), ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
			}
		} else {
			written, err = stdcopy.StdCopy(outwr, errwr, logReader)
			logger.Debugf(ss.context.GetRuntimeContext(), "read container log bytes=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
				written, ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
			if err != nil && err != context.Canceled {
				logger.Errorf(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "read container log error=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
					err.Error(), ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
			}
		}
		// loop broken if container exits
		if closeErr := logReader.Close(); closeErr != nil {
			logger.Warningf(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "close container log error=%v, id:%v\tname:%v\tcreated:%v\tstatus:%v",
				closeErr, ss.info.IDPrefix(), ss.info.ContainerInfo.Name, ss.info.ContainerInfo.Created, ss.info.Status())
		}
		_ = outrd.CloseWithError(io.EOF)
		_ = errrd.CloseWithError(io.EOF)
		ss.lock.Lock()
		if len(ss.startCheckPoint) > 0 {
			ss.stdoutCheckPoint.SaveCheckPoint(ss.info.ContainerInfo.ID, ss.startCheckPoint)
		}
		ss.lock.Unlock()

		switch err {
		case nil, context.Canceled, context.DeadlineExceeded:
			logger.Info(ss.context.GetRuntimeContext(), "docker stdout raw", "stop", "id", ss.info.IDPrefix(),
				"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
			return
		default:
			// after sleep, we need recheck if runtime context is done
			logger.Warning(ss.context.GetRuntimeContext(), "DOCKER_STDOUT_STOP_ALARM", "stdoutSyner stop, retry after 10 seconds, id", ss.info.IDPrefix(),
				"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status(), "error", err)
			if util.RandomSleep(time.Second*time.Duration(10), 0.1, ss.runtimeContext.Done()) {
				logger.Info(ss.context.GetRuntimeContext(), "docker stdout raw", "stop", "id", ss.info.IDPrefix(),
					"name", ss.info.ContainerInfo.Name, "created", ss.info.ContainerInfo.Created, "status", ss.info.Status())
				return
			}
		}
	}
}

func (ss *stdoutSyner) Stop() {
	ss.cancelFun()
	ss.wg.Wait()
}

func (ss *stdoutSyner) Update() {
}

type ServiceDockerStdout struct {
	IncludeLabel          map[string]string // Deprecated： use IncludeContainerLabel and IncludeK8sLabel instead.
	ExcludeLabel          map[string]string // Deprecated： use ExcludeContainerLabel and ExcludeK8sLabel instead.
	IncludeEnv            map[string]string
	ExcludeEnv            map[string]string
	IncludeContainerLabel map[string]string
	ExcludeContainerLabel map[string]string
	IncludeK8sLabel       map[string]string
	ExcludeK8sLabel       map[string]string
	ExternalEnvTag        map[string]string
	ExternalK8sLabelTag   map[string]string
	FlushIntervalMs       int
	TimeoutMs             int
	BeginLineRegex        string
	BeginLineTimeoutMs    int
	BeginLineCheckLength  int
	MaxLogSize            int
	Stdout                bool
	Stderr                bool
	K8sNamespaceRegex     string
	K8sPodRegex           string
	K8sContainerRegex     string

	// export from ilogtail-trace component
	IncludeLabelRegex map[string]*regexp.Regexp
	ExcludeLabelRegex map[string]*regexp.Regexp
	IncludeEnvRegex   map[string]*regexp.Regexp
	ExcludeEnvRegex   map[string]*regexp.Regexp
	K8sFilter         *helper.K8SFilter

	synerMap         map[string]*stdoutSyner
	client           *docker.Client
	shutdown         chan struct{}
	waitGroup        sync.WaitGroup
	context          pipeline.Context
	runtimeContext   context.Context
	stdoutCheckPoint *StdoutCheckPoint
}

func (sds *ServiceDockerStdout) Init(context pipeline.Context) (int, error) {
	sds.context = context
	helper.ContainerCenterInit()
	sds.synerMap = make(map[string]*stdoutSyner)

	var err error
	sds.IncludeEnv, sds.IncludeEnvRegex, err = helper.SplitRegexFromMap(sds.IncludeEnv)
	if err != nil {
		logger.Warning(sds.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include env regex error", err)
	}
	sds.ExcludeEnv, sds.ExcludeEnvRegex, err = helper.SplitRegexFromMap(sds.ExcludeEnv)
	if err != nil {
		logger.Warning(sds.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude env regex error", err)
	}
	if sds.IncludeLabel != nil {
		for k, v := range sds.IncludeContainerLabel {
			sds.IncludeLabel[k] = v
		}
	} else {
		sds.IncludeLabel = sds.IncludeContainerLabel
	}
	if sds.ExcludeLabel != nil {
		for k, v := range sds.ExcludeContainerLabel {
			sds.ExcludeLabel[k] = v
		}
	} else {
		sds.ExcludeLabel = sds.ExcludeContainerLabel
	}
	sds.IncludeLabel, sds.IncludeLabelRegex, err = helper.SplitRegexFromMap(sds.IncludeLabel)
	if err != nil {
		logger.Warning(sds.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include label regex error", err)
	}
	sds.ExcludeLabel, sds.ExcludeLabelRegex, err = helper.SplitRegexFromMap(sds.ExcludeLabel)
	if err != nil {
		logger.Warning(sds.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude label regex error", err)
	}
	sds.K8sFilter, err = helper.CreateK8SFilter(sds.K8sNamespaceRegex, sds.K8sPodRegex, sds.K8sContainerRegex, sds.IncludeK8sLabel, sds.ExcludeK8sLabel)

	return 0, err
}

func (sds *ServiceDockerStdout) Description() string {
	return "docker stdout raw input plugin for logtail"
}

// Collect takes in an accumulator and adds the metrics that the Input
// gathers. This is called every "interval"
func (sds *ServiceDockerStdout) Collect(pipeline.Collector) error {
	return nil
}

func (sds *ServiceDockerStdout) FlushAll(c pipeline.Collector, firstStart bool) error {
	var err error
	dockerInfos := helper.GetContainerByAcceptedInfo(
		sds.IncludeLabel, sds.ExcludeLabel,
		sds.IncludeLabelRegex, sds.ExcludeLabelRegex,
		sds.IncludeEnv, sds.ExcludeEnv,
		sds.IncludeEnvRegex, sds.ExcludeEnvRegex,
		sds.K8sFilter)
	logger.Debug(sds.context.GetRuntimeContext(), "flush all", len(dockerInfos))
	for id, info := range dockerInfos {
		if !logDriverSupported(info.ContainerInfo) {
			continue
		}
		if syner, ok := sds.synerMap[id]; !ok || firstStart {
			runtimeContext, cancelFun := context.WithCancel(sds.runtimeContext)
			var reg *regexp.Regexp
			if len(sds.BeginLineRegex) > 0 {
				if reg, err = regexp.Compile(sds.BeginLineRegex); err != nil {
					logger.Warning(sds.context.GetRuntimeContext(), "REGEX_COMPILE_ALARM", "compile begin line regex error, regex", sds.BeginLineRegex, "error", err)
				}
			}
			syner = &stdoutSyner{
				info:                 info,
				client:               sds.client,
				stdoutCheckPoint:     sds.stdoutCheckPoint,
				startCheckPoint:      sds.stdoutCheckPoint.GetCheckPoint(id),
				context:              sds.context,
				runtimeContext:       runtimeContext,
				cancelFun:            cancelFun,
				beginLineReg:         reg,
				beginLineCheckLength: sds.BeginLineCheckLength,
				beginLineTimeout:     time.Duration(sds.BeginLineTimeoutMs) * time.Millisecond,
				maxLogSize:           sds.MaxLogSize,
				stdout:               sds.Stdout,
				stderr:               sds.Stderr,
				ExternalEnvTag:       sds.ExternalEnvTag,
				ExternalK8sLabelTag:  sds.ExternalK8sLabelTag,
			}
			sds.synerMap[id] = syner
			go syner.Start(c)
		} else {
			syner.lock.Lock()
			if len(syner.startCheckPoint) > 0 {
				sds.stdoutCheckPoint.SaveCheckPoint(syner.info.ContainerInfo.ID, syner.startCheckPoint)
			}
			syner.lock.Unlock()
		}
	}

	// delete container
	for id, syner := range sds.synerMap {
		if _, ok := dockerInfos[id]; !ok {
			logger.Info(sds.context.GetRuntimeContext(), "delete docker stdout raw, id", id, "name", syner.info.ContainerInfo.Name)
			syner.Stop()
			delete(sds.synerMap, id)
			sds.stdoutCheckPoint.DeleteCheckPoint(id)
		}
	}

	return err
}

func (sds *ServiceDockerStdout) SaveCheckPoint() error {
	logger.Debug(sds.context.GetRuntimeContext(), "stdout raw save checkpoint", sds.stdoutCheckPoint.checkpointMap)
	return sds.context.SaveCheckPointObject("service_docker_stdout", sds.stdoutCheckPoint.checkpointMap)
}

func (sds *ServiceDockerStdout) GetCheckPoint() *StdoutCheckPoint {
	if sds.stdoutCheckPoint != nil {
		return sds.stdoutCheckPoint
	}
	stdoutCheckPoint := &StdoutCheckPoint{}
	sds.context.GetCheckPointObject("service_docker_stdout", &stdoutCheckPoint.checkpointMap)
	if stdoutCheckPoint.checkpointMap == nil {
		logger.Debug(sds.context.GetRuntimeContext(), "stdout raw get checkpoint", "failed")
		stdoutCheckPoint.checkpointMap = make(map[string]string)
	} else {
		logger.Debug(sds.context.GetRuntimeContext(), "stdout raw get checkpoint", stdoutCheckPoint.checkpointMap)
	}
	return stdoutCheckPoint
}

// Start starts the ServiceInput's service, whatever that may be
func (sds *ServiceDockerStdout) Start(c pipeline.Collector) error {
	sds.shutdown = make(chan struct{})
	sds.waitGroup.Add(1)
	defer sds.waitGroup.Done()

	sds.stdoutCheckPoint = sds.GetCheckPoint()

	var err error
	if sds.client, err = helper.CreateDockerClient(); err != nil {
		logger.Error(sds.context.GetRuntimeContext(), "DOCKER_CLIENT_ALARM", "create docker client error", err)
		return err
	}
	var cancelFun context.CancelFunc
	sds.runtimeContext, cancelFun = context.WithCancel(context.Background())
	_ = sds.FlushAll(c, true)
	for {
		timer := time.NewTimer(time.Duration(sds.FlushIntervalMs) * time.Millisecond)
		select {
		case <-sds.shutdown:
			logger.Info(sds.context.GetRuntimeContext(), "docker stdout raw main runtime stop", "begin")
			for _, syner := range sds.synerMap {
				syner.Stop()
			}
			cancelFun()
			logger.Info(sds.context.GetRuntimeContext(), "docker stdout raw main runtime stop", "success")

			return nil
		case <-timer.C:
			_ = sds.SaveCheckPoint()
			_ = sds.FlushAll(c, false)
		}
	}
}

// Stop stops the services and closes any necessary channels and connections
func (sds *ServiceDockerStdout) Stop() error {
	close(sds.shutdown)
	sds.waitGroup.Wait()
	_ = sds.SaveCheckPoint()
	return nil
}

func init() {
	pipeline.ServiceInputs["service_docker_stdout_raw"] = func() pipeline.ServiceInput {
		return &ServiceDockerStdout{
			FlushIntervalMs:      3000,
			TimeoutMs:            3000,
			Stdout:               true,
			Stderr:               true,
			BeginLineTimeoutMs:   3000,
			BeginLineCheckLength: 10 * 1024,
		}
	}
}
