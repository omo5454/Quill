# Quill

Quill is a simple, readable scripting language with a clean syntax designed for clarity. It supports variables, functions, conditionals, loops, and a standard library.

---

## Table of Contents

- [Variables](#variables)
- [Printing](#printing)
- [Conditionals](#conditionals)
- [Loops](#loops)
- [Functions](#functions)

---

## Variables

Variables are declared and reassigned using the `let` keyword. There is no separate reassignment keyword — `let` handles both.

```quill
let name = "Alice";
let name = "Bob";
```

Semicolons are optional but recommended for readability.

---

## Printing

Quill has two ways to print output:

**`say`** — Used for printing plain string literals.

```quill
say "Hello, world!";
```

**`printf`** — Used when you want to include a variable in the output. The content is enclosed in parentheses.

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

> **Note:** There is no `i++` shorthand yet. Increment variables manually with `let i = i + 1`.

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

---

## Quick Reference

| Feature       | Syntax                          |
|---------------|---------------------------------|
| Variable      | `let x = value;`                |
| Print literal | `say "text";`                   |
| Print with var| `printf("text" + var);`         |
| If            | `if condition { }`              |
| Else if       | `else if condition { }`         |
| While loop    | `while condition { }`           |
| Function def  | `func name(arg) { }`            |
| Function call | `name(arg);`                    |
| Increment     | `let i = i + 1;`                |