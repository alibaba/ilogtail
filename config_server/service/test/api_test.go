package test

import (
	"config-server/common"
	"config-server/protov2"
	"config-server/test/request"
	"fmt"
	. "github.com/smartystreets/goconvey/convey"
	"math/rand"
	"testing"
	"time"
)

const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

func generateRandomNum(length int) []byte {
	seededRand := rand.New(rand.NewSource(time.Now().UnixNano()))
	b := make([]byte, length)
	for i := range b {
		b[i] = charset[seededRand.Intn(len(charset))]
	}
	return b
}

func TestBaseAgent(t *testing.T) {
	requestId := generateRandomNum(10)
	Convey("Test delete AgentGroup.", t, func() {
		fmt.Print("\n" + "Test delete default. ")
		{
			res, _ := request.DeleteAgentGroup(requestId, "default")
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.InvalidParameter.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.InvalidParameter.Message)

		}
	})
}

func TestBasePipelineConfig(t *testing.T) {

	var configLength int

	Convey("Test create PipelineConfig.", t, func() {

		fmt.Print("\n" + "Test get all configs. ")
		{
			requestId := generateRandomNum(10)
			res, _ := request.GetAllPipelineConfig(requestId)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			configLength = len(res.ConfigDetails)
		}

		fmt.Print("\n" + "Test create config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:   "config-1",
				Detail: []byte("Detail for test config-1"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.InvalidParameter.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.InvalidParameter.Message)
		}

		fmt.Print("\n" + "Test create config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-1",
				Version: 1,
				Detail:  []byte("Detail for test config-1"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(res.ConfigDetail.Name, ShouldEqual, configName)
			So(res.ConfigDetail.Version, ShouldEqual, 1)
			So(string(res.ConfigDetail.Detail), ShouldEqual, "Detail for test config-1")
		}

		fmt.Print("\n" + "Test create config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-1",
				Version: 1,
				Detail:  []byte("Detail for test config-1"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigAlreadyExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigAlreadyExist.Message)
		}

		fmt.Print("\n" + "Test get all configs. ")
		{
			requestId := generateRandomNum(10)
			res, _ := request.GetAllPipelineConfig(requestId)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigDetails), ShouldEqual, configLength+1)
		}

		fmt.Print("\n" + "Test create config-2. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-2",
				Version: 1,
				Detail:  []byte("Detail for test config-2"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test create config-2. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-2",
				Version: 1,
				Detail:  []byte("Detail for test config-2"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigAlreadyExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigAlreadyExist.Message)
		}

		fmt.Print("\n" + "Test get config-2. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(res.ConfigDetail.Name, ShouldEqual, configName)
			So(res.ConfigDetail.Version, ShouldEqual, 1)
			So(string(res.ConfigDetail.Detail), ShouldEqual, "Detail for test config-2")
		}

	})

	Convey("Test update PipelineConfig.", t, func() {

		fmt.Print("\n" + "Test update config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-1",
				Version: 1,
				Detail:  []byte("Detail for update config-1"),
			}
			res, _ := request.UpdatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(res.ConfigDetail.Name, ShouldEqual, configName)
			So(res.ConfigDetail.Version, ShouldEqual, 2)
			So(string(res.ConfigDetail.Detail), ShouldEqual, "Detail for update config-1")
		}

		fmt.Print("\n" + "Test update config-2. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-2",
				Version: 1,
				Detail:  []byte("Detail for update config-2"),
			}
			res, _ := request.UpdatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get config-2. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(res.ConfigDetail.Name, ShouldEqual, configName)
			So(res.ConfigDetail.Version, ShouldEqual, 2)
			So(string(res.ConfigDetail.Detail), ShouldEqual, "Detail for update config-2")
		}

		fmt.Print("\n" + "Test get config-3. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-3"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigNotExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigNotExist.Message)
		}

		fmt.Print("\n" + "Test get all configs. ")
		{
			requestId := generateRandomNum(10)
			res, _ := request.GetAllPipelineConfig(requestId)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigDetails), ShouldEqual, configLength+2)
		}

	})

	Convey("Test delete PipelineConfig.", t, func() {

		fmt.Print("\n" + "Test delete config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.DeletePipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test get config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.GetPipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigNotExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigNotExist.Message)
		}

		fmt.Print("\n" + "Test create config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-1",
				Version: 1,
				Detail:  []byte("Detail for test config-1"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test get config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.GetPipelineConfig(requestId, configName)

			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(res.ConfigDetail.Name, ShouldEqual, configName)
			So(res.ConfigDetail.Version, ShouldEqual, 1)
			So(string(res.ConfigDetail.Detail), ShouldEqual, "Detail for test config-1")
		}

		fmt.Print("\n" + "Test delete config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.DeletePipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test get config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.GetPipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigNotExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigNotExist.Message)
		}

		fmt.Print("\n" + "Test delete config-2. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"

			res, _ := request.DeletePipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test get config-2. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"

			res, _ := request.GetPipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, common.ConfigNotExist.Code)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, common.ConfigNotExist.Message)
		}

		fmt.Print("\n" + "Test get all configs. ")
		{
			requestId := generateRandomNum(10)
			res, _ := request.GetAllPipelineConfig(requestId)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigDetails), ShouldEqual, configLength)
		}
	})
}

func TestOperationBetweenAgentGroupAndPipelineConfig(t *testing.T) {
	Convey("Test apply PipelineConfig.", t, func() {
		var defaultGroupConfigLength int
		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			defaultGroupConfigLength = len(res.ConfigNames)
		}

		fmt.Print("\n" + "Test create config-1. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-1",
				Version: 1,
				Detail:  []byte("Detail for test config-1"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test create config-2. ")
		{
			requestId := generateRandomNum(10)
			detail := &protov2.ConfigDetail{
				Name:    "config-2",
				Version: 1,
				Detail:  []byte("Detail for test config-2"),
			}
			res, _ := request.CreatePipelineConfig(requestId, detail)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test apply config-1 to default. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"
			groupName := "default"
			res, _ := request.ApplyPipelineConfig2AgentGroup(requestId, configName, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigNames), ShouldEqual, defaultGroupConfigLength+1)
		}

		fmt.Print("\n" + "Test get config-1's AgentGroups. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"
			res, _ := request.GetAppliedAgentGroupForPipelineConfig(requestId, configName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test apply config-2 to default. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"
			groupName := "default"
			res, _ := request.ApplyPipelineConfig2AgentGroup(requestId, configName, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigNames), ShouldEqual, defaultGroupConfigLength+2)
		}

	})

	Convey("Test remove PipelineConfig or relationship between config and agentGroup", t, func() {
		var defaultGroupConfigLength int
		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			defaultGroupConfigLength = len(res.ConfigNames)
		}

		fmt.Print("\n" + "Test delete config-1. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-1"

			res, _ := request.DeletePipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")

		}

		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigNames), ShouldEqual, defaultGroupConfigLength-1)
		}

		fmt.Print("\n" + "Test remove config-2 from default. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			configName := "config-2"
			res, _ := request.RemovePipelineConfigFromAgentGroup(requestId, configName, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

		fmt.Print("\n" + "Test get default's Configs. ")
		{
			requestId := generateRandomNum(10)
			groupName := "default"
			res, _ := request.GetAppliedPipelineConfigForAgentGroup(requestId, groupName)
			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
			So(len(res.ConfigNames), ShouldEqual, defaultGroupConfigLength-2)
		}

		fmt.Print("\n" + "Test delete config-2. ")
		{
			requestId := generateRandomNum(10)
			configName := "config-2"

			res, _ := request.DeletePipelineConfig(requestId, configName)

			// check
			So(res.RequestId, ShouldEqual, requestId)
			So(res.CommonResponse.Status, ShouldEqual, 0)
			So(string(res.CommonResponse.ErrorMessage), ShouldEqual, "")
		}

	})
}

func TestAgentSendMessage(t *testing.T) {

}

func TestAgentFetchPipelineConfig(t *testing.T) {

}
