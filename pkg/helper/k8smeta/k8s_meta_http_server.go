package k8smeta

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"strconv"
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

func (m *metadataHandler) K8sServerRun() error {
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

	stopCh := make(chan struct{})

	// TODO: add port in ip endpoint
	mux.HandleFunc("/metadata/ip", m.ServeHTTP)
	mux.HandleFunc("/metadata/containerid", m.ServeHTTP)
	server.Handler = mux
	go func() {
		_ = server.ListenAndServe()
	}()
	<-stopCh
	return nil
}

func (m *metadataHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
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
