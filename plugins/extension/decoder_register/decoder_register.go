package decoderregister

import (
	"github.com/alibaba/ilogtail/helper/decoder"
)

func init() {
	decoder.RegisterDecodersAsExtension()
}
