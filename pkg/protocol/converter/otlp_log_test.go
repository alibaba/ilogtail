package protocol

import (
	"testing"

	"github.com/smartystreets/goconvey/convey"
)

func TestNewConvertToOtlpLogs(t *testing.T) {
	convey.Convey("When constructing converter with supported encoding", t, func() {
		_, err := NewConverter(ProtocolOtlpLog, EncodingNone, nil, nil)
		convey.So(err, convey.ShouldBeNil)
	})
	convey.Convey("When constructing converter with unsupported encoding", t, func() {
		_, err := NewConverter(ProtocolOtlpLog, EncodingJSON, nil, nil)
		convey.So(err, convey.ShouldNotBeNil)
	})
}
