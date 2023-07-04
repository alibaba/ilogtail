package command

import (
	"bufio"
	"bytes"
	"context"
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"os"
	"os/exec"
	"os/user"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/pluginmanager"
)

type ScriptStorage struct {
	StorageDir string
	ScriptMd5  string
	Err        error
}

var storageOnce sync.Once
var storageInstance *ScriptStorage

func GetStorage(dataDir string) *ScriptStorage {
	dataDir = strings.Trim(dataDir, " ")
	if len(dataDir) == 0 {
		dataDir = pluginmanager.LogtailGlobalConfig.LogtailSysConfDir
	}
	storageOnce.Do(func() {
		storageInstance = &ScriptStorage{
			StorageDir: dataDir,
		}
		storageInstance.Init()
	})
	return storageInstance
}

func (storage *ScriptStorage) Init() {
	//创建目录
	//查询用户是否存在
	isExist, err := util.PathExists(storage.StorageDir)
	if err == nil && isExist {
		return
	}
	cmd := exec.Command("mkdir", "-p", storage.StorageDir)
	_, err = cmd.Output()
	if err != nil {
		storage.Err = fmt.Errorf("execute mkdir -p %s failed with error:%s", storage.StorageDir, err.Error())
		return
	}
}

func getContentMd5(content string) string {
	md5 := md5.New()
	md5.Write([]byte(content))
	return hex.EncodeToString(md5.Sum(nil))
}

func (storage *ScriptStorage) SaveContent(content string, scriptType string) (string, error) {
	md5Str := getContentMd5(content)
	storage.ScriptMd5 = md5Str

	suffix := ScriptTypeToSuffix[scriptType].scriptSuffix
	//保存脚本到机器上
	filePath := fmt.Sprintf("%s/%s.%s", storage.StorageDir, md5Str, suffix)
	isExist, err := util.PathExists(filePath)
	if err == nil && isExist {
		return filePath, nil
	}
	file, err := os.OpenFile(filePath, os.O_WRONLY|os.O_CREATE, 0755)
	if err != nil {
		logger.Error(context.Background(), "文件打开失败", err)
	}
	defer file.Close()
	//写入文件时，使用带缓存的 *Writer
	write := bufio.NewWriter(file)
	write.WriteString(content)
	//Flush将缓存的文件真正写入到文件中
	write.Flush()
	logger.Info(context.Background(), "[input command plugin] ScriptStorage write file Success ", filePath, "content", content)
	return filePath, nil
}

func RunCommandWithTimeOut(timeout int, user *user.User, command string, args ...string) (stdout, stderr string, isKilled bool, err error) {
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd := exec.Command(command, args...)
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf

	logger.Info(context.Background(), "user=%s uid=%s,gid=%s", user.Username, user.Uid, user.Gid)
	uid, _ := strconv.Atoi(user.Uid)
	gid, _ := strconv.Atoi(user.Gid)
	cmd.SysProcAttr = &syscall.SysProcAttr{}
	cmd.SysProcAttr.Credential = &syscall.Credential{Uid: uint32(uid), Gid: uint32(gid)}

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
		cmd.Process.Signal(syscall.SIGINT)
		time.Sleep(10 * time.Microsecond)
		cmd.Process.Kill()
		isKilled = true
	case <-done:
		isKilled = false
	}
	stdout = string(bytes.TrimSpace(stdoutBuf.Bytes()))
	stderr = string(bytes.TrimSpace(stderrBuf.Bytes()))
	return
}
