package helper

import (
	"strconv"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

var FILE_OFFSET_KEY = "__tag__:__file_offset__"

func ReviseFileOffset(log *protocol.Log, offset int64) {
	for _, cont := range log.Contents {
		if cont.Key == FILE_OFFSET_KEY {
			if beginOffset, err := strconv.ParseInt(cont.Value, 10, 64); err == nil {
				cont.Value = strconv.FormatInt(beginOffset+offset, 10)
			}
			return
		}
	}
}
