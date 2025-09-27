#include <memory>
#include <vector>
#include <string>

// Forward declaration
class SymbolTable;

class ASTNode {
public:
  int line_number;
  virtual ~ASTNode() = default;
  virtual std::string Evaluate() = 0;
};


// Whole program / whole scope block
class BlockNode : public ASTNode {
public:
  std::vector<std::shared_ptr<ASTNode>> statements;
  std::string Evaluate() override;
};



class PrintNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> expression;
  PrintNode(std::shared_ptr<ASTNode> expr) : expression(expr) {}
  std::string Evaluate() override;
};

class VarDeclNode : public ASTNode {
public:
  std::string var_name;
  std::shared_ptr<ASTNode> expr;
  VarDeclNode(std::string name, std::shared_ptr<ASTNode> expr);
  std::string Evaluate() override;
};

class IfNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> condition;
  std::shared_ptr<ASTNode> then_branch;
  std::shared_ptr<ASTNode> else_branch;
  std::string Evaluate() override;
};

class WhileNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> condition;
  std::shared_ptr<ASTNode> body;
  std::string Evaluate() override;
};



// Expressions
class BinaryOpNode : public ASTNode {
public:
  std::string op;
  std::shared_ptr<ASTNode> left, right;
  std::string Evaluate() override;
};

class AssignNode : public ASTNode {
public:
  std::string var_name;
  std::shared_ptr<ASTNode> value;
  std::string Evaluate() override;
};

class LiteralNode : public ASTNode {
public:
  std::string value;
  LiteralNode(std::string v) : value(v) {}
  std::string Evaluate() override { return value; }
};

class VariableNode : public ASTNode {
public:
  std::string var_name;
  VariableNode(std::string name) : var_name(name) {}
  std::string Evaluate() override;
};

class NotNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> inner;
  std::string Evaluate() override;
};