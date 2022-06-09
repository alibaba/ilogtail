package csv

import (
	"encoding/csv"
	"fmt"
	"io"
	"strings"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

type ProcessorCSVDecoder struct {
	SrcKey            string
	IgnoreMissingKey  bool
	DstKeys           []string
	Seperator         string
	TrimLeadingSpace  bool
	PreserveRemained  bool
	KeepSrc           bool
	KeepSrcIfParseErr bool

	sep     rune
	context ilogtail.Context
}

func (p *ProcessorCSVDecoder) Init(context ilogtail.Context) error {
	sepRunes := []rune(p.Seperator)
	if len(sepRunes) != 1 {
		return fmt.Errorf("invalid seperator: %s", p.Seperator)
	}
	p.sep = sepRunes[0]
	p.context = context
	return nil
}

func (*ProcessorCSVDecoder) Description() string {
	return "csv decoder for logtail"
}

func (p *ProcessorCSVDecoder) decodeCSV(log *protocol.Log, value string) bool {
	if len(p.DstKeys) == 0 {
		if p.PreserveRemained {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "_decode_preserve_", Value: value})
		}
		return true
	}

	r := csv.NewReader(strings.NewReader(value))
	r.Comma = p.sep
	r.TrimLeadingSpace = p.TrimLeadingSpace

	var record []string
	record, err := r.Read()
	if err != nil && err != io.EOF {
		logger.Warning(p.context.GetRuntimeContext(), "DECODE_LOG_ALARM", "cannot decode log", err, "log", util.CutString(value, 1024))
		return false
	}
	// Empty value should also be considered as a valid field.
	// (To be compatible with the fact that value with only blank chars
	// and TrimLeadingSpace=true is considered valid by encoding/csv pkg)
	if err == io.EOF {
		record = append(record, "")
	}

	var keyIndex int
	for keyIndex = 0; keyIndex < len(p.DstKeys) && keyIndex < len(record); keyIndex++ {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: p.DstKeys[keyIndex], Value: record[keyIndex]})
	}

	if keyIndex < len(record) {
		var b strings.Builder
		w := csv.NewWriter(&b)
		w.Comma = p.sep
		w.Write(record[keyIndex:])
		w.Flush()
		remained := b.String()
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "_decode_preserve_", Value: remained[:len(remained)-1]})
	}

	if len(p.DstKeys) != len(record) {
		logger.Warning(p.context.GetRuntimeContext(), "DECODE_LOG_ALARM", "decode keys not match, split len", len(record), "log", util.CutString(value, 1024))
	}
	return true
}

func (p *ProcessorCSVDecoder) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	for _, log := range logArray {
		findKey := false
		for i, cont := range log.Contents {
			if len(p.SrcKey) == 0 || p.SrcKey == cont.Key {
				findKey = true
				res := p.decodeCSV(log, cont.Value)
				if !p.shouldKeepSrc(res) {
					log.Contents = append(log.Contents[:i], log.Contents[i+1:]...)
				}
				break
			}
		}
		if !findKey && p.IgnoreMissingKey {
			logger.Warning(p.context.GetRuntimeContext(), "DECODE_FIND_ALARM", "cannot find key", p.SrcKey)
		}
	}
	return logArray
}

func (p *ProcessorCSVDecoder) shouldKeepSrc(res bool) bool {
	return p.KeepSrc || (p.KeepSrcIfParseErr && !res)
}

func init() {
	ilogtail.Processors["processor_csv_decoder"] = func() ilogtail.Processor {
		return &ProcessorCSVDecoder{
			Seperator:         ",",
			PreserveRemained:  true,
			KeepSrcIfParseErr: true,
		}
	}
}
