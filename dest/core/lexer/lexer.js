import { TokenType } from "../types/types";
export class Lexer {
    peekChar() {
        if (this.position >= this.input.length) {
            return ""; // Return a null or EOF marker if at the end
        }
        else {
            return this.input[this.position];
        }
    }
    constructor(input) {
        this.input = input;
        this.position = 0;
        this.char = "";
        this.readChar(); // Initialize the first character
    }
    readChar() {
        if (this.position >= this.input.length) {
            this.char = null;
        }
        else {
            this.char = this.input[this.position];
        }
        this.position++;
    }
    skipWhitespace() {
        while (this.char && /\s/.test(this.char)) {
            this.readChar();
        }
    }
    nextToken() {
        this.skipWhitespace();
        // Check for end of file first
        if (this.char === null) {
            return { type: TokenType.EOF, value: "" };
        }
        // Handle symbols and operators
        switch (this.char) {
            case "+":
                this.readChar();
                if (this.char === "+") {
                    this.readChar();
                    return { type: TokenType.Incrementation, value: "++" };
                }
                return { type: TokenType.Operator, value: "+" };
            case "-":
                this.readChar();
                if (this.char === "-") {
                    this.readChar();
                    return { type: TokenType.Incrementation, value: "--" };
                }
                return { type: TokenType.Operator, value: "-" };
            case "*":
                this.readChar();
                return { type: TokenType.Operator, value: "*" };
            case "/":
                this.readChar();
                return { type: TokenType.Operator, value: "/" };
            case "#":
                const comment = this.readComment();
                return { type: TokenType.Comment, value: comment };
            case ";":
                this.readChar();
                return { type: TokenType.Operator, value: ";" };
            case '"':
                this.readChar(); // Consume the opening quote
                let strValue = "";
                while (this.char !== null && this.char !== '"') {
                    if (this.char === "\\") {
                        // Handle backslash
                        this.readChar();
                        // Optional: Add logic to handle \n, \t, etc.
                        strValue += this.char;
                    }
                    else {
                        strValue += this.char;
                    }
                    this.readChar();
                }
                this.readChar(); // Consume the closing quote
                return { type: TokenType.String, value: strValue };
            case ".":
                this.readChar(); // now this.char is the char after "."
                if (this.isDigit(this.char)) { // check current char, not peekChar
                    const doubleValue = "." + this.readNumber();
                    return { type: TokenType.Double, value: doubleValue };
                }
                return { type: TokenType.Dot, value: "." };
            case ">":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: TokenType.Operator, value: ">=" };
                }
                return { type: TokenType.Operator, value: ">" };
            case "<":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: TokenType.Operator, value: "<=" };
                }
                return { type: TokenType.Operator, value: "<" };
            case "!":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: TokenType.Operator, value: "!=" };
                }
                return { type: TokenType.Operator, value: "!" };
            case "=":
                this.readChar();
                if (this.char === "=") {
                    this.readChar();
                    return { type: TokenType.Operator, value: "==" };
                }
                return { type: TokenType.Operator, value: "=" };
            case "&":
                this.readChar();
                if (this.char === "&") {
                    this.readChar();
                    return { type: TokenType.Operator, value: "&&" };
                }
                return { type: TokenType.Illegal, value: "&" };
            case "|":
                this.readChar();
                if (this.char === "|") {
                    this.readChar();
                    return { type: TokenType.Operator, value: "||" };
                }
                return { type: TokenType.Illegal, value: "|" };
            case "%":
                this.readChar();
                return { type: TokenType.Operator, value: "%" };
            case "[":
                this.readChar();
                return { type: TokenType.Operator, value: "[" };
            case "]":
                this.readChar();
                return { type: TokenType.Operator, value: "]" };
            case "(":
                this.readChar();
                return { type: TokenType.Operator, value: "(" };
            case ")":
                this.readChar();
                return { type: TokenType.Operator, value: ")" };
            case "{":
                this.readChar();
                return { type: TokenType.Operator, value: "{" };
            case "}":
                this.readChar();
                return { type: TokenType.Operator, value: "}" };
            default:
                // Handle words (Keywords and Identifiers)
                if (this.isLetter(this.char)) {
                    const literal = this.readIdentifier();
                    const keywords = [
                        "let",
                        "printf",
                        "say",
                        "if",
                        "const",
                        "func",
                        "True",
                        "False",
                        "else",
                        "while",
                        ">",
                        "<",
                        ">=",
                        "<=",
                        "==",
                        "!=",
                        "&&",
                        "||",
                        "!",
                    ];
                    // Check if word is a Keyword or Identifier
                    const type = keywords.includes(literal)
                        ? TokenType.Keyword
                        : TokenType.Identifier;
                    return { type, value: literal };
                }
                // Handle Numbers
                if (this.isDigit(this.char)) {
                    const num = this.readNumber();
                    if (num.includes(".")) {
                        return { type: TokenType.Double, value: num };
                    }
                    return { type: TokenType.Number, value: num };
                }
                // If we don't recognize it, it's an illegal character
                const illegalChar = this.char || "";
                this.readChar();
                return { type: TokenType.Illegal, value: illegalChar };
        }
    }
    isLetter(char) {
        return !!char && /[a-zA-Z_]/.test(char);
    }
    isDigit(char) {
        return !!char && /[0-9]/.test(char);
    }
    isInteger(char) {
        return !!char && /[0-9]/.test(char);
    }
    readIdentifier() {
        let literal = "";
        while (this.isLetter(this.char) || (literal.length > 0 && this.isDigit(this.char))) {
            literal += this.char;
            this.readChar();
        }
        return literal;
    }
    readComment() {
        let comment = "";
        while (this.char !== null && this.char !== "\n") {
            comment += this.char;
            this.readChar();
        }
        return comment.trim();
    }
    readNumber() {
        let literal = "";
        while (this.isDigit(this.char) || this.char === ".") {
            literal += this.char;
            this.readChar();
        }
        return literal;
    }
}
