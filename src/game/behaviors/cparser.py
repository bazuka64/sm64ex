import pycparser

c_code = """
int main() {
    int a = 10;
    int b = 20;
    return a + b;
}
"""

parser = pycparser.CParser()
ast = parser.parse(c_code)

class FuncDefVisitor(pycparser.c_ast.NodeVisitor):
    def visit_FuncDef(self, node):
        print(f"関数名: {node.decl.name}")
        
visitor = FuncDefVisitor()
visitor.visit(ast)