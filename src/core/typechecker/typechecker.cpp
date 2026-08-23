#pragma once

#include <stdexcept>
#include <string>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../ast/ast.cpp"

class TypeChecker {
public:
    // functionReturnTypes is the transpiler's own already-resolved map
    // of function name -> return type (explicit annotation, or
    // inferred from the function's own return statements when one
    // wasn't given) -- see buildFunctionReturnTypes() in trans.cpp.
    // Reusing it here means this checker and the code actually emitted
    // by the transpiler can never disagree about what a function
    // returns.
    void check(const Program& program, const std::map<std::string, std::string>& functionReturnTypes) {
        this->functionReturnTypes = functionReturnTypes;
        for (Node* stmt : program.body) {
            checkNode(stmt);
        }
    }

private:
    std::unordered_map<std::string, std::string> symbols;
    std::map<std::string, std::string> functionReturnTypes;

    void checkNode(Node* node) {
        if (!node) return;

        if (auto* decl = dynamic_cast<VariableDeclaration*>(node)) {
            checkVariableDeclaration(decl);
            return;
        }

        if (auto* assign = dynamic_cast<Assignment*>(node)) {
            checkAssignment(assign);
            return;
        }

        if (auto* expr = dynamic_cast<ExpressionStatement*>(node)) {
            checkNode(expr->expression);
            return;
        }

        if (auto* fn = dynamic_cast<Function*>(node)) {
            auto saved = symbols;
            for (const Param& param : fn->params) {
                symbols[param.name] = param.type;
            }

            for (Node* stmt : fn->body) {
                checkNode(stmt);
            }

            symbols = saved;
            return;
        }

        if (auto* ifStmt = dynamic_cast<IfStatement*>(node)) {
            checkNode(ifStmt->condition);
            for (Node* stmt : ifStmt->consequent) checkNode(stmt);
            for (Node* stmt : ifStmt->alternate) checkNode(stmt);
            return;
        }

        if (auto* whileLoop = dynamic_cast<WhileLoop*>(node)) {
            checkNode(whileLoop->condition);
            for (Node* stmt : whileLoop->body) checkNode(stmt);
            return;
        }

        if (auto* ret = dynamic_cast<ReturnStatement*>(node)) {
            checkNode(ret->value);
            return;
        }

        if (auto* print = dynamic_cast<PrintStatement*>(node)) {
            checkNode(print->expression);
            return;
        }

        if (auto* call = dynamic_cast<CallExpression*>(node)) {
            for (Node* arg : call->arguments) {
                checkNode(arg);
            }
            return;
        }

        if (auto* bin = dynamic_cast<BinaryExpression*>(node)) {
            checkNode(bin->left);
            checkNode(bin->right);
            return;
        }

        if (auto* unary = dynamic_cast<UnaryExpression*>(node)) {
            checkNode(unary->operand);
            return;
        }

        if (auto* idx = dynamic_cast<IndexExpression*>(node)) {
            checkNode(idx->object);
            checkNode(idx->index);
            std::string objType = inferType(idx->object);
            bool isArray = objType.find('[') != std::string::npos;
            if (objType != "string" && !isArray) {
                throw std::runtime_error("indexing with [] is only supported on string or array values");
            }
            return;
        }

        if (auto* idxAssign = dynamic_cast<IndexAssignment*>(node)) {
            checkNode(idxAssign->object);
            checkNode(idxAssign->index);
            checkNode(idxAssign->value);

            std::string objType = inferType(idxAssign->object);
            auto bracketPos = objType.find('[');
            if (bracketPos == std::string::npos) {
                throw std::runtime_error("assignment with [] is only supported on array values");
            }

            std::string elemType = objType.substr(0, bracketPos);
            std::string valueType = inferType(idxAssign->value);
            if (valueType != elemType) {
                throw std::runtime_error("type mismatch assigning into array");
            }
            return;
        }

        if (auto* ident = dynamic_cast<Identifier*>(node)) {
            if (symbols.find(ident->name) == symbols.end()) {
                throw std::runtime_error("undefined identifier: " + ident->name);
            }
            return;
        }
    }

    void checkVariableDeclaration(VariableDeclaration* decl) {
        if (!decl->value) {
            // Arrays (and any other declaration without an initializer)
            // still need to be registered so later uses aren't flagged
            // as undefined identifiers.
            if (!decl->declaredType.empty()) {
                symbols[decl->identifier] = decl->declaredType;
            }
            return;
        }
        checkNode(decl->value);

        std::string valueType = inferType(decl->value);
        if (!decl->declaredType.empty() && decl->declaredType != valueType) {
            throw std::runtime_error("type mismatch for variable: " + decl->identifier);
        }
        symbols[decl->identifier] = (decl->declaredType.empty() ? valueType : decl->declaredType);
    }

    void checkAssignment(Assignment* assign) {
        auto it = symbols.find(assign->identifier);
        if (it == symbols.end()) {
            throw std::runtime_error("assignment to undefined variable: " + assign->identifier);
        }

        if (!assign->value) return;
        checkNode(assign->value);

        std::string valueType = inferType(assign->value);
        if (valueType != it->second) {
            throw std::runtime_error("type mismatch on assignment to: " + assign->identifier);
        }
    }

    std::string inferType(Node* node) {
        if (!node) return "void";

        if (dynamic_cast<LiteralInt*>(node)) return "number";
        if (dynamic_cast<LiteralDoudle*>(node)) return "double";
        if (dynamic_cast<LiteralString*>(node)) return "string";
        if (dynamic_cast<LiteralBool*>(node)) return "bool";

        if (auto* ident = dynamic_cast<Identifier*>(node)) {
            auto it = symbols.find(ident->name);
            if (it != symbols.end()) return it->second;
            return "unknown";
        }

        // Indexing a str with [] yields a character, represented the
        // same way C represents char: as a small int.
        if (dynamic_cast<IndexExpression*>(node)) return "number";

        if (auto* binary = dynamic_cast<BinaryExpression*>(node)) {
            std::string left = inferType(binary->left);
            std::string right = inferType(binary->right);
            if (left == right) return left;
            if ((left == "number" && right == "double") || (left == "double" && right == "number")) return "double";
            return "unknown";
        }

        if (auto* unary = dynamic_cast<UnaryExpression*>(node)) {
            return inferType(unary->operand);
        }

        if (auto* call = dynamic_cast<CallExpression*>(node)) {
            auto it = functionReturnTypes.find(call->callee);
            return it != functionReturnTypes.end() ? it->second : "unknown";
        }
        if (dynamic_cast<Assignment*>(node)) return "void";

        return "unknown";
    }
};