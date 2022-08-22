package database

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
)

/*
Interface of store
*/

type Database interface {
	Connect() error
	GetMode() string // store mode
	Close() error

	Get(table string, entityKey string) (interface{}, error)
	Add(table string, entityKey string, entity interface{}) error
	Update(table string, entityKey string, entity interface{}) error
	Has(table string, entityKey string) (bool, error)
	Delete(table string, entityKey string) error
	GetAll(table string) ([]interface{}, error)

	WriteBatch(batch *Batch) error

	CheckAll() // for test
}

type (
	data struct {
		Opt   string
		Table string
		Key   string
		Value interface{}
	}

	Batch struct {
		datas common.Queue
	}
)

func (b *Batch) Add(table string, entityKey string, entity interface{}) {
	b.datas.Push(data{"A", table, entityKey, entity})
}

func (b *Batch) Update(table string, entityKey string, entity interface{}) {
	b.datas.Push(data{"U", table, entityKey, entity})
}

func (b *Batch) Delete(table string, entityKey string) {
	b.datas.Push(data{"D", table, entityKey, nil})
}

func (b *Batch) Empty() bool {
	return b.datas.Empty()
}

func (b *Batch) Pop() data {
	return b.datas.Pop().(data)
}
