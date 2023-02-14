package helper

import (
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

// ReadLogVal returns the log content value for the input key, and returns empty string when not found.
func ReadLogVal(log *protocol.Log, key string) string {
	for _, content := range log.Contents {
		if content.Key == key {
			return content.Value
		}
	}
	return ""
}

// PickLogs select some of original logs to new res logs by the specific pickKey and pickVal.
func PickLogs(logs []*protocol.Log, pickKey string, pickVal string) (res []*protocol.Log) {
	for _, log := range logs {
		if ReadLogVal(log, pickKey) == pickVal {
			res = append(res, log)
		}
	}
	return res
}

// PickEvent select one of original events by the specific event name.
func PickEvent(events []models.PipelineEvent, name string) models.PipelineEvent {
	for _, event := range events {
		if event.GetName() == name {
			return event
		}
	}
	return nil

}
