package types

type TokenType string

const (
	// Literals
	Identifier TokenType = "Identifier"
	Number     TokenType = "Number"
	Float      TokenType = "Float"
	Str        TokenType = "String"
	FString    TokenType = "FString"

	// Keywords
	Keyword TokenType = "Keyword"

	// Operators
	Operator TokenType = "Operator"

	// Special
	EOF            TokenType = "EOF"
	Illegal        TokenType = "Illegal"
	Comment        TokenType = "Comment"
	Dot            TokenType = "Dot"
	Incrementation TokenType = "Incrementation"

	// Categories
	Function    TokenType = "Function"
	Boolean     TokenType = "Boolean"
	Conditional TokenType = "Conditional"
	Loop        TokenType = "Loop"
	Integer     TokenType = "Integer"
	Punctuation TokenType = "Punctuation"
)

type Token struct {
	Type  TokenType
	Value string
}
