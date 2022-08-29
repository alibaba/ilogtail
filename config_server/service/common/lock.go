package common

import (
	"sync"
)

type IdMutex map[string]*sync.RWMutex

var myIdMutex *IdMutex

func Mutex() *IdMutex {
	return myIdMutex
}

func init() {
	myIdMutex = new(IdMutex)
	*myIdMutex = make(map[string]*sync.RWMutex)
}

// lock read
func (this *IdMutex) RLock(id string) {
	m, ok := (*this)[id]
	if !ok {
		m = new(sync.RWMutex)
		(*this)[id] = m
	}
	m.RLock()
}

// unlock read
func (this *IdMutex) RUnlock(id string) {
	m, ok := (*this)[id]
	if !ok {
		return
	}
	m.RUnlock()
}

// lock write
func (this *IdMutex) Lock(id string) {
	m, ok := (*this)[id]
	if !ok {
		m = new(sync.RWMutex)
		(*this)[id] = m
	}
	m.Lock()
}

// unlock write
func (this *IdMutex) Unlock(id string) {
	m, ok := (*this)[id]
	if !ok {
		return
	}
	m.Unlock()
}
