package bytecode

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
)

// ─── Value Types ─────────────────────────────────────────────────────────────

const (
	VAL_INT = iota
	VAL_FLOAT
	VAL_BOOL
	VAL_STRING
	VAL_NIL
	VAL_ARRAY
)

type Value struct {
	Type     int
	AsInt    int64
	AsFloat  float64
	AsBool   bool
	AsString string
	AsArray  []Value
}

// ─── Instruction ─────────────────────────────────────────────────────────────

type Instruction struct {
	OpCode  byte
	Operand int16
}

// ─── OpCodes ─────────────────────────────────────────────────────────────────

const (
	OP_CONST = iota
	OP_POP
	OP_DUP
	OP_SWAP
	OP_LOAD_LOCAL
	OP_STORE_LOCAL
	OP_LOAD_GLOBAL
	OP_STORE_GLOBAL
	OP_ADD
	OP_SUB
	OP_MUL
	OP_DIV
	OP_MOD
	OP_NEG
	OP_EQ
	OP_NE
	OP_LT
	OP_GT
	OP_LE
	OP_GE
	OP_AND
	OP_OR
	OP_NOT
	OP_INC_LOCAL
	OP_DEC_LOCAL
	OP_JUMP
	OP_JUMP_IF_FALSE
	OP_JUMP_IF_TRUE
	OP_CALL
	OP_RETURN
	OP_RETURN_VOID
	OP_PRINT
	OP_PRINTLN
	OP_INDEX_GET
	OP_INDEX_SET
	OP_ARRAY_MAKE
	OP_HALT
)

// ─── Chunk ───────────────────────────────────────────────────────────────────

type Chunk struct {
	Code       []Instruction
	Constants  []Value
	Functions  map[string]*Chunk  // NEW: embedded function chunks
}

// ─── Serialization ───────────────────────────────────────────────────────────

func WriteChunk(chunk *Chunk, filename string) error {
	var buf bytes.Buffer

	// Header
	buf.Write([]byte("QBC"))
	buf.WriteByte(2) // Version 2 (functions support)

	// Write main chunk
	if err := writeChunkData(&buf, chunk); err != nil {
		return err
	}

	// Write function table
	binary.Write(&buf, binary.LittleEndian, int32(len(chunk.Functions)))
	for name, fnChunk := range chunk.Functions {
		binary.Write(&buf, binary.LittleEndian, int32(len(name)))
		buf.WriteString(name)
		if err := writeChunkData(&buf, fnChunk); err != nil {
			return err
		}
	}

	return os.WriteFile(filename, buf.Bytes(), 0644)
}

func ReadChunk(filename string) (*Chunk, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	buf := bytes.NewReader(data)

	// Header
	magic := make([]byte, 3)
	if _, err := buf.Read(magic); err != nil || string(magic) != "QBC" {
		return nil, fmt.Errorf("invalid bytecode file")
	}

	var version byte
	if err := binary.Read(buf, binary.LittleEndian, &version); err != nil {
		return nil, fmt.Errorf("reading version: %w", err)
	}
	if version != 2 {
		return nil, fmt.Errorf("unsupported bytecode version: %d (need 2)", version)
	}

	// Read main chunk
	chunk, err := readChunkData(buf)
	if err != nil {
		return nil, fmt.Errorf("reading main chunk: %w", err)
	}

	// Read function table
	var funcCount int32
	if err := binary.Read(buf, binary.LittleEndian, &funcCount); err != nil {
		return nil, fmt.Errorf("reading function count: %w", err)
	}

	chunk.Functions = make(map[string]*Chunk)
	for i := int32(0); i < funcCount; i++ {
		var nameLen int32
		binary.Read(buf, binary.LittleEndian, &nameLen)
		nameBytes := make([]byte, nameLen)
		buf.Read(nameBytes)
		name := string(nameBytes)

		fnChunk, err := readChunkData(buf)
		if err != nil {
			return nil, fmt.Errorf("reading function %s: %w", name, err)
		}
		chunk.Functions[name] = fnChunk
	}

	return chunk, nil
}

// ─── Internal helpers ────────────────────────────────────────────────────────

func writeChunkData(buf *bytes.Buffer, chunk *Chunk) error {
	// Constants
	binary.Write(buf, binary.LittleEndian, int32(len(chunk.Constants)))
	for _, c := range chunk.Constants {
		binary.Write(buf, binary.LittleEndian, byte(c.Type))
		switch c.Type {
		case VAL_INT:
			binary.Write(buf, binary.LittleEndian, c.AsInt)
		case VAL_FLOAT:
			binary.Write(buf, binary.LittleEndian, c.AsFloat)
		case VAL_BOOL:
			binary.Write(buf, binary.LittleEndian, c.AsBool)
		case VAL_STRING:
			binary.Write(buf, binary.LittleEndian, int32(len(c.AsString)))
			buf.WriteString(c.AsString)
		case VAL_ARRAY:
			binary.Write(buf, binary.LittleEndian, int32(len(c.AsArray)))
			for _, elem := range c.AsArray {
				binary.Write(buf, binary.LittleEndian, byte(elem.Type))
				switch elem.Type {
				case VAL_INT:
					binary.Write(buf, binary.LittleEndian, elem.AsInt)
				case VAL_FLOAT:
					binary.Write(buf, binary.LittleEndian, elem.AsFloat)
				case VAL_BOOL:
					binary.Write(buf, binary.LittleEndian, elem.AsBool)
				case VAL_STRING:
					binary.Write(buf, binary.LittleEndian, int32(len(elem.AsString)))
					buf.WriteString(elem.AsString)
				}
			}
		}
	}

	// Instructions
	binary.Write(buf, binary.LittleEndian, int32(len(chunk.Code)))
	for _, inst := range chunk.Code {
		buf.WriteByte(inst.OpCode)
		binary.Write(buf, binary.LittleEndian, inst.Operand)
	}

	return nil
}

func readChunkData(buf *bytes.Reader) (*Chunk, error) {
	chunk := &Chunk{}

	// Constants
	var constCount int32
	if err := binary.Read(buf, binary.LittleEndian, &constCount); err != nil {
		return nil, err
	}
	chunk.Constants = make([]Value, constCount)

	for i := range chunk.Constants {
		var typ byte
		if err := binary.Read(buf, binary.LittleEndian, &typ); err != nil {
			return nil, err
		}
		chunk.Constants[i].Type = int(typ)

		switch chunk.Constants[i].Type {
		case VAL_INT:
			binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsInt)
		case VAL_FLOAT:
			binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsFloat)
		case VAL_BOOL:
			binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsBool)
		case VAL_STRING:
			var strLen int32
			binary.Read(buf, binary.LittleEndian, &strLen)
			strBytes := make([]byte, strLen)
			buf.Read(strBytes)
			chunk.Constants[i].AsString = string(strBytes)
		case VAL_ARRAY:
			var arrLen int32
			binary.Read(buf, binary.LittleEndian, &arrLen)
			chunk.Constants[i].AsArray = make([]Value, arrLen)
			for j := range chunk.Constants[i].AsArray {
				var elemTyp byte
				binary.Read(buf, binary.LittleEndian, &elemTyp)
				chunk.Constants[i].AsArray[j].Type = int(elemTyp)
				switch chunk.Constants[i].AsArray[j].Type {
				case VAL_INT:
					binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsArray[j].AsInt)
				case VAL_FLOAT:
					binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsArray[j].AsFloat)
				case VAL_BOOL:
					binary.Read(buf, binary.LittleEndian, &chunk.Constants[i].AsArray[j].AsBool)
				case VAL_STRING:
					var strLen int32
					binary.Read(buf, binary.LittleEndian, &strLen)
					strBytes := make([]byte, strLen)
					buf.Read(strBytes)
					chunk.Constants[i].AsArray[j].AsString = string(strBytes)
				}
			}
		}
	}

	// Instructions
	var codeCount int32
	if err := binary.Read(buf, binary.LittleEndian, &codeCount); err != nil {
		return nil, err
	}
	chunk.Code = make([]Instruction, codeCount)

	for i := range chunk.Code {
		op, _ := buf.ReadByte()
		chunk.Code[i].OpCode = op
		binary.Read(buf, binary.LittleEndian, &chunk.Code[i].Operand)
	}

	return chunk, nil
}
