# How to generate plugin doc?

## Usage

The upper case with `comment` tag (exported) field names would be auto extracted. When some field tags with `json`, `yaml`
or `mapstructure`, the original name would be override by the previous sequence.

``` go
type TestDoc struct {
	Field1      int    `json:"field_1" comment:"field one"`
	Field2      string `json:"field_2" comment:"field two"`
	Field3      int64  `json:"field_3" mapstructure:"field_4" comment:"field three"`
	ignoreField string
}

func (t TestDoc) Description() string {
	return "this is a test doc demo"
}
```

``` markdown
# test-plugin
## Description
this is a test doc demo
## Config
|  field   |   type   |   description   |
| ---- | ---- | ---- |
|field_1|int|field one|
|field_2|string|field two|
|field_4|int64|field three|
```



## Register to the doc center


## Commands

1. execute `make docs` to generate plugin docs. Notice: Please run the commands under the OS if your contribute plugin
   only supports the specific OS, such as linux.
2. Because history documentation needs to be replenished constantly，you should purpose only append your plugins to `plugin-list.md` and only your plugin doc is tracked by git.


