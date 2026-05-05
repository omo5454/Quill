import { Lexer } from "../core/lexer/lexer";
import { TokenType } from "../core/types/types";

const input = "let x = 5 + 10";
const lexer = new Lexer(input);

let token = lexer.nextToken();
while (token.type !== TokenType.EOF) {
  console.log(token);
  token = lexer.nextToken();
}
