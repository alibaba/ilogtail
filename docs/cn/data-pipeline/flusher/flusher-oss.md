# OSS

## 简介

`flusher_oss` `flusher`插件可以实现将采集到的数据，经过处理后，以append方式发送到 OSS中。

## 版本

[Alpha](../stability-level.md)

## 配置参数

| 参数              | 类型       | 是否必选 | 说明                                                          |
| --------------- | -------- | ---- | ----------------------------------------------------------- |
| Bucket            | String   | 是    | Oss Bucket名                                                |
| Endpoint            | String   | 是    | Oss接口自定义Endpoint。            |
| KeyFormat           | String   | 是    | Object key格式，支持动态变量。多节点下需要保证每次上传文件名不冲突。                                               |
| ContentKey            | String   | 是    | 日志数据字段名。   |
| Encoding            | String   | 否    | 声明Object的编码方式。<br>identity（默认值）：表示Object未经过压缩或编码。<br>gzip：表示Object采用Lempel-Ziv（LZ77）压缩算法以及32位CRC校验的编码方式。<br>compress：表示Object采用Lempel-Ziv-Welch（LZW）压缩算法的编码方式。<br>deflate：表示Object采用zlib结构和deflate压缩算法的编码方式。<br>br：表示Object采用Brotli算法的编码方式。 |
| ObjectAcl            | String   | 否    | 指定OSS创建Object时的访问权限。<br>default（默认）：Object遵循所在存储空间的访问权限。<br>private：Object是私有资源。只有Object的拥有者和授权用户有该Object的读写权限，其他用户没有权限操作该Object。<br>public-read：Object是公共读资源。只有Object的拥有者和授权用户有该Object的读写权限，其他用户只有该Object的读权限。<br>public-read-write：Object是公共读写资源。所有用户都有该Object的读写权限。                       |
| ObjectStorageClass            | String   | 否    | 指定Object的存储类型。<br>Standard：标准存储（默认）<br>IA：低频访问<br>Archive：归档存储<br>ColdArchive：冷归档存储<br>DeepColdArchive：极冷归档存储                                                        |
| Tagging            | String   | 否    | 以键值对（Key-Value）的形式指定Object的标签信息，可同时设置多个标签，例如TagA=A&TagB=B。   |
| MaximumFileSize           | Int   | 否    | 文件上传的大小上限(MB)，当超过后会将之前的文件重命名，并上传新的文件，默认大小4096   |
| Convert                           | Struct   | 否    | ilogtail数据转换协议配置                                                                   |
| Convert.Protocol                  | String   | 否    | ilogtail数据转换协议，kafka flusher 可选值：`custom_single`,`otlp_log_v1`。默认值：`custom_single` |
| Convert.Encoding                  | String   | 否    | ilogtail flusher数据转换编码，可选值：`json`、`none`、`protobuf`，默认值：`json`                     |
| Convert.TagFieldsRename           | Map      | 否    | 对日志中tags中的json字段重命名                                                                |
| Convert.ProtocolFieldsRename      | Map      | 否    | ilogtail日志协议字段重命名，可当前可重命名的字段：`contents`,`tags`和`time`                              |
| Authentication                    | Struct   | 否    | OSS 连接访问认证配置，优先会读取环境变量中OSS_ACCESS_KEY_ID、OSS_ACCESS_KEY_SECRET变量值，若没有才会读取 Authentication结构下的ak和sk                                                               |
| Authentication.PlainText.AccessKeyId | String   | 否    | OSS AccessKeyId                                                                     |
| Authentication.PlainText.AccessKeySecret | String   | 否    | OSS AccessKeySecret                                                                      |


## 样例

采集`/var/log/`路径下的文件名匹配`messages`规则的文件，并将采集结果发送到 OSS

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /var/log
    FilePattern: "messages"
flushers:
  - Type: flusher_oss
    Endpoint: oss-cn-hangzhou.aliyuncs.com
    Bucket: jinchen-test
    KeyFormat: ilogtail2/%{ilogtail.hostname}/%{+yyyy.MM.dd}/var/log/%{content.filename}
    ContentKey: message
```
