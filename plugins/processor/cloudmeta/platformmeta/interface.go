package platformmeta

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
}

type Platform string
type MetaType string

const (
	Aliyun Platform = "aliyun"
	Mock   Platform = "mock"
)

var register map[Platform]Manager

func GetManager(platform Platform) Manager {
	return register[platform]
}

func init() {
	register = make(map[Platform]Manager)
	initAliyun()
	initMock()
}
