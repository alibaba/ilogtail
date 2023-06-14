package command

import (
	"fmt"
)

type validator struct {
	In *InputCommand
}

var validateError error

// ScriptType    string //脚本类型, bash, shell
// 	User          string //执行的用户
// 	ScriptContent string //脚本内容 PlainText|Base64  两种格式
// 	ContentType   string //指定脚本格式  PlainText|Base64
// 	LineSplitSep  string //行分隔符
// 	//脚本执行后输出的格式
// 	//sls_metrics
// 	//__labels__:hostname#$#idc_cluster_env_name|ip#$#ip_address   __value__:0  __name__:system_load1
// 	//json 暂不支持
// 	//prometheus 暂不支持
// 	OutputDataType string //目前先支持sls_metrics格式
// 	IntervalMs     int    //collect触发频率  单位毫秒
func (v *validator) Validate() bool {
	in := v.In
	if _, ok := supportScriptTypes[in.ScriptType]; !ok {
		validateError = fmt.Errorf("not support script type %s", in.ScriptType)
		return false
	}

	if in.User == "root" {
		validateError = fmt.Errorf("exec command not support use root")
		return false
	}

	if _, ok := supportContentType[in.ContentType]; !ok {
		validateError = fmt.Errorf("not support content type %s", in.ContentType)
		return false
	}

	if in.ScriptContent == "" {
		validateError = fmt.Errorf("ScriptContent can not empty")
		return false
	}

	if _, ok := supportOutPutDataType[in.OutputDataType]; !ok {
		validateError = fmt.Errorf("not support content type %s", in.OutputDataType)
		return false
	}

	if in.ExecScriptTimeOut > in.IntervalMs {
		validateError = fmt.Errorf("not support ExecScriptTimeOut > IntervalMs(脚本执行时间不能大于采集的频率)")
		return false
	}

	return true
}

func (v *validator) Error() error {
	return validateError
}
