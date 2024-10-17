import axios from "axios";
import {ElMessage} from "element-plus";
import {Base64} from "js-base64";
import {isBase64} from "is-base64"
export function strToBytes(str){
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

export function base64ToStr(base64){
    if(isBase64(base64)){
        return Base64.decode(base64)
    }
    return base64
}

const URL_PREFIX="/api/User/"

export {
    URL_PREFIX
}

export function messageShow(data,message) {
    if (data.commonResponse.status) {
        ElMessage.error(data.commonResponse.errorMessage)
    } else {
        if (!message) {
            message = "success"
        }
        ElMessage({
            message: message,
            type: 'success',
        })
    }
}



export async function constructProtobufRequest(url,req,resType) {
    let requestStr = "114514114514"
    req.setRequestId(strToBytes(requestStr))
    let bytes = req.serializeBinary()
    let serializeResult
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
        let data = resType.deserializeBinary(new Uint8Array(res.data)).toObject()
        data.requestId = atob(data.requestId)
        data.commonResponse.errorMessage = atob(data.commonResponse.errorMessage)
        if (data.requestId !== requestStr) {
            console.log(data.requestId)
            console.log(requestStr)
            ElMessage.error("no the same request")
        }
        serializeResult = data
    }).catch((error) => {
        console.log("find err:",error)
    })
    return serializeResult
}
