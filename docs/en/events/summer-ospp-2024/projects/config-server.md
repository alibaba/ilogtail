# ConfigServer Capability Upgrade + UX Optimization (Full Stack)

## Project Link

[https://summer-ospp.ac.cn/org/prodetail/246eb0236?lang=zh&list=pro](https://summer-ospp.ac.cn/org/prodetail/246eb0236?lang=zh&list=pro)

## Project Description

### iLogtail Overview

iLogtail, designed for observability scenarios, boasts lightweight, high performance, and automated configuration features, widely used in Alibaba and thousands of Alibaba Cloud clients. Deploy it on physical machines, virtual machines, or Kubernetes to collect observable data like logs, traces, and metrics. iLogtail is actively contributed to by colleagues from Alibaba Cloud, Mogu Documents, Tongcheng Travel, Xiaohongshu, Toutiao, Bilibili, and Didi Chuxing. The iLogtail code repository: [https://github.com/alibaba/ilogtail](https://github.com/alibaba/ilogtail)

### ConfigServer Overview

Current observability collection agents, like the open-source iLogtail, primarily rely on a local configuration management model, which becomes cumbersome when managing multiple instances. Additionally,缺乏 unified monitoring for agent versions and statuses in multi-instance scenarios prompted the development of ConfigServer.

ConfigServer is a tool for managing observability collection agents, currently supporting:

* Agent registration with ConfigServer
* Centralized management of agents in groups
* Remote batch configuration of agent collection settings
* Monitoring agent runtime status
* A management UI

### Project Requirements

ConfigServer's current deployment is inflexible and lacks comprehensive control. This project aims to enhance the user experience through full-stack development, meeting the following requirements:

* Capability Upgrade:
  * Support for managing multiple agent groups
  * Implement agent alert reporting
* Deployment Upgrade:
  * Enable containerized deployment and out-of-the-box setup
  * Support for distributed deployment
* UX Optimization:
  * Upgrade the ConfigServer UI for multi-agent groups and agent alerts
  * Integrate agent monitoring and self-monitoring capabilities
* Documentation:
  * Write documentation for ConfigServer usage and development

## Mentor

Xuan Yang (<yangzehua.yzh@alibaba-inc.com>)

## Difficulty Level

Advanced

## Technical Tags

* React
* MySQL
* Redis
* Docker

## Programming Language Tags

* Go
* JavaScript
* C++

## Project Output Requirements

1. Capability Upgrade
    * ConfigServer supports multi-agent group management
    * ConfigServer supports agent alert reporting
2. Deployment Upgrade
    * Implement distributed deployment using ConfigServer's data storage layer API
    * Enable containerized deployment for ConfigServer
3. UX Optimization
    * Improve ConfigServer UI for multi-agent groups and agent alerts
    * Provide monitoring capabilities for ConfigServer and agent statuses
4. Documentation
    * Write documentation for ConfigServer usage and development

## Technical Requirements

1. Proficient in Golang, familiar with React or other frontend frameworks, and knowledge of C++
2. Experience with distributed application development
3. Containerization technology expertise
4. Git and GitHub operations
5. Experience with Alibaba Cloud Log Service is a plus

## References

* ConfigServer Introduction: [https://ilogtail.gitbook.io/ilogtail-docs/config-server/quick-start](https://ilogtail.gitbook.io/ilogtail-docs/config-server/quick-start)
