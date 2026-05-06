"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Interpreter = void 0;
class ReturnException extends Error {
    constructor(value) {
        super("ReturnException");
        this.value = value;
        this.name = "ReturnException";
    }
}
class Interpreter {
    constructor() {
        this.globals = new Map();
        this.scopes = [];
        this.setupStandardLibrary();
    }
    currentScope() {
        return this.scopes.length > 0 ? this.scopes[this.scopes.length - 1] : this.globals;
    }
    getVariable(name) {
        for (let i = this.scopes.length - 1; i >= 0; i--) {
            if (this.scopes[i].has(name)) {
                return this.scopes[i].get(name);
            }
        }
        return this.globals.get(name);
    }
    setVariable(name, value, isDeclaration = false) {
        if (!isDeclaration && this.scopes.length > 0) {
            this.scopes[this.scopes.length - 1].set(name, value);
        }
        else {
            this.currentScope().set(name, value);
        }
    }
    enterScope() {
        this.scopes.push(new Map());
    }
    exitScope() {
        if (this.scopes.length > 0)
            this.scopes.pop();
    }
    setupStandardLibrary() {
        this.globals.set("sqrt", {
            type: "built-in-function",
            fn: (args) => Math.sqrt(Number(args[0]) || 0),
        });
        this.globals.set("push", {
            type: "built-in-function",
            fn: (args) => {
                const [list, item] = args;
                if (Array.isArray(list)) {
                    list.push(item);
                    return list;
                }
                throw new Error("push() expects an array as the first argument");
            },
        });
        this.globals.set("random", {
            type: "built-in-function",
            fn: () => Math.random(),
        });
        this.globals.set("len", {
            type: "built-in-function",
            fn: (args) => { var _a, _b; return (_b = (_a = args[0]) === null || _a === void 0 ? void 0 : _a.length) !== null && _b !== void 0 ? _b : 0; },
        });
        this.globals.set("timeNow", {
            type: "built-in-function",
            fn: () => new Date().toLocaleDateString() + " " + new Date().toLocaleTimeString(),
        });
        // Added print as built-in for convenience
        this.globals.set("print", {
            type: "built-in-function",
            fn: (args) => {
                const output = args[0];
                if (Array.isArray(output)) {
                    console.log(output); // prints full array nicely
                }
                else {
                    console.log(output);
                }
                return output;
            },
        });
    }
    interpret(node) {
        switch (node.type) {
            case "Program":
                node.body.forEach((stmt) => this.interpret(stmt));
                return this.globals;
            case "VariableDeclaration":
                const val = this.interpret(node.value);
                this.setVariable(node.identifier, val, true);
                return val;
            case "AssignmentExpression":
                const assignValue = this.interpret(node.value);
                this.setVariable(node.identifier, assignValue);
                return assignValue;
            case "Identifier":
                const value = this.getVariable(node.name);
                if (value === undefined) {
                    throw new Error(`Runtime Error: Undefined variable '${node.name}'`);
                }
                return value;
            case "Literal":
            case "String":
            case "BooleanLiteral":
                return node.value;
            case "ArrayLiteral":
                return node.elements.map((el) => this.interpret(el));
            case "IndexExpression":
                const array = this.interpret(node.object);
                const idx = this.interpret(node.index);
                if (!Array.isArray(array))
                    throw new Error("Runtime Error: Object is not an array");
                if (idx < 0 || idx >= array.length)
                    throw new Error(`Index ${idx} out of bounds`);
                return array[idx];
            case "CallExpression":
                return this.callFunction(node);
            case "Function":
                const funcData = {
                    parameters: node.parameters,
                    body: node.body,
                    type: "user-defined-function",
                };
                this.globals.set(node.name, funcData);
                return null;
            case "PrintStatement":
                const output = this.interpret(node.expression);
                console.log(output);
                return output;
            case "ConditionalExpression":
                const testResult = this.interpret(node.test);
                if (testResult) {
                    return this.executeBlock(node.consequent);
                }
                else if (node.alternate) {
                    return this.executeBlock(node.alternate);
                }
                return null;
            case "LoopExpression":
                while (this.interpret(node.test)) {
                    this.executeBlock(node.body);
                }
                return null;
            case "BinaryExpression":
            case "ComparisonExpression":
                return this.evaluateBinary(node);
            case "Comment":
                return;
            default:
                throw new Error(`Unknown node type: ${node.type}`);
        }
    }
    callFunction(node) {
        const functionData = this.getVariable(node.callee);
        if (!functionData) {
            throw new Error(`Runtime Error: '${node.callee}' is not defined.`);
        }
        const args = node.arguments.map((arg) => this.interpret(arg));
        if (functionData.type === "built-in-function") {
            return functionData.fn(args);
        }
        if (functionData.type === "user-defined-function") {
            this.enterScope();
            functionData.parameters.forEach((paramName, index) => {
                this.setVariable(paramName, args[index], true);
            });
            let result = null;
            try {
                for (const stmt of functionData.body) {
                    result = this.interpret(stmt);
                }
            }
            catch (e) {
                if (e instanceof ReturnException) {
                    result = e.value;
                }
                else {
                    throw e;
                }
            }
            finally {
                this.exitScope();
            }
            return result;
        }
        throw new Error(`Runtime Error: '${node.callee}' is not a function.`);
    }
    evaluateBinary(node) {
        const left = this.interpret(node.left);
        const right = this.interpret(node.right);
        switch (node.operator) {
            case "+": return left + right;
            case "-": return left - right;
            case "*": return left * right;
            case "/": return left / right;
            case "%": return left % right;
            case "&&": return Boolean(left) && Boolean(right);
            case "||": return Boolean(left) || Boolean(right);
            case "==": return left === right;
            case "!=": return left !== right;
            case "<": return left < right;
            case ">": return left > right;
            case "<=": return left <= right;
            case ">=": return left >= right;
            default:
                throw new Error(`Unknown operator: ${node.operator}`);
        }
    }
    executeBlock(statements) {
        let result = null;
        for (const statement of statements) {
            result = this.interpret(statement);
        }
        return result;
    }
}
exports.Interpreter = Interpreter;
