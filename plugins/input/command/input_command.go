package command

import (
	"context"
	"crypto/md5"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

type InputCommand struct {
	ScriptType    string `json:"script_type" comment:"脚本类型, bash, shell"`
	User          string `json:"user" comment:"执行的用户"`
	ScriptContent string `json:"script_content" comment:"脚本内容 PlainText|Base64  两种格式"`
	ContentType   string `json:"content_type" comment:"指定脚本格式  PlainText|Base64"`
	LineSplitSep  string `json:"line_split_sep" comment:"分隔符"`
	ScriptDataDir string `json:"script_data_dir" comment:"执行脚本的存放目前"`
	CmdPath       string `json:"cmd_path" comment:"需要执行可执行命令的路径 默认 /usr/bin/sh`
	ExporterName  string `json:"exporter_name" comment:"注册的exporterName 表示执行脚本内容metric的类别 类似与node_export   mysql_export等 默认为空 会提交到label中"`
	//脚本执行后输出的格式
	//sls_metrics
	//example: __labels__:hostname#$#idc_cluster_env_name|ip#$#ip_address   __value__:0  __name__:metric_command_example
	//json 暂不支持
	//prometheus 暂不支持
	OutputDataType    string `json:"output_data_type" comment:"目前先支持sls_metrics格式"`
	ExecScriptTimeOut int    `json:"exec_script_timeout" comment:"执行一次collect的超时时间，超时后会通知context.Alarm 单位毫秒,  不超过collect的采集频率"`
	IntervalMs        int    `json:"interval_ms" comment:"//collect触发频率  单位毫秒"`

	hostname   string
	ip         string
	context    pipeline.Context
	labelStore helper.KeyValues
	md5        string

	validator *validator
	decoder   Decoder
}

type MetricData struct {
	Name         string
	LabelsString string
	Value        float64
	Labels       map[string]string
}

func (in *InputCommand) setCommonLabels() {
	in.labelStore.Append("hostname", in.hostname)
	in.labelStore.Append("ip", in.ip)
	in.labelStore.Append("script_md5", in.md5)
	in.labelStore.Append("script_exporter", in.ExporterName)
}

func getContentMd5(content string) string {
	md5 := md5.New()
	md5.Write([]byte(content))
	return hex.EncodeToString(md5.Sum(nil))
}

func (in *InputCommand) Init(context pipeline.Context) (int, error) {
	in.context = context
	//init validator
	in.validator = &validator{
		In: in,
	}
	//执行校验
	if in.validator.Validate() == false {
		return 0, fmt.Errorf("validate error on validator errorInfo:%s", in.validator.Error())
	}

	//init decoder
	in.decoder = GetDecoder(in.OutputDataType)

	//解析Base64内容
	if in.ContentType == "Base64" {
		decodeContent, err := base64.StdEncoding.DecodeString(in.ScriptContent)
		if err != nil {
			return 0, fmt.Errorf("base64.StdEncoding error  errorInfo:%s", err)
		}
		in.ScriptContent = string(decodeContent)
	}
	in.md5 = getContentMd5(in.ScriptContent)
	//set 公共的label
	in.setCommonLabels()
	return 0, nil
}

func (in *InputCommand) ExecScript(filePath string) (string, error) {
	if in.ScriptType != "Bash" && in.ScriptType != "Shell" {
		return "", fmt.Errorf("only support bash and shell")
	}
	stdoutStr, stderrStr, isKilled, err := RunCommandWithTimeOut(in.ExecScriptTimeOut, in.User, "/usr/bin/sh", filePath)

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
	storageInstance := GetStorage(in.ScriptDataDir)
	if storageInstance.Err != nil {
		return fmt.Errorf("init storageInstance error : %s", storageInstance.Err)
	}
	filepath, err := storageInstance.SaveContent(in.ScriptContent)
	if err != nil {
		return fmt.Errorf("error on storageInstance.SaveContent %s", err)
	}
	scriptOut, err := in.ExecScript(filepath)
	if err != nil {
		return fmt.Errorf("error on in.ExecScript error: %s", err)
	}
	logger.Infof(context.Background(), "exec output return [%s]", scriptOut)
	var outSplitArr []string

	if in.LineSplitSep == "\\n" {
		outSplitArr = strings.Split(scriptOut, "\n")
	} else {
		outSplitArr = strings.Split(scriptOut, in.LineSplitSep)
	}

	// fmt.Printf("split text len %d, %+v \n", len(outSplitArr), outSplitArr)
	//分拆文本内容并解析
	metricItems := in.ParseToMetricData(outSplitArr)
	//添加Metrics
	logger.Infof(context.Background(), "add metricItems len %d", len(metricItems))

	for _, metricItme := range metricItems {
		// fmt.Printf("metricItemsLabels len %s \n", metricItme.LabelsString)
		helper.AddMetric(collector, metricItme.Name, time.Now(), metricItme.LabelsString, metricItme.Value)
	}
	return nil
}

func (in *InputCommand) ParseToMetricData(execReturnArr []string) (re []*MetricData) {
	for _, v := range execReturnArr {
		v = strings.TrimSpace(v)
		if len(v) == 0 {
			continue
		}
		decodeResult := in.decoder.Decode(v)
		if decodeResult.Err != nil {
			//记录debug log
			logger.Infof(context.Background(), "decode mistake", decodeResult.Err)
			continue
		}
		tempLabelStore := in.labelStore.Clone()
		tempLabelStore.AppendMap(decodeResult.Labels)
		re = append(re, &MetricData{
			Name:         decodeResult.MetricName,
			LabelsString: tempLabelStore.String(),
			Value:        decodeResult.Float64Value,
			Labels:       decodeResult.Labels,
		})
	}
	return re
}

func (in *InputCommand) Description() string {
	return "Collect the metrics by exec script "
}

func init() {
	pipeline.MetricInputs[pluginName] = func() pipeline.MetricInput {
		return &InputCommand{
			ScriptType:        defaultScriptType,
			ScriptContent:     defaultContent,
			ContentType:       defaultContentType,
			User:              defaultUser,
			LineSplitSep:      defaultLineSplitSep,
			OutputDataType:    defaultOutputDataType,
			IntervalMs:        defaultIntervalMs,
			ScriptDataDir:     defaultScirptDataDir,
			ExecScriptTimeOut: defaltExecScriptTimeOut,
			CmdPath:           defaultCmdPath,
			ExporterName:      defaultExporterName,

			hostname: util.GetHostName(),
			ip:       util.GetIPAddress(),
		}
	}
}
