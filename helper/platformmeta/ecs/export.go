package ecs

func GetInstanceID() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.id
}

func GetInstanceName() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.name
}

func GetInstanceRegion() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.region
}

func GetInstanceZone() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.zone
}

func GetInstanceType() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.instanceType
}

func GetInstanceMaxNetEngress() int64 {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return -1
	}
	return manager.data.maxNetEngress
}

func GetInstanceMaxNetIngress() int64 {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return -1
	}
	return manager.data.maxNetIngress
}

func GetInstanceVpcID() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.vpcID
}

func GetInstanceImageID() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.imageID
}
func GetInstanceVswitchID() string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.vswitchID
}

func GetInstanceTags() map[string]string {
	manager.mutex.Lock()
	defer manager.mutex.Unlock()
	manager.startFetch()
	if !manager.fetchRes {
		return map[string]string{}
	}
	res := make(map[string]string)
	for k, v := range manager.data.tags {
		res[k] = v
	}
	return res
}
