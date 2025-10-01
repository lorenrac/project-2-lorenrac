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
  std::unordered_map<std::string, std::string> symbol_table;

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
    if (symbol_table.find(var_name) == symbol_table.end()) {
      Error(token, "Unknown variable '", var_name, "'");
    }
    return symbol_table[var_name];
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

  std::string CompleteCalculation(const Token & token) {
    if (!lexer.Any()) Error(token, "Expected value in calculation");

    // get first string (each string could be either a literal string or a variable)
    std::string result;
    //Token current = lexer.Use();
    if (token == Lexer::ID_ID || token == Lexer::ID_LIT_STRING) {
        result = TokenToString(token);
    } else {
        Error(token, "Expected string literal or variable at start of expression");
    }

    // while loop to check for additional operators and strings
    // perform each subsequent calculation as: new = new operatorx stringx
    while (lexer.Any() && (lexer.Peek() == Lexer::ID_PLUS || lexer.Peek() == Lexer::ID_MINUS 
      || lexer.Peek() == Lexer::ID_SLASH || lexer.Peek() == Lexer::ID_PERCENT)) {
        Token op = lexer.Use();

        if (!lexer.Any()) {
            Error(token, "Expected value after operator");
        }
        Token next = lexer.Use();
        std::string nextStr;
        if (next == Lexer::ID_ID || next == Lexer::ID_LIT_STRING) {
            nextStr = TokenToString(next);
        } else {
            Error(next, "Expected string literal or variable after operator");
        }

        switch(op.id) {
          case Lexer::ID_PLUS: {
            result += nextStr;
            break;
          }
          case Lexer::ID_MINUS: {
            size_t pos = result.find(nextStr);
            if (pos != std::string::npos) {
                result.erase(pos, nextStr.length());
            }
            break;
          }
          case Lexer::ID_SLASH: {
            size_t pos = result.find(nextStr);
            if (pos != std::string::npos) {
                result = result.substr(0, pos);
            }
            break;
          }
          case Lexer::ID_PERCENT: {
            size_t pos = result.find(nextStr);
            if (pos != std::string::npos) {
                result = result.substr(pos + nextStr.length());
            }
            break;
          }
          default: {
            break;
          }
        }
    }
    return result;
  }

  bool ParseExpression(const Token & token) {
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
    else if (op == Lexer::ID_QUESTION) valid = !rightValue.empty();
    else Error(op, "Unknown operator in expression");
  }

  if (notPresent) {
    valid = !valid;
  }
  return valid;
}


public:
  StringStackPlusPlus(std::string filename) : filename(filename) { }

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

      case Lexer::ID_LBRACE: break;
      case Lexer::ID_RBRACE: break;
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
      default:
        UnexpectedToken(token);
    }
  }


  void ProcessPRINT(const Token & token) {
    std::string out;
    if (!HasArg()) { out = StackPop(token); }
    else {
      Token next = lexer.Peek();
      if (next == Lexer::ID_ID || next == Lexer::ID_LIT_STRING) {
        lexer.Use();
        if (lexer.Any() && (lexer.Peek() == Lexer::ID_PLUS || lexer.Peek() == Lexer::ID_MINUS 
          || lexer.Peek() == Lexer::ID_SLASH || lexer.Peek() == Lexer::ID_PERCENT)) {
            out = CompleteCalculation(next);
        }
        else {
          out = TokenToString(next);
          while (HasArg()) {
            out += TokenToString(lexer.Use());
          }
        }
      }
      else {
            Error(next, "Unexpected token in PRINT statement");
      }
    }
    std::cout << out << std::endl;
  }

  void ProcessIF(const Token & token) {
    // Handle LParen
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_LPAREN) {
      Error(token, "Expected '(' after IF");
    }
    lexer.Use();

    bool condition = ParseExpression(token);
    lastIfCondition = condition;
    justProcessedIf = true;

    if (!lexer.Any() || lexer.Peek() != Lexer::ID_RPAREN) {
      Error(token, "Expected ')' after IF");
    }
    lexer.Use();

    if (lexer.Any() && lexer.Peek() == Lexer::ID_LBRACE) {
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
            brace_depth++;
          }
          else if (next == Lexer::ID_RBRACE) {
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
            brace_depth++;
          }
          else if (next == Lexer::ID_RBRACE) {
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

  void ProcessWHILE(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessVAR(const Token & token) {
    // TODO
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_ID) {
      Error(token, "Expected identifer after VAR");
    }
    Token var_token = lexer.Use();

    // store variable name
    std::string var_name = var_token.lexeme;

    // check redeclaration
    if (symbol_table.find(var_name) != symbol_table.end()) {
      Error(var_token, "Variable '", var_name, "' already declared");
    }

    // consume '=' operator
    if (!lexer.Any() || lexer.Peek() != Lexer::ID_ASSIGN) {
      Error(var_token, "Expected '=' after variable name");
    }
    lexer.Use();

    // store variable value (is Lexer::ID_LIT_STRING)
    std::string result;
    if (!lexer.Any()) Error(var_token, "Expected expression after '='");
    Token current = lexer.Use();
    if (current == Lexer::ID_ID || current == Lexer::ID_LIT_STRING) {
      result = TokenToString(current);
    } else {
      Error(current, "Expected string literal or variable in expression");
    }

    // check if next token is '+' operator
    // if is is, add token after '+' to the variable value
    while (lexer.Any() && lexer.Peek() == Lexer::ID_PLUS) {
      lexer.Use(); // consume '+'

      if (!lexer.Any()) Error(current, "Expected value after '+'");
      Token next = lexer.Use();
      if (next == Lexer::ID_ID || next == Lexer::ID_LIT_STRING) {
        result += TokenToString(next);
      } else {
        Error(next, "Expected string literal or variable after '+'");
      }
    }

    symbol_table[var_name] = result;
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
