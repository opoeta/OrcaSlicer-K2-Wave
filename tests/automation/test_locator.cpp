#include <catch2/catch_all.hpp>
#include "slic3r/GUI/Automation/Locator.hpp"

using namespace Slic3r::GUI::Automation;

namespace {
UiNode make_tree() {
    UiNode root;
    root.klass = "MainFrame";
    root.path  = "MainFrame";

    UiNode panel;
    panel.klass = "Panel";
    panel.path  = "MainFrame/Panel[0]";

    UiNode slice;
    slice.id    = "btn_slice";
    slice.klass = "Button";
    slice.label = "Slice plate";
    slice.path  = "MainFrame/Panel[0]/Button[0]";

    UiNode export_btn;
    export_btn.id    = "btn_export";
    export_btn.klass = "Button";
    export_btn.label = "Export";
    export_btn.path  = "MainFrame/Panel[0]/Button[1]";

    UiNode dup;            // duplicate label, used for ambiguity tests
    dup.klass = "Button";
    dup.label = "Export";
    dup.path  = "MainFrame/Panel[0]/Button[2]";

    panel.children = {slice, export_btn, dup};
    root.children  = {panel};
    return root;
}
} // namespace

TEST_CASE("flatten yields parents before children", "[automation][locator]") {
    const auto tree = make_tree();
    const auto all  = flatten(tree);
    REQUIRE(all.size() == 5);
    CHECK(all.front()->klass == "MainFrame");
}

TEST_CASE("find_matches by exact id returns one", "[automation][locator]") {
    const auto tree = make_tree();
    Target t; t.id = "btn_slice";
    const auto m = find_matches(tree, t);
    REQUIRE(m.size() == 1);
    CHECK(m[0]->label == "Slice plate");
}

TEST_CASE("find_matches by exact path returns one", "[automation][locator]") {
    const auto tree = make_tree();
    Target t; t.path = "MainFrame/Panel[0]/Button[1]";
    const auto m = find_matches(tree, t);
    REQUIRE(m.size() == 1);
    CHECK(m[0]->id == "btn_export");
}

TEST_CASE("find_matches by predicate (label) can be ambiguous",
          "[automation][locator]") {
    const auto tree = make_tree();
    Target t; t.label = "Export";
    const auto m = find_matches(tree, t);
    CHECK(m.size() == 2); // btn_export + the duplicate
}

TEST_CASE("find_matches predicate combines fields (AND)",
          "[automation][locator]") {
    const auto tree = make_tree();
    Target t; t.label = "Export"; t.klass = "Button"; t.id = std::nullopt;
    // id/path absent -> predicate mode. Both fields must match.
    t.id = std::nullopt;
    const auto m = find_matches(tree, t);
    CHECK(m.size() == 2);
}

TEST_CASE("find_matches by name matches id OR label", "[automation][locator]") {
    const auto tree = make_tree();
    Target byId; byId.name = "btn_slice";
    CHECK(find_matches(tree, byId).size() == 1);
    Target byLabel; byLabel.name = "Slice plate";
    CHECK(find_matches(tree, byLabel).size() == 1);
}

TEST_CASE("find_matches not found returns empty", "[automation][locator]") {
    const auto tree = make_tree();
    Target t; t.id = "nope";
    CHECK(find_matches(tree, t).empty());
}
