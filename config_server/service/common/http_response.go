package common

type httpStatus struct {
	status int
	code   string
}

var (
	Accept                   httpStatus = httpStatus{200, "Accept"}
	BadRequest               httpStatus = httpStatus{400, "BadRequest"}
	ConfigAlreadyExist       httpStatus = httpStatus{400, "ConfigAlreadyExist"}
	ConfigNotExist           httpStatus = httpStatus{404, "ConfigNotExist"}
	InternalServerError      httpStatus = httpStatus{500, "InternalServerError"}
	InvalidParameter         httpStatus = httpStatus{400, "InvalidParameter"}
	MachineAlreadyExist      httpStatus = httpStatus{400, "MachineAlreadyExist"}
	MachineGroupAlreadyExist httpStatus = httpStatus{400, "MachineGroupAlreadyExist"}
	MachineGroupNotExist     httpStatus = httpStatus{404, "MachineGroupNotExist"}
	MachineNotExist          httpStatus = httpStatus{404, "MachineNotExist"}
	RequestTimeout           httpStatus = httpStatus{500, "RequestTimeout"}
	ServerBusy               httpStatus = httpStatus{503, "ServerBusy"}
)

type Response struct {
	Code    string                 `json:"code"`
	Message string                 `json:"message"`
	Data    map[string]interface{} `json:"data"`
}

type Error struct {
	ErrorCode    string `json:"errorCode"`
	ErrorMessage string `json:"errorMessage"`
}

func AcceptResponse(s httpStatus, msg string, data map[string]interface{}) (int, Response) {
	return s.status, Response{s.code, msg, data}
}

func ErrorResponse(s httpStatus, msg string) (int, Error) {
	return s.status, Error{s.code, msg}
}
