import { Program, Statement, Expression, PrintStatement, VariableDeclaration } from "../ast/ast";
import { TokenType, Token } from "../types/types";

export class Parser {
  private tokens: Token[];
  private current = 0;

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  // Helper to look at current token without consuming it
  private peek(): Token {
    return this.tokens[this.current] || { type: TokenType.EOF, value: "" };
  }

  // Helper to consume current token and move to next
  private advance(): Token {
    return this.tokens[this.current++];
  }

  // Helper to check if we've reached the end
  private isAtEnd(): boolean {
    return this.peek().type === TokenType.EOF;
  }

  public parse(): Program {
    const program: Program = { type: "Program", body: [] };

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

  private parseStatement(): Statement {
    const token = this.peek();

    if (token.type === TokenType.Keyword && token.value === "let") {
      return this.parseVariableDeclaration();
    }
    
    if (token.type === TokenType.Keyword && token.value === "printf") {
      return this.parsePrintStatement();
    }
    
    throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
  }

  private parseVariableDeclaration(): VariableDeclaration {
    this.advance(); // consume "let"
    
    const idToken = this.advance(); // get identifier (x)
    if (idToken.type !== TokenType.Identifier) {
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

  private parsePrintStatement(): PrintStatement {
    this.advance(); // consume "printf"
    const val = this.parseExpression();
    
    if (!this.isAtEnd() && this.peek().value === ";") {
      this.advance();
    }

    return { type: "PrintStatement", expression: val };
  }

  private parseExpression(): Expression {
    let left = this.parsePrimary();

    // ONLY continue if the next token is actually an Operator (+, -, *, /)
    // If it sees "printf" (a Keyword), this loop will now skip and return 'left'
    while (
        !this.isAtEnd() && 
        this.peek().type === TokenType.Operator && 
        this.peek().value !== ";"
     ) {
        const operator = this.advance().value;
        const right = this.parsePrimary();
        left = { type: "BinaryExpression", left, operator, right };
    }

  return left;
}


  // parsePrimary handles the "smallest" units: numbers and variables
  private parsePrimary(): Expression {
    const token = this.advance();

    if (token.type === TokenType.Number) {
      return { type: "Literal", value: Number(token.value) };
    }

    if (token.type === TokenType.Identifier) {
      return { type: "Identifier", name: token.value };
    }

    throw new Error(`Expected number or variable, but got: ${token.value}`);
  }
}
