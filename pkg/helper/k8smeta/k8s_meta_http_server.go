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

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type requestBody struct {
	Keys []string `json:"keys"`
}

type metadataHandler struct {
	metaManager *MetaManager
}

func newMetadataHandler() *metadataHandler {
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
	server.Handler = mux
	for {
		if m.metaManager.ready.Load() {
			break
		}
		time.Sleep(1 * time.Second)
		logger.Warning(context.Background(), "K8S_META_SERVER_WAIT", "waiting for k8s meta manager to be ready")
	}
	logger.Info(context.Background(), "k8s meta server", "started", "port", port)
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
	events := m.metaManager.PodProcessor.Get(rBody.Keys)
	for key, objs := range events {
		podMetadata := convertObj2PodMetadata(key, objs)
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
