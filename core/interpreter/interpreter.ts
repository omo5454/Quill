export class Interpreter {
  // This stores our variables
  private variables = new Map<string, any>();

  public interpret(node: any): any {
    switch (node.type) {
      case "Program":
        node.body.forEach((stmt: any) => this.interpret(stmt));
        return this.variables; // Return the final state of variables

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
