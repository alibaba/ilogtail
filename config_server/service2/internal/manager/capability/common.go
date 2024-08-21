package capability

type Base struct {
	Code  int
	Value string
}

// server
var (
	ServerUnspecified            = Base{0, "unspecified"}
	RememberAttribute            = Base{1, "rememberAttribute"}
	RememberPipelineConfigStatus = Base{2, "rememberPipelineConfigStatus"}
	RememberProcessConfigStatus  = Base{4, "rememberProcessConfigStatus"}
	RememberCustomCommandStatus  = Base{8, "rememberCustomCommandStatus"}
)

// agent
var (
	AgentUnSpecified      = Base{0, "unspecified"}
	AcceptsPipelineConfig = Base{1, "acceptsPipelineConfig"}
	AcceptsProcessConfig  = Base{2, "acceptsProcessConfig"}
	AcceptsCustomCommand  = Base{4, "acceptsCustomCommand"}
)
