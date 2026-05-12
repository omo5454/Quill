import * as fs from 'fs';
import * as path from 'path';
import { Lexer } from './core/lexer/lexer';
import { Parser } from './core/parser/parser';
import { Interpreter } from './core/interpreter/interpreter';
// 1. Get the file path from command line arguments (e.g., node quill.js test.quill)
const arg = process.argv[2];
var version = "0.0.3.3\nMajor: 0\nMinor: 0\nBug/Fix: 3\nStatus: Pre-Release";
if (!arg) {
    console.error("Usage: quill <filename.quill>");
    process.exit(1);
}
if (arg === "-v" || arg === "--version") {
    console.log(version);
    process.exit(0);
}
// 2. Ensure it has the .quill extension
if (path.extname(arg) !== '.quill') {
    console.error("Error: Only .quill files are supported.");
    process.exit(1);
}
try {
    // 3. Read the file content
    const sourceCode = fs.readFileSync(arg, 'utf-8');
    // 4. Run the Pipeline
    const lexer = new Lexer(sourceCode);
    const tokens = [];
    let token = lexer.nextToken();
    while (token.type !== "EOF") {
        tokens.push(token);
        token = lexer.nextToken();
    }
    const parser = new Parser(tokens);
    const ast = parser.parse();
    const interpreter = new Interpreter();
    interpreter.interpret(ast);
}
catch (err) {
    console.error(`Runtime Error: ${err.message}`);
}
