#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/ast.cpp"
#include "../types/types.cpp"
#include "../lexer/lexer.cpp"
#include "../parser/parser.cpp"
#include "../typechecker/typechecker.cpp"

namespace {

enum class ValueType {
    Null,
    Int,
    Float,
    String,
    Bool
};

struct RuntimeValue {
    ValueType type = ValueType::Null;
    long long asInt = 0;
    double asFloat = 0.0;
    std::string asString;
    bool asBool = false;
};

RuntimeValue makeInt(long long value) {
    RuntimeValue out;
    out.type = ValueType::Int;
    out.asInt = value;
    return out;
}

RuntimeValue makeFloat(double value) {
    RuntimeValue out;
    out.type = ValueType::Float;
    out.asFloat = value;
    return out;
}

RuntimeValue makeString(const std::string& value) {
    RuntimeValue out;
    out.type = ValueType::String;
    out.asString = value;
    return out;
}

RuntimeValue makeBool(bool value) {
    RuntimeValue out;
    out.type = ValueType::Bool;
    out.asBool = value;
    return out;
}

std::string stringifyValue(const RuntimeValue& value) {
    switch (value.type) {
        case ValueType::Int: return std::to_string(value.asInt);
        case ValueType::Float: return std::to_string(value.asFloat);
        case ValueType::String: return value.asString;
        case ValueType::Bool: return value.asBool ? "1" : "0";
        default: return "";
    }
}

bool truthy(const RuntimeValue& value) {
    if (value.type == ValueType::Bool) return value.asBool;
    if (value.type == ValueType::Int) return value.asInt != 0;
    if (value.type == ValueType::Float) return value.asFloat != 0.0;
    if (value.type == ValueType::String) return !value.asString.empty();
    return false;
}

RuntimeValue addValues(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String || b.type == ValueType::String) {
        return makeString(stringifyValue(a) + stringifyValue(b));
    }
    if (a.type == ValueType::Float || b.type == ValueType::Float) {
        double lhs = (a.type == ValueType::Float) ? a.asFloat : static_cast<double>(a.asInt);
        double rhs = (b.type == ValueType::Float) ? b.asFloat : static_cast<double>(b.asInt);
        return makeFloat(lhs + rhs);
    }
    return makeInt(a.asInt + b.asInt);
}

RuntimeValue subValues(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::Float || b.type == ValueType::Float) {
        double lhs = (a.type == ValueType::Float) ? a.asFloat : static_cast<double>(a.asInt);
        double rhs = (b.type == ValueType::Float) ? b.asFloat : static_cast<double>(b.asInt);
        return makeFloat(lhs - rhs);
    }
    return makeInt(a.asInt - b.asInt);
}

RuntimeValue mulValues(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::Float || b.type == ValueType::Float) {
        double lhs = (a.type == ValueType::Float) ? a.asFloat : static_cast<double>(a.asInt);
        double rhs = (b.type == ValueType::Float) ? b.asFloat : static_cast<double>(b.asInt);
        return makeFloat(lhs * rhs);
    }
    return makeInt(a.asInt * b.asInt);
}

RuntimeValue divValues(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::Float || b.type == ValueType::Float) {
        double lhs = (a.type == ValueType::Float) ? a.asFloat : static_cast<double>(a.asInt);
        double rhs = (b.type == ValueType::Float) ? b.asFloat : static_cast<double>(b.asInt);
        if (rhs == 0.0) {
            throw std::runtime_error("division by zero");
        }
        return makeFloat(lhs / rhs);
    }
    if (b.asInt == 0) {
        throw std::runtime_error("division by zero");
    }
    return makeInt(a.asInt / b.asInt);
}

double toNumber(const RuntimeValue& value) {
    if (value.type == ValueType::Bool) return value.asBool ? 1.0 : 0.0;
    if (value.type == ValueType::Float) return value.asFloat;
    if (value.type == ValueType::Int) return static_cast<double>(value.asInt);
    return 0.0;
}

RuntimeValue compareEq(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String && b.type == ValueType::String) {
        return makeBool(a.asString == b.asString);
    }
    return makeBool(toNumber(a) == toNumber(b));
}

RuntimeValue compareLt(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String && b.type == ValueType::String) {
        return makeBool(a.asString < b.asString);
    }
    return makeBool(toNumber(a) < toNumber(b));
}

RuntimeValue compareGt(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String && b.type == ValueType::String) {
        return makeBool(a.asString > b.asString);
    }
    return makeBool(toNumber(a) > toNumber(b));
}

RuntimeValue compareLe(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String && b.type == ValueType::String) {
        return makeBool(a.asString <= b.asString);
    }
    return makeBool(toNumber(a) <= toNumber(b));
}

RuntimeValue compareGe(const RuntimeValue& a, const RuntimeValue& b) {
    if (a.type == ValueType::String && b.type == ValueType::String) {
        return makeBool(a.asString >= b.asString);
    }
    return makeBool(toNumber(a) >= toNumber(b));
}

class Interpreter {
public:
    RuntimeValue run(const Program& program) {
        RuntimeValue last;
        for (Node* stmt : program.body) {
            RuntimeValue value = evaluateStatement(stmt);
            if (value.type != ValueType::Null) {
                last = value;
            }
        }
        return last;
    }

private:
    std::unordered_map<std::string, RuntimeValue> env_;
    std::unordered_map<std::string, const Function*> functions_;

    RuntimeValue evaluateStatement(Node* node) {
        if (!node) return RuntimeValue{};

        if (auto* decl = dynamic_cast<VariableDeclaration*>(node)) {
            RuntimeValue value = decl->value ? evaluateExpression(decl->value) : RuntimeValue{};
            env_[decl->identifier] = value;
            return RuntimeValue{};
        }

        if (auto* assign = dynamic_cast<Assignment*>(node)) {
            auto it = env_.find(assign->identifier);
            if (it == env_.end()) {
                throw std::runtime_error("undefined identifier: " + assign->identifier);
            }
            it->second = evaluateExpression(assign->value);
            return RuntimeValue{};
        }

        if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(node)) {
            return evaluateExpression(exprStmt->expression);
        }

        if (auto* print = dynamic_cast<PrintStatement*>(node)) {
            RuntimeValue value = evaluateExpression(print->expression);
            std::cout << stringifyValue(value) << std::endl;
            return RuntimeValue{};
        }

        if (auto* ret = dynamic_cast<ReturnStatement*>(node)) {
            return ret->value ? evaluateExpression(ret->value) : RuntimeValue{};
        }

        if (auto* ifStmt = dynamic_cast<IfStatement*>(node)) {
            RuntimeValue cond = evaluateExpression(ifStmt->condition);
            if (truthy(cond)) {
                for (Node* stmt : ifStmt->consequent) {
                    evaluateStatement(stmt);
                }
            } else {
                for (Node* stmt : ifStmt->alternate) {
                    evaluateStatement(stmt);
                }
            }
            return RuntimeValue{};
        }

        if (auto* loop = dynamic_cast<WhileLoop*>(node)) {
            while (truthy(evaluateExpression(loop->condition))) {
                for (Node* stmt : loop->body) {
                    evaluateStatement(stmt);
                }
            }
            return RuntimeValue{};
        }

        if (auto* fn = dynamic_cast<Function*>(node)) {
            functions_[fn->name] = fn;
            return RuntimeValue{};
        }

        if (auto* inc = dynamic_cast<Increment*>(node)) {
            auto it = env_.find(inc->identifier);
            if (it == env_.end()) {
                throw std::runtime_error("undefined identifier: " + inc->identifier);
            }
            if (it->second.type == ValueType::Float) {
                it->second.asFloat += 1.0;
            }
            else {
                it->second.asInt += 1;
            }
            return RuntimeValue{};
        }

        if (auto* dec = dynamic_cast<Decrement*>(node)) {
            auto it = env_.find(dec->identifier);
            if (it == env_.end()) {
                throw std::runtime_error("undefined identifier: " + dec->identifier);
            }
            if (it->second.type == ValueType::Float) {
                it->second.asFloat -= 1.0;
            }
            else {
                it->second.asInt -= 1;
            }
            return RuntimeValue{};
        }

        if (auto* ret = dynamic_cast<ReturnStatement*>(node)) {
            return ret->value ? evaluateExpression(ret->value) : RuntimeValue{};
        }

        return RuntimeValue{};
    }

    RuntimeValue evaluateExpression(Node* node) {
        if (!node) return RuntimeValue{};

        if (auto* litInt = dynamic_cast<LiteralInt*>(node)) {
            return makeInt(litInt->value);
        }
        if (auto* litFloat = dynamic_cast<LiteralFloat*>(node)) {
            return makeFloat(litFloat->value);
        }
        if (auto* litStr = dynamic_cast<LiteralString*>(node)) {
            return makeString(litStr->value);
        }
        if (auto* litBool = dynamic_cast<LiteralBool*>(node)) {
            return makeBool(litBool->value);
        }

        if (auto* ident = dynamic_cast<Identifier*>(node)) {
            auto it = env_.find(ident->name);
            if (it == env_.end()) {
                throw std::runtime_error("undefined identifier: " + ident->name);
            }
            return it->second;
        }

        if (auto* unary = dynamic_cast<UnaryExpression*>(node)) {
            RuntimeValue value = evaluateExpression(unary->operand);
            if (unary->op == "-") {
                if (value.type == ValueType::Float) return makeFloat(-value.asFloat);
                return makeInt(-value.asInt);
            }
            if (unary->op == "!") {
                return makeBool(!truthy(value));
            }
            return RuntimeValue{};
        }

        if (auto* bin = dynamic_cast<BinaryExpression*>(node)) {
            RuntimeValue left = evaluateExpression(bin->left);
            RuntimeValue right = evaluateExpression(bin->right);

            if (bin->op == "+") return addValues(left, right);
            if (bin->op == "-") return subValues(left, right);
            if (bin->op == "*") return mulValues(left, right);
            if (bin->op == "/") return divValues(left, right);
            if (bin->op == "==") return compareEq(left, right);
            if (bin->op == "!=") return makeBool(!(compareEq(left, right).asBool));
            if (bin->op == "<") return compareLt(left, right);
            if (bin->op == ">") return compareGt(left, right);
            if (bin->op == "<=") return compareLe(left, right);
            if (bin->op == ">=") return compareGe(left, right);
            if (bin->op == "&&") return makeBool(truthy(left) && truthy(right));
            if (bin->op == "||") return makeBool(truthy(left) || truthy(right));
            return RuntimeValue{};
        }

        if (auto* call = dynamic_cast<CallExpression*>(node)) {
            if (call->callee == "print" || call->callee == "printf") {
                for (Node* arg : call->arguments) {
                    std::cout << stringifyValue(evaluateExpression(arg)) << std::endl;
                }
                return RuntimeValue{};
            }

            auto fnIt = functions_.find(call->callee);
            if (fnIt != functions_.end()) {
                const Function* fn = fnIt->second;
                if (fn->params.size() != call->arguments.size()) {
                    throw std::runtime_error("wrong argument count for function: " + fn->name);
                }

                std::unordered_map<std::string, RuntimeValue> savedEnv = env_;
                for (std::size_t i = 0; i < fn->params.size(); ++i) {
                    env_[fn->params[i].name] = evaluateExpression(call->arguments[i]);
                }

                RuntimeValue result;
                for (Node* stmt : fn->body) {
                    RuntimeValue current = evaluateStatement(stmt);
                    if (current.type != ValueType::Null) {
                        result = current;
                    }
                    if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
                        if (ret->value) {
                            result = evaluateExpression(ret->value);
                        }
                        env_ = savedEnv;
                        return result;
                    }
                }

                env_ = savedEnv;
                return result;
            }

            throw std::runtime_error("unknown call: " + call->callee);
        }

        return RuntimeValue{};
    }
};

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("failed to open input file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

int main(int argc, char** argv) {
    bool debug = false;
    bool version = false;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            debug = true;
        } else if (arg == "--version" || arg == "-v") {
            version = true;
        } else {
            args.push_back(arg);
        }
    }

    if (version) {
        std::cout << "Quill version: 1.3.1\n";
        return 0;
    }

    if (args.size() != 1) {
        std::cerr << "Usage: quill-interp [--debug] [--version] <file.qsc>\n";
        return 1;
    }

    std::string inputPath = args[0];
    std::string source;
    try {
        source = readFile(inputPath);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        if (debug) {
            std::cout << "=== LEXING ===\n";
            for (std::size_t i = 0; i < tokens.size() && i < 25; ++i) {
                std::cout << "  [" << i << "] " << static_cast<int>(tokens[i].type) << " : '" << tokens[i].value << "'\n";
            }
        }

        Parser parser(tokens);
        Program program = parser.parse();
        if (debug) {
            std::cout << "=== PARSING ===\n";
            std::cout << "  statements: " << program.body.size() << "\n";
        }

        std::cout << "Made it past parser" << std::endl;

        TypeChecker checker;
        checker.check(program);

        Interpreter interpreter;
        interpreter.run(program);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Interpreter error: " << ex.what() << std::endl;
        return 1;
    }
}
