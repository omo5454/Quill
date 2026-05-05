"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Parser = void 0;
const types_1 = require("../types/types");
class Parser {
    constructor(tokens) {
        this.current = 0;
        this.tokens = tokens;
    }
    // Helper to look at current token without consuming it
    peek() {
        return this.tokens[this.current] || { type: types_1.TokenType.EOF, value: "" };
    }
    // Helper to consume current token and move to next
    advance() {
        return this.tokens[this.current++];
    }
    // Helper to check if we've reached the end
    isAtEnd() {
        return this.peek().type === types_1.TokenType.EOF;
    }
    parse() {
        const program = { type: "Program", body: [] };
        while (!this.isAtEnd()) {
            // Ignore stand-alone semicolons between statements
            if (this.peek().value === ";") {
                this.advance();
                continue;
            }
            // Ignore comments at the top level; they are not part of the statement AST.
            if (this.peek().type === types_1.TokenType.Comment) {
                this.advance();
                continue;
            }
            program.body.push(this.parseStatement());
        }
        return program;
    }
    parseStatement() {
        const token = this.peek();
        if (token.type === types_1.TokenType.Keyword) {
            if (token.value === "let" || token.value === "const")
                return this.parseVariableDeclaration();
            if (token.value === "printf")
                return this.parsePrintStatement();
            if (token.value === "func")
                return this.parseFunctionDeclaration();
        }
        if (token.type === types_1.TokenType.Identifier) {
            return this.parseExpressionStatement();
        }
        throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
    }
    parseExpressionStatement() {
        const expr = this.parseExpression();
        if (this.peek().value === ";")
            this.advance(); // consume ";"
        return expr; // In a simple AST, you can just return the expression node
    }
    parseVariableDeclaration() {
        this.advance(); // consume "let"
        const idToken = this.advance(); // get identifier (x)
        if (idToken.type !== types_1.TokenType.Identifier) {
            throw new Error("Expected variable name after 'let'");
        }
        if (this.peek().value !== "=") {
            throw new Error("Expected '=' after variable name");
        }
        this.advance(); // consume "="
        const val = this.parseExpression();
        // Optional semicolon consumption
        if (!this.isAtEnd() && this.peek().value === ";") {
            this.advance();
        }
        return {
            type: "VariableDeclaration",
            identifier: idToken.value,
            value: val
        };
    }
    parsePrintStatement() {
        this.advance(); // consume "printf"
        const val = this.parseExpression();
        if (!this.isAtEnd() && this.peek().value === ";") {
            this.advance();
        }
        return { type: "PrintStatement", expression: val };
    }
    parseComment() {
        const token = this.advance(); // consume the comment token
        return { type: "Comment", value: token.value };
    }
    parseFunctionDeclaration() {
        this.advance(); // consume "func"
        const nameToken = this.advance(); // get function name
        if (nameToken.type !== types_1.TokenType.Identifier) {
            throw new Error("Expected function name after 'func'");
        }
        if (this.peek().value !== "(") {
            throw new Error("Expected '(' after function name");
        }
        this.advance(); // consume "("
        const parameters = [];
        while (!this.isAtEnd() && this.peek().value !== ")") {
            const paramToken = this.advance();
            if (paramToken.type !== types_1.TokenType.Identifier) {
                throw new Error("Expected parameter name in function declaration");
            }
            parameters.push(paramToken.value);
            if (this.peek().value === ",") {
                this.advance(); // consume ","
            }
        }
        this.advance(); // consume ")"
        const body = [];
        if (this.peek().value === "{") {
            this.advance();
            while (!this.isAtEnd() && this.peek().value !== "}") {
                body.push(this.parseStatement());
            }
            if (this.peek().value !== "}") {
                throw new Error("Expected '}' at end of function body");
            }
            this.advance(); // consume "}"
        }
        else {
            throw new Error("Expected '{' to start function body");
        }
        return {
            type: "Function",
            name: nameToken.value,
            parameters,
            body
        };
    }
    parseExpression() {
        let left = this.parsePrimary();
        // ONLY continue if the next token is actually an Operator (+, -, *, /)
        // If it sees "printf" (a Keyword), this loop will now skip and return 'left'
        while (!this.isAtEnd() &&
            this.peek().type === types_1.TokenType.Operator &&
            this.peek().value !== ";") {
            const operator = this.advance().value;
            const right = this.parsePrimary();
            left = { type: "BinaryExpression", left, operator, right };
        }
        return left;
    }
    // parsePrimary handles the "smallest" units: numbers and variables
    parsePrimary() {
        const token = this.advance();
        if (token.type === types_1.TokenType.Number) {
            return { type: "Literal", value: Number(token.value) };
        }
        if (token.type === types_1.TokenType.Identifier) {
            if (this.peek().value === "(") {
                // This is a function call, not just a variable reference
                const funcName = token.value;
                this.advance(); // consume "("
                const args = [];
                while (!this.isAtEnd() && this.peek().value !== ")") {
                    args.push(this.parseExpression());
                    if (this.peek().value === ",") {
                        this.advance(); // consume ","
                    }
                }
                this.advance(); // consume ")"
                return { type: "CallExpression", callee: token.value, arguments: args };
            }
            return { type: "Identifier", name: token.value };
        }
        throw new Error(`Expected number or variable, but got: ${token.value}`);
    }
}
exports.Parser = Parser;
