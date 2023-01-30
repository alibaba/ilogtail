package models

type ProfileValue struct {
	Type    string
	Unit    string
	AggType string
	Val     uint64
}

func NewProfileValue(valType, unit, aggType string, val uint64) *ProfileValue {
	return &ProfileValue{
		Type:    valType,
		Unit:    unit,
		AggType: aggType,
		Val:     val,
	}
}

type ProfileValues []*ProfileValue
type ProfileStack []string

type Profile struct {
	Name      string
	Stack     ProfileStack
	StackID   string
	StartTime int64
	EndTime   int64
	Tags      Tags
	Values    ProfileValues
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
	return EventTupeProfile
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
