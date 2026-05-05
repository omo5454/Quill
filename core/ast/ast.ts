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

export interface Identifier {
  type: "Identifier";
  name: string;
}

export interface PrintStatement {
    type: "PrintStatement";
    expression: Expression; // Changed from 'identifier' to 'expression'
}

export type Statement = VariableDeclaration | PrintStatement;
export type Expression = BinaryExpression | Literal | Identifier;
