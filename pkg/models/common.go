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

package models

type EventType uint32

const (
	_ EventType = iota
	EventTypeMetric
	EventTypeTrace
	EventTypeLogging
)

type ValueType uint32

const (
	_ ValueType = iota
	ValueTypeString
	ValueTypeBoolean
	ValueTypeArray
	ValueTypeMap
)

type TypedValue struct {
	Type  ValueType
	Value interface{}
}

type KeyValues[TValue string | float64 | *TypedValue] interface {
	Add(key string, value TValue)

	Get(key string) TValue

	Contains(key string) bool

	Delete(key string)

	Merge(other KeyValues[TValue])

	Iterator() map[string]TValue
}

type Tags interface {
	KeyValues[string]
}

type Metadata interface {
	KeyValues[string]
}

type keyValuesImpl[TValue string | float64 | *TypedValue] struct {
	keyValues map[string]TValue
}

func (kv *keyValuesImpl[TValue]) Add(key string, value TValue) {
	kv.keyValues[key] = value
}

func (kv *keyValuesImpl[TValue]) Get(key string) TValue {
	return kv.keyValues[key]
}

func (kv *keyValuesImpl[TValue]) Contains(key string) bool {
	_, ok := kv.keyValues[key]
	return ok
}

func (kv *keyValuesImpl[TValue]) Delete(key string) {
	delete(kv.keyValues, key)
}

func (kv *keyValuesImpl[TValue]) Merge(other KeyValues[TValue]) {
	for k, v := range other.Iterator() {
		kv.keyValues[k] = v
	}
}

func (kv *keyValuesImpl[TValue]) Iterator() map[string]TValue {
	return kv.keyValues
}

func NewTagsWithMap(tags map[string]string) Tags {
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewTagsWithKeyValues(keyValues ...string) Tags {
	if len(keyValues)%2 != 0 {
		keyValues = keyValues[:len(keyValues)-1]
	}
	tags := make(map[string]string)
	for i := 0; i < len(keyValues); i += 2 {
		tags[keyValues[i]] = keyValues[i+1]
	}
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewTags() Tags {
	return &keyValuesImpl[string]{
		keyValues: make(map[string]string),
	}
}
