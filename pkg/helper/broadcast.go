// Copyright 2024 iLogtail Authors
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

package helper

import (
	"sync"
)

type broadcaster struct {
	input chan any
	reg   chan chan<- any
	unreg chan chan<- any

	outputs   map[chan<- any]bool
	stopCh    chan struct{}
	wg        sync.WaitGroup
	sync.Once // close once
}

// The Broadcaster interface describes the main entry points to
// broadcasters.
type Broadcaster interface {
	// Register a new channel to receive broadcasts
	Register(chan<- any)
	// Unregister a channel so that it no longer receives broadcasts.
	Unregister(chan<- any)
	// Shut this broadcaster down.
	Close() error
	// Submit a new object to all subscribers
	Submit(any)
	// Try Submit a new object to all subscribers return false if input chan is fill
	TrySubmit(any) bool
}

func (b *broadcaster) broadcast(m any) {
	for ch := range b.outputs {
		select {
		case ch <- m:
		default:
		}
	}
}

func (b *broadcaster) run() {
	defer b.wg.Done()
	for {
		select {
		case <-b.stopCh:
			return
		case m := <-b.input:
			b.broadcast(m)
		case ch, ok := <-b.reg:
			if ok {
				b.outputs[ch] = true
			}
		case ch := <-b.unreg:
			delete(b.outputs, ch)
		}
	}
}

// NewBroadcaster creates a new broadcaster with the given input
// channel buffer length.
func NewBroadcaster(buflen int) Broadcaster {
	b := &broadcaster{
		input:   make(chan any, buflen),
		reg:     make(chan chan<- any),
		unreg:   make(chan chan<- any),
		outputs: make(map[chan<- any]bool),
		stopCh:  make(chan struct{}),
	}

	b.wg.Add(1)
	go b.run()

	return b
}

func (b *broadcaster) Register(newch chan<- any) {
	b.reg <- newch
}

func (b *broadcaster) Unregister(newch chan<- any) {
	b.unreg <- newch
}

func (b *broadcaster) Close() error {
	b.Do(func() {
		close(b.stopCh)
		b.wg.Wait()
		close(b.reg)               // not allowed to register anymore.
		close(b.unreg)             // not allowed to unregister anymore.
		close(b.input)             // not allowed to submit anymore.
		for v := range b.outputs { // close all registered channel.
			close(v)
		}
	})
	return nil
}

// Submit an item to be broadcast to all listeners.
func (b *broadcaster) Submit(m any) {
	if b != nil {
		b.input <- m
	}
}

// TrySubmit attempts to submit an item to be broadcast, returning
// true iff it the item was broadcast, else false.
func (b *broadcaster) TrySubmit(m any) bool {
	if b == nil {
		return false
	}
	select {
	case b.input <- m:
		return true
	default:
		return false
	}
}
