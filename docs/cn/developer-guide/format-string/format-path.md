
# oss动态路径格式化

开发插件往`oss`写入数据时。用户往往存在一些动态路径的需求，动态路径和kafka的动态topic和elasticsearch的动态topic稍微有些区别。
动态路径常见的使用场景如下：
- 按照业务字段标识动态创建索引，和`Kafka`的动态`topic`一样。
- 基于时间的索引创建，按天、按周、按月等。在`ELK`社区例常见的案例：`%{+yyyy.MM.dd}`基于天创建，`%{+yyyy.ww}`基于周创建。
- 基于服务集群中机器hostname，区分多个机器下同个文件。

结合这几种需求，动态路径的创建规则如下：

- `%{content.fieldname}`。`content`代表从`contents`中取指定字段值
- `%{tag.fieldname}`,`tag`表示从`tags`中取指定字段值，例如：`%{tag.k8s.namespace.name}`
- `${env_name}`, 使用`$`符直接从环境变量中获取，从`1.5.0`版本支持
- `%{+yyyy.MM.dd}`,按天创建，注意表达式前面时间格式前面的`+`符号不能缺少。
- `%{+yyyy.ww}`，按周创建，还有其他时间格式就不一一举例。由于`golang`时间格式看起来不是非常人性化， 表达式使用上直接参考`ELK`社区的格式。
- `%{ilogtail.hostname}`，表示本机hostname

格式化动态索引的函数如下：
```go
func FormatPath(targetValues map[string]string, pathPattern string) (*string, error)
```
- `targetValues`使用Convert协议转换处理后需要替换的键值对。可参考`flusher_kafka_v2`中的对`ToByteStreamWithSelectedFields`的使用。
- `pathPattern`动态路径表达式。
