package sls

import (
	"encoding/json"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

type ConverterFunc func(logs ...*models.PipelineGroupEvents) *protocol.LogGroup

func ConvertProfile(events ...*models.PipelineGroupEvents) *protocol.LogGroup {
	if len(events) == 0 {
		return nil
	}
	g := new(protocol.LogGroup)
	g.Source = util.GetIPAddress()
	for _, event := range events {
		for _, pipelineEvent := range event.Events {
			contents := make([]*protocol.Log_Content, 0, 9)
			p := pipelineEvent.(*models.Profile)
			b, _ := json.Marshal(p.GetTags().Iterator())
			contents = append(contents,
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
			for i, value := range p.Values {
				var res []*protocol.Log_Content
				if i != len(p.Values)-1 {
					res = make([]*protocol.Log_Content, 0, len(contents))
					copy(res, contents)
				} else {
					res = contents
				}
				res = append(res,
					&protocol.Log_Content{
						Key:   "units",
						Value: value.Unit,
					},
					&protocol.Log_Content{
						Key:   "valueTypes",
						Value: value.Type,
					},
					&protocol.Log_Content{
						Key:   "aggTypes",
						Value: value.AggType,
					},
					&protocol.Log_Content{
						Key:   "val",
						Value: strconv.FormatFloat(value.Val, 'f', 2, 64),
					},
				)
				g.Logs = append(g.Logs, &protocol.Log{
					Time:     uint32(p.StartTime / 1e9),
					Contents: res,
				})
			}
		}
	}
	return g
}
