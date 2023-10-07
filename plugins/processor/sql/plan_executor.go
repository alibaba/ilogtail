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
	"fmt"
	"regexp"
	"strconv"

	"github.com/alibaba/ilogtail/pkg/models"
)

func (p *ProcessorSql) predicate(content *LogContentType) (bool, error) {
	if p.Plan.PredicateOper.Root == nil {
		return true, nil
	}
	return p.traverse(p.Plan.PredicateOper.Root, content)
}

func (p *ProcessorSql) project(in *LogContentType) (*models.LogContents, error) {
	out := models.NewLogContents()
	for _, item := range p.Plan.ProjectOper.Fields {
		res, err := p.getValue(item.Expr, in)
		if err != nil {
			return nil, err
		}
		out.Add(item.Alias, res)
	}
	return &out, nil
}

func (p *ProcessorSql) traverse(node *Cond_, content *LogContentType) (bool, error) {
	ltype := node.Lexpr.GetType()
	rtype := node.Rexpr.GetType()
	if ltype == CondType {
		if rtype == CondType {
			return p.evaluateConditionPairs(node, content)
		} else {
			return false, fmt.Errorf("condition node has unexpected children type ltype: %d, rtype: %d", ltype, rtype)
		}
	} else if rtype == CondType {
		return false, fmt.Errorf("condition node has unexpected children type ltype: %d, rtype: %d", ltype, rtype)
	} else {
		return p.evaluateCondition(node, content)
	}
}

func (p *ProcessorSql) evaluateConditionPairs(node *Cond_, content *LogContentType) (bool, error) {
	switch node.Comp.Name {
	case And:
		{
			left, err := p.traverse(node.Lexpr.(*Cond_), content)
			if err != nil {
				return false, err
			}
			if !left {
				return false, nil
			}
			right, err := p.traverse(node.Rexpr.(*Cond_), content)
			if err != nil {
				return false, err
			}
			return right, nil
		}
	case Or:
		{
			left, err := p.traverse(node.Lexpr.(*Cond_), content)
			if err != nil {
				return false, err
			}
			if left {
				return true, nil
			}
			right, err := p.traverse(node.Rexpr.(*Cond_), content)
			if err != nil {
				return false, err
			}
			return right, nil
		}
	default:
		return false, nil
	}
}

func (p *ProcessorSql) evaluateCondition(node *Cond_, content *LogContentType) (bool, error) {
	// now oly not support "expression" comp "none"
	if _, ok := node.Rexpr.(*None_); ok {
		left, err := p.getValue(node.Lexpr, content)
		if err != nil {
			return false, err
		}
		isZeroNumber := isZeroNumber(left)
		if node.Comp.Name == Eq {
			return isZeroNumber, nil
		} else if node.Comp.Name == Neq {
			return !isZeroNumber, nil
		}
	}

	left, err := p.getValue(node.Lexpr, content)
	if err != nil {
		return false, err
	}
	right, err := p.getValue(node.Rexpr, content)
	if err != nil {
		return false, err
	}
	return p.evaluateCompare(left, right, node.Comp.Name, node.Comp.PreVal)
}

func (p *ProcessorSql) getValue(node Expr_, content *LogContentType) (string, error) {
	switch node := node.(type) {
	case *Attr_:
		res := (*content).Get(node.Name)
		if res == "" && p.NoKeyError && !(*content).Contains(node.Name) {
			return res, fmt.Errorf("attribute %s not found", node.Name)
		}
		return res, nil
	case *Const_:
		return node.Value, nil
	case *Func_:
		return p.evaluteFunc(node, content)
	case *Case_:
		return p.evaluateCase(node, content)
	default:
		{
			err := fmt.Errorf("unknown expression type")
			return "", err
		}
	}
}

func (p *ProcessorSql) evaluteFunc(node Expr_, content *LogContentType) (string, error) {
	var param []string
	for _, item := range node.(*Func_).Param {
		switch item := item.(type) {
		case *Attr_:
			res := (*content).Get(item.Name)
			if res == "" && p.NoKeyError && !(*content).Contains(item.Name) {
				return res, fmt.Errorf("attribute %s not found", item.Name)
			}
			param = append(param, res)
		case *Const_:
			param = append(param, item.Value)
		case *Func_:
			{
				res, err := p.evaluteFunc(item, content)
				if err != nil {
					return "", err
				}
				param = append(param, res)
			}
		case *Cond_:
			{
				res, err := p.traverse(item, content)
				if err != nil {
					return "", err
				}
				param = append(param, strconv.FormatBool(res))
			}
		}
	}
	res, err := node.(*Func_).Function.Process(param...)
	if err != nil {
		return "", err
	}
	return res, nil
}

func (p *ProcessorSql) evaluateCase(node *Case_, content *LogContentType) (string, error) {
	for idx, cond := range node.CondExpr {
		res, err := p.evaluateCondition(cond, content)
		if err != nil {
			return "", err
		}
		if res {
			then := node.ThenExpr[idx]
			return p.getValue(
				then,
				content,
			)
		}
	}
	return p.getValue(
		node.ElseExpr,
		content,
	)
}

func (p *ProcessorSql) evaluateCompare(left string, right string, comp string, pattern *regexp.Regexp) (bool, error) {
	switch comp {
	case Eq:
		{
			return left == right, nil
		}
	case Neq:
		{
			return left != right, nil
		}
	case Gt:
		{
			l, err := strconv.ParseFloat(left, 64)
			if err != nil {
				return false, fmt.Errorf("now only support pure number compare in >")
			}
			r, err := strconv.ParseFloat(right, 64)
			if err != nil {
				return false, fmt.Errorf("now only support pure number compare in >")
			}
			return l > r, nil
		}
	case Lt:
		{
			l, err := strconv.ParseFloat(left, 64)
			if err != nil {
				return false, fmt.Errorf("now only support pure number compare in <")
			}
			r, err := strconv.ParseFloat(right, 64)
			if err != nil {
				return false, fmt.Errorf("now only support pure number compare in <")
			}
			return l < r, nil
		}
	case Regex:
		{
			return evaluateRegex(left, right, pattern)
		}
	case Like:
		{
			return evaluateLike(left, right, pattern)
		}
	case Nregex:
		{
			res, err := evaluateRegex(left, right, pattern)
			if err != nil {
				return false, err
			}
			return !res, nil
		}
	case Nlike:
		{
			res, err := evaluateLike(left, right, pattern)
			if err != nil {
				return false, err
			}
			return !res, nil
		}
	default:
		return false, nil
	}
}
