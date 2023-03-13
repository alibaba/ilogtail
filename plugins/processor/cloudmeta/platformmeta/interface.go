package platformmeta

const (
	FlagInstanceID         = "__cloud_instance_id__"
	FlagInstanceName       = "__cloud_instance_name__"
	FlagInstanceRegion     = "__cloud_region__"
	FlagInstanceZone       = "__cloud_zone__"
	FlagInstanceVpcID      = "__cloud_vpc_id__"
	FlagInstanceVswitchID  = "__cloud_vswitch_id__"
	FlagInstanceTags       = "__cloud_instance_tags__"
	FlagInstanceType       = "__cloud_instance_type__"
	FlagInstanceImageID    = "__cloud_image_id__"
	FlagInstanceMaxIngress = "__cloud_max_ingress__"
	FlagInstanceMaxEgress  = "__cloud_max_egress__"

	FlagInstanceIDWrapper         = "{{" + FlagInstanceID + "}}"
	FlagInstanceNameWrapper       = "{{" + FlagInstanceName + "}}"
	FlagInstanceRegionWrapper     = "{{" + FlagInstanceRegion + "}}"
	FlagInstanceZoneWrapper       = "{{" + FlagInstanceZone + "}}"
	FlagInstanceVpcIDWrapper      = "{{" + FlagInstanceVpcID + "}}"
	FlagInstanceVswitchIDWrapper  = "{{" + FlagInstanceVswitchID + "}}"
	FlagInstanceTagsWrapper       = "{{" + FlagInstanceTags + "}}"
	FlagInstanceTypeWrapper       = "{{" + FlagInstanceType + "}}"
	FlagInstanceImageIDWrapper    = "{{" + FlagInstanceImageID + "}}"
	FlagInstanceMaxIngressWrapper = "{{" + FlagInstanceMaxIngress + "}}"
	FlagInstanceMaxEgressWrapper  = "{{" + FlagInstanceMaxEgress + "}}"
)

type Manager interface {
	StartCollect()
	GetInstanceID() string
	GetInstanceImageID() string
	GetInstanceType() string
	GetInstanceRegion() string
	GetInstanceZone() string
	GetInstanceName() string
	GetInstanceVpcID() string
	GetInstanceVswitchID() string
	GetInstanceMaxNetEgress() int64
	GetInstanceMaxNetIngress() int64
	GetInstanceTags() map[string]string
	Ping() bool
}

type Platform string
type MetaType string

const (
	Aliyun Platform = "aliyun"
	Mock   Platform = "mock"
	Auto   Platform = "auto"
)

var register map[Platform]Manager

func GetManager(platform Platform) Manager {
	if platform == Auto {
		for _, manager := range register {
			if manager.Ping() {
				return manager
			}
		}
		return nil
	}
	return register[platform]
}

func init() {
	register = make(map[Platform]Manager)
	initAliyun()
	initMock()
}
