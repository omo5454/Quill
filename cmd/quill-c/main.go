package main

import (
	"fmt"
	"os"

	"quill/src/core/compiler"
	"quill/src/core/lexer"
	"quill/src/core/parser"
	"quill/src/core/bytecode"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Println("Usage: quill-c <input.qsc> <output.qbc>")
		os.Exit(1)
	}

	inputFile := os.Args[1]
	outputFile := os.Args[2]

	// Read source
	source, err := os.ReadFile(inputFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading file: %v\n", err)
		os.Exit(1)
	}

	// Lex
	l := lexer.NewLexer(string(source))
	tokens := l.Tokenize()

	// Parse
	p := parser.NewParser(tokens)
	program, err := p.Parse()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Parse error: %v\n", err)
		os.Exit(1)
	}

	// Compile
	c := compiler.NewCompiler()
	chunk, err := c.Compile(program)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Compile error: %v\n", err)
		os.Exit(1)
	}

	// Serialize to file
	if err := bytecode.WriteChunk(chunk, outputFile); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing bytecode: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("Compiled %s -> %s\n", inputFile, outputFile)
}
