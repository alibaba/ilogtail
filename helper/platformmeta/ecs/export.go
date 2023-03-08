package ecs

func GetInstanceID() string {
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.id
}

func GetInstanceImageID() string {
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.imageID
}

func GetInstanceRegion() string {
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.region
}

func GetInstanceZone() string {
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.zone
}

func GetInstanceType() string {
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.instanceType
}

func GetInstanceName() string {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.name
}

func GetInstanceMaxNetEngress() int64 {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
	manager.startFetch()
	if !manager.fetchRes {
		return -1
	}
	return manager.data.maxNetEngress
}

func GetInstanceMaxNetIngress() int64 {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
	manager.startFetch()
	if !manager.fetchRes {
		return -1
	}
	return manager.data.maxNetIngress
}

func GetInstanceVpcID() string {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.vpcID
}

func GetInstanceVswitchID() string {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
	manager.startFetch()
	if !manager.fetchRes {
		return ""
	}
	return manager.data.vswitchID
}

func GetInstanceTags() map[string]string {
	manager.mutex.RLock()
	defer manager.mutex.RUnlock()
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
