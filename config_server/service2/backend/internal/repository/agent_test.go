package repository

import (
	"config-server2/internal/entity"
	"config-server2/internal/store"
	"fmt"
	"testing"
)

func TestRemoveAgentId(t *testing.T) {
	var err error
	err = store.S.Connect()
	if err != nil {
		return
	}
	err = RemoveAgentById("111")
	if err != nil {
		panic(err)
	}
}

func TestGetAgentById(t *testing.T) {
	var err error
	err = store.S.Connect()
	if err != nil {
		return
	}
	id := GetAgentByiId("11111")
	fmt.Println(id)

}

func TestAddDataPipelineConfig(t *testing.T) {
	store.S.DB.Create(entity.PipelineConfig{
		Name:    "nzh",
		Version: 1,
		Detail:  []byte(""),
	})
}
