package typechecker

import (
	"fmt"
	"quill/src/core/ast"
)

// ─── Type Checker ─────────────────────────────────────────────────────────────

type TypeChecker struct {
	env                   []map[string]VariableInfo
	functions             map[string]FunctionSignature
	errors                []string
	currentFuncReturnType string
}

type VariableInfo struct {
	Type     string
	Mutable  bool
	Declared bool
}

type FunctionSignature struct {
	ParamTypes []string
	ReturnType string
}

func NewTypeChecker() *TypeChecker {
	tc := &TypeChecker{
		env:       []map[string]VariableInfo{{}},
		functions: make(map[string]FunctionSignature),
	}
	return tc
}

// ─── Scope Management ─────────────────────────────────────────────────────────

func (tc *TypeChecker) pushScope() {
	tc.env = append(tc.env, make(map[string]VariableInfo))
}

func (tc *TypeChecker) popScope() {
	if len(tc.env) > 1 {
		tc.env = tc.env[:len(tc.env)-1]
	}
}

func (tc *TypeChecker) declare(name string, info VariableInfo) {
	tc.env[len(tc.env)-1][name] = info
}

func (tc *TypeChecker) lookup(name string) (VariableInfo, bool) {
	for i := len(tc.env) - 1; i >= 0; i-- {
		if info, ok := tc.env[i][name]; ok {
			return info, true
		}
	}
	return VariableInfo{}, false
}

// ─── Public API ───────────────────────────────────────────────────────────────

func (tc *TypeChecker) Check(program ast.Program) error {
	// FIXED: Reset currentFuncReturnType before checking
	tc.currentFuncReturnType = ""
	for _, node := range program.Body {
		tc.checkNode(node)
	}
	if len(tc.errors) > 0 {
		result := ""
		for _, e := range tc.errors {
			result += e + "\n"
		}
		return fmt.Errorf("%s", result)
	}
	return nil
}

// ─── Core Type Checking ───────────────────────────────────────────────────────

func (tc *TypeChecker) checkNode(node ast.Node) string {
	switch n := node.(type) {

	case ast.Program:
		for _, stmt := range n.Body {
			tc.checkNode(stmt)
		}
		return "void"

	case ast.VariableDeclaration:
		valueType := tc.checkNode(n.Value)

		if n.DeclaredType != "" && n.DeclaredType != valueType {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot assign %s to %s — '%s'",
					valueType, n.DeclaredType, n.Identifier))
		}

		finalType := n.DeclaredType
		if finalType == "" {
			finalType = valueType
		}

		if _, exists := tc.env[len(tc.env)-1][n.Identifier]; exists {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' is already declared in this scope", n.Identifier))
		}

		tc.declare(n.Identifier, VariableInfo{
			Type:     finalType,
			Mutable:  n.Mutable,
			Declared: true,
		})
		return finalType

	case ast.Assignment:
		info, exists := tc.lookup(n.Identifier)
		if !exists {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' is not declared", n.Identifier))
			return "unknown"
		}
		if !info.Mutable {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot assign to constant '%s'", n.Identifier))
		}
		valueType := tc.checkNode(n.Value)
		if info.Type != "" && info.Type != valueType {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot assign %s to %s — '%s'",
					valueType, info.Type, n.Identifier))
		}
		return valueType

	case ast.Function:
		sig := FunctionSignature{ReturnType: n.ReturnType}
		for _, p := range n.Params {
			sig.ParamTypes = append(sig.ParamTypes, p.Type)
		}
		tc.functions[n.Name] = sig

		tc.pushScope()
		savedReturnType := tc.currentFuncReturnType
		tc.currentFuncReturnType = n.ReturnType

		for _, p := range n.Params {
			tc.declare(p.Name, VariableInfo{
				Type:     p.Type,
				Mutable:  true,
				Declared: true,
			})
		}

		for _, stmt := range n.Body {
			tc.checkNode(stmt)
		}

		tc.currentFuncReturnType = savedReturnType
		tc.popScope()
		return "void"

	case ast.ReturnStatement:
		if n.Value == nil {
			if tc.currentFuncReturnType != "" && tc.currentFuncReturnType != "void" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: expected return value of type %s, got void",
						tc.currentFuncReturnType))
			}
			return "void"
		}
		valueType := tc.checkNode(n.Value)
		if tc.currentFuncReturnType != "" && tc.currentFuncReturnType != valueType {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: expected return type %s, got %s",
					tc.currentFuncReturnType, valueType))
		}
		return valueType

	case ast.PrintStatement:
		tc.checkNode(n.Expression)
		return "void"

	case ast.IfStatement:
		condType := tc.checkNode(n.Condition)
		if condType != "bool" && condType != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: condition must be bool, got %s", condType))
		}

		tc.pushScope()
		for _, stmt := range n.Consequent {
			tc.checkNode(stmt)
		}
		tc.popScope()

		if len(n.Alternate) > 0 {
			tc.pushScope()
			for _, stmt := range n.Alternate {
				tc.checkNode(stmt)
			}
			tc.popScope()
		}
		return "void"

	case ast.WhileLoop:
		condType := tc.checkNode(n.Condition)
		if condType != "bool" && condType != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: loop condition must be bool, got %s", condType))
		}

		tc.pushScope()
		for _, stmt := range n.Body {
			tc.checkNode(stmt)
		}
		tc.popScope()
		return "void"

	case ast.BinaryExpression:
		left := tc.checkNode(n.Left)
		right := tc.checkNode(n.Right)

		switch n.Operator {
		case "==", "!=", ">", "<", ">=", "<=":
			if left != right && left != "unknown" && right != "unknown" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: cannot compare %s and %s", left, right))
			}
			return "bool"
		case "&&", "||":
			if left != "bool" && left != "unknown" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: left operand of %s must be bool, got %s", n.Operator, left))
			}
			if right != "bool" && right != "unknown" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: right operand of %s must be bool, got %s", n.Operator, right))
			}
			return "bool"
		}

		// FIXED: Allow string concatenation with mixed types (coercion)
		if n.Operator == "+" && (left == "str" || right == "str") {
			return "str"
		}

		if left != right && left != "unknown" && right != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot use '%s' on %s and %s", n.Operator, left, right))
		}

		if left != "int" && left != "float" && left != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: operator '%s' requires numeric types, got %s", n.Operator, left))
		}
		return left

	case ast.UnaryExpression:
		operandType := tc.checkNode(n.Operand)
		switch n.Operator {
		case "-":
			if operandType != "int" && operandType != "float" && operandType != "unknown" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: unary '-' requires numeric type, got %s", operandType))
			}
			return operandType
		case "!":
			if operandType != "bool" && operandType != "unknown" {
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: unary '!' requires bool, got %s", operandType))
			}
			return "bool"
		}
		return "unknown"

	case ast.CallExpression:
		sig, exists := tc.functions[n.Callee]
		if !exists {
			switch n.Callee {
			case "len":
				if len(n.Arguments) != 1 {
					tc.errors = append(tc.errors, "TypeError: 'len' expects 1 argument")
				}
				return "int"
			case "toString":
				if len(n.Arguments) != 1 {
					tc.errors = append(tc.errors, "TypeError: 'toString' expects 1 argument")
				}
				return "str"
			default:
				tc.errors = append(tc.errors,
					fmt.Sprintf("TypeError: unknown function '%s'", n.Callee))
				return "unknown"
			}
		}

		if len(n.Arguments) != len(sig.ParamTypes) {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' expects %d arguments but got %d",
					n.Callee, len(sig.ParamTypes), len(n.Arguments)))
		} else {
			for i, arg := range n.Arguments {
				argType := tc.checkNode(arg)
				if sig.ParamTypes[i] != "" && argType != sig.ParamTypes[i] && argType != "unknown" {
					tc.errors = append(tc.errors,
						fmt.Sprintf("TypeError: argument %d of '%s' expects %s but got %s",
							i+1, n.Callee, sig.ParamTypes[i], argType))
				}
			}
		}
		return sig.ReturnType

	case ast.IndexExpression:
		objectType := tc.checkNode(n.Object)
		indexType := tc.checkNode(n.Index)

		if indexType != "int" && indexType != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: index must be int, got %s", indexType))
		}

		if objectType == "str" {
			return "str"
		}
		return "unknown"

	case ast.Identifier:
		info, exists := tc.lookup(n.Name)
		if !exists {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' is not declared", n.Name))
			return "unknown"
		}
		return info.Type

	case ast.Increment:
		info, exists := tc.lookup(n.Identifier)
		if !exists {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' is not declared", n.Identifier))
			return "int"
		}
		if info.Type != "int" && info.Type != "float" && info.Type != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot increment %s '%s'", info.Type, n.Identifier))
		}
		if !info.Mutable {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot increment constant '%s'", n.Identifier))
		}
		return info.Type

	case ast.Decrement:
		info, exists := tc.lookup(n.Identifier)
		if !exists {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: '%s' is not declared", n.Identifier))
			return "int"
		}
		if info.Type != "int" && info.Type != "float" && info.Type != "unknown" {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot decrement %s '%s'", info.Type, n.Identifier))
		}
		if !info.Mutable {
			tc.errors = append(tc.errors,
				fmt.Sprintf("TypeError: cannot decrement constant '%s'", n.Identifier))
		}
		return info.Type

	case ast.ExpressionStatement:
		return tc.checkNode(n.Expression)

	case ast.LiteralInt:
		return "int"
	case ast.LiteralFloat:
		return "float"
	case ast.LiteralString:
		return "str"
	case ast.LiteralBool:
		return "bool"
	case ast.Comment:
		return "void"
	}

	tc.errors = append(tc.errors,
		fmt.Sprintf("TypeError: unknown AST node type %T", node))
	return "unknown"
}
