```
这是一份ilogtail插件中文文档的格式说明。

在完成文档后，以插件英文名命名文件，“_”改为“-”，
例如“metric_mock”插件的文档名为“metric-mock.md”，
然后在data-pipeline的overview.md里插入该插件，
所有的插件按英文名字典序升序排列，添加的时候注意插入的位置，
最后在SUMMARY.md里添加该插件的链接，所有的插件顺序与overview.md保持一致。

可用于参考的正式文档链接：
https://github.com/alibaba/ilogtail/blob/main/docs/cn/data-pipeline/input/service-journal.md
```
# (插件的中文名)

## 简介
`（插件名）` （描述）（源代码链接）


## 配置参数
| 参数 | 类型，默认值 | 说明 |
| - | - | - |
| Type | String，无默认值（必填） | 插件类型，固定为`（插件名）`。 |
|  | - | （若有参数则填写） |

```
说明：
1、所有的类型如下：
    Integer
    Long
    Boolean
    String
    Map（需注明key和value类型）
    Array（需注明value类型）
2、类型与默认值间以中文逗号分隔
    若无默认值则填写“无默认值（必填）”
    有默认值，在默认值外加上“``”
3、特殊值：
    空字符串：""
    空array：[]
    空map：{}
```

## 样例

* 输入
```
```

* 采集配置
```yaml
```

* 输出
```json
```

```
说明：
    1、在代码块附上json/yaml的标签可以更加美观
    2、根据具体插件差异，可以有多组样例，每组样例也并不一定要有输入。
```