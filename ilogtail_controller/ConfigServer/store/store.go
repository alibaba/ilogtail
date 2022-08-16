package store

type iStore interface {
	SetMode(mode string) // store mode
	GetMode() string
}
