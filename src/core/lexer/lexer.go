package lexer

import (
	"quill/src/core/types"
	"slices"
	"strings"
	"unicode"
)

type Lexer struct {
	input    []rune
	position int
	char     rune
}

func NewLexer(input string) *Lexer {
	l := &Lexer{input: []rune(input)}
	l.readChar()
	return l
}

func (l *Lexer) Tokenize() []types.Token {
	var tokens []types.Token
	for {
		tok := l.NextToken()
		tokens = append(tokens, tok)
		if tok.Type == types.EOF {
			break
		}
	}
	return tokens
}

func (l *Lexer) peekChar() rune {
	if l.position >= len(l.input) {
		return 0
	}
	return l.input[l.position]
}

func (l *Lexer) readChar() {
	if l.position >= len(l.input) {
		l.char = 0
	} else {
		l.char = l.input[l.position]
	}
	l.position++
}

func (l *Lexer) skipWhitespace() {
	for l.char != 0 && unicode.IsSpace(l.char) {
		l.readChar()
	}
}

func (l *Lexer) readNumber() string {
	position := l.position - 1
	dotSeen := false

	for unicode.IsDigit(l.char) || (!dotSeen && l.char == '.') {
		if l.char == '.' {
			dotSeen = true
		}
		l.readChar()
	}

	return string(l.input[position : l.position-1])
}

func (l *Lexer) readComment() string {
	position := l.position

	for l.char != 0 && l.char != '\n' {
		l.readChar()
	}
	return strings.TrimSpace(string(l.input[position:l.position]))
}

func (l *Lexer) readIdentifier() string {
	position := l.position - 1

	for unicode.IsLetter(l.char) || unicode.IsDigit(l.char) || l.char == '_' {
		l.readChar()
	}
	return string(l.input[position : l.position-1])
}

func (l *Lexer) NextToken() types.Token {
	l.skipWhitespace()

	for l.char == ';' {
		l.readChar()
		l.skipWhitespace()
	}

	if l.char == 0 {
		return types.Token{Type: types.EOF, Value: ""}
	}

	switch l.char {
	case '+':
		l.readChar()
		if l.char == '+' {
			l.readChar()
			return types.Token{Type: types.Incrementation, Value: "++"}
		}
		return types.Token{Type: types.Operator, Value: "+"}

	case '-':
		l.readChar()
		if l.char == '-' {
			l.readChar()
			return types.Token{Type: types.Incrementation, Value: "--"}
		}
		return types.Token{Type: types.Operator, Value: "-"}

	case '*':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "*"}

	case '/':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "/"}

	case '#':
		l.readChar()
		comment := l.readComment()
		return types.Token{Type: types.Comment, Value: comment}

	case ':':
		l.readChar()
		return types.Token{Type: types.Operator, Value: ":"}

	case ',':
		l.readChar()
		return types.Token{Type: types.Operator, Value: ","}

	case '"':
		l.readChar()
		var strValue = ""
		for l.char != 0 && l.char != '"' {
			if l.char == '\\' {
				l.readChar() // consume backslash
				switch l.char {
				case 'n':
					strValue += "\n"
				case 't':
					strValue += "\t"
				case '"':
					strValue += "\""
				case '\\':
					strValue += "\\"
				default:
					strValue += string(l.char)
				}
			} else {
				strValue += string(l.char)
			}
			l.readChar()
		}
		l.readChar() // consume closing "
		return types.Token{Type: types.Str, Value: strValue}

	case '.':
		l.readChar()
		if unicode.IsDigit(l.char) {
			num := l.readNumber()
			return types.Token{Type: types.Float, Value: "." + num}
		}
		return types.Token{Type: types.Dot, Value: "."}

	case '>':
		l.readChar()
		if l.char == '=' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: ">="}
		}
		return types.Token{Type: types.Operator, Value: ">"}

	case '<':
		l.readChar()
		if l.char == '=' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: "<="}
		}
		return types.Token{Type: types.Operator, Value: "<"}

	case '!':
		l.readChar()
		if l.char == '=' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: "!="}
		}
		return types.Token{Type: types.Operator, Value: "!"}

	case '=':
		l.readChar()
		if l.char == '=' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: "=="}
		}
		return types.Token{Type: types.Operator, Value: "="}

	case '&':
		l.readChar()
		if l.char == '&' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: "&&"}
		}
		return types.Token{Type: types.Illegal, Value: "&"}

	case '|':
		l.readChar()
		if l.char == '|' {
			l.readChar()
			return types.Token{Type: types.Operator, Value: "||"}
		}
		return types.Token{Type: types.Illegal, Value: "|"}

	case '%':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "%"}

	case '[':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "["}
	case ']':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "]"}

	case '(':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "("}
	case ')':
		l.readChar()
		return types.Token{Type: types.Operator, Value: ")"}

	case '{':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "{"}
	case '}':
		l.readChar()
		return types.Token{Type: types.Operator, Value: "}"}

	default:
		if unicode.IsLetter(l.char) {
			literal := l.readIdentifier()
			keywords := []string{
				"let", "printf", "say", "if", "const", "func", "true", "false", "else",
				"while", "return", "elif",
			}

			if slices.Contains(keywords, literal) {
				return types.Token{Type: types.Keyword, Value: literal}
			}
			return types.Token{Type: types.Identifier, Value: literal}
		}

		if unicode.IsDigit(l.char) {
			num := l.readNumber()
			if strings.Contains(num, ".") {
				return types.Token{Type: types.Float, Value: num}
			}
			return types.Token{Type: types.Number, Value: num}
		}

		var illegalChar = string(l.char)
		l.readChar()
		return types.Token{Type: types.Illegal, Value: illegalChar}
	}
}
