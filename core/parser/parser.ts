// parser.ts
import { Program, Statement, Expression, PrintStatement, VariableDeclaration, Comment, CallExpression, String as StringNode, ConditionalExpression, ArrayLiteral, LoopExpression, Function } from "../ast/ast";
import { TokenType, Token } from "../types/types";

export class Parser {
  private tokens: Token[];
  private current = 0;

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  private peek(): Token {
    return this.tokens[this.current] || { type: TokenType.EOF, value: "" };
  }

  private advance(): Token {
    return this.tokens[this.current++];
  }

  private isAtEnd(): boolean {
    return this.peek().type === TokenType.EOF;
  }

  public parse(): Program {
    const program: Program = { type: "Program", body: [] };

    while (!this.isAtEnd()) {
      const token = this.peek();
      if (token.value === ";" || token.type === TokenType.Comment) {
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

  private parseExpressionStatement(): any {
    const expr = this.parseExpression();
    if (this.peek().value === ";") this.advance();
    return expr;
  }

  private parseVariableDeclaration(): VariableDeclaration {
    this.advance(); // consume let/const
    const id = this.advance();
    if (id.type !== TokenType.Identifier) throw new Error("Expected identifier");

    if (this.peek().value !== "=") throw new Error("Expected '='");
    this.advance();

    const value = this.parseExpression();
    if (this.peek().value === ";") this.advance();

    return { type: "VariableDeclaration", identifier: id.value, value };
  }

  private parsePrintStatement(): PrintStatement {
    this.advance(); // printf
    const expression = this.parseExpression();
    if (this.peek().value === ";") this.advance();
    return { type: "PrintStatement", expression };
  }

  private parseFunctionDeclaration(): Function {
    this.advance(); // func
    const nameToken = this.advance();
    if (nameToken.type !== TokenType.Identifier) throw new Error("Expected function name");

    if (this.peek().value !== "(") throw new Error("Expected '('");
    this.advance();

    const parameters: string[] = [];
    while (this.peek().value !== ")") {
      const param = this.advance();
      if (param.type === TokenType.Identifier) parameters.push(param.value);
      if (this.peek().value === ",") this.advance();
    }
    this.advance(); // )

    if (this.peek().value !== "{") throw new Error("Expected '{'");
    this.advance();

    const body: Statement[] = [];
    while (this.peek().value !== "}") {
      body.push(this.parseStatement());
    }
    this.advance(); // }

    return { type: "Function", name: nameToken.value, parameters, body };
  }

  private parseConditionalExpression(): ConditionalExpression {
    this.advance(); // if
    const test = this.parseExpression();

    if (this.peek().value !== "{") throw new Error("Expected '{' after condition");
    this.advance();

    const consequent: Statement[] = [];
    while (this.peek().value !== "}") {
      consequent.push(this.parseStatement());
    }
    this.advance();

    let alternate: any = undefined;
    if (this.peek().value === "else") {
      this.advance();
      if (this.peek().value === "{") {
        this.advance();
        const elseBody: Statement[] = [];
        while (this.peek().value !== "}") {
          elseBody.push(this.parseStatement());
        }
        this.advance();
        alternate = elseBody;
      } else if (this.peek().value === "if") {
        alternate = this.parseConditionalExpression();
      }
    }

    return { type: "ConditionalExpression", test, consequent, alternate };
  }

  private parseLoopExpression(): LoopExpression {
    this.advance(); // while
    const test = this.parseExpression();

    if (this.peek().value !== "{") throw new Error("Expected '{' after while condition");
    this.advance();

    const body: Statement[] = [];
    while (this.peek().value !== "}") {
      body.push(this.parseStatement());
    }
    this.advance();

    return { type: "LoopExpression", test, body };
  }

  private parseExpression(): Expression {
    let left = this.parsePrimary();

    // Handle array indexing: list[0]
    while (this.peek().value === "[") {
      this.advance();
      const index = this.parseExpression();
      if (this.peek().value !== "]") throw new Error("Expected ']'");
      this.advance();
      left = { type: "IndexExpression", object: left, index };
    }

    // Binary operators
    while (this.isBinaryOperator(this.peek())) {
      const operator = this.advance().value;
      const right = this.parseExpression();   // Changed to parseExpression for better precedence
      left = { type: "BinaryExpression", left, operator, right };
    }

    return left;
  }

  private parsePrimary(): Expression {
    const token = this.advance();

    switch (token.type) {
      case TokenType.Number:
        return { type: "Literal", value: Number(token.value) };

      case TokenType.String:
        return { type: "String", value: token.value };

      case TokenType.Identifier:
        if (this.peek().value === "(") {
          return this.parseCallExpression(token.value);
        }
        return { type: "Identifier", name: token.value };

      case TokenType.Keyword:
        if (token.value === "true") return { type: "BooleanLiteral", value: true };
        if (token.value === "false") return { type: "BooleanLiteral", value: false };
        break;
    }

    if (token.value === "[") {
      return this.parseArrayLiteral();
    }

    if (token.value === "(") {
      const expr = this.parseExpression();
      if (this.peek().value !== ")") throw new Error("Expected ')'");
      this.advance();
      return expr;
    }

    throw new Error(`Unexpected token in primary: ${token.value}`);
  }

  private parseCallExpression(callee: string): CallExpression {
    this.advance(); // consume "("
    const args: Expression[] = [];

    while (this.peek().value !== ")") {
      args.push(this.parseExpression());
      if (this.peek().value === ",") this.advance();
    }
    this.advance(); // consume ")"

    return { type: "CallExpression", callee, arguments: args };
  }

  private parseArrayLiteral(): ArrayLiteral {
    const elements: Expression[] = [];

    while (this.peek().value !== "]") {
      elements.push(this.parseExpression());
      if (this.peek().value === ",") this.advance();
    }
    this.advance(); // consume "]"

    return { type: "ArrayLiteral", elements };
  }

  private isBinaryOperator(token: Token): boolean {
    return token.type === TokenType.Operator &&
      ["+", "-", "*", "/", "%", "==", "!=", ">", "<", ">=", "<=", "&&", "||"].includes(token.value);
  }
}