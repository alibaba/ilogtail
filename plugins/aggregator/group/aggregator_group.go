package group

import (
	"fmt"
	"sync"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/aggregator/baseagg"
)

const groupKeyConnector = "_"

type groupAggregator struct {
	groupKVs map[string]string
	agg      *baseagg.AggregatorBase
}

func (g *groupAggregator) Add(log *protocol.Log, ctx map[string]interface{}) error {
	return g.agg.Add(log, ctx)
}

func (g *groupAggregator) Flush() []*protocol.LogGroup {
	logGroups := g.agg.Flush()
	for _, logGroup := range logGroups {
		addGroupTags(logGroup, g.groupKVs)
	}
	return logGroups
}

type groupQueue struct {
	groupKVs map[string]string
	queue    ilogtail.LogGroupQueue
}

// Add ...
func (q *groupQueue) Add(logGroup *protocol.LogGroup) error {
	addGroupTags(logGroup, q.groupKVs)
	return q.queue.Add(logGroup)
}

// AddWithWait ...
func (q *groupQueue) AddWithWait(logGroup *protocol.LogGroup, duration time.Duration) error {
	addGroupTags(logGroup, q.groupKVs)
	return q.queue.AddWithWait(logGroup, duration)
}

type AggregatorGroup struct {
	GroupKeys        []string
	Topic            string
	ErrIfKeyNotFound bool
	EnablePackID     bool

	groupAggs sync.Map
	lock      *sync.Mutex
	context   ilogtail.Context
	queue     ilogtail.LogGroupQueue
}

// Description ...
func (*AggregatorGroup) Description() string {
	return "aggregator that group logs by a set of keys"
}

func (g *AggregatorGroup) Init(context ilogtail.Context, que ilogtail.LogGroupQueue) (int, error) {
	g.context = context
	g.queue = que

	if len(g.GroupKeys) == 0 {
		return 0, fmt.Errorf("must specify GroupKeys")
	}

	return 0, nil
}

func (g *AggregatorGroup) Add(log *protocol.Log, ctx map[string]interface{}) error {
	agg, err := g.getOrCreateGroupAggs(log)
	if err != nil {
		return err
	}
	return agg.Add(log, ctx)
}

func (g *AggregatorGroup) Flush() []*protocol.LogGroup {
	var logGroups []*protocol.LogGroup
	g.groupAggs.Range(func(key, value any) bool {
		agg := value.(*groupAggregator)
		logGroups = append(logGroups, agg.Flush()...)
		return true
	})
	return logGroups
}

func (g *AggregatorGroup) Reset() {
	g.groupAggs.Range(func(key, value any) bool {
		agg := value.(*groupAggregator)
		agg.agg.Reset()
		return true
	})
}

func (g *AggregatorGroup) getOrCreateGroupAggs(log *protocol.Log) (*groupAggregator, error) {
	groupKey := g.buildGroupKey(log)
	groupAgg, ok := g.groupAggs.Load(groupKey)
	if ok {
		return groupAgg.(*groupAggregator), nil
	}

	groupKVs := g.getGroupKVs(log)
	groupQueue := &groupQueue{groupKVs: groupKVs, queue: g.queue}

	agg := baseagg.NewAggregatorBase()
	if _, err := agg.Init(g.context, groupQueue); err != nil {
		logger.Error(g.context.GetRuntimeContext(), "AGG_GROUP_ALARM", "fail to create agg for group", groupKVs)
		return nil, err
	}
	agg.InitInner(
		g.EnablePackID,
		util.NewPackIDPrefix(g.context.GetConfigName()+groupKey),
		g.lock,
		"",
		g.Topic,
		1024,
		4)

	groupAgg = &groupAggregator{groupKVs: groupKVs, agg: agg}
	groupAgg, _ = g.groupAggs.LoadOrStore(groupKey, groupAgg)
	return groupAgg.(*groupAggregator), nil
}

func (g *AggregatorGroup) buildGroupKey(log *protocol.Log) string {
	var groupKey string
	for idx, key := range g.GroupKeys {
		var val string
		for _, cont := range log.Contents {
			if cont.Key == key {
				val = cont.Value
				break
			}
		}

		isFirstKey := 0 == idx
		if isFirstKey {
			groupKey = val
		} else {
			groupKey += groupKeyConnector + val
		}
	}
	return groupKey
}

func (g *AggregatorGroup) getGroupKVs(log *protocol.Log) map[string]string {
	group := make(map[string]string, len(g.GroupKeys))
	for _, key := range g.GroupKeys {
		var val string
		found := false
		for _, cont := range log.Contents {
			if cont.Key == key {
				val = cont.Value
				found = true
				break
			}
		}
		if !found && g.ErrIfKeyNotFound {
			logger.Warning(g.context.GetRuntimeContext(), "AGG_GROUP_NOT_FOUND_KEY", key)
		}
		group[key] = val
	}
	return group
}

func addGroupTags(logGroup *protocol.LogGroup, tagKVs map[string]string) {
	for k, v := range tagKVs {
		for _, tag := range logGroup.LogTags {
			if tag.Key == k {
				// if any k exists already, just return
				return
			}
		}

		logGroup.LogTags = append(logGroup.LogTags, &protocol.LogTag{
			Key:   k,
			Value: v,
		})
	}
}

func init() {
	ilogtail.Aggregators["aggregator_group"] = func() ilogtail.Aggregator {
		return &AggregatorGroup{
			EnablePackID:     true,
			ErrIfKeyNotFound: false,
			lock:             &sync.Mutex{},
		}
	}
}
