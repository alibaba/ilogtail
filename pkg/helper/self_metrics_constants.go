// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package helper

import (
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

//////////////////////////////////////////////////////////////////////////
// labels
//////////////////////////////////////////////////////////////////////////

// common
const MetricLabelProject = "project"

// pipeline
const (
	MetricLabelLogstore     = "logstore"
	MetricLabelRegion       = "region"
	MetricLabelPipelineName = "pipeline_name"
)

// plugin
const (
	MetricLabelPluginType  = "plugin_type"
	MetricLabelPluginID    = "plugin_id"
	MetricLabelNodeID      = "node_id"
	MetricLabelChildNodeID = "child_node_id"
	// label for service_mysql/
	MetricLabelDriver = "driver"
)

//////////////////////////////////////////////////////////////////////////
// agent
//////////////////////////////////////////////////////////////////////////

const (
	MetricAgentCPUGo           = "agent_go_cpu_percent"
	MetricAgentMemoryGo        = "agent_go_memory_used_mb"
	MetricAgentGoRoutinesTotal = "agent_go_routines_total"
)

//////////////////////////////////////////////////////////////////////////
// plugin
//////////////////////////////////////////////////////////////////////////

// common metrics
const (
	MetricPluginCostTimeMs              = "plugin_cost_time_ms"
	MetricPluginInBufferTotal           = "plugin_in_buffer_total"
	MetricPluginInBufferSizeBytes       = "plugin_in_buffer_size_bytes"
	MetricPluginOutBufferTotal          = "plugin_out_buffer_total"
	MetricPluginOutBufferSizeBytes      = "plugin_out_buffer_size_bytes"
	MetricPluginInEventsTotal           = "plugin_in_events_total"
	MetricPluginInEventsSizeBytes       = "plugin_in_events_size_bytes"
	MetricPluginOutEventsTotal          = "plugin_out_events_total"
	MetricPluginOutEventsSizeBytes      = "plugin_out_events_size_bytes"
	MetricPluginInEventGroupsTotal      = "plugin_in_event_groups_total"
	MetricPluginInEventGroupsSizeBytes  = "plugin_in_event_groups_size_bytes"
	MetricPluginOutEventGroupsTotal     = "plugin_in_event_groups_total"
	MetricPluginOutEventGroupsSizeBytes = "plugin_out_event_groups_size_bytes"
	MetricPluginDiscardEventsTotal      = "plugin_discard_events_total"
	MetricPluginErrorTotal              = "plugin_error_total"
)

// input input_canal custom metrics
const (
	MetricPluginBinlogRotate     = "plugin_binlog_rotate"
	MetricPluginBinlogSync       = "plugin_binlog_sync"
	MetricPluginBinlogDdl        = "plugin_binlog_ddl"
	MetricPluginBinlogRow        = "plugin_binlog_row"
	MetricPluginBinlogXgid       = "plugin_binlog_xgid"
	MetricPluginBinlogCheckpoint = "plugin_binlog_checkpoint"
	MetricPluginBinlogFilename   = "plugin_binlog_filename"
	MetricPluginBinlogGtid       = "plugin_binlog_gtid"
)

// input metric_container_info/service_docker_stdout_v2 custom metrics
const (
	MetricPluginContainerTotal       = "plugin_container_total"
	MetricPluginAddContainerTotal    = "plugin_add_container_total"
	MetricPluginRemoveContainerTotal = "plugin_remove_container_total"
	MetricPluginUpdateContainerTotal = "plugin_update_container_total"
)

// input service_mysql custom metrics
const (
	MetricPluginCollectAvgCostTimeMs = "plugin_collect_avg_cost_time_ms"
	MetricPluginCollectTotal         = "plugin_collect_total"
)

// processor processor_anchor/processor_regex/processor_string_replace custom metrics
const (
	PluginPairsPerLogTotal = "plugin_pairs_per_log_total"
)

func GetCommonLabels(context pipeline.Context, pluginMeta *pipeline.PluginMeta) []pipeline.LabelPair {
	labels := make([]pipeline.LabelPair, 0)
	labels = append(labels, pipeline.LabelPair{Key: MetricLabelProject, Value: context.GetProject()})
	labels = append(labels, pipeline.LabelPair{Key: MetricLabelLogstore, Value: context.GetLogstore()})
	labels = append(labels, pipeline.LabelPair{Key: MetricLabelPipelineName, Value: context.GetConfigName()})

	if len(pluginMeta.PluginID) > 0 {
		labels = append(labels, pipeline.LabelPair{Key: MetricLabelPluginID, Value: pluginMeta.PluginID})
	}
	if len(pluginMeta.NodeID) > 0 {
		labels = append(labels, pipeline.LabelPair{Key: MetricLabelNodeID, Value: pluginMeta.NodeID})
	}
	if len(pluginMeta.ChildNodeID) > 0 {
		labels = append(labels, pipeline.LabelPair{Key: MetricLabelChildNodeID, Value: pluginMeta.ChildNodeID})
	}
	if len(pluginMeta.PluginType) > 0 {
		labels = append(labels, pipeline.LabelPair{Key: MetricLabelPluginType, Value: pluginMeta.PluginType})
	}
	return labels
}
