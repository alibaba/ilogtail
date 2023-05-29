import {Divider, message, Popconfirm, Table} from "antd";
import {useContext, useEffect, useState} from "react";
import {interactive} from "../common/request";
import {FormattedMessage} from "react-intl";
import {AgentGroupContext, AgentGroupsContext, AgentsContext, AppliedConfigsContext} from "../common/context";
import AgentsModal from "./AgentsModal";
import AppliedConfigsModal from "./AppliedConfigsModal";
import AgentGroupModal from "./AgentGroupModal";

export default () => {
  const {
    agentGroup,
    agentGroupVisible,
    setAgentGroup,
    setAgentGroupVisible,
  } = useContext(AgentGroupsContext);

  const [loading, setLoading] = useState(false);
  const [dataSource, setDataSource] = useState([]);
  const [appliedConfigsVisible, setAppliedConfigsVisible] = useState(false);
  const [agents, setAgents] = useState([]);
  const [agentsVisible, setAgentsVisible] = useState(false);

  useEffect(() => {
    fetchDataSource();
  }, []);

  const fetchDataSource = () => {
    setLoading(true);
    interactive(`ListAgentGroups`, {}).then(async ([ok, statusCode, statusText, response]) => {
      setLoading(false);
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }

      console.log(response.message);
      const data = response.agentGroups.map((item) => {
        const old = dataSource.find(it => it.groupName === item.groupName);
        return {
          key: item.groupName,
          rowKey: item.groupName,
          groupName: item.groupName,
          description: item.description,
          tags: item.tags.map(it => `${it.name} = ${it.value}`).join(', '),
          agentCount: old ? old.agentCount : undefined,
          appliedConfigCount: old ? old.appliedConfigCount : undefined,
          appliedConfigs: old ? old.appliedConfigs : undefined,
        };
      });
      setDataSource(data);
    });
  };

  const fetchAgentGroupSummaries = async (key, force) => {
    if (!force) {
      if (dataSource.find(it => it.key === key).agentCount !== undefined &&
        dataSource.find(it => it.key === key).appliedConfigCount !== undefined) {
        return;
      }
    }
    const params = {
      groupName: key,
    };
    const [ok1, statusCode1, statusText1, response1] = await interactive(`ListAgents`, params);
    if (!ok1) {
      console.log(`fetch agents summary: ${statusCode1} ${statusText1}: ${response1.message || 'unknown error'}`);
      return;
    }
    const [ok2, statusCode2, statusText2, response2] = await interactive(`GetAppliedConfigsForAgentGroup`, params);
    if (!ok2) {
      console.log(`fetch applied configs summary: ${statusCode2} ${statusText2}: ${response2.message || 'unknown error'}`);
      return;
    }
    const data = dataSource.map(it => {
      return {
        ...it,
        agentCount: (it.key === key) ? response1.agents.length : it.agentCount,
        appliedConfigCount: (it.key === key) ? response2.configNames.length : it.appliedConfigCount,
        appliedConfigs: (it.key === key) ? response2.configNames : it.appliedConfigs,
      };
    });
    setDataSource(data);
  };

  const fetchAgents = (groupName) => {
    const params = {
      groupName,
    };
    interactive(`ListAgents`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      if (response.agents.length === 0) {
        message.warning('Agent not found');
        return;
      }
      const data = response.agents.map(item => {
        return {
          key: item.agentId,
          rowKey: item.agentId,
          agentType: item.agentType,
          agentId: item.agentId,

          // attributes
          version: item.attributes.version,
          category: item.attributes.category,
          ip: item.attributes.ip,
          region: item.attributes.region,
          zone: item.attributes.zone,

          tags: item.tags.map(it => `${it.name} = ${it.value}`).join(', '),
          runningStatus: item.runningStatus,
          startupTime: item.startupTime,
          interval: item.interval,
        };
      });
      console.log(data);
      setAgents(data);
      setAgentsVisible(true);
    });
  };

  const openAppliedConfigs = (groupName) => {
    setAgentGroup({
      groupName,
    });
    setAppliedConfigsVisible(true);
  };

  const getAgentGroup = (groupName) => {
    const params = {
      groupName,
    };
    interactive(`GetAgentGroup`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      const data = {
        groupName: response.agentGroup.groupName,
        description: response.agentGroup.description,
        tags: response.agentGroup.tags.map(item => {
          return {
            name: item.name,
            value: item.value,
          }
        }),
      };
      console.log(data);
      setAgentGroup(data);
      setAgentGroupVisible(true);
    });
  };

  const deleteAgentGroup = (groupName) => {
    const params = {
      groupName,
    };
    interactive(`DeleteAgentGroup`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      message.success(`Agent group ${groupName} deleted`);
      fetchDataSource();
    });
  };

  const agentGroupColumns = [
    {
      key: 'groupName',
      dataIndex: 'groupName',
      title: <FormattedMessage id="group_name" />,
    },
    {
      key: 'description',
      dataIndex: 'description',
      title: <FormattedMessage id="group_description" />,
    },
    {
      key: 'tags',
      dataIndex: 'tags',
      title: <FormattedMessage id="group_tags" />,
    },
    {
      key: 'agentCount',
      dataIndex: 'agentCount',
      title: <FormattedMessage id="group_agent_count" />,
      render: (value, record) => (
        <a onClick={() => fetchAgents(record.groupName)}>
          {value}
        </a>
      ),
    },
    {
      key: 'appliedConfigCount',
      dataIndex: 'appliedConfigCount',
      title: <FormattedMessage id="group_applied_config_count" />,
      render: (value, record) => (
        <a onClick={() => {openAppliedConfigs(record.groupName)}}>
          {value > 0 ? `${value} [${record.appliedConfigs.join(', ')}]`: (value === 0 ? value : '')}
        </a>
      ),
    },
    {
      key: 'operate',
      title: <FormattedMessage id="operate" />,
      render: (record) => (
        <>
          <a onClick={() => getAgentGroup(record.groupName)}>
            <FormattedMessage id="open" />
          </a>
          <Divider type="vertical" />
          <Popconfirm
            title={<FormattedMessage id="group_delete_confirm" />}
            onConfirm={() => deleteAgentGroup(record.groupName)}
          >
            <a href="#">
              <FormattedMessage id="delete" />
            </a>
          </Popconfirm>
        </>
      ),
    },
  ];

  return (
    <>
      <Table
        dataSource={dataSource}
        columns={agentGroupColumns}
        loading={loading}
        onRow={(record) => {
          return {
            onMouseEnter: () => fetchAgentGroupSummaries(record.key, false),
          };
        }}
      />
      <AgentsContext.Provider
        value={{
          agentsVisible,
          agents,
          setAgentsVisible,
          setAgents,
        }}
      >
        <AgentsModal />
      </AgentsContext.Provider>
      <AppliedConfigsContext.Provider
        value={{
          appliedConfigsVisible,
          setAppliedConfigsVisible,
          agentGroup,
          fetchAgentGroupSummaries,
        }}
      >
        <AppliedConfigsModal />
      </AppliedConfigsContext.Provider>
      <AgentGroupContext.Provider
        value={{
          agentGroupVisible,
          agentGroup,
          setAgentGroupVisible,
          setAgentGroup,
          fetchDataSource,
        }}
      >
        <AgentGroupModal />
      </AgentGroupContext.Provider>
    </>
  );
};