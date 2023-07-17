package command

const (
	pluginName              = "input_command"
	defaultContentType      = "PlainText"
	defaultIntervalMs       = 5000 // The default is Alibaba Cloud's collection frequency of 5s
	defaltExecScriptTimeOut = 3000 // Default 3 seconds timeout
	UserRoot                = "root"
	ContentTypeBase64       = "Base64"
	ContentTypePlainText    = "PlainText"
)

type ScriptMeta struct {
	scriptSuffix   string
	defaultCmdPath string
}

// ScriptTypeToSuffix Supported script types
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

// SupportContentType Supported content types
var SupportContentType = map[string]bool{
	ContentTypePlainText: true,
	ContentTypeBase64:    true,
}
