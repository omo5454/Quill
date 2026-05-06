import { Statement } from "../ast/ast";

export class Interpreter {
  // This stores our variables
  private variables = new Map<string, any>();

  constructor() {
    this.setupStandardLibrary();
  }

  private setupStandardLibrary() {

    this.variables.set("sqrt", {
      type: "built-in-function",
      fn: (args: any[]) => Math.sqrt(args[0])
    });

    this.variables.set("push", {
    type: "built-in-function",
    fn: (args: any[]) => {
      const [list, item] = args;
      if (Array.isArray(list)) {
        list.push(item);
        return list;
      }
      throw new Error("push() expects an array as the first argument");
    }
  });

    this.variables.set("random", {
      type: "built-in-function",
      fn: (args: any[]) => Math.random()
    });

    // String/Array length
    this.variables.set("len", {
      type: "built-in-function",
      fn: (args: any[]) => args[0].length
    });
    
    // Time
    this.variables.set("timeNow", {
      type: "built-in-function",
      fn: () => new Date().toLocaleDateString() + " " + new Date().toLocaleTimeString()
    });

  }




  public interpret(node: any): any {
    switch (node.type) {
      case "Program":
        node.body.forEach((stmt: any) => this.interpret(stmt));
        return this.variables; // Return the final state of variables
      
      case "CallExpression":
        const functionData = this.variables.get(node.callee);

          if (!functionData) {
            throw new Error(`Runtime Error: '${node.callee}' is not defined.`);
          }

          // Evaluate arguments first
          const args = node.arguments.map((arg: any) => this.interpret(arg));

          // Handle Standard Library (Built-ins)
          if (functionData.type === "built-in-function") {
            return functionData.fn(args);
          }

          // Handle User Functions
          if (functionData.type === "user-defined-function") {
            functionData.parameters.forEach((paramName: string, index: number) => {
              this.variables.set(paramName, args[index]);
            });

            let lastResult = null;
            functionData.body.forEach((stmt: any) => {
              lastResult = this.interpret(stmt);
            });
            return lastResult;
          }

          throw new Error(`Runtime Error: '${node.callee}' is not a function.`);

      case "Comment":
        // Comments are ignored in execution
        return;

      case "ConditionalExpression":
          const testResult = this.interpret(node.test);
          
          if (testResult) {
              // consequent is an array: [Statement, Statement, ...]
              return this.executeBlock(node.consequent);
          } else if (node.alternate) {
              // alternate is also an array
              return this.executeBlock(node.alternate);
          }
          return null;

      case "LoopExpression":
          while (this.interpret(node.test)) {
              this.executeBlock(node.body);
          }
          return null;
 


      case "Function":
        const funcData = {
          parameters: node.parameters,
          body: node.body,
          type: "user-defined-function"
        };
        this.variables.set(node.name, funcData);
        return null;

      case "String":
        return node.value;


      case "VariableDeclaration":
        const val = this.interpret(node.value);
        this.variables.set(node.identifier, val);
        return val;
    
      case "PrintStatement":
        const output = this.interpret(node.expression);
        console.log(output);
        return output;

      case "ArrayLiteral":
        return node.elements.map((el: any) => this.interpret(el));

      case "IndexExpression":
        const array = this.interpret(node.object);
        const idx = this.interpret(node.index);
        if (!Array.isArray(array)) {
          throw new Error("Runtime Error: Object is not an array");
        }
        return array[idx];
      
      case "ComparisonExpression":
        const leftVal = this.interpret(node.left);
        const rightVal = this.interpret(node.right);
        switch (node.operator) {
          case "==": return leftVal === rightVal;
          case "!=": return leftVal !== rightVal;
          case "<": return leftVal < rightVal;
          case ">": return leftVal > rightVal;
          case "<=": return leftVal <= rightVal;
          case ">=": return leftVal >= rightVal;
          default: throw new Error(`Unknown comparison operator: ${node.operator}`);
        }

      case "BinaryExpression":
        const left = this.interpret(node.left);
        const right = this.interpret(node.right);
        
        switch (node.operator) {
          case "+": return left + right;
          case "-": return left - right;
          case "*": return left * right;
          case "/": return left / right;
          case "%": return left % right;
          case "&&": return left && right;
          case "||": return left || right;
          case "!": return !left;
          case ">": return left > right;
          case "<": return left < right;
          case ">=": return left >= right;
          case "<=": return left <= right;
          case "==": return left === right;
          case "!=": return left !== right;
          default: throw new Error(`Unknown operator: ${node.operator}`);
        }

      case "Literal":
        return node.value;

      case "BooleanLiteral":
        return node.value;

      case "Identifier":
        if (this.variables.has(node.name)) {
          return this.variables.get(node.name);
        }
        throw new Error(`Undefined variable: ${node.name}`);

      default:
        throw new Error(`Unknown node type: ${node.type}`);
    }
}
private executeBlock(statements: Statement[]): any {
  let result = null;
  for (const statement of statements) {
    result = this.interpret(statement);
  }
  return result;
  }
}