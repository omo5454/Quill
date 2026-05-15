# Quill

Quill is a statically typed, bytecode-compiled scripting language designed for clarity, speed, and shareable function modules. It compiles to a binary `.qbc` bytecode format and runs on a purpose-built VM written in Go.

---

## Why Quill?

- **Statically typed** — type errors are caught at compile time, not runtime
- **Fast** — bytecode VM written in Go, ~3x faster than the original interpreter
- **Simple syntax** — clean, readable, no unnecessary complexity
- **Module-first** — designed around writing and sharing focused function libraries
- **Cross-platform** — runs on Windows, Linux, and macOS

---

## Installation

Download the latest release for your platform from the [releases page](../../releases).

### Windows
Run `QuillInstaller.exe` — optionally adds `quill` and `quill-c` to your PATH during install.

### Linux / macOS
```bash
chmod +x quill
./quill myfile.qsc
```

---

## Usage

```bash
# run a source file directly
quill myfile.qsc

# compile to bytecode
quill-c myfile.qsc output.qbc

# run precompiled bytecode
quill output.qbc
```

---

## Language Tour

### Variables

Variables are declared with `let` (immutable) or `mut` (mutable). Type annotations are supported.

```quill
let x: int = 5;
let name: str = "Alice";
let pi: float = 3.14;
let active: bool = true;
mut counter: int = 0;
```

Semicolons are optional but recommended.

---

### Printing

```quill
say "Hello, world!";
printf("Hello " + name);
```

String concatenation uses `+`. Any type can be concatenated with a string.

---

### Arithmetic

```quill
let sum = 10 + 5;
let diff = 10 - 3;
let product = 6 * 7;
let quotient = 20 / 4;
let remainder = 7 % 3;
```

---

### Conditionals

No parentheses required around conditions.

```quill
let score: int = 75;

if score >= 90 {
    say "Grade: A";
} else if score >= 75 {
    say "Grade: B";
} else if score >= 60 {
    say "Grade: C";
} else {
    say "Grade: F";
}
```

Supported operators: `>` `<` `>=` `<=` `==` `!=` `&&` `||` `!`

---

### Loops

```quill
let i: int = 0;
while i < 10 {
    printf("i: " + i);
    i++;
}
```

---

### Functions

Functions must be declared before they are called. Type annotations on parameters and return type are supported.

```quill
func add(a: int, b: int): int {
    return a + b;
}

func greet(name: str): str {
    return "Hello, " + name;
}

let result: int = add(3, 4);
say greet("Alice");
```

---

### Incrementation

```quill
let i: int = 0;
i++;   # increment
i--;   # decrement
```

---

### Comments

```quill
# this is a comment
let x: int = 5; # inline comment
```

---

## Type System

Quill is statically typed. The type checker runs before bytecode compilation and reports all errors before execution.

| Type | Example |
|------|---------|
| `int` | `42` |
| `float` | `3.14` |
| `str` | `"hello"` |
| `bool` | `true` / `false` |
| `void` | functions with no return value |

### Type errors caught at compile time

```quill
let x: int = "hello";
# TypeError: cannot assign str to int — 'x'

func add(a: int, b: int): int { return a + b; }
add(1, "two");
# TypeError: argument 2 of 'add' expects int but got str
```

---

## Standard Library

| Function | Description | Returns |
|----------|-------------|---------|
| `len(s)` | Length of a string or array | `int` |
| `toString(x)` | Convert any value to string | `str` |

More stdlib functions coming in future releases.

---

## Bytecode Format

Quill source files use the `.qsc` extension. Compiled bytecode files use `.qbc`.

```bash
quill-c myfile.qsc myfile.qbc   # compile
quill myfile.qbc                 # run bytecode directly
quill myfile.qsc                 # compile and run in one step
```

The `.qbc` format is a binary format with:
- Magic header `QBC`
- Version byte for compatibility checking
- Constants pool
- Instruction stream
- Embedded function table

---

## Quick Reference

| Feature | Syntax |
|---------|--------|
| Immutable variable | `let x: int = 5;` |
| Mutable variable | `mut x: int = 5;` |
| Print literal | `say "text";` |
| Print expression | `printf("text" + var);` |
| If | `if condition { }` |
| Else if | `else if condition { }` |
| Else | `else { }` |
| While loop | `while condition { }` |
| Function def | `func name(a: type): returnType { }` |
| Function call | `name(arg);` |
| Increment | `i++;` |
| Decrement | `i--;` |
| Comment | `# text` |
| Compile | `quill-c file.qsc file.qbc` |
| Run source | `quill file.qsc` |
| Run bytecode | `quill file.qbc` |

---

## Versioning

Quill uses a 4-part versioning system: `MAJOR.MINOR.PATCH.STATUS`

| STATUS | Meaning |
|--------|---------|
| `1` | Beta |
| `2` | Meh |
| `3` | Pre-release |

Current version: **0.0.4.1**

---

## Building from Source

Requires Go 1.26+.

```bash
git clone https://github.com/your-username/interperter.git
cd interperter
./build.ps1              # Windows — builds quill.exe and quill-c.exe
./build.ps1 -Target all  # all platforms
./build.ps1 -Clean       # remove bin/
```

---

## Project Structure

```
quill-go/
├── cmd/
│   ├── quill/             # quill runner entry point
│   └── quill-c/           # quill-c compiler entry point
├── src/
│   └── core/
│       ├── ast/           # AST node definitions
│       ├── bytecode/      # bytecode format and serialization
│       ├── compiler/      # bytecode compiler
│       ├── lexer/         # tokenizer
│       ├── parser/        # AST builder
│       ├── typechecker/   # static type checking
│       ├── types/         # token types
│       └── vm/            # bytecode virtual machine
├── bin/                   # compiled binaries
├── tests/                 # .qsc test files
├── build.ps1              # build script
├── go.mod
└── README.md
```

---

## Roadmap

- [ ] Hashmaps / objects
- [ ] String methods (`split`, `trim`, `replace`, `toUpper`, `toLower`)
- [ ] Arrays (in progress)
- [ ] Error handling (`try` / `catch`)
- [ ] HTTP standard library
- [ ] File I/O
- [ ] Module / import system
- [ ] Package registry

---

## License

MIT
