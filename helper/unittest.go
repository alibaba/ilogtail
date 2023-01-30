package helper

import (
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

func ReadLogVal(log *protocol.Log, key string) string {
	for _, content := range log.Contents {
		if content.Key == key {
			return content.Value
		}
	}
	return ""
}

func PickLogs(logs []*protocol.Log, pickKey string, pickVal string) (res []*protocol.Log) {
	for _, log := range logs {
		if ReadLogVal(log, pickKey) == pickVal {
			res = append(res, log)
		}
	}
	return res
}

func PickEvent(events []models.PipelineEvent, name string) models.PipelineEvent {
	for _, event := range events {
		if event.GetName() == name {
			return event
		}
	}
	return nil

}
