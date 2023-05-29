import {v4 as uuidv4} from 'uuid';
import protobuf from 'protobufjs';
import proto from "./user.proto";

const methods = {
  CreateAgentGroup: 'POST',
  UpdateAgentGroup: 'PUT',
  DeleteAgentGroup: 'DELETE',
  GetAgentGroup: 'POST',
  ListAgentGroups: 'POST',

  CreateConfig: 'POST',
  UpdateConfig: 'PUT',
  DeleteConfig: 'DELETE',
  GetConfig: 'POST',
  ListConfigs: 'POST',

  ApplyConfigToAgentGroup: 'PUT',
  RemoveConfigFromAgentGroup: 'DELETE',
  GetAppliedConfigsForAgentGroup: 'POST',
  GetAppliedAgentGroups: 'POST',
  ListAgents: 'POST',
};

export const interactive = async (action, params) => {
  const root = await protobuf.load(proto);
  const reqType = root.lookupType(`configserver.proto.${action}Request`);
  const message = reqType.create({
    requestId: uuidv4(),
    ...params,
  });
  // console.log(`request message = ${JSON.stringify(message)}`);
  const body = reqType.encode(message).finish();
  // console.log(`request raw body = ${Array.prototype.toString.call(body)}`);

  const options = {
    method: methods[action],
    body,
    headers: {
      'Content-Type': 'application/x-protouf',
    },
  };
  const response = await fetch(`/api/v1/User/${action}`, options);
  const blob = await response.blob();
  const arrayBuffer = await blob.arrayBuffer();
  const data = new Uint8Array(arrayBuffer);
  // const contentLength = parseInt(response.headers.get("content-length"));
  // console.log(`response content length: ${contentLength}, raw body: ${data}`);
  const respType = root.lookupType(`configserver.proto.${action}Response`);
  // console.log(`decoded response body: ${JSON.stringify(respType.decode(data))}`);
  return [response.ok, response.status, response.statusText, respType.decode(data)];
};
