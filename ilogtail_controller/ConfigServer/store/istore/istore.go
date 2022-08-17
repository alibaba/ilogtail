package istore

type IStore interface {
	GetMode() string // store mode
	GetConfigs() IConfig
}
