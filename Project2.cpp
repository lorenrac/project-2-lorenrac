// -- Some header files that are likely to be useful --
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
//#include <memory>

//#include "AST.hpp"          // Build file for Abstract Syntax Tree nodes
#include "helpers.hpp"         // A place to put useful helper functions.
#include "lexer.hpp"        // Auto-generate file from Emplex
//#include "SymbolTable.hpp"  // Build file for your own Symbol Table

using emplex::Lexer;        // Simplify use of Lexer and Token types.
using emplex::Token;

class StringStackPlusPlus {
private:
  const std::string filename;
  Lexer lexer;

  std::vector<std::string> stack;
  //std::unordered_map<std::string, std::string> symbol_table;
  std::vector<std::unordered_map<std::string, std::string>> symbol_stack;


  bool lastIfCondition = false;
  bool justProcessedIf = false;

  // === Helper Functions ===

  // A generic Error function that will provide a custom error for a given token.
  template <typename... Ts>
  void Error(Token token, Ts... message) {
    // Print the beginning of all error messages:
    std::cerr << "ERROR (line " << token.line_id << "): ";

    // Print out all of the other arguments as the main message.
    (std::cerr << ... << std::forward<Ts>(message));
    std::cerr << std::endl;

    exit(1); // Exit with a non-zero exit code.
  }

  // An easy way to throw an Unexpected Token error
  void UnexpectedToken(Token token) {
    Error(token, "Unexpected token '", token.lexeme, "'");
  }

  // Determine if the current line has more arguments to process.
  bool HasArg() {
    return lexer.Any() && lexer.Peek() != Lexer::ID_NEWLINE;
  }

  // Pop the top value off of the internal stack.
  std::string StackPop(const Token & token) {
    if (stack.size() == 0) Error(token, "Stack underflow");
    std::string out = stack.back();
    stack.pop_back();
    return out;
  }

  // Convert an ID token into the string value it represents.
  std::string IDToString(const Token & token) {
    assert(token == Lexer::ID_ID);
    const std::string var_name = token.lexeme;
    /*if (symbol_table.find(var_name) == symbol_table.end()) {
      Error(token, "Unknown variable '", var_name, "'");
    }
    return symbol_table[var_name];*/
    for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
      auto &scope = *scope_it;
      if (scope.find(var_name) != scope.end()) {
        return scope[var_name];
      }
    }
    Error(token, "Unknown variable '", var_name, "'");
    return "";
  }

  // Convert a literal string token into the string value it represents.
  std::string LiteralToString(const Token & token) {
    // Simple version: cut off both ends.
    // (A more complex version would translate escape characters)
    return token.lexeme.substr(1, token.lexeme.size()-2);
  }

  // Translate a particular token to a string.
  std::string TokenToString(const Token & token) {
    // If we have a variable name, get its contents.
    if (token == Lexer::ID_ID) return IDToString(token);

    // If we have a literal string, clean it up and return it.
    if (token == Lexer::ID_LIT_STRING) return LiteralToString(token);

    // A quote by itself indicates a non-terminating string literal.
    if (token == '\'' || token == '"') {
      Error(token, "Non-terminating string literal");
    }

    // Otherwise we have an unexpected token!
    UnexpectedToken(token);
    return "";
  }

  std::string ApplyOperator(const Token &op, const std::string &left, const std::string &right) {
    std::string result = left;

    switch (op.id) {
      case Lexer::ID_PLUS: {
        result += right;
        break;
      }
      case Lexer::ID_MINUS: {
        size_t pos = result.find(right);
        if (pos != std::string::npos) {
          result.erase(pos, right.length());
        }
        break;
      }
      case Lexer::ID_SLASH: {
        size_t pos = result.find(right);
        if (pos != std::string::npos) {
          result = result.substr(0, pos);
        }
        break;
      }
      case Lexer::ID_PERCENT: {
        size_t pos = result.find(right);
        if (pos != std::string::npos) {
          result = result.substr(pos + right.length());
        }
        break;
      }
      default:
      Error(op, "Unknown operator");
    }
    return result;
  }

  // Highest level: handles PLUS and MINUS (lowest precedence)
  std::string ParseExpr(const Token &first) {
    std::string left = ParseTerm(first);
    while (lexer.Any() && (lexer.Peek() == Lexer::ID_PLUS || lexer.Peek() == Lexer::ID_MINUS)) {
      Token op = lexer.Use();
      if (!lexer.Any()) Error(op, "Expected value after operator");
      Token next = lexer.Use();
      std::string right = ParseTerm(next);
      left = ApplyOperator(op, left, right);
    }
    return left;
  }

  // High-level: handles SLASH and PERCENT
  std::string ParseTerm(const Token &first) {
    std::string left = ParsePrimary(first);

    while (lexer.Any() && (lexer.Peek() == Lexer::ID_SLASH || lexer.Peek() == Lexer::ID_PERCENT)) {
      Token op = lexer.Use();
      if (!lexer.Any()) Error(op, "Expected value after operator");
      Token next = lexer.Use();
      std::string right = ParsePrimary(next);
      left = ApplyOperator(op, left, right);
    }
    return left;
  }

  // Base-level: just handles a literal or variable
  std::string ParsePrimary(const Token &token) {
    if (token == Lexer::ID_ID || token == Lexer::ID_LIT_STRING) {
      return TokenToString(token);
    }

    if (token == Lexer::ID_LPAREN) {
      if (!lexer.Any()) {
        Error(token, "Expected expression after '('");
      }

      Token next = lexer.Use();
      std::string value = ParseExpr(next);

      if (!lexer.Any() || lexer.Peek() != Lexer::ID_RPAREN) {
        Error(token, "Expected ')' to close parenthesized expression");
      }
      lexer.Use(); // consume ')'
      return value;
    }

    Error(token, "Expected string literal, variable, or parenthesized expression");
    return "";
  }


  std::string CompleteCalculation(const Token & token) {
    Token current = token;
    return ParseExpr(token);
  }

  bool ParseExpression() {
    bool valid = false;
    bool notPresent = false;
    bool rightSide = false;
    std::string leftValue;
    std::string rightValue;
    Token op;

    // if token id is Lexer::ID_NOT
    Token current = lexer.Use();
    if (current == Lexer::ID_NOT) {
      // not = true
      //move to next token
      notPresent = true;
      if (!lexer.Any()) Error(current, "Expected expression after NOT");
      current = lexer.Use();
    }

    // if token id is Lexer::ID_ID or token id is Lexer::ID_LIT_STRING
    if (current == Lexer::ID_ID || current == Lexer::ID_LIT_STRING) {
      // set left side variable to the value of the token
      leftValue = TokenToString(current);

      // if token id is NOT Lexer::ID_RPAREN
      if (lexer.Any() && lexer.Peek() != Lexer::ID_RPAREN) {
        rightSide = true;

        // if token id is an operator id
        Token possible_op = lexer.Use();
        switch (possible_op.id) {
          case Lexer::ID_EQ:
          case Lexer::ID_NEQ:
          case Lexer::ID_LE:
          case Lexer::ID_GE:
          case Lexer::ID_LT:
          case Lexer::ID_GT:
          case Lexer::ID_QUESTION:
            // set operator type
            op = possible_op;
            break;
          default:
            Error(possible_op, "Expected comparison operator, got '", possible_op.lexeme, "'");
        }

        // if token id is Lexer::ID_ID or token id is Lexer::ID_LIT_STRING
        if (!lexer.Any()) Error(op, "Expected right-hand expression after operator");
        // set right side variable to the value of the token
        Token right = lexer.Use();
        if (right == Lexer::ID_ID || right == Lexer::ID_LIT_STRING) {
          rightValue = TokenToString(right);
        } else {
          Error(right, "Expected identifier or string literal after operator");
        }
      }
    } 
    else {
      Error(current, "Expected identifier or string literal in expression");
    }

    if (!rightSide) {
      valid = !leftValue.empty();
    } 
    else {
      // Compare leftValue and rightValue based on operator
      if (op == Lexer::ID_EQ)        valid = (leftValue == rightValue);
      else if (op == Lexer::ID_NEQ)  valid = (leftValue != rightValue);
      else if (op == Lexer::ID_LT)   valid = (leftValue < rightValue);
      else if (op == Lexer::ID_LE)   valid = (leftValue <= rightValue);
      else if (op == Lexer::ID_GT)   valid = (leftValue > rightValue);
      else if (op == Lexer::ID_GE)   valid = (leftValue >= rightValue);
      else if (op == Lexer::ID_QUESTION) {
        valid = (leftValue.find(rightValue) != std::string::npos);
      }
      else Error(op, "Unknown operator in expression");
    }

    if (notPresent) {
      valid = !valid;
    }
    return valid;
  }


public:
  StringStackPlusPlus(std::string filename) : filename(filename) { 
    symbol_stack.push_back({});
  }

  // Run the entire program.
  void Run() {
    std::ifstream fs(filename);
    lexer.Tokenize(fs);

    while (lexer.Any()) { ProcessLine(); }
  }

  // Interpret the next full line of code.
  void ProcessLine() {
    //std::cout<< "ProcessLine reached" << std::endl;
    assert(lexer.Any()); // Make sure there's something to process.

    // Figure out which type of token we are working with.
    auto token = lexer.Use();
    
    switch (token) {
      case Lexer::ID_PRINT:  {
        ProcessPRINT(token);  
        break;
      }
      case Lexer::ID_IF:   {
        //std::cout<< "Is a if line" << std::endl;
        ProcessIF(token);   
        break;
      }
      case Lexer::ID_ELSE:   {
        //std::cout<< "Is a if line" << std::endl;
        ProcessELSE(token);   
        break;
      }
      case Lexer::ID_WHILE: {
        ProcessWHILE(token); 
        break;
      }
      case Lexer::ID_VAR:  {
        ProcessVAR(token);  
        break;
      }
      case Lexer::ID_ID: {
        ProcessID(token);
        break;
      }

      case Lexer::ID_LBRACE: {
        ProcessLBRACE();
        break;
      }
      case Lexer::ID_RBRACE: {
        ProcessRBRACE(token);
        break;
      }
      case Lexer::ID_NEWLINE: return; // Empty line -- nothing to process.
      default:
        // If we made it here, this is not a valid line.
        //std::cout << "1 ERROR REACHED" << std::endl;
        Error(token, "Unknown command '", token.lexeme, "'");
    }

    // Make sure the line ends in a newline.
    if (lexer.Any()) {
      Token line_end = lexer.Use();
      if (line_end != Lexer::ID_NEWLINE) {
        //std::cout <<"Unexpected reached" << std::endl;
        UnexpectedToken(line_end);
      }
      else if (line_end == Lexer::ID_IF) {
        //std::cout << "UNEXPECTED IF REACHED" << std::endl;
      }
    }
  }

  void ProcessSingleStatement() {
    if (!lexer.Any()) return;
    Token token = lexer.Use();

    switch (token.id) {
      case Lexer::ID_PRINT:
        ProcessPRINT(token);
        break;
      case Lexer::ID_IF:
        ProcessIF(token);
        break;
      case Lexer::ID_VAR:
        ProcessVAR(token);
        break;
      case Lexer::ID_WHILE:
        ProcessWHILE(token);
        break;
      case Lexer::ID_ID:
        ProcessID(token);
        break;
      default:
        UnexpectedToken(token);
    }
  }


  
  void ProcessPRINT(const Token &token) {
    bool reverse = false;
    std::string out;

    if (!HasArg()) {
      out = StackPop(token);
    } else {
      Token next = lexer.Peek();

      if (next.id == Lexer::ID_NOT) {
        reverse = true;
        lexer.Use();
        next = lexer.Peek();
      }

      if (!lexer.Any()) Error(token, "Expected expression in PRINT");
      Token first = lexer.Use();

      if (first == Lexer::ID_LPAREN) {
        Token lookahead = lexer.Peek();
        Token lookahead2 = lexer.Peek(1);

        // Is it a boolean expression?
        if ((lookahead == Lexer::ID_ID || lookahead == Lexer::ID_LIT_STRING) && (lookahead2 == Lexer::ID_EQ || lookahead2 == Lexer::ID_NEQ || lookahead2 == Lexer::ID_LE || lookahead2 == Lexer::ID_GE || lookahead2 == Lexer::ID_LT || lookahead2 == Lexer::ID_GT || lookahead2 == Lexer::ID_QUESTION)) {
          bool result = ParseExpression();

          // Check for RPAREN
          if (!lexer.Any() || lexer.Peek() != Lexer::ID_RPAREN) {
            Error(token, "Expected ')' after expression in PRINT");
          }
          lexer.Use();

          out = result ? "1" : "";
        } else {
          out = CompleteCalculation(first);
        }
      }
      else {
        out = CompleteCalculation(first);
      }
    }

    if (out == "" && reverse) out = "1";
    std::cout << out << std::endl;
  }




  void ProcessIF(const Token & token) {
    // Handle LParen
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_LPAREN) {
      Error(token, "Expected '(' after IF");
    }
    lexer.Use();

    bool condition = ParseExpression();
    lastIfCondition = condition;
    justProcessedIf = true;

    if (!lexer.Any() || lexer.Peek() != Lexer::ID_RPAREN) {
      Error(token, "Expected ')' after IF");
    }
    lexer.Use();

    if (lexer.Any() && lexer.Peek() == Lexer::ID_LBRACE) {
      //ProcessLBRACE();
      lexer.Use();

      if (condition) {
        // Execute lines until '}'
        while (lexer.Any() && lexer.Peek() != Lexer::ID_RBRACE) {
          ProcessLine();
        }
      } else {
        // Skip lines until matching '}'
        int brace_depth = 1;
        while (lexer.Any() && brace_depth > 0) {
          Token next = lexer.Peek();
          if (next == Lexer::ID_LBRACE) {
            //ProcessLBRACE();
            brace_depth++;
          }
          else if (next == Lexer::ID_RBRACE) {
            //ProcessRBRACE(next);
            brace_depth--;
          }
          if (brace_depth != 0) {
              next = lexer.Use();
          }
        }
      }

      // Make sure we found the closing '}'
      if (!lexer.Any() || lexer.Peek() != Lexer::ID_RBRACE) {
        Error(token, "Expected '}' to close IF block");
      }
      //ProcessRBRACE(lexer.Peek());
      lexer.Use(); // consume '}'
    } else {
      // Single line IF
      //std::cout<< "Single line if" << std::endl;
      if (condition) {
        //std::cout<< "Single line if true" << std::endl;
        ProcessSingleStatement();
      } else {
        // Skip the next line
        //std::cout<< "Single line if false" << std::endl;
        while (lexer.Any()) {
          Token tok = lexer.Peek();
          if (tok == Lexer::ID_NEWLINE) return;
          tok = lexer.Use();
        }
      }
    }

  }

  void ProcessELSE(const Token & token) {
    if (!justProcessedIf) {
      Error(token, "ELSE without matching IF");
    }
    justProcessedIf = false;  // Reset after ELSE
    
    if (lexer.Any() && lexer.Peek() == Lexer::ID_LBRACE) {
      //ProcessLBRACE();
      lexer.Use();
      if (!lastIfCondition) {
        // Run else statement
        while (lexer.Any() && lexer.Peek() != Lexer::ID_RBRACE) {
          ProcessLine();
        }
      } else {
        // Skip else statement
        int brace_depth = 1;
        while (lexer.Any() && brace_depth > 0) {
          Token next = lexer.Peek();
          if (next == Lexer::ID_LBRACE) {
            //ProcessLBRACE();
            brace_depth++;
          }
          else if (next == Lexer::ID_RBRACE) {
            //ProcessRBRACE(next);
            brace_depth--;
          }
          if (brace_depth != 0) {
              next = lexer.Use();
          }
        }
      }

      // Consume '}'
      if (!lexer.Any() || lexer.Peek() != Lexer::ID_RBRACE) {
        Error(token, "Expected '}' to close ELSE block");
      }
      //ProcessRBRACE(lexer.Peek());
      lexer.Use();
    } else {
      // Single-line else
      if (!lastIfCondition) {
        ProcessSingleStatement();
      } else {
        // Skip single statement
        while (lexer.Any()) {
          Token tok = lexer.Peek();
          if (tok == Lexer::ID_NEWLINE) return;
          tok = lexer.Use();
        }
      }
    }
  }

  bool ParseWHILEExpression(const std::vector<Token>& expression) {
    bool valid = false;
    bool notPresent = false;
    bool rightSide = false;
    std::string leftValue;
    std::string rightValue;
    Token op;

    size_t index = 0;
    auto next = [&]() -> Token {
      if (index >= expression.size()) {
          Error(expression.back(), "Unexpected end of expression");
      }
      return expression[index++];
    };

    Token current = next();
    if (current == Lexer::ID_NOT) {
      notPresent = true;
      if (index >= expression.size()) {
          Error(current, "Expected expression after NOT");
      }
      current = next();
    }

    if (current == Lexer::ID_ID || current == Lexer::ID_LIT_STRING) {
      if (current == Lexer::ID_ID) {
        std::string name = current.lexeme;
        bool leftFound = false;
        for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
          auto it = scope_it->find(name);
          if (it != scope_it->end()) {
            leftValue = scope_it->find(name)->second;
            leftFound = true;
            break;
          }
        }
        if (!leftFound) Error(current, "Undefined variable '", name, "'");
      }else {
        leftValue = TokenToString(current);
      }

      if (index < expression.size()) {
        rightSide = true;
        Token possible_op = next();

        switch (possible_op.id) {
          case Lexer::ID_EQ:
          case Lexer::ID_NEQ:
          case Lexer::ID_LE:
          case Lexer::ID_GE:
          case Lexer::ID_LT:
          case Lexer::ID_GT:
          case Lexer::ID_QUESTION:
            op = possible_op;
              break;
            default:
              Error(possible_op, "Expected comparison operator, got '", possible_op.lexeme, "'");
        }
        if (index >= expression.size()) {
            Error(op, "Expected right-hand expression after operator");
        }

        Token right = next();
        if (right == Lexer::ID_ID || right == Lexer::ID_LIT_STRING) {
          if (right == Lexer::ID_ID) {
          std::string name = right.lexeme;
          bool rightFound = false;
          for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
            auto it = scope_it->find(name);
            if (it != scope_it->end()) {
            rightValue = scope_it->find(name)->second;
            rightFound = true;
            break;
            }
          }
          if (!rightFound) Error(right, "Undefined variable '", name, "'");
        }else {
          rightValue = TokenToString(right);
        }
        } else {
          Error(right, "Expected identifier or string literal after operator");
        }
      }
    } else {
      Error(current, "Expected identifier or string literal in expression");
    }

    if (!rightSide) {
      if (leftValue == "") {
        valid = false;
      }
      else {
        valid = true;
      }
    } else {
      if (op == Lexer::ID_EQ)         valid = (leftValue == rightValue);
      else if (op == Lexer::ID_NEQ)   valid = (leftValue != rightValue);
      else if (op == Lexer::ID_LT)    valid = (leftValue < rightValue);
      else if (op == Lexer::ID_LE)    valid = (leftValue <= rightValue);
      else if (op == Lexer::ID_GT)    valid = (leftValue > rightValue);
      else if (op == Lexer::ID_GE)    valid = (leftValue >= rightValue);
      else if (op == Lexer::ID_QUESTION) valid = (leftValue.find(rightValue) != std::string::npos);
      else Error(op, "Unknown operator in expression");
    }

    if (notPresent) {
      valid = !valid;
    }

    return valid;
  }

  int ProcessPRINTFromVector(const std::vector<Token>& tokens, size_t k) {
    bool reverse = false;
    std::string out;

    const Token& token = tokens[k];
    k++;

    if (k >= tokens.size()) {
      out = StackPop(token);
    } else {
      Token next = tokens[k];

      if (next.id == Lexer::ID_NOT) {
        reverse = true;
        k++;
        if (k >= tokens.size()) Error(token, "Expected expression after NOT in PRINT");
      }
      if (tokens[k].id == Lexer::ID_LPAREN) {
        int parenDepth = 1;
        int start = k + 1;
        size_t end = start;

        while (end < tokens.size() && parenDepth > 0) {
            if (tokens[end].id == Lexer::ID_LPAREN) parenDepth++;
            else if (tokens[end].id == Lexer::ID_RPAREN) parenDepth--;
            end++;
        }
        if (parenDepth != 0) {
          Error(tokens[k], "Unmatched parentheses in PRINT expression");
        }

        std::vector<Token> subExpr(tokens.begin() + start, tokens.begin() + end - 1);
        if (subExpr.size() >= 3 &&
          (subExpr[1].id == Lexer::ID_EQ || subExpr[1].id == Lexer::ID_NEQ ||
            subExpr[1].id == Lexer::ID_LE || subExpr[1].id == Lexer::ID_GE ||
            subExpr[1].id == Lexer::ID_LT || subExpr[1].id == Lexer::ID_GT ||
            subExpr[1].id == Lexer::ID_QUESTION)) {
          bool result = ParseWHILEExpression(subExpr);
          out = result ? "1" : "";
        } else {
          size_t dummy = 0;
          out = CompleteCalculationFromVector(subExpr, 0, &dummy);
        }
        k = end;
      }
      else {
        out = CompleteCalculationFromVector(tokens, k, &k);
      }
    }

    if (reverse && out.empty()) {
      out = "1";
    }

    std::cout << out << std::endl;
    return k;
  }



  int ProcessIFFromVector(const std::vector<Token>& tokens, size_t k) {
    const Token& token = tokens[k];
    k++;

    if (k >= tokens.size() || tokens[k].id != Lexer::ID_LPAREN) {
      Error(token, "Expected '(' after IF");
    }
    k++;

    int parenDepth = 1;
    int exprStart = k;
    size_t exprEnd = k;

    while (exprEnd < tokens.size() && parenDepth > 0) {
      if (tokens[exprEnd].id == Lexer::ID_LPAREN) parenDepth++;
      else if (tokens[exprEnd].id == Lexer::ID_RPAREN) parenDepth--;
      exprEnd++;
    }

    if (parenDepth != 0) {
      Error(token, "Unclosed '(' in IF condition");
    }

    // Check condition
    std::vector<Token> conditionExpr(tokens.begin() + exprStart, tokens.begin() + exprEnd - 1);
    bool condition = ParseWHILEExpression(conditionExpr);
    lastIfCondition = condition;
    justProcessedIf = true;

    k = exprEnd;

    if (k < tokens.size() && tokens[k].id == Lexer::ID_LBRACE) {
      k++;

      if (condition) {
        // IF block
        int braceDepth = 1;
        int stmtStart = k;
        while (k < tokens.size() && braceDepth > 0) {
          if (tokens[k].id == Lexer::ID_LBRACE) braceDepth++;
          else if (tokens[k].id == Lexer::ID_RBRACE) braceDepth--;
          if (braceDepth > 0) k++;
        }

        if (braceDepth != 0) {
          Error(token, "Expected '}' to close IF block");
        }

        size_t execK = stmtStart;
        while (execK < k) {
          execK = ProcessLineFromVector(tokens, execK);
        }

          k++;
      } else {
        // Skip IF block
        int braceDepth = 1;
        while (k < tokens.size() && braceDepth > 0) {
          if (tokens[k].id == Lexer::ID_LBRACE) braceDepth++;
          else if (tokens[k].id == Lexer::ID_RBRACE) braceDepth--;
          k++;
        }

        if (braceDepth != 0) {
          Error(token, "Expected '}' to close skipped IF block");
        }
      }
    } else {
      // Single-line IF
      if (condition) {
        k = ProcessLineFromVector(tokens, k);
      } else {
        // Skip Single-line IF
        while (k < tokens.size() && tokens[k].id != Lexer::ID_NEWLINE &&
          tokens[k].id != Lexer::ID_RBRACE) {
          k++;
        }
      }
    }
    return k;
  }

  int ProcessELSEFromVector(const std::vector<Token>& tokens, size_t k) {
    const Token& token = tokens[k];

    if (!justProcessedIf) {
      Error(token, "ELSE without matching IF");
    }
    justProcessedIf = false;

    k++;

    if (k < tokens.size() && tokens[k].id == Lexer::ID_LBRACE) {
      k++;

      if (!lastIfCondition) {
        // ELSE block
        int braceDepth = 1;
        int stmtStart = k;
        while (k < tokens.size() && braceDepth > 0) {
          if (tokens[k].id == Lexer::ID_LBRACE) braceDepth++;
          else if (tokens[k].id == Lexer::ID_RBRACE) braceDepth--;

          if (braceDepth > 0) k++;
        }

        if (braceDepth != 0) {
          Error(token, "Expected '}' to close ELSE block");
        }

        size_t execK = stmtStart;
        while (execK < k) {
          execK = ProcessLineFromVector(tokens, execK);
        }

        k++;
      } else {
        // Skip ELSE block
        int braceDepth = 1;
        while (k < tokens.size() && braceDepth > 0) {
          if (tokens[k].id == Lexer::ID_LBRACE) braceDepth++;
          else if (tokens[k].id == Lexer::ID_RBRACE) braceDepth--;
          k++;
        }

        if (braceDepth != 0) {
          Error(token, "Expected '}' to close ELSE block");
        }
      }
    } else {
      // Single-line ELSE
      if (!lastIfCondition) {
        k = ProcessLineFromVector(tokens, k);
      } else {
        // Skip single-line else
        while (k < tokens.size() &&
          tokens[k].id != Lexer::ID_NEWLINE &&
          tokens[k].id != Lexer::ID_RBRACE) {
          k++;
        }
      }
    }
    return k;
  }

  int ProcessVARFromVector(const std::vector<Token>& tokens, size_t k) {
    const Token& token = tokens[k];
    k++;

    if (k >= tokens.size() || tokens[k].id != Lexer::ID_ID) {
      Error(token, "Expected identifier after VAR");
    }

    Token var_token = tokens[k++];
    std::string var_name = var_token.lexeme;

    // ensure new variable
    auto& current_scope = symbol_stack.back();
    if (current_scope.find(var_name) != current_scope.end()) {
      Error(var_token, "Variable '", var_name, "' already declared in this scope");
    }

    if (k >= tokens.size() || tokens[k].id != Lexer::ID_ASSIGN) {
      Error(var_token, "Expected '=' after variable name");
    }
    k++;

    if (k >= tokens.size()) {
      Error(var_token, "Expected expression after '='");
    }

    std::string result;
    Token current = tokens[k];
    if (current.id == Lexer::ID_ID || current.id == Lexer::ID_LIT_STRING) {
      result = TokenToString(current);
      k++;
    } else {
      Error(current, "Expected string literal or variable in expression");
    }

    while (k < tokens.size() && tokens[k].id == Lexer::ID_PLUS) {
      k++;

      if (k >= tokens.size()) {
        Error(current, "Expected value after '+'");
      }

      Token next = tokens[k++];
      if (next.id == Lexer::ID_ID || next.id == Lexer::ID_LIT_STRING) {
        result += TokenToString(next);
      } else {
        Error(next, "Expected string literal or variable after '+'");
      }
    }

    // Chains
    if (k < tokens.size() && tokens[k].id == Lexer::ID_ASSIGN) {
      k++;

      if (k >= tokens.size()) {
        Error(current, "Expected value after '='");
      }

      Token next2 = tokens[k++];
      if (next2.id == Lexer::ID_ID || next2.id == Lexer::ID_LIT_STRING) {
        result = TokenToString(next2);

        if (current.id == Lexer::ID_ID) {
          current_scope[current.lexeme] = result;
        }
      } else {
        Error(next2, "Expected string literal or variable after '='");
      }
    }

    current_scope[var_name] = result;

    return k;
  }

  int ProcessIDFromVector(const std::vector<Token>& tokens, size_t k) {
    if (k >= tokens.size()) {
      Error(Token{}, "Unexpected end of tokens in ProcessIDFromVector");
    }

    const Token& token = tokens[k++];
    std::string name = token.lexeme;

    // Ensure variable already exists
    bool found = false;
    for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
      if (scope_it->find(name) != scope_it->end()) {
        found = true;
        break;
      }
    }
    if (!found) {
      Error(token, "Assignment to undeclared variable '", name, "'");
    }

    if (k >= tokens.size() || tokens[k].id != Lexer::ID_ASSIGN) {
      Error(token, "Expected '=' after variable name");
    }
    k++;

    if (k >= tokens.size()) {
      Error(token, "Expected expression after '='");
    }

    bool reverse = false;
    Token first = tokens[k];

    if (first.id == Lexer::ID_NOT) {
      reverse = true;
      k++;
      if (k >= tokens.size()) {
        Error(token, "Expected expression after '!'");
      }
      first = tokens[k];
    }

    std::string value;

    if (first.id == Lexer::ID_ID || first.id == Lexer::ID_LIT_STRING || first.id == Lexer::ID_LPAREN) {
      std::vector<Token> exprTokens;
      int depth = 0;

      while (k < tokens.size()) {
        Token t = tokens[k];
        if (t.id == Lexer::ID_LPAREN) depth++;
        else if (t.id == Lexer::ID_RPAREN) {
          if (depth == 0) break;
          depth--;
        } else if (depth == 0 && t.id == Lexer::ID_NEWLINE) break;
        exprTokens.push_back(t);
        k++;
      }
      int dummyIndex = 0;
      value = CompleteCalculationFromVector(exprTokens, dummyIndex);
    } else {
        Error(first, "Expected identifier, string literal, or expression after '='");
    }

    if (reverse) {
      value = (value.empty()) ? "1" : "";
    }

    // Reassign variable
    for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
      if (scope_it->find(name) != scope_it->end()) {
        (*scope_it)[name] = value;
        break;
      }
    }
    return k;
  }


  int ProcessLineFromVector(const std::vector<Token>& tokens, size_t k) {
    if (k >= tokens.size()) {
      Error(Token{}, "Unexpected end of tokens in ProcessLineFromVector");
    }
    const Token& token = tokens[k++];
    switch (token.id) {
      case Lexer::ID_PRINT:
        k = ProcessPRINTFromVector(tokens, k);
        break;
      case Lexer::ID_IF:
        k = ProcessIFFromVector(tokens, k);
        break;
      case Lexer::ID_ELSE:
        k = ProcessELSEFromVector(tokens, k);
        break;
      case Lexer::ID_WHILE:
        break;
      case Lexer::ID_VAR:
        k = ProcessVARFromVector(tokens, k);
        break;
      case Lexer::ID_ID:
        k = ProcessIDFromVector(tokens, k);
        break;
      case Lexer::ID_LBRACE:
        break;
      case Lexer::ID_RBRACE:
        break;
      case Lexer::ID_NEWLINE:
        return k;
        break;
      default:
        Error(token, "Unknown command '", token.lexeme, "'");
    }
    if (k < tokens.size()) {
      if (tokens[k].id == Lexer::ID_NEWLINE) {
        k++;
      } else {
        UnexpectedToken(tokens[k]);
      }
    }
    return k;
  }

  std::string CompleteCalculationFromVector(const std::vector<Token>& tokens, size_t k, size_t* outIndex = nullptr) {
    if (k >= tokens.size()) {
      Error(Token{}, "Expected expression but reached end of tokens");
    }

    std::vector<Token> expressionTokens;
    int depth = 0;

    while (k < tokens.size()) {
      const Token& t = tokens[k];

      if (t.id == Lexer::ID_LPAREN) {
        depth++;
      } else if (t.id == Lexer::ID_RPAREN) {
        if (depth == 0) break;
        depth--;
      } else if (depth == 0 && t.id == Lexer::ID_NEWLINE) {
        break;
      }
      expressionTokens.push_back(t);
      k++;
    }

    if (expressionTokens.empty()) {
      Error(tokens[k], "Expected expression for calculation");
    }

    size_t exprIndex = 0;
    std::string result = ParseExprFromVector(expressionTokens, exprIndex);

    if (outIndex) {
      *outIndex = k;
    }

    return result;
  }


  std::string ParseExprFromVector(const std::vector<Token>& tokens, size_t& index) {
    if (index >= tokens.size()) {
      Error(Token{}, "Expected expression but reached end of tokens");
    }

    std::string left = ParseTermFromVector(tokens, index);

    while (index < tokens.size() && 
      (tokens[index].id == Lexer::ID_PLUS || tokens[index].id == Lexer::ID_MINUS)) {
        
      Token op = tokens[index];
      index++;

      if (index >= tokens.size()) {
        Error(op, "Expected value after operator");
      }

      std::string right = ParseTermFromVector(tokens, index);
      left = ApplyOperator(op, left, right);
    }
    return left;
  }

  std::string ParseTermFromVector(const std::vector<Token> &tokens, size_t &index) {
    if (index >= tokens.size()) {
      Error(Token{}, "Expected term but reached end of tokens");
    }

    std::string left = ParsePrimaryFromVector(tokens, index);

    while (index < tokens.size() && (tokens[index].id == Lexer::ID_SLASH || tokens[index].id == Lexer::ID_PERCENT)) {
      Token op = tokens[index];
      index++;  // consume operator

      if (index >= tokens.size()) {
        Error(op, "Expected value after operator");
      }

      std::string right = ParsePrimaryFromVector(tokens, index);
      left = ApplyOperator(op, left, right);
    }
    return left;
  }

  std::string ParsePrimaryFromVector(const std::vector<Token> &tokens, size_t &index) {
    if (index >= tokens.size()) {
      Error(Token{}, "Expected expression but reached end of tokens");
    }

    Token token = tokens[index];
    index++;

    if (token == Lexer::ID_ID || token == Lexer::ID_LIT_STRING) {
      return TokenToString(token);
    }

    if (token == Lexer::ID_LPAREN) {
      if (index >= tokens.size()) {
        Error(token, "Expected expression after '('");
      }

      std::string value = ParseExprFromVector(tokens, index);

      if (index >= tokens.size() || tokens[index] != Lexer::ID_RPAREN) {
        Error(token, "Expected ')' to close parenthesized expression");
      }
      index++;
      return value;
    }

    Error(token, "Expected string literal, variable, or parenthesized expression");
    return "";
  }


  void ProcessWHILE(const Token & token) {
    // Check for '('
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_LPAREN) {
      Error(token, "Expected '(' after WHILE");
    }
    lexer.Use();

    // create condition vector
    std::vector<Token> cond;
    int depth = 1;
    while (lexer.Any() && depth > 0) {
      Token t = lexer.Use();
      if (t.id == Lexer::ID_LPAREN) depth++;
      else if (t.id == Lexer::ID_RPAREN) depth--;
      if (depth > 0) {
        cond.push_back(t);
      }
    }

    // Check for '{'
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_LBRACE) {
      Error(token, "Expected '{' after WHILE condition");
    }
    lexer.Use();

    // create body vector
    std::vector<Token> body;
    int brace_depth = 1;
    while (lexer.Any() && brace_depth > 0) {
      Token t = lexer.Use();
      if (t.id == Lexer::ID_LBRACE) brace_depth++;
      else if (t.id == Lexer::ID_RBRACE) brace_depth--;
      if (brace_depth > 0) {
        body.push_back(t);
      }
    }

    while (ParseWHILEExpression(cond)) {
      size_t k = 0;
      while (k < body.size()) {
        Token current = body[k];
        switch (current.id) {
          case Lexer::ID_PRINT:
            k = ProcessPRINTFromVector(body, k);
            break;
          case Lexer::ID_VAR:
            k = ProcessVARFromVector(body, k);
            break;
          case Lexer::ID_ID:
            k = ProcessIDFromVector(body, k);
            break;
          case Lexer::ID_IF:
            k = ProcessIFFromVector(body, k);
            break;
          case Lexer::ID_ELSE:
            k = ProcessELSEFromVector(body, k);
            break;
          case Lexer::ID_NEWLINE:
            k++;
            break;
          default:
            Error(current, "Unknown command '", current.lexeme, "'");
        }
      }
    }
  }


  void ProcessVAR(const Token & token) {
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_ID) {
      Error(token, "Expected identifer after VAR");
    }
    Token var_token = lexer.Use();
    Token next = var_token;

    // store variable name
    std::string var_name = var_token.lexeme;

    // check redeclaration
    auto &current_scope = symbol_stack.back();
    if (current_scope.find(var_name) != current_scope.end()) {
      Error(var_token, "Variable '", var_name, "' already declared in this scope");
    }

    // consume '=' operator
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_ASSIGN) {
      Error(var_token, "Expected '=' after variable name");
    }
    next = lexer.Use();

    // store variable value (is Lexer::ID_LIT_STRING)
    std::string result;
    if (!lexer.Any()) Error(var_token, "Expected expression after '='");
    Token current = lexer.Use();
    next = current;
    if (current == Lexer::ID_ID || current == Lexer::ID_LIT_STRING) {
      result = TokenToString(current);
    } else {
      Error(current, "Expected string literal or variable in expression");
    }

    // check if next token is '+' operator
    // if is is, add token after '+' to the variable value
    while (lexer.Any() && lexer.Peek() == Lexer::ID_PLUS) {
      next = lexer.Use(); // consume '+'

      if (!lexer.Any()) Error(current, "Expected value after '+'");
      Token next1 = lexer.Use();
      next = next1;
      if (next1 == Lexer::ID_ID || next1 == Lexer::ID_LIT_STRING) {
        result += TokenToString(next1);
      } else {
        Error(next1, "Expected string literal or variable after '+'");
      }
    }

    // check if next token is '=' operator
    // if it is, chain variable assignment
    if (lexer.Any() && lexer.Peek() == Lexer::ID_ASSIGN) {
      Token middle = next;
      next = lexer.Use(); // consume '='

      if (!lexer.Any()) Error(current, "Expected value after '='");
      Token next2 = lexer.Use();
      if (next2 == Lexer::ID_ID || next2 == Lexer::ID_LIT_STRING) {
        // handle chaining logic
        //var_token, middle, next2
        if (middle == Lexer::ID_ID) {
          current_scope[middle.lexeme] = TokenToString(next2);
          result = TokenToString(next2);
        }
      }
    }

    current_scope[var_name] = result;
  }

  void ProcessID(const Token & token) {
    // check if id is in the symbol_table
    // if not, throw an error
    bool reverse = false;
    std::string name = token.lexeme;
    bool found = false;
    for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
      if (scope_it->find(name) != scope_it->end()) {
        found = true;
        break;
      }
    }
    if (!found) {
      Error(token, "Assignment to undeclared variable '", name, "'");
    }

    if (!lexer.Any() || lexer.Peek() != Lexer::ID_ASSIGN) {
      Error(token, "Expected '=' after variable name");
    }
    lexer.Use();

    // if it is:
      // check if it is a simple x = y, or a more complex expression(like in CompleteCalculation)
      // handle reassignment accordingly
    if (!lexer.Any()) {
      Error(token, "Expected expression after '='");
    }
    Token first = lexer.Use();
    if (first.id == Lexer::ID_NOT) {
      reverse = true;
      if (!lexer.Any()) {
        Error(token, "Expected expression after '!'");
      }
      first = lexer.Use();
    }

    std::string value;
    if (first == Lexer::ID_ID || first == Lexer::ID_LIT_STRING) {
      if (lexer.Any() && (
        lexer.Peek() == Lexer::ID_PLUS || lexer.Peek() == Lexer::ID_MINUS ||
        lexer.Peek() == Lexer::ID_SLASH || lexer.Peek() == Lexer::ID_PERCENT)) {

        value = CompleteCalculation(first);
      } else {
        value = TokenToString(first);
      }
    } else {
      Error(first, "Expected identifier or string literal after '='");
    }

    if (reverse) {
      if (value == "") {
        value = " ";
      }
      else {
        value = "";
      }
    }
    for (auto scope_it = symbol_stack.rbegin(); scope_it != symbol_stack.rend(); ++scope_it) {
      if (scope_it->find(name) != scope_it->end()) {
        (*scope_it)[name] = value;
        break;
      }
    }
  }

  void ProcessLBRACE() {
    symbol_stack.push_back({});
  }

  void ProcessRBRACE(const Token & token) {
    if (symbol_stack.size() <= 1) {
      Error(token, "Extra '}' without matching '{'");
    }
    symbol_stack.pop_back();
  }
};

int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cout << "Format: " << argv[0] << " [filename]" << std::endl;
    exit(1);
  }

  StringStackPlusPlus prog(argv[1]);
  prog.Run();

  return 0;
}
