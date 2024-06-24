
#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>

// Metrics Data
enum MeasureType {MEASURE_TYPE_APP, MEASURE_TYPE_NET, MEASURE_TYPE_PROCESS, MEASURE_TYPE_MAX};

struct AbstractSingleMeasure {

};

struct NetSingleMeasre : public AbstractSingleMeasure {
  uint64_t tcp_drop_total_;
  uint64_t tcp_retran_total_;
  uint64_t tcp_connect_total_;
  uint64_t tcp_rtt_;
  uint64_t tcp_rtt_var_;
  
  uint64_t recv_pkt_total_;
  uint64_t send_pkt_total_;
  uint64_t recv_byte_total_;
  uint64_t send_byte_total_;
};

struct AppSingleMeasure : public AbstractSingleMeasure {
  uint64_t request_total_;
  uint64_t slow_total_;
  uint64_t error_total_;
  uint64_t duration_ms_sum_;
  uint64_t status_2xx_count_;
  uint64_t status_3xx_count_;
  uint64_t status_4xx_count_;
  uint64_t status_5xx_count_;
};

struct Measure {
  std::map<std::string, std::string> tags_;
  MeasureType type_;
  std::unique_ptr<AbstractSingleMeasure> inner_measure_;
};

struct ApplicationBatchMeasure {
  std::string app_id_;
  std::string region_id_; 
  std::string ip_;
  std::vector<std::unique_ptr<Measure>> measures_;
};

enum SpanKindInner { Unspecified, Internal, Server, Client, Producer, Consumer };
struct SingleSpan {
  std::map<std::string, std::string> tags_;
  std::string trace_id_;
  std::string span_id_;
  std::string span_name_;
  SpanKindInner span_kind_;

  uint64_t start_timestamp_;
  uint64_t end_timestamp_;
};

struct ApplicationBatchSpan {
  std::string app_id_;
  std::vector<std::unique_ptr<SingleSpan>> single_spans_;
};

using HandleBatchMeasureFunc = std::function<void(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp)>;
using HandleBatchSpanFunc = std::function<void(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans)>;

struct SockettraceConfig {
  bool enable_libbpf_debug_ = false;
  bool enable_so_ = false;
  std::string btf_;
  int32_t btf_size;
  std::string so_;
  int32_t so_size_;
  long uprobe_offset_;
  long upca_offset_;
  long upps_offset_;
  long upcr_offset_;
  HandleBatchMeasureFunc measure_cb_;
  HandleBatchSpanFunc span_cb_;
};