# Decoder 注册器扩展

[decoder-register](https://github.com/alibaba/ilogtail/blob/main/plugins/extension/decoder_register/decoder_register.go) 扩展，严格来讲，并不是一个标准扩展，其只是一个可选的编译模块扩展，该扩展将内置的几个 [Decoder](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/decoder.go) 扩展的实现注册到工厂方法中，以便于外部插件可以引用内置的 Decoder。


该设计的考虑是，Decoder 在 iLogtail 中存在已久，且惯用的配置方式是 Format: <protocol> 这样的格式，为了保持这种配置风格，针对 Decoder 扩展定义了专用的 [注册方法](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/decoder.go)：
```go
func AddDecoderCreator(protocol string, creator DecoderCreator) {
    decoderFactories[protocol] = creator
}
```

因此，若想添加自定义的 `Decoder`，仅需要在自定义插件的 init 函数中，调用 `AddDecoderCreator` 注册即可。