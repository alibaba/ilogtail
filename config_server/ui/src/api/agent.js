import {constructProtobufRequest, URL_PREFIX} from "@/api/common";
import userProto from "@/proto/user_pb";

export async function getPipelineConfigStatusList(instanceId){
    let url=URL_PREFIX+"GetPipelineConfigStatusList"
    let req = new userProto.GetConfigStatusListRequest()
    req.setInstanceId(instanceId)
    return await constructProtobufRequest(url, req, userProto.GetConfigStatusListResponse)
}

export async function getInstanceConfigStatusList(instanceId){
    let url=URL_PREFIX+"GetInstanceConfigStatusList"
    let req = new userProto.GetConfigStatusListRequest()
    req.setInstanceId(instanceId)
    return await constructProtobufRequest(url, req, userProto.GetConfigStatusListResponse)
}