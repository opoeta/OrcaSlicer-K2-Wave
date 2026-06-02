#pragma once
#include "IUiBackend.hpp"
#include <optional>
#include <string>
#include <vector>

namespace Slic3r { namespace GUI { namespace Automation {

// A target specification. Resolution order: id -> path -> predicate
// (name OR class OR label OR value, all provided fields must match).
struct Target {
    std::optional<std::string> id;
    std::optional<std::string> path;
    std::optional<std::string> name;  // matches id OR label
    std::optional<std::string> klass;
    std::optional<std::string> label;
    std::optional<std::string> value;
    std::optional<BackendKind> backend;
    bool empty() const {
        return !id && !path && !name && !klass && !label && !value;
    }
};

// Depth-first flatten of a tree into stable-ordered pointers (parents before children).
std::vector<const UiNode*> flatten(const UiNode& root);

// All nodes matching the target spec (resolution-order aware).
std::vector<const UiNode*> find_matches(const UiNode& root, const Target& target);

enum class WaitState { Exists, Visible, Enabled, Value };

// True if `node` satisfies the wait condition. A null node only satisfies a
// negative... here we keep it simple: null => false for all states.
bool evaluate_state(const UiNode* node, WaitState state,
                    const std::optional<std::string>& expected_value);

}}} // namespace
