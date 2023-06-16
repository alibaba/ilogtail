package command

const (
	pluginName              = "metric_command"
	defaultContent          = `echo -e "__labels__:hostname#\$#idc_cluster_env_name|ip#\$#ip_address    __value__:0  __name__:metric_command_example"`
	defaultScriptType       = "Bash"
	defaultCmdPath          = "/usr/bin/sh"
	defaultUser             = "nobody"
	defaultContentType      = "PlainText"
	defaultLineSplitSep     = `\n`
	defaultOutputDataType   = SlsMetricDataType
	defaultScirptDataDir    = "/workspaces/ilogtail/scriptStorage/"
	defaultIntervalMs       = 5000     //ms 默认5s
	defaltExecScriptTimeOut = 3000      //单位ms 设置为3秒超时
	defaultExporterName     = "default" //默认为default 提交到coommonlabels中
)

//执行脚本输出的格式
const (
	SlsMetricDataType = "sls_metrics"
)

//支持的脚本类型
var supportScriptTypes = map[string]bool{
	"Bash":  true,
	"Shell": true,
	// "python": false,
}

//支持的脚本输出类型
var supportOutPutDataType = map[string]bool{
	SlsMetricDataType: true,
}

//支持的contentType
var supportContentType = map[string]bool{
	"PlainText": true,
	"Base64":    true,
}
