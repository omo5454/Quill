"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Parser = void 0;
const types_1 = require("../types/types");
class Parser {
    constructor(tokens) {
        this.current = 0;
        this.tokens = tokens;
    }
    peek() {
        return this.tokens[this.current] || { type: types_1.TokenType.EOF, value: "" };
    }
    advance() {
        return this.tokens[this.current++];
    }
    isAtEnd() {
        return this.peek().type === types_1.TokenType.EOF;
    }
    parse() {
        const program = { type: "Program", body: [] };
        while (!this.isAtEnd()) {
            const token = this.peek();
            if (token.value === ";" || token.type === types_1.TokenType.Comment) {
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
            switch (token.value) {
                case "let":
                case "const":
                    return this.parseVariableDeclaration();
                case "printf":
                    return this.parsePrintStatement();
                case "func":
                    return this.parseFunctionDeclaration();
                case "if":
                    return this.parseConditionalExpression();
                case "while":
                    return this.parseLoopExpression();
            }
        }
        // Expression statements (function calls, identifiers, arrays, etc.)
        return this.parseExpressionStatement();
    }
    parseExpressionStatement() {
        const expr = this.parseExpression();
        if (this.peek().value === ";")
            this.advance();
        return expr;
    }
    parseVariableDeclaration() {
        this.advance(); // consume let/const
        const id = this.advance();
        if (id.type !== types_1.TokenType.Identifier)
            throw new Error("Expected identifier");
        if (this.peek().value !== "=")
            throw new Error("Expected '='");
        this.advance();
        const value = this.parseExpression();
        if (this.peek().value === ";")
            this.advance();
        return { type: "VariableDeclaration", identifier: id.value, value };
    }
    parsePrintStatement() {
        this.advance(); // printf
        const expression = this.parseExpression();
        if (this.peek().value === ";")
            this.advance();
        return { type: "PrintStatement", expression };
    }
    parseFunctionDeclaration() {
        this.advance(); // func
        const nameToken = this.advance();
        if (nameToken.type !== types_1.TokenType.Identifier)
            throw new Error("Expected function name");
        if (this.peek().value !== "(")
            throw new Error("Expected '('");
        this.advance();
        const parameters = [];
        while (this.peek().value !== ")") {
            const param = this.advance();
            if (param.type === types_1.TokenType.Identifier)
                parameters.push(param.value);
            if (this.peek().value === ",")
                this.advance();
        }
        this.advance(); // )
        if (this.peek().value !== "{")
            throw new Error("Expected '{'");
        this.advance();
        const body = [];
        while (this.peek().value !== "}") {
            body.push(this.parseStatement());
        }
        this.advance(); // }
        return { type: "Function", name: nameToken.value, parameters, body };
    }
    parseConditionalExpression() {
        this.advance(); // if
        const test = this.parseExpression();
        if (this.peek().value !== "{")
            throw new Error("Expected '{' after condition");
        this.advance();
        const consequent = [];
        while (this.peek().value !== "}") {
            consequent.push(this.parseStatement());
        }
        this.advance();
        let alternate = undefined;
        if (this.peek().value === "else") {
            this.advance();
            if (this.peek().value === "{") {
                this.advance();
                const elseBody = [];
                while (this.peek().value !== "}") {
                    elseBody.push(this.parseStatement());
                }
                this.advance();
                alternate = elseBody;
            }
            else if (this.peek().value === "if") {
                alternate = this.parseConditionalExpression();
            }
        }
        return { type: "ConditionalExpression", test, consequent, alternate };
    }
    parseLoopExpression() {
        this.advance(); // while
        const test = this.parseExpression();
        if (this.peek().value !== "{")
            throw new Error("Expected '{' after while condition");
        this.advance();
        const body = [];
        while (this.peek().value !== "}") {
            body.push(this.parseStatement());
        }
        this.advance();
        return { type: "LoopExpression", test, body };
    }
    parseExpression() {
        let left = this.parsePrimary();
        // Handle array indexing: list[0]
        while (this.peek().value === "[") {
            this.advance();
            const index = this.parseExpression();
            if (this.peek().value !== "]")
                throw new Error("Expected ']'");
            this.advance();
            left = { type: "IndexExpression", object: left, index };
        }
        // Binary operators
        while (this.isBinaryOperator(this.peek())) {
            const operator = this.advance().value;
            const right = this.parseExpression(); // Changed to parseExpression for better precedence
            left = { type: "BinaryExpression", left, operator, right };
        }
        return left;
    }
    parsePrimary() {
        const token = this.advance();
        switch (token.type) {
            case types_1.TokenType.Number:
                return { type: "Literal", value: Number(token.value) };
            case types_1.TokenType.String:
                return { type: "String", value: token.value };
            case types_1.TokenType.Identifier:
                if (this.peek().value === "(") {
                    return this.parseCallExpression(token.value);
                }
                return { type: "Identifier", name: token.value };
            case types_1.TokenType.Keyword:
                if (token.value === "true")
                    return { type: "BooleanLiteral", value: true };
                if (token.value === "false")
                    return { type: "BooleanLiteral", value: false };
                break;
        }
        if (token.value === "[") {
            return this.parseArrayLiteral();
        }
        if (token.value === "(") {
            const expr = this.parseExpression();
            if (this.peek().value !== ")")
                throw new Error("Expected ')'");
            this.advance();
            return expr;
        }
        throw new Error(`Unexpected token in primary: ${token.value}`);
    }
    parseCallExpression(callee) {
        this.advance(); // consume "("
        const args = [];
        while (this.peek().value !== ")") {
            args.push(this.parseExpression());
            if (this.peek().value === ",")
                this.advance();
        }
        this.advance(); // consume ")"
        return { type: "CallExpression", callee, arguments: args };
    }
    parseArrayLiteral() {
        const elements = [];
        while (this.peek().value !== "]") {
            elements.push(this.parseExpression());
            if (this.peek().value === ",")
                this.advance();
        }
        this.advance(); // consume "]"
        return { type: "ArrayLiteral", elements };
    }
    isBinaryOperator(token) {
        return token.type === types_1.TokenType.Operator &&
            ["+", "-", "*", "/", "%", "==", "!=", ">", "<", ">=", "<=", "&&", "||"].includes(token.value);
    }
}
exports.Parser = Parser;
