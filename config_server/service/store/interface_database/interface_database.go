// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
	Count(table string) (int, error)

	WriteBatch(batch *Batch) error
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
