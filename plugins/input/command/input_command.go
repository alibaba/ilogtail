package command

import (
	"context"
	"encoding/base64"
	"fmt"
	"os/user"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type InputCommand struct {
	ScriptType          string `comment:"脚本类型, bash, shell，python2，python3"`
	User                string `comment:"执行的用户"`
	ScriptContent       string `comment:"脚本内容 PlainText|Base64  两种格式"`
	ContentEncoding     string `comment:"指定脚本格式  PlainText|Base64"`
	LineSplitSep        string `comment:"分隔符"`
	TimeoutMilliSeconds int    `comment:"执行一次collect的超时时间，超时后会通知context.Alarm 单位毫秒,  不超过collect的采集频率"`
	ScriptDataDir       string `comment:"执行脚本的存放目前"`
	CmdPath             string `comment:"需要执行可执行命令的路径  默认/usr/bin/${bin}"`
	IntervalMs          int    `comment:"collect触发频率  单位毫秒"`

	context         pipeline.Context
	storageInstance ScriptStorage
	scriptPath      string
	cmdUser         *user.User
}

func (in *InputCommand) Init(context pipeline.Context) (int, error) {
	in.context = context
	//执行校验
	valid, err := in.Validate()
	if err != nil || !valid {
		return 0, err
	}

	//解析Base64内容
	if in.ContentEncoding == ContentTypeBase64 {
		decodeContent, err := base64.StdEncoding.DecodeString(in.ScriptContent)
		if err != nil {
			return 0, fmt.Errorf("base64.StdEncoding error:%s", err)
		}
		in.ScriptContent = string(decodeContent)
	}

	storageInstance = GetStorage(in.ScriptDataDir)
	if storageInstance.Err != nil {
		return 0, fmt.Errorf("init storageInstance error : %s", storageInstance.Err)
	}
	scriptPath, err := storageInstance.SaveContent(in.ScriptContent, in.ScriptType)
	if in.CmdPath == "" {
		in.CmdPath = ScriptTypeToSuffix[in.ScriptType].defaultCmdPath
	}
	if err != nil {
		return 0, fmt.Errorf("storageInstance.SaveContent error: %s", err)
	}
	in.storageInstance = *storageInstance
	in.scriptPath = scriptPath

	user, err := user.Lookup(in.User)
	if err != nil {
		return 0, fmt.Errorf("cannot find user %s, error: %s", in.User, err)
	}
	in.cmdUser = user
	return 0, nil
}

func (in *InputCommand) Validate() (bool, error) {
	if _, ok := ScriptTypeToSuffix[in.ScriptType]; !ok {
		return false, fmt.Errorf("not support ScriptType %s", in.ScriptType)
	}

	if in.User == UserRoot {
		//return false, fmt.Errorf("exec command not support user root")
	} else if len(in.User) == 0 {
		return false, fmt.Errorf("params User can not be empty")
	}

	if _, ok := SupportContentType[in.ContentEncoding]; !ok {
		return false, fmt.Errorf("not support ContentType %s", in.ContentEncoding)
	}

	if in.ScriptContent == "" {
		return false, fmt.Errorf("params ScriptContent can not empty")
	}

	if in.TimeoutMilliSeconds > in.IntervalMs {
		return false, fmt.Errorf("not support ExecScriptTimeOut > IntervalMs(脚本执行时间不能大于采集的频率)")
	}
	return true, nil
}

func (in *InputCommand) ExecScript(filePath string) (string, error) {
	stdoutStr, stderrStr, isKilled, err := RunCommandWithTimeOut(in.TimeoutMilliSeconds, in.cmdUser, in.CmdPath, in.scriptPath)
	if err != nil {
		return "", fmt.Errorf("exec cmd error errInfo:%s, stderr:%s, stdout:%s", err, stderrStr, stdoutStr)
	}
	if isKilled {
		fmt.Println("timeout run exec script file filepath", filePath)
		return "", fmt.Errorf("timeout run exec script file filepath %s", filePath)
	}
	if stderrStr != "" {
		fmt.Println("run exec script file filepath", filePath, "stderr", stderrStr, "stdout", stdoutStr)
	}
	return stdoutStr, nil
}

func (in *InputCommand) Collect(collector pipeline.Collector) error {
	stdoutStr, stderrStr, isKilled, err := RunCommandWithTimeOut(in.TimeoutMilliSeconds, in.cmdUser, in.CmdPath, in.scriptPath)

	if err != nil {
		return fmt.Errorf("exec cmd error errInfo:%s, stderr:%s, stdout:%s", err, stderrStr, stdoutStr)
	}
	if isKilled {
		return fmt.Errorf("timeout run exec script file filepath %s", in.scriptPath)
	}
	if len(stderrStr) != 0 {
		fmt.Println("run exec script file ", in.scriptPath, "stderr", stderrStr, "stdout", stdoutStr)
	}

	logger.Infof(context.Background(), "exec output return [%s]", stdoutStr)

	outSplitArr := strings.Split(stdoutStr, in.LineSplitSep)

	logger.Infof(context.Background(), "outSplitArr len %d", len(outSplitArr))

	for _, splitStr := range outSplitArr {
		log := &protocol.Log{
			Time: uint32(time.Now().Unix()),
			Contents: []*protocol.Log_Content{
				{
					Key:   models.ContentKey,
					Value: string(splitStr),
				},
				{
					Key:   ScriptMd5,
					Value: in.storageInstance.ScriptMd5,
				},
			},
		}
		if len(stderrStr) > 0 {
			log.Contents = append(log.Contents, &protocol.Log_Content{
				Key:   "stderr",
				Value: stderrStr,
			})
		}
		collector.AddRawLog(log)
	}
	return nil
}

func (in *InputCommand) Description() string {
	return "Collect the metrics by exec script "
}

func init() {
	pipeline.MetricInputs[pluginName] = func() pipeline.MetricInput {
		return &InputCommand{
			ContentEncoding:     defaultContentType,
			LineSplitSep:        defaultLineSplitSep,
			IntervalMs:          defaultIntervalMs,
			TimeoutMilliSeconds: defaltExecScriptTimeOut,
		}
	}
}
