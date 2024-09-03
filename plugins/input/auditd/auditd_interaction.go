// Licensed to Elasticsearch B.V. under one or more contributor
// license agreements. See the NOTICE file distributed with
// this work for additional information regarding copyright
// ownership. Elasticsearch B.V. licenses this file to you under
// the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

//go:build !windows
// +build !windows

package auditd

import (
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/elastic/go-libaudit/v2"
	"github.com/elastic/go-libaudit/v2/auparse"
	"github.com/elastic/go-libaudit/v2/rule"
)

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
	// 补充指标
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
		// 补充指标
	}
}

func (s *nonBlockingStream) EventsLost(count int) {
	(*stream)(s).EventsLost(count)
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
		if fm, _ := s.failureMode(); status.Failure != fm {
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
	if s.client, err = newAuditClient(s); err != nil {
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

func buildPIDIgnoreRule(pid int) (ruleData auditRule, err error) {
	r := rule.SyscallRule{
		Type:   rule.AppendSyscallRuleType,
		List:   "exit",
		Action: "never",
		Filters: []rule.FilterSpec{
			{
				Type:       rule.ValueFilterType,
				LHS:        "pid",
				Comparator: "=",
				RHS:        strconv.Itoa(pid),
			},
		},
		Syscalls: []string{"all"},
		Keys:     nil,
	}
	ruleData.flags = fmt.Sprintf("-A exit,never -F pid=%d -S all", pid)
	ruleData.data, err = rule.Build(&r)
	return ruleData, err
}

func filterRecordType(typ auparse.AuditMessageType) bool {
	switch {
	// REPLACE messages are tests to check if Auditbeat is still healthy by
	// seeing if unicast messages can be sent without error from the kernel.
	// Ignore them.
	case typ == auparse.AUDIT_REPLACE:
		return true
	// Messages from 1300-2999 are valid audit message types.
	case (typ < auparse.AUDIT_USER_AUTH || typ > auparse.AUDIT_LAST_USER_MSG2) && typ != auparse.AUDIT_LOGIN:
		return true
	}

	return false
}

func createAuditdData(data map[string]string) string {
	out := make(map[string]string)
	for key, v := range data {
		if strings.HasPrefix(key, "socket_") {
			out["socket."+key[7:]] = v
			continue
		}

		out[key] = v
	}
	return util.InterfaceToJSONStringIgnoreErr(out)
}

func newAuditClient(s *ServiceLinuxAuditd) (*libaudit.AuditClient, error) {
	var err error
	s.SocketType, err = getSocketType(s)
	if err != nil {
		return nil, err
	}

	if s.SocketType == multicast {
		return libaudit.NewMulticastAuditClient(nil)
	}
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

// maintain periodically evicts timed-out events from the Reassembler. This
// function will block until the done channel is closed or the Reassembler is
// closed.
func maintain(done <-chan struct{}, reassembler *libaudit.Reassembler) {
	tick := time.NewTicker(500 * time.Millisecond)
	defer tick.Stop()

	for {
		select {
		case <-done:
			return
		case <-tick.C:
			if err := reassembler.Maintain(); err != nil {
				return
			}
		}
	}
}
