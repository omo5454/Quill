import * as fs from 'fs';
import * as path from 'path';
import { Lexer } from './core/lexer/lexer';
import { Parser } from './core/parser/parser';
import { Interpreter } from './core/interpreter/interpreter';

// 1. Get the file path from command line arguments (e.g., node quill.js test.quill)
const filePath = process.argv[2];

if (!filePath) {
    console.error("Usage: quill <filename.quill>");
    process.exit(1);
}

// 2. Ensure it has the .quill extension
if (path.extname(filePath) !== '.quill') {
    console.error("Error: Only .quill files are supported.");
    process.exit(1);
}

try {
    // 3. Read the file content
    const sourceCode = fs.readFileSync(filePath, 'utf-8');

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

} catch (err: any) {
    console.error(`Runtime Error: ${err.message}`);
}
