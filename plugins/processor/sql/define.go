// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package sql

import (
	"regexp"

	"github.com/alibaba/ilogtail/pkg/models"
)

// compare functions
const (
	Lt     = "<"
	Eq     = "="
	Gt     = ">"
	Neq    = "!="
	Regex  = "REGEXP"
	Nregex = "NOT REGEXP"
	Like   = "LIKE"
	Nlike  = "NOT LIKE"
	And    = "AND"
	Or     = "OR"
)

// expression types
const (
	NoneType = iota
	AttrType
	ConstType
	FuncType
	CondType
	CaseType
)

type ExprType int

// Some types
type AttrBase struct {
	Name string
}

type ConstBase struct {
	Value string
}

type FuncBase struct {
	Param []ExprBase // assume there must be non-func expression
	Function
}

type CaseBase struct {
	CondExpr []*CondBase
	ThenExpr []ExprBase
	ElseExpr ExprBase
}

type CondBase struct {
	Lexpr ExprBase  `json:"-"`
	Rexpr ExprBase  `json:"-"`
	Comp  *CompBase // compare operator
}

// NoneBase represents there is no left or right input in condition expression, not represents null value
type NoneBase struct {
}

type FieldBase struct {
	Expr  ExprBase
	Alias string
}

type CompBase struct {
	Name   string
	PreVal *regexp.Regexp // precompiled object for regex
}

// Operators
//
//	type ScanOper_ struct {
//		From interface{}
//	}
type PredicateOperator struct {
	Root *CondBase // root node of predicate condition tree
}
type ProjectOperator struct {
	Fields []*FieldBase
}

// select plan
type SelectPlan struct {
	ProjectOper   *ProjectOperator
	PredicateOper *PredicateOperator
	// ScanOper      ScanOper_
}

// I/O type
type LogContentType models.KeyValues[string]

// expression
type ExprBase interface {
	GetType() ExprType
}

func (a *AttrBase) GetType() ExprType {
	return AttrType
}

func (c *ConstBase) GetType() ExprType {
	return ConstType
}

func (f *FuncBase) GetType() ExprType {
	return FuncType
}

func (c *CondBase) GetType() ExprType {
	return CondType
}

func (c *CaseBase) GetType() ExprType {
	return CaseType
}

func (n *NoneBase) GetType() ExprType {
	return NoneType
}

// function
type Function interface {
	Init(param ...string) error
	Process(param ...string) (string, error)
	Name() string
}
