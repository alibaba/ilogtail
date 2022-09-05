// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package common

type httpStatus struct {
	status int
	code   string
}

var (
	Accept                 httpStatus = httpStatus{200, "Accept"}
	BadRequest             httpStatus = httpStatus{400, "BadRequest"}
	ConfigAlreadyExist     httpStatus = httpStatus{400, "ConfigAlreadyExist"}
	ConfigNotExist         httpStatus = httpStatus{404, "ConfigNotExist"}
	InternalServerError    httpStatus = httpStatus{500, "InternalServerError"}
	InvalidParameter       httpStatus = httpStatus{400, "InvalidParameter"}
	AgentAlreadyExist      httpStatus = httpStatus{400, "AgentAlreadyExist"}
	AgentGroupAlreadyExist httpStatus = httpStatus{400, "AgentGroupAlreadyExist"}
	AgentGroupNotExist     httpStatus = httpStatus{404, "AgentGroupNotExist"}
	AgentNotExist          httpStatus = httpStatus{404, "AgentNotExist"}
	RequestTimeout         httpStatus = httpStatus{500, "RequestTimeout"}
	ServerBusy             httpStatus = httpStatus{503, "ServerBusy"}
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
