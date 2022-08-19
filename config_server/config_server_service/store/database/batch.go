package database

type Batch interface {
	Add(table string, entityKey string, entity interface{}) error
	Mod(table string, entityKey string, entity interface{}) error
	Delete(table string, entityKey string) error

	Write() error
}
