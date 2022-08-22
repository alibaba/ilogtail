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

// batch

const (
	OPT_ADD    string = "ADD"
	OPT_DELETE string = "DELETE"
	OPT_UPDATE string = "UPDATE"
)

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
	b.datas.Push(data{OPT_ADD, table, entityKey, entity})
}

func (b *Batch) Update(table string, entityKey string, entity interface{}) {
	b.datas.Push(data{OPT_UPDATE, table, entityKey, entity})
}

func (b *Batch) Delete(table string, entityKey string) {
	b.datas.Push(data{OPT_DELETE, table, entityKey, nil})
}

func (b *Batch) Empty() bool {
	return b.datas.Empty()
}

func (b *Batch) Pop() data {
	return b.datas.Pop().(data)
}
