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
            program.body.push(this.parseStatement());
        }
        return program;
    }
    parseStatement() {
        const token = this.peek();
        if (token.type === types_1.TokenType.Keyword && token.value === "let") {
            return this.parseVariableDeclaration();
        }
        if (token.type === types_1.TokenType.Keyword && token.value === "printf") {
            return this.parsePrintStatement();
        }
        throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
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
            return { type: "Identifier", name: token.value };
        }
        throw new Error(`Expected number or variable, but got: ${token.value}`);
    }
}
exports.Parser = Parser;
