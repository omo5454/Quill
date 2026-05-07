export type Node = Program | Statement | Expression;

// The root of the entire tree
export interface Program {
  type: "Program";
  body: Statement[];
}

// Actions like "let x = 5"
export interface VariableDeclaration {
  type: "VariableDeclaration";
  identifier: string;
  value: Expression;
}

// Math or values like "5 + 10"
export interface BinaryExpression {
  type: "BinaryExpression";
  left: Expression;
  operator: string;
  right: Expression;
}

export interface Literal {
  type: "Literal";
  value: number | string;
}

export interface Comment {
  type: "Comment";
  value: string;
}

export interface String {
  type: "String";
  value: string;
}

export interface Double {
  type: "Double";
  value: number;
}

export interface Integer {
  type: "Integer";
  value: number;
}

export interface Function {
  type: "Function";
  name: string;
  parameters: string[];
  body: Statement[];
}

export interface BooleanLiteral {
  type: "BooleanLiteral";
  value: boolean;
}

export interface Identifier {
  type: "Identifier";
  name: string;
}

export interface PrintStatement {
  type: "PrintStatement";
  expression: Expression; // Changed from 'identifier' to 'expression'
}

export interface ArrayLiteral {
  type: "ArrayLiteral";
  elements: Expression[];
}

export interface CallExpression {
  type: "CallExpression";
  callee: string;
  arguments: Expression[];
}

export interface Dot {
  type: "Dot";
  name: string
}

export interface ComparisonExpression {
  type: "ComparisonExpression";
  left: Expression;
  operator: string; // e.g., "==", "!=", "<", ">", "<=", ">="
  right: Expression;
}

export interface ConditionalExpression {
  type: "ConditionalExpression";
  test: Expression; // The condition to evaluate
  consequent: Statement[]; // Statements to execute if condition is true
  alternate: Statement[]; // Statements to execute if condition is false (optional)
}

export interface LoopExpression {
  type: "LoopExpression";
  test: Expression; // The condition to evaluate before each iteration
  body: Statement[]; // Statements to execute in each iteration
}

export interface IndexExpression {
  type: "IndexExpression";
  object: Expression; // The array or object being indexed
  index: Expression; // The index or key being accessed
}

export type Statement =
  | VariableDeclaration
  | PrintStatement
  | Comment
  | Function
  | Expression
  | String
  | BooleanLiteral
  | CallExpression
  | ComparisonExpression
  | ConditionalExpression
  | LoopExpression
  | ArrayLiteral
  | IndexExpression
  | Double
  | Integer
  | Dot;
export type Expression =
  | BinaryExpression
  | Literal
  | Identifier
  | CallExpression
  | String
  | BooleanLiteral
  | ComparisonExpression
  | ConditionalExpression
  | LoopExpression
  | ArrayLiteral
  | IndexExpression
  | Double
  | Integer
  | Dot;
