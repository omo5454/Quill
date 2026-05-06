// lexer.ts
import { TokenType, Token } from "../types/types";

export class Lexer {
  private position = 0;
  private char: string | null = "";

  constructor(private input: string) {
    this.readChar();
  }

  private readChar() {
    this.char = this.position >= this.input.length ? null : this.input[this.position];
    this.position++;
  }

  private skipWhitespace() {
    while (this.char && /\s/.test(this.char)) {
      this.readChar();
    }
  }

  public nextToken(): Token {
    this.skipWhitespace();

    if (this.char === null) {
      return { type: TokenType.EOF, value: "" };
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
        return { type: TokenType.Operator, value: op };

      case "#":
        const comment = this.readComment();
        return { type: TokenType.Comment, value: comment };

      case '"':
        return this.readString();

      case "=":
        this.readChar();
        if (this.char === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: "==" };
        }
        return { type: TokenType.Operator, value: "=" };

      case "!":
        this.readChar();
        if ((this.char as string) === "=") {
          this.readChar();
          return { type: TokenType.Operator, value: "!=" };
        }
        return { type: TokenType.Operator, value: "!" };

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

      default:
        if (this.isLetter(this.char)) {
          return this.readIdentifierOrKeyword();
        }

        if (this.isDigit(this.char)) {
          return this.readNumber();
        }

        const illegal = this.char;
        this.readChar();
        return { type: TokenType.Illegal, value: illegal };
    }
  }

  private readString(): Token {
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
    return { type: TokenType.String, value };
  }

  private readIdentifierOrKeyword(): Token {
    let literal = "";
    while (this.char && /[a-zA-Z_]/.test(this.char)) {
      literal += this.char;
      this.readChar();
    }

    const keywords = ["let", "const", "printf", "if", "else", "while", "func", "true", "false"];

    return {
      type: keywords.includes(literal) ? TokenType.Keyword : TokenType.Identifier,
      value: literal
    };
  }

  private readNumber(): Token {
    let value = "";
    while (this.char && /[0-9]/.test(this.char)) {
      value += this.char;
      this.readChar();
    }
    return { type: TokenType.Number, value };
  }

  private readComment(): string {
    let comment = "";
    while (this.char !== null && this.char !== "\n") {
      comment += this.char;
      this.readChar();
    }
    return comment.trim();
  }

  private isLetter(char: string | null): boolean {
    return !!char && /[a-zA-Z_]/.test(char);
  }

  private isDigit(char: string | null): boolean {
    return !!char && /[0-9]/.test(char);
  }
}