#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
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
        if (typeName == "number")
            return "int";
        if (typeName == "double")
            return "double";
        if (typeName == "string")
            return "const char*";
        if (typeName == "bool")
            return "bool";
        if (typeName == "void")
            return "void";
        return typeName;
    }

    // Maps any spelling of a Quill/C type name to Quill's own canonical
    // name ("number", "double", "string", "bool", "void"). Unknown/future type
    // names fall back to "number" so a typo doesn't crash the transpiler --
    // the typechecker is responsible for catching genuine type errors.
    std::string normalizeTypeName(const std::string &typeName)
    {
        std::string type = trim(typeName);
        if (type == "string" || type == "const char*")
            return "string";
        if (type == "double" || type == "double")
            return "double";
        if (type == "bool")
            return "bool";
        if (type == "number")
            return "number";
        if (type == "void")
            return "void";
        return "number";
    }

    // --- array type helpers -------------------------------------------------
    // "number[5]"  -> fixed, size 5
    // "number[]"   -> dynamic
    bool isArrayTypeName(const std::string &t)
    {
        return t.find('[') != std::string::npos;
    }
    bool isDynamicArrayType(const std::string &t)
    {
        return t.size() >= 2 && t.find("[]") != std::string::npos;
    }
    std::string arrayElemType(const std::string &t)
    {
        auto b = t.find('[');
        if (b == std::string::npos)
            return t;
        return t.substr(0, b);
    }
    // C struct name for a dynamic array of element type
    std::string dynArrCType(const std::string &elem)
    {
        if (elem == "double")
            return "quill_arr_double";
        if (elem == "bool")
            return "quill_arr_bool";
        if (elem == "string")
            return "quill_arr_str";
        return "quill_arr_int"; // number
    }
    std::string dynArrPrefix(const std::string &elem)
    {
        if (elem == "double")
            return "quill_arr_double";
        if (elem == "bool")
            return "quill_arr_bool";
        if (elem == "string")
            return "quill_arr_str";
        return "quill_arr_int";
    }

    std::string inferNodeType(Node *node,
                              const std::map<std::string, std::string> &scope,
                              const std::map<std::string, std::string> &functionReturnTypes)
    {
        if (!node)
            return "void";

        if (dynamic_cast<LiteralInt *>(node))
            return "number";
        if (dynamic_cast<LiteralDoudle *>(node))
            return "double";
        if (dynamic_cast<LiteralString *>(node))
            return "string";
        if (dynamic_cast<LiteralBool *>(node))
            return "bool";

        if (auto *ident = dynamic_cast<Identifier *>(node))
        {
            auto it = scope.find(ident->name);
            return it != scope.end() ? it->second : "number";
        }

        // Indexing yields either a character code (indexing a plain
        // string -- always "number", same as C's char) or, for a real
        // array, the array's own element type ("string[3]" indexed
        // gives "string", "double[2]" gives "double", etc.) -- not
        // unconditionally "number", which was only ever correct back
        // when number[] was the only array element type in use.
        if (auto *idx = dynamic_cast<IndexExpression *>(node))
        {
            std::string objType = inferNodeType(idx->object, scope, functionReturnTypes);
            auto bracketPos = objType.find('[');
            if (bracketPos != std::string::npos)
            {
                return objType.substr(0, bracketPos);
            }
            return "number";
        }

        if (auto *lit = dynamic_cast<ArrayLiteral *>(node))
        {
            if (lit->elements.empty())
                return "number[]";
            std::string elem = inferNodeType(lit->elements[0], scope, functionReturnTypes);
            for (size_t i = 1; i < lit->elements.size(); ++i)
            {
                std::string t = inferNodeType(lit->elements[i], scope, functionReturnTypes);
                if (t != elem)
                {
                    if ((elem == "number" && t == "double") || (elem == "double" && t == "number"))
                        elem = "double";
                    else
                        return "unknown";
                }
            }
            return elem + "[]";
        }

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

            if (bin->op == "+" && (leftType == "string" || rightType == "string"))
            {
                return "string";
            }
            if (leftType == "double" || rightType == "double")
            {
                return "double";
            }
            return "number";
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
            return it != functionReturnTypes.end() ? it->second : "number";
        }

        return "number";
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
                bool isArrayType = decl->declaredType.find('[') != std::string::npos;
                if (isArrayType)
                {
                    type = decl->declaredType;
                }
                else if (!decl->declaredType.empty())
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

    std::map<std::string, std::string> buildScope(const std::vector<Param> &params,
                                                  const std::vector<Node *> &body,
                                                  const std::map<std::string, std::string> &functionReturnTypes)
    {
        std::map<std::string, std::string> scope;
        for (const Param &p : params)
        {
            scope[p.name] = p.type.empty() ? "number" : normalizeTypeName(p.type);
        }
        collectDeclaredLocals(body, scope, functionReturnTypes);
        return scope;
    }

    // Combines two return-site types the same way binary-expression
    // codegen already promotes mixed arithmetic: number+double ->
    // double (the more general of the two). A genuinely incompatible
    // pair (e.g. number and string) keeps whichever was found first --
    // Quill's typechecker doesn't cross-check return-path consistency,
    // so this stays lenient rather than introducing a new hard error
    // class here.
    std::string combineReturnTypes(const std::string &a, const std::string &b)
    {
        if (a == "void")
            return b;
        if (b == "void")
            return a;
        if (a == b)
            return a;
        if ((a == "number" && b == "double") || (a == "double" && b == "number"))
            return "double";
        return a;
    }

    // Walks *every* return statement reachable in this function body --
    // not just the first one found -- since an early bail-out of a
    // different numeric type (e.g. `return 0;` inside a safety-valve
    // check partway through a function that mainly `return`s a
    // double) must not be allowed to decide the whole function's
    // return type just because it happens to be visited first.
    std::string inferFunctionReturnType(const std::vector<Node *> &body,
                                        const std::map<std::string, std::string> &scope,
                                        const std::map<std::string, std::string> &functionReturnTypes)
    {
        std::string found = "void";
        for (Node *node : body)
        {
            if (!node)
                continue;

            std::string candidate = "void";

            if (auto *ret = dynamic_cast<ReturnStatement *>(node))
            {
                if (ret->value)
                {
                    candidate = inferNodeType(ret->value, scope, functionReturnTypes);
                }
            }
            else if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
            {
                std::string fromConsequent = inferFunctionReturnType(ifStmt->consequent, scope, functionReturnTypes);
                std::string fromAlternate = inferFunctionReturnType(ifStmt->alternate, scope, functionReturnTypes);
                candidate = combineReturnTypes(fromConsequent, fromAlternate);
            }
            else if (auto *loop = dynamic_cast<WhileLoop *>(node))
            {
                candidate = inferFunctionReturnType(loop->body, scope, functionReturnTypes);
            }

            found = combineReturnTypes(found, candidate);
        }
        return found;
    }

    std::map<std::string, std::string> buildFunctionReturnTypes(const Program &program)
    {
        std::map<std::string, std::string> types;
        types["len"] = "number";
        types["toString"] = "string";
        types["push"] = "void";
        types["pop"] = "number"; // element type approximated; real type depends on array

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

    std::string astToC(Node *node,
                       const std::map<std::string, std::string> &scope,
                       const std::map<std::string, std::string> &functionReturnTypes);

    std::string toCStringExpr(Node *node,
                              const std::map<std::string, std::string> &scope,
                              const std::map<std::string, std::string> &functionReturnTypes)
    {
        std::string type = inferNodeType(node, scope, functionReturnTypes);
        std::string code = astToC(node, scope, functionReturnTypes);
        if (type == "string")
            return code;
        if (type == "double")
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

        if (auto *decl = dynamic_cast<VariableDeclaration *>(node))
        {
            auto it = scope.find(decl->identifier);
            std::string typeName = it != scope.end() ? it->second : "number";

            // Dynamic array: number[] / double[] / ...
            if (isDynamicArrayType(typeName))
            {
                std::string elem = arrayElemType(typeName);
                std::string ctype = dynArrCType(elem);
                std::string pref = dynArrPrefix(elem);
                if (auto *lit = dynamic_cast<ArrayLiteral *>(decl->value))
                {
                    std::ostringstream init;
                    init << ctype << " " << decl->identifier << " = " << pref << "_new();\n";
                    for (Node *el : lit->elements)
                    {
                        init << "    " << pref << "_push(&" << decl->identifier << ", "
                             << astToC(el, scope, functionReturnTypes) << ");\n";
                    }
                    std::string s = init.str();
                    // last extra newline handled by caller
                    if (!s.empty() && s.back() == '\n')
                        s.pop_back();
                    return s;
                }
                if (decl->value)
                {
                    return ctype + " " + decl->identifier + " = " +
                           astToC(decl->value, scope, functionReturnTypes) + ";";
                }
                return ctype + " " + decl->identifier + " = " + pref + "_new();";
            }

            // Fixed-size array: number[5]
            auto bracketPos = typeName.find('[');
            if (bracketPos != std::string::npos)
            {
                std::string baseType = typeName.substr(0, bracketPos);
                std::string sizePart = typeName.substr(bracketPos);
                if (auto *lit = dynamic_cast<ArrayLiteral *>(decl->value))
                {
                    std::ostringstream init;
                    init << "{";
                    for (size_t i = 0; i < lit->elements.size(); ++i)
                    {
                        if (i)
                            init << ", ";
                        init << astToC(lit->elements[i], scope, functionReturnTypes);
                    }
                    init << "}";
                    return convertTypeName(baseType) + " " + decl->identifier + sizePart +
                           " = " + init.str() + ";";
                }
                return convertTypeName(baseType) + " " + decl->identifier + sizePart + ";";
            }

            std::string type = convertTypeName(typeName);
            std::string val = decl->value ? astToC(decl->value, scope, functionReturnTypes) : "0";
            return type + " " + decl->identifier + " = " + val + ";";
        }

        if (auto *assign = dynamic_cast<Assignment *>(node))
        {
            return assign->identifier + " = " + astToC(assign->value, scope, functionReturnTypes) + ";";
        }

        if (auto *idxAssign = dynamic_cast<IndexAssignment *>(node))
        {
            std::string objCode = astToC(idxAssign->object, scope, functionReturnTypes);
            std::string idxCode = astToC(idxAssign->index, scope, functionReturnTypes);
            std::string valCode = astToC(idxAssign->value, scope, functionReturnTypes);
            std::string objType = inferNodeType(idxAssign->object, scope, functionReturnTypes);
            if (isDynamicArrayType(objType))
            {
                std::string pref = dynArrPrefix(arrayElemType(objType));
                return pref + "_set(&" + objCode + ", " + idxCode + ", " + valCode + ");";
            }
            return objCode + "[" + idxCode + "] = " + valCode + ";";
        }

        if (auto *bin = dynamic_cast<BinaryExpression *>(node))
        {
            if (bin->op == "+")
            {
                std::string leftType = inferNodeType(bin->left, scope, functionReturnTypes);
                std::string rightType = inferNodeType(bin->right, scope, functionReturnTypes);
                if (leftType == "string" || rightType == "string")
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

        if (auto *litInt = dynamic_cast<LiteralInt *>(node))
            return std::to_string(litInt->value);
        if (auto *litDoudle = dynamic_cast<LiteralDoudle *>(node))
            return std::to_string(litDoudle->value);
        if (auto *litBool = dynamic_cast<LiteralBool *>(node))
            return litBool->value ? "true" : "false";
        if (auto *litStr = dynamic_cast<LiteralString *>(node))
            return "\"" + litStr->value + "\"";
        if (auto *ident = dynamic_cast<Identifier *>(node))
            return ident->name;

        if (auto *idx = dynamic_cast<IndexExpression *>(node))
        {
            std::string objCode = astToC(idx->object, scope, functionReturnTypes);
            std::string idxCode = astToC(idx->index, scope, functionReturnTypes);
            std::string objType = inferNodeType(idx->object, scope, functionReturnTypes);
            if (isDynamicArrayType(objType))
            {
                std::string pref = dynArrPrefix(arrayElemType(objType));
                return pref + "_get(&" + objCode + ", " + idxCode + ")";
            }
            return objCode + "[" + idxCode + "]";
        }

        if (auto *lit = dynamic_cast<ArrayLiteral *>(node))
        {
            // Bare array literal as expression -> temporary dynamic array
            std::string elem = "number";
            if (!lit->elements.empty())
                elem = inferNodeType(lit->elements[0], scope, functionReturnTypes);
            std::string pref = dynArrPrefix(elem);
            // Not ideal as expression; prefer used only as initializer.
            // Emit a compound that creates a temp — callers should use decl form.
            return pref + "_new() /* array literal used as expression; prefer initializer */";
        }

        if (auto *print = dynamic_cast<PrintStatement *>(node))
        {
            std::string exprCode = astToC(print->expression, scope, functionReturnTypes);
            std::string type = inferNodeType(print->expression, scope, functionReturnTypes);
            if (type == "string")
                return "printf(\"%s\\n\", " + exprCode + ");";
            if (type == "double")
                return "printf(\"%f\\n\", " + exprCode + ");";
            if (type == "bool")
                return "printf(\"%s\\n\", (" + exprCode + ") ? \"true\" : \"false\");";
            return "printf(\"%d\\n\", " + exprCode + ");";
        }

        if (auto *inc = dynamic_cast<Increment *>(node))
        {
            return inc->identifier + "++;";
        }
        if (auto *dec = dynamic_cast<Decrement *>(node))
        {
            return dec->identifier + "--;";
        }

        if (auto *ifStmt = dynamic_cast<IfStatement *>(node))
        {
            std::ostringstream block;
            block << "if (" << astToC(ifStmt->condition, scope, functionReturnTypes) << ") {\n";
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

        if (auto *loop = dynamic_cast<WhileLoop *>(node))
        {
            std::ostringstream block;
            block << "while (" << astToC(loop->condition, scope, functionReturnTypes) << ") {\n";
            for (Node *stmt : loop->body)
            {
                block << "        " << astToC(stmt, scope, functionReturnTypes) << "\n";
            }
            block << "    }";
            return block.str();
        }

        if (auto *exprStmt = dynamic_cast<ExpressionStatement *>(node))
        {
            std::string code = astToC(exprStmt->expression, scope, functionReturnTypes);
            if (!code.empty() && code.back() != ';')
            {
                code += ";";
            }
            return code;
        }

        if (auto *ret = dynamic_cast<ReturnStatement *>(node))
        {
            if (ret->value)
            {
                return "return " + astToC(ret->value, scope, functionReturnTypes) + ";";
            }
            return "return;";
        }

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
                std::string t = argTypes[0];
                if (isDynamicArrayType(t))
                {
                    std::string pref = dynArrPrefix(arrayElemType(t));
                    return pref + "_len(&" + argCodes[0] + ")";
                }
                if (isArrayTypeName(t) && !isDynamicArrayType(t))
                {
                    // fixed-size: sizeof(a)/sizeof(a[0])
                    return "(int)(sizeof(" + argCodes[0] + ") / sizeof((" + argCodes[0] + ")[0]))";
                }
                return "(int)strlen(" + argCodes[0] + ")";
            }
            else if (name == "push" && argCodes.size() == 2)
            {
                std::string t = argTypes[0];
                if (!isDynamicArrayType(t))
                    return "/* push requires dynamic array */ 0";
                std::string pref = dynArrPrefix(arrayElemType(t));
                return pref + "_push(&" + argCodes[0] + ", " + argCodes[1] + ")";
            }
            else if (name == "pop" && argCodes.size() == 1)
            {
                std::string t = argTypes[0];
                if (!isDynamicArrayType(t))
                    return "/* pop requires dynamic array */ 0";
                std::string pref = dynArrPrefix(arrayElemType(t));
                return pref + "_pop(&" + argCodes[0] + ")";
            }
            else if ((name == "C_call" || name == "C_top") && argCodes.size() == 1)
            {

                std::string &targetStr = argCodes[0];

                if (targetStr.size() >= 2 && targetStr.front() == '"' && targetStr.back() == '"')
                {
                    targetStr = targetStr.substr(1, targetStr.size() - 2);
                }

                std::string arg = {targetStr};
                return "" + arg + "";
            }
            else if (name == "input" && argCodes.size() == 1)
            {
                if (argTypes[0] == "string")
                    return "// string type not supported on input.";
                if (argTypes[0] == "number")
                    return "scanf(\"%d\", &" + argCodes[0] + ")";

                if (argTypes[0] == "double")
                    return "scanf(\"%lf\", &" + argCodes[0] + ")";

                return "scanf(\"%d\", &" + argCodes[0] + ")";
            }
            else if (name == "toString" && argCodes.size() == 1)
            {
                if (argTypes[0] == "string")
                    return argCodes[0];
                if (argTypes[0] == "double")
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

    // ------------------------------------------------------------------
    // import "path.qsc";
    //
    // Splices the named file's entire contents -- functions and
    // top-level code alike -- in place of the import statement, the
    // same way C's #include works. Nested imports inside an imported
    // file resolve relative to *that* file's own directory, not the
    // original file's.
    // ------------------------------------------------------------------

    std::string resolveImportPath(const std::string &baseDir, const std::string &importPath)
    {
        if (!importPath.empty() && importPath[0] == '/')
        {
            return importPath; // already absolute
        }
        if (baseDir.empty())
        {
            return importPath;
        }
        return baseDir + "/" + importPath;
    }

    // `inProgress` tracks paths currently being expanded on this call
    // stack, so A importing B importing A is reported as a clear cycle
    // error instead of recursing forever.
    Program expandImports(const Program &program, const std::string &baseDir, std::set<std::string> &inProgress)
    {
        Program expanded;
        for (Node *node : program.body)
        {
            if (!node)
                continue;

            if (auto *imp = dynamic_cast<ImportStatement *>(node))
            {
                std::string resolvedPath = resolveImportPath(baseDir, imp->path);

                if (inProgress.find(resolvedPath) != inProgress.end())
                {
                    throw std::runtime_error("import cycle detected involving: " + resolvedPath);
                }

                std::ifstream importFile(resolvedPath);
                if (!importFile.is_open())
                {
                    throw std::runtime_error("could not open imported file: " + resolvedPath +
                                             " (imported as \"" + imp->path + "\")");
                }
                std::string importedSource((std::istreambuf_iterator<char>(importFile)),
                                           std::istreambuf_iterator<char>());

                Lexer importLexer(importedSource);
                std::vector<Token> importTokens = importLexer.tokenize();
                Parser importParser(importTokens);
                Program importedProgram = importParser.parse();

                std::size_t slashPos = resolvedPath.find_last_of('/');
                std::string importedBaseDir = slashPos == std::string::npos ? "" : resolvedPath.substr(0, slashPos);

                inProgress.insert(resolvedPath);
                Program expandedImport = expandImports(importedProgram, importedBaseDir, inProgress);
                inProgress.erase(resolvedPath);

                for (Node *importedNode : expandedImport.body)
                {
                    expanded.body.push_back(importedNode);
                }
            }
            else
            {
                expanded.body.push_back(node);
            }
        }
        return expanded;
    }

    // Every Quill function becomes its own C function -- two Function
    // nodes with the same name (whether both written locally, both
    // pulled in via import, or one of each) would silently emit two C
    // functions with identical names, which fails to compile. Catching
    // it here gives a clear Quill-level error instead of a confusing C
    // compiler error pointing at generated code the student never wrote.
    void checkForDuplicateFunctions(const Program &program)
    {
        std::map<std::string, bool> seen;
        for (Node *node : program.body)
        {
            if (auto *fn = dynamic_cast<Function *>(node))
            {
                if (seen.find(fn->name) != seen.end())
                {
                    throw std::runtime_error("duplicate function definition: " + fn->name);
                }
                seen[fn->name] = true;
            }
        }
    }

    class Transpiler
    {
    public:
        std::string transpile(const std::string &source, const std::string &baseDir, const bool typeCheck)
        {
            Lexer lexer(source);
            std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            Program program = parser.parse();

            std::set<std::string> inProgress;
            program = expandImports(program, baseDir, inProgress);
            checkForDuplicateFunctions(program);

            // Built before type-checking runs (not after, as before) so
            // the checker can be handed the transpiler's own, already-
            // correct notion of each function's return type -- both
            // explicitly annotated and inferred -- instead of
            // maintaining a second, weaker copy of that logic.
            std::map<std::string, std::string> functionReturnTypes = buildFunctionReturnTypes(program);

            // Type-checking runs on the *expanded* program -- otherwise
            // anything defined only in an imported file would never be
            // checked at all, since the checker would just see an
            // ImportStatement it doesn't know how to interpret.
            if (typeCheck)
            {
                try
                {
                    TypeChecker checker;
                    checker.check(program, functionReturnTypes);
                }
                catch (const std::exception &ex)
                {
                    throw std::runtime_error("Type error: " + std::string(ex.what()));
                }
            }

            std::map<std::string, std::string> mainScope = buildScope({}, program.body, functionReturnTypes);

            std::vector<std::string> functionBlocks;
            std::vector<std::string> mainLines;

            // A C_call() written directly at the top level (not nested
            // inside any function, if, or while) is raw C meant to sit
            // at file scope -- #include lines, struct/typedef
            // definitions, extern globals -- not inside main(). A
            // C_call() written *inside* a function or loop body still
            // ends up exactly where it already did, since this only
            // special-cases the flat top-level statement list.
            std::vector<std::string> preambleLines;

            for (Node *node : program.body)
            {
                if (!node)
                    continue;

                if (auto *fnNode = dynamic_cast<Function *>(node))
                {
                    // extern functions are signature-only -- the real
                    // implementation is whatever library the student
                    // linked (already declared via a C_top() #include).
                    // Nothing to emit here; buildFunctionReturnTypes()
                    // already picked up its return type from
                    // fnNode->returnType regardless of isExtern, so
                    // calls to it are still typed correctly everywhere
                    // else in the file.
                    if (fnNode->isExtern)
                    {
                        continue;
                    }

                    std::map<std::string, std::string> scope =
                        buildScope(fnNode->params, fnNode->body, functionReturnTypes);

                    std::string returnType = convertTypeName(functionReturnTypes.at(fnNode->name));

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
                            if (stmtCode.back() != ';' && stmtCode.back() != '}')
                            {
                                stmtCode += ";";
                            }
                            funcStream << "    " << stmtCode << "\n";
                        }
                    }
                    funcStream << "}\n";
                    functionBlocks.push_back(funcStream.str());
                }
                else if (auto *topCall = dynamic_cast<CallExpression *>(node);
                         topCall && topCall->callee == "C_top")
                {
                    // C_top(), unlike C_call(), always lands at file
                    // scope above main() -- for #include directives,
                    // struct/typedef definitions, extern globals, or
                    // whole helper function definitions. C_call() keeps
                    // its original meaning unchanged: it runs wherever
                    // it's textually written, top-level or not.
                    std::string code = astToC(node, mainScope, functionReturnTypes);
                    if (!code.empty())
                    {
                        preambleLines.push_back(code);
                    }
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

            out << R"QUILL_ARR(
            typedef struct { int *data; int length; int capacity; } quill_arr_int;
            typedef struct { double *data; int length; int capacity; } quill_arr_double;
            typedef struct { bool *data; int length; int capacity; } quill_arr_bool;
            typedef struct { const char **data; int length; int capacity; } quill_arr_str;

            static quill_arr_int quill_arr_int_new(void) {
                quill_arr_int a; a.data = NULL; a.length = 0; a.capacity = 0; return a;
            }
            static quill_arr_double quill_arr_double_new(void) {
                quill_arr_double a; a.data = NULL; a.length = 0; a.capacity = 0; return a;
            }
            static quill_arr_bool quill_arr_bool_new(void) {
                quill_arr_bool a; a.data = NULL; a.length = 0; a.capacity = 0; return a;
            }
            static quill_arr_str quill_arr_str_new(void) {
                quill_arr_str a; a.data = NULL; a.length = 0; a.capacity = 0; return a;
            }

            static void quill_arr_int_push(quill_arr_int *a, int v) {
                if (a->length >= a->capacity) {
                    int nc = a->capacity == 0 ? 8 : a->capacity * 2;
                    int *nd = (int*)realloc(a->data, (size_t)nc * sizeof(int));
                    if (!nd) return;
                    a->data = nd; a->capacity = nc;
                }
                a->data[a->length++] = v;
            }
            static void quill_arr_double_push(quill_arr_double *a, double v) {
                if (a->length >= a->capacity) {
                    int nc = a->capacity == 0 ? 8 : a->capacity * 2;
                    double *nd = (double*)realloc(a->data, (size_t)nc * sizeof(double));
                    if (!nd) return;
                    a->data = nd; a->capacity = nc;
                }
                a->data[a->length++] = v;
            }
            static void quill_arr_bool_push(quill_arr_bool *a, bool v) {
                if (a->length >= a->capacity) {
                    int nc = a->capacity == 0 ? 8 : a->capacity * 2;
                    bool *nd = (bool*)realloc(a->data, (size_t)nc * sizeof(bool));
                    if (!nd) return;
                    a->data = nd; a->capacity = nc;
                }
                a->data[a->length++] = v;
            }
            static void quill_arr_str_push(quill_arr_str *a, const char *v) {
                if (a->length >= a->capacity) {
                    int nc = a->capacity == 0 ? 8 : a->capacity * 2;
                    const char **nd = (const char**)realloc(a->data, (size_t)nc * sizeof(const char*));
                    if (!nd) return;
                    a->data = nd; a->capacity = nc;
                }
                a->data[a->length++] = v;
            }

            static int quill_arr_int_get(quill_arr_int *a, int i) {
                if (i < 0 || i >= a->length) return 0;
                return a->data[i];
            }
            static double quill_arr_double_get(quill_arr_double *a, int i) {
                if (i < 0 || i >= a->length) return 0.0;
                return a->data[i];
            }
            static bool quill_arr_bool_get(quill_arr_bool *a, int i) {
                if (i < 0 || i >= a->length) return false;
                return a->data[i];
            }
            static const char *quill_arr_str_get(quill_arr_str *a, int i) {
                if (i < 0 || i >= a->length) return "";
                return a->data[i] ? a->data[i] : "";
            }

            static void quill_arr_int_set(quill_arr_int *a, int i, int v) {
                if (i < 0 || i >= a->length) return;
                a->data[i] = v;
            }
            static void quill_arr_double_set(quill_arr_double *a, int i, double v) {
                if (i < 0 || i >= a->length) return;
                a->data[i] = v;
            }
            static void quill_arr_bool_set(quill_arr_bool *a, int i, bool v) {
                if (i < 0 || i >= a->length) return;
                a->data[i] = v;
            }
            static void quill_arr_str_set(quill_arr_str *a, int i, const char *v) {
                if (i < 0 || i >= a->length) return;
                a->data[i] = v;
            }

            static int quill_arr_int_len(quill_arr_int *a) { return a->length; }
            static int quill_arr_double_len(quill_arr_double *a) { return a->length; }
            static int quill_arr_bool_len(quill_arr_bool *a) { return a->length; }
            static int quill_arr_str_len(quill_arr_str *a) { return a->length; }

            static int quill_arr_int_pop(quill_arr_int *a) {
                if (a->length <= 0) return 0;
                return a->data[--a->length];
            }
            static double quill_arr_double_pop(quill_arr_double *a) {
                if (a->length <= 0) return 0.0;
                return a->data[--a->length];
            }
            static bool quill_arr_bool_pop(quill_arr_bool *a) {
                if (a->length <= 0) return false;
                return a->data[--a->length];
            }
            static const char *quill_arr_str_pop(quill_arr_str *a) {
                if (a->length <= 0) return "";
                return a->data[--a->length];
            }
            )QUILL_ARR";

            for (const std::string &line : preambleLines)
                out << line << "\n";
            if (!preambleLines.empty())
                out << "\n";

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
    bool typeCheck = false;
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
        else if (arg == "-tc" || arg == "--typeCheck")
        {
            typeCheck = true;
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
        std::cout << "Quill version: 2.4.2\n";
        return 0;
    }

    if (args.size() != 1)
    {
        std::cerr << "Usage: quill [--compile] [-o output] <input.qsc>\n";
        return 1;
    }

    if (!transpileMode)
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

    // Relative import paths inside this file resolve against its own
    // directory, so `import "helpers.qsc";` works regardless of which
    // directory quillc itself was invoked from.
    std::size_t slashPos = inputPath.find_last_of('/');
    std::string baseDir = slashPos == std::string::npos ? "" : inputPath.substr(0, slashPos);

    if (debug)
    {
        Lexer debugLexer(source);
        std::vector<Token> debugTokens = debugLexer.tokenize();
        std::cout << "=== LEXING ===\n";
        for (std::size_t i = 0; i < debugTokens.size() && i < 30; ++i)
        {
            std::cout << "  [" << i << "] type=" << static_cast<int>(debugTokens[i].type) << " value='" << debugTokens[i].value << "'\n";
        }
    }

    // Lexing, parsing, import expansion, duplicate-function checking,
    // and type checking all happen inside transpile() now, against the
    // fully expanded program -- see the comment on expandImports() for
    // why that matters. Any failure at any of those stages throws, and
    // is reported here with whatever message the failing stage attached
    // (already prefixed appropriately, e.g. "Type error: ...").
    Transpiler transpiler;
    std::string cOutput;
    try
    {
        cOutput = transpiler.transpile(source, baseDir, typeCheck);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << "\n";
        return 1;
    }

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