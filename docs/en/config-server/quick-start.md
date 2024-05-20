# Introduction

## Overview

The current observable data collection agents, such as iLogtail, primarily offer a local configuration management model. Managing multiple instances becomes complex when dealing with a large number of instances, as individual configurations need to be adjusted for each one. Additionally, in scenarios with multiple instances, there's a lack of unified monitoring for agent versions and operational statuses. To address these challenges, a global control service is needed to manage the collection configurations, agent versions, and operational states of these observable agents.

**ConfigServer** is a tool designed for this purpose, supporting the following features:

* Agent registration with ConfigServer
* Centralized management of agents in groups
* Remote batch configuration of agent collection configurations
* Monitoring of agent runtime states

## Terminology

* Data collection agent: A data collector, such as iLogtail or others.
* Collection configuration: A data pipeline, corresponding to a set of independent data collection configurations.
* AgentGroup: A grouping of agents with similar characteristics. Configurations are applied to agents within a group, and the default group is `default`.
* ConfigServer: The server-side component for managing collection configurations.

## Function Description

Provides global control for data collection agents. Any agent adhering to the protocol can be managed uniformly.

### Instance Registration

* Agents configure the deployed ConfigServer information.
* After starting, agents periodically send heartbeats to ConfigServer to prove their availability.
* They report information such as:
  * Unique instance_id
  * Version
  * Start time
  * Runtime status

### AgentGroup Management

* The basic unit of agent management is AgentGroup. Collection configurations are applied to agents in a group.
* The default group, `default`, is automatically created and all agents are added to it.
* Custom groups can be created using agent tags. Agents with matching tags join the group.
* An AgentGroup can contain multiple agents, and an agent can belong to multiple groups.

### Global Collection Configuration Management

* Collection configurations are set through the API on the server side, and then bound to an AgentGroup to apply to agents.
* Differences between configurations are tracked by version numbers, with incremental updates to agents.

### Status Monitoring

* Agents periodically send heartbeats to ConfigServer, reporting their status.
* ConfigServer consolidates and exposes this information through APIs.

## Running

ConfigServer consists of UI and Service components, which can run independently.

### Agent Configuration

Agents need to configure ConfigServer details to utilize the management features.

### iLogtail ConfigServer Configuration

Open the `ilogtail_config.json` file in the iLogtail directory and configure the ConfigServer parameters `ilogtail_configserver_address` and `ilogtail_tags`.

`ilogtail_configserver_address` is the address and port of the deployed ConfigServer, and can include multiple addresses. iLogtail will automatically switch to a connected ConfigServer. Note that the current ConfigServer is designed for single-server deployments, and multiple addresses do not support data synchronization. We've预留扩展性 for a distributed ConfigServer deployment, and contributions from the community are welcome.

`ilogtail_tags` are the tags for iLogtail in the ConfigServer. Although this parameter is currently unused, we've also reserved the possibility of using custom tags for agent grouping management.

Here's a simple configuration example:

```json
{
    ...
    "ilogtail_configserver_address": [
        "127.0.0.1:8899"
    ],
    ...
}
```

### Service

Service is a distributed structure that supports multiple deployments, responsible for communication with data collection agents and users/UI, providing the core management capabilities.

### Starting

Download the iLogtail source code, navigate to the ConfigServer backend directory, and compile and run the code:

```bash
cd config_server/service
go build -o ConfigServer
nohup ./ConfigServer > stdout.log 2> stderr.log &
```

### Configuration Options

The configuration file is `config_server/service/seeting/setting.json`. The following options can be configured:

* `ip`: Service IP address, defaults to `127.0.0.1`.
* `port`: Service port, defaults to `8899`.
* `store_mode`: Data persistence tool, defaults to `leveldb`. Currently, only single-machine persistence with leveldb is supported.
* `db_path`: Storage address or database connection string, defaults to `./DB`.
* `agent_update_interval`: Interval for batch writes of agent-reported data to storage, in seconds, default is `1` second.
* `config_sync_interval`: Interval for Service to sync collection configurations from storage, in seconds, default is `3` seconds.

Example configuration:

```json
{
    "ip": "127.0.0.1",
    "store_mode": "leveldb",
    "port": "8899",
    "db_path": "./DB",
    "agent_update_interval": 1,
    "config_sync_interval": 3
}
```

### UI

The UI is a web-based visualization interface that connects to the Service, facilitating agent management for users.

### Quick Start

```shell
git clone https://github.com/iLogtail/config-server-ui
cd config-server-ui
yarn install
yarn start
```

### More Information

For further details, please refer to [this documentation](https://github.com/iLogtail/config-server-ui/blob/master/README.md).
