# Development History

Adhering to the simplicity of Alibaba employees, the naming of iLogtail is straightforward. Initially, we aimed for a unified tool to Tail logs, hence Logtail. The "i" was added due to the use of inotify technology, enabling millisecond-level log collection latency. iLogtail was born in 2013, and its development can be broadly divided into four stages: Sky 5K, Alibaba Group, Cloud Native, and Open Source Collaboration.

![iLogtail Development History](<../.gitbook/assets/ilogtail-history.png>)

## Sky 5K Phase <a href="#4ever-bi-127" id="4ever-bi-127"></a>

As a milestone in China's cloud computing, on August 15, 2013, Alibaba Group officially launched a cluster with 5,000 (5K) servers, becoming the first Chinese company to independently develop a large-scale general-purpose computing platform and the world's first to offer 5K cloud computing services to the public. The Sky 5K project began in 2009, growing from 30 machines to 5,000, constantly addressing core issues such as scalability, stability, maintenance, and disaster recovery. iLogtail emerged during this phase, initially tasked with monitoring, problem analysis, and pinpointing issues for 5,000 machines (now referred to as "observability"). The leap from 30 to 5,000 posed numerous challenges for observability, including single-machine bottlenecks, problem complexity, ease of troubleshooting, and management complexity.

During the Sky 5K phase, iLogtail primarily addressed the challenges of monitoring and maintenance for single machines and small-scale clusters to large-scale environments. Key features included:

* **Functionality:** Real-time log and monitoring collection, with log retrieval delays in milliseconds
* **Performance:** Single-core processing capability of 10 MB/s, with an average resource usage of 0.5% CPU cores across 5,000 nodes
* **Reliability:** Automatic file monitoring, support for file rotation, and handling network disruptions
* **Management:** Remote web management, automatic configuration deployment
* **Maintenance:** Integrated into the group's yum repository, monitoring run state, and automatic issue reporting
* **Scale:** Deployed in over 30,000 instances, with thousands of collection configurations, processing 10 TB of data daily

## Alibaba Group Phase <a href="#4ever-bi-265" id="4ever-bi-265"></a>

iLogtail's application in the Alibaba Cloud's Sky 5K project addressed the unification of log and monitoring collection. However, the Alibaba Group and Ant Group lacked a unified, reliable log collection system at the time. This led to the adoption of iLogtail as the group's and Ant's log collection infrastructure. Transitioning from a relatively isolated project to a full-scale group application was not a simple replication; it involved managing a larger scale, higher requirements, and more departments:

1. **Million-scale maintenance:** With over a million physical and virtual machines across Alibaba and Ant, the goal was to maintain these with only one-third of the workforce.
2. **Higher stability:** Initially, iLogtail's collected data was mainly for problem-solving. As the group's applications expanded, the reliability of logs increased, particularly for billing and transaction data, and it needed to withstand the pressure of massive data traffic during events like Double 11 and Double 12.
3. **Multiple departments and teams:** iLogtail went from serving a 5K team to supporting hundreds of teams, each using different configurations, and a single instance could be used by multiple teams, presenting new challenges in tenant isolation.

After several years of collaboration, iLogtail made significant progress in multi-tenant and stability. Key features during this phase included:

* **Functionality:** Support for various log formats, such as regular expressions, separators, and JSON, with support for different log encoding methods and advanced processing like data filtering and anonymization
* **Performance:** Increased to 100 MB/s in minimal mode, and up to 20 MB/s for formats like regular expressions, separators, and JSON
* **Reliability:** Enhanced collection reliability with Polling, queue ordering, log cleanup protection, and CheckPoint improvements; added critical recovery and automatic crash reporting, as well as multi-level守护进程
* **Multi-tenancy:** Implemented full multi-tenant isolation, multi-level high/low watermarks, priority-based collection, configuration-level/instance-level traffic control, and temporary downgrading mechanisms
* **Maintenance:** Automated installation and monitoring through the group's StarAgent, with proactive notifications and self-diagnostic tools
* **Scale:** Deployed in millions, supporting thousands of internal tenants, with over 100,000 collection configurations, processing PB-level data daily

## Cloud Native Phase <a href="#4ever-bi-329" id="4ever-bi-329"></a>

As Alibaba's IT infrastructure fully embraced cloudification and iLogtail's parent product, SLS (Log Service), was commercialized on the Alibaba Cloud, iLogtail fully embraced cloud-native principles. Moving from internal use to serving businesses worldwide, the focus shifted from performance and reliability to adapting to cloud-native environments (containers, Kubernetes), compatibility with open-source protocols, and handling fragmented needs. This was the fastest-growing period for iLogtail, marked by significant changes:

* **Unified version:** iLogtail initially relied on GCC 4.1.2 and the Alibaba Sky Base. To support a wider range of environments, it underwent a complete rewrite, using a single codebase for Windows/Linux, x86/ARM, server/embedded, and other platforms

* **Full support for containerization and Kubernetes:** In addition to collecting logs and monitoring in container and Kubernetes environments, it enhanced configuration management, allowing users to extend with an Operator by configuring an AliyunLogConfig Kubernetes custom resource

* **Plugin-based expansion:** iLogtail introduced a plugin system, enabling custom functionality through Input, Processor, Aggregator, and Flusher plugins

* **Scale:** Deployed in the millions, serving thousands of external clients, with millions of collection configurations, processing tens of PB of data daily

## Open Source Collaboration Phase

A closed-source software can never keep pace with the times, especially in the cloud-native era. iLogtail, as a foundational software in the observability domain, was open-sourced to foster collaboration with the community and strive for excellence. As a world-class log collection tool, our vision for iLogtail's future includes:

1. **Performance and resource efficiency:** iLogtail aims to outperform other open-source collectors in terms of performance and resource usage, reducing memory consumption by 100 TB and CPU core hours by 10 million annually at the scale of millions of deployments and daily data processing in the tens of PB range.

2. **Enriching the ecosystem:** While currently used mainly within Alibaba and a few cloud companies, iLogtail seeks to expand its reach to more industries and companies, encouraging them to request new data sources, processing, and output targets, thereby diversifying the upstream and downstream ecosystem.

3. **Continual improvement:** Performance and stability are fundamental, and we hope to attract more talented developers through the open-source community to enhance iLogtail and maintain its status as a top-notch observability data collector.
