# Quill
---

## Why Quill?

- **Statically Typed** — Type errors are caught early at compile time before execution.
- **Blazing Fast** — Powered by a C++ transpilation pipeline (giving you near-native performance).
- **Simple & Clean Syntax** — Readable syntax with optional semicolons and zero clutter.
- **Module-First** — Built around writing, sharing, and importing function libraries directly from GitHub.
- **Cross-Platform** — Native executable support for **Linux** and **Windows**.

---

## Installation

Download the pre-compiled binary for your platform from the [Releases](https://github.com/omrimorgan5-hub/Quill/releases) page.

### Windows
Install the quill-windows[32 or 64].exe and run it in powershell.

### Linux
Make the binary executable and run:
```bash
chmod +x quill-linux
./quill-linux myfile.qsc

```

> **Note on macOS:** Support for macOS is currently paused to ensure maximum stability on core Linux and Windows platforms.

---

## Usage

```bash
# Transpiling to C++ and compiling to a native binary
quill-[os] --compile myfile.qsc

# Using the standalone transpiler CLI tool directly
quill-[os] myfile.qsc -o output.c
```

---

## Language Tour

### Variables

Variables are declared with `let`. Type annotations are supported.

```quill
let x: int = 5;
let name: str = "Alice";
let pi: float = 3.14;
let active: bool = true;
let counter: int = 0;

```

### Printing

```quill
say "Hello, world!";
printf("Hello " + name);

```

### Arithmetic & Incrementing

```quill
let sum = 10 + 5;
let product = 6 * 7;

let i: int = 0;
i++;   # increment
i--;   # decrement

```

### Control Flow

Conditions do not require parentheses.

```quill
let score: int = 75;

if score >= 90 {
    say "Grade: A";
} else if score >= 75 {
    say "Grade: B";
} else {
    say "Grade: F";
}

let x: int = 0;
while x < 5 {
    printf("x: " + x);
    x++;
}

```

### Functions

Functions must be declared before they are called.

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

## Type System

Quill runs static type checking before generating bytecode or C++ output.

| Type | Example |
| --- | --- |
| `int` | `42` |
| `float` | `3.14` |
| `str` | `"hello"` |
| `bool` | `true` / `false` |
| `void` | Functions with no return value |

---

## Building from Source

Requires **C++17** or higher.

### Windows (PowerShell)
Currently has no helper file due to complexity.

### Linux

```bash
git clone [https://github.com/omrimorgan5-hub/Quill.git](https://github.com/omrimorgan5-hub/Quill.git)
cd Quill
./build.sh

```

---

## Project Structure

```
Quill/
├── src/
│   └── core/
│       ├── ast/               # AST node definitions
│       ├── lexer/             # Tokenizer
│       ├── parser/            # AST parser
│       ├── transpiler/        # Quill -> C Transpiler also acts as main CLI entrypoint
│       └── typechecker/       # Static type checker
├── bin/                       # Output directory for compiled binaries
├── build.sh                   # Linux build script
└── README.md

```

---

## Roadmap

* [ ] Arrays and slice operations
* [ ] Hashmaps / Objects
* [ ] Extended String utilities (`split`, `trim`, `replace`)
* [ ] Native File I/O
* [ ] Structured Error Handling (`try` / `catch`)
* [x] Package registry (GitHub repo integration)
* [x] C++ Transpiler pipeline backend rewrite

---

## License

Distributed under the **MIT License**.