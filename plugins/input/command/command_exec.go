package command

import (
	"bytes"
	"fmt"
	"log"
	"os/exec"
	"os/user"
	"strconv"
	"syscall"
	"time"
)

func RunCommandWithTimeOut(timeout int, execUser string, command string, args ...string) (stdout, stderr string, isKilled bool, err error) {
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd := exec.Command(command, args...)
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf

	if execUser != "" {
		user, err := user.Lookup(execUser)
		if err == nil {
			log.Printf("uid=%s,gid=%s", user.Uid, user.Gid)
			uid, _ := strconv.Atoi(user.Uid)
			gid, _ := strconv.Atoi(user.Gid)
			cmd.SysProcAttr = &syscall.SysProcAttr{}
			cmd.SysProcAttr.Credential = &syscall.Credential{Uid: uint32(uid), Gid: uint32(gid)}
		}
	}

	err = cmd.Start()
	if err != nil {
		return
	}

	done := make(chan error)

	go func() {
		done <- cmd.Wait()
	}()
	after := time.After(time.Millisecond * time.Duration(timeout))

	select {
	case <-after:
		fmt.Print("after")
		cmd.Process.Signal(syscall.SIGINT)
		time.Sleep(10 * time.Microsecond)
		cmd.Process.Kill()
		isKilled = true
	case <-done:
		fmt.Print("done")
		isKilled = false
	}
	stdout = string(bytes.TrimSpace(stdoutBuf.Bytes()))
	stderr = string(bytes.TrimSpace(stderrBuf.Bytes()))
	return
}
