package command

import (
	"fmt"
	"regexp"
	"strconv"
	"strings"
)

type Decoder interface {
	Decode(value string) *DecoderResult
}

type DecoderResult struct {
	Labels       map[string]string
	MetricName   string
	Value        string
	Err          error
	Float64Value float64
}

func (decoderResult *DecoderResult) ToFloat64() {
	floatValue, err := strconv.ParseFloat(decoderResult.Value, 64)
	if err != nil {
		decoderResult.Err = fmt.Errorf("can not convert to float64 value:%s", decoderResult.Value)
		return
	}
	decoderResult.Float64Value = floatValue
}

type SlsMetricsDecoder struct {
	DecoderResult *DecoderResult
	InputValue    string
	SplitSep      string
}

//__labels__:hostname#$#idc_cluster_env_name|ip#$#ip_address   __value__:0.0  __name__:metric_command_example
func (sls *SlsMetricsDecoder) Decode(value string) *DecoderResult {
	value = strings.Trim(value, " ")
	//先校验是否合法 不合法则认为是注释或其他字符串 忽略解析
	pattern := `^(__.*?__\:[^\s]+\s*?){3,4}$`
	r := regexp.MustCompile(pattern)
	if r.MatchString(value) == false {
		sls.DecoderResult.Err = fmt.Errorf("validate value error value[%s]", value)
		return sls.DecoderResult
	}

	items := strings.Split(value, sls.SplitSep)
	for _, item := range items {
		item = strings.Trim(item, " ")
		if item == "" {
			continue
		}
		//处理注释的情况
		if string(item[0]) == "#" {
			continue
		}
		k, v := sls.decodeItemKv(item)
		switch k {
		case "__name__":
			sls.DecoderResult.MetricName = v
		case "__value__":
			sls.DecoderResult.Value = v
			sls.DecoderResult.ToFloat64()
			//如果转换float64失败 直接返回
			if sls.DecoderResult.Err != nil {
				return sls.DecoderResult
			}
		case "__labels__":
			sls.DecoderResult.Labels = sls.decodeToLabels(v)
		}

	}
	if sls.DecoderResult.Err != nil {
		fmt.Print("errpr on decode", sls.DecoderResult.Err)
	}
	return sls.DecoderResult
}

func (sls *SlsMetricsDecoder) decodeToLabels(labelString string) map[string]string {
	labelMap := make(map[string]string, 0)
	labelsArr := strings.Split(labelString, "|")
	for _, v := range labelsArr {
		kvArr := strings.Split(v, "#$#")
		if len(kvArr) == 1 {
			labelMap[kvArr[0]] = ""
		} else if len(kvArr) == 2 {
			labelMap[kvArr[0]] = kvArr[1]
		}
	}
	return labelMap

}
func (sls *SlsMetricsDecoder) decodeItemKv(item string) (k, v string) {
	kvArr := strings.Split(item, ":")
	kvArrlen := len(kvArr)
	if kvArrlen == 1 {
		return kvArr[0], ""
	} else if kvArrlen == 2 {
		return kvArr[0], kvArr[1]
	} else {
		sls.DecoderResult.Err = fmt.Errorf("error on decodeItemKv from string[%s]", item)
		return "__emptykey__", "__emptyvalue__"
	}
}

//__labels__:hostname#$#idc_cluster_env_name|ip#$#ip_address   __value__:0.0  __name__:metric_command_example
//
func GetDecoder(dataType string) Decoder {
	if dataType == SlsMetricDataType {
		decoder := &SlsMetricsDecoder{
			SplitSep:      " ",
			DecoderResult: &DecoderResult{},
		}
		return decoder
	}
	return nil
}
