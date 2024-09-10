/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
"use strict";

var $protobuf = require("protobufjs/light");

var $root = ($protobuf.roots["default"] || ($protobuf.roots["default"] = new $protobuf.Root()))
.addJSON({
  AgentGroupTag: {
    fields: {
      name: {
        type: "string",
        id: 1
      },
      value: {
        type: "string",
        id: 2
      }
    }
  },
  ConfigStatus: {
    values: {
      UNSET: 0,
      APPLYING: 1,
      APPLIED: 2,
      FAILED: 3
    }
  },
  ConfigInfo: {
    fields: {
      name: {
        type: "string",
        id: 1
      },
      version: {
        type: "int64",
        id: 2
      },
      status: {
        type: "ConfigStatus",
        id: 3
      },
      message: {
        type: "string",
        id: 4
      }
    }
  },
  CommandInfo: {
    fields: {
      type: {
        type: "string",
        id: 1
      },
      name: {
        type: "string",
        id: 2
      },
      status: {
        type: "ConfigStatus",
        id: 3
      },
      message: {
        type: "string",
        id: 4
      }
    }
  },
  AgentAttributes: {
    fields: {
      version: {
        type: "bytes",
        id: 1
      },
      ip: {
        type: "bytes",
        id: 2
      },
      hostname: {
        type: "bytes",
        id: 3
      },
      extras: {
        keyType: "string",
        type: "bytes",
        id: 100
      }
    }
  },
  AgentCapabilities: {
    values: {
      UnspecifiedAgentCapability: 0,
      AcceptsPipelineConfig: 1,
      AcceptsInstanceConfig: 2,
      AcceptsCustomCommand: 4
    }
  },
  RequestFlags: {
    values: {
      RequestFlagsUnspecified: 0,
      FullState: 1
    }
  },
  HeartbeatRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      sequenceNum: {
        type: "uint64",
        id: 2
      },
      capabilities: {
        type: "uint64",
        id: 3
      },
      instanceId: {
        type: "bytes",
        id: 4
      },
      agentType: {
        type: "string",
        id: 5
      },
      attributes: {
        type: "AgentAttributes",
        id: 6
      },
      tags: {
        rule: "repeated",
        type: "AgentGroupTag",
        id: 7
      },
      runningStatus: {
        type: "string",
        id: 8
      },
      startupTime: {
        type: "int64",
        id: 9
      },
      pipelineConfigs: {
        rule: "repeated",
        type: "ConfigInfo",
        id: 10
      },
      instanceConfigs: {
        rule: "repeated",
        type: "ConfigInfo",
        id: 11
      },
      customCommands: {
        rule: "repeated",
        type: "CommandInfo",
        id: 12
      },
      flags: {
        type: "uint64",
        id: 13
      },
      opaque: {
        type: "bytes",
        id: 14
      }
    }
  },
  ConfigDetail: {
    fields: {
      name: {
        type: "string",
        id: 1
      },
      version: {
        type: "int64",
        id: 2
      },
      detail: {
        type: "bytes",
        id: 3
      }
    }
  },
  CommandDetail: {
    fields: {
      type: {
        type: "string",
        id: 1
      },
      name: {
        type: "string",
        id: 2
      },
      detail: {
        type: "bytes",
        id: 3
      },
      expireTime: {
        type: "int64",
        id: 4
      }
    }
  },
  ServerCapabilities: {
    values: {
      UnspecifiedServerCapability: 0,
      RembersAttribute: 1,
      RembersPipelineConfigStatus: 2,
      RembersInstanceConfigStatus: 4,
      RembersCustomCommandStatus: 8
    }
  },
  ResponseFlags: {
    values: {
      ResponseFlagsUnspecified: 0,
      ReportFullState: 1,
      FetchPipelineConfigDetail: 2,
      FetchInstanceConfigDetail: 4
    }
  },
  HeartbeatResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      capabilities: {
        type: "uint64",
        id: 3
      },
      pipelineConfigUpdates: {
        rule: "repeated",
        type: "ConfigDetail",
        id: 4
      },
      instanceConfigUpdates: {
        rule: "repeated",
        type: "ConfigDetail",
        id: 5
      },
      customCommandUpdates: {
        rule: "repeated",
        type: "CommandDetail",
        id: 6
      },
      flags: {
        type: "uint64",
        id: 7
      },
      opaque: {
        type: "bytes",
        id: 8
      }
    }
  },
  FetchConfigRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      instanceId: {
        type: "bytes",
        id: 2
      },
      reqConfigs: {
        rule: "repeated",
        type: "ConfigInfo",
        id: 3
      }
    }
  },
  FetchConfigResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      configDetails: {
        rule: "repeated",
        type: "ConfigDetail",
        id: 3
      }
    }
  },
  CommonResponse: {
    fields: {
      status: {
        type: "int32",
        id: 1
      },
      errorMessage: {
        type: "bytes",
        id: 2
      }
    }
  },
  Agent: {
    fields: {
      capabilities: {
        type: "uint64",
        id: 1
      },
      instanceId: {
        type: "bytes",
        id: 2
      },
      agentType: {
        type: "string",
        id: 3
      },
      attributes: {
        type: "AgentAttributes",
        id: 4
      },
      runningStatus: {
        type: "string",
        id: 5
      },
      startupTime: {
        type: "int64",
        id: 6
      },
      flags: {
        type: "uint64",
        id: 7
      },
      opaque: {
        type: "bytes",
        id: 8
      }
    }
  },
  CreateAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      agentGroup: {
        type: "AgentGroupTag",
        id: 2
      }
    }
  },
  CreateAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  UpdateAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      agentGroup: {
        type: "AgentGroupTag",
        id: 2
      }
    }
  },
  UpdateAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  DeleteAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      groupName: {
        type: "string",
        id: 2
      }
    }
  },
  DeleteAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  GetAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      groupName: {
        type: "string",
        id: 2
      }
    }
  },
  GetAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      agentGroup: {
        type: "AgentGroupTag",
        id: 3
      }
    }
  },
  ListAgentGroupsRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      }
    }
  },
  ListAgentGroupsResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      agentGroups: {
        rule: "repeated",
        type: "AgentGroupTag",
        id: 4
      }
    }
  },
  CreateConfigRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configDetail: {
        type: "ConfigDetail",
        id: 2
      }
    }
  },
  CreateConfigResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  UpdateConfigRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configDetail: {
        type: "ConfigDetail",
        id: 2
      }
    }
  },
  UpdateConfigResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  DeleteConfigRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configName: {
        type: "string",
        id: 2
      }
    }
  },
  DeleteConfigResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  GetConfigRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configName: {
        type: "string",
        id: 2
      }
    }
  },
  GetConfigResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      configDetail: {
        type: "ConfigDetail",
        id: 3
      }
    }
  },
  ListConfigsRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      }
    }
  },
  ListConfigsResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      configDetails: {
        rule: "repeated",
        type: "ConfigDetail",
        id: 3
      }
    }
  },
  ApplyConfigToAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configName: {
        type: "string",
        id: 2
      },
      groupName: {
        type: "string",
        id: 3
      }
    }
  },
  ApplyConfigToAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  RemoveConfigFromAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configName: {
        type: "string",
        id: 2
      },
      groupName: {
        type: "string",
        id: 3
      }
    }
  },
  RemoveConfigFromAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      }
    }
  },
  GetAppliedConfigsForAgentGroupRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      groupName: {
        type: "string",
        id: 2
      }
    }
  },
  GetAppliedConfigsForAgentGroupResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      configNames: {
        rule: "repeated",
        type: "string",
        id: 4
      }
    }
  },
  GetAppliedAgentGroupsRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      configName: {
        type: "string",
        id: 2
      }
    }
  },
  GetAppliedAgentGroupsResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      agentGroupNames: {
        rule: "repeated",
        type: "string",
        id: 3
      }
    }
  },
  ListAgentsRequest: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      groupName: {
        type: "string",
        id: 2
      }
    }
  },
  ListAgentsResponse: {
    fields: {
      requestId: {
        type: "bytes",
        id: 1
      },
      commonResponse: {
        type: "CommonResponse",
        id: 2
      },
      agents: {
        rule: "repeated",
        type: "Agent",
        id: 3
      }
    }
  }
});

module.exports = $root;
