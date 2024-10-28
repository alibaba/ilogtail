import axios from "axios";
import {ElMessage} from "element-plus";
export function strToBytes(str){
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

export function base64ToStr(base64) {
    // if (isBase64(base64) && !(notBase64.includes(base64))) {
    //     return Base64.decode(base64)
    // }
    // return base64
    return decodeURIComponent(escape(atob(base64)));
}

const URL_PREFIX="/api/User/"

export {
    URL_PREFIX
}

export function messageShow(data,message) {
    if (data.commonResponse.status) {
        let msg = data.commonResponse.errorMessage
        ElMessage.error(msg)
        throw new Error(msg)
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

export function decodeBase64(obj) {
    if (typeof obj === 'object' && obj !== null) {
        for (let key in obj) {
            if (obj[key] instanceof Uint8Array) {
                obj[key] = new TextDecoder().decode(obj[key]);
            } else if (typeof obj[key] === 'object' && obj[key] !== null) {
                decodeBase64(obj[key]);
            } else if (typeof obj[key] === 'string') {
                try {
                    obj[key] = base64ToStr(obj[key]);
                } catch (e) {
                    // console.warn(`Failed to decode value for key: ${key}, value: ${obj[key]}`);
                }
            }
        }
    }

}

export function generateRandomString(length) {
    const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let result = '';
    for (let i = 0; i < length; i++) {
        result += characters.charAt(Math.floor(Math.random() * characters.length));
    }
    return result;
}

export async function constructProtobufRequest(url,req,resType) {
    let requestStr=generateRandomString(10)
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
        decodeBase64(data)
        if (data.requestId !== requestStr) {
            ElMessage.error("no the same request")
        }
        serializeResult = data
    }).catch((error) => {
        let data = resType.deserializeBinary(new Uint8Array(error.response.data)).toObject()
        decodeBase64(data)
        if (data.requestId !== requestStr) {
            ElMessage.error("no the same request")
        }
        console.log(data)
        serializeResult = data
    })
    return serializeResult
}
