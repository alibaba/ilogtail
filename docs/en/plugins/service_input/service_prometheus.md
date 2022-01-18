# service_prometheus
## Description
prometheus scrape plugin for logtail, use vmagent lib
## Config
|  field   |   type   |   description   | default value   |
| ---- | ---- | ---- | ---- |
|Yaml|string|the prometheus configuration content, more details please see [here](https://prometheus.io/docs/prometheus/latest/configuration/configuration/)|""|
|ConfigFilePath|string|the prometheus configuration path, and the param would be ignored when Yaml param is configured.|"prometheus_scrape_config_default.yml"|