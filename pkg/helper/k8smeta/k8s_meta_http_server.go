package k8smeta

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	cs "github.com/alibabacloud-go/cs-20151215/v5/client"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	"github.com/alibabacloud-go/tea/tea"
	v1 "k8s.io/api/core/v1"
)

type requestBody struct {
	Keys []string `json:"keys"`
}

type metadataHandler struct {
	metaManager *MetaManager
}

func NewMetadataHandler() *metadataHandler {
	metadataHandler := &metadataHandler{
		metaManager: GetMetaManagerInstance(),
	}
	return metadataHandler
}

func (m *metadataHandler) K8sServerRun(stopCh <-chan struct{}) error {
	portEnv := os.Getenv("KUBERNETES_METADATA_PORT")
	if len(portEnv) == 0 {
		logger.Error(context.Background(), "K8S_META_PORT_NOT_SET", "KUBERNETES_METADATA_PORT is not set")
		return fmt.Errorf("KUBERNETES_METADATA_PORT is not set")
	}
	port, err := strconv.Atoi(portEnv)
	if err != nil {
		logger.Error(context.Background(), "K8S_META_PORT_INVALID", "KUBERNETES_METADATA_PORT is not a valid port number")
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
	for {
		if m.metaManager.ready.Load() {
			break
		}
		time.Sleep(1 * time.Second)
		logger.Warning(context.Background(), "K8S_META_SERVER_WAIT", "waiting for k8s meta manager to be ready")
	}
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
	metadata := make(map[string]*PodMetadata)
	for _, key := range rBody.Keys {
		events := m.metaManager.PodProcessor.Get(key)
		podMetadata := convertObj2PodMetadata(key, events)
		for k, v := range podMetadata {
			metadata[k] = v
		}
	}
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
	if response.Body.SubnetCidr == nil {
		return "172.16.0.0/12", nil
	}
	return *response.Body.SubnetCidr, nil
}

func convertObj2PodMetadata(key string, events []*K8sMetaEvent) map[string]*PodMetadata {
	result := make(map[string]*PodMetadata)
	for _, event := range events {
		pod := event.RawObject.(*v1.Pod)
		images := make(map[string]string)
		for _, container := range pod.Spec.Containers {
			images[container.Name] = container.Image
		}
		envs := make(map[string]string)
		for _, container := range pod.Spec.Containers {
			for _, env := range container.Env {
				envs[env.Name] = env.Value
			}
		}
		podMetadata := &PodMetadata{
			Namespace: pod.Namespace,
			Labels:    pod.Labels,
			Images:    images,
			Envs:      envs,
			IsDeleted: false,
		}
		if len(pod.GetOwnerReferences()) == 0 {
			podMetadata.WorkloadName = ""
			podMetadata.WorkloadKind = ""
			logger.Warning(context.Background(), "Pod has no owner", pod.Name)
		} else {
			podMetadata.WorkloadName = pod.GetOwnerReferences()[0].Name
			podMetadata.WorkloadKind = strings.ToLower(pod.GetOwnerReferences()[0].Kind)
		}
		if len(events) > 1 { // pod ip, container id ...
			result[pod.Status.PodIP] = podMetadata
		} else { // host ip ...
			result[key] = podMetadata
		}
	}
	return result
}
