"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const lexer_1 = require("./core/lexer/lexer");
const parser_1 = require("./core/parser/parser");
const interpreter_1 = require("./core/interpreter/interpreter");
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
    const lexer = new lexer_1.Lexer(sourceCode);
    const tokens = [];
    let token = lexer.nextToken();
    while (token.type !== "EOF") {
        tokens.push(token);
        token = lexer.nextToken();
    }
    const parser = new parser_1.Parser(tokens);
    const ast = parser.parse();
    const interpreter = new interpreter_1.Interpreter();
    interpreter.interpret(ast);
}
catch (err) {
    console.error(`Runtime Error: ${err.message}`);
}
