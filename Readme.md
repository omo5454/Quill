# Quill
Quill is a simple, readable scripting language with a clean syntax designed for clarity. It supports variables, functions, conditionals, loops, incrementation, and a standard library.

---

## Table of Contents
- [Installation](#installation)
- [Variables](#variables)
- [Printing](#printing)
- [Conditionals](#conditionals)
- [Loops](#loops)
- [Functions](#functions)
- [Incrementation](#incrementation)
- [Standard Library](#standard-library)
- [Comments](#comments)
- [Versioning](#versioning)

---

## Installation
Download the latest release and run `QuillInstaller.exe`. During installation you can opt in to adding Quill to your system PATH, which lets you run `.quill` files from any terminal:

```bash
quill myfile.quill
```

---

## Variables
Variables are declared and reassigned using the `let` keyword. There is no separate reassignment keyword — `let` handles both.

```quill
let name = "Alice";
let name = "Bob";
```

Quill supports the following value types:

- **Integers** — `let x = 10;`
- **Floats** — `let pi = 3.14;`
- **Strings** — `let name = "Alice";`
- **Booleans** — `let t = True;` / `let f = False;`

Semicolons are optional but recommended for readability.

---

## Printing
Quill has two ways to print output:

**`say`** — Used for printing plain string literals.
```quill
say "Hello, world!";
```

**`printf`** — Used when you want to include a variable or expression in the output. The content is enclosed in parentheses.
```quill
let name = "Alice";
printf("Hello " + name);
```

String concatenation is done with the `+` operator.

---

## Conditionals
Quill supports `if` / `else if` / `else` blocks. The condition is written directly after the keyword with no parentheses required.

```quill
let i = 15;
if i > 10 {
    say "i is greater than 10";
} else if i < 10 {
    say "i is less than 10";
} else {
    say "i is exactly 10";
}
```

Supported comparison operators: `>` `<` `>=` `<=` `==` `!=`

Logical operators `&&` and `||` are also supported in conditions:

```quill
let sunny = True;
let warm = True;

if sunny == 1 && warm == 1 {
    say "Great day for a walk";
}
```

---

## Loops
Quill supports `while` loops. The loop runs as long as the condition is true.

```quill
let i = 0;
while i < 10 {
    printf("i: " + i);
    let i = i + 1;
}
```

---

## Functions
Functions are defined with the `func` keyword, followed by the function name and its parameters in parentheses. They are called by name with arguments passed in parentheses.

```quill
func greet(arg) {
    printf("Hello " + arg);
}

let name = "Alice";
greet(name);
```

Functions can accept multiple parameters:

```quill
func add(a, b) {
    printf(a + b);
}

add(3, 4);
```

> **Note:** Variables declared inside a function do not leak into the outer scope.

---

## Incrementation
Quill supports `++` and `--` postfix operators for incrementing and decrementing variables.

```quill
let i = 0;
i++;
i++;
printf(i); # 2

i--;
printf(i); # 1
```

You can also increment manually:

```quill
let i = i + 1;
```

---

## Standard Library
Quill ships with a small set of built-in functions:

| Function | Description | Example |
|----------|-------------|---------|
| `sqrt(n)` | Square root of n | `sqrt(144)` → `12` |
| `len(s)` | Length of a string | `len("hello")` → `5` |
| `random()` | Random float between 0 and 1 | `random()` |
| `timeNow()` | Current date and time as a string | `timeNow()` |
| `push(arr, val)` | Appends a value to an array | `push(arr, 1)` |

```quill
let root = sqrt(144);
printf("sqrt(144) = " + root);

let size = len("Quill");
printf("length = " + size);

let now = timeNow();
printf("time: " + now);
```

---

## Comments
Comments start with `#` and run to the end of the line. They are ignored during execution.

```quill
# This is a comment
let x = 5; # inline comment
```

---

## Quick Reference
| Feature | Syntax |
|---------|--------|
| Variable | `let x = value;` |
| Print literal | `say "text";` |
| Print with var | `printf("text" + var);` |
| If | `if condition { }` |
| Else if | `else if condition { }` |
| Else | `else { }` |
| While loop | `while condition { }` |
| Function def | `func name(arg) { }` |
| Function call | `name(arg);` |
| Increment | `i++;` |
| Decrement | `i--;` |
| Comment | `# text` |
| Square root | `sqrt(n)` |
| String length | `len(s)` |
| Random | `random()` |
| Time | `timeNow()` |

---

## Versioning
Quill uses a 4-part versioning system: `MAJOR.MINOR.PATCH.STATUS`

| STATUS | Meaning |
|--------|---------|
| `1` | Beta |
| `2` | Alomost |
| `3` | Pre-release |

Current version: **0.0.3.1**