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

package auditd

import (
	"errors"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/elastic/go-libaudit/v2"
	"github.com/elastic/go-libaudit/v2/auparse"
)

const (
	v1 = iota
	v2

	namespace = "auditd"

	auditLocked = 2

	unicast   = "unicast"
	multicast = "multicast"
	uidUnset  = "unset"

	lostEventsUpdateInterval        = time.Second * 15
	maxDefaultStreamBufferConsumers = 4

	setPIDMaxRetries = 5
)

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

	auditRules []auditRule
	client     *libaudit.AuditClient

	context pipeline.Context
}

type auditRule struct {
	flags string
	data  []byte
}

// stream type

// stream receives callbacks from the libaudit.Reassembler for completed events
// or lost events that are detected by gaps in sequence numbers.
type stream struct {
	done <-chan struct{}
	out  chan<- []*auparse.AuditMessage
}

func (s *stream) ReassemblyComplete(msgs []*auparse.AuditMessage) {
	select {
	case <-s.done:
		return
	case s.out <- msgs:
	}
}

func (s *stream) EventsLost(count int) {

}

// nonBlockingStream behaves as stream above, except that it will never block
// on backpressure from the publishing pipeline.
// Instead, events will be discarded.
type nonBlockingStream stream

func (s *nonBlockingStream) ReassemblyComplete(msgs []*auparse.AuditMessage) {
	select {
	case <-s.done:
		return
	case s.out <- msgs:
	default:

	}
}

func (s *nonBlockingStream) EventsLost(count int) {
	(*stream)(s).EventsLost(count)
}

func (s *ServiceLinuxAuditd) Init(context pipeline.Context) (int, error) {
	s.context = context

	_, _, kernel, _ := kernelVersion()
	logger.Infof(s.context.GetRuntimeContext(), "auditd module is running as euid=%v on kernel=%v", os.Geteuid(), kernel)

	return 0, nil
}

func (s *ServiceLinuxAuditd) Description() string {
	return "This is a service for collect audit events from Linux Audit."
}

// Start the service example plugin would run in a separate go routine, so it is blocking method.
func (s *ServiceLinuxAuditd) Start(collector pipeline.Collector) error {
	logger.Info(s.context.GetRuntimeContext(), "start the ServiceAuditd plugin")

	var err error
	s.client, err = newAuditClient()
	if err != nil {
		return fmt.Errorf("failed to create audit client: %w", err)
	}

	status, err := s.client.GetStatus()
	if err != nil {
		return fmt.Errorf("failed to get audit status before adding rules: %w", err)
	}

	if status.Enabled == auditLocked {

	}

	status, err = s.client.GetStatus()
	if err != nil {
		return fmt.Errorf("failed to get audit status: %w", err)
	}
	log.Printf("received audit status=%+v", status)

	if status.Enabled == 0 {
		log.Println("enabling auditing in the kernel")
		if err = s.client.SetEnabled(true, libaudit.WaitForReply); err != nil {
			return fmt.Errorf("failed to set enabled=true: %w", err)
		}
	}

	if err = s.client.SetRateLimit(uint32(0), libaudit.NoWait); err != nil {
		return fmt.Errorf("failed to set rate limit to unlimited: %w", err)
	}

	if err = s.client.SetBacklogLimit(uint32(8192), libaudit.NoWait); err != nil {
		return fmt.Errorf("failed to set backlog limit: %w", err)
	}

	// if status.Enabled != 2 && *immutable {
	// 	log.Printf("setting kernel settings as immutable")
	// 	if err = client.SetImmutable(libaudit.NoWait); err != nil {
	// 		return fmt.Errorf("failed to set kernel as immutable: %w", err)
	// 	}
	// }

	log.Printf("sending message to kernel registering our PID (%v) as the audit daemon", os.Getpid())
	if err = s.client.SetPID(libaudit.NoWait); err != nil {
		return fmt.Errorf("failed to set audit PID: %w", err)
	}

	for {
		rawEvent, err := s.client.Receive(false)
		if err != nil {
			return fmt.Errorf("receive failed: %w", err)
		}

		// Messages from 1300-2999 are valid audit messages.
		if rawEvent.Type < auparse.AUDIT_USER_AUTH ||
			rawEvent.Type > auparse.AUDIT_LAST_USER_MSG2 {
			continue
		}

		fmt.Printf("type=%v msg=%s\n", rawEvent.Type, rawEvent.Data)
	}

	// out, err := s.receiveEvents()
	// if err != nil {
	// 	return err
	// }

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

func (s *ServiceLinuxAuditd) receiveEvents() (<-chan []*auparse.AuditMessage, error) {
	out := make(chan []*auparse.AuditMessage, 8192)

	var st libaudit.Stream = &stream{nil, out}
	// if ms.backpressureStrategy&bsUserSpace != 0 {
	// 	// "user-space" backpressure mitigation strategy
	// 	//
	// 	// Consume events from our side as fast as possible, by dropping events
	// 	// if the publishing pipeline would block.
	// 	ms.log.Info("Using non-blocking stream to prevent backpressure propagating to the kernel.")
	// 	st = &nonBlockingStream{done, out}
	// }
	reassembler, err := libaudit.NewReassembler(int(50), 2*time.Second, st)
	if err != nil {
		return nil, fmt.Errorf("failed to create Reassembler: %w", err)
	}
	//go maintain(done, reassembler)

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

			// if filterRecordType(raw.Type) {
			// 	continue
			// }
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

func kernelVersion() (major, minor int, full string, err error) {
	var uname syscall.Utsname
	if err := syscall.Uname(&uname); err != nil {
		return 0, 0, "", err
	}

	length := len(uname.Release)
	data := make([]byte, length)
	for i, v := range uname.Release {
		if v == 0 {
			length = i
			break
		}
		data[i] = byte(v)
	}

	release := string(data[:length])
	parts := strings.SplitN(release, ".", 3)
	if len(parts) < 2 {
		return 0, 0, release, fmt.Errorf("failed to parse uname release '%v'", release)
	}

	major, err = strconv.Atoi(parts[0])
	if err != nil {
		return 0, 0, release, fmt.Errorf("failed to parse major version from '%v': %w", release, err)
	}

	minor, err = strconv.Atoi(parts[1])
	if err != nil {
		return 0, 0, release, fmt.Errorf("failed to parse minor version from '%v': %w", release, err)
	}

	return major, minor, release, nil
}

func newAuditClient() (*libaudit.AuditClient, error) {
	return libaudit.NewAuditClient(nil)
}

func closeAuditClient(client *libaudit.AuditClient) error {
	discard := func(bytes []byte) ([]syscall.NetlinkMessage, error) {
		return nil, nil
	}
	// Drain the netlink channel in parallel to Close() to prevent a deadlock.
	// This goroutine will terminate once receive from netlink errors (EBADF,
	// EBADFD, or any other error). This happens because the fd is closed.
	go func() {
		for {
			_, err := client.Netlink.Receive(true, discard)
			switch {
			case err == nil, errors.Is(err, syscall.EINTR):
			case errors.Is(err, syscall.EAGAIN):
				time.Sleep(50 * time.Millisecond)
			default:
				return
			}
		}
	}()
	if err := client.Close(); err != nil {
		return fmt.Errorf("Error closing audit monitoring client %w", err)
	}
	return nil
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
