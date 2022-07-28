package store

import (
	"errors"
	"sync"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/tool"
)

type StoreMessage struct {
	opt string // U:new, M:modify, D:delete
	trg string // CO:config, MA:machine, CG:configGroup, MG:machineGroup
	id  string
}

type iStore interface {
	SetName(name string)
	GetName() string
	ReadData()
	UpdateData()
	ShowData()
	SendMessage(opt string, trg string, id string)
}

type store struct {
	name         string
	messageQueue *tool.Queue
}

func (s *store) SetName(name string) {
	s.name = name
}

func (s *store) GetName() string {
	return s.name
}

func (s *store) SendMessage(opt string, trg string, id string) {
	s.messageQueue.Push(StoreMessage{opt, trg, id})
}

func createStore(storeType string) (iStore, error) {
	if storeType == "file" {
		return newJsonStore(), nil
	}
	if storeType == "leveldb" {
		return newLeveldbStore(), nil
	}
	return nil, errors.New("Wrong store type.")
}

var myStore iStore

var storeOnce sync.Once

func GetMyStore() iStore {
	storeOnce.Do(func() {
		var err error
		myStore, err = createStore(GetMyBaseSetting().StoreMode)
		if err != nil {
			panic(err)
		}
	})
	return myStore
}

func Update() {
	GetMyStore().UpdateData()
}
