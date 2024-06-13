# Testhub

## 行为列表

| 行为类型 | 模板 | 参数 | 说明 |
| --- | --- | --- | --- |
| Given | ^\{(\S+)\} environment$ | 环境类型 | 初始化远程测试环境 |
| Given | ^\{(\S+)\} config as below | 1. 配置名 2. 配置文件内容 | 添加本地配置 |
| When | ^generate \{(\d+)\} regex logs, with interval \{(\d+)\}ms$ | 1. 生成日志数量 2. 生成日志间隔 | 生成正则文本日志（路径为/tmp/ilogtail/regex_single.log） |
| When | add k8s label \{(.*)\} | k8s标签 | 为k8s资源添加标签 |
| When | remove k8s label \{(.*)\} | k8s标签 | 为k8s资源移除标签 |
| Then | there is \{(\d+)\} logs | 日志数量 | 验证日志数量 |
| Then | the log contents match regex single |  | 验证正则文本日志内容 |

