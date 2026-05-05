export enum TokenType {
    Identifier = "Identifier",
    Number = "Number",
    String = "String",
    Operator = "Operator",
    Keyword = "Keyword",
    Punctuation = "Punctuation",
    EOF = "EOF",
    Illegal = "Illegal",
    Comment = "Comment",
    Function = "Function"
}

export interface Token {
    type: TokenType;
    value: string;
}