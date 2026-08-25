#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../ast/ast.cpp"
#include "../types/types.cpp"

class Parser
{
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), index_(0) {}

    Program parse()
    {
        Program program;
        while (!isAtEnd())
        {
            if (peek().type == TokenType::Comment)
            {
                advance();
                continue;
            }
            if (peek().value == ";")
            {
                advance();
                continue;
            }

            Node *stmt = parseStatement();
            if (stmt)
            {
                program.body.push_back(stmt);
            }
        }
        return program;
    }

private:
    bool isAtEnd() const
    {
        return peek().type == TokenType::EOF_T;
    }

    Token peek() const
    {
        if (index_ >= tokens_.size())
            return Token{TokenType::EOF_T, ""};
        return tokens_[index_];
    }

    Token advance()
    {
        if (index_ >= tokens_.size())
            return Token{TokenType::EOF_T, ""};
        return tokens_[index_++];
    }

    bool isBinaryOperator(const Token &token) const
    {
        if (token.type != TokenType::Operator)
            return false;
        static const std::vector<std::string> ops = {"+", "-", "*", "/", "%", "==", "!=", ">", "<", ">=", "<=", "&&", "||"};
        return std::find(ops.begin(), ops.end(), token.value) != ops.end();
    }

    Node *parseStatement()
    {
        // Comments were only ever skipped by the top-level parse() loop,
        // so one written inside a function/if/while body would fall
        // through every check below and hit the catch-all -- which
        // consumes the comment's own token, not the comment, and hands
        // back an empty Identifier as if it were real code. Skipping
        // here instead means every nested body-parsing loop (which all
        // call parseStatement in a loop and already handle a null
        // return the same way ';' is handled) gets this for free.
        if (peek().type == TokenType::Comment)
        {
            advance();
            return nullptr;
        }

        if (peek().value == ";")
        {
            advance();
            return nullptr;
        }

        if (peek().value == "let" || peek().value == "const" || peek().value == "mut")
        {
            return parseVariableDeclaration();
        }

        if (peek().value == "func" || peek().value == "fn")
        {
            return parseFunctionDeclaration();
        }

        if (peek().value == "extern")
        {
            advance(); // "extern"
            Node *node = parseFunctionDeclaration();
            if (auto *fn = dynamic_cast<Function *>(node))
            {
                fn->isExtern = true;
            }
            return node;
        }

        if (peek().value == "printf" || peek().value == "say")
        {
            return parsePrintStatement();
        }

        if (peek().value == "return")
        {
            return parseReturnStatement();
        }

        if (peek().value == "if")
        {
            return parseIfStatement();
        }

        if (peek().value == "while")
        {
            return parseWhileLoop();
        }

        if (peek().value == "import")
        {
            return parseImportStatement();
        }

        if (peek().type == TokenType::Identifier)
        {
            return parseIdentifierStatement();
        }

        if (peek().type == TokenType::Number || peek().type == TokenType::Double || peek().type == TokenType::String)
        {
            return parseLiteralStatement();
        }

        if (peek().value == "true" || peek().value == "false")
        {
            return parseLiteralStatement();
        }

        ExpressionStatement *node = new ExpressionStatement();
        node->expression = parseExpression();
        return node;
    }

    Node *parseVariableDeclaration()
    {
        std::string kind = advance().value;
        std::string name = advance().value;

        std::string type = "";
        if (peek().value == ":")
        {
            advance();
            type = advance().value;

            // Array type: `number[200]` -- a fixed size, given as a
            // literal, folded straight into the type string ("number[200]")
            // so nothing else needs a new AST field for it.
            if (peek().value == "[")
            {
                advance();
                std::string sizeTok = advance().value;
                type += "[" + sizeTok + "]";
                if (peek().value == "]")
                {
                    advance();
                }
            }
        }

        // Arrays are declared with a fixed size and no initializer --
        // `let nums: number[5];` -- so only parse a value if `=` was
        // actually written.
        Node *value = nullptr;
        if (peek().value == "=")
        {
            advance();
            value = parseExpression();
        }

        VariableDeclaration *node = new VariableDeclaration();
        node->identifier = name;
        node->declaredType = type;
        node->value = value;
        (void)kind;
        return node;
    }

    Node *parseFunctionDeclaration()
    {
        advance(); // func
        Function *node = new Function();

        Token nameTok = advance();
        node->name = nameTok.value;

        if (peek().value == "(")
        {
            advance();
            while (!isAtEnd() && peek().value != ")")
            {
                Token paramName = advance();
                Param p;
                p.name = paramName.value;
                if (peek().value == ":")
                {
                    advance();
                    p.type = advance().value;
                }
                node->params.push_back(p);
                if (peek().value == ",")
                {
                    advance();
                }
            }
            if (peek().value == ")")
            {
                advance();
            }
        }

        if (peek().value == ":")
        {
            advance();
            node->returnType = advance().value;
        }

        if (peek().value == "{")
        {
            advance();
            while (!isAtEnd() && peek().value != "}")
            {
                Node *stmt = parseStatement();
                if (stmt)
                {
                    node->body.push_back(stmt);
                }
            }
            if (peek().value == "}")
            {
                advance();
            }
        }

        return node;
    }

    Node *parsePrintStatement()
    {
        advance();
        PrintStatement *node = new PrintStatement();
        node->expression = parseExpression();
        return node;
    }

    Node *parseReturnStatement()
    {
        advance(); // "return"

        // A bare `return;` with no value is valid (early-exit from a
        // void function). Only parse an expression if the next token
        // could actually start one -- otherwise, since ';' produces no
        // token at all (the lexer discards it), parseExpression() would
        // fall through to its catch-all and consume whatever comes
        // next instead (e.g. a block's closing '}'), desynchronizing
        // the rest of the parse.
        bool hasValue = peek().type == TokenType::Identifier ||
                        peek().type == TokenType::Number ||
                        peek().type == TokenType::Double ||
                        peek().type == TokenType::String ||
                        peek().value == "true" || peek().value == "false" ||
                        peek().value == "(" ||
                        (peek().type == TokenType::Operator && (peek().value == "!" || peek().value == "-"));

        ReturnStatement *node = new ReturnStatement();
        node->value = hasValue ? parseExpression() : nullptr;
        return node;
    }

    Node *parseIfStatement()
    {
        advance();
        IfStatement *node = new IfStatement();
        node->condition = parseExpression();

        if (peek().value == "{")
        {
            advance();
            while (!isAtEnd() && peek().value != "}")
            {
                Node *stmt = parseStatement();
                if (stmt)
                {
                    node->consequent.push_back(stmt);
                }
            }
            if (peek().value == "}")
            {
                advance();
            }
        }

        if (peek().value == "else")
        {
            advance();
            if (peek().value == "if")
            {
                Node *nested = parseIfStatement();
                node->alternate.push_back(nested);
            }
            else if (peek().value == "{")
            {
                advance();
                while (!isAtEnd() && peek().value != "}")
                {
                    Node *stmt = parseStatement();
                    if (stmt)
                    {
                        node->alternate.push_back(stmt);
                    }
                }
                if (peek().value == "}")
                {
                    advance();
                }
            }
        }

        return node;
    }

    Node *parseWhileLoop()
    {
        advance();
        WhileLoop *node = new WhileLoop();
        node->condition = parseExpression();

        if (peek().value == "{")
        {
            advance();
            while (!isAtEnd() && peek().value != "}")
            {
                Node *stmt = parseStatement();
                if (stmt)
                {
                    node->body.push_back(stmt);
                }
            }
            if (peek().value == "}")
            {
                advance();
            }
        }

        return node;
    }

    Node *parseImportStatement()
    {
        advance(); // "import"

        ImportStatement *node = new ImportStatement();
        if (peek().type == TokenType::String)
        {
            node->path = advance().value;
        }
        return node;
    }

    Node *parseIdentifierStatement()
    {
        std::string name = advance().value;

        if (peek().value == "[")
        {
            advance();
            Node *indexExpr = parseExpression();
            if (peek().value == "]")
            {
                advance();
            }

            if (peek().value == "=")
            {
                advance();
                IndexAssignment *node = new IndexAssignment();
                Identifier *obj = new Identifier();
                obj->name = name;
                node->object = obj;
                node->index = indexExpr;
                node->value = parseExpression();
                return node;
            }

            // `name[i]` used as a bare statement (rare, but falls back
            // safely rather than dropping the tokens).
            IndexExpression *idx = new IndexExpression();
            Identifier *obj = new Identifier();
            obj->name = name;
            idx->object = obj;
            idx->index = indexExpr;
            return idx;
        }

        if (peek().value == "=")
        {
            advance();
            Assignment *node = new Assignment();
            node->identifier = name;
            node->value = parseExpression();
            return node;
        }

        if (peek().value == "(")
        {
            advance();
            CallExpression *node = new CallExpression();
            node->callee = name;
            while (!isAtEnd() && peek().value != ")")
            {
                node->arguments.push_back(parseExpression());
                if (peek().value == ",")
                {
                    advance();
                }
            }
            if (peek().value == ")")
            {
                advance();
            }
            return node;
        }

        if (peek().value == "++" || peek().value == "--")
        {
            std::string op = advance().value;
            if (op == "++")
            {
                auto *node = new Increment();
                node->identifier = name;
                return node;
            }
            else
            {
                auto *node = new Decrement();
                node->identifier = name;
                return node;
            }
        }

        Identifier *node = new Identifier();
        node->name = name;
        return node;
    }

    Node *parseLiteralStatement()
    {
        Token tok = advance();

        if (tok.type == TokenType::Number)
        {
            LiteralInt *val = new LiteralInt();
            val->value = std::stoll(tok.value);
            return val;
        }

        if (tok.type == TokenType::Double)
        {
            LiteralDoudle *val = new LiteralDoudle();
            val->value = std::stod(tok.value);
            return val;
        }

        if (tok.type == TokenType::String)
        {
            LiteralString *val = new LiteralString();
            val->value = tok.value;
            return val;
        }

        if (tok.value == "true")
        {
            LiteralBool *val = new LiteralBool();
            val->value = true;
            return val;
        }

        if (tok.value == "false")
        {
            LiteralBool *val = new LiteralBool();
            val->value = false;
            return val;
        }

        return new Identifier();
    }

    Node *parseExpression()
    {
        return parsePrecedence(0);
    }

    Node *parsePrecedence(int minPrecedence)
    {
        Node *left = parsePrimary();

        while (!isAtEnd() && peek().type == TokenType::Operator && isBinaryOperator(peek()))
        {
            std::string op = peek().value;
            int precedence = precedenceOf(op);
            if (precedence < minPrecedence)
            {
                break;
            }
            advance();

            Node *previousLeft = left;
            Node *right = parsePrecedence(precedence + 1);

            BinaryExpression *expr = new BinaryExpression();
            expr->op = op;
            expr->left = previousLeft;
            expr->right = right;
            left = expr;
        }

        return left;
    }

    int precedenceOf(const std::string &op) const
    {
        if (op == "||")
            return 1;
        if (op == "&&")
            return 2;
        if (op == "==" || op == "!=")
            return 3;
        if (op == "<" || op == ">" || op == "<=" || op == ">=")
            return 4;
        if (op == "+" || op == "-")
            return 5;
        if (op == "*" || op == "/" || op == "%")
            return 6;
        return -1;
    }

    Node *parsePrimary()
    {
        if (peek().type == TokenType::Identifier)
        {
            std::string name = advance().value;
            if (peek().value == "(")
            {
                advance();
                CallExpression *node = new CallExpression();
                node->callee = name;
                while (!isAtEnd() && peek().value != ")")
                {
                    node->arguments.push_back(parseExpression());
                    if (peek().value == ",")
                    {
                        advance();
                    }
                }
                if (peek().value == ")")
                {
                    advance();
                }
                return node;
            }

            if (peek().value == "[")
            {
                advance();
                IndexExpression *idx = new IndexExpression();
                Identifier *obj = new Identifier();
                obj->name = name;
                idx->object = obj;
                idx->index = parseExpression();
                if (peek().value == "]")
                {
                    advance();
                }
                return idx;
            }

            Identifier *node = new Identifier();
            node->name = name;
            return node;
        }

        if (peek().type == TokenType::Number)
        {
            LiteralInt *val = new LiteralInt();
            val->value = std::stoll(advance().value);
            return val;
        }

        if (peek().type == TokenType::Double)
        {
            LiteralDoudle *val = new LiteralDoudle();
            val->value = std::stod(advance().value);
            return val;
        }

        if (peek().type == TokenType::String)
        {
            LiteralString *val = new LiteralString();
            val->value = advance().value;
            return val;
        }

        if (peek().value == "true" || peek().value == "false")
        {
            LiteralBool *val = new LiteralBool();
            val->value = (advance().value == "true");
            return val;
        }

        if (peek().value == "(")
        {
            advance();
            Node *inner = parseExpression();
            if (peek().value == ")")
            {
                advance();
            }
            return inner;
        }

        if (peek().type == TokenType::Operator && (peek().value == "!" || peek().value == "-"))
        {
            UnaryExpression *node = new UnaryExpression();
            node->op = advance().value;
            node->operand = parsePrimary();
            return node;
        }

        Token t = advance();
        (void)t;
        return new Identifier();
    }

    std::vector<Token> tokens_;
    size_t index_;
};