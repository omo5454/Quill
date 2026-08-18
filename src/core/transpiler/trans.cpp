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
    std::string convertTypeName(const std::string &typeName)
    {
        if (typeName == "int")
            return "int";
        if (typeName == "float")
            return "double";
        if (typeName == "str")
            return "char*";
        if (typeName == "bool")
            return "bool";
        if (typeName == "void")
            return "void";
        return typeName;
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
    // Maps any spelling of a Quill/C type name to Quill's own canonical
    // name ("int", "float", "str", "bool", "void"). Unknown/future type
    // names fall back to "int" so a typo doesn't crash the transpiler --
    // the typechecker is responsible for catching genuine type errors.
    std::string normalizeTypeName(const std::string &typeName)
    {
        std::string type = trim(typeName);
        if (type == "str" || type == "char*")
            return "str";
        if (type == "float" || type == "double")
            return "float";
        if (type == "bool")
            return "bool";
        if (type == "int")
            return "int";
        if (type == "void")
            return "void";
        return "int";
    }

    // ------------------------------------------------------------------
    // Type inference and per-scope symbol tables.
    //
    // The old transpiler re-scanned the raw source text for "let"/"mut"
    // lines and built ONE flat, file-wide name -> type map. That has two
    // problems that only show up once real programs are written by real
    // people:
    //   1. Two functions that both happen to use a variable called `x`
    //      (or `i`, `result`, `total`, ...) stomp on each other's type,
    //      because there's only one map for the whole file.
    //   2. Function parameters were never added to that map at all, and
    //      a `let` with no explicit `: type` was skipped entirely, so
    //      both silently defaulted to `int` no matter what they held.
    //
    // Below, every function (and the top-level "main" body) gets its own
    // scope, built straight from the AST: parameters come from
    // Function::params, and each local's type comes from its own
    // declaredType if it wrote one, or is inferred from its initializer
    // if it didn't. Nothing is guessed from raw text.
    // ------------------------------------------------------------------

    std::string inferNodeType(Node *node,
                              const std::map<std::string, std::string> &scope,
                              const std::map<std::string, std::string> &functionReturnTypes)
    {
        if (!node)
            return "void";

        if (dynamic_cast<LiteralInt *>(node))
            return "int";
        if (dynamic_cast<LiteralFloat *>(node))
            return "float";
        if (dynamic_cast<LiteralString *>(node))
            return "str";
        if (dynamic_cast<LiteralBool *>(node))
            return "bool";

        if (auto *ident = dynamic_cast<Identifier *>(node))
        {
            auto it = scope.find(ident->name);
            // The typechecker already rejects genuinely undefined
            // identifiers before we ever get here, so a miss just means
            // "not tracked in this scope" -- fall back to int rather
            // than crash the transpiler.
            return it != scope.end() ? it->second : "int";
        }

        // Indexing a str with [] yields a character -- represented as
        // an int, the same way C represents char.
        if (dynamic_cast<IndexExpression *>(node))
            return "int";

        if (auto *bin = dynamic_cast<BinaryExpression *>(node))
        {
            static const std::vector<std::string> comparisonOps = {
                "==", "!=", "<", ">", "<=", ">=", "&&", "||"};
            if (std::find(comparisonOps.begin(), comparisonOps.end(), bin->op) != comparisonOps.end())
            {
                return "bool";
            }

            std::string leftType = inferNodeType(bin->left, scope, functionReturnTypes);
            std::string rightType = inferNodeType(bin->right, scope, functionReturnTypes);

            if (bin->op == "+" && (leftType == "str" || rightType == "str"))
            {
                return "str";
            }
            if (leftType == "float" || rightType == "float")
            {
                return "float";
            }
            return "int";
        }

        if (auto *unary = dynamic_cast<UnaryExpression *>(node))
        {
            if (unary->op == "!")
                return "bool";
            return inferNodeType(unary->operand, scope, functionReturnTypes);
        }

        if (auto *call = dynamic_cast<CallExpression *>(node))
        {
            auto it = functionReturnTypes.find(call->callee);
            return it != functionReturnTypes.end() ? it->second : "int";
        }

        return "int";
    }

    void collectDeclaredLocals(const std::vector<Node *> &stmts,
                               std::map<std::string, std::string> &scope,
                               const std::map<std::string, std::string> &functionReturnTypes)
    {
        for (Node *node : stmts)
        {
            if (!node)
                continue;

            if (auto *decl = dynamic_cast<VariableDeclaration *>(node))
            {
                std::string type;
                if (!decl->declaredType.empty())
                {
                    type = normalizeTypeName(decl->declaredType);
                }
                else
                {
                    type = inferNodeType(decl->value, scope, functionReturnTypes);
                }
                scope[decl->identifier] = type;
            }
            else if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
            {
                collectDeclaredLocals(ifStmt->consequent, scope, functionReturnTypes);
                collectDeclaredLocals(ifStmt->alternate, scope, functionReturnTypes);
            }
            else if (auto *loop = dynamic_cast<WhileLoop *>(node))
            {
                collectDeclaredLocals(loop->body, scope, functionReturnTypes);
            }
        }
    }

    // Builds one scope: parameters first, then every local declared
    // anywhere in the body (including inside if/while blocks -- this
    // matches how TypeChecker itself tracks symbols, so codegen and type
    // checking never disagree about what's in scope).
    std::map<std::string, std::string> buildScope(const std::vector<Param> &params,
                                                  const std::vector<Node *> &body,
                                                  const std::map<std::string, std::string> &functionReturnTypes)
    {
        std::map<std::string, std::string> scope;
        for (const Param &p : params)
        {
            scope[p.name] = p.type.empty() ? "int" : normalizeTypeName(p.type);
        }
        collectDeclaredLocals(body, scope, functionReturnTypes);
        return scope;
    }

    // Return-type annotations are optional in Quill (only parameter
    // types are required by the grammar). When a function omits one,
    // infer its return type from what it actually returns -- searching
    // inside if/while blocks too, not just the top level of the body,
    // since a `return` is very often written inside a branch.
    std::string inferFunctionReturnType(const std::vector<Node *> &body,
                                        const std::map<std::string, std::string> &scope,
                                        const std::map<std::string, std::string> &functionReturnTypes)
    {
        for (Node *node : body)
        {
            if (!node)
                continue;

            if (auto *ret = dynamic_cast<ReturnStatement *>(node))
            {
                if (ret->value)
                {
                    return inferNodeType(ret->value, scope, functionReturnTypes);
                }
            }
            else if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
            {
                std::string fromConsequent = inferFunctionReturnType(ifStmt->consequent, scope, functionReturnTypes);
                if (fromConsequent != "void")
                    return fromConsequent;
                std::string fromAlternate = inferFunctionReturnType(ifStmt->alternate, scope, functionReturnTypes);
                if (fromAlternate != "void")
                    return fromAlternate;
            }
            else if (auto *loop = dynamic_cast<WhileLoop *>(node))
            {
                std::string fromLoop = inferFunctionReturnType(loop->body, scope, functionReturnTypes);
                if (fromLoop != "void")
                    return fromLoop;
            }
        }
        return "void";
    }

    // Seeds the stdlib's known signatures, then resolves every
    // user-defined function's return type: the explicit annotation if
    // the student wrote one, otherwise inference from its `return`
    // statements. Walking program.body in order and adding each
    // function's type as we go (rather than computing them all
    // independently) means a function's own return-inference can see
    // the resolved types of every function declared before it --
    // exactly the "must be declared before called" rule Quill already
    // requires, so this never needs a forward reference.
    std::map<std::string, std::string> buildFunctionReturnTypes(const Program &program)
    {
        std::map<std::string, std::string> types;
        types["len"] = "int";
        types["toString"] = "str";

        for (Node *node : program.body)
        {
            if (auto *fn = dynamic_cast<Function *>(node))
            {
                if (!fn->returnType.empty())
                {
                    types[fn->name] = normalizeTypeName(fn->returnType);
                }
                else
                {
                    std::map<std::string, std::string> scope = buildScope(fn->params, fn->body, types);
                    types[fn->name] = inferFunctionReturnType(fn->body, scope, types);
                }
            }
        }
        return types;
    }

    // ------------------------------------------------------------------
    // AST -> C codegen.
    // ------------------------------------------------------------------

    std::string astToC(Node *node,
                       const std::map<std::string, std::string> &scope,
                       const std::map<std::string, std::string> &functionReturnTypes);

    // Renders `node` as a C expression that evaluates to a `const char*`,
    // converting non-string values through the small runtime helpers
    // emitted at the top of every generated file. This is what makes
    // `"score: " + score` (int + str) work, per the language tour.
    std::string toCStringExpr(Node *node,
                              const std::map<std::string, std::string> &scope,
                              const std::map<std::string, std::string> &functionReturnTypes)
    {
        std::string type = inferNodeType(node, scope, functionReturnTypes);
        std::string code = astToC(node, scope, functionReturnTypes);
        if (type == "str")
            return code;
        if (type == "float")
            return "quill_ftoa(" + code + ")";
        if (type == "bool")
            return "((" + code + ") ? \"true\" : \"false\")";
        return "quill_itoa(" + code + ")";
    }

    std::string astToC(Node *node,
                       const std::map<std::string, std::string> &scope,
                       const std::map<std::string, std::string> &functionReturnTypes)
    {
        if (!node)
            return "";

        // Variable declarations
        if (auto *decl = dynamic_cast<VariableDeclaration *>(node))
        {
            auto it = scope.find(decl->identifier);
            std::string typeName = it != scope.end() ? it->second : "int";
            std::string type = convertTypeName(typeName);
            std::string val = decl->value ? astToC(decl->value, scope, functionReturnTypes) : "0";
            return type + " " + decl->identifier + " = " + val + ";";
        }

        // Assignments
        if (auto *assign = dynamic_cast<Assignment *>(node))
        {
            return assign->identifier + " = " + astToC(assign->value, scope, functionReturnTypes) + ";";
        }

        // Binary operations -- '+' on anything involving a string becomes
        // a quill_concat() call instead of an invalid C '+' on pointers.
        if (auto *bin = dynamic_cast<BinaryExpression *>(node))
        {
            if (bin->op == "+")
            {
                std::string leftType = inferNodeType(bin->left, scope, functionReturnTypes);
                std::string rightType = inferNodeType(bin->right, scope, functionReturnTypes);
                if (leftType == "str" || rightType == "str")
                {
                    return "quill_concat(" +
                           toCStringExpr(bin->left, scope, functionReturnTypes) + ", " +
                           toCStringExpr(bin->right, scope, functionReturnTypes) + ")";
                }
            }
            return "(" + astToC(bin->left, scope, functionReturnTypes) + " " + bin->op + " " +
                   astToC(bin->right, scope, functionReturnTypes) + ")";
        }

        if (auto *unary = dynamic_cast<UnaryExpression *>(node))
        {
            return unary->op + astToC(unary->operand, scope, functionReturnTypes);
        }

        // Literals
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

        // s[i] -- indexing a str. Codegen is trivial since Quill strs
        // are already C's `const char*`, so this maps straight across.
        if (auto *idx = dynamic_cast<IndexExpression *>(node))
        {
            return astToC(idx->object, scope, functionReturnTypes) + "[" +
                   astToC(idx->index, scope, functionReturnTypes) + "]";
        }

        // say / printf -- picks the right printf() format from the
        // expression's inferred type instead of dropping the statement.
        if (auto *print = dynamic_cast<PrintStatement *>(node))
        {
            std::string exprCode = astToC(print->expression, scope, functionReturnTypes);
            std::string type = inferNodeType(print->expression, scope, functionReturnTypes);
            if (type == "str")
                return "printf(\"%s\\n\", " + exprCode + ");";
            if (type == "float")
                return "printf(\"%f\\n\", " + exprCode + ");";
            if (type == "bool")
                return "printf(\"%s\\n\", (" + exprCode + ") ? \"true\" : \"false\");";
            return "printf(\"%d\\n\", " + exprCode + ");";
        }

        // i++ / i--
        if (auto *inc = dynamic_cast<Increment *>(node))
        {
            return inc->identifier + "++;";
        }
        if (auto *dec = dynamic_cast<Decrement *>(node))
        {
            return dec->identifier + "--;";
        }

        // Conditionals
        if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
        {
            std::ostringstream block;
            block << "if " << astToC(ifStmt->condition, scope, functionReturnTypes) << " {\n";
            for (Node *stmt : ifStmt->consequent)
            {
                block << "        " << astToC(stmt, scope, functionReturnTypes) << "\n";
            }
            block << "    }";
            if (!ifStmt->alternate.empty())
            {
                block << " else {\n";
                for (Node *stmt : ifStmt->alternate)
                {
                    block << "        " << astToC(stmt, scope, functionReturnTypes) << "\n";
                }
                block << "    }";
            }
            return block.str();
        }

        // Loops
        if (auto *loop = dynamic_cast<WhileLoop *>(node))
        {
            std::ostringstream block;
            block << "while " << astToC(loop->condition, scope, functionReturnTypes) << " {\n";
            for (Node *stmt : loop->body)
            {
                block << "        " << astToC(stmt, scope, functionReturnTypes) << "\n";
            }
            block << "    }";
            return block.str();
        }

        // Expression statements
        if (auto *exprStmt = dynamic_cast<ExpressionStatement *>(node))
        {
            std::string code = astToC(exprStmt->expression, scope, functionReturnTypes);
            if (!code.empty() && code.back() != ';')
            {
                code += ";";
            }
            return code;
        }

        // return
        if (auto *ret = dynamic_cast<ReturnStatement *>(node))
        {
            if (ret->value)
            {
                return "return " + astToC(ret->value, scope, functionReturnTypes) + ";";
            }
            return "return;";
        }

        // Function calls, including the small stdlib (len, toString).
        // Adding a new stdlib function later means adding one branch
        // here (and its return type in buildFunctionReturnTypes above)
        // -- not touching anything else in the transpiler.
        if (auto *call = dynamic_cast<CallExpression *>(node))
        {
            std::string name = call->callee;

            std::vector<std::string> argCodes;
            std::vector<std::string> argTypes;
            for (Node *arg : call->arguments)
            {
                argCodes.push_back(astToC(arg, scope, functionReturnTypes));
                argTypes.push_back(inferNodeType(arg, scope, functionReturnTypes));
            }

            if (name == "print")
            {
                name = "printf";
            }
            else if (name == "len" && argCodes.size() == 1)
            {
                return "(int)strlen(" + argCodes[0] + ")";
            }
            else if (name == "input" && argCodes.size() == 1)
            {
                if (argTypes[0] == "str")
                    return "// string type not supported on input.";
                if (argTypes[0] == "int")
                    return "scanf(\"%d\", &" + argCodes[0] + ")";

                if (argTypes[0] == "float")
                    return "scanf(\"%lf\", &" + argCodes[0] + ")";

                return "scanf(\"%d\", &" + argCodes[0] + ")";
            }
            else if (name == "toString" && argCodes.size() == 1)
            {
                if (argTypes[0] == "str")
                    return argCodes[0];
                if (argTypes[0] == "float")
                    return "quill_ftoa(" + argCodes[0] + ")";
                if (argTypes[0] == "bool")
                    return "((" + argCodes[0] + ") ? \"true\" : \"false\")";
                return "quill_itoa(" + argCodes[0] + ")";
            }

            std::string argsList;
            for (std::size_t i = 0; i < argCodes.size(); ++i)
            {
                if (i > 0)
                    argsList += ", ";
                argsList += argCodes[i];
            }
            return name + "(" + argsList + ")";
        }

        return "";
    }

    class Transpiler
    {
    public:
        std::string transpile(const std::string &source)
        {
            Lexer lexer(source);
            std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            Program program = parser.parse();

            std::map<std::string, std::string> functionReturnTypes = buildFunctionReturnTypes(program);
            std::map<std::string, std::string> mainScope = buildScope({}, program.body, functionReturnTypes);

            std::vector<std::string> functionBlocks;
            std::vector<std::string> mainLines;

            for (Node *node : program.body)
            {
                if (!node)
                    continue;

                if (auto *fnNode = dynamic_cast<Function *>(node))
                {
                    std::map<std::string, std::string> scope =
                        buildScope(fnNode->params, fnNode->body, functionReturnTypes);

                    // Return type: already resolved once in
                    // buildFunctionReturnTypes -- the explicit annotation
                    // if the student wrote one, otherwise inferred from
                    // the function's own `return` statements. Reusing
                    // that single resolution (instead of recomputing it
                    // here) guarantees this signature always matches what
                    // call sites elsewhere assume the function returns.
                    std::string returnType = convertTypeName(functionReturnTypes.at(fnNode->name));

                    // Parameter list: built from fnNode->params for
                    // every function, not just one named "Pow".
                    std::string params;
                    for (std::size_t i = 0; i < fnNode->params.size(); ++i)
                    {
                        if (i > 0)
                            params += ", ";
                        params += convertTypeName(normalizeTypeName(fnNode->params[i].type)) +
                                  " " + fnNode->params[i].name;
                    }
                    if (params.empty())
                        params = "void";

                    std::ostringstream funcStream;
                    funcStream << returnType << " " << fnNode->name << "(" << params << ") {\n";
                    for (Node *bodyStmt : fnNode->body)
                    {
                        std::string stmtCode = astToC(bodyStmt, scope, functionReturnTypes);
                        if (!stmtCode.empty())
                        {
                            funcStream << "    " << stmtCode << "\n";
                        }
                    }
                    funcStream << "}\n";
                    functionBlocks.push_back(funcStream.str());
                }
                else
                {
                    std::string lineCode = astToC(node, mainScope, functionReturnTypes);
                    if (!lineCode.empty())
                    {
                        if (lineCode.back() != ';' && lineCode.back() != '}')
                        {
                            lineCode += ";";
                        }
                        mainLines.push_back(lineCode);
                    }
                }
            }

            std::ostringstream out;
            out << "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include <string.h>\n#include <stdlib.h>\n#include <ctype.h>\n\n";
            out << "static char* quill_concat(const char* a, const char* b) {\n    size_t lenA = a ? strlen(a) : 0;\n    size_t lenB = b ? strlen(b) : 0;\n    char* result = (char*)malloc(lenA + lenB + 1);\n    if (!result) { return NULL; }\n    if (lenA > 0) memcpy(result, a, lenA);\n    if (lenB > 0) memcpy(result + lenA, b, lenB);\n    result[lenA + lenB] = '\\0';\n    return result;\n}\n\n";
            out << "static char* quill_dup(const char* s) {\n    size_t len = strlen(s);\n    char* out = (char*)malloc(len + 1);\n    if (!out) { return NULL; }\n    memcpy(out, s, len + 1);\n    return out;\n}\n\n";
            out << "static char* quill_itoa(long long v) {\n    char buffer[32];\n    snprintf(buffer, sizeof(buffer), \"%lld\", v);\n    return quill_dup(buffer);\n}\n\n";
            out << "static char* quill_ftoa(double v) {\n    char buffer[64];\n    snprintf(buffer, sizeof(buffer), \"%f\", v);\n    return quill_dup(buffer);\n}\n\n";

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