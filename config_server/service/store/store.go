package store

type Store interface {
	Connect() error
	Close() error

	CreateTables() error
}
