"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Lexer = void 0;
// lexer.ts
const types_1 = require("../types/types");
class Lexer {
    constructor(input) {
        this.input = input;
        this.position = 0;
        this.char = "";
        this.readChar();
    }
    readChar() {
        this.char = this.position >= this.input.length ? null : this.input[this.position];
        this.position++;
    }
    skipWhitespace() {
        while (this.char && /\s/.test(this.char)) {
            this.readChar();
        }
    }
    nextToken() {
        this.skipWhitespace();
        if (this.char === null) {
            return { type: types_1.TokenType.EOF, value: "" };
        }
        // Single character operators / symbols
        switch (this.char) {
            case "+":
            case "-":
            case "*":
            case "/":
            case "%":
            case "(":
            case ")":
            case "[":
            case "]":
            case "{":
            case "}":
            case ";":
                const op = this.char;
                this.readChar();
                return { type: types_1.TokenType.Operator, value: op };
            case "#":
                const comment = this.readComment();
                return { type: types_1.TokenType.Comment, value: comment };
            case '"':
                return this.readString();
            case "=":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: "==" };
                }
                return { type: types_1.TokenType.Operator, value: "=" };
            case "!":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: "!=" };
                }
                return { type: types_1.TokenType.Operator, value: "!" };
            case ">":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: ">=" };
                }
                return { type: types_1.TokenType.Operator, value: ">" };
            case "<":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: "<=" };
                }
                return { type: types_1.TokenType.Operator, value: "<" };
            case "&":
                this.readChar();
                if (this.char === "&") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: "&&" };
                }
                return { type: types_1.TokenType.Illegal, value: "&" };
            case "|":
                this.readChar();
                if (this.char === "|") {
                    this.readChar();
                    return { type: types_1.TokenType.Operator, value: "||" };
                }
                return { type: types_1.TokenType.Illegal, value: "|" };
            default:
                if (this.isLetter(this.char)) {
                    return this.readIdentifierOrKeyword();
                }
                if (this.isDigit(this.char)) {
                    return this.readNumber();
                }
                const illegal = this.char;
                this.readChar();
                return { type: types_1.TokenType.Illegal, value: illegal };
        }
    }
    readString() {
        this.readChar(); // consume opening "
        let value = "";
        while (this.char !== null && this.char !== '"') {
            if (this.char === '\\') {
                this.readChar(); // skip escape char for now
            }
            value += this.char;
            this.readChar();
        }
        this.readChar(); // consume closing "
        return { type: types_1.TokenType.String, value };
    }
    readIdentifierOrKeyword() {
        let literal = "";
        while (this.char && /[a-zA-Z_]/.test(this.char)) {
            literal += this.char;
            this.readChar();
        }
        const keywords = ["let", "const", "printf", "if", "else", "while", "func", "true", "false"];
        return {
            type: keywords.includes(literal) ? types_1.TokenType.Keyword : types_1.TokenType.Identifier,
            value: literal
        };
    }
    readNumber() {
        let value = "";
        while (this.char && /[0-9]/.test(this.char)) {
            value += this.char;
            this.readChar();
        }
        return { type: types_1.TokenType.Number, value };
    }
    readComment() {
        let comment = "";
        while (this.char !== null && this.char !== "\n") {
            comment += this.char;
            this.readChar();
        }
        return comment.trim();
    }
    isLetter(char) {
        return !!char && /[a-zA-Z_]/.test(char);
    }
    isDigit(char) {
        return !!char && /[0-9]/.test(char);
    }
}
exports.Lexer = Lexer;
