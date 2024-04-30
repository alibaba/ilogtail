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

func (p *ProcessorSQL) predicate(content *LogContentType) (bool, error) {
	if p.Plan.PredicateOper.Root == nil {
		return true, nil
	}
	return p.traverse(p.Plan.PredicateOper.Root, content)
}

func (p *ProcessorSQL) project(in *LogContentType) (*models.LogContents, error) {
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

func (p *ProcessorSQL) traverse(node *CondBase, content *LogContentType) (bool, error) {
	ltype := node.Lexpr.GetType()
	rtype := node.Rexpr.GetType()
	switch ltype {
	case CondType:
		switch rtype {
		case CondType:
			return p.evaluateConditionPairs(node, content)
		default:
			return false, fmt.Errorf("condition node has unexpected children type ltype: %d, rtype: %d", ltype, rtype)
		}
	default:
		switch rtype {
		case CondType:
			return false, fmt.Errorf("condition node has unexpected children type ltype: %d, rtype: %d", ltype, rtype)
		default:
			return p.evaluateCondition(node, content)
		}
	}
}

func (p *ProcessorSQL) evaluateConditionPairs(node *CondBase, content *LogContentType) (bool, error) {
	switch node.Comp.Name {
	case And:
		{
			left, err := p.traverse(node.Lexpr.(*CondBase), content)
			if err != nil {
				return false, err
			}
			if !left {
				return false, nil
			}
			right, err := p.traverse(node.Rexpr.(*CondBase), content)
			if err != nil {
				return false, err
			}
			return right, nil
		}
	case Or:
		{
			left, err := p.traverse(node.Lexpr.(*CondBase), content)
			if err != nil {
				return false, err
			}
			if left {
				return true, nil
			}
			right, err := p.traverse(node.Rexpr.(*CondBase), content)
			if err != nil {
				return false, err
			}
			return right, nil
		}
	default:
		return false, nil
	}
}

func (p *ProcessorSQL) evaluateCondition(node *CondBase, content *LogContentType) (bool, error) {
	// now oly not support "expression" comp "none"
	if _, ok := node.Rexpr.(*NoneBase); ok {
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

func (p *ProcessorSQL) getValue(node ExprBase, content *LogContentType) (string, error) {
	switch node := node.(type) {
	case *AttrBase:
		res := (*content).Get(node.Name)
		if res == "" && p.NoKeyError && !(*content).Contains(node.Name) {
			return res, fmt.Errorf("attribute %s not found", node.Name)
		}
		return res, nil
	case *ConstBase:
		return node.Value, nil
	case *FuncBase:
		return p.evaluteFunc(node, content)
	case *CaseBase:
		return p.evaluateCase(node, content)
	default:
		{
			err := fmt.Errorf("unknown expression type")
			return "", err
		}
	}
}

func (p *ProcessorSQL) evaluteFunc(node ExprBase, content *LogContentType) (string, error) {
	var param []string
	for _, item := range node.(*FuncBase).Param {
		switch item := item.(type) {
		case *AttrBase:
			res := (*content).Get(item.Name)
			if res == "" && p.NoKeyError && !(*content).Contains(item.Name) {
				return res, fmt.Errorf("attribute %s not found", item.Name)
			}
			param = append(param, res)
		case *ConstBase:
			param = append(param, item.Value)
		case *FuncBase:
			{
				res, err := p.evaluteFunc(item, content)
				if err != nil {
					return "", err
				}
				param = append(param, res)
			}
		case *CondBase:
			{
				res, err := p.traverse(item, content)
				if err != nil {
					return "", err
				}
				param = append(param, strconv.FormatBool(res))
			}
		}
	}
	res, err := node.(*FuncBase).Function.Process(param...)
	if err != nil {
		return "", err
	}
	return res, nil
}

func (p *ProcessorSQL) evaluateCase(node *CaseBase, content *LogContentType) (string, error) {
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

func (p *ProcessorSQL) evaluateCompare(left string, right string, comp string, pattern *regexp.Regexp) (bool, error) {
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
