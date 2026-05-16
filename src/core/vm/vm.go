package vm

import (
	"fmt"
	"quill/src/core/bytecode"
	"time"
)

// ─── VM ──────────────────────────────────────────────────────────────────────

type VM struct {
	chunk     *bytecode.Chunk
	ip        int
	stack     []bytecode.Value
	sp        int
	globals   map[string]bytecode.Value
	functions map[string]*bytecode.Chunk
	frames    []callFrame
}

type callFrame struct {
	chunk       *bytecode.Chunk
	ip          int
	basePointer int
}

const stackSize = 1024

func New() *VM {
	return &VM{
		stack:     make([]bytecode.Value, stackSize),
		sp:        0,
		globals:   make(map[string]bytecode.Value),
		functions: make(map[string]*bytecode.Chunk),
		frames:    make([]callFrame, 0),
	}
}

func (vm *VM) LoadChunk(chunk *bytecode.Chunk) {
	vm.chunk = chunk
	vm.ip = 0
}

func (vm *VM) RegisterFunction(name string, chunk *bytecode.Chunk) {
	vm.functions[name] = chunk
}

// ─── Stack Operations ────────────────────────────────────────────────────────

func (vm *VM) push(v bytecode.Value) {
	if vm.sp >= stackSize {
		panic("stack overflow")
	}
	vm.stack[vm.sp] = v
	vm.sp++
}

func (vm *VM) pop() bytecode.Value {
	if vm.sp == 0 {
		panic("stack underflow")
	}
	vm.sp--
	return vm.stack[vm.sp]
}

func (vm *VM) peek(distance int) bytecode.Value {
	if vm.sp-1-distance < 0 {
		panic("stack underflow on peek")
	}
	return vm.stack[vm.sp-1-distance]
}

// ─── Execution ───────────────────────────────────────────────────────────────

func (vm *VM) Run() error {
	for {
		if vm.ip >= len(vm.chunk.Code) {
			return fmt.Errorf("instruction pointer out of bounds")
		}

		instr := vm.chunk.Code[vm.ip]
		vm.ip++

		switch instr.OpCode {

		case bytecode.OP_CONST:
			val := vm.chunk.Constants[instr.Operand]
			vm.push(val)

		case bytecode.OP_POP:
			vm.pop()

		case bytecode.OP_DUP:
			v := vm.peek(0)
			vm.push(v)

		case bytecode.OP_SWAP:
			a := vm.pop()
			b := vm.pop()
			vm.push(a)
			vm.push(b)

		case bytecode.OP_LOAD_LOCAL:
			idx := instr.Operand
			frame := vm.currentFrame()
			vm.push(vm.stack[frame.basePointer+int(idx)])

		case bytecode.OP_STORE_LOCAL:
			idx := instr.Operand
			frame := vm.currentFrame()
			val := vm.stack[vm.sp-1]
			vm.stack[frame.basePointer+int(idx)] = val

		case bytecode.OP_LOAD_GLOBAL:
			idx := int(instr.Operand)
			name := vm.chunk.Constants[idx].AsString
			val, ok := vm.globals[name]
			if !ok {
				return fmt.Errorf("undefined global variable '%s'", name)
			}
			vm.push(val)

		case bytecode.OP_STORE_GLOBAL:
			idx := int(instr.Operand)
			name := vm.chunk.Constants[idx].AsString
			val := vm.pop()
			vm.globals[name] = val

		case bytecode.OP_ADD:
    			b := vm.pop()
    			a := vm.pop()
    			if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
        			vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: a.AsInt + b.AsInt})
        			continue
    			}
    			vm.push(vm.addValues(a, b))

		case bytecode.OP_SUB:
			b := vm.pop()
			a := vm.pop()
			if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: a.AsInt - b.AsInt})
				continue
			}
			vm.push(vm.subValues(a, b))

		case bytecode.OP_MUL:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: a.AsInt * b.AsInt})
				continue
		    	}
		    	vm.push(vm.mulValues(a, b))

		case bytecode.OP_DIV:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				if b.AsInt == 0 {
			    		return fmt.Errorf("division by zero")
				}
				vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: a.AsInt / b.AsInt})
				continue
		    	}
		    	if vm.isZero(b) {
				return fmt.Errorf("division by zero")
		    	}
		    	vm.push(vm.divValues(a, b))

		case bytecode.OP_LT:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt < b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.compareValues(a, b) < 0})

		case bytecode.OP_GT:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt > b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.compareValues(a, b) > 0})

		case bytecode.OP_LE:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt <= b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.compareValues(a, b) <= 0})

		case bytecode.OP_GE:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt >= b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.compareValues(a, b) >= 0})

		case bytecode.OP_EQ:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt == b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.valuesEqual(a, b)})

		case bytecode.OP_NE:
		    	b := vm.pop()
		    	a := vm.pop()
		    	if a.Type == bytecode.VAL_INT && b.Type == bytecode.VAL_INT {
				vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: a.AsInt != b.AsInt})
				continue
		    	}
		    	vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: !vm.valuesEqual(a, b)})
				case bytecode.OP_AND:
			b := vm.pop()
			a := vm.pop()
			vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.isTruthy(a) && vm.isTruthy(b)})

		case bytecode.OP_OR:
			b := vm.pop()
			a := vm.pop()
			vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: vm.isTruthy(a) || vm.isTruthy(b)})

		case bytecode.OP_NOT:
			v := vm.pop()
			vm.push(bytecode.Value{Type: bytecode.VAL_BOOL, AsBool: !vm.isTruthy(v)})

		case bytecode.OP_INC_LOCAL:
			idx := instr.Operand
			frame := vm.currentFrame()
			val := vm.stack[frame.basePointer+int(idx)]
			vm.stack[frame.basePointer+int(idx)] = vm.incrementValue(val)

		case bytecode.OP_DEC_LOCAL:
			idx := instr.Operand
			frame := vm.currentFrame()
			val := vm.stack[frame.basePointer+int(idx)]
			vm.stack[frame.basePointer+int(idx)] = vm.decrementValue(val)

		case bytecode.OP_JUMP:
			offset := int(instr.Operand)
			vm.ip += offset

		case bytecode.OP_JUMP_IF_FALSE:
			offset := int(instr.Operand)
			cond := vm.pop()
			if !vm.isTruthy(cond) {
				vm.ip += offset
			}

		case bytecode.OP_JUMP_IF_TRUE:
			offset := int(instr.Operand)
			cond := vm.pop()
			if vm.isTruthy(cond) {
				vm.ip += offset
			}

		case bytecode.OP_CALL:
			nameIdx := int(instr.Operand)
			name := vm.chunk.Constants[nameIdx].AsString

			// FIXED: Read arity from next instruction
			if vm.ip >= len(vm.chunk.Code) {
				return fmt.Errorf("missing arity instruction after OP_CALL")
			}
			arityInstr := vm.chunk.Code[vm.ip]
			vm.ip++
			arity := int(arityInstr.Operand)

			funcChunk, ok := vm.functions[name]
			if !ok {
				if err := vm.callBuiltin(name, arity); err != nil {
					return err
				}
				continue
			}

			vm.frames = append(vm.frames, callFrame{
				chunk:       vm.chunk,
				ip:          vm.ip,
				basePointer: vm.sp - arity,
			})

			vm.chunk = funcChunk
			vm.ip = 0

		case bytecode.OP_RETURN:
			retVal := vm.pop()
			vm.popFrame()
			vm.push(retVal)

		case bytecode.OP_RETURN_VOID:
			// FIXED: Push nil before popping frame so caller gets a value
			vm.popFrame()
			vm.push(bytecode.Value{Type: bytecode.VAL_NIL})

		case bytecode.OP_PRINT:
			val := vm.pop()
			fmt.Println(vm.valueToString(val))

		case bytecode.OP_PRINTLN:
			val := vm.pop()
			fmt.Println(vm.valueToString(val))

		case bytecode.OP_INDEX_GET:
			idx := vm.pop()
			obj := vm.pop()
			vm.push(vm.getIndex(obj, idx))

		case bytecode.OP_INDEX_SET:
			val := vm.pop()
			idx := vm.pop()
			obj := vm.pop()
			vm.setIndex(obj, idx, val)

		case bytecode.OP_ARRAY_MAKE:
			count := int(instr.Operand)
			elems := make([]bytecode.Value, count)
			for i := count - 1; i >= 0; i-- {
				elems[i] = vm.pop()
			}
			// FIXED: Push the array value onto the stack
			vm.push(bytecode.Value{Type: bytecode.VAL_ARRAY, AsArray: elems})

		case bytecode.OP_HALT:
			return nil

		default:
			return fmt.Errorf("unknown opcode: %d", instr.OpCode)
		}
	}
}

// ─── Frame Management ────────────────────────────────────────────────────────

func (vm *VM) currentFrame() callFrame {
	if len(vm.frames) == 0 {
		return callFrame{basePointer: 0}
	}
	return vm.frames[len(vm.frames)-1]
}

func (vm *VM) popFrame() {
	if len(vm.frames) == 0 {
		return
	}
	frame := vm.frames[len(vm.frames)-1]
	vm.frames = vm.frames[:len(vm.frames)-1]
	vm.chunk = frame.chunk
	vm.ip = frame.ip
	vm.sp = frame.basePointer
}

// ─── Value Operations ───────────────────────────────────────────────────────────

func (vm *VM) addValues(a, b bytecode.Value) bytecode.Value {
	if a.Type == bytecode.VAL_STRING || b.Type == bytecode.VAL_STRING {
		return bytecode.Value{
			Type:     bytecode.VAL_STRING,
			AsString: vm.valueToString(a) + vm.valueToString(b),
		}
	}
	if a.Type == bytecode.VAL_FLOAT || b.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{
			Type:    bytecode.VAL_FLOAT,
			AsFloat: vm.toFloat(a) + vm.toFloat(b),
		}
	}
	return bytecode.Value{
		Type:  bytecode.VAL_INT,
		AsInt: a.AsInt + b.AsInt,
	}
}

func (vm *VM) subValues(a, b bytecode.Value) bytecode.Value {
	if a.Type == bytecode.VAL_FLOAT || b.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{
			Type:    bytecode.VAL_FLOAT,
			AsFloat: vm.toFloat(a) - vm.toFloat(b),
		}
	}
	return bytecode.Value{
		Type:  bytecode.VAL_INT,
		AsInt: a.AsInt - b.AsInt,
	}
}

func (vm *VM) mulValues(a, b bytecode.Value) bytecode.Value {
	if a.Type == bytecode.VAL_FLOAT || b.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{
			Type:    bytecode.VAL_FLOAT,
			AsFloat: vm.toFloat(a) * vm.toFloat(b),
		}
	}
	return bytecode.Value{
		Type:  bytecode.VAL_INT,
		AsInt: a.AsInt * b.AsInt,
	}
}

func (vm *VM) divValues(a, b bytecode.Value) bytecode.Value {
	if a.Type == bytecode.VAL_FLOAT || b.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{
			Type:    bytecode.VAL_FLOAT,
			AsFloat: vm.toFloat(a) / vm.toFloat(b),
		}
	}
	return bytecode.Value{
		Type:  bytecode.VAL_INT,
		AsInt: a.AsInt / b.AsInt,
	}
}

func (vm *VM) modValues(a, b bytecode.Value) bytecode.Value {
	if a.Type == bytecode.VAL_FLOAT || b.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{
			Type:    bytecode.VAL_FLOAT,
			AsFloat: float64(int64(vm.toFloat(a)) % int64(vm.toFloat(b))),
		}
	}
	return bytecode.Value{
		Type:  bytecode.VAL_INT,
		AsInt: a.AsInt % b.AsInt,
	}
}

func (vm *VM) negValue(v bytecode.Value) bytecode.Value {
	if v.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{Type: bytecode.VAL_FLOAT, AsFloat: -v.AsFloat}
	}
	return bytecode.Value{Type: bytecode.VAL_INT, AsInt: -v.AsInt}
}

func (vm *VM) incrementValue(v bytecode.Value) bytecode.Value {
	if v.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{Type: bytecode.VAL_FLOAT, AsFloat: v.AsFloat + 1}
	}
	return bytecode.Value{Type: bytecode.VAL_INT, AsInt: v.AsInt + 1}
}

func (vm *VM) decrementValue(v bytecode.Value) bytecode.Value {
	if v.Type == bytecode.VAL_FLOAT {
		return bytecode.Value{Type: bytecode.VAL_FLOAT, AsFloat: v.AsFloat - 1}
	}
	return bytecode.Value{Type: bytecode.VAL_INT, AsInt: v.AsInt - 1}
}

func (vm *VM) toFloat(v bytecode.Value) float64 {
	if v.Type == bytecode.VAL_FLOAT {
		return v.AsFloat
	}
	return float64(v.AsInt)
}

func (vm *VM) isZero(v bytecode.Value) bool {
	if v.Type == bytecode.VAL_FLOAT {
		return v.AsFloat == 0
	}
	if v.Type == bytecode.VAL_INT {
		return v.AsInt == 0
	}
	return false
}

func (vm *VM) valuesEqual(a, b bytecode.Value) bool {
	if a.Type != b.Type {
		if (a.Type == bytecode.VAL_INT || a.Type == bytecode.VAL_FLOAT) &&
			(b.Type == bytecode.VAL_INT || b.Type == bytecode.VAL_FLOAT) {
			return vm.toFloat(a) == vm.toFloat(b)
		}
		return false
	}
	switch a.Type {
	case bytecode.VAL_INT:
		return a.AsInt == b.AsInt
	case bytecode.VAL_FLOAT:
		return a.AsFloat == b.AsFloat
	case bytecode.VAL_BOOL:
		return a.AsBool == b.AsBool
	case bytecode.VAL_STRING:
		return a.AsString == b.AsString
	}
	return false
}

func (vm *VM) compareValues(a, b bytecode.Value) float64 {
	return vm.toFloat(a) - vm.toFloat(b)
}

func (vm *VM) isTruthy(v bytecode.Value) bool {
	switch v.Type {
	case bytecode.VAL_BOOL:
		return v.AsBool
	case bytecode.VAL_INT:
		return v.AsInt != 0
	case bytecode.VAL_FLOAT:
		return v.AsFloat != 0
	case bytecode.VAL_STRING:
		return v.AsString != ""
	case bytecode.VAL_NIL:
		return false
	}
	return false
}

func (vm *VM) valueToString(v bytecode.Value) string {
	switch v.Type {
	case bytecode.VAL_INT:
		return fmt.Sprintf("%d", v.AsInt)
	case bytecode.VAL_FLOAT:
		return fmt.Sprintf("%g", v.AsFloat)
	case bytecode.VAL_BOOL:
		return fmt.Sprintf("%t", v.AsBool)
	case bytecode.VAL_STRING:
		return v.AsString
	case bytecode.VAL_NIL:
		return "nil"
	case bytecode.VAL_ARRAY:
		result := "["
		for i, elem := range v.AsArray {
			if i > 0 {
				result += ", "
			}
			result += vm.valueToString(elem)
		}
		result += "]"
		return result
	}
	return "unknown"
}

// ─── Built-ins ─────────────────────────────────────────────────────────────────

func (vm *VM) callBuiltin(name string, arity int) error {
	switch name {
	case "printf", "say":
		if arity != 1 {
			return fmt.Errorf("'%s' expects 1 argument, got %d", name, arity)
		}
		val := vm.pop()
		fmt.Print(vm.valueToString(val))
		// FIXED: Push nil return value to keep stack balanced
		vm.push(bytecode.Value{Type: bytecode.VAL_NIL})

	case "timeNow":
		if arity != 0 {
			return fmt.Errorf(" 'timeNow' expects 0 arguments, got %d", arity)
		}
		now := float64(time.Now().UnixNano()) / 1e6
		vm.push(bytecode.Value{Type: bytecode.VAL_FLOAT, AsFloat: now})

	case "len":
		if arity != 1 {
			return fmt.Errorf("'len' expects 1 argument, got %d", arity)
		}
		val := vm.pop()
		if val.Type == bytecode.VAL_STRING {
			vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: int64(len(val.AsString))})
		} else if val.Type == bytecode.VAL_ARRAY {
			vm.push(bytecode.Value{Type: bytecode.VAL_INT, AsInt: int64(len(val.AsArray))})
		} else {
			return fmt.Errorf("'len' unsupported type")
		}
	case "toString":
		if arity != 1 {
			return fmt.Errorf("'toString' expects 1 argument, got %d", arity)
		}
		val := vm.pop()
		vm.push(bytecode.Value{Type: bytecode.VAL_STRING, AsString: vm.valueToString(val)})
	default:
		return fmt.Errorf("unknown function '%s'", name)
	}
	return nil
}

// ─── Index Operations ──────────────────────────────────────────────────────────

func (vm *VM) getIndex(obj, idx bytecode.Value) bytecode.Value {
	if obj.Type == bytecode.VAL_STRING && idx.Type == bytecode.VAL_INT {
		i := idx.AsInt
		if i < 0 || i >= int64(len(obj.AsString)) {
			panic("index out of bounds")
		}
		return bytecode.Value{
			Type:     bytecode.VAL_STRING,
			AsString: string(obj.AsString[i]),
		}
	}
	if obj.Type == bytecode.VAL_ARRAY && idx.Type == bytecode.VAL_INT {
		i := idx.AsInt
		if i < 0 || i >= int64(len(obj.AsArray)) {
			panic("index out of bounds")
		}
		return obj.AsArray[i]
	}
	return bytecode.Value{Type: bytecode.VAL_NIL}
}

func (vm *VM) setIndex(obj, idx, val bytecode.Value) {
	// FIXED: Implement array index assignment
	if obj.Type == bytecode.VAL_ARRAY && idx.Type == bytecode.VAL_INT {
		i := idx.AsInt
		if i < 0 || i >= int64(len(obj.AsArray)) {
			panic("index out of bounds")
		}
		obj.AsArray[i] = val
	}
}
