import protoRoot from "@/proto/proto.js";
import axios from "axios";

// function getRandomBytes(length) {
//     const array = new Uint8Array(length);
//     window.crypto.getRandomValues(array);
//     return array;
// }

function bytesToString(bytes) {
    const decoder = new TextDecoder('utf-8');
    return decoder.decode(bytes);
}

export async function test() {
    let url = "/api/User/GetAgentGroup"
    let reqNamespace = protoRoot.lookup("GetAgentGroupRequest")
    let req = reqNamespace.create()
    req.groupName="default"
    req = reqNamespace.encode(req).finish()

    axios.post(url, req, {
        responseType: 'arraybuffer',  // 指定返回的数据是二进制
        headers: {
            'Content-Type': 'application/x-protobuf',
            'Accept': 'application/x-protobuf'
        }
    }).then(response => {
        // 解析响应的 protobuf 数据
        const responseBuffer = new Uint8Array(response.data);
        const decodedResponse = protoRoot.lookup("GetAgentGroupResponse").decode(responseBuffer)
        console.log('Decoded Response:', decodedResponse);
        console.log(bytesToString(decodedResponse.commonResponse.errorMessage))
    }).catch(error => {
        console.error('Error:', error);
    });
}