// -- Some header files that are likely to be useful --
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "AST.hpp"          // Build file for Abstract Syntax Tree nodes
#include "helpers.hpp"         // A place to put useful helper functions.
#include "lexer.hpp"        // Auto-generate file from Emplex
#include "SymbolTable.hpp"  // Build file for your own Symbol Table

using emplex::Lexer;        // Simplify use of Lexer and Token types.
// using emplex::Token;

int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cout << "Format: " << argv[0] << " [filename]" << std::endl;
    exit(1);
  }

  // Run input file through a lexer to tokenize it.
  std::ifstream fs(argv[1]);
  Lexer lexer;
  lexer.Tokenize(fs);
  SymbolTable table;

  // Parse the tokens to generate an Abstract Syntax Tree (AST).
  // Run the AST to execute the program.
  return 0;
}
