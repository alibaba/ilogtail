import axios from "axios";
import {ElMessage} from "element-plus";
import {Base64} from "js-base64";
import {isBase64} from "is-base64"
import {logRefObj} from "@/utils/log";
export function strToBytes(str){
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

let notBase64=["osDetail"]

export function base64ToStr(base64) {
    if (isBase64(base64) && !(notBase64.includes(base64))) {
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

export function decodeBase64(obj) {
    if (typeof obj === 'object' && obj !== null) {
        for (let key in obj) {
            if (obj[key] instanceof Uint8Array){
                obj[key]=new TextDecoder().decode(obj[key]);
            } else if (typeof obj[key] === 'object' && obj[key] !== null) {
                decodeBase64(obj[key]);
            } else if (typeof obj[key] === 'string') {
                try {
                    obj[key] = base64ToStr(obj[key]);
                } catch (e) {
                    console.warn(`Failed to decode value for key: ${key}, value: ${obj[key]}`);
                }
            }

        }
    }

}


// export function decodeBase64(data){
//     if (typeof data === 'string') {
//         // 如果数据是字符串且符合 Base64 编码格式，则解码
//         try {
//             return base64ToStr(data);
//         } catch (e) {
//             return data; // 如果解码失败，返回原始值
//         }
//     } else if (Array.isArray(data)) {
//         // 如果是数组，递归解码每个元素
//         return data.map(decodeBase64);
//     } else if (typeof data === 'object' && data !== null) {
//         // 如果是对象，递归解码每个键值对
//         const decodedObj = {};
//         for (const key in data) {
//             decodedObj[key] = decodeBase64(data[key]);
//         }
//         return decodedObj;
//     }
//     return data; // 如果不是字符串、数组或对象，直接返回
// }



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
        decodeBase64(data)
        if (data.requestId !== requestStr) {
            ElMessage.error("no the same request")
        }
        serializeResult = data
    }).catch((error) => {
        console.log("find err:",error)
    })
    return serializeResult
}
