// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: opentelemetry/proto/collector/metrics/v1/metrics_service.proto

package v1

import (
	context "context"
	fmt "fmt"
	io "io"
	math "math"
	math_bits "math/bits"

	_ "github.com/gogo/protobuf/gogoproto"
	proto "github.com/gogo/protobuf/proto"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"

	v1 "go.opentelemetry.io/collector/pdata/internal/data/protogen/metrics/v1"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.GoGoProtoPackageIsVersion3 // please upgrade the proto package

type ExportMetricsServiceRequest struct {
	// An array of ResourceMetrics.
	// For data coming from a single resource this array will typically contain one
	// element. Intermediary nodes (such as OpenTelemetry Collector) that receive
	// data from multiple origins typically batch the data before forwarding further and
	// in that case this array will contain multiple elements.
	ResourceMetrics []*v1.ResourceMetrics `protobuf:"bytes,1,rep,name=resource_metrics,json=resourceMetrics,proto3" json:"resource_metrics,omitempty"`
}

func (m *ExportMetricsServiceRequest) Reset()         { *m = ExportMetricsServiceRequest{} }
func (m *ExportMetricsServiceRequest) String() string { return proto.CompactTextString(m) }
func (*ExportMetricsServiceRequest) ProtoMessage()    {}
func (*ExportMetricsServiceRequest) Descriptor() ([]byte, []int) {
	return fileDescriptor_75fb6015e6e64798, []int{0}
}
func (m *ExportMetricsServiceRequest) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *ExportMetricsServiceRequest) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_ExportMetricsServiceRequest.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *ExportMetricsServiceRequest) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ExportMetricsServiceRequest.Merge(m, src)
}
func (m *ExportMetricsServiceRequest) XXX_Size() int {
	return m.Size()
}
func (m *ExportMetricsServiceRequest) XXX_DiscardUnknown() {
	xxx_messageInfo_ExportMetricsServiceRequest.DiscardUnknown(m)
}

var xxx_messageInfo_ExportMetricsServiceRequest proto.InternalMessageInfo

func (m *ExportMetricsServiceRequest) GetResourceMetrics() []*v1.ResourceMetrics {
	if m != nil {
		return m.ResourceMetrics
	}
	return nil
}

type ExportMetricsServiceResponse struct {
	// The details of a partially successful export request.
	//
	// If the request is only partially accepted
	// (i.e. when the server accepts only parts of the data and rejects the rest)
	// the server MUST initialize the `partial_success` field and MUST
	// set the `rejected_<signal>` with the number of items it rejected.
	//
	// Servers MAY also make use of the `partial_success` field to convey
	// warnings/suggestions to senders even when the request was fully accepted.
	// In such cases, the `rejected_<signal>` MUST have a value of `0` and
	// the `error_message` MUST be non-empty.
	//
	// A `partial_success` message with an empty value (rejected_<signal> = 0 and
	// `error_message` = "") is equivalent to it not being set/present. Senders
	// SHOULD interpret it the same way as in the full success case.
	PartialSuccess *ExportMetricsPartialSuccess `protobuf:"bytes,1,opt,name=partial_success,json=partialSuccess,proto3" json:"partial_success,omitempty"`
}

func (m *ExportMetricsServiceResponse) Reset()         { *m = ExportMetricsServiceResponse{} }
func (m *ExportMetricsServiceResponse) String() string { return proto.CompactTextString(m) }
func (*ExportMetricsServiceResponse) ProtoMessage()    {}
func (*ExportMetricsServiceResponse) Descriptor() ([]byte, []int) {
	return fileDescriptor_75fb6015e6e64798, []int{1}
}
func (m *ExportMetricsServiceResponse) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *ExportMetricsServiceResponse) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_ExportMetricsServiceResponse.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *ExportMetricsServiceResponse) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ExportMetricsServiceResponse.Merge(m, src)
}
func (m *ExportMetricsServiceResponse) XXX_Size() int {
	return m.Size()
}
func (m *ExportMetricsServiceResponse) XXX_DiscardUnknown() {
	xxx_messageInfo_ExportMetricsServiceResponse.DiscardUnknown(m)
}

var xxx_messageInfo_ExportMetricsServiceResponse proto.InternalMessageInfo

func (m *ExportMetricsServiceResponse) GetPartialSuccess() *ExportMetricsPartialSuccess {
	if m != nil {
		return m.PartialSuccess
	}
	return nil
}

type ExportMetricsPartialSuccess struct {
	// The number of rejected data points.
	//
	// A `rejected_<signal>` field holding a `0` value indicates that the
	// request was fully accepted.
	RejectedDataPoints int64 `protobuf:"varint,1,opt,name=rejected_data_points,json=rejectedDataPoints,proto3" json:"rejected_data_points,omitempty"`
	// A developer-facing human-readable message in English. It should be used
	// either to explain why the server rejected parts of the data during a partial
	// success or to convey warnings/suggestions during a full success. The message
	// should offer guidance on how users can address such issues.
	//
	// error_message is an optional field. An error_message with an empty value
	// is equivalent to it not being set.
	ErrorMessage string `protobuf:"bytes,2,opt,name=error_message,json=errorMessage,proto3" json:"error_message,omitempty"`
}

func (m *ExportMetricsPartialSuccess) Reset()         { *m = ExportMetricsPartialSuccess{} }
func (m *ExportMetricsPartialSuccess) String() string { return proto.CompactTextString(m) }
func (*ExportMetricsPartialSuccess) ProtoMessage()    {}
func (*ExportMetricsPartialSuccess) Descriptor() ([]byte, []int) {
	return fileDescriptor_75fb6015e6e64798, []int{2}
}
func (m *ExportMetricsPartialSuccess) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *ExportMetricsPartialSuccess) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_ExportMetricsPartialSuccess.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *ExportMetricsPartialSuccess) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ExportMetricsPartialSuccess.Merge(m, src)
}
func (m *ExportMetricsPartialSuccess) XXX_Size() int {
	return m.Size()
}
func (m *ExportMetricsPartialSuccess) XXX_DiscardUnknown() {
	xxx_messageInfo_ExportMetricsPartialSuccess.DiscardUnknown(m)
}

var xxx_messageInfo_ExportMetricsPartialSuccess proto.InternalMessageInfo

func (m *ExportMetricsPartialSuccess) GetRejectedDataPoints() int64 {
	if m != nil {
		return m.RejectedDataPoints
	}
	return 0
}

func (m *ExportMetricsPartialSuccess) GetErrorMessage() string {
	if m != nil {
		return m.ErrorMessage
	}
	return ""
}

func init() {
	proto.RegisterType((*ExportMetricsServiceRequest)(nil), "opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest")
	proto.RegisterType((*ExportMetricsServiceResponse)(nil), "opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceResponse")
	proto.RegisterType((*ExportMetricsPartialSuccess)(nil), "opentelemetry.proto.collector.metrics.v1.ExportMetricsPartialSuccess")
}

func init() {
	proto.RegisterFile("opentelemetry/proto/collector/metrics/v1/metrics_service.proto", fileDescriptor_75fb6015e6e64798)
}

var fileDescriptor_75fb6015e6e64798 = []byte{
	// 419 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xa4, 0x53, 0xbf, 0x8e, 0xd3, 0x30,
	0x18, 0x8f, 0xef, 0xa4, 0x93, 0xf0, 0xc1, 0x1d, 0x32, 0x37, 0x9c, 0x0a, 0x8a, 0x4e, 0x61, 0x89,
	0x04, 0x72, 0x68, 0xd9, 0x19, 0x0e, 0x8e, 0xed, 0xd4, 0x28, 0x45, 0x0c, 0x5d, 0x22, 0xe3, 0x7e,
	0x8a, 0x82, 0x52, 0xdb, 0xd8, 0x6e, 0x45, 0x5f, 0x02, 0xb1, 0xf2, 0x0a, 0x88, 0x07, 0x61, 0xec,
	0xd8, 0x11, 0xb5, 0x2f, 0x82, 0x12, 0xa7, 0x05, 0x97, 0x0c, 0x15, 0xb7, 0xd9, 0x3f, 0xff, 0xfe,
	0xf9, 0xb3, 0x8c, 0x5f, 0x49, 0x05, 0xc2, 0x42, 0x05, 0x53, 0xb0, 0x7a, 0x91, 0x28, 0x2d, 0xad,
	0x4c, 0xb8, 0xac, 0x2a, 0xe0, 0x56, 0xea, 0xa4, 0x46, 0x4b, 0x6e, 0x92, 0x79, 0x7f, 0xbb, 0xcc,
	0x0d, 0xe8, 0x79, 0xc9, 0x81, 0x36, 0x54, 0x12, 0x7b, 0x7a, 0x07, 0xd2, 0x9d, 0x9e, 0xb6, 0x22,
	0x3a, 0xef, 0xf7, 0x2e, 0x0a, 0x59, 0x48, 0xe7, 0x5f, 0xaf, 0x1c, 0xb5, 0xf7, 0xbc, 0x2b, 0xff,
	0xdf, 0x54, 0xc7, 0x8e, 0x16, 0xf8, 0xf1, 0xcd, 0x67, 0x25, 0xb5, 0xbd, 0x75, 0xf0, 0xc8, 0x75,
	0xc9, 0xe0, 0xd3, 0x0c, 0x8c, 0x25, 0x63, 0xfc, 0x50, 0x83, 0x91, 0x33, 0xcd, 0x21, 0x6f, 0x85,
	0x97, 0xe8, 0xea, 0x38, 0x3e, 0x1d, 0x24, 0xb4, 0xab, 0xe7, 0x9f, 0x76, 0x34, 0x6b, 0x75, 0xad,
	0x71, 0x76, 0xae, 0x7d, 0x20, 0xfa, 0x82, 0xf0, 0x93, 0xee, 0x6c, 0xa3, 0xa4, 0x30, 0x40, 0x04,
	0x3e, 0x57, 0x4c, 0xdb, 0x92, 0x55, 0xb9, 0x99, 0x71, 0x0e, 0xa6, 0xce, 0x46, 0xf1, 0xe9, 0xe0,
	0x86, 0x1e, 0x3a, 0x23, 0xea, 0x05, 0xa4, 0xce, 0x6d, 0xe4, 0xcc, 0xb2, 0x33, 0xe5, 0xed, 0x23,
	0xbb, 0x37, 0x0b, 0x9f, 0x4e, 0x5e, 0xe0, 0x0b, 0x0d, 0x1f, 0x81, 0x5b, 0x98, 0xe4, 0x13, 0x66,
	0x59, 0xae, 0x64, 0x29, 0xac, 0xeb, 0x74, 0x9c, 0x91, 0xed, 0xd9, 0x1b, 0x66, 0x59, 0xda, 0x9c,
	0x90, 0xa7, 0xf8, 0x01, 0x68, 0x2d, 0x75, 0x3e, 0x05, 0x63, 0x58, 0x01, 0x97, 0x47, 0x57, 0x28,
	0xbe, 0x97, 0xdd, 0x6f, 0xc0, 0x5b, 0x87, 0x0d, 0x7e, 0x20, 0x7c, 0xe6, 0x0f, 0x80, 0x7c, 0x43,
	0xf8, 0xc4, 0x35, 0x21, 0xff, 0x7b, 0x55, 0xff, 0x1d, 0x7b, 0x6f, 0xef, 0x6a, 0xe3, 0x9e, 0x24,
	0x0a, 0xae, 0x57, 0xe8, 0xe7, 0x3a, 0x44, 0xcb, 0x75, 0x88, 0x7e, 0xad, 0x43, 0xf4, 0x75, 0x13,
	0x06, 0xcb, 0x4d, 0x18, 0xac, 0x36, 0x61, 0x80, 0x9f, 0x95, 0xf2, 0xe0, 0x98, 0xeb, 0x47, 0x7e,
	0x42, 0x5a, 0x33, 0x53, 0x34, 0x1e, 0x16, 0xfb, 0x1e, 0xe5, 0xdf, 0xbf, 0x47, 0xd5, 0x83, 0x4f,
	0x4a, 0x61, 0x41, 0x0b, 0x56, 0x25, 0xcd, 0xae, 0x09, 0x29, 0x40, 0x74, 0x7e, 0xb2, 0xef, 0x47,
	0xf1, 0x50, 0x81, 0x78, 0xb7, 0xb3, 0x6b, 0x82, 0xe8, 0xeb, 0x5d, 0xa5, 0xb6, 0x06, 0x7d, 0xdf,
	0xff, 0x70, 0xd2, 0x38, 0xbd, 0xfc, 0x1d, 0x00, 0x00, 0xff, 0xff, 0xc7, 0x82, 0x2e, 0x21, 0xc2,
	0x03, 0x00, 0x00,
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// MetricsServiceClient is the client API for MetricsService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
type MetricsServiceClient interface {
	// For performance reasons, it is recommended to keep this RPC
	// alive for the entire life of the application.
	Export(ctx context.Context, in *ExportMetricsServiceRequest, opts ...grpc.CallOption) (*ExportMetricsServiceResponse, error)
}

type metricsServiceClient struct {
	cc *grpc.ClientConn
}

func NewMetricsServiceClient(cc *grpc.ClientConn) MetricsServiceClient {
	return &metricsServiceClient{cc}
}

func (c *metricsServiceClient) Export(ctx context.Context, in *ExportMetricsServiceRequest, opts ...grpc.CallOption) (*ExportMetricsServiceResponse, error) {
	out := new(ExportMetricsServiceResponse)
	err := c.cc.Invoke(ctx, "/opentelemetry.proto.collector.metrics.v1.MetricsService/Export", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// MetricsServiceServer is the server API for MetricsService service.
type MetricsServiceServer interface {
	// For performance reasons, it is recommended to keep this RPC
	// alive for the entire life of the application.
	Export(context.Context, *ExportMetricsServiceRequest) (*ExportMetricsServiceResponse, error)
}

// UnimplementedMetricsServiceServer can be embedded to have forward compatible implementations.
type UnimplementedMetricsServiceServer struct {
}

func (*UnimplementedMetricsServiceServer) Export(ctx context.Context, req *ExportMetricsServiceRequest) (*ExportMetricsServiceResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Export not implemented")
}

func RegisterMetricsServiceServer(s *grpc.Server, srv MetricsServiceServer) {
	s.RegisterService(&_MetricsService_serviceDesc, srv)
}

func _MetricsService_Export_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ExportMetricsServiceRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetricsServiceServer).Export(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/opentelemetry.proto.collector.metrics.v1.MetricsService/Export",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetricsServiceServer).Export(ctx, req.(*ExportMetricsServiceRequest))
	}
	return interceptor(ctx, in, info, handler)
}

var _MetricsService_serviceDesc = grpc.ServiceDesc{
	ServiceName: "opentelemetry.proto.collector.metrics.v1.MetricsService",
	HandlerType: (*MetricsServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "Export",
			Handler:    _MetricsService_Export_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "opentelemetry/proto/collector/metrics/v1/metrics_service.proto",
}

func (m *ExportMetricsServiceRequest) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *ExportMetricsServiceRequest) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *ExportMetricsServiceRequest) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if len(m.ResourceMetrics) > 0 {
		for iNdEx := len(m.ResourceMetrics) - 1; iNdEx >= 0; iNdEx-- {
			{
				size, err := m.ResourceMetrics[iNdEx].MarshalToSizedBuffer(dAtA[:i])
				if err != nil {
					return 0, err
				}
				i -= size
				i = encodeVarintMetricsService(dAtA, i, uint64(size))
			}
			i--
			dAtA[i] = 0xa
		}
	}
	return len(dAtA) - i, nil
}

func (m *ExportMetricsServiceResponse) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *ExportMetricsServiceResponse) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *ExportMetricsServiceResponse) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if m.PartialSuccess != nil {
		{
			size, err := m.PartialSuccess.MarshalToSizedBuffer(dAtA[:i])
			if err != nil {
				return 0, err
			}
			i -= size
			i = encodeVarintMetricsService(dAtA, i, uint64(size))
		}
		i--
		dAtA[i] = 0xa
	}
	return len(dAtA) - i, nil
}

func (m *ExportMetricsPartialSuccess) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *ExportMetricsPartialSuccess) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *ExportMetricsPartialSuccess) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if len(m.ErrorMessage) > 0 {
		i -= len(m.ErrorMessage)
		copy(dAtA[i:], m.ErrorMessage)
		i = encodeVarintMetricsService(dAtA, i, uint64(len(m.ErrorMessage)))
		i--
		dAtA[i] = 0x12
	}
	if m.RejectedDataPoints != 0 {
		i = encodeVarintMetricsService(dAtA, i, uint64(m.RejectedDataPoints))
		i--
		dAtA[i] = 0x8
	}
	return len(dAtA) - i, nil
}

func encodeVarintMetricsService(dAtA []byte, offset int, v uint64) int {
	offset -= sovMetricsService(v)
	base := offset
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return base
}
func (m *ExportMetricsServiceRequest) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	if len(m.ResourceMetrics) > 0 {
		for _, e := range m.ResourceMetrics {
			l = e.Size()
			n += 1 + l + sovMetricsService(uint64(l))
		}
	}
	return n
}

func (m *ExportMetricsServiceResponse) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	if m.PartialSuccess != nil {
		l = m.PartialSuccess.Size()
		n += 1 + l + sovMetricsService(uint64(l))
	}
	return n
}

func (m *ExportMetricsPartialSuccess) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	if m.RejectedDataPoints != 0 {
		n += 1 + sovMetricsService(uint64(m.RejectedDataPoints))
	}
	l = len(m.ErrorMessage)
	if l > 0 {
		n += 1 + l + sovMetricsService(uint64(l))
	}
	return n
}

func sovMetricsService(x uint64) (n int) {
	return (math_bits.Len64(x|1) + 6) / 7
}
func sozMetricsService(x uint64) (n int) {
	return sovMetricsService(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *ExportMetricsServiceRequest) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMetricsService
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= uint64(b&0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: ExportMetricsServiceRequest: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: ExportMetricsServiceRequest: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ResourceMetrics", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= int(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthMetricsService
			}
			postIndex := iNdEx + msglen
			if postIndex < 0 {
				return ErrInvalidLengthMetricsService
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.ResourceMetrics = append(m.ResourceMetrics, &v1.ResourceMetrics{})
			if err := m.ResourceMetrics[len(m.ResourceMetrics)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipMetricsService(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if (skippy < 0) || (iNdEx+skippy) < 0 {
				return ErrInvalidLengthMetricsService
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *ExportMetricsServiceResponse) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMetricsService
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= uint64(b&0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: ExportMetricsServiceResponse: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: ExportMetricsServiceResponse: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field PartialSuccess", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= int(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthMetricsService
			}
			postIndex := iNdEx + msglen
			if postIndex < 0 {
				return ErrInvalidLengthMetricsService
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.PartialSuccess == nil {
				m.PartialSuccess = &ExportMetricsPartialSuccess{}
			}
			if err := m.PartialSuccess.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipMetricsService(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if (skippy < 0) || (iNdEx+skippy) < 0 {
				return ErrInvalidLengthMetricsService
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *ExportMetricsPartialSuccess) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMetricsService
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= uint64(b&0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: ExportMetricsPartialSuccess: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: ExportMetricsPartialSuccess: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field RejectedDataPoints", wireType)
			}
			m.RejectedDataPoints = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.RejectedDataPoints |= int64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ErrorMessage", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthMetricsService
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthMetricsService
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.ErrorMessage = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipMetricsService(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if (skippy < 0) || (iNdEx+skippy) < 0 {
				return ErrInvalidLengthMetricsService
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func skipMetricsService(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	depth := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowMetricsService
			}
			if iNdEx >= l {
				return 0, io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		wireType := int(wire & 0x7)
		switch wireType {
		case 0:
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				iNdEx++
				if dAtA[iNdEx-1] < 0x80 {
					break
				}
			}
		case 1:
			iNdEx += 8
		case 2:
			var length int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowMetricsService
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				length |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if length < 0 {
				return 0, ErrInvalidLengthMetricsService
			}
			iNdEx += length
		case 3:
			depth++
		case 4:
			if depth == 0 {
				return 0, ErrUnexpectedEndOfGroupMetricsService
			}
			depth--
		case 5:
			iNdEx += 4
		default:
			return 0, fmt.Errorf("proto: illegal wireType %d", wireType)
		}
		if iNdEx < 0 {
			return 0, ErrInvalidLengthMetricsService
		}
		if depth == 0 {
			return iNdEx, nil
		}
	}
	return 0, io.ErrUnexpectedEOF
}

var (
	ErrInvalidLengthMetricsService        = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowMetricsService          = fmt.Errorf("proto: integer overflow")
	ErrUnexpectedEndOfGroupMetricsService = fmt.Errorf("proto: unexpected end of group")
)
