import { Program, Statement, Expression, PrintStatement, VariableDeclaration, Comment, CallExpression } from "../ast/ast";
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

      // Ignore comments at the top level; they are not part of the statement AST.
      if (this.peek().type === TokenType.Comment) {
        this.advance();
        continue;
      }

      program.body.push(this.parseStatement());
    }

    return program;
  }

  private parseStatement(): Statement {
    const token = this.peek();

    if (token.type === TokenType.Keyword) {
    if (token.value === "let" || token.value === "const") return this.parseVariableDeclaration();
    if (token.value === "printf") return this.parsePrintStatement();
    if (token.value === "func") return this.parseFunctionDeclaration();
    }

    if (token.type === TokenType.Identifier) {
    return this.parseExpressionStatement();
    }
    
    throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
  }

  private parseExpressionStatement(): any {
  const expr = this.parseExpression();
  if (this.peek().value === ";") this.advance(); // consume ";"
  return expr; // In a simple AST, you can just return the expression node
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

  private parseComment(): Comment {
    const token = this.advance(); // consume the comment token
    return { type: "Comment", value: token.value };
  }

  private parseFunctionDeclaration(): Statement {
    this.advance(); // consume "func"
    const nameToken = this.advance(); // get function name
    if (nameToken.type !== TokenType.Identifier) {
      throw new Error("Expected function name after 'func'");
    }

    if (this.peek().value !== "(") {
      throw new Error("Expected '(' after function name");
    }
    this.advance(); // consume "("

    const parameters: string[] = [];
    while (!this.isAtEnd() && this.peek().value !== ")") {
      const paramToken = this.advance();
      if (paramToken.type !== TokenType.Identifier) {
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
    } else {
      throw new Error("Expected '{' to start function body");
    }

    return {
      type: "Function",
      name: nameToken.value,
      parameters,
      body
    };
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
      if (this.peek().value === "(") {
        // This is a function call, not just a variable reference
        const funcName = token.value;
        this.advance(); // consume "("
        const args: Expression[] = [];
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
