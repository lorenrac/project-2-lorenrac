#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct VarData {
    std::string name;
    std::string value;
    int declared_line;
};

class SymbolTable {
private:
    std::vector<VarData> all_vars;

    std::vector<std::unordered_map<std::string, int>> scope_stack;

public:
    SymbolTable() {
        EnterScope();
    }

    void EnterScope() {
        scope_stack.push_back({});
    }

    void ExitScope(int line) {
        if (scope_stack.empty()) {
            // TODO: Throw error
        } else {
            scope_stack.pop_back();
        }
    }

    void DeclareVariable(const std::string& name, int line) {
        auto& current_scope = scope_stack.back();
        if (current_scope.count(name)) {
            int orig_line = all_vars[current_scope[name]].declared_line;
            // TODO: Throw error
        }

        VarData var;
        var.name = name;
        var.value = "";
        var.declared_line = line;

        int id = all_vars.size();
        all_vars.push_back(var);
        current_scope[name] = id;
    }

    bool VariableExists(const std::string& name) const {
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            if (it->count(name)) return true;
        }
        return false;
    }

    std::string GetValue(const std::string& name, int line) const {
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            const auto& scope = *it;
            if (scope.count(name)) {
                int id = scope.at(name);
                return all_vars[id].value;
            }
        }
        // TODO: Throw error
        return "";
    }

    void SetValue(const std::string& name, const std::string& value, int line) {
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto& scope = *it;
            if (scope.count(name)) {
                int id = scope.at(name);
                all_vars[id].value = value;
                return;
            }
        }
        // TODO: Throw error
    }
};
