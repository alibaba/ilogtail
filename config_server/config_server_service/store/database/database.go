package database

/*
Interface of store
*/

type Database interface {
	Connect() error
	GetMode() string // store mode
	Close() error

	Get(table string, entityKey string) (interface{}, error)
	Add(table string, entityKey string, entity interface{}) error
	Mod(table string, entityKey string, entity interface{}) error
	Has(table string, entityKey string) (bool, error)
	Delete(table string, entityKey string) error
	GetAll(table string) ([]interface{}, error)

	CreateBatch() Batch

	CheckAll() // for test
}
