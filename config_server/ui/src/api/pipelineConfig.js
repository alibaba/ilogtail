import { constructProtobufRequest, strToBytes, URL_PREFIX} from "@/api/common";
import userProto from "@/proto/user_pb";
import agentProto from "@/proto/agent_pb"


export async function listPipelineConfigs() {
    let url = URL_PREFIX + "ListPipelineConfigs"
    let req = new userProto.ListConfigsRequest()
    let res = await constructProtobufRequest(url, req, userProto.ListConfigsResponse)
    // res.configDetailsList.forEach(res => {
    //     res.detail =  base64ToStr(res.detail)
    // })
    return res
}

export async function getPipelineConfig(name){
    let url=URL_PREFIX+"GetPipelineConfig"
    let req = new userProto.GetConfigRequest()
    req.setConfigName(name)
    return await constructProtobufRequest(url, req, userProto.GetConfigResponse);
}

export async function createPipelineConfig(name,version,detail){
    let url=URL_PREFIX+"CreatePipelineConfig"
    let req = new userProto.CreateConfigRequest()
    let configDetail=new agentProto.ConfigDetail()
    configDetail.setName(name)
    configDetail.setVersion(version)
    configDetail.setDetail(strToBytes(detail))
    req.setConfigDetail(configDetail)
    return await constructProtobufRequest(url, req, userProto.CreateConfigResponse);
}

export async function updatePipelineConfig(name,version,detail){
    let url=URL_PREFIX+"UpdatePipelineConfig"
    let req = new userProto.UpdateConfigRequest()
    let configDetail=new agentProto.ConfigDetail()
    configDetail.setName(name)
    configDetail.setVersion(version)
    configDetail.setDetail(strToBytes(detail))
    req.setConfigDetail(configDetail)
    return await constructProtobufRequest(url, req, userProto.UpdateConfigResponse);
}

export async function deletePipelineConfig(name) {
    let url = URL_PREFIX + "DeletePipelineConfig"
    let req = new userProto.DeleteConfigRequest()
    req.setConfigName(name)
    return await constructProtobufRequest(url, req, userProto.DeleteConfigResponse);
}

export async function applyPipelineConfigToAgentGroup(configName,groupName){
    let url = URL_PREFIX + "ApplyPipelineConfigToAgentGroup"
    let req = new userProto.ApplyConfigToAgentGroupRequest()
    req.setConfigName(configName)
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.ApplyConfigToAgentGroupResponse);
}

export async function removePipelineConfigFromAgentGroup(configName,groupName){
    let url = URL_PREFIX + "RemovePipelineConfigFromAgentGroup"
    let req = new userProto.RemoveConfigFromAgentGroupRequest()
    req.setConfigName(configName)
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.RemoveConfigFromAgentGroupResponse);
}
export async function getAppliedAgentGroupsWithPipelineConfig(configName) {
    let url = URL_PREFIX + "GetAppliedAgentGroupsWithPipelineConfig"
    let req = new userProto.GetAppliedAgentGroupsRequest()
    req.setConfigName(configName)
    return await constructProtobufRequest(url, req, userProto.GetAppliedAgentGroupsResponse);
}