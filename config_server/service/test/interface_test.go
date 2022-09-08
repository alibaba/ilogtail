package test

import (
	"bytes"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/gin-gonic/gin"
	. "github.com/smartystreets/goconvey/convey"
	"google.golang.org/protobuf/proto"

	configserverproto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/router"
)

func TestAgentInterfce(t *testing.T) {
	gin.SetMode(gin.TestMode)
	r := gin.New()
	router.InitAgentRouter(r)

	Convey("Test HeartBeat.", t, func() {
		// data
		reqBody := configserverproto.HeartBeatRequest{}
		reqBody.AgentId = "config-1"
		reqBody.AgentType = "ilogtail"
		reqBody.AgentVersion = "1.1.1"
		reqBody.Ip = "127.0.0.1"
		reqBody.RequestId = "1"
		reqBody.RunningStatus = "good"
		reqBody.StartupTime = 0
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
		So(resBody.ResponseId, ShouldEqual, "1")
		So(resBody.Code, ShouldEqual, "Accept")
		So(resBody.Message, ShouldEqual, "Send heartbeat success")
	})
}

func TestUserInterface(t *testing.T) {

}
