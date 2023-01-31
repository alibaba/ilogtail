package sls

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

type ConverterFunc func(logs ...*models.PipelineGroupEvents) *protocol.LogGroup

func ConvertProfile(events ...*models.PipelineGroupEvents) *protocol.LogGroup {
	g := new(protocol.LogGroup)
	g.Source = util.GetIPAddress()
	for _, event := range events {
		for _, pipelineEvent := range event.Events {
			var log protocol.Log
			p := pipelineEvent.(*models.Profile)
			log.Time = uint32(p.StartTime / 1e9)
			b, _ := json.Marshal(p.GetTags().Iterator())
			log.Contents = append(log.Contents,
				&protocol.Log_Content{
					Key:   "name",
					Value: p.Name,
				},
				&protocol.Log_Content{
					Key:   "stack",
					Value: strings.Join(p.Stack, "\n"),
				},
				&protocol.Log_Content{
					Key:   "stackID",
					Value: p.StackID,
				},
				&protocol.Log_Content{
					Key:   "language",
					Value: p.Language,
				},
				&protocol.Log_Content{
					Key:   "type",
					Value: p.ProfileType.String(),
				},
				&protocol.Log_Content{
					Key:   "dataType",
					Value: p.DataType,
				},
				&protocol.Log_Content{
					Key:   "durationNs",
					Value: strconv.FormatInt(p.EndTime-p.StartTime, 10),
				},
				&protocol.Log_Content{
					Key:   "labels",
					Value: string(b),
				},
				&protocol.Log_Content{
					Key:   "profileID",
					Value: p.ProfileID,
				},
			)
			var types, aggs, units []string
			for i, value := range p.Values {
				types = append(types, value.Type)
				aggs = append(aggs, value.AggType)
				units = append(units, value.Unit)
				log.Contents = append(log.Contents, &protocol.Log_Content{
					Key:   fmt.Sprintf("value_%d", i),
					Value: strconv.FormatFloat(value.Val, 'f', 2, 64),
				})
			}
			log.Contents = append(log.Contents,
				&protocol.Log_Content{
					Key:   "units",
					Value: strings.Join(units, ","),
				},
				&protocol.Log_Content{
					Key:   "valueTypes",
					Value: strings.Join(types, ","),
				},
				&protocol.Log_Content{
					Key:   "aggTypes",
					Value: strings.Join(aggs, ","),
				},
			)
			g.Logs = append(g.Logs, &log)
		}
	}
	return g
}
