"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const lexer_1 = require("../core/lexer/lexer");
const parser_1 = require("../core/parser/parser");
const interpreter_1 = require("../core/interpreter/interpreter"); // Using your renamed class
const input = "let x = 100; printf x + 50;";
// 1. Lexer: String -> Tokens
const lexer = new lexer_1.Lexer(input);
const tokens = [];
let token = lexer.nextToken();
while (token.type !== "EOF") {
    tokens.push(token);
    token = lexer.nextToken();
}
// 2. Parser: Tokens -> AST
const parser = new parser_1.Parser(tokens);
const ast = parser.parse(); // This creates the 'Program' object
// 3. Interpreter: AST -> Execution
const interpreter = new interpreter_1.Interpreter();
const memory = interpreter.interpret(ast);
console.log("Memory State:", memory);
