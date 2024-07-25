package k8smeta

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"strconv"

	cs "github.com/alibabacloud-go/cs-20151215/v5/client"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	"github.com/alibabacloud-go/tea/tea"
)

type requestBody struct {
	Keys []string `json:"keys"`
}

type metadataHandler struct {
	watchCache *WatchCache
}

func NewMetadataHandler(store K8sMetaStore) *metadataHandler {
	metadataHandler := &metadataHandler{
		watchCache: newHttpServerWatchCache(GetMetaManagerInstance(), store),
	}
	return metadataHandler
}

func (m *metadataHandler) K8sServerRun(stopCh <-chan struct{}) error {
	portEnv := os.Getenv("KUBERNETES_METADATA_PORT")
	if len(portEnv) == 0 {
		return nil
	}
	port, err := strconv.Atoi(portEnv)
	if err != nil {
		return fmt.Errorf("KUBERNETES_METADATA_PORT is not a valid port number")
	}
	server := &http.Server{ //nolint:gosec
		Addr: ":" + strconv.Itoa(port),
	}
	mux := http.NewServeMux()

	// TODO: add port in ip endpoint
	mux.HandleFunc("/metadata/ip", m.handlePodMeta)
	mux.HandleFunc("/metadata/containerid", m.handlePodMeta)
	mux.HandleFunc("/metadata/host", m.handlePodMeta)
	mux.HandleFunc("/metadata/cidr", m.handleCIDR)
	server.Handler = mux
	go func() {
		_ = server.ListenAndServe()
	}()
	<-stopCh
	return nil
}

func (m *metadataHandler) handlePodMeta(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()
	var rBody requestBody
	// Decode the JSON data into the struct
	err := json.NewDecoder(r.Body).Decode(&rBody)
	if err != nil {
		http.Error(w, "Error parsing JSON: "+err.Error(), http.StatusBadRequest)
		return
	}

	// Get the metadata
	metadata := m.watchCache.getPodMetadata(rBody.Keys)
	// Convert metadata to JSON
	metadataJSON, err := json.Marshal(metadata)
	if err != nil {
		http.Error(w, "Error converting metadata to JSON: "+err.Error(), http.StatusInternalServerError)
		return
	}
	// Set the response content type to application/json
	w.Header().Set("Content-Type", "application/json")
	// Write the metadata JSON to the response body
	_, err = w.Write(metadataJSON)
	if err != nil {
		http.Error(w, "Error writing response: "+err.Error(), http.StatusInternalServerError)
		return
	}
}

func (m *metadataHandler) handleCIDR(w http.ResponseWriter, r *http.Request) {
	cidr, err := getPodCIDR()
	if err != nil {
		http.Error(w, "Error getting pod CIDR: "+err.Error(), http.StatusInternalServerError)
		return
	}
	cidrJSON, err := json.Marshal(map[string]string{"cidr": cidr})
	if err != nil {
		http.Error(w, "Error converting metadata to JSON: "+err.Error(), http.StatusInternalServerError)
		return
	}
	// Set the response content type to application/json
	w.Header().Set("Content-Type", "application/json")
	// Write the metadata JSON to the response body
	_, err = w.Write(cidrJSON)
	if err != nil {
		http.Error(w, "Error writing response: "+err.Error(), http.StatusInternalServerError)
		return
	}
}

func getPodCIDR() (string, error) {
	clusterId := os.Getenv("ALICLOUD_ACK_CLUSTER_ID")
	if len(clusterId) == 0 {
		return "", fmt.Errorf("ALICLOUD_ACK_CLUSTER_ID is not set")
	}
	config := &openapi.Config{
		AccessKeyId:     tea.String(os.Getenv("ALICLOUD_ACCESS_KEY_ID")),
		AccessKeySecret: tea.String(os.Getenv("ALICLOUD_ACCESS_KEY_SECRET")),
		Endpoint:        tea.String(os.Getenv("ALICLOUD_ENDPOINT")),
	}
	client, err := cs.NewClient(config)
	if err != nil {
		return "", err
	}
	response, err := client.DescribeClusterDetail(&clusterId)
	if err != nil {
		return "", err
	}
	fmt.Println(response.Body)
	return *response.Body.SubnetCidr, nil
}
