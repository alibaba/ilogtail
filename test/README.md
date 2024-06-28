# Testhub

## 行为列表

| 行为类型 | 模板 | 参数 | 说明 |
| --- | --- | --- | --- |
| Given | ^\{(\S+)\} environment$ | 环境类型 | 初始化远程测试环境 |
| Given | ^iLogtail depends on containers \{(.*)\} | 容器 | iLogtail依赖容器，可多次执行，累积添加 |
| Given | ^iLogtail expose port \{(.*)\} to \{(.*)\} | 端口号 | iLogtail暴露端口，可多次执行，累积添加 |
| Given | ^\{(.*)\} local config as below | 1. 配置名 2. 配置文件内容 | 添加本地配置 |
| Given | ^\{(.*)\} http config as below | 1. 配置名 2. 配置文件内容 | 通过http添加配置 |
| Given | ^remove http config \{(.*)\} | 配置名 | 通过http移除配置 |
| Given | ^subcribe data from \{(\S+)\} with config | 1. 数据源 2. 配置文件内容 | 订阅数据源 |
| When | ^generate \{(\d+)\} regex logs, with interval \{(\d+)\}ms$ | 1. 生成日志数量 2. 生成日志间隔 | 生成正则文本日志（路径为/tmp/ilogtail/regex_single.log） |
| When | ^generate \{(\d+)\} http logs, with interval \{(\d+)\}ms, url: \{(.*)\}, method: \{(.*)\}, body: | 1. 生成日志数量 2. 生成日志间隔 3. url 4. method 5. body | 生成http日志，发送到iLogtail input_http_server |
| When | ^add k8s label \{(.*)\} | k8s标签 | 为k8s资源添加标签 |
| When | ^remove k8s label \{(.*)\} | k8s标签 | 为k8s资源移除标签 |
| When | ^start docker-compose dependencies \{(\S+)\} | 依赖服务 | 启动docker-compose依赖服务 |
| Then | ^there is \{(\d+)\} logs$ | 日志数量 | 验证日志数量 |
| Then | ^there is at least \{(\d+)\} logs$ | 日志数量 | 验证日志数量 |
| Then | ^there is at least \{(\d+)\} logs with filter key \{(.*)\} value \{(.*)\}$ | 1. 日志数量 2. 过滤键 3. 过滤值 | 验证日志数量 |
| Then | ^the log contents match regex single |  | 验证正则文本日志内容 |
| Then | ^the log fields match kv | kv键值对列表 | 验证kv日志内容 |
| Then | ^the log tags match kv | kv键值对列表 | 验证kv日志标签内容 |
| Then | ^the context of log is valid$ | | 验证日志上下文 |
| Then | ^the log fields match | 日志字段列表 | 验证日志字段内容 |
| Then | ^the log labels match | 日志标签列表 | 验证日志标签内容 |
| Then | ^the logtail log contains \{(\d+)\} times of \{(.*)\}$ | 1. 匹配次数 2. 匹配内容 | 验证日志内容 |
| Then | wait \{(\d+)\} seconds | 等待时间 | 等待时间 |
