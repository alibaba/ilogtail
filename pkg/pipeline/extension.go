package pipeline

// Extension ...
type Extension interface {
	// Description returns a one-sentence description on the Extension
	Description() string

	// Init called for init some system resources, like socket, mutex...
	Init(Context) error

	// Stop stops the services and release resources
	Stop() error
}

type ExtensionV1 interface {
	Extension
}

type ExtensionV2 interface {
	Extension
}
