package k8smeta

import (
	"context"
	"encoding/json"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	app "k8s.io/api/apps/v1"
	v1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/labels"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type requestBody struct {
	Keys []string `json:"keys"`
}

type metadataHandler struct {
	metaManager *MetaManager
}

func newMetadataHandler(metaManager *MetaManager) *metadataHandler {
	metadataHandler := &metadataHandler{
		metaManager: metaManager,
	}
	return metadataHandler
}

func (m *metadataHandler) K8sServerRun(stopCh <-chan struct{}) error {
	defer panicRecover()
	portEnv := os.Getenv("KUBERNETES_METADATA_PORT")
	if len(portEnv) == 0 {
		portEnv = "9000"
	}
	port, err := strconv.Atoi(portEnv)
	if err != nil {
		port = 9000
	}
	server := &http.Server{ //nolint:gosec
		Addr: ":" + strconv.Itoa(port),
	}
	mux := http.NewServeMux()

	// TODO: add port in ip endpoint
	mux.HandleFunc("/metadata/ip", m.handler(m.handlePodMetaByPodIP))
	mux.HandleFunc("/metadata/containerid", m.handler(m.handlePodMetaByContainerID))
	mux.HandleFunc("/metadata/host", m.handler(m.handlePodMetaByHostIP))
	server.Handler = mux
	logger.Info(context.Background(), "k8s meta server", "started", "port", port)
	go func() {
		defer panicRecover()
		_ = server.ListenAndServe()
	}()
	<-stopCh
	return nil
}

func (m *metadataHandler) handler(handleFunc func(w http.ResponseWriter, r *http.Request)) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if !m.metaManager.IsReady() {
			w.WriteHeader(http.StatusServiceUnavailable)
			return
		}
		startTime := time.Now()
		m.metaManager.httpRequestCount.Add(1)
		handleFunc(w, r)
		latency := time.Since(startTime).Milliseconds()
		m.metaManager.httpAvgDelayMs.Add(latency)
		m.metaManager.httpMaxDelayMs.Set(float64(latency))
	}
}

func (m *metadataHandler) handlePodMetaByPodIP(w http.ResponseWriter, r *http.Request) {
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
		objs := m.metaManager.cacheMap[POD].Get([]string{key})
		if len(objs) == 0 {
			// try service IP
			svcObjs := m.metaManager.cacheMap[SERVICE].Get([]string{key})
			for key, svcObj := range svcObjs {
				service, ok := svcObj[0].Raw.(*v1.Service)
				if !ok {
					continue
				}
				lm := newLabelMatcher(service, labels.SelectorFromSet(service.Spec.Selector))
				podObjs := m.metaManager.cacheMap[POD].Filter(func(ow *ObjectWrapper) bool {
					pod := ow.Raw.(*v1.Pod)
					if pod.Namespace != service.Namespace {
						return false
					}
					return lm.selector.Matches(labels.Set(pod.Labels))
				}, 1)
				if len(podObjs) != 0 {
					podMetadata := m.convertObj2PodResponse(podObjs[0])
					podMetadata.ServiceName = service.Name
					metadata[key] = podMetadata
				}
			}
		} else {
			for key, obj := range objs {
				podMetadata := m.convertObj2PodResponse(obj[0])
				metadata[key] = podMetadata
			}
		}
	}
	wrapperResponse(w, metadata)
}

func (m *metadataHandler) convertObj2PodResponse(obj *ObjectWrapper) *PodMetadata {
	pod := obj.Raw.(*v1.Pod)
	podMetadata := m.getCommonPodMetadata(pod)
	containerIDs := make([]string, 0)
	for _, container := range pod.Status.ContainerStatuses {
		containerIDs = append(containerIDs, truncateContainerID(container.ContainerID))
	}
	podMetadata.ContainerIDs = containerIDs
	podMetadata.PodIP = pod.Status.PodIP
	return podMetadata
}

func (m *metadataHandler) handlePodMetaByContainerID(w http.ResponseWriter, r *http.Request) {
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
	objs := m.metaManager.cacheMap[POD].Get(rBody.Keys)
	for key, obj := range objs {
		podMetadata := m.convertObjs2ContainerResponse(obj)
		if len(podMetadata) > 1 {
			logger.Warning(context.Background(), "Multiple pods found for unique container ID", key)
		}
		if len(podMetadata) > 0 {
			metadata[key] = podMetadata[0]
		}
	}
	wrapperResponse(w, metadata)
}

func (m *metadataHandler) convertObjs2ContainerResponse(objs []*ObjectWrapper) []*PodMetadata {
	metadatas := make([]*PodMetadata, 0)
	for _, obj := range objs {
		pod := obj.Raw.(*v1.Pod)
		podMetadata := m.getCommonPodMetadata(pod)
		podMetadata.PodIP = pod.Status.PodIP
		metadatas = append(metadatas, podMetadata)
	}
	return metadatas
}

func (m *metadataHandler) handlePodMetaByHostIP(w http.ResponseWriter, r *http.Request) {
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
	objs := m.metaManager.cacheMap[POD].Get(rBody.Keys)
	for _, obj := range objs {
		podMetadata := m.convertObjs2HostResponse(obj)
		for i, meta := range podMetadata {
			pod := obj[i].Raw.(*v1.Pod)
			metadata[pod.Status.PodIP] = meta
		}
	}
	wrapperResponse(w, metadata)
}

func (m *metadataHandler) convertObjs2HostResponse(objs []*ObjectWrapper) []*PodMetadata {
	metadatas := make([]*PodMetadata, 0)
	for _, obj := range objs {
		pod := obj.Raw.(*v1.Pod)
		podMetadata := m.getCommonPodMetadata(pod)
		containerIDs := make([]string, 0)
		for _, container := range pod.Status.ContainerStatuses {
			containerIDs = append(containerIDs, truncateContainerID(container.ContainerID))
		}
		podMetadata.ContainerIDs = containerIDs
		metadatas = append(metadatas, podMetadata)
	}
	return metadatas
}

func (m *metadataHandler) getCommonPodMetadata(pod *v1.Pod) *PodMetadata {
	images := make(map[string]string)
	envs := make(map[string]string)
	for _, container := range pod.Spec.Containers {
		images[container.Name] = container.Image
		for _, env := range container.Env {
			envs[env.Name] = env.Value
		}
	}
	podMetadata := &PodMetadata{
		PodName:   pod.Name,
		StartTime: pod.CreationTimestamp.Time.Unix(),
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
		if podMetadata.WorkloadKind == REPLICASET {
			// replicaset -> deployment
			replicasetKey := generateNameWithNamespaceKey(pod.Namespace, podMetadata.WorkloadName)
			replicasets := m.metaManager.cacheMap[REPLICASET].Get([]string{replicasetKey})
			for _, replicaset := range replicasets[replicasetKey] {
				logger.Warning(context.Background(), "ReplicaSet has no owner1", podMetadata.WorkloadName)
				if len(replicaset.Raw.(*app.ReplicaSet).OwnerReferences) > 0 {
					podMetadata.WorkloadName = replicaset.Raw.(*app.ReplicaSet).OwnerReferences[0].Name
					podMetadata.WorkloadKind = strings.ToLower(replicaset.Raw.(*app.ReplicaSet).OwnerReferences[0].Kind)
					break
				}
			}
			if podMetadata.WorkloadKind == "replicaset" {
				logger.Warning(context.Background(), "ReplicaSet has no owner", podMetadata.WorkloadName)
			}
		}
	}
	return podMetadata
}

func truncateContainerID(containerID string) string {
	sep := "://"
	separated := strings.SplitN(containerID, sep, 2)
	if len(separated) < 2 {
		return ""
	}
	return separated[1]
}

func wrapperResponse(w http.ResponseWriter, metadata map[string]*PodMetadata) {
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
