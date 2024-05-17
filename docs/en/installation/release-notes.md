# Release History

## 2.0.0

### Release Notes

- **Release Date:** January 12, 2024

**iLogtail 2.0 is a major architecture upgrade, and its configuration is not fully compatible with the 1.x series.** For a migration guide, please refer to the discussion thread [#1294](https://github.com/alibaba/ilogtail/discussions/1294).

**Key features in version 2.0:**

- **New V2 Collector Configuration Support** [#1185](https://github.com/alibaba/ilogtail/pull/1185)
- **Introduced SLS Processing Language (SPL) for efficient data processing** [#1278](https://github.com/alibaba/ilogtail/pull/1278)
- **Native plugin chaining support for C++ and C++ to Go plugins** [#1184](https://github.com/alibaba/ilogtail/pull/1184)
- **Introduced SPL for on-device data processing** [#1169](https://github.com/alibaba/ilogtail/pull/1169)
- **JSONLine output protocol support** [#1265](https://github.com/alibaba/ilogtail/pull/1165)
- **Env-based configuration management with 2.0 API** [#1282](https://github.com/alibaba/ilogtail/pull/1282)

**Improvements:**

- **Enhanced flusher_http plugin with queue caching and asynchronous interceptors** [#1203](https://github.com/alibaba/ilogtail/pull/1203)
- **goprofile plugin reports machine IP in data** [#1281](https://github.com/alibaba/ilogtail/pull/1281)
- **Improved VSCode development environment** [#1219](https://github.com/alibaba/ilogtail/pull/1219)
- **Kafka output plugin: added MaxOpenRequests option** [#1224](https://github.com/alibaba/ilogtail/pull/1224)
- **Plugin monitoring metrics now support labels** [#1240](https://github.com/alibaba/ilogtail/pull/1240)
- **Enhanced loki flusher to only output content** [#1256](https://github.com/alibaba/ilogtail/pull/1256)

**Bug fixes:**

- **Fixed issue with empty container IP in K8s Pod with HostNetwork** [#1280](https://github.com/alibaba/ilogtail/pull/1280)
- **Resolved libcurl crashes due to missing CURLOPT_NOSIGNAL setting** [#1283](https://github.com/alibaba/ilogtail/pull/1283)
- **Fixed field misalignment in logs with leading spaces in native delimiter parser** [#1289](https://github.com/alibaba/ilogtail/pull/1289)
- **Fixed timezone handling in native timeout log discard** [#1293](https://github.com/alibaba/ilogtail/pull/1293)
- **Resolved issue with JSON content key preservation in native JSON plugin** [#1296](https://github.com/alibaba/ilogtail/pull/1296)
- **Fixed memory leak in native delimiter plugin** [#1300](https://github.com/alibaba/ilogtail/pull/1300)
- **Resolved log duplication due to checkpoint dump before directory registration** [#1291](https://github.com/alibaba/ilogtail/pull/1291)
- **Fixed compatibility issue with comma-separated time format in FDT logs** [#1285](https://github.com/alibaba/ilogtail/pull/1285)
- **Improved handling of raw logs when native parsing fails** [#1304](https://github.com/alibaba/ilogtail/pull/1304)
- **Resolved parsing errors in native FDT parser in multi-threaded scenarios** [#1305](https://github.com/alibaba/ilogtail/pull/1305)

**For more details and source code, see:** [v2.0.0 release notes](https://github.com/alibaba/ilogtail/blob/main/changes/v2.0.0.md)

### Downloads

| Filename                                                                                              | OS     | Architecture | SHA256 Checksum                                                                                     |
| ------------------------------------------------------------------------------------------------------ | ------ | ------------ | ------------------------------------------------------------------------------------------------- |
| [ilogtail-2.0.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.0/ilogtail-2.0.0.linux-amd64.tar.gz) | Linux   | x86-64      | 0bcd191bc82f1e33d0d4a032ff2c9ea9e75de1dee04f11418107dde9d05b4185 |
| [ilogtail-2.0.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.0/ilogtail-2.0.0.linux-arm64.tar.gz) | Linux   | arm64       | fc825b4879fd1c00bcba94ed19a4484555ced1f9b778f78786bc3e2bfc9ebad8 |

### Docker Images

**To pull the Docker image:**

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:2.0.0
```

## 1.x Releases

[Release Notes (1.x series)](./release-notes-1.md)
