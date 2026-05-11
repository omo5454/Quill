"use strict";
/**
 * quill.test.ts
 * Extensive test suite for the Quill scripting language.
 * Covers: lexer, parser, and interpreter behaviour end-to-end.
 *
 * Run with:  npx jest quill.test.ts  (or your preferred test runner)
 */
Object.defineProperty(exports, "__esModule", { value: true });
const lexer_1 = require("./core/lexer/lexer");
const parser_1 = require("./core/parser/parser");
const interpreter_1 = require("./core/interpreter/interpreter");
const types_1 = require("./core/types/types");
// ─── helpers ────────────────────────────────────────────────────────────────
function tokenize(source) {
    const lexer = new lexer_1.Lexer(source);
    const tokens = [];
    let tok = lexer.nextToken();
    while (tok.type !== types_1.TokenType.EOF) {
        tokens.push(tok);
        tok = lexer.nextToken();
    }
    return tokens;
}
function parse(source) {
    const lexer = new lexer_1.Lexer(source);
    const tokens = [];
    let tok = lexer.nextToken();
    while (tok.type !== types_1.TokenType.EOF) {
        tokens.push(tok);
        tok = lexer.nextToken();
    }
    tokens.push(tok); // push EOF
    return new parser_1.Parser(tokens).parse();
}
/** Run source and return the interpreter's final variable map. */
function run(source) {
    const ast = parse(source);
    const interpreter = new interpreter_1.Interpreter();
    return interpreter.interpret(ast);
}
/** Capture console.log output produced while running source. */
function runCapture(source) {
    const lines = [];
    const orig = console.log;
    console.log = (...args) => lines.push(args.join(" "));
    try {
        run(source);
    }
    finally {
        console.log = orig;
    }
    return lines;
}
// ─── LEXER ──────────────────────────────────────────────────────────────────
describe("Lexer", () => {
    // --- keywords ---
    describe("keywords", () => {
        test("recognises 'let'", () => {
            const toks = tokenize("let");
            expect(toks[0]).toMatchObject({ type: types_1.TokenType.Keyword, value: "let" });
        });
        test("recognises 'const'", () => {
            const toks = tokenize("const");
            expect(toks[0]).toMatchObject({
                type: types_1.TokenType.Keyword,
                value: "const",
            });
        });
        test("recognises 'printf'", () => {
            expect(tokenize("printf")[0].type).toBe(types_1.TokenType.Keyword);
        });
        test("recognises 'say'", () => {
            expect(tokenize("say")[0].type).toBe(types_1.TokenType.Keyword);
        });
        test("recognises 'if', 'else', 'while', 'func'", () => {
            const toks = tokenize("if else while func");
            expect(toks.map((t) => t.value)).toEqual(["if", "else", "while", "func"]);
        });
        test("recognises boolean literals True / False", () => {
            const toks = tokenize("True False");
            expect(toks[0]).toMatchObject({ type: types_1.TokenType.Keyword, value: "True" });
            expect(toks[1]).toMatchObject({
                type: types_1.TokenType.Keyword,
                value: "False",
            });
        });
    });
    // --- identifiers ---
    describe("identifiers", () => {
        test("simple identifier", () => {
            expect(tokenize("foo")[0]).toMatchObject({
                type: types_1.TokenType.Identifier,
                value: "foo",
            });
        });
        test("identifier with underscore", () => {
            expect(tokenize("my_var")[0]).toMatchObject({
                type: types_1.TokenType.Identifier,
                value: "my_var",
            });
        });
        test("identifier with trailing digits", () => {
            expect(tokenize("x1")[0]).toMatchObject({
                type: types_1.TokenType.Identifier,
                value: "x1",
            });
        });
    });
    // --- literals ---
    describe("number literals", () => {
        test("integer", () => {
            expect(tokenize("42")[0]).toMatchObject({
                type: types_1.TokenType.Number,
                value: "42",
            });
        });
        test("float / double", () => {
            expect(tokenize("3.14")[0]).toMatchObject({
                type: types_1.TokenType.Double,
                value: "3.14",
            });
        });
    });
    describe("string literals", () => {
        test("basic string", () => {
            expect(tokenize('"hello"')[0]).toMatchObject({
                type: types_1.TokenType.String,
                value: "hello",
            });
        });
        test("string with spaces", () => {
            expect(tokenize('"hello world"')[0]).toMatchObject({
                type: types_1.TokenType.String,
                value: "hello world",
            });
        });
        test("empty string", () => {
            expect(tokenize('""')[0]).toMatchObject({
                type: types_1.TokenType.String,
                value: "",
            });
        });
    });
    // --- operators ---
    describe("operators", () => {
        const cases = [
            ["+", "+"],
            ["-", "-"],
            ["*", "*"],
            ["/", "/"],
            ["%", "%"],
            ["=", "="],
            ["==", "=="],
            ["!=", "!="],
            [">", ">"],
            [">=", ">="],
            ["<", "<"],
            ["<=", "<="],
            ["&&", "&&"],
            ["||", "||"],
        ];
        test.each(cases)("operator %s", (src, expected) => {
            expect(tokenize(src)[0]).toMatchObject({
                type: types_1.TokenType.Operator,
                value: expected,
            });
        });
    });
    // --- incrementation ---
    describe("incrementation tokens", () => {
        test("++ produces Incrementation token", () => {
            expect(tokenize("++")[0]).toMatchObject({
                type: types_1.TokenType.Incrementation,
                value: "++",
            });
        });
        test("-- produces Incrementation token", () => {
            expect(tokenize("--")[0]).toMatchObject({
                type: types_1.TokenType.Incrementation,
                value: "--",
            });
        });
        test("i++ tokenizes identifier then incrementation", () => {
            const toks = tokenize("i++");
            expect(toks[0]).toMatchObject({ type: types_1.TokenType.Identifier, value: "i" });
            expect(toks[1]).toMatchObject({
                type: types_1.TokenType.Incrementation,
                value: "++",
            });
        });
    });
    // --- comments ---
    describe("comments", () => {
        test("# comment token", () => {
            expect(tokenize("# hello")[0]).toMatchObject({ type: types_1.TokenType.Comment });
        });
        test("comment does not bleed into next line", () => {
            const toks = tokenize("# comment\nlet x = 1");
            expect(toks[0].type).toBe(types_1.TokenType.Comment);
            expect(toks[1]).toMatchObject({ type: types_1.TokenType.Keyword, value: "let" });
        });
    });
    // --- braces / brackets ---
    describe("delimiters", () => {
        test("{ } ( ) [ ] are tokenized", () => {
            const vals = tokenize("{ } ( ) [ ]").map((t) => t.value);
            expect(vals).toEqual(["{", "}", "(", ")", "[", "]"]);
        });
    });
});
// ─── PARSER ─────────────────────────────────────────────────────────────────
describe("Parser", () => {
    test("empty program", () => {
        expect(parse("").body).toHaveLength(0);
    });
    test("variable declaration", () => {
        const ast = parse("let x = 5;");
        expect(ast.body[0]).toMatchObject({
            type: "VariableDeclaration",
            identifier: "x",
            value: { type: "Literal", value: 5 },
        });
    });
    test("variable declaration with string", () => {
        const ast = parse('let name = "Alice";');
        expect(ast.body[0]).toMatchObject({
            type: "VariableDeclaration",
            identifier: "name",
            value: { type: "String", value: "Alice" },
        });
    });
    test("variable declaration with double", () => {
        const ast = parse("let pi = 3.14;");
        expect(ast.body[0]).toMatchObject({
            type: "VariableDeclaration",
            identifier: "pi",
        });
    });
    test("print statement (say)", () => {
        const ast = parse('say "hi";');
        expect(ast.body[0]).toMatchObject({ type: "PrintStatement" });
    });
    test("print statement (printf)", () => {
        const ast = parse('printf("hello");');
        expect(ast.body[0]).toMatchObject({ type: "PrintStatement" });
    });
    test("binary expression", () => {
        const ast = parse("let z = 1 + 2;");
        const decl = ast.body[0];
        expect(decl.value).toMatchObject({
            type: "BinaryExpression",
            operator: "+",
        });
    });
    test("if statement", () => {
        const ast = parse('if True { say "yes"; }');
        expect(ast.body[0]).toMatchObject({ type: "ConditionalExpression" });
    });
    test("if / else", () => {
        const ast = parse('if True { say "yes"; } else { say "no"; }');
        const cond = ast.body[0];
        expect(cond.alternate).toBeDefined();
    });
    test("while loop", () => {
        const ast = parse("let i = 0; while i < 3 { let i = i + 1; }");
        expect(ast.body[1]).toMatchObject({ type: "LoopExpression" });
    });
    test("function declaration", () => {
        const ast = parse("func add(a, b) { let r = a + b; }");
        expect(ast.body[0]).toMatchObject({ type: "Function", name: "add" });
    });
    test("function call expression", () => {
        const ast = parse('func f(x) { say "x"; } f(1);');
        expect(ast.body[1]).toMatchObject({
            type: "CallExpression",
            callee: "f",
        });
    });
    test("standalone semicolons are ignored", () => {
        expect(parse(";;;").body).toHaveLength(0);
    });
    test("comments are skipped at top level", () => {
        expect(parse("# this is a comment\nlet x = 1;").body).toHaveLength(1);
    });
    test("throws on unexpected token", () => {
        expect(() => parse("@@@")).toThrow();
    });
});
// ─── INTERPRETER ────────────────────────────────────────────────────────────
describe("Interpreter", () => {
    // --- variables ---
    describe("variables", () => {
        test("let stores a number", () => {
            const vars = run("let x = 42;");
            expect(vars.get("x")).toBe(42);
        });
        test("let stores a string", () => {
            const vars = run('let name = "Alice";');
            expect(vars.get("name")).toBe("Alice");
        });
        test("let stores a float", () => {
            const vars = run("let pi = 3.14;");
            expect(vars.get("pi")).toBeCloseTo(3.14);
        });
        test("let reassigns a variable", () => {
            const vars = run("let x = 1; let x = 2;");
            expect(vars.get("x")).toBe(2);
        });
        test("const stores a value", () => {
            const vars = run("const y = 99;");
            expect(vars.get("y")).toBe(99);
        });
        test("True is stored as 1", () => {
            const vars = run("let t = True;");
            expect(vars.get("t")).toBe(1);
        });
        test("False is stored as 0", () => {
            const vars = run("let f = False;");
            expect(vars.get("f")).toBe(0);
        });
        test("undefined variable throws", () => {
            expect(() => run("printf(z);")).toThrow();
        });
    });
    // --- arithmetic ---
    describe("arithmetic", () => {
        test("addition", () => {
            expect(run("let r = 2 + 3;").get("r")).toBe(5);
        });
        test("subtraction", () => {
            expect(run("let r = 10 - 4;").get("r")).toBe(6);
        });
        test("multiplication", () => {
            expect(run("let r = 3 * 4;").get("r")).toBe(12);
        });
        test("division", () => {
            expect(run("let r = 10 / 2;").get("r")).toBe(5);
        });
        test("modulo", () => {
            expect(run("let r = 7 % 3;").get("r")).toBe(1);
        });
        test("chained arithmetic", () => {
            expect(run("let r = 1 + 2 + 3;").get("r")).toBe(6);
        });
        test("float arithmetic", () => {
            expect(run("let r = 1.5 + 1.5;").get("r")).toBeCloseTo(3.0);
        });
        test("variable used in arithmetic", () => {
            const vars = run("let x = 5; let y = x + 3;");
            expect(vars.get("y")).toBe(8);
        });
    });
    // --- comparison ---
    describe("comparisons", () => {
        const cases = [
            ["let r = 5 > 3;", true],
            ["let r = 3 > 5;", false],
            ["let r = 5 < 3;", false],
            ["let r = 5 >= 5;", true],
            ["let r = 5 <= 4;", false],
            ["let r = 5 == 5;", true],
            ["let r = 5 != 5;", false],
        ];
        test.each(cases)("%s", (src, expected) => {
            expect(run(src).get("r")).toBe(expected);
        });
    });
    // --- logical operators ---
    describe("logical operators", () => {
        test("&& true && true", () => {
            expect(run("let r = 1 == 1 && 2 == 2;").get("r")).toBeTruthy();
        });
        test("|| false || true", () => {
            expect(run("let r = 1 == 2 || 2 == 2;").get("r")).toBeTruthy();
        });
    });
    // --- printing ---
    describe("printing", () => {
        test("say prints a string literal", () => {
            const lines = runCapture('say "hello";');
            expect(lines).toContain("hello");
        });
        test("printf prints a variable", () => {
            const lines = runCapture('let name = "Alice"; printf("Hi " + name);');
            expect(lines[0]).toContain("Alice");
        });
        test("printf prints a number", () => {
            const lines = runCapture("let x = 7; printf(x);");
            expect(lines[0]).toContain("7");
        });
        test("multiple print statements", () => {
            const lines = runCapture('say "a"; say "b"; say "c";');
            expect(lines).toEqual(["a", "b", "c"]);
        });
    });
    // --- string concatenation ---
    describe("string concatenation", () => {
        test("two string literals", () => {
            const vars = run('let r = "hello" + " world";');
            expect(vars.get("r")).toBe("hello world");
        });
        test("string + number", () => {
            const vars = run('let r = "count: " + 5;');
            expect(vars.get("r")).toBe("count: 5");
        });
    });
    // --- conditionals ---
    describe("conditionals", () => {
        test("if true branch executes", () => {
            const vars = run("let x = 0; if True { let x = 1; }");
            expect(vars.get("x")).toBe(1);
        });
        test("if false branch skipped", () => {
            const vars = run("let x = 0; if False { let x = 1; }");
            expect(vars.get("x")).toBe(0);
        });
        test("else branch executes when condition false", () => {
            const vars = run("let x = 0; if False { let x = 1; } else { let x = 2; }");
            expect(vars.get("x")).toBe(2);
        });
        test("else branch skipped when condition true", () => {
            const vars = run("let x = 0; if True { let x = 1; } else { let x = 2; }");
            expect(vars.get("x")).toBe(1);
        });
        test("else if chain — first branch", () => {
            const vars = run(`
        let i = 15;
        let r = 0;
        if i > 10 { let r = 1; } else if i < 10 { let r = 2; }
      `);
            expect(vars.get("r")).toBe(1);
        });
        test("else if chain — second branch", () => {
            const vars = run(`
        let i = 5;
        let r = 0;
        if i > 10 { let r = 1; } else if i < 10 { let r = 2; }
      `);
            expect(vars.get("r")).toBe(2);
        });
        test("comparison in condition", () => {
            const vars = run("let x = 0; let i = 3; if i > 2 { let x = 99; }");
            expect(vars.get("x")).toBe(99);
        });
        test("nested conditionals", () => {
            const vars = run(`
        let x = 5;
        let r = 0;
        if x > 0 {
          if x > 3 {
            let r = 1;
          } else {
            let r = 2;
          }
        }
      `);
            expect(vars.get("r")).toBe(1);
        });
    });
    // --- loops ---
    describe("while loops", () => {
        test("basic counting loop", () => {
            const vars = run(`
        let i = 0;
        while i < 5 {
          let i = i + 1;
        }
      `);
            expect(vars.get("i")).toBe(5);
        });
        test("loop body runs correct number of times", () => {
            const lines = runCapture(`
        let i = 0;
        while i < 3 {
          printf(i);
          let i = i + 1;
        }
      `);
            expect(lines).toHaveLength(3);
        });
        test("loop that never runs", () => {
            const vars = run(`
        let x = 10;
        while x < 0 {
          let x = x + 1;
        }
      `);
            expect(vars.get("x")).toBe(10);
        });
        test("loop accumulator", () => {
            const vars = run(`
        let sum = 0;
        let i = 1;
        while i <= 5 {
          let sum = sum + i;
          let i = i + 1;
        }
      `);
            expect(vars.get("sum")).toBe(15);
        });
        test("nested loops", () => {
            const vars = run(`
        let count = 0;
        let i = 0;
        while i < 3 {
          let j = 0;
          while j < 3 {
            let count = count + 1;
            let j = j + 1;
          }
          let i = i + 1;
        }
      `);
            expect(vars.get("count")).toBe(9);
        });
    });
    // --- functions ---
    describe("functions", () => {
        test("function is stored in variables", () => {
            const vars = run('func greet(name) { say "hi"; }');
            expect(vars.get("greet")).toBeDefined();
        });
        test("function call executes body", () => {
            const lines = runCapture(`
        func greet(arg) {
          printf("Hello " + arg);
        }
        greet("Alice");
      `);
            expect(lines[0]).toContain("Alice");
        });
        test("function with multiple parameters", () => {
            const lines = runCapture(`
        func add(a, b) {
          printf(a + b);
        }
        add(3, 4);
      `);
            expect(lines[0]).toContain("7");
        });
        test("function called multiple times", () => {
            const lines = runCapture(`
        func hi(name) {
          printf("hi " + name);
        }
        hi("Alice");
        hi("Bob");
      `);
            expect(lines).toHaveLength(2);
            expect(lines[0]).toContain("Alice");
            expect(lines[1]).toContain("Bob");
        });
        test("function variables do not leak into outer scope", () => {
            const vars = run(`
        let x = 1;
        func setX(v) {
          let x = v;
        }
        setX(99);
      `);
            expect(vars.get("x")).toBe(1);
        });
        test("function with loop inside", () => {
            const lines = runCapture(`
        func countUp(n) {
          let i = 0;
          while i < n {
            printf(i);
            let i = i + 1;
          }
        }
        countUp(3);
      `);
            expect(lines).toHaveLength(3);
        });
        test("calling undefined function throws", () => {
            expect(() => run("foo();")).toThrow();
        });
    });
    // --- incrementation ---
    describe("incrementation (i++)", () => {
        test("postfix ++ increments variable", () => {
            const vars = run("let i = 0; i++;");
            expect(vars.get("i")).toBe(1);
        });
        test("postfix -- decrements variable", () => {
            const vars = run("let i = 5; i--;");
            expect(vars.get("i")).toBe(4);
        });
    });
    // --- standard library ---
    describe("standard library", () => {
        test("sqrt", () => {
            const vars = run("let r = sqrt(16);");
            expect(vars.get("r")).toBeCloseTo(4);
        });
        test("len on string", () => {
            const vars = run('let r = len("hello");');
            expect(vars.get("r")).toBe(5);
        });
        test("random returns a number between 0 and 1", () => {
            const vars = run("let r = random();");
            expect(vars.get("r")).toBeGreaterThanOrEqual(0);
            expect(vars.get("r")).toBeLessThan(1);
        });
        test("timeNow returns a non-empty string", () => {
            const vars = run("let r = timeNow();");
            expect(typeof vars.get("r")).toBe("string");
            expect(vars.get("r").length).toBeGreaterThan(0);
        });
        test("push adds element to array", () => {
            const vars = run(`
        let arr = push(push(push([], 1), 2), 3);
      `);
            expect(vars.get("arr")).toEqual([1, 2, 3]);
        });
    });
    // --- comments ---
    describe("comments", () => {
        test("comment does not affect execution", () => {
            const vars = run("# set x\nlet x = 5;");
            expect(vars.get("x")).toBe(5);
        });
    });
    // --- end-to-end programs ---
    describe("end-to-end programs", () => {
        test("fibonacci (iterative)", () => {
            const vars = run(`
        let a = 0;
        let b = 1;
        let i = 0;
        while i < 7 {
          let tmp = b;
          let b = a + b;
          let a = tmp;
          let i = i + 1;
        }
      `);
            expect(vars.get("a")).toBe(13); // 8th fibonacci number
        });
        test("factorial via loop", () => {
            const vars = run(`
        let n = 5;
        let result = 1;
        while n > 1 {
          let result = result * n;
          let n = n - 1;
        }
      `);
            expect(vars.get("result")).toBe(120);
        });
        test("readme greet example", () => {
            const lines = runCapture(`
        func greet(arg) {
          printf("Hello " + arg);
        }
        let name = "Alice";
        greet(name);
      `);
            expect(lines[0]).toBe("Hello Alice");
        });
        test("readme while loop example", () => {
            const lines = runCapture(`
        let i = 0;
        while i < 10 {
          printf("i: " + i);
          let i = i + 1;
        }
      `);
            expect(lines).toHaveLength(10);
            expect(lines[0]).toBe("i: 0");
            expect(lines[9]).toBe("i: 9");
        });
        test("readme conditional example", () => {
            const lines = runCapture(`
        let i = 15;
        if i > 10 {
          say "i is greater than 10";
        } else if i < 10 {
          say "i is less than 10";
        }
      `);
            expect(lines[0]).toBe("i is greater than 10");
        });
        test("variable reassignment across multiple lets", () => {
            const vars = run(`
        let name = "Alice";
        let name = "Bob";
      `);
            expect(vars.get("name")).toBe("Bob");
        });
        test("function that uses outer stdlib", () => {
            const vars = run(`
        func hypotenuse(a, b) {
          let r = sqrt(a * a + b * b);
        }
        hypotenuse(3, 4);
      `);
            // stdlib available inside function
            expect(() => run(`
        func hypotenuse(a, b) {
          let r = sqrt(a * a + b * b);
        }
        hypotenuse(3, 4);
      `)).not.toThrow();
        });
    });
});
