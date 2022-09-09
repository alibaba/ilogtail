# 单条协议

单条协议在内存中对应的数据结构为：

```Go
type SingleLog struct {
    time     uint32
    contents map[string]string
    tags     map[string]string
}
```
