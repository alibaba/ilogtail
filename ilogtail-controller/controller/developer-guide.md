# ilogtail-controller 开发指南

## 后端

后端的目录树如下：

```plain
.
|____ilogtail_controller.go
|____go.mod
|____developer-guide.md
|____go.sum
|____controller
|____README.md
|____manager
| |____manager.md
| |____manager.go
| |____structure
| | |____config.go
| | |____machine_group.go
| | |____machine.go
| | |____config_group.go
| |____store
| | |____store.go
| | |____base_set.go
| | |____store_json.go
| | |____store_leveldb.go
|____service
| |____config.go
| |____message.go
| |____machine.go
|____base_setting.json
|____controller.log
|____tool
| |____queue.go
| |____dichotomous_lookup.go
| |____json.go
| |____is_contain.go
```
