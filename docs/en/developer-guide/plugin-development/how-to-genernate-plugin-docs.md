# How to Generate Plugin Documentation

## Usage

Fields in your code marked with a `comment` tag, starting with an uppercase character, will be automatically extracted into the documentation. If a field also has a `json`, `yaml`, or `mapstructure` tag, the field name will be replaced with the name specified in that tag. Additionally, default values set in the init registration function will be included in the "default value" field.

```go
type TestDoc struct {
    Field1 int               `json:"field_1" comment:"field one"`
    Field2 string            `json:"field_2" comment:"field two"`
    Field3 int64             `json:"field_3" mapstructure:"field_33" comment:"field three"`
    Field4 []string          `json:"field_4" comment:"field four"`
    Field5 map[string]string `json:"field_5" comment:"field five"`
    ignoreField string
}

func (t TestDoc) Description() string {
    return "this is a test doc demo"
}
```

```markdown
# test-plugin

## Description

this is a test doc demo

## Configuration

| Field | Type | Description | Default Value |
| --- | --- | --- | --- |
| field_1 | int | field one | 1 |
| field_2 | string | field two | "filed 2" |
| field_33 | int64 | field three | 3 |
| field_4 | []string | field four | ["field","4"] |
| field_5 | map[string]string | field five | {"k":"v"} |
```

## Generating the Documentation

1. Run `make docs` to generate the plugin documentation. Note: If your plugin is only intended for a specific operating system, such as Linux, run the command in that environment, as the documentation will not be generated otherwise.

2. Since the documentation infrastructure is still under development, each generation will overwrite the `plugin-list.md` file containing the plugin list. Ensure that you only add your plugin to this list and commit only the plugin documentation files for your contributions.
