package k8s_event

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
	CreateTagAlarm Alarm = "CREATE_TAG_ALARM"

	DeleteConfigAlarm Alarm = "DELETE_CONFIG_ALARM"

	UpdateConfigAlarm Alarm = "UPDATE_CONFIG_ALARM"

	UpdateConfigStatusAlarm Alarm = "UPDATE_CONFIG_STATUS_ALARM"

	CreateProjectAlarm Alarm = "CREATE_PROJECT_ALARM"

	CreateLogstoreAlarm Alarm = "CREATE_LOGSTORE_ALARM"

	CreateIndexAlarm Alarm = "CREATE_INDEX_ALARM"

	CreateProductLogStoreAlarm Alarm = "CREATE_PRODUCT_LOGSTORE_ALARM"

	CreateProductAppAlarm Alarm = "CREATE_PRODUCT_APP_ALARM"
)

func NewEventDefine(ModuleCode string) *EventDefine {
	eventDefine := &EventDefine{}
	eventDefine.currentModule = ModuleCode
	return eventDefine
}

func (e *EventDefine) getErrorAction(Action Action, alarm Alarm) string {
	return fmt.Sprintf("%s.%s.%s.%s", ProductCode, e.currentModule, Action, alarm)
}

func (e *EventDefine) getInfoAction(Action Action) string {
	return fmt.Sprintf("%s.%s.%s.%s", ProductCode, e.currentModule, Action, "Success")
}
