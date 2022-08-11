package eventrecorder

import "fmt"

const ProductCode = "SLS"

const (
	LogController string = "LogController"
	Logtail       string = "Logtail"
)

type EventDefine struct {
	currentModule string
}

type Action string

const (
	UpdateConfig Action = "UpdateConfig"

	UpdateConfigStatus Action = "UpdateConfigStatus"

	DeleteConfig Action = "DeleteConfig"

	CreateProject Action = "CreateProject"

	CreateLogstore Action = "CreateLogstore"

	CreateTag Action = "CreateTag"

	CreateIndex Action = "CreateIndex"

	CreateProductLogStore Action = "CreateProductLogstore"

	CreateProductApp Action = "CreateProductApp"
)

type Alarm string

const (
	CommonAlarm Alarm = "Fail"
)

func NewEventDefine(moduleCode string) *EventDefine {
	eventDefine := &EventDefine{}
	eventDefine.currentModule = moduleCode
	return eventDefine
}

func (e *EventDefine) getErrorAction(action Action, alarm Alarm) string {
	return fmt.Sprintf("%s.%s.%s.%s", ProductCode, e.currentModule, action, alarm)
}

func (e *EventDefine) getInfoAction(action Action) string {
	return fmt.Sprintf("%s.%s.%s.%s", ProductCode, e.currentModule, action, "Success")
}
