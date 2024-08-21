package store

import (
	"config-server2/internal/entity"
	"fmt"
	"testing"
)

func TestRemoveAgentId(t *testing.T) {
	var err error
	err = S.Connect()
	if err != nil {
		return
	}
	err = S.RemoveAgentById("111")
	if err != nil {
		panic(err)
	}
}

func TestGetAllAgents(t *testing.T) {

	//err = S.Connect()
	//if err != nil {
	//	return
	//}
	var list []entity.Agent
	list = S.GetAllAgentsBasicInfo()
	fmt.Println(list)
}

func TestGetAgentById(t *testing.T) {
	var err error
	err = S.Connect()
	if err != nil {
		return
	}
	id := S.GetAgentByiId("11111")
	fmt.Println(id)

}

func TestAddDataPipelineConfig(t *testing.T) {
	S.DB.Create(entity.PipelineConfig{
		Name:    "nzh",
		Version: 1,
		Detail:  []byte(""),
	})
}
