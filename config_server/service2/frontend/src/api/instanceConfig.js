import {base64ToStr, constructProtobufRequest, strToBytes, URL_PREFIX} from "@/api/common";
import userProto from "@/proto/user_pb";
import agentProto from "@/proto/agent_pb"

export async function listInstanceConfigs(){
    let url=URL_PREFIX+"ListInstanceConfigs"
    let req = new userProto.ListConfigsRequest()
    let res=await constructProtobufRequest(url, req, userProto.ListConfigsResponse)
    res.configDetailsList.forEach(res => {
        res.detail =  base64ToStr(res.detail)
    })
    return res
}

export async function getInstanceConfig(name){
    let url=URL_PREFIX+"GetInstanceConfig"
    let req = new userProto.GetConfigRequest()
    req.setConfigName(name)
    return await constructProtobufRequest(url, req, userProto.GetConfigResponse);
}

export async function createInstanceConfig(name,version,detail){
    let url=URL_PREFIX+"CreateInstanceConfig"
    let req = new userProto.CreateConfigRequest()
    let configDetail=new agentProto.ConfigDetail()
    configDetail.setName(name)
    configDetail.setVersion(version)
    configDetail.setDetail(strToBytes(detail))
    req.setConfigDetail(configDetail)
    return await constructProtobufRequest(url, req, userProto.CreateConfigResponse);
}

export async function updateInstanceConfig(name,version,detail){
    let url=URL_PREFIX+"UpdateInstanceConfig"
    let req = new userProto.UpdateConfigRequest()
    let configDetail=new agentProto.ConfigDetail()
    configDetail.setName(name)
    configDetail.setVersion(version)
    configDetail.setDetail(strToBytes(detail))
    req.setConfigDetail(configDetail)
    return await constructProtobufRequest(url, req, userProto.UpdateConfigResponse);
}

export async function deleteInstanceConfig(name) {
    let url = URL_PREFIX + "DeleteInstanceConfig"
    let req = new userProto.DeleteConfigRequest()
    req.setConfigName(name)
    return await constructProtobufRequest(url, req, userProto.DeleteConfigResponse);
}

export async function applyInstanceConfigToAgentGroup(configName,groupName){
    let url = URL_PREFIX + "ApplyInstanceConfigToAgentGroup"
    let req = new userProto.ApplyConfigToAgentGroupRequest()
    req.setConfigName(configName)
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.ApplyConfigToAgentGroupResponse);
}

export async function removeInstanceConfigFromAgentGroup(configName,groupName){
    let url = URL_PREFIX + "RemoveInstanceConfigFromAgentGroup"
    let req = new userProto.RemoveConfigFromAgentGroupRequest()
    req.setConfigName(configName)
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.RemoveConfigFromAgentGroupResponse);
}
export async function getAppliedAgentGroupsWithInstanceConfig(configName) {
    let url = URL_PREFIX + "GetAppliedAgentGroupsWithInstanceConfig"
    let req = new userProto.GetAppliedAgentGroupsRequest()
    req.setConfigName(configName)
    return await constructProtobufRequest(url, req, userProto.GetAppliedAgentGroupsResponse);
}