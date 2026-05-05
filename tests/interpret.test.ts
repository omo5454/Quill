import { Lexer } from "../core/lexer/lexer";
import { Parser } from "../core/parser/parser";
import { Interpreter } from "../core/interpreter/interpreter"; // Using your renamed class

const input = "let x = 100; printf x + 50;";

// 1. Lexer: String -> Tokens
const lexer = new Lexer(input);
const tokens = [];
let token = lexer.nextToken();
while (token.type !== "EOF") {
  tokens.push(token);
  token = lexer.nextToken();
}

// 2. Parser: Tokens -> AST
const parser = new Parser(tokens);
const ast = parser.parse(); // This creates the 'Program' object

// 3. Interpreter: AST -> Execution
const interpreter = new Interpreter();
const memory = interpreter.interpret(ast);

console.log("Memory State:", memory);
