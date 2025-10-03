// AST.hpp
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "lexer.hpp"
#include <functional>


using emplex::Token;

// === Abstract Base Class for All AST Nodes ===
class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual void execute() = 0;
};

// === Expression Nodes ===
class ExprNode {
public:
  virtual ~ExprNode() noexcept = default;
  virtual std::string evaluate() = 0;
};

using ExprNodePtr = std::unique_ptr<ExprNode>;

// === Literal Expression ===
class LiteralExpr : public ExprNode {
  std::string value;
public:
  LiteralExpr(std::string val) : value(std::move(val)) {}
  std::string evaluate() override {
    return value;
  }
};

// === Variable Expression ===
class VariableExpr : public ExprNode {
  std::string name;
  std::function<std::string(const std::string&)> lookup;
public:
    virtual ~VariableExpr() noexcept override = default;
  VariableExpr(const std::string &name, std::function<std::string(const std::string&)> lookup)
    : name(name), lookup(lookup) {}

  std::string evaluate() override {
    return lookup(name);
  }
};

// === Binary Expression (e.g., a + b, a - b, etc.) ===
class BinaryExpr : public ExprNode {
  ExprNodePtr left;
  Token op;
  ExprNodePtr right;
public:
  BinaryExpr(ExprNodePtr left, Token op, ExprNodePtr right)
    : left(std::move(left)), op(op), right(std::move(right)) {}

  std::string evaluate() override {
    std::string l = left->evaluate();
    std::string r = right->evaluate();

    switch (op.id) {
      case emplex::Lexer::ID_PLUS: return l + r;
      case emplex::Lexer::ID_MINUS: {
        size_t pos = l.find(r);
        if (pos != std::string::npos) l.erase(pos, r.length());
        return l;
      }
      case emplex::Lexer::ID_SLASH: {
        size_t pos = l.find(r);
        return (pos != std::string::npos) ? l.substr(0, pos) : l;
      }
      case emplex::Lexer::ID_PERCENT: {
        size_t pos = l.find(r);
        return (pos != std::string::npos) ? l.substr(pos + r.length()) : "";
      }
      default:
        throw std::runtime_error("Invalid binary operator");
    }
  }
};

// === Statement Nodes ===
using ASTNodePtr = std::unique_ptr<ASTNode>;

// === Print Statement ===
class PrintNode : public ASTNode {
  ExprNodePtr expr;
public:
  PrintNode(ExprNodePtr expr) : expr(std::move(expr)) {}
  void execute() override {
    std::string val = expr->evaluate();
    std::cout << val << std::endl;
  }
};

// === Variable Declaration ===
class VarDeclNode : public ASTNode {
  std::string name;
  ExprNodePtr expr;
  std::function<void(const std::string&, const std::string&)> assign;
public:
  VarDeclNode(const std::string &name, ExprNodePtr expr,
              std::function<void(const std::string&, const std::string&)> assign)
    : name(name), expr(std::move(expr)), assign(assign) {}

  void execute() override {
    assign(name, expr->evaluate());
  }
};

// === Variable Assignment ===
class AssignNode : public ASTNode {
  std::string name;
  ExprNodePtr expr;
  std::function<void(const std::string&, const std::string&)> assign;
public:
  AssignNode(const std::string &name, ExprNodePtr expr,
             std::function<void(const std::string&, const std::string&)> assign)
    : name(name), expr(std::move(expr)), assign(assign) {}

  void execute() override {
    assign(name, expr->evaluate());
  }
};

// === Block of Statements ===
class BlockNode : public ASTNode {
  std::vector<ASTNodePtr> statements;
public:
  void addStatement(ASTNodePtr stmt) {
    statements.emplace_back(std::move(stmt));
  }

  void execute() override {
    for (auto& stmt : statements) {
      stmt->execute();
    }
  }
};

// === While Loop ===
class WhileNode : public ASTNode {
  std::function<bool()> condition;
  ASTNodePtr body;
public:
  WhileNode(std::function<bool()> cond, ASTNodePtr body)
    : condition(std::move(cond)), body(std::move(body)) {}

  void execute() override {
    while (condition()) {
      body->execute();
    }
  }
};

// === If Statement ===
class IfNode : public ASTNode {
  std::function<bool()> condition;
  ASTNodePtr thenBranch;
  ASTNodePtr elseBranch;
public:
  IfNode(std::function<bool()> cond, ASTNodePtr thenBranch, ASTNodePtr elseBranch = nullptr)
    : condition(std::move(cond)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

  void execute() override {
    if (condition()) {
      thenBranch->execute();
    } else if (elseBranch) {
      elseBranch->execute();
    }
  }
};
