// dest/quill.js
import * as fs from "fs";
import * as path from "path";

// dest/core/types/types.js
var TokenType = {
  Identifier: "Identifier",
  Number: "Number",
  String: "String",
  FString: "FString",
  Operator: "Operator",
  Keyword: "Keyword",
  Double: "Double",
  Punctuation: "Punctuation",
  EOF: "EOF",
  Illegal: "Illegal",
  Comment: "Comment",
  Function: "Function",
  Boolean: "Boolean",
  Conditional: "Conditional",
  Loop: "Loop",
  Integer: "Integer",
  Dot: "Dot",
  Incrementation: "Incrementation"
};

// dest/core/lexer/lexer.js
var Lexer = class {
  peekChar() {
    if (this.position >= this.input.length) {
      return "";
    } else {
      return this.input[this.position];
    }
  }
  constructor(input) {
    this.input = input;
    this.position = 0;
    this.char = "";
    this.readChar();
  }
  readChar() {
    if (this.position >= this.input.length) {
      this.char = null;
    } else {
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
    if (this.char === null) {
      return { type: TokenType.EOF, value: "" };
    }
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
        this.readChar();
        let strValue = "";
        while (this.char !== null && this.char !== '"') {
          if (this.char === "\\") {
            this.readChar();
            strValue += this.char;
          } else {
            strValue += this.char;
          }
          this.readChar();
        }
        this.readChar();
        return { type: TokenType.String, value: strValue };
      case ".":
        this.readChar();
        if (this.isDigit(this.char)) {
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
            "!"
          ];
          const type = keywords.includes(literal) ? TokenType.Keyword : TokenType.Identifier;
          return { type, value: literal };
        }
        if (this.isDigit(this.char)) {
          const num = this.readNumber();
          if (num.includes(".")) {
            return { type: TokenType.Double, value: num };
          }
          return { type: TokenType.Number, value: num };
        }
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
    while (this.isLetter(this.char) || literal.length > 0 && this.isDigit(this.char)) {
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
};

// dest/core/parser/parser.js
var Parser = class {
  constructor(tokens) {
    this.current = 0;
    this.tokens = tokens;
  }
  // Helper to look at current token without consuming it
  peek() {
    return this.tokens[this.current] || { type: TokenType.EOF, value: "" };
  }
  consume(expectedValue, errorMessage) {
    if (this.peek().value !== expectedValue) {
      throw new Error(errorMessage);
    }
    return this.advance();
  }
  // Helper to consume current token and move to next
  advance() {
    return this.tokens[this.current++];
  }
  // Helper to check if we've reached the end
  isAtEnd() {
    return this.peek().type === TokenType.EOF;
  }
  parse() {
    const program = { type: "Program", body: [] };
    while (!this.isAtEnd()) {
      if (this.peek().value === ";") {
        this.advance();
        continue;
      }
      if (this.peek().type === TokenType.Comment) {
        this.advance();
        continue;
      }
      program.body.push(this.parseStatement());
    }
    return program;
  }
  parseStatement() {
    const token = this.peek();
    if (token.type === TokenType.Keyword) {
      if (token.value === "let" || token.value === "const")
        return this.parseVariableDeclaration();
      if (token.value === "printf" || token.value === "say")
        return this.parsePrintStatement();
      if (token.value === "func")
        return this.parseFunctionDeclaration();
      if (token.value === "True" || token.value === "False")
        return this.parseExpressionStatement();
      if (token.value === ">" || token.value === "<" || token.value === ">=" || token.value === "<=" || token.value === "==" || token.value === "!=")
        return this.parseExpressionStatement();
      if (token.value === "&&" || token.value === "||" || token.value === "!")
        return this.parseExpressionStatement();
      if (token.value === "if")
        return this.parseConditionalExpression();
      if (token.value === "else")
        return this.parseConditionalExpression();
      if (token.value === "elif")
        return this.parseConditionalExpression();
      if (token.value === "while")
        return this.parseLoopExpression();
    }
    if (token.type === TokenType.Identifier || token.value === "(") {
      return this.parseExpressionStatement();
    } else if (token.type === TokenType.String) {
      return this.parseExpressionStatement();
    } else if (token.type === TokenType.Loop) {
      return this.parseLoopExpression();
    } else if (token.type === TokenType.Comment) {
      return this.parseComment();
    } else if (token.type === TokenType.Double) {
      return this.parseExpressionStatement();
    } else if (token.type === TokenType.Conditional) {
      return this.parseConditionalExpression();
    }
    throw new Error(`Unexpected token at statement level: ${token.value} (${token.type})`);
  }
  parseExpressionStatement() {
    const expr = this.parseExpression();
    if (this.peek().value === ";")
      this.advance();
    return expr;
  }
  parseString() {
    const token = this.advance();
    if (token.type !== TokenType.String) {
      throw new Error(`Expected string literal, but got: ${token.value}`);
    }
    return { type: "String", value: token.value };
  }
  parseVariableDeclaration() {
    this.advance();
    const idToken = this.advance();
    if (idToken.type !== TokenType.Identifier) {
      throw new Error("Expected variable name after 'let'");
    }
    if (this.peek().value !== "=") {
      throw new Error("Expected '=' after variable name");
    }
    this.advance();
    const val = this.parseExpression();
    if (!this.isAtEnd() && this.peek().value === ";") {
      this.advance();
    }
    return {
      type: "VariableDeclaration",
      identifier: idToken.value,
      value: val
    };
  }
  parseLoopExpression() {
    this.advance();
    const test = this.parseExpression();
    if (this.peek().value !== "{") {
      throw new Error(`Expected '{' to start loop body, but got: ${this.peek().value}`);
    }
    this.advance();
    const body = [];
    while (!this.isAtEnd() && this.peek().value !== "}") {
      body.push(this.parseStatement());
    }
    if (this.peek().value !== "}") {
      throw new Error("Expected '}' at end of loop body");
    }
    this.advance();
    return { type: "LoopExpression", test, body };
  }
  parseConditionalExpression() {
    const token = this.advance();
    const test = this.parseExpression();
    const consequent = [];
    if (this.peek().value !== "{") {
      throw new Error("Expected '{' to start block");
    }
    this.advance();
    while (!this.isAtEnd() && this.peek().value !== "}") {
      consequent.push(this.parseStatement());
    }
    this.advance();
    let alternate;
    if (this.peek().value === "else") {
      this.advance();
      if (this.peek().value === "{") {
        this.advance();
        const elseBody = [];
        while (!this.isAtEnd() && this.peek().value !== "}") {
          elseBody.push(this.parseStatement());
        }
        this.advance();
        alternate = elseBody;
      } else if (this.peek().value === "if") {
        alternate = [this.parseConditionalExpression()];
      } else if (this.peek().value === "elif") {
        alternate = [this.parseConditionalExpression()];
      }
    }
    return {
      type: "ConditionalExpression",
      test,
      consequent,
      alternate
    };
  }
  parsePrintStatement() {
    this.advance();
    const val = this.parseExpression();
    if (!this.isAtEnd() && this.peek().value === ";") {
      this.advance();
    }
    return { type: "PrintStatement", expression: val };
  }
  parseComment() {
    const token = this.advance();
    return { type: "Comment", value: token.value };
  }
  parseFunctionDeclaration() {
    this.advance();
    const nameToken = this.advance();
    if (nameToken.type !== TokenType.Identifier) {
      throw new Error("Expected function name after 'func'");
    }
    if (this.peek().value !== "(") {
      throw new Error("Expected '(' after function name");
    }
    this.advance();
    const parameters = [];
    while (!this.isAtEnd() && this.peek().value !== ")") {
      const paramToken = this.advance();
      if (paramToken.type !== TokenType.Identifier) {
        throw new Error("Expected parameter name in function declaration");
      }
      parameters.push(paramToken.value);
      if (this.peek().value === ",") {
        this.advance();
      }
    }
    this.advance();
    const body = [];
    if (this.peek().value === "{") {
      this.advance();
      while (!this.isAtEnd() && this.peek().value !== "}") {
        body.push(this.parseStatement());
      }
      if (this.peek().value !== "}") {
        throw new Error("Expected '}' at end of function body");
      }
      this.advance();
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
  parseExpression() {
    let left = this.parsePrimary();
    while (!this.isAtEnd()) {
      const next = this.peek();
      if (next.value === "[") {
        this.advance();
        const index = this.parseExpression();
        this.consume("]", "Expected ']'");
        left = { type: "IndexExpression", object: left, index };
      } else if (next.type === TokenType.Incrementation) {
        const operator = this.advance().value;
        left = {
          type: "incrementationExpression",
          identifier: left.name || left.value,
          operator,
          isPrefix: false
        };
      } else {
        break;
      }
    }
    while (!this.isAtEnd() && this.isBinaryOperator(this.peek())) {
      const operator = this.advance().value;
      const right = this.parsePrimary();
      left = { type: "BinaryExpression", left, operator, right };
    }
    return left;
  }
  // Helper to identify operators that join two expressions
  isBinaryOperator(token) {
    if (token.value === "{" || token.value === "}" || token.value === ";")
      return false;
    return token.type === TokenType.Operator && [
      "+",
      "-",
      "*",
      "/",
      "==",
      "!=",
      ">",
      "<",
      ">=",
      "<=",
      "&&",
      "||"
    ].includes(token.value);
  }
  // parsePrimary handles the "smallest" units: numbers and variables
  parsePrimary() {
    const token = this.peek();
    let currentToken;
    if (token.value === "(") {
      this.advance();
      const expr = this.parseExpression();
      if (this.peek().value !== ")") {
        throw new Error(`Expected ')' but got: ${this.peek().value}`);
      }
      this.advance();
      return expr;
    }
    const consumed = this.advance();
    if (consumed.type === TokenType.Number)
      return { type: "Literal", value: Number(consumed.value) };
    if (consumed.type === TokenType.Double)
      return { type: "Double", value: parseFloat(consumed.value) };
    if (consumed.type === TokenType.String)
      return { type: "String", value: consumed.value };
    if (consumed.type === TokenType.Keyword && (consumed.value === "True" || consumed.value === "False"))
      return { type: "Literal", value: consumed.value === "True" ? 1 : 0 };
    if (consumed.type === TokenType.Identifier) {
      if (this.peek().value === "(") {
        this.advance();
        const args = [];
        if (this.peek().value !== ")") {
          while (true) {
            args.push(this.parseExpression());
            if (this.peek().value === ",") {
              this.advance();
            } else {
              break;
            }
          }
        }
        if (this.peek().value !== ")") {
          throw new Error(`Expected ')' after arguments, but got: ${this.peek().value}`);
        }
        this.advance();
        return {
          type: "CallExpression",
          callee: consumed.value,
          arguments: args
        };
      }
      return { type: "Identifier", name: consumed.value };
    }
    throw new Error(`Expected expression, but got: ${consumed.value}`);
  }
};

// dest/core/interpreter/interpreter.js
var Interpreter = class {
  constructor() {
    this.variables = /* @__PURE__ */ new Map();
    this.setupStandardLibrary();
  }
  setupStandardLibrary() {
    this.variables.set("sqrt", {
      type: "built-in-function",
      fn: (args) => Math.sqrt(args[0])
    });
    this.variables.set("push", {
      type: "built-in-function",
      fn: (args) => {
        const [list, item] = args;
        if (Array.isArray(list)) {
          list.push(item);
          return list;
        }
        throw new Error("push() expects an array as the first argument");
      }
    });
    this.variables.set("random", {
      type: "built-in-function",
      fn: (args) => Math.random()
    });
    this.variables.set("len", {
      type: "built-in-function",
      fn: (args) => args[0].length
    });
    this.variables.set("timeNow", {
      type: "built-in-function",
      fn: () => (/* @__PURE__ */ new Date()).toLocaleDateString() + " " + (/* @__PURE__ */ new Date()).toLocaleTimeString()
    });
  }
  interpret(node) {
    switch (node.type) {
      case "Program":
        node.body.forEach((stmt) => this.interpret(stmt));
        return this.variables;
      // Return the final state of variables
      case "CallExpression":
        const functionData = this.variables.get(node.callee);
        if (!functionData)
          throw new Error(`Runtime Error: '${node.callee}' is not defined.`);
        const args = node.arguments.map((arg2) => this.interpret(arg2));
        if (functionData.type === "built-in-function") {
          return functionData.fn(args);
        }
        if (functionData.type === "user-defined-function") {
          const previousScope = new Map(this.variables);
          try {
            functionData.parameters.forEach((paramName, index) => {
              this.variables.set(paramName, args[index]);
            });
            let lastResult = null;
            for (const stmt of functionData.body) {
              lastResult = this.interpret(stmt);
            }
            return lastResult;
          } finally {
            this.variables = previousScope;
          }
        }
        throw new Error(`Runtime Error: '${node.callee}' is not a function.`);
      case "Comment":
        return;
      case "ConditionalExpression":
        const testResult = this.interpret(node.test);
        if (testResult) {
          return this.executeBlock(node.consequent);
        } else if (node.alternate) {
          return this.executeBlock(node.alternate);
        }
        return null;
      case "LoopExpression":
        while (this.interpret(node.test)) {
          this.executeBlock(node.body);
        }
        return null;
      case "Function":
        const funcData = {
          parameters: node.parameters,
          body: node.body,
          closure: new Map(this.variables),
          // Capture the current variable state for closures
          type: "user-defined-function"
        };
        this.variables.set(node.name, funcData);
        return null;
      case "String":
        return node.value;
      case "Double":
        return node.value;
      case "Integer":
        return parseInt(node.value, 10);
      case "VariableDeclaration":
        const val = this.interpret(node.value);
        this.variables.set(node.identifier, val);
        return val;
      case "PrintStatement":
        const output = this.interpret(node.expression);
        console.log(`${output}`);
        return output;
      case "ArrayLiteral":
        return node.elements.map((el) => this.interpret(el));
      case "IndexExpression":
        const array = this.interpret(node.object);
        const idx = this.interpret(node.index);
        if (!Array.isArray(array)) {
          throw new Error("Runtime Error: Object is not an array");
        }
        return array[idx];
      case "ComparisonExpression":
        const leftVal = this.interpret(node.left);
        const rightVal = this.interpret(node.right);
        switch (node.operator) {
          case "==":
            return leftVal === rightVal;
          case "!=":
            return leftVal !== rightVal;
          case "<":
            return leftVal < rightVal;
          case ">":
            return leftVal > rightVal;
          case "<=":
            return leftVal <= rightVal;
          case ">=":
            return leftVal >= rightVal;
          default:
            throw new Error(`Unknown comparison operator: ${node.operator}`);
        }
      case "incrementationExpression":
        const currentVal = this.variables.get(node.identifier);
        if (currentVal === void 0)
          throw new Error(`Undefined variable: ${node.identifier}`);
        const newVal = node.operator === "++" ? currentVal + 1 : currentVal - 1;
        this.variables.set(node.identifier, newVal);
        return newVal;
      case "BinaryExpression":
        const left = this.interpret(node.left);
        const right = this.interpret(node.right);
        switch (node.operator) {
          case "+":
            return left + right;
          case "-":
            return left - right;
          case "*":
            return left * right;
          case "/":
            return left / right;
          case "%":
            return left % right;
          case "&&":
            return left && right;
          case "||":
            return left || right;
          case "!":
            return !left;
          case ">":
            return left > right;
          case "<":
            return left < right;
          case ">=":
            return left >= right;
          case "<=":
            return left <= right;
          case "==":
            return left === right;
          case "!=":
            return left !== right;
          default:
            throw new Error(`Unknown operator: ${node.operator}`);
        }
      case "Literal":
        return node.value;
      case "BooleanLiteral":
        return node.value;
      case "Identifier":
        if (this.variables.has(node.name)) {
          return this.variables.get(node.name);
        }
        throw new Error(`Undefined variable: ${node.name}`);
      default:
        throw new Error(`Unknown node type: ${node.type}`);
    }
  }
  executeBlock(statements) {
    let result = null;
    for (const statement of statements) {
      result = this.interpret(statement);
    }
    return result;
  }
};

// dest/quill.js
var arg = process.argv[2];
var version = "0.0.3.1\nMajor: 0\nMinor: 0\nBug/Fix: 3\nStatus: Beta";
if (!arg) {
  console.error("Usage: quill <filename.quill>");
  process.exit(1);
}
if (arg === "-v" || "--version") {
  console.log(version);
  process.exit(0);
}
if (path.extname(arg) !== ".quill") {
  console.error("Error: Only .quill files are supported.");
  process.exit(1);
}
try {
  const sourceCode = fs.readFileSync(arg, "utf-8");
  const lexer = new Lexer(sourceCode);
  const tokens = [];
  let token = lexer.nextToken();
  while (token.type !== "EOF") {
    tokens.push(token);
    token = lexer.nextToken();
  }
  const parser = new Parser(tokens);
  const ast = parser.parse();
  const interpreter = new Interpreter();
  interpreter.interpret(ast);
} catch (err) {
  console.error(`Runtime Error: ${err.message}`);
}
