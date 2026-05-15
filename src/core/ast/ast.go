package ast

// Node is the base interface every AST node implements
type Node interface{}

// Program is the top-level AST node
type Program struct {
	Body []Node
}

// VariableDeclaration: let x: int = 5; or const x: int = 5;
type VariableDeclaration struct {
	Identifier   string
	DeclaredType string
	Value        Node
	Mutable      bool // true for let, false for const
}

// Function: func add(a: int, b: int): int { ... }
type Function struct {
	Name       string
	Params     []Param
	ReturnType string
	Body       []Node
}

type Param struct {
	Name string
	Type string
}

// ReturnStatement: return x + 1;
type ReturnStatement struct {
	Value Node
}

// PrintStatement: print "hello";
type PrintStatement struct {
	Expression Node
}

// IfStatement: if x > 5 { ... } else { ... }
type IfStatement struct {
	Condition  Node
	Consequent []Node
	Alternate  []Node
}

// WhileLoop: while x < 10 { ... }
type WhileLoop struct {
	Condition Node
	Body      []Node
}

// Comment: # this is a comment
type Comment struct {
	Value string
}

// BinaryExpression: x + y, x == y, etc.
type BinaryExpression struct {
	Left     Node
	Operator string
	Right    Node
}

// CallExpression: add(3, 4)
type CallExpression struct {
	Callee    string
	Arguments []Node
}

// Increment: x++
type Increment struct {
	Identifier string
}

// Decrement: x--
type Decrement struct {
	Identifier string
}

// Identifier: a variable reference
type Identifier struct {
	Name string
}

// LiteralInt: 42
type LiteralInt struct {
	Value int64
}

// LiteralFloat: 3.14
type LiteralFloat struct {
	Value float64
}

// LiteralString: "hello"
type LiteralString struct {
	Value string
}

// LiteralBool: true / false
type LiteralBool struct {
	Value bool
}

// IndexExpression: arr[0]
type IndexExpression struct {
	Object Node
	Index  Node
}

// Assignment: x = 5
type Assignment struct {
	Identifier string
	Value      Node
}

// ExpressionStatement: wrapper for expressions used as statements
type ExpressionStatement struct {
	Expression Node
}

// UnaryExpression: -5, !true
type UnaryExpression struct {
	Operator string
	Operand  Node
}
