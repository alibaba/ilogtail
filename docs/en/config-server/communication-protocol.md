# Communication protocol

Jointly built by iLogtail community and beilibili. It follows the HTTP/HTTPS protocol.

## Common parameters

The ip address and port are ConfigServer ip addresses and receiving ports. The default values are 127.0.0.1 and 8899.

## API response specifications

Data is sent and received in protocol buffer format. For more information, see [config_server/protocol](https://github.com/alibaba/ilogtail/tree/main/config_server/protocol) folder (v1.0）。

If the HTTP status code 200 is returned, the request is successful. If the HTTP status code 404 is returned, the call to the API fails. The server returns the following response information. code is the response code, indicating the type of the request success/failure;message is the response information, which contains detailed request success/failure information.

```Json
{
  "code" : <Code>,
"Message": <Message>
}
```

The main response codes are as follows:

| HTTP status code | Response code | Description |
|-|-|-|
| 200 | Accept | Request succeeded |
| 400 | INVALID_PARAMETER | Invalid parameter |
| 500 | INTERNAL_SERVER_ERROR | Internal service call error |

## Common data types in communication

### Agent

The information of the collection Agent in communication transmission.

| Parameter | Type | Description |
| - | - | - |
| agent_type | string | Agent's type |
| agent_id | string | The unique identifier of the Agent. |
| attributes | AgentAttributes | Agent's attributes |
| tags | string[] | Agent's tags |
| running_status | string | The running status of the Agent. |
| startup_time | int64 | Agent's startup time |
| interval | int32 | Heartbeat interval of the Agent |

### AgentAttributes

| Parameter | Type | Description |
| - | - | - |
| version | string | Agent's version |
| category | string | The type of the Agent (used to pull AGENT_CONFIG) |
| ip | string | Agent's ip |
| hostname | string | The hostname of the Agent. |
| region | string | Agent's region |
| zone | string | Agent zone |
| extras | map<string,string> | Agent extra attributes |

### AgentGroup

AgentGroup information in communication transmission.

| Parameter | Type | Description |
| - | - | - |
| group_name | string | AgentGroup's name |
| description | string | AgentGroup's description |
| tags | AgentGroupTag[] | AgentGroup's tags |

### Agent group tag

The Tag information of the AgentGroup in the communication transmission.

| Parameter | Type | Description |
| - | - | - |
| name | string | AgentGroupTag's name |
| value | string | AgentGroupTag's value |

### CheckStatus (enumeration type)

Pull the update status returned when the configuration file is updated.

| Value | Description |
| - | - |
| NEW | NEW configuration file |
| DELETED | DELETED configuration file |
| MODIFIED | MODIFIED configuration file |

### Command

The command that the Agent receives from the ConfigServer.

| Parameter | Type | Description |
| - | - | - |
| type | string | The type of the instruction. |
| name | string | Instruction name |
| id | string | The number of the instruction. |
| args | map<string,string> | Parameters of the instruction |

### ConfigCheckResult

ConfigServer the configuration file update information returned to the Agent.

| Parameter | Type | Description |
| - | - | - |
| type | ConfigType | Config's type |
| name | string | Unique identifier of Config |
| old_version | int64 | Local version of Config |
| new_version | int64 | Latest version of Config |
| context | string | Config context |
| check_status | CheckStatus | Config's status |

### ConfigDetail

The information about the collection Config in communication transmission.

| Parameter | Type | Description |
| - | - | - |
| type | ConfigType | Config's type |
| name | string | Unique identifier of Config |
| version | int64 | Config version |
| context | string | Config context |
| detail | string | Config details |

### ConfigInfo

The configuration file information that the Agent carries when requesting the ConfigServer.

| Parameter | Type | Description |
| - | - | - |
| type | ConfigType | Config's type |
| name | string | Unique identifier of Config |
| version | int64 | Config version |
| context | string | Config context |

### ConfigType (enumeration type)

The type of the configuration file.

| Value | Description |
| - | - |
| PIPELINE_CONFIG | collection pipeline config |
| AGENT_CONFIG | Agent running configuration |

### RespCode (enumeration type)

The status code returned during communication.

| Value | Description |
| - | - |
| ACCEPT | The request is successful. |
| INVALID_PARAMETER | Request failed with incorrect parameters |
| INTERNAL_SERVER_ERROR | Request failed, service internal error |

## Interface Description

Provides unified external control in the form of APIs. There are two categories:

* User API
* Create, modify, delete, and view AgentGroup
* Create, modify, delete, and View collection configurations
* Bind or unbind the collection configuration from the AgentGroup.
* Agent API
* Agent heartbeat
* Obtain the collection configuration

### User API: create, modify, delete, and view AgentGroup

#### `ip:port/User/CreateAgentGroup`

* Function: Create a AgentGroup
* Request method: POST
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| agent_group | AgentGoup, no default value (required) | Information about the newly created AgentGroup |

* Return value: None

#### `ip:port/User/UpdateAgentGroup`

* Function: modify the information of an existing AgentGroup.
* Request Method: PUT
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| agent_group | AgentGoup, no default value (required) | Information about the modified AgentGroup |

* Return value: None

#### `ip:port/User/DeleteAgentGroup/`

* Function: delete an existing AgentGroup
* Request method: DELETE
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | The name of the AgentGroup to be deleted |

* Return value: None

#### `IP: Port/user/GetAgent Group/`

* Function: View AgentGroup information
* Request method: GET
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | The name of the AgentGroup to be obtained |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| agent_group | AgentGroup | info of target AgentGroup |

#### `IP: Port/user/listangent groups/`

* Function: obtains a list of all AgentGroup.
* Request method: GET
* Parameter: None
* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| agent_groups | AgentGroup[] | An array containing all AgentGroup information |

### User API: create, modify, delete, and View collection configurations

#### `ip:port/User/CreateConfig/`

* Function: import local Config
* Request method: POST
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| config_detail | ConfigDetail, no default value (required) | Information about the new Config |

* Return value: None

#### `ip:port/User/UpdateConfig/`

* Function: modify the information of the existing Config.
* Request Method: PUT
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| config_detail | ConfigDetail, no default value (required) | Information about the modified Config |

* Return value: None

#### `ip:port/User/DeleteConfig/`

* Function: delete the saved Config
* Request method: DELETE
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| config_name | string, no default value (required) | The name of the Config to be deleted |

* Return value: None

#### `ip:port/User/GetConfig/`

* Function: view the information of the specified Config.
* Request method: GET
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| config_name | string, no default value (required) | The name of the config to be obtained |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| config_detail | ConfigDetail | Target Config's information |

#### `ip:port/User/ListConfigs/`

* Function: obtains the list of all Config items.
* Request method: GET
* Parameter: None
* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| config_details | ConfigDetail[] | An array containing all Config information |

### User API: bind or unbind the collection configuration from the AgentGroup

#### `ip:port/User/ApplyConfigToAgentGroup/`

* Function: add Config to the AgentGroup
* Request Method: PUT
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | Name of the target AgentGroup |
| config_name | string, no default value (required) | Name of the target Config |

* Return value: None

#### `ip:port/User/RemoveConfigFromAgentGroup/`

* Function: delete the Config in the AgentGroup.
* Request method: DELETE
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | Name of the target AgentGroup |
| config_name | string, no default value (required) | Name of the target Config |

* Return value: None

#### `IP: Port/user/GetAppliedConfigsForAgentGroup/`

* Function: view the Config list in the AgentGroup.
* Request method: GET
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | Name of the target AgentGroup |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| config_names | string[] | An array of all Config names associated with the AgentGroup |

#### `IP: Port/user/GetAppliedAgentGroups/`

* Function: view the list of AgentGroup associated with Config.
* Request method: GET
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| config_name | string, no default value (required) | Target Config name |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| agent_group_names | string[] | An array of all AgentGroup names associated with Config |

#### `Ip: port/User/ListAgents/`

* Function: obtains the list of agents in the AgentGroup.
* Request method: GET
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| group_name | string, no default value (required) | Name of the target AgentGroup |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| agents | Agent[] | An array containing all Agent information and their statuses in the AgentGroup |

### Agent API: write heartbeat

#### `ip:port/Agent/HeartBeat/`

* Function: the Agent sends a heartbeat to the ConfigServer.
* Request method: POST
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| agent_id | string, no default value (required) | Unique identifier of the Agent |
| agent_type | string, no default value (required) | Agent type |
| attributes | AgentAttributes, no default value (required) | Agent information |
| tags | string[], no default value (required) | tags owned by the Agent |
| running_status | string, no default value (required) | Agent running status |
| startup_time | int64, no default value (required) | Agent startup time |
| interval | int32, no default value (required) | Heartbeat interval of the Agent |
| pipeline_configs | ConfigInfo, no default value (required) | Collection configuration information owned by the Agent |
| agent_configs | ConfigInfo, no default value (required) | The running configuration information owned by the Agent |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| pipeline_check_results | ConfigCheckResult[] | Collection configuration update information pulled by the Agent |
| agent_check_results | ConfigCheckResult[] | Update information about the running configuration pulled by the Agent |
| custom_commands | Command[] | Commands received by the Agent |

### Agent API: obtain the collection configuration (including change information) based on the Agent

#### `IP: Port/agent/fetch pipeline config/`

* Function: the Agent sends a list of existing collection configurations to the ConfigServer to obtain updates.
* Request method: POST
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| agent_id | string, no default value (required) | Unique identifier of the Agent |
| req_configs | ConfigInfo[] | Configuration file information to be pulled by the Agent |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| config_details | ConfigDetail[] | Complete information about the configuration file pulled by the Agent |

#### `ip:port/Agent/FetchAgentConfig/`

* Function: the Agent sends a list of existing running configurations to the ConfigServer to obtain updates.
* Request method: POST
* Parameters:

| Parameter | Type, default value | Description |
| --- | --- | --- |
| agent_id | string, no default value (required) | Unique identifier of the Agent |
| attributes | AgentAttributes, no default value (required) | Agent information |
| req_configs | ConfigInfo[] | Configuration file information to be pulled by the Agent |

* Return value:

| Parameter | Type | Description |
| --- | --- | --- |
| config_details | ConfigDetail[] | Complete information about the configuration file pulled by the Agent |
