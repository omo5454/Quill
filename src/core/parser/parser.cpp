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
        if (peek().value == ";")
        {
            advance();
            return nullptr;
        }

        if (peek().value == "let" || peek().value == "const" || peek().value == "mut")
        {
            return parseVariableDeclaration();
        }

        if (peek().value == "func")
        {
            return parseFunctionDeclaration();
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

        if (peek().type == TokenType::Identifier)
        {
            return parseIdentifierStatement();
        }

        if (peek().type == TokenType::Number || peek().type == TokenType::Float || peek().type == TokenType::Str)
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
        }

        if (peek().value == "=")
        {
            advance();
        }

        Node *value = parseExpression();

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
        advance();
        ReturnStatement *node = new ReturnStatement();
        node->value = parseExpression();
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

    Node *parseIdentifierStatement()
    {
        std::string name = advance().value;

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

        if (tok.type == TokenType::Float)
        {
            LiteralFloat *val = new LiteralFloat();
            val->value = std::stod(tok.value);
            return val;
        }

        if (tok.type == TokenType::Str)
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

        if (peek().type == TokenType::Float)
        {
            LiteralFloat *val = new LiteralFloat();
            val->value = std::stod(advance().value);
            return val;
        }

        if (peek().type == TokenType::Str)
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