export enum TokenType {
    Identifier = "Identifier",
    Number = "Number",
    String = "String",
    Operator = "Operator",
    Keyword = "Keyword",
    Double = "Double",
    Punctuation = "Punctuation",
    EOF = "EOF",
    Illegal = "Illegal",
    Comment = "Comment",
    Function = "Function",
    Boolean = "Boolean",
    Conditional = "Conditional",
    Loop = "Loop",
    Integer = "Integer",
}

export interface Token {
    type: TokenType;
    value: string;
}

