package platformmeta

type MockManager struct {
}

func (m *MockManager) StartCollect() {

}

func (m *MockManager) GetInstanceID() string {
	return "id_xxx"
}

func (m *MockManager) GetInstanceImageID() string {
	return "image_xxx"
}

func (m *MockManager) GetInstanceType() string {
	return "type_xxx"
}

func (m *MockManager) GetInstanceRegion() string {
	return "region_xxx"
}

func (m *MockManager) GetInstanceZone() string {
	return "zone_xxx"
}

func (m *MockManager) GetInstanceName() string {
	return "name_xxx"
}

func (m *MockManager) GetInstanceVpcID() string {
	return "vpc_xxx"
}

func (m *MockManager) GetInstanceVswitchID() string {
	return "vswitch_xxx"
}

func (m *MockManager) GetInstanceMaxNetEgress() int64 {
	return 1000
}

func (m *MockManager) GetInstanceMaxNetIngress() int64 {
	return 100
}

func (m *MockManager) GetInstanceTags() map[string]string {
	return map[string]string{
		"tag_key": "tag_val",
	}
}

func initMock() {
	register[Mock] = &MockManager{}
}
