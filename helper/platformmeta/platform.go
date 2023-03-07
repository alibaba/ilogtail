package platformmeta

import "strings"

const (
	AliyunEcs = "aliyun-ecs"
)

type CollectPlatformMeta func(input map[string]string, minimize bool) map[string]string

func GetPlatformMetaCollectors(name string) CollectPlatformMeta {
	name = strings.ToLower(name)
	switch name {
	case AliyunEcs:
		return AlibabaCloudEcsPlatformMetaCollect
	default:
		return nil
	}
}
