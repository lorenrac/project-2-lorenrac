#pragma once
#include <memory>
#include <vector>
#include <string>

// Forward declaration
class SymbolTable;

class ASTNode {
public:
  int line_number;
  ASTNode(int line);
  virtual ~ASTNode() = default;
  virtual std::string Evaluate() = 0;
};


// Whole program / whole scope block
class BlockNode : public ASTNode {
public:
  std::vector<std::shared_ptr<ASTNode>> statements;
  std::string Evaluate() override;
  BlockNode(int line);
  void AddStatement(std::shared_ptr<ASTNode> statement);
};



class PrintNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> expression;
  PrintNode(int line, std::shared_ptr<ASTNode> expr);
  std::string Evaluate() override;
};

class VarDeclNode : public ASTNode {
public:
  std::string var_name;
  std::shared_ptr<ASTNode> expr;
  VarDeclNode(int line, std::string name, std::shared_ptr<ASTNode> expr);
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
  AssignNode(int line, const std::string& name, std::shared_ptr<ASTNode> val);
  std::string Evaluate() override;
};

class LiteralNode : public ASTNode {
public:
  std::string value;
  LiteralNode(int line, std::string v);
  std::string Evaluate() override;
};

class VariableNode : public ASTNode {
public:
  std::string var_name;
  VariableNode(int line, std::string name);
  std::string Evaluate() override;
};

class NotNode : public ASTNode {
public:
  std::shared_ptr<ASTNode> inner;
  NotNode(int line, std::shared_ptr<ASTNode> inner_expr);
  std::string Evaluate() override;
};