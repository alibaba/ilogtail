# Plugin Documentation Template

This is a template and format guide for the Chinese documentation of the ilogtail plugin.

## Format Instructions

### Main Body

#### File Naming

Name the document using the plugin's English name, replace `_` with `-`, e.g., for the "metric_mock" plugin, the document would be named `metric-mock.md` and saved in the `docs/cn/data-pipline` folder in the corresponding directory.

#### Title Section

The title should be the plugin's Chinese name.

#### Introduction Section

Include the plugin's English name and a brief description. End the introduction with a link to the source code.

#### Configuration Parameters Section

Fill in the parameter list, following the template below. Some guidelines to consider:

1. All types are as follows:
   * Integer
   * Long
   * Boolean
   * String
   * Map (key and value types must be specified)
   * Array (value type must be specified)

2. Separate type and default value with a Chinese comma. If no default value, write "无默认值（必填）"; if there is a default value, include it after the default value.

3. Special values:
   * Empty string: `""`
   * Empty array: `[]`
   * Empty map: `{}`

#### Examples Section

Examples consist of input, collection configuration, and output parts.

1. Add tags like `bash`, `json`, or `yaml` to code blocks for better formatting.

2. Depending on the plugin's specifics, there can be multiple examples, and each example may not necessarily have an input.

### References

Refer to the `service_journal` plugin documentation: [service-journal.md](https://github.com/alibaba/ilogtail/blob/main/docs/cn/plugins/input/service-journal.md) for more information.

### Summary Page

After completing the documentation, update `docs/cn/data-pipline/overview.md` and `docs/cn/SUMMARY.md`.

1. In `overview.md`, all plugins are listed in ascending order of their English names. Be careful about the insertion position.

2. The order of plugins in `SUMMARY.md` should match `overview.md`, and include links.

## Document Template

The template looks like this:

```markdown
# (pugin name)

## Introduction

`(plugin name)`-(description)-[source code]

## Configuration Parameters

| Parameter | Type, Default | Description |
| - | - | - |
| Type | String, No Default | plugin type`(plugin name)`。 |
|  | - | (if needed) |

## Examples

* Input

\```bash
\```

* Config

\```yaml
\```

* Output

\```json
\```
```
