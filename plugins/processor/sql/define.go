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
type Attr_ struct {
	Name string
}

type Const_ struct {
	Value string
}

type Func_ struct {
	Param []Expr_ // assume there must be non-func expresions
	Function
}

type Case_ struct {
	CondExpr []*Cond_
	ThenExpr []Expr_
	ElseExpr Expr_
}

type Cond_ struct {
	Lexpr Expr_  `json:"-"`
	Rexpr Expr_  `json:"-"`
	Comp  *Comp_ // compare operator
}

// None_ represents there is no left or right input in condition expression, not represents null value
type None_ struct {
}

type Field_ struct {
	Expr  Expr_
	Alias string
}

type Comp_ struct {
	Name   string
	PreVal *regexp.Regexp // precompiled object for regex
}

// Operators
//
//	type ScanOper_ struct {
//		From interface{}
//	}
type PredicateOper_ struct {
	Root *Cond_ // root node of predicate condition tree
}
type ProjectOper_ struct {
	Fields []*Field_
}

// select plan
type SelectPlan struct {
	ProjectOper   *ProjectOper_
	PredicateOper *PredicateOper_
	// ScanOper      ScanOper_
}

// I/O type
type LogContentType models.KeyValues[string]

// expression
type Expr_ interface {
	GetType() ExprType
}

func (a *Attr_) GetType() ExprType {
	return AttrType
}

func (c *Const_) GetType() ExprType {
	return ConstType
}

func (f *Func_) GetType() ExprType {
	return FuncType
}

func (c *Cond_) GetType() ExprType {
	return CondType
}

func (c *Case_) GetType() ExprType {
	return CaseType
}

func (n *None_) GetType() ExprType {
	return NoneType
}

// function
type Function interface {
	Init(param ...string) error
	Process(param ...string) (string, error)
	Name() string
}
