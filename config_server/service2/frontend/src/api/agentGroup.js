import {URL_PREFIX,constructProtobufRequest} from "@/api/common";
import userProto from "@/proto/user_pb";
import agentProto from "@/proto/agent_pb"

export async function createAgentGroup(groupName,value) {
    let url = URL_PREFIX + "CreateAgentGroup"
    let req = new userProto.CreateAgentGroupRequest()
    let group = new agentProto.AgentGroupTag()
    group.setName(groupName)
    group.setValue(value)
    req.setAgentGroup(group)
    return await constructProtobufRequest(url, req, userProto.CreateAgentGroupResponse);
}

export async function getAgentGroup(groupName) {
    let url = URL_PREFIX + "GetAgentGroup"
    let req = new userProto.GetAgentGroupRequest()
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.GetAgentGroupResponse);
}

export async function deleteAgentGroup(groupName){
    let url = URL_PREFIX + "DeleteAgentGroup"
    let req = new userProto.DeleteAgentGroupRequest()
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.DeleteAgentGroupResponse);
}

export async function updateAgentGroup(groupName,value){
    let url = URL_PREFIX + "UpdateAgentGroup"
    let req = new userProto.UpdateAgentGroupRequest()
    let group = new agentProto.AgentGroupTag()
    group.setName(groupName)
    group.setValue(value)
    req.setAgentGroup(group)
    return await constructProtobufRequest(url, req, userProto.UpdateAgentGroupResponse);
}

export async function listAgentGroups(){
    let url=URL_PREFIX+"ListAgentGroups"
    let req = new userProto.ListAgentGroupsRequest()
    return await constructProtobufRequest(url, req, userProto.ListAgentGroupsResponse);
}

export async function listAgentsForAgentGroup(groupName){
    let url=URL_PREFIX+"ListAgents"
    let req = new userProto.ListAgentsRequest()
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.ListAgentsResponse);
}

export async function getAppliedInstanceConfigsForAgentGroup(groupName){
    let url=URL_PREFIX+"GetAppliedInstanceConfigsForAgentGroup"
    let req = new userProto.GetAppliedConfigsForAgentGroupRequest()
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.GetAppliedConfigsForAgentGroupResponse);
}

export async function getAppliedPipelineConfigsForAgentGroup(groupName){
    let url=URL_PREFIX+"GetAppliedPipelineConfigsForAgentGroup"
    let req = new userProto.GetAppliedConfigsForAgentGroupRequest()
    req.setGroupName(groupName)
    return await constructProtobufRequest(url, req, userProto.GetAppliedConfigsForAgentGroupResponse);
}



