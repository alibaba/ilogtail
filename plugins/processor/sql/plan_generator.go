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
	"strings"

	"github.com/xwb1989/sqlparser"
)

/** functions for generating select plan**/

// function: generate project operator
func (p *ProcessorSQL) genProjectOper(stmt *sqlparser.Select) error {
	p.Plan.ProjectOper = &ProjectOperator{}
	for _, field := range stmt.SelectExprs {
		switch stmtOut := field.(type) {
		// select non star field
		case *sqlparser.AliasedExpr:
			{
				if stmt, ok := stmtOut.Expr.(*sqlparser.CaseExpr); ok {
					caseExpr := CaseBase{}
					// select CASE attibute WHEN expression THEN expression ELSE expression END;
					if stmt.Expr != nil {
						for _, stmtIn := range stmt.Whens {
							condition := CondBase{}
							// left
							condition.Lexpr = &AttrBase{Name: stmt.Expr.(*sqlparser.ColName).Name.String()}
							// right
							res, err := p.genNonCondExpr(stmtIn.Cond)
							if err != nil {
								return err
							}
							condition.Rexpr = res
							// operator
							condition.Comp = &CompBase{Name: Eq}
							caseExpr.CondExpr = append(caseExpr.CondExpr, &condition)
							res, err = p.genNonCondExpr(stmtIn.Val)
							if err != nil {
								return err
							}
							caseExpr.ThenExpr = append(caseExpr.ThenExpr, res)

						}
						if stmt.Else != nil {
							res, err := p.genNonCondExpr(stmt.Else)
							if err != nil {
								return err
							}
							caseExpr.ElseExpr = res
						}
						// select CASE WHEN condition THEN expression ELSE expression END;
					} else {
						for _, stmtIn := range stmt.Whens {
							var condExpr ExprBase
							var err error
							condExpr, err = p.genExpr(stmtIn.Cond)
							if err != nil {
								return err
							}
							if condExpr.GetType() != CondType {
								return fmt.Errorf("not support: select CASE WHEN non-condition!! THEN expression ELSE expression END;")
							}
							caseExpr.CondExpr = append(caseExpr.CondExpr, condExpr.(*CondBase))
							caseExpr.ThenExpr = append(caseExpr.ThenExpr, &ConstBase{Value: string(stmtIn.Val.(*sqlparser.SQLVal).Val)})
						}
						if stmt.Else != nil {
							caseExpr.ElseExpr = &ConstBase{Value: string(stmt.Else.(*sqlparser.SQLVal).Val)}
						}
					}
					p.Plan.ProjectOper.Fields = append(p.Plan.ProjectOper.Fields, &FieldBase{Expr: &caseExpr, Alias: stmtOut.As.String()})

				} else {
					expr, err := p.genNonCondExpr(stmtOut.Expr)
					if err != nil {
						return err
					}
					name := stmtOut.As.String()
					if name == "" {
						name = p.getName(expr)
					}
					p.Plan.ProjectOper.Fields = append(p.Plan.ProjectOper.Fields, &FieldBase{Expr: expr, Alias: name})
				}

			}
		// to do: select star field
		// case *sqlparser.StarExpr:
		// 	{
		// 	}
		default:
			return fmt.Errorf("unsupported expression type: %T", stmtOut)
		}
	}
	return nil
}

// function: generate predicate operator
func (p *ProcessorSQL) genPredicateOper(stmt *sqlparser.Select) error {
	p.Plan.PredicateOper = &PredicateOperator{}
	if stmt.Where == nil {
		return nil
	}
	root, err := p.genExpr(stmt.Where.Expr)
	if err != nil {
		return err
	}
	if root.GetType() != CondType {
		return fmt.Errorf("error in genExpr: select expression where non-condition")
	}
	p.Plan.PredicateOper.Root = root.(*CondBase)
	return nil
}

// function: generate expression with condition
func (p *ProcessorSQL) genExpr(expr sqlparser.Expr) (ExprBase, error) {
	switch expr := expr.(type) {
	// non-leaf node
	case *sqlparser.ComparisonExpr:
		{
			condition := CondBase{}
			// left
			left, err := p.genNonCondExpr(expr.Left)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = left
			// right
			right, err := p.genNonCondExpr(expr.Right)
			if err != nil {
				return nil, err
			}
			condition.Rexpr = right
			// operator
			condition.Comp = &CompBase{Name: strings.ToUpper(expr.Operator)}
			if expr.Operator == sqlparser.RegexpStr || expr.Operator == sqlparser.NotRegexpStr {
				if right.GetType() == ConstType {
					condition.Comp.PreVal, err = preEvaluateRegex(right.(*ConstBase).Value)
					if err != nil {
						return nil, err
					}
				}
			} else if expr.Operator == sqlparser.LikeStr || expr.Operator == sqlparser.NotLikeStr {
				if right.GetType() == ConstType {
					condition.Comp.PreVal, err = preEvaluateLike(right.(*ConstBase).Value)
					if err != nil {
						return nil, err
					}
				}
			}
			return &condition, nil
		}
	case *sqlparser.AndExpr:
		{
			condition := CondBase{}
			// left
			left, err := p.genExpr(expr.Left)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = left
			// right
			right, err := p.genExpr(expr.Right)
			if err != nil {
				return nil, err
			}
			condition.Rexpr = right
			// operator
			condition.Comp = &CompBase{Name: And}
			return &condition, nil
		}
	case *sqlparser.OrExpr:
		{
			condition := CondBase{}
			// left
			left, err := p.genExpr(expr.Left)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = left
			// right
			right, err := p.genExpr(expr.Right)
			if err != nil {
				return nil, err
			}
			condition.Rexpr = right
			// operator
			condition.Comp = &CompBase{Name: Or}
			return &condition, nil
		}
	// e.g.:select name from users where id;  select name from users where md5(id);  select name from users where 1;
	case *sqlparser.ColName, *sqlparser.SQLVal, *sqlparser.FuncExpr, *sqlparser.NullVal:
		{
			condition := CondBase{}
			expr, err := p.genNonCondExpr(expr)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = expr
			condition.Rexpr = &NoneBase{}
			condition.Comp = &CompBase{Name: Neq}
			return &condition, nil
		}
	// e.g.:select name from users where !id;   select name from users where not id;
	// to do: not support select name from users where not !id;
	case *sqlparser.UnaryExpr:
		{
			condition := CondBase{}
			expr, err := p.genNonCondExpr(expr.Expr)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = expr
			condition.Rexpr = &NoneBase{}
			condition.Comp = &CompBase{Name: Eq}
			return &condition, nil
		}
	case *sqlparser.NotExpr:
		{
			condition := CondBase{}
			expr, err := p.genNonCondExpr(expr.Expr)
			if err != nil {
				return nil, err
			}
			condition.Lexpr = expr
			condition.Rexpr = &NoneBase{}
			condition.Comp = &CompBase{Name: Eq}
			return &condition, nil
		}
	case *sqlparser.ParenExpr:
		{
			return p.genExpr(expr.Expr)
		}
	default:
		return nil, fmt.Errorf("unsupported expression type: %T", expr)
	}
}

// function: generate expression except condition
func (p *ProcessorSQL) genNonCondExpr(leafNode sqlparser.Expr) (ExprBase, error) {
	switch leafNode := leafNode.(type) {
	// ordinary column
	case *sqlparser.ColName:
		{
			expr := AttrBase{}
			expr.Name = leafNode.Name.String()
			return &expr, nil
		}
	case *sqlparser.SQLVal:
		{
			expr := ConstBase{}
			expr.Value = string(leafNode.Val)
			return &expr, nil
		}
	// scalar function
	case *sqlparser.FuncExpr:
		{
			expr := FuncBase{}
			for _, stmt := range leafNode.Exprs {
				switch stmt := stmt.(*sqlparser.AliasedExpr).Expr.(type) {
				case *sqlparser.ColName:
					expr.Param = append(expr.Param, &AttrBase{
						Name: stmt.Name.String(),
					})
				case *sqlparser.SQLVal:
					expr.Param = append(expr.Param, &ConstBase{
						Value: string(stmt.Val),
					})
				case *sqlparser.FuncExpr:
					{
						res, err := p.genNonCondExpr(stmt)
						if err != nil {
							return nil, err
						}
						expr.Param = append(expr.Param, res)
					}
				case *sqlparser.ComparisonExpr:
					{
						condition := CondBase{}
						// left
						left, err := p.genNonCondExpr(stmt.Left)
						if err != nil {
							return nil, err
						}
						condition.Lexpr = left
						// right
						right, err := p.genNonCondExpr(stmt.Right)
						if err != nil {
							return nil, err
						}
						condition.Rexpr = right
						// operator
						condition.Comp = &CompBase{Name: strings.ToUpper(stmt.Operator)} // to do: check operator
						if stmt.Operator == sqlparser.RegexpStr || stmt.Operator == sqlparser.NotRegexpStr {
							if right.GetType() == ConstType {
								condition.Comp.PreVal, err = preEvaluateRegex(right.(*ConstBase).Value)
								if err != nil {
									return nil, err
								}
							}
						} else if stmt.Operator == sqlparser.LikeStr || stmt.Operator == sqlparser.NotLikeStr {
							if right.GetType() == ConstType {
								condition.Comp.PreVal, err = preEvaluateLike(right.(*ConstBase).Value)
								if err != nil {
									return nil, err
								}
							}
						}
						expr.Param = append(expr.Param, &condition)
					}
				case *sqlparser.NullVal:
					expr.Param = append(expr.Param, &ConstBase{
						Value: "NULL",
					})
				}
			}
			functionName := strings.ToLower(leafNode.Name.String())
			var res Function
			creator, ok := FunctionMap[functionName]
			if !ok || creator == nil {
				return nil, fmt.Errorf("unsupported function: %s", functionName)
			}
			res = creator()
			var param []string
			for _, item := range expr.Param {
				switch item := item.(type) {
				case *AttrBase:
					param = append(param, item.Name)
				case *ConstBase:
					param = append(param, item.Value)
				case *FuncBase:
					param = append(param, p.getName(item))
				case *CondBase:
					param = append(param, p.getName(item))
				}
			}
			err := res.Init(param...)
			expr.Function = res
			if err != nil {
				return nil, err
			}
			return &expr, nil
		}
	case *sqlparser.NullVal:
		{
			expr := ConstBase{}
			expr.Value = "NULL"
			return &expr, nil
		}
	default:
		return nil, fmt.Errorf("unsupported expression type: %T", leafNode)
	}
}

func (p *ProcessorSQL) getName(expr ExprBase) string {
	switch expr := expr.(type) {
	case *AttrBase:
		return expr.Name
	case *FuncBase:
		name := expr.Function.Name()
		name += "("
		for _, item := range expr.Param {
			switch item := item.(type) {
			case *AttrBase:
				name += item.Name
			case *ConstBase:
				name += item.Value
			case *FuncBase:
				name += p.getName(item)
			}
		}
		name += ")"
		return name
	case *ConstBase:
		return expr.Value
	case *NoneBase:
		return ""
	case *CondBase:
		return p.getName(expr.Lexpr) + " " + expr.Comp.Name + " " + p.getName(expr.Rexpr)
	default:
		return "" // to do:case
	}
}
