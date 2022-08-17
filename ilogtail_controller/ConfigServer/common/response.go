package common

const (
	BadRequest               string = "BadRequest"
	ConfigAlreadyExist       string = "ConfigAlreadyExist"
	ConfigNotExist           string = "ConfigNotExist"
	InternalServerError      string = "InternalServerError"
	InvalidParameter         string = "InvalidParameter"
	MachineAlreadyExist      string = "MachineAlreadyExist"
	MachineGroupAlreadyExist string = "MachineGroupAlreadyExist"
	MachineGroupNotExist     string = "MachineGroupNotExist"
	MachineNotExist          string = "MachineNotExist"
	RequestTimeout           string = "RequestTimeout"
	ServerBusy               string = "ServerBusy"
)

type Response struct {
	Message string `json:"message"`
}

type ErrorResponse struct {
	ErrorCode    string `json:"errorCode"`
	ErrorMessage string `json:"errorMessage"`
}

func Accept(msg string) Response {
	return Response{msg}
}

func Error(code string, msg string) ErrorResponse {
	return ErrorResponse{code, msg}
}
