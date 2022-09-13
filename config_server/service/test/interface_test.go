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

package test

import (
	"bytes"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/gin-gonic/gin"
	. "github.com/smartystreets/goconvey/convey"
	"google.golang.org/protobuf/proto"

	configserverproto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/router"
)

func TestInterface(t *testing.T) {
	gin.SetMode(gin.TestMode)
	r := gin.New()
	router.InitAgentRouter(r)
	router.InitUserRouter(r)

	Convey("Test agent send data.", t, func() {
		Convey("Test /Agent/Heartbeat.", func() {
			// data
			reqBody := configserverproto.HeartBeatRequest{}
			reqBody.RequestId = "0"
			reqBody.AgentId = "ilogtail-1"
			reqBody.AgentType = "ilogtail"
			reqBody.AgentVersion = "1.1.1"
			reqBody.Ip = "127.0.0.1"
			reqBody.RunningStatus = "good"
			reqBody.StartupTime = 100
			reqBody.Tags = map[string]string{}
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/Agent/HeartBeat", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.HeartBeatResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Send heartbeat success")
		})

		Convey("Test /Agent/Alarm.", func() {
			// data
			reqBody := configserverproto.AlarmRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentId = "ilogtail-1"
			reqBody.Type = "test"
			reqBody.Detail = "test alarm"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/Agent/Alarm", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.AlarmResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Alarm success")
		})

		Convey("Test /Agent/RunningStatistics.", func() {
			// data
			reqBody := configserverproto.RunningStatisticsRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentId = "ilogtail-1"
			reqBody.RunningDetails = &configserverproto.RunningStatistics{Cpu: 0, Memory: 0, Extras: map[string]string{}}
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/Agent/RunningStatistics", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.RunningStatisticsResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Send running statistics success")
		})

		// wait for write to memory
		time.Sleep(time.Second * 5)
	})

	Convey("Test agent group's CURD.", t, func() {
		Convey("Test CreateAgentGroup.", func() {
			// data
			reqBody := configserverproto.CreateAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentGroup = new(configserverproto.AgentGroup)
			reqBody.AgentGroup.GroupName = "default"
			reqBody.AgentGroup.Tags = make([]*configserverproto.AgentGroupTag, 0)
			reqBody.AgentGroup.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add agent group success")

			// check data
			checkReqBody := configserverproto.GetAgentGroupRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.GroupName = "default"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetAgentGroup", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetAgentGroupResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 200)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "Accept")
			So(checkResBody.Message, ShouldEqual, "Get agent group success")
			So(checkResBody.AgentGroup.GroupName, ShouldEqual, "default")
			So(checkResBody.AgentGroup.Tags, ShouldBeEmpty)
			So(checkResBody.AgentGroup.Description, ShouldEqual, "test")
		})

		Convey("Test UpdateAgentGroup.", func() {
			// data
			reqBody := configserverproto.UpdateAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentGroup = new(configserverproto.AgentGroup)
			reqBody.AgentGroup.GroupName = "default"
			reqBody.AgentGroup.Tags = make([]*configserverproto.AgentGroupTag, 0)
			reqBody.AgentGroup.Description = "test-updated"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("PUT", "/User/UpdateAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.UpdateAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Update agent group success")

			// check data
			checkReqBody := configserverproto.GetAgentGroupRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.GroupName = "default"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetAgentGroup", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetAgentGroupResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 200)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "Accept")
			So(checkResBody.Message, ShouldEqual, "Get agent group success")
			So(checkResBody.AgentGroup.GroupName, ShouldEqual, "default")
			So(checkResBody.AgentGroup.Tags, ShouldBeEmpty)
			So(checkResBody.AgentGroup.Description, ShouldEqual, "test-updated")
		})

		Convey("Test DeleteAgentGroup.", func() {
			// data
			reqBody := configserverproto.DeleteAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete agent group success")

			// check data
			checkReqBody := configserverproto.GetAgentGroupRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.GroupName = "default"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetAgentGroup", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetAgentGroupResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 404)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "AgentGroupNotExist")
			So(checkResBody.Message, ShouldEqual, "Agent group default doesn't exist.")
		})

		Convey("Test ListAgentGroups.", func() {
			// data
			reqBody := configserverproto.ListAgentGroupsRequest{}
			reqBody.RequestId = "1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("GET", "/User/ListAgentGroups", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.ListAgentGroupsResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get agent group list success")
			So(resBody.AgentGroups, ShouldBeEmpty)
		})
	})

	Convey("Test collection config's CURD.", t, func() {
		Convey("Test CreateConfig.", func() {
			// data
			reqBody := configserverproto.CreateConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigDetail = new(configserverproto.Config)
			reqBody.ConfigDetail.ConfigName = "config-1"
			reqBody.ConfigDetail.AgentType = "ilogtail"
			reqBody.ConfigDetail.Content = "test"
			reqBody.ConfigDetail.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add config success")

			// check data
			checkReqBody := configserverproto.GetConfigRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.ConfigName = "config-1"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetConfig", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetConfigResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 200)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "Accept")
			So(checkResBody.Message, ShouldEqual, "Get config success")
			So(checkResBody.ConfigDetail.ConfigName, ShouldEqual, "config-1")
			So(checkResBody.ConfigDetail.AgentType, ShouldEqual, "ilogtail")
			So(checkResBody.ConfigDetail.Content, ShouldEqual, "test")
			So(checkResBody.ConfigDetail.Description, ShouldEqual, "test")
		})

		Convey("Test UpdateConfig.", func() {
			// data
			reqBody := configserverproto.UpdateConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigDetail = new(configserverproto.Config)
			reqBody.ConfigDetail.ConfigName = "config-1"
			reqBody.ConfigDetail.AgentType = "ilogtail"
			reqBody.ConfigDetail.Content = "test-updated"
			reqBody.ConfigDetail.Description = "test-updated"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("PUT", "/User/UpdateConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.UpdateConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Update config success")

			// check data
			checkReqBody := configserverproto.GetConfigRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.ConfigName = "config-1"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetConfig", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetConfigResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 200)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "Accept")
			So(checkResBody.Message, ShouldEqual, "Get config success")
			So(checkResBody.ConfigDetail.ConfigName, ShouldEqual, "config-1")
			So(checkResBody.ConfigDetail.AgentType, ShouldEqual, "ilogtail")
			So(checkResBody.ConfigDetail.Content, ShouldEqual, "test-updated")
			So(checkResBody.ConfigDetail.Description, ShouldEqual, "test-updated")
		})

		Convey("Test DeleteConfig.", func() {
			// data
			reqBody := configserverproto.DeleteConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete config success")

			// check data
			checkReqBody := configserverproto.GetConfigRequest{}
			checkReqBody.RequestId = "2"
			checkReqBody.ConfigName = "config-1"
			checkReqBodyByte, _ := proto.Marshal(&checkReqBody)

			// check request
			w = httptest.NewRecorder()
			checkReq, _ := http.NewRequest("GET", "/User/GetConfig", bytes.NewBuffer(checkReqBodyByte))
			r.ServeHTTP(w, checkReq)

			// check response
			checkRes := w.Result()
			checkResBodyByte, _ := ioutil.ReadAll(checkRes.Body)
			checkResBody := new(configserverproto.GetConfigResponse)
			proto.Unmarshal(checkResBodyByte, checkResBody)

			// check
			So(checkRes.StatusCode, ShouldEqual, 404)
			So(checkResBody.ResponseId, ShouldEqual, checkReqBody.RequestId)
			So(checkResBody.Code, ShouldEqual, "ConfigNotExist")
			So(checkResBody.Message, ShouldEqual, "Config config-1 doesn't exist.")
		})

		Convey("Test ListConfigs.", func() {
			// data
			reqBody := configserverproto.ListConfigsRequest{}
			reqBody.RequestId = "1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("GET", "/User/ListConfigs", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.ListConfigsResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get config list success")
			So(resBody.ConfigDetails, ShouldBeEmpty)
		})

	})

	Convey("Test operations between collection config and agent group.", t, func() {
		Convey("Create an agent group.", func() {
			// data
			reqBody := configserverproto.CreateAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentGroup = new(configserverproto.AgentGroup)
			reqBody.AgentGroup.GroupName = "default"
			reqBody.AgentGroup.Tags = make([]*configserverproto.AgentGroupTag, 0)
			reqBody.AgentGroup.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add agent group success")
		})

		Convey("Create a config.", func() {
			// data
			reqBody := configserverproto.CreateConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigDetail = new(configserverproto.Config)
			reqBody.ConfigDetail.ConfigName = "config-1"
			reqBody.ConfigDetail.AgentType = "ilogtail"
			reqBody.ConfigDetail.Content = "test"
			reqBody.ConfigDetail.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add config success")
		})

		Convey("Test ApplyConfigToAgentGroup.", func() {
			// data
			reqBody := configserverproto.ApplyConfigToAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("PUT", "/User/ApplyConfigToAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.ApplyConfigToAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add config to agent group success")
		})

		Convey("Test GetAppliedConfigsForAgentGroup.", func() {
			// data
			reqBody := configserverproto.GetAppliedConfigsForAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("GET", "/User/GetAppliedConfigsForAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.GetAppliedConfigsForAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get agent group's applied configs success")
			So(len(resBody.ConfigNames), ShouldEqual, 1)
			So(resBody.ConfigNames[0], ShouldEqual, "config-1")
		})

		Convey("Test GetAppliedAgentGroups.", func() {
			// data
			reqBody := configserverproto.GetAppliedAgentGroupsRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("GET", "/User/GetAppliedAgentGroups", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.GetAppliedAgentGroupsResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get group list success")
			So(len(resBody.AgentGroupNames), ShouldEqual, 1)
			So(resBody.AgentGroupNames[0], ShouldEqual, "default")
		})

		Convey("Test ListAgents.", func() {
			// data
			reqBody := configserverproto.ListAgentsRequest{}
			reqBody.RequestId = "1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("GET", "/User/ListAgents", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.ListAgentsResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get agent list success")
			So(len(resBody.Agents), ShouldEqual, 1)
			So(resBody.Agents[0].AgentId, ShouldEqual, "ilogtail-1")
			So(resBody.Agents[0].AgentType, ShouldEqual, "ilogtail")
			So(resBody.Agents[0].Ip, ShouldEqual, "127.0.0.1")
			So(resBody.Agents[0].StartupTime, ShouldEqual, 100)
			So(resBody.Agents[0].RunningStatus, ShouldEqual, "good")
			So(resBody.Agents[0].Version, ShouldEqual, "1.1.1")
		})

		Convey("Test RemoveConfigFromAgentGroup.", func() {
			// data
			reqBody := configserverproto.RemoveConfigFromAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/RemoveConfigFromAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.RemoveConfigFromAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Remove config from agent group success")
		})

		Convey("Delet agent group.", func() {
			// data
			reqBody := configserverproto.DeleteAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete agent group success")
		})

		Convey("Delete config.", func() {
			// data
			reqBody := configserverproto.DeleteConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete config success")
		})
	})

	Convey("Test agent recieve data.", t, func() {
		Convey("Create an agent group.", func() {
			// data
			reqBody := configserverproto.CreateAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentGroup = new(configserverproto.AgentGroup)
			reqBody.AgentGroup.GroupName = "default"
			reqBody.AgentGroup.Tags = make([]*configserverproto.AgentGroupTag, 0)
			reqBody.AgentGroup.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add agent group success")
		})

		Convey("Create a config.", func() {
			// data
			reqBody := configserverproto.CreateConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigDetail = new(configserverproto.Config)
			reqBody.ConfigDetail.ConfigName = "config-1"
			reqBody.ConfigDetail.AgentType = "ilogtail"
			reqBody.ConfigDetail.Content = "test"
			reqBody.ConfigDetail.Description = "test"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/User/CreateConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.CreateConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add config success")
		})

		Convey("Apply config to agent group.", func() {
			// data
			reqBody := configserverproto.ApplyConfigToAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("PUT", "/User/ApplyConfigToAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.ApplyConfigToAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Add config to agent group success")
		})

		Convey("Test /Agent/GetConfigList.", func() {
			// data
			reqBody := configserverproto.AgentGetConfigListRequest{}
			reqBody.RequestId = "1"
			reqBody.AgentId = "ilogtail-1"
			reqBody.ConfigVersions = map[string]int64{}
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("POST", "/Agent/GetConfigList", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.AgentGetConfigListResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Get config update infos success")
			So(len(resBody.ConfigUpdateInfos), ShouldEqual, 1)
			So(resBody.ConfigUpdateInfos[0].ConfigName, ShouldEqual, "config-1")
			So(resBody.ConfigUpdateInfos[0].UpdateStatus, ShouldEqual, configserverproto.ConfigUpdateInfo_NEW)
			So(resBody.ConfigUpdateInfos[0].Content, ShouldEqual, "test")
		})

		Convey("Remove config from agent group.", func() {
			// data
			reqBody := configserverproto.RemoveConfigFromAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/RemoveConfigFromAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.RemoveConfigFromAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Remove config from agent group success")
		})

		Convey("Delet agent group.", func() {
			// data
			reqBody := configserverproto.DeleteAgentGroupRequest{}
			reqBody.RequestId = "1"
			reqBody.GroupName = "default"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteAgentGroup", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteAgentGroupResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete agent group success")
		})

		Convey("Delete config.", func() {
			// data
			reqBody := configserverproto.DeleteConfigRequest{}
			reqBody.RequestId = "1"
			reqBody.ConfigName = "config-1"
			reqBodyByte, _ := proto.Marshal(&reqBody)

			// request
			w := httptest.NewRecorder()
			req, _ := http.NewRequest("DELETE", "/User/DeleteConfig", bytes.NewBuffer(reqBodyByte))
			r.ServeHTTP(w, req)

			// response
			res := w.Result()
			resBodyByte, _ := ioutil.ReadAll(res.Body)
			resBody := new(configserverproto.DeleteConfigResponse)
			proto.Unmarshal(resBodyByte, resBody)

			// check
			So(res.StatusCode, ShouldEqual, 200)
			So(resBody.ResponseId, ShouldEqual, reqBody.RequestId)
			So(resBody.Code, ShouldEqual, "Accept")
			So(resBody.Message, ShouldEqual, "Delete config success")
		})
	})
}
