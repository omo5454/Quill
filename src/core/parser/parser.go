package parser

import (
	"fmt"
	"quill/src/core/ast"
	"quill/src/core/types"
)

// ─── Parser ──────────────────────────────────────────────────────────────────

type Parser struct {
	tokens  []types.Token
	current int
}

func NewParser(tokens []types.Token) *Parser {
	// Strip semicolons — purely decorative
	var filtered []types.Token
	for _, t := range tokens {
		if t.Value != ";" {
			filtered = append(filtered, t)
		}
	}
	filtered = append(filtered, types.Token{Type: types.EOF, Value: ""})
	return &Parser{tokens: filtered, current: 0}
}

// ─── Position / Error Helpers ──────────────────────────────────────────────

func (p *Parser) pos() int {
	return p.current
}

func (p *Parser) peek() types.Token {
	if p.current >= len(p.tokens) {
		return types.Token{Type: types.EOF, Value: ""}
	}
	return p.tokens[p.current]
}

func (p *Parser) advance() types.Token {
	tok := p.tokens[p.current]
	p.current++
	return tok
}

func (p *Parser) isAtEnd() bool {
	return p.peek().Type == types.EOF
}

func (p *Parser) errf(format string, args ...interface{}) error {
	return fmt.Errorf("token %d: %s", p.pos(), fmt.Sprintf(format, args...))
}

func (p *Parser) consume(expected string) (types.Token, error) {
	if p.peek().Value != expected {
		return types.Token{}, p.errf("expected '%s', got '%s'", expected, p.peek().Value)
	}
	return p.advance(), nil
}

func (p *Parser) peekIs(value string) bool {
	return p.peek().Value == value
}

func (p *Parser) peekTypeIs(t types.TokenType) bool {
	return p.peek().Type == t
}

// ─── Expression Boundary ───────────────────────────────────────────────────

func (p *Parser) canContinueExpression() bool {
	if p.isAtEnd() {
		return false
	}
	switch p.peek().Value {
	case "}", ")", ",", "{", "]", "else", "elif":
		return false
	}
	if p.peek().Type == types.EOF {
		return false
	}
	return true
}

// ─── Program ─────────────────────────────────────────────────────────────────

func (p *Parser) Parse() (ast.Program, error) {
	program := ast.Program{}
	for !p.isAtEnd() {
		if p.peekTypeIs(types.Comment) {
			p.advance()
			continue
		}
		stmt, err := p.parseStatement()
		if err != nil {
			return ast.Program{}, err
		}
		program.Body = append(program.Body, stmt)
	}
	return program, nil
}

// ─── Statements ──────────────────────────────────────────────────────────────

func (p *Parser) parseStatement() (ast.Node, error) {
	tok := p.peek()

	if tok.Type == types.Keyword {
		switch tok.Value {
		case "let", "const":
			return p.parseVariableDeclaration()
		case "printf", "say":
			return p.parsePrintStatement()
		case "func":
			return p.parseFunctionDeclaration()
		case "if", "elif", "else":
			return p.parseConditionalExpression()
		case "while":
			return p.parseLoopExpression()
		case "return":
			return p.parseReturnStatement()
		case "true", "false":
			return p.parseExpressionStatement()
		}
	}

	switch tok.Type {
	case types.Identifier:
		if p.isAssignmentStart() {
			return p.parseAssignment()
		}
		return p.parseExpressionStatement()
	case types.Str, types.Float, types.Number:
		return p.parseExpressionStatement()
	case types.Comment:
		return p.parseComment()
	}

	if tok.Value == "(" {
		return p.parseExpressionStatement()
	}

	return nil, p.errf("unexpected token '%s' (%s) at statement level", tok.Value, tok.Type)
}

func (p *Parser) isAssignmentStart() bool {
	if p.current+1 >= len(p.tokens) {
		return false
	}
	next := p.tokens[p.current+1].Value
	return next == "=" || next == "+=" || next == "-=" || next == "*=" || next == "/=" || next == "%="
}

func (p *Parser) parseExpressionStatement() (ast.Node, error) {
	expr, err := p.parseExpression()
	if err != nil {
		return nil, err
	}
	return ast.ExpressionStatement{Expression: expr}, nil
}

// ─── Variable Declaration ────────────────────────────────────────────────────

func (p *Parser) parseVariableDeclaration() (ast.Node, error) {
	isConst := p.peek().Value == "const"
	p.advance()

	idToken := p.advance()
	if idToken.Type != types.Identifier {
		return nil, p.errf("expected variable name after 'let'/'const', got '%s'", idToken.Value)
	}

	declaredType := ""
	if p.peekIs(":") {
		p.advance()
		typeTok := p.advance()
		declaredType = typeTok.Value
	}

	if _, err := p.consume("="); err != nil {
		return nil, p.errf("expected '=' after variable name, got '%s'", p.peek().Value)
	}

	val, err := p.parseExpression()
	if err != nil {
		return nil, err
	}

	return ast.VariableDeclaration{
		Identifier:     idToken.Value,
		DeclaredType: declaredType,
		Value:          val,
		Mutable:        !isConst, // FIXED: const = not mutable, let = mutable
	}, nil
}

// ─── Assignment ──────────────────────────────────────────────────────────────

func (p *Parser) parseAssignment() (ast.Node, error) {
	idToken := p.advance()
	if idToken.Type != types.Identifier {
		return nil, p.errf("expected identifier, got '%s'", idToken.Value)
	}

	compoundOps := map[string]string{
		"+=": "+",
		"-=": "-",
		"*=": "*",
		"/=": "/",
		"%=": "%",
	}

	if op, ok := compoundOps[p.peek().Value]; ok {
		p.advance()
		val, err := p.parseExpression()
		if err != nil {
			return nil, err
		}
		return ast.Assignment{
			Identifier: idToken.Value,
			Value: ast.BinaryExpression{
				Left:     ast.Identifier{Name: idToken.Value},
				Operator: op,
				Right:    val,
			},
		}, nil
	}

	if _, err := p.consume("="); err != nil {
		return nil, p.errf("expected '=' after identifier '%s', got '%s'", idToken.Value, p.peek().Value)
	}

	val, err := p.parseExpression()
	if err != nil {
		return nil, err
	}

	return ast.Assignment{
		Identifier: idToken.Value,
		Value:      val,
	}, nil
}

// ─── Print Statement ───────────────────────────────────────────────────────────

func (p *Parser) parsePrintStatement() (ast.Node, error) {
	p.advance()
	val, err := p.parseExpression()
	if err != nil {
		return nil, err
	}
	return ast.PrintStatement{Expression: val}, nil
}

// ─── Return Statement ──────────────────────────────────────────────────────────

func (p *Parser) parseReturnStatement() (ast.Node, error) {
	p.advance()
	if p.peekIs("}") || p.isAtEnd() {
		return ast.ReturnStatement{Value: nil}, nil
	}
	val, err := p.parseExpression()
	if err != nil {
		return nil, err
	}
	return ast.ReturnStatement{Value: val}, nil
}

// ─── Comment ─────────────────────────────────────────────────────────────────

func (p *Parser) parseComment() (ast.Node, error) {
	tok := p.advance()
	return ast.Comment{Value: tok.Value}, nil
}

// ─── Function Declaration ────────────────────────────────────────────────────

func (p *Parser) parseFunctionDeclaration() (ast.Node, error) {
	p.advance()

	nameTok := p.advance()
	if nameTok.Type != types.Identifier {
		return nil, p.errf("expected function name after 'func', got '%s'", nameTok.Value)
	}

	if _, err := p.consume("("); err != nil {
		return nil, err
	}

	var params []ast.Param
	for !p.isAtEnd() && !p.peekIs(")") {
		paramTok := p.advance()
		if paramTok.Type != types.Identifier {
			return nil, p.errf("expected parameter name, got '%s'", paramTok.Value)
		}
		paramType := ""
		if p.peekIs(":") {
			p.advance()
			paramType = p.advance().Value
		}
		params = append(params, ast.Param{Name: paramTok.Value, Type: paramType})
		if p.peekIs(",") {
			p.advance()
		}
	}

	if _, err := p.consume(")"); err != nil {
		return nil, err
	}

	returnType := ""
	if p.peekIs(":") {
		p.advance()
		returnType = p.advance().Value
	}

	if _, err := p.consume("{"); err != nil {
		return nil, err
	}

	var body []ast.Node
	for !p.isAtEnd() && !p.peekIs("}") {
		stmt, err := p.parseStatement()
		if err != nil {
			return nil, err
		}
		body = append(body, stmt)
	}

	if _, err := p.consume("}"); err != nil {
		return nil, err
	}

	return ast.Function{
		Name:       nameTok.Value,
		Params:     params,
		ReturnType: returnType,
		Body:       body,
	}, nil
}

// ─── Loop Expression ───────────────────────────────────────────────────────────

func (p *Parser) parseLoopExpression() (ast.Node, error) {
	p.advance()

	test, err := p.parseExpression()
	if err != nil {
		return nil, err
	}

	if _, err := p.consume("{"); err != nil {
		return nil, err
	}

	var body []ast.Node
	for !p.isAtEnd() && !p.peekIs("}") {
		stmt, err := p.parseStatement()
		if err != nil {
			return nil, err
		}
		body = append(body, stmt)
	}

	if _, err := p.consume("}"); err != nil {
		return nil, err
	}

	return ast.WhileLoop{Condition: test, Body: body}, nil
}

// ─── Conditional Expression ────────────────────────────────────────────────────

func (p *Parser) parseConditionalExpression() (ast.Node, error) {
	p.advance()

	test, err := p.parseExpression()
	if err != nil {
		return nil, err
	}

	if _, err := p.consume("{"); err != nil {
		return nil, err
	}

	var consequent []ast.Node
	for !p.isAtEnd() && !p.peekIs("}") {
		stmt, err := p.parseStatement()
		if err != nil {
			return nil, err
		}
		consequent = append(consequent, stmt)
	}

	if _, err := p.consume("}"); err != nil {
		return nil, err
	}

	var alternate []ast.Node
	if p.peekIs("else") {
		p.advance()
		if p.peekIs("{") {
			// else { ... }
			p.advance()
			for !p.isAtEnd() && !p.peekIs("}") {
				stmt, err := p.parseStatement()
				if err != nil {
					return nil, err
				}
				alternate = append(alternate, stmt)
			}
			if _, err := p.consume("}"); err != nil {
				return nil, err
			}
		} else if p.peekIs("if") || p.peekIs("elif") {
			// else if / else elif
			nested, err := p.parseConditionalExpression()
			if err != nil {
				return nil, err
			}
			alternate = []ast.Node{nested}
		}
	} else if p.peekIs("elif") {
		// elif without preceding else
		nested, err := p.parseConditionalExpression()
		if err != nil {
			return nil, err
		}
		alternate = []ast.Node{nested}
	}

	return ast.IfStatement{
		Condition:  test,
		Consequent: consequent,
		Alternate:  alternate,
	}, nil
}

// ─── Expressions ───────────────────────────────────────────────────────────────

var binaryOps = map[string]bool{
	"+": true, "-": true, "*": true, "/": true,
	"==": true, "!=": true, ">": true, "<": true,
	">=": true, "<=": true, "&&": true, "||": true,
}

func (p *Parser) isBinaryOperator(tok types.Token) bool {
	return tok.Type == types.Operator && binaryOps[tok.Value]
}

func (p *Parser) parseExpression() (ast.Node, error) {
	return p.parsePrecedence(0)
}

var precedence = map[string]int{
	"||": 1,
	"&&": 2,
	"==": 3, "!=": 3,
	"<": 4, ">": 4, "<=": 4, ">=": 4,
	"+": 5, "-": 5,
	"*": 6, "/": 6, "%": 6,
}

func (p *Parser) parsePrecedence(minPrec int) (ast.Node, error) {
	left, err := p.parsePrimary()
	if err != nil {
		return nil, err
	}

	// Postfix
	for p.canContinueExpression() {
		tok := p.peek()
		if tok.Value == "[" {
			p.advance()
			index, err := p.parseExpression()
			if err != nil {
				return nil, err
			}
			if _, err := p.consume("]"); err != nil {
				return nil, err
			}
			left = ast.IndexExpression{Object: left, Index: index}
		} else if tok.Type == types.Incrementation {
			op := p.advance().Value
			name := ""
			switch n := left.(type) {
			case ast.Identifier:
				name = n.Name
			}
			if op == "++" {
				left = ast.Increment{Identifier: name}
			} else {
				left = ast.Decrement{Identifier: name}
			}
		} else {
			break
		}
	}

	// Binary with precedence climbing
	for p.canContinueExpression() {
		tok := p.peek()
		prec, ok := precedence[tok.Value]
		if !ok || prec < minPrec {
			break
		}
		if !p.isBinaryOperator(tok) {
			break
		}
		op := p.advance().Value
		right, err := p.parsePrecedence(prec + 1)
		if err != nil {
			return nil, err
		}
		left = ast.BinaryExpression{Left: left, Operator: op, Right: right}
	}

	return left, nil
}

func (p *Parser) parsePrimary() (ast.Node, error) {
	tok := p.peek()

	// Unary operators — ONLY when token is actually an Operator
	if tok.Type == types.Operator && (tok.Value == "!" || tok.Value == "-") {
		op := p.advance().Value
		operand, err := p.parsePrimary()
		if err != nil {
			return nil, err
		}
		return ast.UnaryExpression{Operator: op, Operand: operand}, nil
	}

	// Parenthesized expression
	if tok.Value == "(" {
		p.advance()
		expr, err := p.parseExpression()
		if err != nil {
			return nil, err
		}
		if _, err := p.consume(")"); err != nil {
			return nil, err
		}
		return expr, nil
	}

	consumed := p.advance()

	switch consumed.Type {
	case types.Number:
		var val int64
		fmt.Sscanf(consumed.Value, "%d", &val)
		return ast.LiteralInt{Value: val}, nil

	case types.Float:
		var val float64
		fmt.Sscanf(consumed.Value, "%f", &val)
		return ast.LiteralFloat{Value: val}, nil

	case types.Str:
		return ast.LiteralString{Value: consumed.Value}, nil

	case types.Keyword:
		switch consumed.Value {
		case "true":
			return ast.LiteralBool{Value: true}, nil
		case "false":
			return ast.LiteralBool{Value: false}, nil
		}

	case types.Identifier:
		// Function call
		if p.peekIs("(") {
			p.advance()
			var args []ast.Node
			for !p.isAtEnd() && !p.peekIs(")") {
				arg, err := p.parseExpression()
				if err != nil {
					return nil, err
				}
				args = append(args, arg)
				if p.peekIs(",") {
					p.advance()
				}
			}
			if _, err := p.consume(")"); err != nil {
				return nil, err
			}
			return ast.CallExpression{Callee: consumed.Value, Arguments: args}, nil
		}
		return ast.Identifier{Name: consumed.Value}, nil
	}

	return nil, p.errf("expected expression, got '%s'", consumed.Value)
}
