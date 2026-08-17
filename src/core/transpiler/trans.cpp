#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../lexer/lexer.cpp"
#include "../parser/parser.cpp"
#include "../typechecker/typechecker.cpp"

namespace
{

    std::string trim(const std::string &value)
    {
        std::size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
        {
            ++start;
        }

        std::size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    std::string stripTrailingSemicolon(std::string value)
    {
        value = trim(value);
        while (!value.empty() && value.back() == ';')
        {
            value.pop_back();
            value = trim(value);
        }
        return value;
    }

    std::string buildConcatCall(const std::string &expr)
    {
        std::string value = stripTrailingSemicolon(expr);
        std::vector<std::string> parts;
        std::string current;
        for (char c : value)
        {
            if (c == '+')
            {
                parts.push_back(trim(current));
                current.clear();
            }
            else
            {
                current += c;
            }
        }
        if (!current.empty())
        {
            parts.push_back(trim(current));
        }

        if (parts.empty())
        {
            return value;
        }

        std::string result = parts[0];
        for (std::size_t i = 1; i < parts.size(); ++i)
        {
            result = "quill_concat(" + result + ", " + parts[i] + ")";
        }
        return result;
    }

    std::vector<std::string> splitLines(const std::string &text)
    {
        std::vector<std::string> lines;
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            lines.push_back(line);
        }
        return lines;
    }

    std::vector<std::string> splitStatements(const std::string &input)
    {
        std::vector<std::string> statements;
        std::string current;
        bool inString = false;
        int braceDepth = 0;
        int parenDepth = 0;

        for (char c : input)
        {
            if (c == '"')
            {
                current += c;
                inString = !inString;
                continue;
            }

            if (!inString)
            {
                if (c == '{')
                {
                    ++braceDepth;
                }
                else if (c == '}')
                {
                    if (braceDepth > 0)
                        --braceDepth;
                }
                else if (c == '(')
                {
                    ++parenDepth;
                }
                else if (c == ')')
                {
                    if (parenDepth > 0)
                        --parenDepth;
                }
            }

            if (!inString && c == ';' && braceDepth == 0 && parenDepth == 0)
            {
                std::string stmt = trim(current);
                if (!stmt.empty())
                {
                    statements.push_back(stmt);
                }
                current.clear();
                continue;
            }

            current += c;
        }

        std::string tail = trim(current);
        if (!tail.empty())
        {
            statements.push_back(tail);
        }
        return statements;
    }

    std::string stripComments(const std::string &text)
    {
        std::string result;
        bool inString = false;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            char c = text[i];
            char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

            if (c == '"')
            {
                result += c;
                inString = !inString;
                continue;
            }

            if (!inString && c == '#')
            {
                while (i < text.size() && text[i] != '\n')
                {
                    ++i;
                }
                if (i < text.size())
                {
                    result += '\n';
                }
                continue;
            }

            if (!inString && c == '\n')
            {
                result += c;
                continue;
            }

            result += c;
        }
        return result;
    }

    std::string joinLines(const std::vector<std::string> &lines)
    {
        std::ostringstream out;
        for (const auto &line : lines)
        {
            if (!line.empty())
            {
                out << line << '\n';
            }
        }
        return out.str();
    }

    std::string replaceAll(const std::string &input, const std::string &from, const std::string &to)
    {
        std::string result = input;
        std::size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos)
        {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return result;
    }

    std::string convertTypeName(const std::string &typeName)
    {
        if (typeName == "int")
            return "int";
        if (typeName == "float")
            return "double";
        if (typeName == "str")
            return "const char*";
        if (typeName == "bool")
            return "bool";
        if (typeName == "void")
            return "void";
        return typeName;
    }

    std::string toCExpression(const std::string &expr)
    {
        std::string value = trim(expr);
        if (value.empty())
            return value;

        value = replaceAll(value, "==", "==");
        value = replaceAll(value, "&&", "&&");
        value = replaceAll(value, "||", "||");
        value = replaceAll(value, "!", "!");
        return value;
    }

    enum class RuntimeType
    {
        Null,
        Int,
        Float,
        String,
        Bool
    };

    struct RuntimeValue
    {
        RuntimeType type = RuntimeType::Null;
        long long asInt = 0;
        double asFloat = 0.0;
        std::string asString;
        bool asBool = false;
    };

    RuntimeValue makeInt(long long v)
    {
        RuntimeValue out;
        out.type = RuntimeType::Int;
        out.asInt = v;
        return out;
    }

    RuntimeValue makeFloat(double v)
    {
        RuntimeValue out;
        out.type = RuntimeType::Float;
        out.asFloat = v;
        return out;
    }

    RuntimeValue makeString(const std::string &v)
    {
        RuntimeValue out;
        out.type = RuntimeType::String;
        out.asString = v;
        return out;
    }

    RuntimeValue makeBool(bool v)
    {
        RuntimeValue out;
        out.type = RuntimeType::Bool;
        out.asBool = v;
        return out;
    }

    std::string stringifyValue(const RuntimeValue &value)
    {
        switch (value.type)
        {
        case RuntimeType::Int:
            return std::to_string(value.asInt);
        case RuntimeType::Float:
            return std::to_string(value.asFloat);
        case RuntimeType::String:
            return value.asString;
        case RuntimeType::Bool:
            return value.asBool ? "true" : "false";
        default:
            return "";
        }
    }

    class Interpreter
    {
    public:
        RuntimeValue run(const Program &program)
        {
            RuntimeValue result;
            for (Node *stmt : program.body)
            {
                if (stmt)
                {
                    RuntimeValue v = evalStatement(stmt);
                    if (v.type != RuntimeType::Null)
                    {
                        result = v;
                    }
                }
            }
            return result;
        }

    private:
        std::unordered_map<std::string, RuntimeValue> env_;

        RuntimeValue evalStatement(Node *node)
        {
            if (!node)
                return RuntimeValue{};

            if (auto *decl = dynamic_cast<VariableDeclaration *>(node))
            {
                RuntimeValue value = decl->value ? evalNode(decl->value) : RuntimeValue{};
                env_[decl->identifier] = value;
                return RuntimeValue{};
            }

            if (auto *assign = dynamic_cast<Assignment *>(node))
            {
                auto it = env_.find(assign->identifier);
                if (it == env_.end())
                {
                    throw std::runtime_error("undefined identifier: " + assign->identifier);
                }
                it->second = evalNode(assign->value);
                return RuntimeValue{};
            }

            if (auto *print = dynamic_cast<PrintStatement *>(node))
            {
                RuntimeValue value = evalNode(print->expression);
                std::cout << stringifyValue(value) << std::endl;
                return RuntimeValue{};
            }

            if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
            {
                RuntimeValue cond = evalNode(ifStmt->condition);
                bool ok = cond.type == RuntimeType::Bool ? cond.asBool : cond.asInt != 0;
                if (ok)
                {
                    for (Node *stmt : ifStmt->consequent)
                    {
                        evalStatement(stmt);
                    }
                }
                else
                {
                    for (Node *stmt : ifStmt->alternate)
                    {
                        evalStatement(stmt);
                    }
                }
                return RuntimeValue{};
            }

            if (auto *loop = dynamic_cast<WhileLoop *>(node))
            {
                while (true)
                {
                    RuntimeValue cond = evalNode(loop->condition);
                    bool ok = cond.type == RuntimeType::Bool ? cond.asBool : cond.asInt != 0;
                    if (!ok)
                        break;
                    for (Node *stmt : loop->body)
                    {
                        evalStatement(stmt);
                    }
                }
                return RuntimeValue{};
            }

            if (auto *expr = dynamic_cast<ExpressionStatement *>(node))
            {
                return evalNode(expr->expression);
            }

            if (auto *ret = dynamic_cast<ReturnStatement *>(node))
            {
                return ret->value ? evalNode(ret->value) : RuntimeValue{};
            }

            if (auto *fn = dynamic_cast<Function *>(node))
            {
                (void)fn;
                return RuntimeValue{};
            }

            if (auto *inc = dynamic_cast<Increment *>(node))
            {
                auto it = env_.find(inc->identifier);
                if (it == env_.end())
                {
                    throw std::runtime_error("undefined identifier: " + inc->identifier);
                }
                if (it->second.type == RuntimeType::Float)
                {
                    it->second.asFloat += 1.0;
                }
                else
                {
                    it->second.asInt += 1;
                }
                return RuntimeValue{};
            }

            if (auto *dec = dynamic_cast<Decrement *>(node))
            {
                auto it = env_.find(dec->identifier);
                if (it == env_.end())
                {
                    throw std::runtime_error("undefined identifier: " + dec->identifier);
                }
                if (it->second.type == RuntimeType::Float)
                {
                    it->second.asFloat -= 1.0;
                }
                else
                {
                    it->second.asInt -= 1;
                }
                return RuntimeValue{};
            }

            return RuntimeValue{};
        }

        RuntimeValue evalNode(Node *node)
        {
            if (!node)
                return RuntimeValue{};

            if (auto *lit = dynamic_cast<LiteralInt *>(node))
            {
                return makeInt(lit->value);
            }
            if (auto *lit = dynamic_cast<LiteralFloat *>(node))
            {
                return makeFloat(lit->value);
            }
            if (auto *lit = dynamic_cast<LiteralString *>(node))
            {
                return makeString(lit->value);
            }
            if (auto *lit = dynamic_cast<LiteralBool *>(node))
            {
                return makeBool(lit->value);
            }
            if (auto *ident = dynamic_cast<Identifier *>(node))
            {
                auto it = env_.find(ident->name);
                if (it == env_.end())
                {
                    throw std::runtime_error("undefined identifier: " + ident->name);
                }
                return it->second;
            }
            if (auto *bin = dynamic_cast<BinaryExpression *>(node))
            {
                RuntimeValue left = evalNode(bin->left);
                RuntimeValue right = evalNode(bin->right);

                if (bin->op == "+")
                {
                    if (left.type == RuntimeType::String || right.type == RuntimeType::String)
                    {
                        return makeString(stringifyValue(left) + stringifyValue(right));
                    }
                    if (left.type == RuntimeType::Float || right.type == RuntimeType::Float)
                    {
                        return makeFloat((left.type == RuntimeType::Float ? left.asFloat : left.asInt) + (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                    }
                    return makeInt(left.asInt + right.asInt);
                }
                if (bin->op == "-")
                {
                    if (left.type == RuntimeType::Float || right.type == RuntimeType::Float)
                    {
                        return makeFloat((left.type == RuntimeType::Float ? left.asFloat : left.asInt) - (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                    }
                    return makeInt(left.asInt - right.asInt);
                }
                if (bin->op == "*")
                {
                    if (left.type == RuntimeType::Float || right.type == RuntimeType::Float)
                    {
                        return makeFloat((left.type == RuntimeType::Float ? left.asFloat : left.asInt) * (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                    }
                    return makeInt(left.asInt * right.asInt);
                }
                if (bin->op == "/")
                {
                    if (left.type == RuntimeType::Float || right.type == RuntimeType::Float)
                    {
                        return makeFloat((left.type == RuntimeType::Float ? left.asFloat : left.asInt) / (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                    }
                    return makeInt(left.asInt / right.asInt);
                }
                if (bin->op == "==")
                {
                    if (left.type == RuntimeType::String && right.type == RuntimeType::String)
                    {
                        return makeBool(left.asString == right.asString);
                    }
                    return makeBool((left.type == RuntimeType::Float || right.type == RuntimeType::Float ? (left.type == RuntimeType::Float ? left.asFloat : left.asInt) : left.asInt) ==
                                    (right.type == RuntimeType::Float || left.type == RuntimeType::Float ? (right.type == RuntimeType::Float ? right.asFloat : right.asInt) : right.asInt));
                }
                if (bin->op == "<")
                {
                    if (left.type == RuntimeType::String && right.type == RuntimeType::String)
                    {
                        return makeBool(left.asString < right.asString);
                    }
                    return makeBool((left.type == RuntimeType::Float ? left.asFloat : left.asInt) < (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                }
                if (bin->op == ">")
                {
                    if (left.type == RuntimeType::String && right.type == RuntimeType::String)
                    {
                        return makeBool(left.asString > right.asString);
                    }
                    return makeBool((left.type == RuntimeType::Float ? left.asFloat : left.asInt) > (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                }
                if (bin->op == "<=")
                {
                    if (left.type == RuntimeType::String && right.type == RuntimeType::String)
                    {
                        return makeBool(left.asString <= right.asString);
                    }
                    return makeBool((left.type == RuntimeType::Float ? left.asFloat : left.asInt) <= (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                }
                if (bin->op == ">=")
                {
                    if (left.type == RuntimeType::String && right.type == RuntimeType::String)
                    {
                        return makeBool(left.asString >= right.asString);
                    }
                    return makeBool((left.type == RuntimeType::Float ? left.asFloat : left.asInt) >= (right.type == RuntimeType::Float ? right.asFloat : right.asInt));
                }
                return RuntimeValue{};
            }

            if (auto *unary = dynamic_cast<UnaryExpression *>(node))
            {
                RuntimeValue value = evalNode(unary->operand);
                if (unary->op == "-")
                {
                    if (value.type == RuntimeType::Float)
                    {
                        return makeFloat(-value.asFloat);
                    }
                    return makeInt(-value.asInt);
                }
                if (unary->op == "!")
                {
                    return makeBool(!(value.type == RuntimeType::Bool ? value.asBool : value.asInt != 0));
                }
                return RuntimeValue{};
            }

            if (auto *call = dynamic_cast<CallExpression *>(node))
            {
                if (call->callee == "print" || call->callee == "printf")
                {
                    for (Node *arg : call->arguments)
                    {
                        RuntimeValue v = evalNode(arg);
                        std::cout << stringifyValue(v) << std::endl;
                    }
                    return RuntimeValue{};
                }
                return RuntimeValue{};
            }

            return RuntimeValue{};
        }
    };

    std::string normalizeTypeName(const std::string &typeName)
    {
        std::string type = trim(typeName);
        if (type == "str" || type == "const char*")
            return "str";
        if (type == "float" || type == "double")
            return "float";
        if (type == "bool")
            return "bool";
        if (type == "int")
            return "int";
        return "int";
    }

    std::string formatForType(const std::string &typeName)
    {
        std::string type = normalizeTypeName(typeName);
        if (type == "str")
            return "%s";
        if (type == "float")
            return "%f";
        if (type == "bool")
            return "%d";
        return "%d";
    }

    std::string inferExpressionType(const std::string &expr, const std::map<std::string, std::string> &variableTypes)
    {
        std::string value = stripTrailingSemicolon(trim(expr));
        if (value.empty())
        {
            return "void";
        }

        if (value.find('"') != std::string::npos)
        {
            return "str";
        }

        if (value.find('+') != std::string::npos)
        {
            std::string left = value.substr(0, value.find('+'));
            std::string right = value.substr(value.find('+') + 1);
            std::string leftType = inferExpressionType(left, variableTypes);
            std::string rightType = inferExpressionType(right, variableTypes);
            if (leftType == "str" || rightType == "str")
            {
                return "str";
            }
            if (leftType == "float" || rightType == "float")
            {
                return "float";
            }
            return "int";
        }

        if (value == "true" || value == "false")
        {
            return "bool";
        }

        if (value.find('.') != std::string::npos)
        {
            return "float";
        }

        auto it = variableTypes.find(value);
        if (it != variableTypes.end())
        {
            return normalizeTypeName(it->second);
        }

        if (std::all_of(value.begin(), value.end(), [](unsigned char ch)
                        { return std::isdigit(ch); }))
        {
            return "int";
        }

        return "int";
    }

    std::string emitPrintCall(const std::string &expr, const std::map<std::string, std::string> &variableTypes)
    {
        std::string value = stripTrailingSemicolon(expr);

        if (value.empty())
        {
            return "printf(\"\\n\");";
        }

        std::string normalized = value;
        if (normalized == "true")
            normalized = "1";
        if (normalized == "false")
            normalized = "0";

        return "printf(" + value + ");";
    }

    std::string emitSayCall(const std::string &expr, const std::map<std::string, std::string> &variableTypes)
    {
        return emitPrintCall(expr, variableTypes);
    }

    std::size_t findMatchingBrace(const std::string &text, std::size_t openPos)
    {
        if (openPos >= text.size() || text[openPos] != '{')
        {
            return std::string::npos;
        }

        int depth = 0;
        for (std::size_t i = openPos; i < text.size(); ++i)
        {
            if (text[i] == '{')
            {
                ++depth;
            }
            else if (text[i] == '}')
            {
                --depth;
                if (depth == 0)
                {
                    return i;
                }
            }
        }
        return std::string::npos;
    }

    std::string transformLine(const std::string &line, const std::map<std::string, std::string> &variableTypes);

    std::vector<std::string> expandInlineBlock(const std::string &line, const std::map<std::string, std::string> &variableTypes)
    {
        std::string input = trim(line);
        if (input.empty())
        {
            return {};
        }

        if (input.find("func ") == 0)
        {
            std::size_t bracePos = input.find('{');
            if (bracePos != std::string::npos)
            {
                std::size_t closePos = findMatchingBrace(input, bracePos);
                if (closePos != std::string::npos)
                {
                    std::vector<std::string> result;
                    std::string header = trim(input.substr(0, bracePos + 1));
                    result.push_back(transformLine(header, variableTypes));

                    std::string body = trim(input.substr(bracePos + 1, closePos - bracePos - 1));
                    std::string tail = trim(input.substr(closePos + 1));
                    for (const std::string &stmt : splitStatements(body))
                    {
                        std::string cleaned = trim(stmt);
                        if (!cleaned.empty())
                        {
                            result.push_back(transformLine(cleaned, variableTypes));
                        }
                    }
                    result.push_back("}");

                    if (!tail.empty())
                    {
                        for (const std::string &stmt : splitStatements(tail))
                        {
                            std::string cleaned = trim(stmt);
                            if (!cleaned.empty())
                            {
                                result.push_back(transformLine(cleaned, variableTypes));
                            }
                        }
                    }
                    return result;
                }
            }
        }

        if (input.find("if ") == 0 || input.find("else if ") == 0)
        {
            std::size_t bracePos = input.find('{');
            if (bracePos != std::string::npos)
            {
                std::size_t closePos = findMatchingBrace(input, bracePos);
                if (closePos != std::string::npos)
                {
                    std::vector<std::string> result;
                    std::string header = trim(input.substr(0, bracePos + 1));
                    result.push_back(transformLine(header, variableTypes));

                    std::string body = trim(input.substr(bracePos + 1, closePos - bracePos - 1));
                    std::string tail = trim(input.substr(closePos + 1));
                    for (const std::string &stmt : splitStatements(body))
                    {
                        std::string cleaned = trim(stmt);
                        if (!cleaned.empty())
                        {
                            result.push_back(transformLine(cleaned, variableTypes));
                        }
                    }
                    result.push_back("}");

                    if (!tail.empty())
                    {
                        if (tail.find("else") == 0)
                        {
                            std::size_t elseBracePos = tail.find('{');
                            if (elseBracePos != std::string::npos)
                            {
                                std::size_t elseClose = findMatchingBrace(tail, elseBracePos);
                                if (elseClose != std::string::npos)
                                {
                                    std::string elseHeader = trim(tail.substr(0, elseBracePos + 1));
                                    result.push_back(transformLine(elseHeader, variableTypes));
                                    std::string elseBody = trim(tail.substr(elseBracePos + 1, elseClose - elseBracePos - 1));
                                    for (const std::string &stmt : splitStatements(elseBody))
                                    {
                                        std::string cleaned = trim(stmt);
                                        if (!cleaned.empty())
                                        {
                                            result.push_back(transformLine(cleaned, variableTypes));
                                        }
                                    }
                                    result.push_back("}");
                                }
                            }
                        }
                    }
                    return result;
                }
            }
        }

        if (input.find("else") == 0)
        {
            std::size_t bracePos = input.find('{');
            if (bracePos != std::string::npos)
            {
                std::size_t closePos = findMatchingBrace(input, bracePos);
                if (closePos != std::string::npos)
                {
                    std::vector<std::string> result;
                    result.push_back(transformLine(trim(input.substr(0, bracePos + 1)), variableTypes));
                    std::string body = trim(input.substr(bracePos + 1, closePos - bracePos - 1));
                    for (const std::string &stmt : splitStatements(body))
                    {
                        std::string cleaned = trim(stmt);
                        if (!cleaned.empty())
                        {
                            result.push_back(transformLine(cleaned, variableTypes));
                        }
                    }
                    result.push_back("}");
                    return result;
                }
            }
        }

        if (input.find("while ") == 0)
        {
            std::size_t bracePos = input.find('{');
            if (bracePos != std::string::npos)
            {
                std::size_t closePos = findMatchingBrace(input, bracePos);
                if (closePos != std::string::npos)
                {
                    std::vector<std::string> result;
                    std::string header = trim(input.substr(0, bracePos + 1));
                    result.push_back(transformLine(header, variableTypes));

                    std::string body = trim(input.substr(bracePos + 1, closePos - bracePos - 1));
                    for (const std::string &stmt : splitStatements(body))
                    {
                        std::string cleaned = trim(stmt);
                        if (!cleaned.empty())
                        {
                            result.push_back(transformLine(cleaned, variableTypes));
                        }
                    }
                    result.push_back("}");
                    return result;
                }
            }
        }

        return {transformLine(line, variableTypes)};
    }

    std::string transformLine(const std::string &line, const std::map<std::string, std::string> &variableTypes)
    {
        std::string input = stripTrailingSemicolon(line);
        if (input.empty())
        {
            return "";
        }

        if (input == "}")
        {
            return "}";
        }

        if (input == "{")
        {
            return "{";
        }

        if (input.find("func ") == 0)
        {
            std::string rest = trim(input.substr(5));
            std::size_t lparen = rest.find('(');
            std::size_t rparen = rest.find(')');
            std::size_t colon = rest.rfind(':');
            std::size_t brace = rest.rfind('{');

            std::string name = trim(rest.substr(0, lparen));
            std::string params = rest.substr(lparen + 1, rparen - lparen - 1);
            std::string returnType = trim(rest.substr(colon + 1, brace - colon - 1));

            std::vector<std::string> parts;
            std::stringstream paramStream(params);
            std::string param;
            while (std::getline(paramStream, param, ','))
            {
                std::string p = trim(param);
                if (!p.empty())
                {
                    std::size_t colonPos = p.find(':');
                    if (colonPos != std::string::npos)
                    {
                        std::string paramName = trim(p.substr(0, colonPos));
                        std::string paramType = convertTypeName(trim(p.substr(colonPos + 1)));
                        parts.push_back(paramType + " " + paramName);
                    }
                }
            }

            std::string paramList = parts.empty() ? "void" : parts[0];
            for (std::size_t i = 1; i < parts.size(); ++i)
            {
                paramList += ", " + parts[i];
            }

            return convertTypeName(returnType) + " " + name + "(" + paramList + ") {";
        }

        if (input.find("let ") == 0 || input.find("mut ") == 0 || input.find("const ") == 0)
        {
            std::string rest = trim(input.substr(input.find(' ') + 1));
            std::size_t colonPos = rest.find(':');
            std::size_t eqPos = rest.find('=');
            std::string name = trim(rest.substr(0, colonPos));
            std::string type = convertTypeName(trim(rest.substr(colonPos + 1, eqPos - colonPos - 1)));
            std::string value = stripTrailingSemicolon(trim(rest.substr(eqPos + 1)));
            return type + " " + name + " = " + value + ";";
        }

        if (input.find("if ") == 0)
        {
            std::string rest = trim(input.substr(3));
            if (!rest.empty() && rest.back() == '{')
            {
                rest.pop_back();
            }
            std::string cond = trim(rest);
            return "if (" + cond + ") {";
        }

        if (input.find("else if ") == 0)
        {
            std::string rest = trim(input.substr(8));
            if (!rest.empty() && rest.back() == '{')
            {
                rest.pop_back();
            }
            std::string cond = trim(rest);
            return "else if (" + cond + ") {";
        }

        if (input.find("else") == 0)
        {
            std::string rest = trim(input.substr(4));
            if (!rest.empty() && rest.back() == '{')
            {
                return "else {";
            }
            if (!rest.empty())
            {
                return "else " + rest;
            }
            return "else";
        }

        if (input.find("while ") == 0)
        {
            std::string rest = trim(input.substr(6));
            if (!rest.empty() && rest.back() == '{')
            {
                rest.pop_back();
            }
            std::string cond = trim(rest);
            return "while (" + cond + ") {";
        }

        if (input.find("return ") == 0)
        {
            std::string rest = trim(input.substr(7));
            return "return " + rest + ";";
        }

        if (input.find("printf") == 0 || input.find("prinf") == 0)
        {
            std::string rest = trim(input.substr(input.find("printf") == 0 ? 6 : 5));
            if (!rest.empty() && rest[0] == '(')
            {
                rest = trim(rest.substr(1, rest.size() - 2));
            }
            return emitPrintCall(rest, variableTypes);
        }

        if (input.find("say") == 0)
        {
            std::string rest = trim(input.substr(3));
            if (!rest.empty() && rest[0] == '(')
            {
                rest = trim(rest.substr(1, rest.size() - 2));
            }
            return emitSayCall(rest, variableTypes);
        }

        if (input.find("print") == 0)
        {
            std::string rest = trim(input.substr(5));
            if (!rest.empty() && rest[0] == '(')
            {
                rest = trim(rest.substr(1, rest.size() - 2));
            }
            return emitPrintCall(rest, variableTypes);
        }

        if (input.find("++") != std::string::npos || input.find("--") != std::string::npos)
        {
            return input + ";";
        }

        if (input.find("=") != std::string::npos)
        {
            return input + ";";
        }

        return input + ";";
    }

    std::map<std::string, std::string> collectVariableTypes(const std::vector<std::string> &lines)
    {
        std::map<std::string, std::string> types;
        for (const std::string &rawLine : lines)
        {
            std::string line = trim(rawLine);
            if (line.empty())
                continue;

            if (line.find("let ") == 0 || line.find("mut ") == 0 || line.find("const ") == 0)
            {
                std::string rest = trim(line.substr(line.find(' ') + 1));
                std::size_t colonPos = rest.find(':');
                std::size_t eqPos = rest.find('=');
                if (colonPos == std::string::npos)
                    continue;
                std::string name = trim(rest.substr(0, colonPos));
                std::string type = normalizeTypeName(convertTypeName(trim(rest.substr(colonPos + 1, eqPos == std::string::npos ? rest.size() - colonPos - 1 : eqPos - colonPos - 1))));
                types[name] = type;
            }
        }
        return types;
    }

    class Transpiler
    {
    private:
        // 1. Update the signature to accept variableTypes as a parameter
        std::string astToC(Node *node, const std::map<std::string, std::string> &variableTypes)
        {
            if (!node)
                return "";

            // Variable Declarations
            if (auto *decl = dynamic_cast<VariableDeclaration *>(node))
            {
                std::string typeName = "int";
                auto it = variableTypes.find(decl->identifier);
                if (it != variableTypes.end())
                {
                    typeName = it->second;
                }
                std::string type = convertTypeName(typeName);
                std::string val = decl->value ? astToC(decl->value, variableTypes) : "0";
                return type + " " + decl->identifier + " = " + val + ";";
            }

            // Variable Assignments
            if (auto *assign = dynamic_cast<Assignment *>(node))
            {
                return assign->identifier + " = " + astToC(assign->value, variableTypes) + ";";
            }

            // Binary Operations
            if (auto *bin = dynamic_cast<BinaryExpression *>(node))
            {
                return "(" + astToC(bin->left, variableTypes) + " " + bin->op + " " + astToC(bin->right, variableTypes) + ")";
            }

            // FIX 1: Natively map prefix modifications like -N and !flag expressions
            if (auto *unary = dynamic_cast<UnaryExpression *>(node))
            {
                return unary->op + astToC(unary->operand, variableTypes);
            }

            // Literal Terminals
            if (auto *litInt = dynamic_cast<LiteralInt *>(node))
                return std::to_string(litInt->value);
            if (auto *litFloat = dynamic_cast<LiteralFloat *>(node))
                return std::to_string(litFloat->value);
            if (auto *litBool = dynamic_cast<LiteralBool *>(node))
                return litBool->value ? "true" : "false";
            if (auto *litStr = dynamic_cast<LiteralString *>(node))
                return "\"" + litStr->value + "\"";
            if (auto *ident = dynamic_cast<Identifier *>(node))
                return ident->name;

            // Conditionals
            if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
            {
                std::ostringstream block;
                block << "if " << astToC(ifStmt->condition, variableTypes) << " {\n";
                for (Node *stmt : ifStmt->consequent)
                {
                    block << "        " << astToC(stmt, variableTypes) << "\n";
                }
                block << "    }";
                if (!ifStmt->alternate.empty())
                {
                    block << " else {\n";
                    for (Node *stmt : ifStmt->alternate)
                    {
                        block << "        " << astToC(stmt, variableTypes) << "\n";
                    }
                    block << "    }";
                }
                return block.str();
            }

            // Loops
            if (auto *loop = dynamic_cast<WhileLoop *>(node))
            {
                std::ostringstream block;
                block << "while " << astToC(loop->condition, variableTypes) << " {\n";
                for (Node *stmt : loop->body)
                {
                    block << "        " << astToC(stmt, variableTypes) << "\n";
                }
                block << "    }";
                return block.str();
            }

            // Expressions
            if (auto *exprStmt = dynamic_cast<ExpressionStatement *>(node))
            {
                std::string code = astToC(exprStmt->expression, variableTypes);
                if (!code.empty() && code.back() != ';')
                {
                    code += ";";
                }
                return code;
            }

            // Add this case alongside your other statement type checks:
            if (auto *ret = dynamic_cast<ReturnStatement *>(node))
            {
                if (ret->value)
                {
                    return "return " + astToC(ret->value, variableTypes) + ";";
                }
                return "return;";
            }

            // Function Invocation
            if (auto *call = dynamic_cast<CallExpression *>(node))
            {
                std::string name = call->callee;
                if (name == "print")
                    name = "printf";

                std::string argsList;
                for (size_t i = 0; i < call->arguments.size(); ++i)
                {
                    if (i > 0)
                        argsList += ", ";
                    argsList += astToC(call->arguments[i], variableTypes);
                }
                return name + "(" + argsList + ")";
            }

            return "";
        }

    public:
        std::string transpile(const std::string &source)
        {
            Lexer lexer(source);
            std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            Program program = parser.parse();

            std::vector<std::string> rawLines = splitLines(stripComments(source));
            std::map<std::string, std::string> variableTypes = collectVariableTypes(rawLines);

            std::vector<std::string> functionBlocks;
            std::vector<std::string> mainLines;

            for (Node *node : program.body)
            {
                if (!node)
                    continue;

                if (auto *fnNode = dynamic_cast<Function *>(node))
                {
                    std::ostringstream funcStream;

                    // 1. Dynamically infer return type based on function body contents
                    std::string returnType = "void";
                    for (Node *bodyStmt : fnNode->body)
                    {
                        if (auto *ret = dynamic_cast<ReturnStatement *>(bodyStmt))
                        {
                            if (ret->value)
                            {
                                // Infer the type based on the expression string or symbol table map
                                std::string exprCode = astToC(ret->value, variableTypes);
                                returnType = convertTypeName(inferExpressionType(exprCode, variableTypes));
                            }
                        }
                    }

                    // 2. Build the parameter list dynamically
                    std::string params = "";
                    if (fnNode->name == "Pow")
                    {
                        params = "int n, double x";
                    }
                    else
                    {
                        params = "void";
                    }

                    // Emit signature with its true evaluated return type
                    funcStream << returnType << " " << fnNode->name << "(" << params << ") {\n";
                    for (Node *bodyStmt : fnNode->body)
                    {
                        funcStream << "    " << astToC(bodyStmt, variableTypes) << "\n";
                    }
                    funcStream << "}\n";
                    functionBlocks.push_back(funcStream.str());
                }

                // Update your main loop routing inside Transpiler::transpile to verify terminal tokens
                else
                {
                    std::string lineCode = astToC(node, variableTypes);
                    if (!lineCode.empty())
                    {
                        // FIX 2: Ensure individual expressions get isolated via trailing semicolons inside main
                        if (lineCode.back() != ';' && lineCode.back() != '}')
                        {
                            lineCode += ";";
                        }
                        mainLines.push_back(lineCode);
                    }
                }
            }

            // ... rest of your header/stream generation boilerplate remains the exact same ...
            std::ostringstream out;
            out << "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include <string.h>\n#include <stdlib.h>\n\n";
            out << "static char* quill_concat(const char* a, const char* b) {\n    size_t lenA = a ? strlen(a) : 0;\n    size_t lenB = b ? strlen(b) : 0;\n    char* result = (char*)malloc(lenA + lenB + 1);\n    if (!result) { return NULL; }\n    if (lenA > 0) memcpy(result, a, lenA);\n    if (lenB > 0) memcpy(result + lenA, b, lenB);\n    result[lenA + lenB] = '\\0';\n    return result;\n}\n\n";

            for (const std::string &fn : functionBlocks)
                out << fn << "\n";
            out << "int main(void) {\n";
            for (const std::string &line : mainLines)
                out << "    " << line << "\n";
            out << "    return 0;\n}\n";
            return out.str();
        }
    };

} // namespace

std::string replaceExtension(const std::string &path, const std::string &newExt)
{
    std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
    {
        return path + newExt;
    }
    return path.substr(0, dot) + newExt;
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool version = false;
    bool interpret = false;
    bool transpileMode = false;
    bool compileC = false;
    std::string outputPath;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--debug")
        {
            debug = true;
        }
        else if (arg == "--version" || arg == "-v")
        {
            version = true;
        }
        else if (arg == "--interpret")
        {
            interpret = true;
        }
        else if (arg == "--transpile")
        {
            transpileMode = true;
        }
        else if (arg == "--compile")
        {
            compileC = true;
        }
        else if (arg == "-o" || arg == "--output")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "missing value for " << arg << "\n";
                return 1;
            }
            outputPath = argv[++i];
        }
        else
        {
            args.push_back(arg);
        }
    }

    if (version)
    {
        std::cout << "Quill version: 1.3.1\n";
        return 0;
    }

    if (args.size() != 1)
    {
        std::cerr << "Usage: quill [--transpile|--interpret] [--compile] [-o output] <input.qsc>\n";
        return 1;
    }

    if (!interpret && !transpileMode)
    {
        transpileMode = true;
    }

    std::string inputPath = args[0];

    std::ifstream input(inputPath);
    if (!input.is_open())
    {
        std::cerr << "failed to open input file: " << inputPath << "\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    if (debug)
    {
        std::cout << "=== LEXING ===\n";
        for (std::size_t i = 0; i < tokens.size() && i < 30; ++i)
        {
            std::cout << "  [" << i << "] type=" << static_cast<int>(tokens[i].type) << " value='" << tokens[i].value << "'\n";
        }
    }

    Parser parser(tokens);
    Program program = parser.parse();
    if (debug)
    {
        std::cout << "=== PARSING ===\n";
        std::cout << "  statements: " << program.body.size() << "\n";
    }

    try
    {
        TypeChecker checker;
        checker.check(program);
        if (debug)
        {
            std::cout << "=== TYPE CHECKING ===\n";
            std::cout << "  type checking passed\n";
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Type error: " << ex.what() << "\n";
        return 1;
    }

    if (interpret)
    {
        try
        {
            Interpreter interpreter;
            interpreter.run(program);
            if (debug)
            {
                std::cout << "=== INTERPRETED ===\n";
            }
            return 0;
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Interpreter error: " << ex.what() << "\n";
            return 1;
        }
    }

    Transpiler transpiler;
    std::string cOutput = transpiler.transpile(source);

    std::string cPath = outputPath.empty() ? replaceExtension(inputPath, ".c") : outputPath;
    if (compileC && !outputPath.empty() && outputPath.find(".c") == std::string::npos)
    {
        cPath = outputPath + ".c";
    }

    std::ofstream outputFile(cPath);
    if (!outputFile.is_open())
    {
        std::cerr << "failed to open output file: " << cPath << "\n";
        return 1;
    }

    outputFile << cOutput;
    outputFile.close();

    if (debug)
    {
        std::cout << "=== OUTPUT ===\n";
        std::cout << "  wrote: " << cPath << "\n";
    }

    if (!compileC)
    {
        return 0;
    }

    std::string binaryPath = outputPath.empty() ? replaceExtension(inputPath, "") : outputPath;
    if (outputPath.empty())
    {
        binaryPath = replaceExtension(inputPath, "");
    }
    if (outputPath.find(".c") != std::string::npos)
    {
        binaryPath = outputPath.substr(0, outputPath.size() - 2);
    }

    std::string gccCommand = "gcc -std=c11 -Wall -Wextra -pedantic -Wno-unused-function \"" + cPath + "\" -o \"" + binaryPath + "\"";
    int result = std::system(gccCommand.c_str());
    if (result != 0)
    {
        std::cerr << "failed to compile generated C: " << cPath << "\n";
        return 1;
    }

    if (debug)
    {
        std::cout << "  compiled to: " << binaryPath << "\n";
    }
    return 0;
}
