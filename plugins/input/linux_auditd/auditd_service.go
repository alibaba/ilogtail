// Copyright 2023 iLogtail Authors
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

//go:build !windows
// +build !windows

package linux_auditd

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/elastic/go-libaudit/v2"
	"github.com/elastic/go-libaudit/v2/aucoalesce"
	"github.com/elastic/go-libaudit/v2/auparse"
)

const (
	v1 = iota
	v2
)
const (
	namespace = "auditd"

	unicast   = "unicast"
	multicast = "multicast"
	uidUnset  = "unset"

	lostEventsUpdateInterval = time.Second * 15

	auditLocked = 2

	maxDefaultStreamBufferConsumers = 4

	setPIDMaxRetries = 5
)

type backpressureStrategy uint8

const (
	bsKernel backpressureStrategy = 1 << iota
	bsUserSpace
	bsAuto
)

// ServiceLinuxAuditd struct implement the ServiceInput interface.
type ServiceLinuxAuditd struct {
	ResolveIDs   bool   // Resolve UID/GIDs to names.
	FailureMode  string // Failure mode for the kernel (silent, log, panic).
	BacklogLimit uint32 // Max number of message to buffer in the auditd.
	RateLimit    uint32 // Rate limit in messages/sec of messages from auditd.
	KeepSource   bool   // Include the list of raw audit messages in the event.
	RulesBlob    string // Audit rules. One rule per line.
	SocketType   string // Socket type to use with the kernel (unicast or multicast).
	Immutable    bool   // Sets kernel audit config immutable.
	IgnoreErrors bool   // Ignore errors when reading and parsing rules, equivalent to auditctl -i.

	BackpressureStrategy  string // The strategy used to mitigate backpressure. One of "user-space", "kernel", "both", "none", "auto" (default)
	StreamBufferConsumers int
	StreamBufferQueueSize int
	ReassemblerTimeout    time.Duration

	auditRules           []auditRule
	client               *libaudit.AuditClient
	supportMulticast     bool
	backpressureStrategy backpressureStrategy

	context   pipeline.Context
	collector pipeline.Collector
	done      chan struct{}
	version   int8
}

func (s *ServiceLinuxAuditd) Init(context pipeline.Context) (int, error) {
	var err error

	s.context = context
	s.supportMulticast = s.isSupportMulticast()
	s.backpressureStrategy = getBackpressureStrategy(s.BackpressureStrategy)

	s.SocketType, err = getSocketType(s)
	if err != nil {
		return 0, err
	}
	s.SocketType = strings.ToLower(s.SocketType)

	err = s.ValidateConfig()
	if err != nil {
		logger.Errorf(s.context.GetRuntimeContext(), "failed to init ServiceLinuxAuditd:", "error", err)
		return 0, err
	}
	logger.Info(s.context.GetRuntimeContext(), "socket_type=%s will be used.", s.SocketType)

	return 0, nil
}

func (s *ServiceLinuxAuditd) Description() string {
	return "This is a service for collect audit events from Linux Audit."
}

// Start the service example plugin would run in a separate go routine, so it is blocking method.
func (s *ServiceLinuxAuditd) Start(collector pipeline.Collector) error {
	logger.Info(s.context.GetRuntimeContext(), "start the ServiceAuditd plugin")

	s.done = make(chan struct{}, 1)
	s.collector = collector
	s.version = v1

	var err error
	s.client, err = newAuditClient(s)
	if err != nil {
		return fmt.Errorf("failed to create audit client: %w", err)
	}

	status, err := s.client.GetStatus()
	if err != nil {
		return fmt.Errorf("failed to get audit status before adding rules: %w", err)
	}

	if status.Enabled == auditLocked {
		logger.Error(s.context.GetRuntimeContext(), "Skipping rule configuration: Audit rules are locked")
	} else if err := s.addRules(); err != nil {
		logger.Errorf(s.context.GetRuntimeContext(), "Failure adding audit rules", "error", err)
		return err
	}

	err = s.initAuditClient()
	if err != nil {
		return fmt.Errorf("failed to init audit client: %w", err)
	}

	if s.Immutable && status.Enabled != auditLocked {
		if err := s.client.SetImmutable(libaudit.WaitForReply); err != nil {
			logger.Errorf(s.context.GetRuntimeContext(), "Failure setting audit config as immutable", "error", err)
			return fmt.Errorf("failed to set audit as immutable: %w", err)
		}
	}

	out, err := s.receiveEvents(s.done)
	if err != nil {
		return err
	}

	go func() {
		for {
			select {
			case <-s.done:
				return
			case msgs := <-out:
				fmt.Print(msgs)
				//converterAuditEventToSLSLog
			}
		}
	}()

	return nil
}

func (s *ServiceLinuxAuditd) Stop() error {
	logger.Info(s.context.GetRuntimeContext(), "close the ServiceAuditd plugin")

	close(s.done)
	err := closeAuditClient(s.client)

	return err
}

func (s *ServiceLinuxAuditd) receiveEvents(done <-chan struct{}) (<-chan []*auparse.AuditMessage, error) {
	out := make(chan []*auparse.AuditMessage, s.StreamBufferQueueSize)

	var st libaudit.Stream = &stream{done, out}
	if s.backpressureStrategy&bsUserSpace != 0 {
		// "user-space" backpressure mitigation strategy
		//
		// Consume events from our side as fast as possible, by dropping events
		// if the publishing pipeline would block.
		logger.Info(s.context.GetRuntimeContext(),
			"Using non-blocking stream to prevent backpressure propagating to the kernel.")
		st = &nonBlockingStream{done, out}
	}
	reassembler, err := libaudit.NewReassembler(int(50), 2*time.Second, st)
	if err != nil {
		return nil, fmt.Errorf("failed to create Reassembler: %w", err)
	}
	go maintain(done, reassembler)

	go func() {
		//defer ms.log.Debug("receiveEvents goroutine exited")
		defer close(out)
		defer reassembler.Close()

		for {
			raw, err := s.client.Receive(false)
			if err != nil {
				if errors.Is(err, syscall.EBADF) {
					// Client has been closed.
					break
				}
				continue
			}

			if filterRecordType(raw.Type) {
				continue
			}
			if err := reassembler.Push(raw.Type, raw.Data); err != nil {
				// ms.log.Debugw("Dropping audit message",
				// 	"record_type", raw.Type,
				// 	"message", string(raw.Data),
				// 	"error", err)
				continue
			}
		}
	}()

	return out, nil
}

func converterAuditEventToSLSLog(msgs []*auparse.AuditMessage, resolveIDs bool) (*protocol.Log, error) {
	auditEvent, err := aucoalesce.CoalesceMessages(msgs)
	if err != nil {
		return nil, err
	}

	if resolveIDs {
		aucoalesce.ResolveIDs(auditEvent)
	}

	contents := []*protocol.Log_Content{
		{
			Key:   "category",
			Value: auditEvent.Category.String(),
		},
		{
			Key:   "action",
			Value: auditEvent.Summary.Action,
		},
		{
			Key:   "message_type",
			Value: strings.ToLower(auditEvent.Type.String()),
		},
		{
			Key:   "sequence",
			Value: strconv.FormatUint(uint64(auditEvent.Sequence), 10),
		},
		{
			Key:   "result",
			Value: auditEvent.Result,
		},
		{
			Key:   "data",
			Value: createAuditdData(auditEvent.Data),
		},
		// {
		// 	Key:   "user",
		// 	Value: addUser(auditEvent.User),
		// },
		// {
		// 	Key:   "process",
		// 	Value: addProcess(auditEvent.Process),
		// },
		// {
		// 	Key:   "file",
		// 	Value: addFile(auditEvent.File),
		// },
		// {
		// 	Key:   "source",
		// 	Value: addAddress(auditEvent.Source),
		// },
		// {
		// 	Key:   "destination",
		// 	Value: addAddress(auditEvent.Dest),
		// },
		// {
		// 	Key:   "network",
		// 	Value: addNetwork(auditEvent.Net),
		// },
		// {
		// 	Key:   "tags",
		// 	Value: util.InterfaceToJSONStringIgnoreErr(auditEvent.Tags),
		// },
		// {
		// 	Key:   "summary",
		// 	Value: addSummary(auditEvent.Summary),
		// },
		// {
		// 	Key:   "paths",
		// 	Value: util.InterfaceToJSONStringIgnoreErr(auditEvent.Paths),
		// },
		// {
		// 	Key:   "event",
		// 	Value: normalizeEventFields(auditEvent),
		// },
	}

	r := &protocol.Log{
		Time:     uint32(auditEvent.Timestamp.Unix()),
		Contents: contents,
	}
	return r, err
}

// Register the plugin to the ServiceInputs array.
func init() {
	pipeline.ServiceInputs["service_linux_auditd"] = func() pipeline.ServiceInput {
		return &ServiceLinuxAuditd{
			ResolveIDs:   true,
			FailureMode:  "silent",
			BacklogLimit: 8192,
			RateLimit:    0,
			KeepSource:   false,
			Immutable:    false,
			SocketType:   "unicast",

			BackpressureStrategy:  "auto",
			StreamBufferConsumers: 0,
			ReassemblerTimeout:    2 * time.Second,
			StreamBufferQueueSize: 8192,
		}
	}
}
