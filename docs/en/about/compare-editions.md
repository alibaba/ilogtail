# Comparison between Community and Enterprise Editions

| **Category** | **Comparison Item** | **Community Edition** | **Enterprise Edition** |
| --- | --- | --- | --- |
| Deployment | Linux | Supported | Supported |
|      | Windows | Not supported | Supported |
|      | Kubernetes DaemonSet | Supported | Supported |
|      | Kubernetes Sidecar | Supported | Supported |
|      | Kubernetes Operator | Not supported | Supported |
| Data Collection | Log | Supported | Supported |
|      | Trace | Supported | Supported |
|      | Metrics | Supported | Supported |
|      | Data Processing | Supported | Supported |
| Performance | Single-core Collection Speed | Minimal 100M/s, Regular expressions 20M/s | Minimal 100M/s, Regular expressions 20M/s |
|      | Supported Configuration Count | 2000+ | 2000+ |
| Installation & Deployment | Automation Level | Manual or third-party tools | Native support from Alibaba Cloud |
|      | Third-party Agent Integration | Limited to data integration | Automated installation of third-party agents |
| Configuration Management | Configuration UI | Not supported | Supported |
|      | Local Configuration Management | Supported | Supported |
|      | Centralized Configuration Management | Not supported | Supported |
|      | SDK/API Management | Not supported | Supported |
| Reliability | Self-Monitoring | Supported | [CloudLens for SLS](https://help.aliyun.com/document_detail/425764.html) |
|      | CheckPoint | Supported | Supported |
|      | Log Context | Not supported | Supported |
|      | Exactly Once Write | Not supported | Supported (manual activation) |
| Advanced Control | Historical Data Import | Supported | Supported |
|      | Multi-tenant Isolation | Supported | Supported |
|      | Collection Degradation/Recovery | Supported | Supported |
|      | Automatic Installation/Upgrade | Not supported | Supported |
| Self-Observability | Runtime Monitoring | Local logs | Server-side visualization |
|      | Data Collection Statistics | Local logs | Server-side visualization |
|      | Error Messages | Local logs | Server-side visualization |
|      | Machine Group Heartbeat Monitoring | Not supported | Supported |
|      | Custom Alarms | Not supported | Supported |
| Services & Support | Documentation | Basic usage introduction | Comprehensive and up-to-date documentation for enterprise scenarios |
|      | Services | Community-driven | Prompt expert support (tickets, group support, and demand fulfillment). Professional support for large-scale or complex scenarios: e.g., short K8s jobs, single-node high-traffic logs, and thousands of log directories |
