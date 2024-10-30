
export function logRefObj(obj){
    if (obj===undefined || obj===null){
        console.log(obj)
        return
    }
    console.log(JSON.parse(JSON.stringify(obj)))
}