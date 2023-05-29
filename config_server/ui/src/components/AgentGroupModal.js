import {Button, Form, Input, message, Modal, Space, Table} from "antd";
import {FormattedMessage} from "react-intl";
import FormBuilder from "antd-form-builder";
import React, {useContext, useEffect, useState} from "react";
import {AgentGroupContext} from "../common/context";
import {MinusCircleOutlined, PlusOutlined} from "@ant-design/icons";
import {interactive} from "../common/request";
import {mapTimestamp} from "../common/util";

export default () => {
  const {
    agentGroupVisible,
    agentGroup,
    setAgentGroupVisible,
    setAgentGroup,
    fetchDataSource,
  } = useContext(AgentGroupContext);

  const [loading, setLoading] = useState(false);
  const [action, setAction] = useState('');
  const [agents, setAgents] = useState([]);
  const [form] = Form.useForm();

  useEffect(() => {
    if (agentGroupVisible) {
      form.setFieldsValue(agentGroup);
      if (agentGroup.groupName) {
        setAction('UpdateAgentGroup');
        fetchAgents(agentGroup.groupName);
      } else {
        setAction('CreateAgentGroup');
      }
    } else {
      form.resetFields();
      setAction('');
    }
  }, [agentGroupVisible]);

  const closeAgentGroup = () => {
    setAgentGroup({});
    setAgentGroupVisible(false);
  };

  const submitAgentGroup = (values) => {
    const params = {
      agentGroup: {
        ...values,
      },
    };
    setLoading(true);
    interactive(action, params).then(async ([ok, statusCode, statusText, response]) => {
      setLoading(false);
      if (!ok) {
        message.error(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
        return;
      }
      console.log(response.message);
      message.success(`Agent group ${form.getFieldValue('name')} created or updated`);
      closeAgentGroup();
      fetchDataSource();
    });
  };

  const fetchAgents = (groupName) => {
    const params = {
      groupName,
    };
    interactive(`ListAgents`, params).then(async ([ok, statusCode, statusText, response]) => {
      if (!ok) {
        console.log(`${statusCode} ${statusText}: ${response.message || 'unknown error'}`);
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
    });
  };

  const meta = {
    columns: 1,
    formItemLayout: [4, 20],
    fields: [
      {
        key: 'groupName',
        label: <FormattedMessage id="group_name" />,
        disabled: action === 'UpdateAgentGroup',
        required: action === 'CreateAgentGroup',
        message: <FormattedMessage id="form_item_required" />,
      },
      {
        key: 'description',
        label: <FormattedMessage id="group_description" />,
        required: true,
        message: <FormattedMessage id="form_item_required" />,
      },
      {
        key: 'tags',
        label: <FormattedMessage id="group_tags" />,
        widget: () => (
          <Form.List name="tags">
            {(fields, { add, remove }) => (
              <>
                {fields.map(({ key, name, ...restField }) => (
                  <Space key={key} style={{ display: 'flex' }} align="baseline">
                    <Form.Item
                      {...restField}
                      name={[name, 'name']}
                      rules={[{
                        required: true,
                        message: <FormattedMessage id="form_item_required" />,
                      }]}
                      style={{ width: 150, marginBottom: 4 }}
                    >
                      <Input placeholder="Key" />
                    </Form.Item>
                    =
                    <Form.Item
                      {...restField}
                      name={[name, 'value']}
                      rules={[{
                        required: true,
                        message: <FormattedMessage id="form_item_required" />,
                      }]}
                      style={{ width: 400, marginBottom: 4 }}
                    >
                      <Input placeholder="Value" />
                    </Form.Item>
                    <MinusCircleOutlined onClick={() => remove(name)} />
                  </Space>
                ))}
                <Form.Item>
                  <Button type="dashed" onClick={() => add()} block icon={<PlusOutlined />}>
                    <FormattedMessage id="add_field" />
                  </Button>
                </Form.Item>
              </>
            )}
          </Form.List>
        ),
      },
    ],
  };

  const agentColumns = [
    {
      key: 'agentId',
      dataIndex: 'agentId',
      title: <FormattedMessage id="agent_id" />,
    },
    {
      key: 'version',
      dataIndex: 'version',
      title: <FormattedMessage id="agent_version" />,
    },
    {
      key: 'ip',
      dataIndex: 'ip',
      title: <FormattedMessage id="agent_ip" />,
    },
    {
      key: 'startupTime',
      dataIndex: 'startupTime',
      title: <FormattedMessage id="agent_startup_time" />,
      render: (ts) => mapTimestamp(ts),
    },
  ];

  return (
    <Modal
      title={<FormattedMessage id="group_open_title" />}
      width={800}
      open={agentGroupVisible}
      onCancel={closeAgentGroup}
      forceRender
      footer={[]}
    >
      <Form
        layout="horizontal"
        form={form}
        onFinish={submitAgentGroup}
      >
        <FormBuilder
          meta={meta}
          form={form}
        />
        {action === 'UpdateAgentGroup' && (
          <Form.Item
            label={<FormattedMessage id="agent_title" />}
            labelCol={{ span: 4 }}
          >
            <Table
              size="small"
              dataSource={agents}
              columns={agentColumns}
            />
          </Form.Item>
        )}
        <Form.Item wrapperCol={{ offset: 4 }}>
          <Button
            type="primary"
            htmlType="submit"
            loading={loading}
          >
            <FormattedMessage id="group_save" />
          </Button>
        </Form.Item>
      </Form>
    </Modal>
  );
};