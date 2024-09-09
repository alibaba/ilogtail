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

//go:build unix

package linux_auditd

import (
	"bufio"
	"bytes"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"syscall"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/elastic/go-libaudit/v2"
	"github.com/elastic/go-libaudit/v2/rule"
	"github.com/elastic/go-libaudit/v2/rule/flags"
	"github.com/joeshaw/multierror"
	"github.com/siddontang/go/log"
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

// Validate validates the rules specified in the config.
func (s *ServiceLinuxAuditd) ValidateConfig() error {
	var errs multierror.Errors
	err := s.LoadRules()
	if err != nil {
		errs = append(errs, err)
	}
	_, err = s.failureMode()
	if err != nil {
		errs = append(errs, err)
	}

	s.SocketType = strings.ToLower(s.SocketType)
	switch s.SocketType {
	case "multicast":
		if s.Immutable {
			errs = append(errs, fmt.Errorf("immutable can't be used with socket_type: multicast"))
		}
	case "", "unicast":
	default:
		errs = append(errs, fmt.Errorf("invalid socket_type "+
			"'%v' (use unicast, multicast, or don't set a value)", s.SocketType))
	}

	return errs.Err()
}

func (s *ServiceLinuxAuditd) LoadRules() error {
	knownRules := ruleSet{}

	rules, err := readRules(bytes.NewBufferString(s.RulesBlob), "(audit_rules at auditbeat.yml)", knownRules)
	if err != nil {
		return err
	}
	s.auditRules = append(s.auditRules, rules...)

	return nil
}

func (s *ServiceLinuxAuditd) failureMode() (uint32, error) {
	switch strings.ToLower(s.FailureMode) {
	case "silent":
		return 0, nil
	case "log":
		return 1, nil
	case "panic":
		return 2, nil
	default:
		return 0, fmt.Errorf("invalid failure_mode '%v' (use silent, log, or panic)", s.FailureMode)
	}
}

// readRules reads the audit rules from reader, adding them to knownRules. If
// log is nil, errors will result in an empty rules set being returned. Otherwise
// errors will be logged as warnings and any successfully parsed rules will be
// returned.
func readRules(reader io.Reader, source string, knownRules ruleSet) (rules []auditRule, err error) {
	var errs multierror.Errors

	s := bufio.NewScanner(reader)
	for lineNum := 1; s.Scan(); lineNum++ {
		location := fmt.Sprintf("%s:%d", source, lineNum)
		line := strings.TrimSpace(s.Text())
		if len(line) == 0 || line[0] == '#' {
			continue
		}

		// Parse the CLI flags into an intermediate rule specification.
		r, err := flags.Parse(line)
		if err != nil {
			errs = append(errs, fmt.Errorf("at %s: failed to parse rule '%v': %w", location, line, err))
			continue
		}

		// Convert rule specification to a binary rule representation.
		data, err := rule.Build(r)
		if err != nil {
			errs = append(errs, fmt.Errorf("at %s: failed to interpret rule '%v': %w", location, line, err))
			continue
		}

		// Detect duplicates based on the normalized binary rule representation.
		existing, found := knownRules[string(data)]
		if found {
			errs = append(errs, fmt.Errorf("at %s: rule '%v' is a duplicate of '%v' at %s", location, line, existing.rule.flags, existing.source))
			continue
		}
		rule := auditRule{flags: line, data: []byte(data)}
		knownRules[string(data)] = ruleWithSource{rule, location}

		rules = append(rules, rule)
	}

	if len(errs) != 0 {
		log.Warnf("errors loading rules: %v", errs.Err())
	}
	return rules, nil
}

func getSocketType(s *ServiceLinuxAuditd) (string, error) {
	client, err := libaudit.NewAuditClient(nil)
	if err != nil {
		if s.SocketType == "" {
			return "", fmt.Errorf("failed to create audit client: %w", err)
		}
		// Ignore errors if a socket type has been specified. It will fail during
		// further setup and its necessary for unit tests to pass
		return s.SocketType, nil
	}
	defer client.Close()
	status, err := client.GetStatus()
	if err != nil {
		if s.SocketType == "" {
			return "", fmt.Errorf("failed to get audit status: %w", err)
		}
		return s.SocketType, nil
	}

	isLocked := (status.Enabled == auditLocked)
	hasRules := len(s.auditRules) > 0

	const useAutodetect = "Remove the socket_type option to have auditbeat " +
		"select the most suitable subscription method."
	switch s.SocketType {
	case unicast:
		if isLocked && !s.Immutable {
			logger.Errorf(s.context.GetRuntimeContext(), "requested unicast socket_type is not available "+
				"because audit configuration is locked in the kernel "+
				"(enabled=2). %s", useAutodetect)
			return "", errors.New("unicast socket_type not available")
		}
		return s.SocketType, nil

	case multicast:
		if s.supportMulticast {
			if hasRules {
				logger.Warning(s.context.GetRuntimeContext(), "The audit rules specified in the configuration "+
					"cannot be applied when using a multicast socket_type.")
			}
			return s.SocketType, nil
		}
		logger.Error(s.context.GetRuntimeContext(), "socket_type is set to multicast but based on the "+
			"kernel version, multicast audit subscriptions are not supported. %s", useAutodetect)
		return "", errors.New("multicast is not supported for current kernel")

	default:
		// attempt to determine the optimal socket_type
		if s.supportMulticast {
			if hasRules {
				if isLocked && !s.Immutable {
					logger.Warning(s.context.GetRuntimeContext(), "Audit rules specified in the configuration "+
						"cannot be applied because the audit rules have been locked "+
						"in the kernel (enabled=2). A multicast audit subscription "+
						"will be used instead, which does not support setting rules")
					return multicast, nil
				}
				return unicast, nil
			}
			return multicast, nil
		}
		if isLocked && !s.Immutable {
			logger.Error(s.context.GetRuntimeContext(), "Cannot continue: audit configuration is locked "+
				"in the kernel (enabled=2) which prevents using unicast "+
				"sockets. Multicast audit subscriptions are not available "+
				"in this kernel. Disable locking the audit configuration "+
				"to use auditbeat.")
			return "", errors.New("no connection to audit available")
		}
		return unicast, nil
	}
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

func getBackpressureStrategy(value string) backpressureStrategy {
	switch value {
	case "", "default":
		return bsAuto
	case "kernel":
		return bsKernel
	case "userspace", "user-space":
		return bsUserSpace
	case "auto":
		return bsAuto
	case "both":
		return bsKernel | bsUserSpace
	default:
		return 0
	}
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
