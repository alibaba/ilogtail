package command

import (
	"encoding/base64"
	"fmt"
	"io/ioutil"
	"os/user"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

type parseRe struct {
	Name   string
	Value  string
	Labels map[string]string
}

func GetLabelByKey(key string, data map[string]string) string {
	if k, ok := data[key]; ok {
		return k
	}
	return "___"
}

func parseContent(contents []*protocol.Log_Content) parseRe {
	convert2Map := func(str string) (res map[string]string) {
		res = make(map[string]string)
		labelsArr := strings.Split(str, "|")
		for _, v := range labelsArr {
			kvArr := strings.Split(v, "#$#")
			res[kvArr[0]] = kvArr[1]
		}
		return res
	}
	tempRe := parseRe{
		Labels: make(map[string]string),
	}
	for _, content := range contents {
		switch content.Key {
		case "__name__":
			tempRe.Name = content.Value
		case "__labels__":
			tempRe.Labels = convert2Map(content.Value)
		case "__value__":
			tempRe.Value = content.Value
		}
	}
	return tempRe
}

/*
func TestOneLine(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	// c := new(test.MockMetricCollector)
	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	testReturn := `
     __value__:0.0  __name__:metric_command_example_4 __test__:1

		__value__:0.0  __name__:metric_command_example_5 __test__:1

		__value__:0.0  __name__:metric_command_example_no_break __test__:1

		__labels__:b#$#2|a#$#1   __value__:0.0  __name__:metric_command_example
	a:1  b:2  c:3
	     __value__:0.0  __name__:metric_command_example_big_space __test__:1
	__value__:0  __name__:metric_command_example_without_labels
	# 这是一个注释
	#  __value__:0  __name__:metric_command_example_without_labels
	`
	testReArr := strings.Split(testReturn, "\n")
	fmt.Println("testReArr", testReArr)
	re := p.ParseToMetricData(testReArr)
	for _, item := range re {
		fmt.Println(item.Name, item.LabelsString)
	}
	fmt.Print(testReturn)
}

// 测试文本输出结果的解析
func TestCommandParseToMetricData(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	// c := new(test.MockMetricCollector)
	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	testReturn := `__labels__:b#$#2|a#$#1   __value__:0.0  __name__:metric_command_example  \n
	a:1  b:2  c:3 \n
	__value__:0  __name__:metric_command_example_without_labels \n
	# 这是一个注释
	#  __value__:0  __name__:metric_command_example_without_labels
	`
	testReArr := strings.Split(testReturn, "\n")
	re := p.ParseToMetricData(testReArr)
	if len(re) > 2 {
		t.Errorf("expect parse right line 2 get %d", len(re))
		return
	}
	for _, metricItem := range re {
		fmt.Println("metricItem", metricItem)
		labels := metricItem.Labels
		if metricItem.Name == "metric_command_example_without_labels" {
			if metricItem.Value != float64(0) {
				t.Errorf("parse metric_command_example_without_labels error expect value 0 get %f", metricItem.Value)
				return
			}
			if len(labels) != 0 {
				t.Errorf("expect labels empty get %+v", labels)
				return
			}
		}
		if metricItem.Name == "metric_command_example" {
			if GetLabelByKey("hostname", labels) != util.GetHostName() {
				t.Errorf("parse hostname error expect %s get %s ", util.GetHostName(), GetLabelByKey("hostname", labels))
				return
			}
			if GetLabelByKey("ip", labels) != util.GetIPAddress() {
				t.Errorf("parse ip error expect %s get %s ", util.GetIPAddress(), GetLabelByKey("ip", labels))
				return
			}

			if GetLabelByKey("a", labels) != "1" {
				t.Errorf("parse a error expect %s get %s ", "1", GetLabelByKey("a", labels))
				return
			}

			if GetLabelByKey("b", labels) != "1" {
				t.Errorf("parse b error expect %s get %s ", "1", GetLabelByKey("b", labels))
				return
			}
		}
	}
}
*/

func TestCommandTestCollecetUserBase64WithTimeout(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	c := new(test.MockMetricCollector)
	//脚本设置睡眠5秒
	scriptContent := `sleep 5 &&  echo -e "__labels__:a#\$#1|b#\$#2    __value__:0  __name__:metric_command_example \n __labels__:a#\$#3|b#\$#4    __value__:3  __name__:metric_command_example2"`

	//指定base64
	p.ScriptContent = base64.StdEncoding.EncodeToString([]byte(scriptContent))
	p.ContentEncoding = "Base64"
	p.TimeoutMilliSeconds = 3000
	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	err := p.Collect(c)
	if err == nil {
		t.Errorf("expect error with timeout")
	}

	//变更timeout
	p.TimeoutMilliSeconds = 6000
	//指定base64
	p.ScriptContent = base64.StdEncoding.EncodeToString([]byte(scriptContent))
	p.ContentEncoding = "Base64"
	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	err = p.Collect(c)
	if err != nil {
		t.Errorf("Collect() error = %v", err)
		return
	}

	// fmt.Println("--------labels-----", meta.Labels)
	shouldReturn := assertLogs(c, t, p)
	if shouldReturn {
		return
	}

}

func TestCommandTestCollecetUserBase64(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	c := new(test.MockMetricCollector)
	scriptContent := `echo -e "__labels__:a#\$#1|b#\$#2    __value__:0  __name__:metric_command_example \n __labels__:a#\$#3|b#\$#4    __value__:3  __name__:metric_command_example2"`
	//指定base64
	p.ScriptType = "shell"
	p.CmdPath = "/usr/bin/sh"
	p.ScriptContent = base64.StdEncoding.EncodeToString([]byte(scriptContent))
	p.ContentEncoding = "Base64"
	p.User = "root"
	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	if err := p.Collect(c); err != nil {
		t.Errorf("Collect() error = %v", err)
		return
	}
	// fmt.Println("--------labels-----", meta.Labels)
	shouldReturn := assertLogs(c, t, p)
	if shouldReturn {
		return
	}
}

func TestCommandTestCollect(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	c := new(test.MockMetricCollector)

	p.ScriptContent = `cat /var/log/messages`
	p.ScriptType = "shell"
	p.ContentEncoding = "PlainText"
	p.User = "root"

	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	if err := p.Collect(c); err != nil {
		t.Errorf("Collect() error = %v", err)
		return
	}

	shouldReturn := assertLogs(c, t, p)
	if shouldReturn {
		return
	}
}

func TestCommandTestExceptionCollect(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	c := new(test.MockMetricCollector)

	p.ScriptContent = `xxxxxX`
	p.ScriptType = "shell"
	p.ContentEncoding = "PlainText"
	p.User = "root"

	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	if err := p.Collect(c); err != nil {
		t.Errorf("Collect() error = %v", err)
		return
	}

	shouldReturn := assertLogs(c, t, p)
	if shouldReturn {
		return
	}
}

func TestCommandTestTimeoutCollect(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	c := new(test.MockMetricCollector)

	p.ScriptContent = `sleep 10`
	p.ScriptType = "shell"
	p.ContentEncoding = "PlainText"
	p.User = "root"

	if _, err := p.Init(ctx); err != nil {
		t.Errorf("cannot init InputCommand: %v", err)
		return
	}
	if err := p.Collect(c); err != nil {
		t.Errorf("Collect() error = %v", err)
		return
	}

	shouldReturn := assertLogs(c, t, p)
	if shouldReturn {
		return
	}
}

func assertLogs(c *test.MockMetricCollector, t *testing.T, p *InputCommand) bool {
	for _, log := range c.Logs {
		fmt.Println("logs", log)
	}
	return false
}

func TestCommandTestInit(t *testing.T) {
	ctx := mock.NewEmptyContext("project", "store", "config")
	p := pipeline.MetricInputs[pluginName]().(*InputCommand)
	_, err := p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("default config error", err)
	}

	//测试错误的script type
	p.ScriptType = "golang_none"
	_, err = p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("expect error with script type not support", err)
	}
	p.ScriptType = "bash"

	//测试错误的User
	p.User = "root"
	_, err = p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("expect error with wrong user root", err)
	}
	p.User = "someone"

	//测试contentType
	p.ContentEncoding = "mixin"
	_, err = p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("expect error with ContentType error", err)
	}
	p.ContentEncoding = defaultContentType

	//测试脚本内容为空
	p.ScriptContent = ""
	_, err = p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("expect error with ScriptContent empty error", err)
	}
	p.ScriptContent = "some"
	//测试执行脚本超时
	//测试输出的dataType
	p.IntervalMs = 3000
	p.TimeoutMilliSeconds = 4000
	_, err = p.Init(ctx)
	require.Error(t, err)
	if err != nil {
		fmt.Println("expect error with ExecScriptTimeOut > IntervalMs ", err)
	}
}

// 测试script storage
func TestScriptStorage(t *testing.T) {
	u, err := user.Current()
	fmt.Printf("Username %s\n", u.Username)

	content := `echo -e "__labels__:hostname#\$#idc_cluster_env_name|ip#\$#ip_address    __value__:0  __name__:metric_command_example"`
	storage := GetStorage("/data/workspaces/ilogtail/scriptStorage/")
	if storage.Err != nil {
		t.Errorf("create Storage error %s", storage.Err)
		return
	}
	filepath, err := storage.SaveContent(content, "shell")
	if err != nil {
		t.Errorf("ScriptStorage save content error %s", err)
		return
	}
	data, err := ioutil.ReadFile(filepath)
	if err != nil {
		t.Errorf("read file error")
		return
	}
	if string(data) != content {
		t.Errorf("content compare error")
		return
	}

	fmt.Print("\n---TestScriptStorage filepath", filepath, "\n")

	//再次获取
	filepath, _ = storage.SaveContent(content, "shell")
	data, _ = ioutil.ReadFile(filepath)
	if string(data) != content {
		t.Errorf("content compare error")
		return
	}
}
