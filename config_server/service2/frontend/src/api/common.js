import protoRoot from "@/proto/user_pb.js";
import axios from "axios";

function strToBytes(str){
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

export async function test() {
    let url = "/api/User/GetAgentGroup"
    let message = new protoRoot.GetAgentGroupRequest()
    message.setRequestId(strToBytes("我是共产主义接班"))
    message.setGroupName("nzh")
    let bytes = message.serializeBinary()
    await axios({
        method: 'post',
        url: url,
        data: bytes,
        headers: {
            'Content-Type': 'application/x-protobuf',
            'Accept': 'application/x-protobuf'
        },
        responseType: 'arraybuffer'
    }).then(res => {
        let data = protoRoot.GetAgentGroupResponse.deserializeBinary(new Uint8Array(res.data)).toObject()
        console.log(data)
        if (data.commonResponse.status) {
            console.log(atob(data.commonResponse.errorMessage))
        }
    }).catch((error) => {
        console.log(error)
    })
}