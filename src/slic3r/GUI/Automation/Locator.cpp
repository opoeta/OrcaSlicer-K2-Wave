#include "Locator.hpp"

namespace Slic3r { namespace GUI { namespace Automation {

static void flatten_into(const UiNode& n, std::vector<const UiNode*>& out) {
    out.push_back(&n);
    for (const UiNode& c : n.children)
        flatten_into(c, out);
}

std::vector<const UiNode*> flatten(const UiNode& root) {
    std::vector<const UiNode*> out;
    flatten_into(root, out);
    return out;
}

static bool matches_predicate(const UiNode& n, const Target& t) {
    if (t.backend && n.backend != *t.backend) return false;
    if (t.name && !(n.id == *t.name || n.label == *t.name)) return false;
    if (t.klass && n.klass != *t.klass) return false;
    if (t.label && n.label != *t.label) return false;
    if (t.value && !(n.has_value && n.value == *t.value)) return false;
    return true;
}

std::vector<const UiNode*> find_matches(const UiNode& root, const Target& target) {
    const auto all = flatten(root);
    std::vector<const UiNode*> out;

    // Resolution order: exact id -> exact path -> predicate.
    if (target.id) {
        for (const UiNode* n : all)
            if (n->id == *target.id) out.push_back(n);
        return out;
    }
    if (target.path) {
        for (const UiNode* n : all)
            if (n->path == *target.path) out.push_back(n);
        return out;
    }
    if (target.empty())
        return out; // nothing to match on
    for (const UiNode* n : all)
        if (matches_predicate(*n, target)) out.push_back(n);
    return out;
}

bool evaluate_state(const UiNode* node, WaitState state,
                    const std::optional<std::string>& expected_value) {
    if (node == nullptr)
        return false;
    switch (state) {
        case WaitState::Exists:  return true;
        case WaitState::Visible: return node->visible;
        case WaitState::Enabled: return node->enabled && node->visible;
        case WaitState::Value:
            return node->has_value && expected_value && node->value == *expected_value;
    }
    return false;
}

}}} // namespace
