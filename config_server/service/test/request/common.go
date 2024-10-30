package request

import (
	"bytes"
	"config-server/config"
	"fmt"
	"google.golang.org/protobuf/proto"
	"io"
	"net/http"
)

var (
	UserUrlPrefix  = fmt.Sprintf("http://%s/User", config.ServerConfigInstance.Address)
	AgentUrlPrefix = fmt.Sprintf("http://%s/Agent", config.ServerConfigInstance.Address)
)

const (
	GET    = "GET"
	POST   = "POST"
	DELETE = "DELETE"
	PUT    = "PUT"
)

func sendProtobufRequest[T, S proto.Message](url string, methodType string, req T, res S) error {
	var err error
	data, err := proto.Marshal(req)
	if err != nil {
		return fmt.Errorf("failed to marshal request: %w", err)
	}

	httpReq, err := http.NewRequest(methodType, url, bytes.NewReader(data))
	if err != nil {
		return fmt.Errorf("failed to create request: %w", err)
	}
	httpReq.Header.Set("Content-Type", "application/x-protobuf")

	client := &http.Client{}
	resp, err := client.Do(httpReq)
	if err != nil {
		return fmt.Errorf("failed to send request: %w", err)
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("failed to read response body: %w", err)
	}

	if err = proto.Unmarshal(body, res); err != nil {
		return fmt.Errorf("failed to unmarshal response: %w", err)
	}
	return nil
}
