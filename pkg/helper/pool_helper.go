// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
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

// GenericPool 是一个泛型结构体，它包含了sync.Pool，它的类型通过类型参数T指定。
type GenericPool[T any] struct {
	sync.Pool
}

// NewGenericPool 创建一个新的 GenericPool 实例，使用构造函数fn来初始化对象。
func NewGenericPool[T any](fn func() []T) GenericPool[T] {
	return GenericPool[T]{
		sync.Pool{
			New: func() interface{} {
				t := fn()
				return &t
			},
		},
	}
}

// Get 从池中检索一个类型为 *T 的对象。
func (p *GenericPool[T]) Get() *[]T {
	res := p.Pool.Get().(*[]T)
	*res = (*res)[:0]
	return res
}

// Put 将一个类型为 *T 的对象放回池中。
func (p *GenericPool[T]) Put(t *[]T) {
	if t != nil {
		p.Pool.Put(t)
	}
}
