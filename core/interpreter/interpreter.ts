export class Interpreter {
  // This stores our variables
  private variables = new Map<string, any>();

  public interpret(node: any): any {
    switch (node.type) {
      case "Program":
        node.body.forEach((stmt: any) => this.interpret(stmt));
        return this.variables; // Return the final state of variables

      case "Comment":
        // Comments are ignored in execution
        return;

      case "Function":
        const funcData = {
          parameters: node.parameters,
          body: node.body,
          type: "user-defined-function"
        };
        this.variables.set(node.name, funcData);
        return null;

      case "CallExpression":
          // 1. Look up the function data we stored during the "Function" case
          const functionData = this.variables.get(node.callee);

          if (!functionData || functionData.type !== "user-defined-function") {
            throw new Error(`Runtime Error: '${node.callee}' is not a function.`);
          }

          // 2. Map arguments to parameters
          // Note: For now, this uses your global 'this.variables' map.
          node.arguments.forEach((arg: any, index: number) => {
            const paramName = functionData.parameters[index];
            const value = this.interpret(arg);
            this.variables.set(paramName, value);
          });

          // 3. Execute the function body
          let lastResult = null;
          functionData.body.forEach((stmt: any) => {
            lastResult = this.interpret(stmt);
          });

          return lastResult;


      case "VariableDeclaration":
        const val = this.interpret(node.value);
        this.variables.set(node.identifier, val);
        return val;
    
      case "PrintStatement":
        const output = this.interpret(node.expression);
        console.log(output);
        return output;

      case "BinaryExpression":
        const left = this.interpret(node.left);
        const right = this.interpret(node.right);
        
        switch (node.operator) {
          case "+": return left + right;
          case "-": return left - right;
          case "*": return left * right;
          case "/": return left / right;
          default: throw new Error(`Unknown operator: ${node.operator}`);
        }

      case "Literal":
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
}
