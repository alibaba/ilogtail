// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.28.1
// 	protoc        v3.21.5
// source: agent.proto

package configserver_proto

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type ConfigUpdateInfo_UpdateStatus int32

const (
	ConfigUpdateInfo_SAME     ConfigUpdateInfo_UpdateStatus = 0
	ConfigUpdateInfo_NEW      ConfigUpdateInfo_UpdateStatus = 1
	ConfigUpdateInfo_DELETED  ConfigUpdateInfo_UpdateStatus = 2
	ConfigUpdateInfo_MODIFIED ConfigUpdateInfo_UpdateStatus = 3
)

// Enum value maps for ConfigUpdateInfo_UpdateStatus.
var (
	ConfigUpdateInfo_UpdateStatus_name = map[int32]string{
		0: "SAME",
		1: "NEW",
		2: "DELETED",
		3: "MODIFIED",
	}
	ConfigUpdateInfo_UpdateStatus_value = map[string]int32{
		"SAME":     0,
		"NEW":      1,
		"DELETED":  2,
		"MODIFIED": 3,
	}
)

func (x ConfigUpdateInfo_UpdateStatus) Enum() *ConfigUpdateInfo_UpdateStatus {
	p := new(ConfigUpdateInfo_UpdateStatus)
	*p = x
	return p
}

func (x ConfigUpdateInfo_UpdateStatus) String() string {
	return protoimpl.X.EnumStringOf(x.Descriptor(), protoreflect.EnumNumber(x))
}

func (ConfigUpdateInfo_UpdateStatus) Descriptor() protoreflect.EnumDescriptor {
	return file_agent_proto_enumTypes[0].Descriptor()
}

func (ConfigUpdateInfo_UpdateStatus) Type() protoreflect.EnumType {
	return &file_agent_proto_enumTypes[0]
}

func (x ConfigUpdateInfo_UpdateStatus) Number() protoreflect.EnumNumber {
	return protoreflect.EnumNumber(x)
}

// Deprecated: Use ConfigUpdateInfo_UpdateStatus.Descriptor instead.
func (ConfigUpdateInfo_UpdateStatus) EnumDescriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{8, 0}
}

type HeartBeatRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	RequestId     string            `protobuf:"bytes,1,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	AgentId       string            `protobuf:"bytes,2,opt,name=agent_id,json=agentId,proto3" json:"agent_id,omitempty"`
	AgentVersion  string            `protobuf:"bytes,3,opt,name=agent_version,json=agentVersion,proto3" json:"agent_version,omitempty"`
	Ip            string            `protobuf:"bytes,4,opt,name=ip,proto3" json:"ip,omitempty"`
	Tags          map[string]string `protobuf:"bytes,5,rep,name=tags,proto3" json:"tags,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	RunningStatus string            `protobuf:"bytes,6,opt,name=running_status,json=runningStatus,proto3" json:"running_status,omitempty"`
	StartupTime   int64             `protobuf:"varint,7,opt,name=startup_time,json=startupTime,proto3" json:"startup_time,omitempty"`
}

func (x *HeartBeatRequest) Reset() {
	*x = HeartBeatRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *HeartBeatRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*HeartBeatRequest) ProtoMessage() {}

func (x *HeartBeatRequest) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use HeartBeatRequest.ProtoReflect.Descriptor instead.
func (*HeartBeatRequest) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{0}
}

func (x *HeartBeatRequest) GetRequestId() string {
	if x != nil {
		return x.RequestId
	}
	return ""
}

func (x *HeartBeatRequest) GetAgentId() string {
	if x != nil {
		return x.AgentId
	}
	return ""
}

func (x *HeartBeatRequest) GetAgentVersion() string {
	if x != nil {
		return x.AgentVersion
	}
	return ""
}

func (x *HeartBeatRequest) GetIp() string {
	if x != nil {
		return x.Ip
	}
	return ""
}

func (x *HeartBeatRequest) GetTags() map[string]string {
	if x != nil {
		return x.Tags
	}
	return nil
}

func (x *HeartBeatRequest) GetRunningStatus() string {
	if x != nil {
		return x.RunningStatus
	}
	return ""
}

func (x *HeartBeatRequest) GetStartupTime() int64 {
	if x != nil {
		return x.StartupTime
	}
	return 0
}

type HeartBeatResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	ResponseId string `protobuf:"bytes,1,opt,name=response_id,json=responseId,proto3" json:"response_id,omitempty"`
	Code       string `protobuf:"bytes,2,opt,name=code,proto3" json:"code,omitempty"`
	Message    string `protobuf:"bytes,3,opt,name=message,proto3" json:"message,omitempty"`
}

func (x *HeartBeatResponse) Reset() {
	*x = HeartBeatResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *HeartBeatResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*HeartBeatResponse) ProtoMessage() {}

func (x *HeartBeatResponse) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use HeartBeatResponse.ProtoReflect.Descriptor instead.
func (*HeartBeatResponse) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{1}
}

func (x *HeartBeatResponse) GetResponseId() string {
	if x != nil {
		return x.ResponseId
	}
	return ""
}

func (x *HeartBeatResponse) GetCode() string {
	if x != nil {
		return x.Code
	}
	return ""
}

func (x *HeartBeatResponse) GetMessage() string {
	if x != nil {
		return x.Message
	}
	return ""
}

type RunningStatistics struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Cpu    float32           `protobuf:"fixed32,1,opt,name=cpu,proto3" json:"cpu,omitempty"`
	Memory int64             `protobuf:"varint,2,opt,name=memory,proto3" json:"memory,omitempty"`
	Extras map[string]string `protobuf:"bytes,100,rep,name=extras,proto3" json:"extras,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
}

func (x *RunningStatistics) Reset() {
	*x = RunningStatistics{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *RunningStatistics) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*RunningStatistics) ProtoMessage() {}

func (x *RunningStatistics) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use RunningStatistics.ProtoReflect.Descriptor instead.
func (*RunningStatistics) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{2}
}

func (x *RunningStatistics) GetCpu() float32 {
	if x != nil {
		return x.Cpu
	}
	return 0
}

func (x *RunningStatistics) GetMemory() int64 {
	if x != nil {
		return x.Memory
	}
	return 0
}

func (x *RunningStatistics) GetExtras() map[string]string {
	if x != nil {
		return x.Extras
	}
	return nil
}

type RunningStatisticsRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	RequestId      string             `protobuf:"bytes,1,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	AgentId        string             `protobuf:"bytes,2,opt,name=agent_id,json=agentId,proto3" json:"agent_id,omitempty"`
	RunningDetails *RunningStatistics `protobuf:"bytes,3,opt,name=running_details,json=runningDetails,proto3" json:"running_details,omitempty"`
}

func (x *RunningStatisticsRequest) Reset() {
	*x = RunningStatisticsRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[3]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *RunningStatisticsRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*RunningStatisticsRequest) ProtoMessage() {}

func (x *RunningStatisticsRequest) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[3]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use RunningStatisticsRequest.ProtoReflect.Descriptor instead.
func (*RunningStatisticsRequest) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{3}
}

func (x *RunningStatisticsRequest) GetRequestId() string {
	if x != nil {
		return x.RequestId
	}
	return ""
}

func (x *RunningStatisticsRequest) GetAgentId() string {
	if x != nil {
		return x.AgentId
	}
	return ""
}

func (x *RunningStatisticsRequest) GetRunningDetails() *RunningStatistics {
	if x != nil {
		return x.RunningDetails
	}
	return nil
}

type RunningStatisticsResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	ResponseId string `protobuf:"bytes,1,opt,name=response_id,json=responseId,proto3" json:"response_id,omitempty"`
	Code       string `protobuf:"bytes,2,opt,name=code,proto3" json:"code,omitempty"`
	Message    string `protobuf:"bytes,3,opt,name=message,proto3" json:"message,omitempty"`
}

func (x *RunningStatisticsResponse) Reset() {
	*x = RunningStatisticsResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[4]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *RunningStatisticsResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*RunningStatisticsResponse) ProtoMessage() {}

func (x *RunningStatisticsResponse) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[4]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use RunningStatisticsResponse.ProtoReflect.Descriptor instead.
func (*RunningStatisticsResponse) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{4}
}

func (x *RunningStatisticsResponse) GetResponseId() string {
	if x != nil {
		return x.ResponseId
	}
	return ""
}

func (x *RunningStatisticsResponse) GetCode() string {
	if x != nil {
		return x.Code
	}
	return ""
}

func (x *RunningStatisticsResponse) GetMessage() string {
	if x != nil {
		return x.Message
	}
	return ""
}

type AlarmRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	RequestId string `protobuf:"bytes,1,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	AgentId   string `protobuf:"bytes,2,opt,name=agent_id,json=agentId,proto3" json:"agent_id,omitempty"`
	Type      string `protobuf:"bytes,3,opt,name=type,proto3" json:"type,omitempty"`
	Detail    string `protobuf:"bytes,4,opt,name=detail,proto3" json:"detail,omitempty"`
}

func (x *AlarmRequest) Reset() {
	*x = AlarmRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[5]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *AlarmRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*AlarmRequest) ProtoMessage() {}

func (x *AlarmRequest) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[5]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use AlarmRequest.ProtoReflect.Descriptor instead.
func (*AlarmRequest) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{5}
}

func (x *AlarmRequest) GetRequestId() string {
	if x != nil {
		return x.RequestId
	}
	return ""
}

func (x *AlarmRequest) GetAgentId() string {
	if x != nil {
		return x.AgentId
	}
	return ""
}

func (x *AlarmRequest) GetType() string {
	if x != nil {
		return x.Type
	}
	return ""
}

func (x *AlarmRequest) GetDetail() string {
	if x != nil {
		return x.Detail
	}
	return ""
}

type AlarmResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	ResponseId string `protobuf:"bytes,1,opt,name=response_id,json=responseId,proto3" json:"response_id,omitempty"`
	Code       string `protobuf:"bytes,2,opt,name=code,proto3" json:"code,omitempty"`
	Message    string `protobuf:"bytes,3,opt,name=message,proto3" json:"message,omitempty"`
}

func (x *AlarmResponse) Reset() {
	*x = AlarmResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[6]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *AlarmResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*AlarmResponse) ProtoMessage() {}

func (x *AlarmResponse) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[6]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use AlarmResponse.ProtoReflect.Descriptor instead.
func (*AlarmResponse) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{6}
}

func (x *AlarmResponse) GetResponseId() string {
	if x != nil {
		return x.ResponseId
	}
	return ""
}

func (x *AlarmResponse) GetCode() string {
	if x != nil {
		return x.Code
	}
	return ""
}

func (x *AlarmResponse) GetMessage() string {
	if x != nil {
		return x.Message
	}
	return ""
}

type AgentGetConfigListRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	RequestId      string           `protobuf:"bytes,1,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	AgentId        string           `protobuf:"bytes,2,opt,name=agent_id,json=agentId,proto3" json:"agent_id,omitempty"`
	ConfigVersions map[string]int64 `protobuf:"bytes,3,rep,name=config_versions,json=configVersions,proto3" json:"config_versions,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"varint,2,opt,name=value,proto3"`
}

func (x *AgentGetConfigListRequest) Reset() {
	*x = AgentGetConfigListRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[7]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *AgentGetConfigListRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*AgentGetConfigListRequest) ProtoMessage() {}

func (x *AgentGetConfigListRequest) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[7]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use AgentGetConfigListRequest.ProtoReflect.Descriptor instead.
func (*AgentGetConfigListRequest) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{7}
}

func (x *AgentGetConfigListRequest) GetRequestId() string {
	if x != nil {
		return x.RequestId
	}
	return ""
}

func (x *AgentGetConfigListRequest) GetAgentId() string {
	if x != nil {
		return x.AgentId
	}
	return ""
}

func (x *AgentGetConfigListRequest) GetConfigVersions() map[string]int64 {
	if x != nil {
		return x.ConfigVersions
	}
	return nil
}

type ConfigUpdateInfo struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	ConfigName    string                        `protobuf:"bytes,1,opt,name=config_name,json=configName,proto3" json:"config_name,omitempty"`
	UpdateStatus  ConfigUpdateInfo_UpdateStatus `protobuf:"varint,2,opt,name=update_status,json=updateStatus,proto3,enum=configserver.proto.ConfigUpdateInfo_UpdateStatus" json:"update_status,omitempty"`
	ConfigVersion int64                         `protobuf:"varint,3,opt,name=config_version,json=configVersion,proto3" json:"config_version,omitempty"`
	Content       string                        `protobuf:"bytes,4,opt,name=content,proto3" json:"content,omitempty"`
}

func (x *ConfigUpdateInfo) Reset() {
	*x = ConfigUpdateInfo{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[8]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *ConfigUpdateInfo) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*ConfigUpdateInfo) ProtoMessage() {}

func (x *ConfigUpdateInfo) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[8]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use ConfigUpdateInfo.ProtoReflect.Descriptor instead.
func (*ConfigUpdateInfo) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{8}
}

func (x *ConfigUpdateInfo) GetConfigName() string {
	if x != nil {
		return x.ConfigName
	}
	return ""
}

func (x *ConfigUpdateInfo) GetUpdateStatus() ConfigUpdateInfo_UpdateStatus {
	if x != nil {
		return x.UpdateStatus
	}
	return ConfigUpdateInfo_SAME
}

func (x *ConfigUpdateInfo) GetConfigVersion() int64 {
	if x != nil {
		return x.ConfigVersion
	}
	return 0
}

func (x *ConfigUpdateInfo) GetContent() string {
	if x != nil {
		return x.Content
	}
	return ""
}

type AgentGetConfigListResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	ResponseId        string              `protobuf:"bytes,1,opt,name=response_id,json=responseId,proto3" json:"response_id,omitempty"`
	Code              string              `protobuf:"bytes,2,opt,name=code,proto3" json:"code,omitempty"`
	Message           string              `protobuf:"bytes,3,opt,name=message,proto3" json:"message,omitempty"`
	ConfigUpdateInfos []*ConfigUpdateInfo `protobuf:"bytes,4,rep,name=config_update_infos,json=configUpdateInfos,proto3" json:"config_update_infos,omitempty"`
}

func (x *AgentGetConfigListResponse) Reset() {
	*x = AgentGetConfigListResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_agent_proto_msgTypes[9]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *AgentGetConfigListResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*AgentGetConfigListResponse) ProtoMessage() {}

func (x *AgentGetConfigListResponse) ProtoReflect() protoreflect.Message {
	mi := &file_agent_proto_msgTypes[9]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use AgentGetConfigListResponse.ProtoReflect.Descriptor instead.
func (*AgentGetConfigListResponse) Descriptor() ([]byte, []int) {
	return file_agent_proto_rawDescGZIP(), []int{9}
}

func (x *AgentGetConfigListResponse) GetResponseId() string {
	if x != nil {
		return x.ResponseId
	}
	return ""
}

func (x *AgentGetConfigListResponse) GetCode() string {
	if x != nil {
		return x.Code
	}
	return ""
}

func (x *AgentGetConfigListResponse) GetMessage() string {
	if x != nil {
		return x.Message
	}
	return ""
}

func (x *AgentGetConfigListResponse) GetConfigUpdateInfos() []*ConfigUpdateInfo {
	if x != nil {
		return x.ConfigUpdateInfos
	}
	return nil
}

var File_agent_proto protoreflect.FileDescriptor

var file_agent_proto_rawDesc = []byte{
	0x0a, 0x0b, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x12, 0x63,
	0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x2e, 0x70, 0x72, 0x6f, 0x74,
	0x6f, 0x22, 0xc8, 0x02, 0x0a, 0x10, 0x48, 0x65, 0x61, 0x72, 0x74, 0x42, 0x65, 0x61, 0x74, 0x52,
	0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x12, 0x1d, 0x0a, 0x0a, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73,
	0x74, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x72, 0x65, 0x71, 0x75,
	0x65, 0x73, 0x74, 0x49, 0x64, 0x12, 0x19, 0x0a, 0x08, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x5f, 0x69,
	0x64, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x49, 0x64,
	0x12, 0x23, 0x0a, 0x0d, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x5f, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f,
	0x6e, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0c, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x56, 0x65,
	0x72, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x0e, 0x0a, 0x02, 0x69, 0x70, 0x18, 0x04, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x02, 0x69, 0x70, 0x12, 0x42, 0x0a, 0x04, 0x74, 0x61, 0x67, 0x73, 0x18, 0x05, 0x20,
	0x03, 0x28, 0x0b, 0x32, 0x2e, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76,
	0x65, 0x72, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x48, 0x65, 0x61, 0x72, 0x74, 0x42, 0x65,
	0x61, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x2e, 0x54, 0x61, 0x67, 0x73, 0x45, 0x6e,
	0x74, 0x72, 0x79, 0x52, 0x04, 0x74, 0x61, 0x67, 0x73, 0x12, 0x25, 0x0a, 0x0e, 0x72, 0x75, 0x6e,
	0x6e, 0x69, 0x6e, 0x67, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x18, 0x06, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x0d, 0x72, 0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x53, 0x74, 0x61, 0x74, 0x75, 0x73,
	0x12, 0x21, 0x0a, 0x0c, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x5f, 0x74, 0x69, 0x6d, 0x65,
	0x18, 0x07, 0x20, 0x01, 0x28, 0x03, 0x52, 0x0b, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x54,
	0x69, 0x6d, 0x65, 0x1a, 0x37, 0x0a, 0x09, 0x54, 0x61, 0x67, 0x73, 0x45, 0x6e, 0x74, 0x72, 0x79,
	0x12, 0x10, 0x0a, 0x03, 0x6b, 0x65, 0x79, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x03, 0x6b,
	0x65, 0x79, 0x12, 0x14, 0x0a, 0x05, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x05, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x3a, 0x02, 0x38, 0x01, 0x22, 0x62, 0x0a, 0x11,
	0x48, 0x65, 0x61, 0x72, 0x74, 0x42, 0x65, 0x61, 0x74, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73,
	0x65, 0x12, 0x1f, 0x0a, 0x0b, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x5f, 0x69, 0x64,
	0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0a, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65,
	0x49, 0x64, 0x12, 0x12, 0x0a, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x12, 0x18, 0x0a, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67,
	0x65, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65,
	0x22, 0xc3, 0x01, 0x0a, 0x11, 0x52, 0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x53, 0x74, 0x61, 0x74,
	0x69, 0x73, 0x74, 0x69, 0x63, 0x73, 0x12, 0x10, 0x0a, 0x03, 0x63, 0x70, 0x75, 0x18, 0x01, 0x20,
	0x01, 0x28, 0x02, 0x52, 0x03, 0x63, 0x70, 0x75, 0x12, 0x16, 0x0a, 0x06, 0x6d, 0x65, 0x6d, 0x6f,
	0x72, 0x79, 0x18, 0x02, 0x20, 0x01, 0x28, 0x03, 0x52, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79,
	0x12, 0x49, 0x0a, 0x06, 0x65, 0x78, 0x74, 0x72, 0x61, 0x73, 0x18, 0x64, 0x20, 0x03, 0x28, 0x0b,
	0x32, 0x31, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x2e,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x52, 0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x53, 0x74, 0x61,
	0x74, 0x69, 0x73, 0x74, 0x69, 0x63, 0x73, 0x2e, 0x45, 0x78, 0x74, 0x72, 0x61, 0x73, 0x45, 0x6e,
	0x74, 0x72, 0x79, 0x52, 0x06, 0x65, 0x78, 0x74, 0x72, 0x61, 0x73, 0x1a, 0x39, 0x0a, 0x0b, 0x45,
	0x78, 0x74, 0x72, 0x61, 0x73, 0x45, 0x6e, 0x74, 0x72, 0x79, 0x12, 0x10, 0x0a, 0x03, 0x6b, 0x65,
	0x79, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x03, 0x6b, 0x65, 0x79, 0x12, 0x14, 0x0a, 0x05,
	0x76, 0x61, 0x6c, 0x75, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x76, 0x61, 0x6c,
	0x75, 0x65, 0x3a, 0x02, 0x38, 0x01, 0x22, 0xa4, 0x01, 0x0a, 0x18, 0x52, 0x75, 0x6e, 0x6e, 0x69,
	0x6e, 0x67, 0x53, 0x74, 0x61, 0x74, 0x69, 0x73, 0x74, 0x69, 0x63, 0x73, 0x52, 0x65, 0x71, 0x75,
	0x65, 0x73, 0x74, 0x12, 0x1d, 0x0a, 0x0a, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x5f, 0x69,
	0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74,
	0x49, 0x64, 0x12, 0x19, 0x0a, 0x08, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x5f, 0x69, 0x64, 0x18, 0x02,
	0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x49, 0x64, 0x12, 0x4e, 0x0a,
	0x0f, 0x72, 0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x5f, 0x64, 0x65, 0x74, 0x61, 0x69, 0x6c, 0x73,
	0x18, 0x03, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x25, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73,
	0x65, 0x72, 0x76, 0x65, 0x72, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x52, 0x75, 0x6e, 0x6e,
	0x69, 0x6e, 0x67, 0x53, 0x74, 0x61, 0x74, 0x69, 0x73, 0x74, 0x69, 0x63, 0x73, 0x52, 0x0e, 0x72,
	0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x44, 0x65, 0x74, 0x61, 0x69, 0x6c, 0x73, 0x22, 0x6a, 0x0a,
	0x19, 0x52, 0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x53, 0x74, 0x61, 0x74, 0x69, 0x73, 0x74, 0x69,
	0x63, 0x73, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x12, 0x1f, 0x0a, 0x0b, 0x72, 0x65,
	0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52,
	0x0a, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x49, 0x64, 0x12, 0x12, 0x0a, 0x04, 0x63,
	0x6f, 0x64, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x12,
	0x18, 0x0a, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x22, 0x74, 0x0a, 0x0c, 0x41, 0x6c, 0x61,
	0x72, 0x6d, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x12, 0x1d, 0x0a, 0x0a, 0x72, 0x65, 0x71,
	0x75, 0x65, 0x73, 0x74, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x72,
	0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x49, 0x64, 0x12, 0x19, 0x0a, 0x08, 0x61, 0x67, 0x65, 0x6e,
	0x74, 0x5f, 0x69, 0x64, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x61, 0x67, 0x65, 0x6e,
	0x74, 0x49, 0x64, 0x12, 0x12, 0x0a, 0x04, 0x74, 0x79, 0x70, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x04, 0x74, 0x79, 0x70, 0x65, 0x12, 0x16, 0x0a, 0x06, 0x64, 0x65, 0x74, 0x61, 0x69,
	0x6c, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x06, 0x64, 0x65, 0x74, 0x61, 0x69, 0x6c, 0x22,
	0x5e, 0x0a, 0x0d, 0x41, 0x6c, 0x61, 0x72, 0x6d, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65,
	0x12, 0x1f, 0x0a, 0x0b, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x5f, 0x69, 0x64, 0x18,
	0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0a, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x49,
	0x64, 0x12, 0x12, 0x0a, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52,
	0x04, 0x63, 0x6f, 0x64, 0x65, 0x12, 0x18, 0x0a, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65,
	0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x22,
	0x84, 0x02, 0x0a, 0x19, 0x41, 0x67, 0x65, 0x6e, 0x74, 0x47, 0x65, 0x74, 0x43, 0x6f, 0x6e, 0x66,
	0x69, 0x67, 0x4c, 0x69, 0x73, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x12, 0x1d, 0x0a,
	0x0a, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x09, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x49, 0x64, 0x12, 0x19, 0x0a, 0x08,
	0x61, 0x67, 0x65, 0x6e, 0x74, 0x5f, 0x69, 0x64, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07,
	0x61, 0x67, 0x65, 0x6e, 0x74, 0x49, 0x64, 0x12, 0x6a, 0x0a, 0x0f, 0x63, 0x6f, 0x6e, 0x66, 0x69,
	0x67, 0x5f, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x73, 0x18, 0x03, 0x20, 0x03, 0x28, 0x0b,
	0x32, 0x41, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x2e,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x41, 0x67, 0x65, 0x6e, 0x74, 0x47, 0x65, 0x74, 0x43, 0x6f,
	0x6e, 0x66, 0x69, 0x67, 0x4c, 0x69, 0x73, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x2e,
	0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x73, 0x45, 0x6e,
	0x74, 0x72, 0x79, 0x52, 0x0e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x56, 0x65, 0x72, 0x73, 0x69,
	0x6f, 0x6e, 0x73, 0x1a, 0x41, 0x0a, 0x13, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x56, 0x65, 0x72,
	0x73, 0x69, 0x6f, 0x6e, 0x73, 0x45, 0x6e, 0x74, 0x72, 0x79, 0x12, 0x10, 0x0a, 0x03, 0x6b, 0x65,
	0x79, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x03, 0x6b, 0x65, 0x79, 0x12, 0x14, 0x0a, 0x05,
	0x76, 0x61, 0x6c, 0x75, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x03, 0x52, 0x05, 0x76, 0x61, 0x6c,
	0x75, 0x65, 0x3a, 0x02, 0x38, 0x01, 0x22, 0x8a, 0x02, 0x0a, 0x10, 0x43, 0x6f, 0x6e, 0x66, 0x69,
	0x67, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x49, 0x6e, 0x66, 0x6f, 0x12, 0x1f, 0x0a, 0x0b, 0x63,
	0x6f, 0x6e, 0x66, 0x69, 0x67, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x0a, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x56, 0x0a, 0x0d,
	0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x18, 0x02, 0x20,
	0x01, 0x28, 0x0e, 0x32, 0x31, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76,
	0x65, 0x72, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x55,
	0x70, 0x64, 0x61, 0x74, 0x65, 0x49, 0x6e, 0x66, 0x6f, 0x2e, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65,
	0x53, 0x74, 0x61, 0x74, 0x75, 0x73, 0x52, 0x0c, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x53, 0x74,
	0x61, 0x74, 0x75, 0x73, 0x12, 0x25, 0x0a, 0x0e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x5f, 0x76,
	0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x18, 0x03, 0x20, 0x01, 0x28, 0x03, 0x52, 0x0d, 0x63, 0x6f,
	0x6e, 0x66, 0x69, 0x67, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x18, 0x0a, 0x07, 0x63,
	0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x63, 0x6f,
	0x6e, 0x74, 0x65, 0x6e, 0x74, 0x22, 0x3c, 0x0a, 0x0c, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x53,
	0x74, 0x61, 0x74, 0x75, 0x73, 0x12, 0x08, 0x0a, 0x04, 0x53, 0x41, 0x4d, 0x45, 0x10, 0x00, 0x12,
	0x07, 0x0a, 0x03, 0x4e, 0x45, 0x57, 0x10, 0x01, 0x12, 0x0b, 0x0a, 0x07, 0x44, 0x45, 0x4c, 0x45,
	0x54, 0x45, 0x44, 0x10, 0x02, 0x12, 0x0c, 0x0a, 0x08, 0x4d, 0x4f, 0x44, 0x49, 0x46, 0x49, 0x45,
	0x44, 0x10, 0x03, 0x22, 0xc1, 0x01, 0x0a, 0x1a, 0x41, 0x67, 0x65, 0x6e, 0x74, 0x47, 0x65, 0x74,
	0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x4c, 0x69, 0x73, 0x74, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e,
	0x73, 0x65, 0x12, 0x1f, 0x0a, 0x0b, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x5f, 0x69,
	0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0a, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73,
	0x65, 0x49, 0x64, 0x12, 0x12, 0x0a, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x04, 0x63, 0x6f, 0x64, 0x65, 0x12, 0x18, 0x0a, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61,
	0x67, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x07, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67,
	0x65, 0x12, 0x54, 0x0a, 0x13, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x5f, 0x75, 0x70, 0x64, 0x61,
	0x74, 0x65, 0x5f, 0x69, 0x6e, 0x66, 0x6f, 0x73, 0x18, 0x04, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x24,
	0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x2e, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x2e, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65,
	0x49, 0x6e, 0x66, 0x6f, 0x52, 0x11, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x55, 0x70, 0x64, 0x61,
	0x74, 0x65, 0x49, 0x6e, 0x66, 0x6f, 0x73, 0x42, 0x16, 0x5a, 0x14, 0x2e, 0x3b, 0x63, 0x6f, 0x6e,
	0x66, 0x69, 0x67, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x5f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62,
	0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_agent_proto_rawDescOnce sync.Once
	file_agent_proto_rawDescData = file_agent_proto_rawDesc
)

func file_agent_proto_rawDescGZIP() []byte {
	file_agent_proto_rawDescOnce.Do(func() {
		file_agent_proto_rawDescData = protoimpl.X.CompressGZIP(file_agent_proto_rawDescData)
	})
	return file_agent_proto_rawDescData
}

var file_agent_proto_enumTypes = make([]protoimpl.EnumInfo, 1)
var file_agent_proto_msgTypes = make([]protoimpl.MessageInfo, 13)
var file_agent_proto_goTypes = []interface{}{
	(ConfigUpdateInfo_UpdateStatus)(0), // 0: configserver.proto.ConfigUpdateInfo.UpdateStatus
	(*HeartBeatRequest)(nil),           // 1: configserver.proto.HeartBeatRequest
	(*HeartBeatResponse)(nil),          // 2: configserver.proto.HeartBeatResponse
	(*RunningStatistics)(nil),          // 3: configserver.proto.RunningStatistics
	(*RunningStatisticsRequest)(nil),   // 4: configserver.proto.RunningStatisticsRequest
	(*RunningStatisticsResponse)(nil),  // 5: configserver.proto.RunningStatisticsResponse
	(*AlarmRequest)(nil),               // 6: configserver.proto.AlarmRequest
	(*AlarmResponse)(nil),              // 7: configserver.proto.AlarmResponse
	(*AgentGetConfigListRequest)(nil),  // 8: configserver.proto.AgentGetConfigListRequest
	(*ConfigUpdateInfo)(nil),           // 9: configserver.proto.ConfigUpdateInfo
	(*AgentGetConfigListResponse)(nil), // 10: configserver.proto.AgentGetConfigListResponse
	nil,                                // 11: configserver.proto.HeartBeatRequest.TagsEntry
	nil,                                // 12: configserver.proto.RunningStatistics.ExtrasEntry
	nil,                                // 13: configserver.proto.AgentGetConfigListRequest.ConfigVersionsEntry
}
var file_agent_proto_depIdxs = []int32{
	11, // 0: configserver.proto.HeartBeatRequest.tags:type_name -> configserver.proto.HeartBeatRequest.TagsEntry
	12, // 1: configserver.proto.RunningStatistics.extras:type_name -> configserver.proto.RunningStatistics.ExtrasEntry
	3,  // 2: configserver.proto.RunningStatisticsRequest.running_details:type_name -> configserver.proto.RunningStatistics
	13, // 3: configserver.proto.AgentGetConfigListRequest.config_versions:type_name -> configserver.proto.AgentGetConfigListRequest.ConfigVersionsEntry
	0,  // 4: configserver.proto.ConfigUpdateInfo.update_status:type_name -> configserver.proto.ConfigUpdateInfo.UpdateStatus
	9,  // 5: configserver.proto.AgentGetConfigListResponse.config_update_infos:type_name -> configserver.proto.ConfigUpdateInfo
	6,  // [6:6] is the sub-list for method output_type
	6,  // [6:6] is the sub-list for method input_type
	6,  // [6:6] is the sub-list for extension type_name
	6,  // [6:6] is the sub-list for extension extendee
	0,  // [0:6] is the sub-list for field type_name
}

func init() { file_agent_proto_init() }
func file_agent_proto_init() {
	if File_agent_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_agent_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*HeartBeatRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*HeartBeatResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*RunningStatistics); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[3].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*RunningStatisticsRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[4].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*RunningStatisticsResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[5].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*AlarmRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[6].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*AlarmResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[7].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*AgentGetConfigListRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[8].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*ConfigUpdateInfo); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_agent_proto_msgTypes[9].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*AgentGetConfigListResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_agent_proto_rawDesc,
			NumEnums:      1,
			NumMessages:   13,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_agent_proto_goTypes,
		DependencyIndexes: file_agent_proto_depIdxs,
		EnumInfos:         file_agent_proto_enumTypes,
		MessageInfos:      file_agent_proto_msgTypes,
	}.Build()
	File_agent_proto = out.File
	file_agent_proto_rawDesc = nil
	file_agent_proto_goTypes = nil
	file_agent_proto_depIdxs = nil
}
