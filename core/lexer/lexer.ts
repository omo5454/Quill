import { TokenType, Token } from "../types/types";

export class Lexer {
  private position = 0;
  private char: string | null = "";

  constructor(private input: string) {
    this.readChar(); // Initialize the first character
  }

  private readChar() {
    if (this.position >= this.input.length) {
      this.char = null;
    } else {
      this.char = this.input[this.position];
    }
    this.position++;
  }

  private skipWhitespace() {
    while (this.char && /\s/.test(this.char)) {
      this.readChar();
    }
  }

  public nextToken(): Token {
    this.skipWhitespace();

    // Check for end of file first
    if (this.char === null) {
      return { type: TokenType.EOF, value: "" };
    }

    // Handle symbols and operators
    switch (this.char) {
      case "+":
        this.readChar();
        return { type: TokenType.Operator, value: "+" };
      case "-":
        this.readChar();
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
          } else {
            strValue += this.char;
          }
          this.readChar();
        }
        this.readChar(); // Consume the closing quote
        return { type: TokenType.String, value: strValue };

      case ".": // Handle double literals
        this.readChar();
        if (this.isDigit(this.char)) {
          const doubleValue = this.readNumber();
          return { type: TokenType.Double, value: doubleValue };
        }
        return { type: TokenType.Illegal, value: "." };
      case ">":
        this.readChar();
        if ((this.char as string) === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: ">=" };
        }
        return { type: TokenType.Operator, value: ">" };
      case "<":
        this.readChar();
        if ((this.char as string) === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: "<=" };
        }
        return { type: TokenType.Operator, value: "<" };
      case "!":
        this.readChar();
        if ((this.char as string) === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: "!=" };
        }
        return { type: TokenType.Operator, value: "!" };
      case "=":
        this.readChar();
        if ((this.char as string) === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: "==" };
        }
        return { type: TokenType.Operator, value: "=" };

      case "&":
        this.readChar();
        if ((this.char as string) === "&") {
          this.readChar();
          return { type: TokenType.Operator, value: "&&" };
        }
        return { type: TokenType.Illegal, value: "&" };

      case "|":
        this.readChar();
        if ((this.char as string) === "|") {
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

      case "{": // Add these if missing!
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
            "if",
            "const",
            "func",
            "true",
            "false",
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
          return { type: TokenType.Number, value: num };
        }

        if (this.isDouble(this.char)) {
          const double = this.readNumber();
          return { type: TokenType.Double, value: double };
        }

        // If we don't recognize it, it's an illegal character
        const illegalChar = this.char || "";
        this.readChar();
        return { type: TokenType.Illegal, value: illegalChar };
    }
  }

  private isLetter(char: string | null): boolean {
    return !!char && /[a-zA-Z_]/.test(char);
  }

  private isDigit(char: string | null): boolean {
    return !!char && /[0-9]/.test(char);
  }

  private isDouble(char: string | null): boolean {
    return !!char && /[0-9.]/.test(char);
  }

  private isInteger(char: string | null): boolean {
    return !!char && /[0-9]/.test(char);
  }

  private readIdentifier(): string {
    let literal = "";
    while (this.isLetter(this.char)) {
      literal += this.char;
      this.readChar();
    }
    return literal;
  }

  private readComment(): string {
    let comment = "";
    while (this.char !== null && this.char !== "\n") {
      comment += this.char;
      this.readChar();
    }
    return comment.trim();
  }

  private readNumber(): string {
    let literal = "";
    while (this.isDigit(this.char)) {
      literal += this.char;
      this.readChar();
    }
    return literal;
  }
}
