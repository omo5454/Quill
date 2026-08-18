#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../ast/ast.cpp"

class TypeChecker {
public:
    void check(const Program& program) {
        for (Node* stmt : program.body) {
            checkNode(stmt);
        }
    }

private:
    std::unordered_map<std::string, std::string> symbols;

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
            if (objType != "str") {
                throw std::runtime_error("indexing with [] is only supported on str values");
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
        if (!decl->value) return;
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

        if (dynamic_cast<LiteralInt*>(node)) return "int";
        if (dynamic_cast<LiteralFloat*>(node)) return "float";
        if (dynamic_cast<LiteralString*>(node)) return "str";
        if (dynamic_cast<LiteralBool*>(node)) return "bool";

        if (auto* ident = dynamic_cast<Identifier*>(node)) {
            auto it = symbols.find(ident->name);
            if (it != symbols.end()) return it->second;
            return "unknown";
        }

        // Indexing a str with [] yields a character, represented the
        // same way C represents char: as a small int.
        if (dynamic_cast<IndexExpression*>(node)) return "int";

        if (auto* binary = dynamic_cast<BinaryExpression*>(node)) {
            std::string left = inferType(binary->left);
            std::string right = inferType(binary->right);
            if (left == right) return left;
            if ((left == "int" && right == "float") || (left == "float" && right == "int")) return "float";
            return "unknown";
        }

        if (auto* unary = dynamic_cast<UnaryExpression*>(node)) {
            return inferType(unary->operand);
        }

        if (dynamic_cast<CallExpression*>(node)) return "unknown";
        if (dynamic_cast<Assignment*>(node)) return "void";

        return "unknown";
    }
};