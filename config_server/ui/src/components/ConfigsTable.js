import {Divider, message, Popconfirm, Table} from "antd";
import {AppliedAgentGroupsContext, ConfigContext, ConfigsContext} from "../common/context";
import AppliedAgentGroupsModal from "./AppliedAgentGroupsModal";
import ConfigModal from "./ConfigModal";
import {useContext, useEffect, useState} from "react";
import {interactive} from "../common/request";
import {FormattedMessage} from "react-intl";
import {configTypes} from "../common/const";

export default () => {
  const {
    config,
    configVisible,
    setConfig,
    setConfigVisible,
  } = useContext(ConfigsContext);

  const [loading, setLoading] = useState(false);
  const [dataSource, setDataSource] = useState([]);
  const [appliedAgentGroupsVisible, setAppliedAgentGroupsVisible] = useState(false);

  useEffect(() => {
    fetchDataSource();
  }, []);

  const fetchDataSource = () => {
    setLoading(true);
    interactive(`ListConfigs`, {}).then(async ([ok, statusCode, statusText, response]) => {
      setLoading(false);
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }

      console.log(response.message);
      const data = response.configDetails.map((item) => {
        return {
          key: item.name,
          rowKey: item.name,
          name: item.name,
          type: item.type,
          version: item.version,
          context: item.context,
          detail: item.detail,
        };
      });
      setDataSource(data);
    });
  };

  const fetchConfigSummaries = async (key, force) => {
    if (!force) {
      if (dataSource.find(it => it.key === key).appliedAgentGroupCount !== undefined) {
        return;
      }
    }
    const params = {
      configName: key,
    };
    const [ok, statusCode, statusText, response] = await interactive(`GetAppliedAgentGroups`, params);
    if (!ok) {
      console.log(`fetch applied agent groups summary: ${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
      return;
    }
    const data = dataSource.map(it => {
      return {
        ...it,
        appliedAgentGroupCount: (it.key === key) ? response.agentGroupNames.length : it.appliedAgentGroupCount,
        appliedAgentGroups: (it.key === key) ? response.agentGroupNames : it.appliedAgentGroups,
      };
    });
    setDataSource(data);
  };

  const openAppliedAgentGroups = (name) => {
    setConfig({
      name,
    });
    setAppliedAgentGroupsVisible(true);
  };

  const getConfig = (configName) => {
    const params = {
      configName,
    };
    interactive(`GetConfig`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      const data = {
        name: response.configDetail.name,
        type: response.configDetail.type,
        version: response.configDetail.version,
        context: response.configDetail.context,
        detail: response.configDetail.detail,
      };
      console.log(data);
      setConfig(data);
      setConfigVisible(true);
    });
  };

  const deleteConfig = (configName) => {
    const params = {
      configName,
    };
    interactive(`DeleteConfig`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      message.success(`Config ${configName} deleted`);
      fetchDataSource();
    });
  };

  const configColumns = [
    {
      key: 'name',
      dataIndex: 'name',
      title: <FormattedMessage id="config_name" />,
    },
    {
      key: 'type',
      dataIndex: 'type',
      title: <FormattedMessage id="config_type" />,
      render: (v) => v !== undefined ? <FormattedMessage id={`${configTypes[v]}`} /> : '',
    },
    {
      key: 'appliedAgentGroupCount',
      dataIndex: 'appliedAgentGroupCount',
      title: <FormattedMessage id="config_applied_group_count" />,
      render: (value, record) => (
        <a onClick={() => {openAppliedAgentGroups(record.name)}}>
          {value > 0 ? `${value} [${record.appliedAgentGroups.join(', ')}]`: (value === 0 ? value : '')}
        </a>
      ),
    },
    {
      key: 'operate',
      title: <FormattedMessage id="operate" />,
      render: (record) => (
        <>
          <a onClick={() => getConfig(record.name)}>
            <FormattedMessage id="open" />
          </a>
          <Divider type="vertical" />
          <Popconfirm
            title={<FormattedMessage id="config_delete_confirm" />}
            onConfirm={() => deleteConfig(record.name)}
          >
            <a href="#">
              <FormattedMessage id="delete" />
            </a>
          </Popconfirm>
        </>
      ),
    }
  ];

  return (
    <>
      <Table
        dataSource={dataSource}
        columns={configColumns}
        loading={loading}
        onRow={(record) => {
          return {
            onMouseEnter: () => fetchConfigSummaries(record.key, false),
          };
        }}
      />
      <AppliedAgentGroupsContext.Provider
        value={{
          appliedAgentGroupsVisible,
          setAppliedAgentGroupsVisible,
          config,
          fetchConfigSummaries,
        }}
      >
        <AppliedAgentGroupsModal />
      </AppliedAgentGroupsContext.Provider>
      <ConfigContext.Provider
        value={{
          configVisible,
          config,
          setConfigVisible,
          setConfig,
          fetchDataSource,
        }}
      >
        <ConfigModal />
      </ConfigContext.Provider>
    </>
  );
};