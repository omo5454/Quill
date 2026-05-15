package compiler

import (
	"fmt"
	"quill/src/core/ast"
	"quill/src/core/bytecode"
)

// ─── Compiler ────────────────────────────────────────────────────────────────

type Compiler struct {
	chunk       *bytecode.Chunk
	localVars   []localVar
	scopeDepth  int
	stackSize   int  // NEW: track stack height for local slots
	functions   map[string]funcInfo
	currentFunc string
	errors      []string
}

type localVar struct {
	name  string
	depth int
}

type funcInfo struct {
	arity      int
	chunk      *bytecode.Chunk
	returnType string
}

func NewCompiler() *Compiler {
	return &Compiler{
		chunk:     &bytecode.Chunk{},
		localVars: make([]localVar, 0),
		functions: make(map[string]funcInfo),
	}
}

func (c *Compiler) Compile(program ast.Program) (*bytecode.Chunk, error) {
	for _, stmt := range program.Body {
		c.compileNode(stmt)
	}
	c.emit(bytecode.OP_HALT, 0)

	if len(c.errors) > 0 {
		result := ""
		for _, e := range c.errors {
			result += e + "\n"
		}
		return nil, fmt.Errorf("%s", result)
	}

	// Embed functions into the main chunk for serialization
	c.chunk.Functions = make(map[string]*bytecode.Chunk)
	for name, info := range c.functions {
		c.chunk.Functions[name] = info.chunk
	}

	return c.chunk, nil
}

// ─── Scope Management ───────────────────────────────────────────────────────

func (c *Compiler) beginScope() {
	c.scopeDepth++
}

func (c *Compiler) endScope() {
	c.scopeDepth--
	count := 0
	for len(c.localVars) > 0 && c.localVars[len(c.localVars)-1].depth > c.scopeDepth {
		count++
		c.localVars = c.localVars[:len(c.localVars)-1]
	}
	c.stackSize -= count
	}

func (c *Compiler) addLocal(name string) int {
	idx := c.stackSize
	c.localVars = append(c.localVars, localVar{name: name, depth: c.scopeDepth})
	c.stackSize++
	return idx
}

func (c *Compiler) resolveLocal(name string) (int, bool) {
	for i := len(c.localVars) - 1; i >= 0; i-- {
		if c.localVars[i].name == name {
			return i, true
		}
	}
	return -1, false
}

// ─── Emit Helpers ───────────────────────────────────────────────────────────

func (c *Compiler) emit(op byte, operand int16) {
	c.chunk.Code = append(c.chunk.Code, bytecode.Instruction{OpCode: op, Operand: operand})
}

func (c *Compiler) emitJump(op byte) int {
	c.emit(op, 0x7FFF)
	return len(c.chunk.Code) - 1
}

func (c *Compiler) patchJump(offset int) {
	jump := len(c.chunk.Code) - offset - 1
	c.chunk.Code[offset].Operand = int16(jump)
}

func (c *Compiler) emitLoop(loopStart int) {
	offset := len(c.chunk.Code) - loopStart + 1
	c.emit(bytecode.OP_JUMP, int16(-offset))
}

// ─── Constant Pool ──────────────────────────────────────────────────────────

func (c *Compiler) addConstant(v bytecode.Value) int {
	c.chunk.Constants = append(c.chunk.Constants, v)
	return len(c.chunk.Constants) - 1
}

func (c *Compiler) addConstantInt(i int64) int {
	return c.addConstant(bytecode.Value{Type: bytecode.VAL_INT, AsInt: i})
}

func (c *Compiler) addConstantFloat(f float64) int {
	return c.addConstant(bytecode.Value{Type: bytecode.VAL_FLOAT, AsFloat: f})
}

func (c *Compiler) addConstantString(s string) int {
	return c.addConstant(bytecode.Value{Type: bytecode.VAL_STRING, AsString: s})
}

func (c *Compiler) addConstantBool(b bool) int {
	return c.addConstant(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: b})
}

// ─── Node Compilation ─────────────────────────────────────────────────────────
func isEffectOnly(expr ast.Node) bool {
	switch expr.(type) {
	case ast.Increment, ast.Decrement, ast.Assignment:
		return true
	default:
		return false
	}
}


func (c *Compiler) compileNode(node ast.Node) {
	switch n := node.(type) {

	case ast.Program:
		for _, stmt := range n.Body {
			c.compileNode(stmt)
		}

	case ast.VariableDeclaration:
		c.compileExpression(n.Value)
		if c.scopeDepth > 0 {
			idx := c.addLocal(n.Identifier)
			c.emit(bytecode.OP_STORE_LOCAL, int16(idx))
		} else {
			nameIdx := c.addConstantString(n.Identifier)
			c.emit(bytecode.OP_STORE_GLOBAL, int16(nameIdx))
		}

	case ast.Assignment:
		c.compileExpression(n.Value)
		if idx, ok := c.resolveLocal(n.Identifier); ok {
			c.emit(bytecode.OP_STORE_LOCAL, int16(idx))
		} else {
			nameIdx := c.addConstantString(n.Identifier)
			c.emit(bytecode.OP_STORE_GLOBAL, int16(nameIdx))
		}

	case ast.PrintStatement:
		c.compileExpression(n.Expression)
		c.emit(bytecode.OP_PRINT, 0)

	case ast.IfStatement:
		c.compileIfStatement(n)

	case ast.WhileLoop:
		c.compileWhileLoop(n)

	case ast.Function:
		c.compileFunction(n)

	case ast.ReturnStatement:
		if n.Value != nil {
			c.compileExpression(n.Value)
			c.emit(bytecode.OP_RETURN, 0)
		} else {
			c.emit(bytecode.OP_RETURN_VOID, 0)
		}

	case ast.ExpressionStatement:
    		c.compileExpression(n.Expression)
    		// Don't pop if the expression is pure side-effect (no value produced)
    		if !isEffectOnly(n.Expression) {
        		c.emit(bytecode.OP_POP, 0)
    		}

	case ast.Comment:
		// No-op
	}
}

// ─── Control Flow ───────────────────────────────────────────────────────────

func (c *Compiler) compileIfStatement(n ast.IfStatement) {
	c.compileExpression(n.Condition)

	thenJump := c.emitJump(bytecode.OP_JUMP_IF_FALSE)

	c.beginScope()
	for _, stmt := range n.Consequent {
		c.compileNode(stmt)
	}
	c.endScope()

	elseJump := -1
	if len(n.Alternate) > 0 {
		elseJump = c.emitJump(bytecode.OP_JUMP)
	}

	c.patchJump(thenJump)

	if len(n.Alternate) > 0 {
		c.beginScope()
		for _, stmt := range n.Alternate {
			c.compileNode(stmt)
		}
		c.endScope()
		c.patchJump(elseJump)
	}
}

func (c *Compiler) compileWhileLoop(n ast.WhileLoop) {
	loopStart := len(c.chunk.Code)

	c.compileExpression(n.Condition)

	exitJump := c.emitJump(bytecode.OP_JUMP_IF_FALSE)

	c.beginScope()
	for _, stmt := range n.Body {
		c.compileNode(stmt)
	}
	c.endScope()

	c.emitLoop(loopStart)

	c.patchJump(exitJump)
}

// ─── Functions ──────────────────────────────────────────────────────────────

func (c *Compiler) compileFunction(n ast.Function) {
	savedChunk := c.chunk
	savedLocals := c.localVars
	savedDepth := c.scopeDepth

	funcChunk := &bytecode.Chunk{}
	c.chunk = funcChunk
	c.localVars = make([]localVar, 0)
	c.scopeDepth = 1

	for _, param := range n.Params {
		c.addLocal(param.Name)
	}

	for _, stmt := range n.Body {
		c.compileNode(stmt)
	}

	c.emit(bytecode.OP_RETURN_VOID, 0)

	c.functions[n.Name] = funcInfo{
		arity:      len(n.Params),
		chunk:      funcChunk,
		returnType: n.ReturnType,
	}

	c.chunk = savedChunk
	c.localVars = savedLocals
	c.scopeDepth = savedDepth
}

// ─── Expressions ──────────────────────────────────────────────────────────────

func (c *Compiler) compileExpression(expr ast.Node) {
	switch n := expr.(type) {

	case ast.LiteralInt:
		idx := c.addConstantInt(n.Value)
		c.emit(bytecode.OP_CONST, int16(idx))

	case ast.LiteralFloat:
		idx := c.addConstantFloat(n.Value)
		c.emit(bytecode.OP_CONST, int16(idx))

	case ast.LiteralString:
		idx := c.addConstantString(n.Value)
		c.emit(bytecode.OP_CONST, int16(idx))

	case ast.LiteralBool:
		idx := c.addConstantBool(n.Value)
		c.emit(bytecode.OP_CONST, int16(idx))

	case ast.Identifier:
		if idx, ok := c.resolveLocal(n.Name); ok {
			c.emit(bytecode.OP_LOAD_LOCAL, int16(idx))
		} else {
			nameIdx := c.addConstantString(n.Name)
			c.emit(bytecode.OP_LOAD_GLOBAL, int16(nameIdx))
		}

	case ast.BinaryExpression:
		c.compileBinaryExpression(n)

	case ast.CallExpression:
		c.compileCall(n)

	case ast.Increment:
		if idx, ok := c.resolveLocal(n.Identifier); ok {
			c.emit(bytecode.OP_INC_LOCAL, int16(idx))
		} else {
			nameIdx := c.addConstantString(n.Identifier)
			c.emit(bytecode.OP_LOAD_GLOBAL, int16(nameIdx))
			c.emit(bytecode.OP_CONST, int16(c.addConstantInt(1)))
			c.emit(bytecode.OP_ADD, 0)
			c.emit(bytecode.OP_STORE_GLOBAL, int16(nameIdx))
		}

	case ast.Decrement:
		if idx, ok := c.resolveLocal(n.Identifier); ok {
			c.emit(bytecode.OP_DEC_LOCAL, int16(idx))
		} else {
			nameIdx := c.addConstantString(n.Identifier)
			c.emit(bytecode.OP_LOAD_GLOBAL, int16(nameIdx))
			c.emit(bytecode.OP_CONST, int16(c.addConstantInt(1)))
			c.emit(bytecode.OP_SUB, 0)
			c.emit(bytecode.OP_STORE_GLOBAL, int16(nameIdx))
		}

	case ast.IndexExpression:
		c.compileExpression(n.Object)
		c.compileExpression(n.Index)
		c.emit(bytecode.OP_INDEX_GET, 0)
	}
}

func (c *Compiler) compileBinaryExpression(n ast.BinaryExpression) {
	c.compileExpression(n.Left)
	c.compileExpression(n.Right)

	switch n.Operator {
	case "+":
		c.emit(bytecode.OP_ADD, 0)
	case "-":
		c.emit(bytecode.OP_SUB, 0)
	case "*":
		c.emit(bytecode.OP_MUL, 0)
	case "/":
		c.emit(bytecode.OP_DIV, 0)
	case "%":
		c.emit(bytecode.OP_MOD, 0)
	case "==":
		c.emit(bytecode.OP_EQ, 0)
	case "!=":
		c.emit(bytecode.OP_NE, 0)
	case "<":
		c.emit(bytecode.OP_LT, 0)
	case ">":
		c.emit(bytecode.OP_GT, 0)
	case "<=":
		c.emit(bytecode.OP_LE, 0)
	case ">=":
		c.emit(bytecode.OP_GE, 0)
	case "&&":
		c.emit(bytecode.OP_AND, 0)
	case "||":
		c.emit(bytecode.OP_OR, 0)
	}
}

func (c *Compiler) compileCall(n ast.CallExpression) {
	for _, arg := range n.Arguments {
		c.compileExpression(arg)
	}
	nameIdx := c.addConstantString(n.Callee)
	c.emit(bytecode.OP_CALL, int16(nameIdx))
	c.emit(bytecode.OP_CONST, int16(len(n.Arguments)))
}

// ─── Expose Functions ─────────────────────────────────────────────────────────

func (c *Compiler) GetFunctions() map[string]*bytecode.Chunk {
	if c.chunk.Functions == nil {
		return make(map[string]*bytecode.Chunk)
	}
	return c.chunk.Functions
}
