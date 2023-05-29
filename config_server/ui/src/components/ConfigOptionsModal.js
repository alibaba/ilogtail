import {FormattedMessage} from "react-intl";
import {Checkbox, message, Modal, Table} from "antd";
import React, {useContext, useState} from "react";
import {ConfigOptionsContext} from "../common/context";
import {configTypes} from "../common/const";
import {interactive} from "../common/request";

export default () => {
  const {
    configOptionsVisible,
    configOptions,
    setConfigOptionsVisible,
    setConfigOptions,
    agentGroup,
    fetchAppliedConfigs,
  } = useContext(ConfigOptionsContext);

  const [configsBuffer, setConfigsBuffer] = useState([]);

  const toggleConfigOption = (target) => {
    if (configsBuffer.includes(target)) {
      setConfigsBuffer(configsBuffer.filter(item => item !== target));
    } else {
      const buffer = [
        ...configsBuffer,
        target,
      ];
      setConfigsBuffer(buffer);
    }
  };

  const closeConfigOptions = () => {
    setConfigOptionsVisible(false);
    setConfigOptions([]);
    setConfigsBuffer([]);
  };

  const applyConfigs = async () => {
    if (configsBuffer.length === 0) {
      closeConfigOptions();
      return;
    }
    const requests = configsBuffer.map(configName => {
      return interactive('ApplyConfigToAgentGroup', {
        groupName: agentGroup.groupName,
        configName,
      });
    });
    // res: [[ok, statusCode, statusText, response], ...]
    const res = await Promise.all(requests);
    if (res.some(it => !it[0])) {
      const err = res.find(it => !it[0]);
      message.error(`${err[1]} ${err[2]}: ${err[3].message || 'unknown error'}`);
      return;
    }
    console.log(res.map(it => it[3].message));
    message.success(`Apply config to agent group ${agentGroup.groupName} success`);
    closeConfigOptions();
    fetchAppliedConfigs(agentGroup.groupName);
  };

  const configOptionColumns = [
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
      key: 'context',
      dataIndex: 'context',
      title: <FormattedMessage id="config_context" />,
    },
    {
      key: 'operate',
      title: <FormattedMessage id="select" />,
      render: (record) => (
        <Checkbox
          onChange={() => {toggleConfigOption(record.name)}}
          checked={record.applied || configsBuffer.includes(record.name)}
          disabled={record.applied}
        />
      ),
    },
  ];

  return (
    <Modal
      title={<FormattedMessage id="config_option_title" values={{ groupName: agentGroup.groupName }} />}
      width={800}
      open={configOptionsVisible}
      onOk={applyConfigs}
      onCancel={closeConfigOptions}
    >
      <Table
        dataSource={configOptions}
        columns={configOptionColumns}
      />
    </Modal>
  );
};