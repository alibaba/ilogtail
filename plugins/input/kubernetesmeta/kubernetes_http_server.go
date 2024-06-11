package kubernetesmeta

import (
	"encoding/json"
	"net/http"
)

type requestBody struct {
	Keys []string `json:"keys"`
}

type metadataHandler struct {
	watchClient *WatchClient
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
	metadata := m.watchClient.getPodMetadata(rBody.Keys)
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
