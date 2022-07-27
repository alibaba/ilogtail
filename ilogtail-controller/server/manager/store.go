package manager

import (
	"errors"
	"ilogtail-controller/tool"
	"sync"
)

type StoreMessage struct {
	opt string // U:new, M:modify, D:delete
	trg string // CO:config, MA:machine, CG:configGroup, MG:machineGroup
	id  string
}

type iStore interface {
	setName(name string)
	getName() string
	readData()
	updateData()
	showData()
	sendMessage(opt string, trg string, id string)
}

type store struct {
	name         string
	messageQueue *tool.Queue
}

func (s *store) setName(name string) {
	s.name = name
}

func (s *store) getName() string {
	return s.name
}

func (s *store) sendMessage(opt string, trg string, id string) {
	s.messageQueue.Push(StoreMessage{opt, trg, id})
}

func createStore(storeType int) (iStore, error) {
	if storeType == 0 {
		return newJsonStore(), nil
	}
	if storeType == 1 {
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
	GetMyStore().updateData()
}
