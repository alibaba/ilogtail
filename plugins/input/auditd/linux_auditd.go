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

package auditd

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
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

type auditRule struct {
	flags string
	data  []byte
}

type ruleWithSource struct {
	rule   auditRule
	source string
}

type ruleSet map[string]ruleWithSource

// ServiceLinuxAuditd struct implement the ServiceInput interface.
type ServiceLinuxAuditd struct {
	ResolveIDs   bool     // Resolve UID/GIDs to names.
	FailureMode  string   // Failure mode for the kernel (silent, log, panic).
	BacklogLimit uint32   // Max number of message to buffer in the auditd.
	RateLimit    uint32   // Rate limit in messages/sec of messages from auditd.
	KeepSource   bool     // Include the list of raw audit messages in the event.
	KeepWarnings bool     // Include warnings in the event (for dev/debug purposes only).
	RulesBlob    string   // Audit rules. One rule per line.
	RuleFiles    []string // List of rule files.
	SocketType   string   // Socket type to use with the kernel (unicast or multicast).
	Immutable    bool     // Sets kernel audit config immutable.

	BackpressureStrategy  string // The strategy used to mitigate backpressure. One of "user-space", "kernel", "both", "none", "auto" (default)
	StreamBufferConsumers int

	auditRules           []auditRule
	client               *libaudit.AuditClient
	supportMulticast     bool
	backpressureStrategy backpressureStrategy

	context   pipeline.Context
	collector pipeline.Collector
	version   int8
}

// stream type

// stream receives callbacks from the libaudit.Reassembler for completed events
// or lost events that are detected by gaps in sequence numbers.
type stream struct {
	collector  pipeline.Collector
	resolveIDs bool
}

func (s *stream) ReassemblyComplete(msgs []*auparse.AuditMessage) {
	l, _ := converterAuditEventToSLSLog(msgs, s.resolveIDs)

	s.collector.AddRawLog(l)
}

func (s *stream) EventsLost(count int) {

}

// nonBlockingStream behaves as stream above, except that it will never block
// on backpressure from the publishing pipeline.
// Instead, events will be discarded.
type nonBlockingStream stream

func (s *nonBlockingStream) ReassemblyComplete(msgs []*auparse.AuditMessage) {
	event, _ := aucoalesce.CoalesceMessages(msgs)
	jsonBytes, _ := json.Marshal(event)
	fmt.Print(string(jsonBytes) + "\n")
}

func (s *nonBlockingStream) EventsLost(count int) {
	(*stream)(s).EventsLost(count)
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

	err = s.validateConfig()
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

	s.collector = collector
	s.version = v1

	var err error
	s.client, err = newAuditClient(s.SocketType)
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

	err = s.receiveEvents()
	if err != nil {
		return err
	}

	// go func() {
	// 	defer func() { // Close the most recently allocated "client" instance.
	// 		if s.client != nil {
	// 			closeAuditClient(s.client)
	// 		}
	// 	}()
	// 	timer := time.NewTicker(lostEventsUpdateInterval)
	// 	defer timer.Stop()
	// 	for {
	// 		select {
	// 		case <-timer.C:
	// 			if status, err := s.client.GetStatus(); err == nil {
	// 				//ms.updateKernelLostMetric(status.Lost)
	// 			} else {
	// 				//ms.log.Error("get status request failed:", err)
	// 				closeAuditClient(s.client, ms.log)
	// 				client, err = libaudit.NewAuditClient(nil)
	// 				if err != nil {
	// 					ms.log.Errorw("Failure creating audit monitoring client", "error", err)
	// 					reporter.Error(err)
	// 					return
	// 				}
	// 			}
	// 		}
	// 	}
	// }()

	// go func() {
	// 	for {
	// 		select {
	// 		case msgs := <-out:
	// 			fmt.Print(msgs)
	// 		}
	// 	}
	// }()

	return nil
}

func (s *ServiceLinuxAuditd) Stop() error {
	logger.Info(s.context.GetRuntimeContext(), "close the ServiceAuditd plugin")

	err := closeAuditClient(s.client)

	return err
}

func (s *ServiceLinuxAuditd) validateConfig() error {
	var err error
	s.auditRules, err = loadRules(s.RulesBlob, s.RuleFiles)
	if err != nil {
		return err
	}
	_, err = failureMode(s.FailureMode)
	if err != nil {
		return err
	}

	switch s.SocketType {
	case "multicast":
		if s.Immutable {
			return fmt.Errorf("immutable can't be used with socket_type: multicast")
		}
	case "", "unicast":
	default:
		return fmt.Errorf("invalid socket_type "+
			"'%v' (use unicast, multicast, or don't set a value)", s.SocketType)
	}

	return nil
}

func (s *ServiceLinuxAuditd) receiveEvents() error {
	var st libaudit.Stream = &stream{s.collector, s.ResolveIDs}
	if s.backpressureStrategy&bsUserSpace != 0 {
		// "user-space" backpressure mitigation strategy
		//
		// Consume events from our side as fast as possible, by dropping events
		// if the publishing pipeline would block.
		logger.Info(s.context.GetRuntimeContext(),
			"Using non-blocking stream to prevent backpressure propagating to the kernel.")
		st = &nonBlockingStream{s.collector, s.ResolveIDs}
	}
	reassembler, err := libaudit.NewReassembler(int(50), 2*time.Second, st)
	if err != nil {
		return fmt.Errorf("failed to create Reassembler: %w", err)
	}
	//go maintain(done, reassembler)

	go func() {
		//defer ms.log.Debug("receiveEvents goroutine exited")
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

	return nil
}

// multicast can only be used in kernel version >= 3.16.
func (s *ServiceLinuxAuditd) isSupportMulticast() bool {
	major, minor, kernel, err := kernelVersion()
	if err != nil {
		logger.Infof(s.context.GetRuntimeContext(), "auditd module is running as euid=%v on kernel=%v", os.Geteuid(), kernel)

		if major > 3 || major == 3 && minor >= 16 {
			return true
		} else {
			return false
		}
	}
	return false
}

func (s *ServiceLinuxAuditd) initAuditClient() error {
	if s.SocketType == "multicast" {
		// This request will fail with EPERM if this process does not have
		// CAP_AUDIT_CONTROL, but we will ignore the response. The user will be
		// required to ensure that auditing is enabled if the process is only
		// given CAP_AUDIT_READ.
		err := s.client.SetEnabled(true, libaudit.NoWait)
		if err != nil {
			return fmt.Errorf("failed to enable auditing in the kernel: %w", err)
		}
		return nil
	}

	// Unicast client initialization (requires CAP_AUDIT_CONTROL and that the
	// process be in initial PID namespace).
	status, err := s.client.GetStatus()
	if err != nil {
		return fmt.Errorf("failed to get audit status: %w", err)
	}

	logger.Infof(s.context.GetRuntimeContext(), "audit status from kernel at start", "audit_status", status)

	if status.Enabled == auditLocked {
		if !s.Immutable {
			return errors.New("failed to configure: The audit system is locked")
		}
	}

	if status.Enabled != auditLocked {
		if fm, _ := failureMode(s.FailureMode); status.Failure != fm {
			if err = s.client.SetFailure(libaudit.FailureMode(fm), libaudit.NoWait); err != nil {
				return fmt.Errorf("failed to set audit failure mode in kernel: %w", err)
			}
		}

		if status.BacklogLimit != s.BacklogLimit {
			if err = s.client.SetBacklogLimit(s.BacklogLimit, libaudit.NoWait); err != nil {
				return fmt.Errorf("failed to set audit backlog limit in kernel: %w", err)
			}
		}

		if s.backpressureStrategy&(bsKernel|bsAuto) != 0 {
			// "kernel" backpressure mitigation strategy
			//
			// configure the kernel to drop audit events immediately if the
			// backlog queue is full.
			if status.FeatureBitmap&libaudit.AuditFeatureBitmapBacklogWaitTime != 0 {
				logger.Infof(s.context.GetRuntimeContext(),
					"Setting kernel backlog wait time to prevent backpressure propagating to the kernel.")
				if err = s.client.SetBacklogWaitTime(0, libaudit.NoWait); err != nil {
					return fmt.Errorf("failed to set audit backlog wait time in kernel: %w", err)
				}
			} else {
				if s.backpressureStrategy == bsAuto {
					logger.Warning(s.context.GetRuntimeContext(),
						"setting backlog wait time is not supported in this kernel. Enabling workaround.")
					s.backpressureStrategy |= bsUserSpace
				} else {
					return errors.New("kernel backlog wait time not supported by kernel, but required by backpressure_strategy")
				}
			}
		}

		if s.backpressureStrategy&(bsKernel|bsUserSpace) == bsUserSpace && s.RateLimit == 0 {
			// force a rate limit if the user-space strategy will be used without
			// corresponding backlog_wait_time setting in the kernel
			s.RateLimit = 5000
		}

		if status.RateLimit != s.RateLimit {
			if err = s.client.SetRateLimit(s.RateLimit, libaudit.NoWait); err != nil {
				return fmt.Errorf("failed to set audit rate limit in kernel: %w", err)
			}
		}

		if status.Enabled == 0 {
			if err = s.client.SetEnabled(true, libaudit.NoWait); err != nil {
				return fmt.Errorf("failed to enable auditing in the kernel: %w", err)
			}
		}
	}

	if err := s.client.WaitForPendingACKs(); err != nil {
		return fmt.Errorf("failed to wait for ACKs: %w", err)
	}

	if err := s.setRecvPID(setPIDMaxRetries); err != nil {
		var errno syscall.Errno
		if ok := errors.As(err, &errno); ok && errno == syscall.EEXIST && status.PID != 0 {
			return fmt.Errorf("failed to set audit PID. An audit process is already running (PID %d)", status.PID)
		}
		return fmt.Errorf("failed to set audit PID (current audit PID %d): %w", status.PID, err)
	}
	return nil
}

func (s *ServiceLinuxAuditd) setRecvPID(retries int) (err error) {
	if err = s.client.SetPID(libaudit.WaitForReply); err == nil || !errors.Is(err, syscall.ENOBUFS) || retries == 0 {
		return err
	}
	// At this point the netlink channel is congested (ENOBUFS).
	// Drain and close the client, then retry with a new client.
	closeAuditClient(s.client)
	if s.client, err = newAuditClient(s.SocketType); err != nil {
		return fmt.Errorf("failed to recover from ENOBUFS: %w", err)
	}
	logger.Info(s.context.GetRuntimeContext(), "Recovering from ENOBUFS ...")
	return s.setRecvPID(retries - 1)
}

func (s *ServiceLinuxAuditd) addRules() error {
	rules := s.auditRules

	if len(rules) == 0 {
		return nil
	}

	client, err := libaudit.NewAuditClient(nil)
	if err != nil {
		return fmt.Errorf("failed to create audit client for adding rules: %w", err)
	}
	defer closeAuditClient(client)

	// Delete existing rules.
	n, err := client.DeleteRules()
	if err != nil {
		return fmt.Errorf("failed to delete existing rules: %w", err)
	}
	logger.Infof(s.context.GetRuntimeContext(), "Deleted %v pre-existing audit rules.", n)

	// Add rule to ignore syscalls from this process
	if rule, err := buildPIDIgnoreRule(os.Getpid()); err == nil {
		rules = append([]auditRule{rule}, rules...)
	} else {
		logger.Errorf(s.context.GetRuntimeContext(), "Failed to build a rule to ignore self", "error", err)
	}
	// Add rules from config.
	var failCount int
	for _, rule := range rules {
		if err = client.AddRule(rule.data); err != nil {
			// Treat rule add errors as warnings and continue.
			err = fmt.Errorf("failed to add audit rule '%v': %w", rule.flags, err)
			logger.Warningf(s.context.GetRuntimeContext(), "Failure adding audit rule", "error", err)
			failCount++
		}
	}
	logger.Infof(s.context.GetRuntimeContext(), "Successfully added %d of %d audit rules.",
		len(rules)-failCount, len(rules))
	return nil
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
		{
			Key:   "user",
			Value: addUser(auditEvent.User),
		},
		{
			Key:   "process",
			Value: addProcess(auditEvent.Process),
		},
		{
			Key:   "file",
			Value: addFile(auditEvent.File),
		},
		{
			Key:   "source",
			Value: addAddress(auditEvent.Source),
		},
		{
			Key:   "destination",
			Value: addAddress(auditEvent.Dest),
		},
		{
			Key:   "network",
			Value: addNetwork(auditEvent.Net),
		},
		{
			Key:   "tags",
			Value: util.InterfaceToJSONStringIgnoreErr(auditEvent.Tags),
		},
		{
			Key:   "summary",
			Value: addSummary(auditEvent.Summary),
		},
		{
			Key:   "paths",
			Value: util.InterfaceToJSONStringIgnoreErr(auditEvent.Paths),
		},
		{
			Key:   "event",
			Value: normalizeEventFields(auditEvent),
		},
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
			ResolveIDs:            true,
			FailureMode:           "silent",
			BacklogLimit:          8192,
			RateLimit:             0,
			KeepSource:            false,
			KeepWarnings:          false,
			Immutable:             false,
			StreamBufferConsumers: 0,
		}
	}
}
