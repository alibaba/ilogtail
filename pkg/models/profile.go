package models

import "encoding/json"

type ProfileValue struct {
	Type    string  // profile type, sum as cpu
	Unit    string  // profile unit, such as sample
	AggType string  // profile aggregation type, such as sum or avg
	Val     float64 // current value
}

func (p *ProfileValue) String() string {
	bytes, _ := json.Marshal(p)
	return string(bytes)
}

type ProfileValues []*ProfileValue
type ProfileStack []string

type ProfileKind int

const (
	_ ProfileKind = iota
	ProfileCPU
	ProfileMem
	ProfileMutex
	ProfileGoRoutines
	ProfileException
	ProfileUnknown
)

type Profile struct {
	// 100 ==>1000
	Name        string        // means the top of the stack
	Stack       ProfileStack  // means the call stack removed the top method.
	StackID     string        // a unique id for current invoke stack
	StartTime   int64         // profile begin time
	EndTime     int64         // profile end time
	Tags        Tags          // profile tags
	Values      ProfileValues // profile value list for multi values
	ProfileID   string        // means the unique id for one profiling
	ProfileType ProfileKind   // profile category, such as profile_cpu
	Language    string        // the language of the profiling target
	DataType    string        // the data structure for profile struct, currently default with CallStack
}

func NewProfileValue(valType, unit, aggType string, val float64) *ProfileValue {
	return &ProfileValue{
		Type:    valType,
		Unit:    unit,
		AggType: aggType,
		Val:     val,
	}
}

func (p ProfileKind) String() string {
	switch p {
	case ProfileCPU:
		return "profile_cpu"
	case ProfileMem:
		return "profile_mem"
	case ProfileMutex:
		return "profile_mutex"
	case ProfileGoRoutines:
		return "profile_goroutines"
	case ProfileException:
		return "profile_exception"
	default:
		return "profile_unknown"
	}
}

func (p *Profile) GetName() string {
	if p != nil {
		return p.Name
	}
	return ""
}

func (p *Profile) SetName(name string) {
	if p != nil {
		p.Name = name
	}
}

func (p *Profile) GetTags() Tags {
	if p != nil {
		return p.Tags
	}
	return noopStringValues
}

func (p *Profile) GetType() EventType {
	return EventTypeProfile
}

func (p *Profile) GetTimestamp() uint64 {
	if p != nil {
		return uint64(p.StartTime)
	}
	return 0
}

func (p *Profile) GetObservedTimestamp() uint64 {
	if p != nil {
		return uint64(p.StartTime)
	}
	return 0
}

func (p *Profile) SetObservedTimestamp(u uint64) {
	if p != nil {
		p.StartTime = int64(u)
	}
}
