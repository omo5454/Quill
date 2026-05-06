"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.TokenType = void 0;
var TokenType;
(function (TokenType) {
    TokenType["Identifier"] = "Identifier";
    TokenType["Number"] = "Number";
    TokenType["String"] = "String";
    TokenType["Operator"] = "Operator";
    TokenType["Keyword"] = "Keyword";
    TokenType["Punctuation"] = "Punctuation";
    TokenType["EOF"] = "EOF";
    TokenType["Illegal"] = "Illegal";
    TokenType["Comment"] = "Comment";
    TokenType["Function"] = "Function";
    TokenType["Boolean"] = "Boolean";
    TokenType["Conditional"] = "Conditional";
    TokenType["Loop"] = "Loop";
})(TokenType || (exports.TokenType = TokenType = {}));
