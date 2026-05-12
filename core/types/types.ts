export const TokenType = {
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
  Incrementation: "Incrementation",
} as const;

export type TokenType = typeof TokenType[keyof typeof TokenType];

export interface Token {
  type: TokenType;
  value: string;
}