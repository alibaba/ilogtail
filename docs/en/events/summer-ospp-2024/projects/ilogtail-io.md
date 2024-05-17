# iLogtail Performance Optimization for Data Throughput

## Project Links

<https://summer-ospp.ac.cn/org/prodetail/246eb0231?lang=zh&list=pro>

## Project Description

### iLogtail Overview

iLogtail, designed for observability scenarios, boasts lightweight, high-performance, and automated configuration features. It is widely used within Alibaba and thousands of Alibaba Cloud clients. Deployable on physical machines, virtual machines, Kubernetes, and more, iLogtail collects observable data such as logs, traces, and metrics. The community is actively built by colleagues from Alibaba Cloud, Tuniu, Xiaohongshu, Bytedance, Bilibili, Didi, and more. The iLogtail codebase: <https://github.com/alibaba/ilogtail>

### iLogtail Data Collection Introduction

As an observability data collector provided by Alibaba Cloud SLS, iLogtail can run in various environments like servers, containers, Kubernetes, and embedded, supporting hundreds of observable data sources (logs, monitoring, traces, events, etc.). With millions of installations, we've introduced a Golang-based data接入 system to iLogtail to facilitate integration with the open-source ecosystem and meet diverse connectivity demands. The system now supports data sources like Kafka, OTLP, HTTP, MySQL Query, and MySQL Binlog, among others.

### Specific Requirements for the Project

While the Golang implementation of data intake, processing, and transmission lacks the same efficiency as C++, in iLogtail 2.0, we've implemented a C++ version of the data processing pipeline. The goal is to upgrade some high-frequency data sources' output, parsing, and output modules from the Golang version to C++ to enhance iLogtail's data throughput in these scenarios.

The project aims to achieve the following in the C++ version:

* Improved performance:
  * Higher read/sending rates for Syslog input and Kafka output compared to the pre-upgraded Golang version.
  * Nginx parsing speed should surpass the C++ regex-based approach.

* Memory optimization: Same processing rate with lower memory consumption than the Golang version.

* Data tag management: iLogtail's internal tags need to be reconciled with input/output source tags.

* Data integrity: Ensuring "at least once" semantics, preventing data loss.

## Project Mentor

Aping (<<bingchang.cbc@alibaba-inc.com>>)

## Project Difficulty

Advanced

## Technical Areas Tag

Kafka, TCP/IP, DataOps

## Programming Languages Tag

C++, Go

## Project Output Requirements

1. Implement C++ versions of Syslog input, Nginx parsing, and Kafka output modules.
2. Provide comprehensive documentation and usage cases for the implemented modules.
3. Add comprehensive testing for the implemented modules.
4. Write a performance comparison test report between the implemented C++ modules and the Golang version.

## Project Technical Requirements

1. Proficiency in C++ and understanding of Golang.
2. Knowledge of Kafka client API and cluster architecture.
3. Familiarity with syslog protocol and related library functions.
4. Experience with Git and GitHub operations.
5. Experience in developing high-performance network services/plugins is a plus.

## References

* What is iLogtail: <https://ilogtail.gitbook.io/ilogtail-docs>
* Hello, iLogtail 2.0!: <https://developer.aliyun.com/article/1441630>
