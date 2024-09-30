package router

import (
	"bytes"
	"config-server/config"
	"config-server/protov2"
	"fmt"
	"google.golang.org/protobuf/proto"
	"io"
	"log"
	"net/http"
	"testing"
)

var UserUrlPrefix = fmt.Sprintf("http://%s/User", config.ServerConfigInstance.Address)

const (
	GET    = "GET"
	POST   = "POST"
	DELETE = "DELETE"
	PUT    = "PUT"
)

func sendProtobufRequest[T, S proto.Message](url string, methodType string, req T, res S) error {

	// 序列化 Protobuf 消息
	data, err := proto.Marshal(req)
	if err != nil {
		return fmt.Errorf("failed to marshal request: %w", err)
	}

	// 创建 HTTP 请求
	httpReq, err := http.NewRequest(methodType, url, bytes.NewReader(data))
	if err != nil {
		return fmt.Errorf("failed to create request: %w", err)
	}

	// 设置请求头
	httpReq.Header.Set("Content-Type", "application/x-protobuf")

	// 发送请求
	client := &http.Client{}
	resp, err := client.Do(httpReq)
	if err != nil {
		return fmt.Errorf("failed to send request: %w", err)
	}
	defer resp.Body.Close()

	// 读取响应
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("failed to read response body: %w", err)
	}

	if err := proto.Unmarshal(body, res); err != nil {
		return fmt.Errorf("failed to unmarshal response: %w", err)
	}
	fmt.Println(res)
	return nil
}

func TestListAgents(t *testing.T) {
	url := fmt.Sprintf("%s/ApplyPipelineConfigToAgentGroup", UserUrlPrefix)

	req := &protov2.ApplyConfigToAgentGroupRequest{}
	res := &protov2.ApplyConfigToAgentGroupResponse{}

	req.RequestId = []byte("123456")
	req.GroupName = "nzh"
	req.ConfigName = "nzh"

	if err := sendProtobufRequest(url, PUT, req, res); err != nil {
		log.Fatalf("Request failed: %v", err)
	}
}
