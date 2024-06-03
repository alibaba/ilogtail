# 统一管控协议

本规范定义了 Agent 管控网络协议以及 iLogtail 和 Config Server 的预期行为。

1. 只要XxxConfigServer实现了协议，那么就可以管控Agent做Yyy事情。
2. 只要Agent实现了协议，那么任何XxxConfigServer就能过管控该Agent做Yyy事情。

## 管控协议

/Agent/GetAgentConfig?InstanceId=$instance\_id&WaitForChange=(true|false)

### HeartBeatRequest 消息

    message HeartBeatRequest {
        string request_id = 1;
        uint64 sequence_num = 2;                   // Increment every request, for server to check sync status
        uint64 capabilities = 3;                   // Bitmask of flags defined by AgentCapabilities enum
        string instance_id = 4;                     // Required, Agent's unique identification, consistent throughout the process lifecycle
        string agent_type = 5;                      // Required, Agent's type(ilogtail, ..)
        AgentAttributes attributes = 6;             // Agent's basic attributes
        repeated AgentGroupTag tags =  7;           // Agent's tags
        string running_status = 8;                  // Agent's running status
        int64 startup_time = 9;                     // Required, Agent's startup time
        repeated ConfigInfo pipeline_configs = 10;   // Information about the current PIPELINE_CONFIG held by the Agent
        repeated ConfigInfo process_configs = 11;     // Information about the current AGENT_CONFIG held by the Agent
        repeated CommandInfo custom_commands = 12;  // Information about command history
        uint64 flags = 13;                          // Predefined command flag
        // 14-99 reserved for future official fields
    }
    
    message AgentGroupTag {
        string name = 1;
        string value = 2;
    }

    enum ConfigStatus {
        // The value of status field is not set.
        UNSET = 0;
        // Agent is currently applying the remote config that it received earlier.
        APPLYING = 1;
        // Remote config was successfully applied by the Agent.
        APPLIED = 2;
        // Agent tried to apply the config received earlier, but it failed.
        // See error_message for more details.
        FAILED = 3;
    }

    // Define the Config information carried in the request
    message ConfigInfo {
        string name = 1;         // Required, Config's unique identification
        int64 version = 2;       // Required, Config's version number or hash code
        ConfigStatus status = 3; // Config's status
    }

    // Define the Command information carried in the request
    message CommandInfo {
        string type = 1;         // Command's type
        string name = 2;         // Required, Command's unique identification
        ConfigStatus status = 3; // Command's status
    }

    // Define Agent's basic attributes
    message AgentAttributes {
        string version = 1;                 // Agent's version
        string ip = 2;                      // Agent's ip
        string hostname = 3;                // Agent's hostname
        // 4-99 reserved for future official fields
        map<string, string> extras = 100;   // Agent's other attributes
    }

    enum AgentCapabilities {
        // The capabilities field is unspecified.
        UnspecifiedAgentCapability = 0;
        // The Agent can accept pipeline configuration from the Server.
        AcceptsPipelineConfig          = 0x00000001;
        // The Agent can accept process configuration from the Server.
        AcceptsProcessConfig           = 0x00000002;
        // The Agent can accept custom command from the Server.
        AcceptsCustomCommand           = 0x00000004;

        // Add new capabilities here, continuing with the least significant unused bit.
    }

    enum RequestFlags {
        RequestFlagsUnspecified = 0;

        // Flags is a bit mask. Values below define individual bits.
    }

### HeartBeatResponse 消息

    message HeartBeatResponse {
        string request_id = 1;  
        int32 code = 2;      
        string message = 3;     

        repeated ConfigDetail pipeline_config_updates = 4;  // Agent's pipeline config update status
        repeated ConfigDetail process_config_updates = 5;   // Agent's process config update status
        repeated CommandDetail custom_command_updates = 6;  // Agent's commands updates
    }
    
    message ConfigDetail {
        string name = 1;        // Required, Config's unique identification
        int64 version = 2;      // Required, Config's version number
        string detail = 3;      // Required, Config's detail
        string context = 4;     // Config's context
    }

    message CommandDetail {
        string type = 1;                // Required, Command type
        string name = 2;                // Required, Command name
        map<string, string> args = 3;   // Command's parameter arrays
        int64 expire_time = 4;          // After which the command can be safely removed from history
        string context = 5;     // Command's context
    }

    enum ServerCapabilities {
        // The capabilities field is unspecified.
        UnspecifiedServerCapability = 0;
        // The Server can remember agent attributes.
        RembersAttribute                   = 0x00000001;
        // The Server can remember pipeline config status.
        RembersPipelineConfigStatus        = 0x00000002;
        // The Server can remember process config status.
        RembersProcessConfigStatus         = 0x00000004;
        // The Server can remember custom command status.
        RembersCustomCommandStatus         = 0x00000008;

        // Add new capabilities here, continuing with the least significant unused bit.
    }

    enum ResponseFlags {
        ResponseFlagsUnspecified = 0;

        // Flags is a bit mask. Values below define individual bits.

        // ReportFullState flag can be used by the Server if the Client did not include
        // some sub-message in the last AgentToServer message (which is an allowed
        // optimization) but the Server detects that it does not have it (e.g. was
        // restarted and lost state).
        ReportFullState           = 0x00000001;
        FetchPipelineConfigDetail = 0x00000002;
        FetchProcessConfigDetail  = 0x00000004;
    }

## 行为规范

对于管控协议来说 iLogtail 的预期行为是确定性的，对于实现本管控协议的其他 Agent 其具体行为可自行确定，但语义应保持一致。Server 端定义了可选的行为的不同实现，此时对于这些差异 Agent 侧在实现时必须都考虑到且做好兼容。这样，Agent只需要实现一个CommonConfigProvider就可以受任意符合此协议规范的ConfigServer管控。

### 能力报告

Client：应当通过capbilitiies上报Agent自身的能力，这样如果老的客户端接入新的ConfigServer，ConfigServer便知道客户端不具备某项能力，从而不会向其发送不支持的配置或命令而得不到状态汇报导致无限循环。

Server：应当通过capbilitiies上报Server自身的能力，这样如果新的客户端接入老的ConfigServer，Agent便知道服务端不具备某项能力，从而不会被其响应所误导，如其不具备记忆Attributes能力，那么Attributes字段无论如何都不应该在心跳中被省略。

### 注册

Client：Agent启动后第一次向Server汇报全量信息，request字段应填尽填。request\_id、sequence\_num、capabilities、instance\_id、agent\_type、startup\_time为必填字段。

Server：Server根据上报的信息返回响应。pipeline\_config\_updates、process\_config\_updates中包含agent需要同步的配置，updates中必然包含name和version，是否包含详情context和detail取决于server端实现。custom\_command_updates包含要求agent执行的命令command中必然包含type、name和expire\_time。

Server是否保存Client信息取决于Server实现，如果服务端找不到或保存的sequence\_num + 1 ≠ 心跳的sequence\_num，那么就立刻返回并且flags中必须设置ReportFullStatus标识位。

Server根据agent\_type + attributes 查询进程配置，根据ip和tags查询机器组和关联采集配置。

![image](https://github.com/alibaba/ilogtail/assets/1827594/05799ac2-9249-49ed-8088-3b927821ac73)

### 心跳（心跳压缩）

Client：若接收到的响应中没有ReportFullStatus，且client的属性、配置状态、命令状态在上次上报后没有变化，那么可以只填instance\_id、sequence\_num，sequence\_num每次请求+1。若有ReportStatus或任何属性、配置状态变化或Server不支持属性、配置状态记忆能力，则必须完整上报状态。

Server：同注册

允许心跳压缩

![image](https://github.com/alibaba/ilogtail/assets/1827594/ad22bcfa-b14a-41b5-b4b8-4956bb065bf7)

不允许心跳压缩

![image](https://github.com/alibaba/ilogtail/assets/1827594/35a823fd-6b8e-499f-951e-2231f3319420)

### 进程配置

若Server的注册/心跳响应中有process\_config\_updates.detail

Client：直接从response中获得detail，应用成功后下次心跳需要上报完整状态。

若Server的响应不包含detail

Client：根据process\_config\_updates的信息构造FetchProcessConfigRequest

Server：返回FetchProcessConfigResponse

Client获取到多个进程配置时，自动合并，若产生冲突默认行为是未定义。

### 采集配置

若Server的注册/心跳响应中有pipeline\_config\_updates.detail

Client：直接从response中获得detail，应用成功后下次心跳需要上报完整状态。

若Server的响应不包含detail

Client：根据pipeline\_config\_updates的信息构造FetchPipelineConfigRequest

Server：返回FetchPipelineConfigResponse

客户端支持以下2种实现

实现1：直接将Detail返回在心跳响应中（FetchConfigDetail flag is unset）

![image](https://github.com/alibaba/ilogtail/assets/1827594/be645615-dd99-42dd-9deb-681e9a4069bb)

实现2：仅返回配置名和版本，Detail使用单独请求获取（FetchConfigDetail flag is set）

![image](https://github.com/alibaba/ilogtail/assets/1827594/c409c35c-2a81-4927-bfd2-7fb321ef1ca8)

### 配置状态上报

Client：这个版本的配置状态上报中修改了version的定义，-1仍然表示删除，0作为保留值，其他值都是合法version，只要version不同Client都应该视为配置更新。此外参考OpAMP增加了配置应用状态上报的字段，能反应出下发的配置是否生效。

Server：这些信息是Agent状态的一部分，可选保存。与通过Event上报可观测信息不同的是，作为状态信息没有时间属性，用户可通过接口可获取即刻状态，而不需要选择时间窗口合并事件。

### 预定义命令

Client: 通过request的flag传递，尚未定义

Server: 通过response的flag传递，定义了ReportFullStatus

### 自定义命令

Client: 为了防止服务端重复下发命令以及感知命令执行结果，在command expire前，Client始终应具备向服务端上报command执行状态的能力，实际是否上报取决于心跳压缩机制。在expire\_time超过后，client不应该再上报超时的command状态。

Server: 如果上报+已知的Agent状态中，缺少应下发的custom\_command\_updates（通过name识别），那么server应该在响应中下发缺少的custom\_command\_updates。
