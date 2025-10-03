#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdexcept>

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, std::string>> scopes;

public:
    SymbolTable() {
        // Start with global scope
        scopes.emplace_back();
    }

    // Enter a new scope
    void EnterScope() {
        scopes.emplace_back();
    }

    // Exit the current scope
    void ExitScope() {
        if (scopes.size() <= 1) {
            throw std::runtime_error("Attempted to pop global scope");
        }
        scopes.pop_back();
    }

    // Declare a new variable in the current scope
    void Declare(const std::string& name, const std::string& value) {
        auto& current = scopes.back();
        if (current.find(name) != current.end()) {
            throw std::runtime_error("Variable '" + name + "' already declared in this scope");
        }
        current[name] = value;
    }

    // Assign a value to a variable (searches up the scope chain)
    void Assign(const std::string& name, const std::string& value) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                (*it)[name] = value;
                return;
            }
        }
        throw std::runtime_error("Assignment to undeclared variable '" + name + "'");
    }

    // Get the value of a variable (searches up the scope chain)
    std::string Get(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        throw std::runtime_error("Unknown variable '" + name + "'");
    }

    // Check if a variable is declared (in any scope)
    bool IsDeclared(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) return true;
        }
        return false;
    }

    // Check if a variable is declared in the current scope only
    bool IsDeclaredInCurrentScope(const std::string& name) const {
        const auto& current = scopes.back();
        return current.find(name) != current.end();
    }
};

#endif // SYMBOL_TABLE_HPP
