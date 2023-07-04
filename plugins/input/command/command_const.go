package command

const (
	pluginName              = "metric_command"
	defaultContentType      = "PlainText"
	defaultLineSplitSep     = "\n"
	defaultIntervalMs       = 5000 //ms 3默认为阿里云的采集频率5s
	defaltExecScriptTimeOut = 3000 //单位ms 设置为3秒超时
	ScriptMd5               = "script_md5"
	UserRoot                = "root"
	ContentTypeBase64       = "Base64"
	ContentTypePlainText    = "PlainText"
)

type ScriptMeta struct {
	scriptSuffix   string
	defaultCmdPath string
}

// 支持的脚本类型
var ScriptTypeToSuffix = map[string]ScriptMeta{
	"bash": {
		scriptSuffix:   "sh",
		defaultCmdPath: "/usr/bin/bash",
	},
	"shell": {
		scriptSuffix:   "sh",
		defaultCmdPath: "/usr/bin/sh",
	},
	"python2": {
		scriptSuffix:   "py",
		defaultCmdPath: "/usr/bin/python2",
	},
	"python3": {
		scriptSuffix:   "py",
		defaultCmdPath: "/usr/bin/python3",
	},
}

// 支持的contentType
var SupportContentType = map[string]bool{
	ContentTypePlainText: true,
	ContentTypeBase64:    true,
}
