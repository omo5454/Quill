"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const lexer_1 = require("../core/lexer/lexer");
const types_1 = require("../core/types/types");
const input = "let x = 5 + 10";
const lexer = new lexer_1.Lexer(input);
let token = lexer.nextToken();
while (token.type !== types_1.TokenType.EOF) {
    console.log(token);
    token = lexer.nextToken();
}
