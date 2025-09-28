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
    assert(lexer.Any()); // Make sure there's something to process.

    // Figure out which type of token we are working with.
    auto token = lexer.Use();
    switch (token) {
      case Lexer::ID_PRINT:  ProcessPRINT(token);  break;
      case Lexer::ID_IF:   ProcessIF(token);   break;
      case Lexer::ID_ELSE:    ProcessELSE(token);    break;
      case Lexer::ID_WHILE: ProcessWHILE(token); break;
      case Lexer::ID_VAR:  ProcessVAR(token);  break;

      case Lexer::ID_EQ:  ProcessEQ(token);  break;
      case Lexer::ID_NEQ:  ProcessNEQ(token);  break;
      case Lexer::ID_LE:  ProcessLE(token);  break;
      case Lexer::ID_GE:  ProcessGE(token);  break;
      case Lexer::ID_LT:  ProcessLT(token);  break;
      case Lexer::ID_GT:  ProcessGT(token);  break;
      case Lexer::ID_ASSIGN:  ProcessASSIGN(token);  break;
      case Lexer::ID_NOT:  ProcessNOT(token);  break;
      case Lexer::ID_QUESTION:  ProcessQUESTION(token);  break;
      case Lexer::ID_PLUS:  ProcessPLUS(token);  break;
      case Lexer::ID_MINUS:  ProcessMINUS(token);  break;
      case Lexer::ID_SLASH:  ProcessSLASH(token);  break;
      case Lexer::ID_PERCENT:  ProcessPERCENT(token);  break;
      case Lexer::ID_LPAREN:  ProcessLPAREN(token);  break;
      case Lexer::ID_RPAREN:  ProcessRPAREN(token);  break;
      case Lexer::ID_LBRACE:  ProcessLBRACE(token);  break;
      case Lexer::ID_RBRACE:  ProcessRBRACE(token);  break;

      case Lexer::ID_NEWLINE: return; // Empty line -- nothing to process.
      default:
        // If we made it here, this is not a valid line.
        Error(token, "Unknown command '", token.lexeme, "'");
    }

    // Make sure the line ends in a newline.
    if (lexer.Any()) {
      Token line_end = lexer.Use();
      if (line_end != Lexer::ID_NEWLINE) UnexpectedToken(line_end);
    }
  }

  void ProcessPRINT(const Token & token) {
    std::string out;
    if (!HasArg()) { out = StackPop(token); }
    else {
      while (HasArg()) out += TokenToString(lexer.Use());
    }
    std::cout << out << std::endl;
  }

  void ProcessIF(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessELSE(const Token & token) {
    // TODO   
    if (token) {
      return;
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
    if (token) {
      return;
    }
  }

  void ProcessEQ(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessNEQ(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessLE(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessGE(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessLT(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessGT(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessASSIGN(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessNOT(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessQUESTION(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessPLUS(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessMINUS(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessSLASH(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessPERCENT(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessLPAREN(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessRPAREN(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessLBRACE(const Token & token) {
    // TODO
    if (token) {
      return;
    }
  }

  void ProcessRBRACE(const Token & token) {
    // TODO
    if (token) {
      return;
    }
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
