package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"

	"quill/src/core/bytecode"
	"quill/src/core/compiler"
	"quill/src/core/lexer"
	"quill/src/core/parser"
	"quill/src/core/vm"
)

func main() {
	debug := flag.Bool("debug", false, "Enable debug output")
	flag.Parse()

	args := flag.Args()
	if len(args) < 1 {
		fmt.Println("Usage: quill [--debug] <file.qsc|file.qbc>")
		os.Exit(1)
	}

	inputFile := args[0]
	ext := filepath.Ext(inputFile)

	var err error
	switch ext {
	case ".qsc":
		err = runFromSource(inputFile, *debug)
	case ".qbc":
		err = runFromBytecode(inputFile, *debug)
	default:
		err = fmt.Errorf("unknown file extension: %s (expected .qsc or .qbc)", ext)
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}
}

func runFromSource(filename string, debug bool) error {
	source, err := os.ReadFile(filename)
	if err != nil {
		return fmt.Errorf("reading file: %w", err)
	}

	if debug {
		fmt.Println("=== LEXING ===")
	}
	tokens := lexer.NewLexer(string(source)).Tokenize()
	if debug {
		for i, tok := range tokens {
			if i > 20 {
				fmt.Println("...")
				break
			}
			fmt.Printf("  %v: %q\n", tok.Type, tok.Value)
		}
	}

	if debug {
		fmt.Println("\n=== PARSING ===")
	}
	program, err := parser.NewParser(tokens).Parse()
	if err != nil {
		return fmt.Errorf("parse: %w", err)
	}
	if debug {
		fmt.Printf("  AST nodes: %d\n", len(program.Body))
	}

	if debug {
		fmt.Println("\n=== COMPILING ===")
	}
	comp := compiler.NewCompiler()
	chunk, err := comp.Compile(program)
	if err != nil {
		return fmt.Errorf("compile: %w", err)
	}
	if debug {
		fmt.Printf("  Instructions: %d\n", len(chunk.Code))
		fmt.Printf("  Constants: %d\n", len(chunk.Constants))
		fmt.Printf("  Functions: %d\n", len(chunk.Functions))
	}

	if debug {
		fmt.Println("\n=== BYTECODE DUMP ===")
		for i, inst := range chunk.Code {
			opName := opCodeName(inst.OpCode)
			fmt.Printf("  [%3d] %s %d\n", i, opName, inst.Operand)
			if i > 50 {
				fmt.Println("  ...")
				break
			}
		}
	}

	return executeChunk(chunk, debug)
}

func runFromBytecode(filename string, debug bool) error {
	chunk, err := bytecode.ReadChunk(filename)
	if err != nil {
		return fmt.Errorf("reading bytecode: %w", err)
	}
	if debug {
		fmt.Printf("Loaded chunk: %d instructions, %d functions\n", len(chunk.Code), len(chunk.Functions))
	}
	return executeChunk(chunk, debug)
}

func executeChunk(chunk *bytecode.Chunk, debug bool) error {
	machine := vm.New()
	machine.LoadChunk(chunk)

	for name, funcChunk := range chunk.Functions {
		if debug {
			fmt.Printf("  Registering function: %s (%d instructions)\n", name, len(funcChunk.Code))
		}
		machine.RegisterFunction(name, funcChunk)
	}

	if debug {
		fmt.Println("\n=== OUTPUT ===")
	}
	err := machine.Run()
	if debug {
		fmt.Println("\n=== DONE ===")
	}
	return err
}

func opCodeName(op byte) string {
	names := []string{
		"CONST", "POP", "DUP", "SWAP", "LOAD_LOCAL", "STORE_LOCAL",
		"LOAD_GLOBAL", "STORE_GLOBAL", "ADD", "SUB", "MUL", "DIV", "MOD",
		"NEG", "EQ", "NE", "LT", "GT", "LE", "GE", "AND", "OR", "NOT",
		"INC_LOCAL", "DEC_LOCAL", "JUMP", "JUMP_IF_FALSE", "JUMP_IF_TRUE",
		"CALL", "RETURN", "RETURN_VOID", "PRINT", "PRINTLN",
		"INDEX_GET", "INDEX_SET", "ARRAY_MAKE", "HALT",
	}
	if int(op) < len(names) {
		return names[op]
	}
	return fmt.Sprintf("UNKNOWN(%d)", op)
}
