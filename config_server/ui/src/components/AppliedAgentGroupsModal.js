import {Form, message, Modal, Tabs} from "antd";
import {FormattedMessage} from "react-intl";
import FormBuilder from "antd-form-builder";
import React, {useContext, useEffect, useState} from "react";
import {AppliedAgentGroupsContext, AgentGroupOptionsContext} from "../common/context";
import {interactive} from "../common/request";
import AgentGroupOptionsModal from "./AgentGroupOptionsModal";

export default () => {
  const {
    appliedAgentGroupsVisible,
    setAppliedAgentGroupsVisible,
    config,
    fetchConfigSummaries,
  } = useContext(AppliedAgentGroupsContext);

  const [appliedAgentGroups, setAppliedAgentGroups] = useState([]);
  const [agentGroupsBuffer, setAgentGroupsBuffer] = useState([]);
  const [activeAppliedAgentGroupTab, setActiveAppliedAgentGroupTab] = useState('');
  const [agentGroupOptions, setAgentGroupOptions] = useState([]);
  const [agentGroupOptionsVisible, setAgentGroupOptionsVisible] = useState(false);
  const [form] = Form.useForm();

  useEffect(() => {
    if (appliedAgentGroupsVisible) {
      fetchAppliedAgentGroups(config.name);
    }
  }, [appliedAgentGroupsVisible]);

  useEffect(() => {
    if (appliedAgentGroups.length > 0) {
      setActiveAppliedAgentGroupTab(appliedAgentGroups[0].key);
    }
  }, [appliedAgentGroups]);

  const fetchAppliedAgentGroups = (configName) => {
    const params = {
      configName,
    };
    interactive(`GetAppliedAgentGroups`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      const requests = response.agentGroupNames.map(groupName => {
        return interactive('GetAgentGroup', {
          groupName,
        });
      });
      // res: [[ok, statusCode, statusText, response], ...]
      const res = await Promise.all(requests);
      if (res.some(it => !it[0])) {
        const err = res.find(it => !it[0]);
        message.error(`${err[1]} ${err[2]}: ${err[3].message || 'unknown error'}`);
        return;
      }
      const data = res.map(it => it[3].agentGroup).map(it => {
        return {
          key: it.groupName,
          groupName: it.groupName,
          description: it.description,
          tags: it.tags.map(i => {
            return {
              name: i.name,
              value: i.value,
            }
          }),
        };
      });
      console.log(data);
      setAppliedAgentGroups(data);
    });
  };

  const fetchAgentGroupOptions = () => {
    interactive(`ListAgentGroups`, {}).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }

      const data = response.agentGroups.map(item => {
        return {
          key: item.groupName,
          rowKey: item.groupName,
          groupName: item.groupName,
          description: item.description,
          tags: item.tags.map(it => {
            return {
              name: it.name,
              value: it.value,
            }
          }),
          applied: appliedAgentGroups.map(it => it.groupName).includes(item.groupName),
        };
      });
      console.log(data);
      setAgentGroupOptions(data);
      setAgentGroupOptionsVisible(true);
    });
  };

  const removeAppliedAgentGroups = async () => {
    if (agentGroupsBuffer.length === 0) {
      closeAppliedAgentGroups();
      fetchConfigSummaries(config.name, true);
      return;
    }
    const requests = agentGroupsBuffer.map(groupName => {
      return interactive('RemoveConfigFromAgentGroup', {
        groupName,
        configName: config.name,
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
    message.success(`Remove applied agent group from config ${config.name} success`);
    closeAppliedAgentGroups();
    fetchConfigSummaries(config.name, true);
  };

  const closeAppliedAgentGroups = () => {
    setActiveAppliedAgentGroupTab('');
    setAppliedAgentGroupsVisible(false);
    setAppliedAgentGroups([]);
    setAgentGroupsBuffer([]);
  };

  const switchAppliedConfigTab = (activeTab) => {
    setActiveAppliedAgentGroupTab(activeTab);
  };

  const removeAppliedAgentGroupTab = (targetTab) => {
    const buffer = [
      ...agentGroupsBuffer,
      targetTab,
    ];
    setAgentGroupsBuffer(buffer);
    let newActiveKey = activeAppliedAgentGroupTab;
    let lastIndex = -1;
    appliedAgentGroups.forEach((item, i) => {
      if (item.key === targetTab) {
        lastIndex = i - 1;
      }
    });
    const newPanes = appliedAgentGroups.filter((item) => item.key !== targetTab);
    if (newPanes.length && newActiveKey === targetTab) {
      if (lastIndex >= 0) {
        newActiveKey = newPanes[lastIndex].key;
      } else {
        newActiveKey = newPanes[0].key;
      }
    }
    setAppliedAgentGroups(newPanes);
    setActiveAppliedAgentGroupTab(newActiveKey);
  };

  const editAppliedAgentGroupTab = (targetTab, action) => {
    if (action === 'remove') {
      removeAppliedAgentGroupTab(targetTab);
    }
    if (action === 'add') {
      if (agentGroupsBuffer.length > 0) {
        message.warning(`Applied agent group changed, please save first`).then();
        return;
      }
      fetchAgentGroupOptions();
    }
  };

  const meta = {
    columns: 1,
    formItemLayout: [4, 20],
    fields: [
      {
        key: 'groupName',
        label: <FormattedMessage id="group_name" />,
      },
      {
        key: 'description',
        label: <FormattedMessage id="group_description" />,
      },
      {
        key: 'tags',
        label: <FormattedMessage id="group_tags" />,
        viewWidget: (field) => (field.value.map(item => `${item.name} = ${item.value}`).join(', ')),
      },
    ],
  };

  return (
    <Modal
      title={<FormattedMessage id="applied_agent_group_title" values={{ configName: config.name }} />}
      width={800}
      open={appliedAgentGroupsVisible}
      onOk={removeAppliedAgentGroups}
      onCancel={closeAppliedAgentGroups}
      forceRender
    >
      <Tabs
        type="editable-card"
        activeKey={activeAppliedAgentGroupTab}
        onChange={switchAppliedConfigTab}
        onEdit={editAppliedAgentGroupTab}
        items={appliedAgentGroups.map((agentGroup) => {
          return {
            key: agentGroup.key,
            label: agentGroup.groupName,
            children:
              <Form
                layout="horizontal"
                form={form}
              >
                <FormBuilder
                  meta={meta}
                  form={form}
                  initialValues={agentGroup}
                  viewMode
                />
              </Form>
          };
        })}
      />
      <AgentGroupOptionsContext.Provider
        value={{
          agentGroupOptionsVisible,
          agentGroupOptions,
          setAgentGroupOptionsVisible,
          setAgentGroupOptions,
          config,
          fetchAppliedAgentGroups,
        }}
      >
        <AgentGroupOptionsModal />
      </AgentGroupOptionsContext.Provider>
    </Modal>
  );
};