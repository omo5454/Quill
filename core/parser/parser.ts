import { Program, Statement, Expression, PrintStatement, VariableDeclaration, Comment, CallExpression, String, ConditionalExpression, ArrayLiteral } from "../ast/ast";
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
    if (token.value === "true" || token.value === "false") return this.parseExpressionStatement();
    if (token.value === ">" || token.value === "<" || token.value === ">=" || token.value === "<=" || token.value === "==" || token.value === "!=") return this.parseExpressionStatement();
    if (token.value === "&&" || token.value === "||" || token.value === "!") return this.parseExpressionStatement();
    if (token.value === "if") return this.parseConditionalExpression();
    if (token.value === "else") return this.parseConditionalExpression();
    if (token.value === "elif") return this.parseConditionalExpression();
    if (token.value === "while") return this.parseLoopExpression();
    }

    if (token.type === TokenType.Identifier || token.value === "(") {
      return this.parseExpressionStatement();
    }else if (token.type === TokenType.String) {
      return this.parseExpressionStatement();
    } else if (token.type === TokenType.Loop) {
      return this.parseLoopExpression();
    }
    
    throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
  }

  private parseExpressionStatement(): any {
  const expr = this.parseExpression();
  if (this.peek().value === ";") this.advance(); // consume ";"
  return expr; // In a simple AST, you can just return the expression node
}

private parseString(): String {
    const token = this.advance();
    if (token.type !== TokenType.String) {
      throw new Error(`Expected string literal, but got: ${token.value}`);
    }
    return { type: "String", value: token.value };
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

  private parseLoopExpression(): any {
      this.advance(); // consume "while"

      // 1. Parse the full condition (x < 15 && running == true)
      const test = this.parseExpression();

      // 2. Explicitly look for the opening brace
      if (this.peek().value !== "{") {
        // This is where your error was likely triggered
        throw new Error(`Expected '{' to start loop body, but got: ${this.peek().value}`);
      }
      this.advance(); // consume "{"

      const body: Statement[] = [];
      while (!this.isAtEnd() && this.peek().value !== "}") {
        body.push(this.parseStatement());
      }

      if (this.peek().value !== "}") {
        throw new Error("Expected '}' at end of loop body");
      }
      this.advance(); // consume "}"

      return { type: "LoopExpression", test, body };
  }


  private parseConditionalExpression(): ConditionalExpression {
      const token = this.advance(); // consume "if" (or "elif")
      
      // 1. Parse the condition
      const test = this.parseExpression(); 

      // 2. Parse the 'then' block
      const consequent: Statement[] = [];
      if (this.peek().value !== "{") {
        throw new Error("Expected '{' to start block");
      }
      this.advance(); // consume "{"
      while (!this.isAtEnd() && this.peek().value !== "}") {
        consequent.push(this.parseStatement());
      }
      this.advance(); // consume "}"

      // 3. Handle 'else' and 'elif'
      let alternate: Statement[] | ConditionalExpression | undefined;
      
      if (this.peek().value === "else") {
        this.advance(); // consume "else"
        
        if (this.peek().value === "{") {
          // Standard else block
          this.advance();
          const elseBody: Statement[] = [];
          while (!this.isAtEnd() && this.peek().value !== "}") {
            elseBody.push(this.parseStatement());
          }
          this.advance(); // consume "}"
          alternate = elseBody;
        } else if (this.peek().value === "if") {
          // This handles "else if" by recursion
          alternate = [this.parseConditionalExpression() as any];
        }
      }

      return { type: "ConditionalExpression", test, consequent, alternate: alternate as any };
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

  // ADD THIS: Handle Indexing (e.g., list)
  while (this.peek().value === "[") {
    this.advance(); // consume "["
    const index = this.parseExpression();
    if (this.peek().value !== "]") throw new Error("Expected ']'");
    this.advance(); // consume "]"
    left = { type: "IndexExpression", object: left, index };
  }

  // Your existing binary operator loop follows...
  while (!this.isAtEnd() && this.isBinaryOperator(this.peek())) {
    const operator = this.advance().value;
    const right = this.parsePrimary();
    left = { type: "BinaryExpression", left, operator, right };
  }

  return left;
}


// Helper to identify operators that join two expressions
  private isBinaryOperator(token: Token): boolean {
    return token.type === TokenType.Operator && 
      ["+", "-", "*", "/", "==", "!=", ">", "<", ">=", "<=", "&&", "||"].includes(token.value);
  }


  // parsePrimary handles the "smallest" units: numbers and variables
  private parsePrimary(): Expression {
    const token = this.advance();

    if (token.type === TokenType.Number) {
      return { type: "Literal", value: Number(token.value) };
    }

    if (token.value === "(") {
      const expr = this.parseExpression(); // Parse everything inside
      if (this.peek().value !== ")") {
        throw new Error("Expected ')' after expression");
      }
      this.advance(); // consume ")"
      return expr;
      }

    // ADD THIS: Handle Array Literals
    if (token.value === "[") {
      const elements: Expression[] = [];
      if (this.peek().value !== "]") { // Handle non-empty array
        while (!this.isAtEnd()) {
          elements.push(this.parseExpression());
          if (this.peek().value === ",") {
            this.advance(); // consume ","
          } else {
            break;
          }
        }
      }
      if (this.peek().value !== "]") {
        throw new Error("Expected ']' at end of array literal");
      }
      this.advance(); // consume "]"
      return { type: "ArrayLiteral", elements };
    }

    if (token.type === TokenType.String) {
      return { type: "String", value: token.value };
    }

    if (token.type === TokenType.Keyword) {
      if (token.value === "true" || token.value === "false") {
        return { type: "BooleanLiteral", value: token.value === "true" };
      }
    }

    if (token.type === TokenType.Identifier) {
      if (this.peek().value === "(") {
        // ... your function call logic ...
      }
      return { type: "Identifier", name: token.value };
    }

    throw new Error(`Expected number or variable, but got: ${token.value}`);
  }

}
