#pragma once

#include <iostream>
#include <string>

// Various helper functions can go here.

template <typename... Ts>
void Error(size_t line_id, Ts... message) {
  std::cerr << "ERROR (line " << line_id << "): ";
  (std::cerr << ... << std::forward<Ts>(message)) << std::endl;
  exit(1);
}

// Convert a bool value to a "" or "1"
std::string StringBool(bool in) { return in ? "1" : ""; }
