#include "AST.hpp"
#include "SymbolTable.hpp"
#include "helpers.hpp"

#include <iostream>

extern SymbolTable g_symbol_table;

// ASTNode Stuff
ASTNode::ASTNode(int line) : line_number(line) {}
ASTNode::~ASTNode() = default;



// BlockNode Stuff
BlockNode::BlockNode(int line) : ASTNode(line) {}

void BlockNode::AddStatement(std::shared_ptr<ASTNode> statement) {
  statements.push_back(std::move(statement));
}

std::string BlockNode::Evaluate() {
  g_symbol_table.EnterScope();
  for (auto& stmt : statements) {
    stmt->Evaluate();
  }
  g_symbol_table.ExitScope(this->line_number);
  return "";
}



// PrintNode Stuff
PrintNode::PrintNode(int line, std::shared_ptr<ASTNode> expr)
  : ASTNode(line), expression(std::move(expr)) {}

std::string PrintNode::Evaluate() {
  std::string result = expression->Evaluate();
  std::cout << result << std::endl;
  return result;
}



// LiteralNode Stuff
LiteralNode::LiteralNode(int line, std::string val)
  : ASTNode(line), value(val) {}

std::string LiteralNode::Evaluate() {
  return value;
}



// VariableNode Stuff
VariableNode::VariableNode(int line, std::string name)
  : ASTNode(line), var_name(name) {}

std::string VariableNode::Evaluate() {
  return g_symbol_table.GetValue(var_name, line_number);
}



// AssignNode Stuff
AssignNode::AssignNode(int line, const std::string& name, std::shared_ptr<ASTNode> val)
  : ASTNode(line), var_name(name), value(std::move(val)) {}

std::string AssignNode::Evaluate() {
  if (!g_symbol_table.VariableExists(var_name)) {
    Error(line_number, "Unknown variable '" + var_name + "'");
  }

  std::string val = value->Evaluate();
  g_symbol_table.SetValue(var_name, val, line_number);
  return val;
}



// VarDeclNode Stuff
VarDeclNode::VarDeclNode(int line, std::string name, std::shared_ptr<ASTNode> expr)
  : ASTNode(line), var_name(name), expr(std::move(expr)) {}

std::string VarDeclNode::Evaluate() {
  g_symbol_table.DeclareVariable(var_name, line_number);
  std::string val = expr->Evaluate();
  g_symbol_table.SetValue(var_name, val, line_number);
  return val;
}



// NotNode Stuff
NotNode::NotNode(int line, std::shared_ptr<ASTNode> inner_expr)
  : ASTNode(line), inner(std::move(inner_expr)) {}

std::string NotNode::Evaluate() {
  std::string result = inner->Evaluate();
  return result.empty() ? "1" : "";
}
