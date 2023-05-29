import {Button, Form, message, Modal} from "antd";
import {FormattedMessage} from "react-intl";
import FormBuilder from "antd-form-builder";
import React, {useContext, useEffect, useState} from "react";
import {ConfigContext} from "../common/context";
import {interactive} from "../common/request";
import {configTypes} from "../common/const";
import yaml from 'js-yaml';

import AceEditor from "react-ace";

import 'brace/mode/yaml';
import 'brace/theme/xcode';

export default () => {
  const {
    configVisible,
    config,
    setConfigVisible,
    setConfig,
    fetchDataSource,
  } = useContext(ConfigContext);

  const [loading, setLoading] = useState(false);
  const [action, setAction] = useState('');
  const [form] = Form.useForm();

  useEffect(() => {
    if (configVisible) {
      form.setFieldsValue(config);
      if (config.name) {
        setAction('UpdateConfig');
      } else {
        setAction('CreateConfig');
      }
    } else {
      form.resetFields();
      setAction('');
    }
  }, [configVisible]);

  const closeConfig = () => {
    setConfig({});
    setConfigVisible(false);
  };

  const someDoesNotHaveType = (plugins) => {
    return plugins.some(item => !item.Type);
  };

  const submitConfig = (values) => {
    try {
      const obj = yaml.load(values.detail);
      console.log(obj);
      if (!obj || !obj.inputs || !obj.flushers ||
        someDoesNotHaveType(obj.inputs) ||
        someDoesNotHaveType(obj.flushers) ||
        obj.processors === null || obj.aggregators === null ||
        (obj.processors && someDoesNotHaveType(obj.processors)) ||
        (obj.aggregators && someDoesNotHaveType(obj.aggregators))
      ) {
        message.warning(`Config invalid`).then();
        return;
      }
    } catch (e) {
      message.warning(`${e.name} - ${e.reason}`).then();
      return;
    }
    const params = {
      configDetail: {
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
      message.success(`Config ${form.getFieldValue('name')} created or updated`);
      closeConfig();
      fetchDataSource();
    });
  };

  const meta = {
    columns: 1,
    formItemLayout: [4, 20],
    fields: [
      {
        key: 'name',
        label: <FormattedMessage id="config_name" />,
        disabled: action === 'UpdateConfig',
        required: action === 'CreateConfig',
        message: <FormattedMessage id="form_item_required" />,
      },
      {
        key: 'type',
        label: <FormattedMessage id="config_type" />,
        required: true,
        message: <FormattedMessage id="form_item_required" />,
        widget: 'radio-group',
        buttonGroup: true,
        options: configTypes.map((item, index) => {
          return {
            value: index,
            label: <FormattedMessage id={item} />,
          };
        }),
      },
      {
        key: 'context',
        label: <FormattedMessage id="config_context" />,
        required: true,
        message: <FormattedMessage id="form_item_required" />,
      },
      {
        key: 'detail',
        label: <FormattedMessage id="config_detail" />,
        required: true,
        message: <FormattedMessage id="form_item_required" />,
        widget: field => (
          <AceEditor
            name="config_detail_editor"
            mode="yaml"
            theme="xcode"
            value={field.value}
            onChange={(detail) => form.setFieldsValue({
              detail,
            })}
            debounceChangePeriod={1000}
            fontSize={13}
            width="100%"
            minLines={10}
            maxLines={20}
          />
        ),
      },
    ],
  };

  return (
    <Modal
      title={<FormattedMessage id="config_open_title" />}
      width={800}
      open={configVisible}
      onCancel={closeConfig}
      forceRender
      footer={[]}
    >
      <Form
        layout="horizontal"
        form={form}
        onFinish={submitConfig}
      >
        <FormBuilder
          meta={meta}
          form={form}
        />
        <Form.Item wrapperCol={{ offset: 4 }}>
          <Button
            type="primary"
            htmlType="submit"
            loading={loading}
          >
            <FormattedMessage id="config_save" />
          </Button>
        </Form.Item>
      </Form>
    </Modal>
  );
};