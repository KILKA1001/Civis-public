#include "civ5/ui/frontend_save_discovery.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using namespace civ5::ui;
namespace {
unsigned checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
void write(const fs::path &path, std::string_view text = "invalid save payload") {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
    if (!file) throw std::runtime_error("Cannot write save discovery fixture");
}
bool diagnostic(const FrontendSaveDiscoveryResult &result, std::string_view text) {
    return std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [&](const auto &line) {
        return line.find(text) != std::string::npos;
    });
}
bool contains(const FrontendSaveDiscoveryResult &result, const fs::path &path) {
    const auto canonical = fs::canonical(path);
    return std::find(result.saves.begin(), result.saves.end(), canonical) != result.saves.end();
}
void hierarchy_and_metadata(const fs::path &root) {
    const auto saves = root / "Saves";
    const auto old = saves / "single/auto/old.Civ5Save";
    const auto first = saves / "single/A.CivisSave";
    const auto second = saves / "single/B.cIv5SaVe";
    const auto newest = saves / "multi/newest.CIVISSAVE";
    for (const auto &path : {old, first, second, newest}) write(path);
    write(saves / "notes.txt");
    write(saves / "copied.Civ5Save.bak");
    write(saves / "partial.CivisSave.tmp");
    write(saves / "pretendCiv5Save");
    fs::create_directories(saves / "directory.Civ5Save");
    const auto time = fs::file_time_type::clock::now() - std::chrono::hours(48);
    fs::last_write_time(old, time);
    fs::last_write_time(first, time + std::chrono::hours(1));
    fs::last_write_time(second, time + std::chrono::hours(1));
    fs::last_write_time(newest, time + std::chrono::hours(2));
    const auto original_size = fs::file_size(first);
    const auto original_time = fs::last_write_time(first);
    const auto result = discover_frontend_saves({saves});
    need(result.diagnostics.empty(), "Ordinary save hierarchy produced diagnostics");
    need(result.saves == std::vector<fs::path>{fs::canonical(newest), fs::canonical(first),
             fs::canonical(second), fs::canonical(old)},
         "Save recursion, extension filtering, newest ordering or equal-time path tie failed");
    need(fs::file_size(first) == original_size && fs::last_write_time(first) == original_time,
         "Save discovery modified a save file");
    std::ifstream payload(first, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(payload)), {});
    need(text == "invalid save payload", "Save discovery changed payload bytes");
    need(contains(result, first), "Discovery tried to validate or decode a corrupt save payload");
    need(std::all_of(result.saves.begin(), result.saves.end(), [](const auto &path) { return path.is_absolute(); }),
         "Discovered paths are not absolute");
    const auto duplicates = discover_frontend_saves({saves / "single", saves / ".", saves, saves / "single/auto/.."});
    need(duplicates.saves == result.saves && duplicates.diagnostics.empty(),
         "Duplicate or overlapping roots duplicated canonical save paths");
    const auto reversed = discover_frontend_saves({saves, saves / "single/auto/..", saves / "single"});
    need(reversed.saves == result.saves, "Root order changed deterministic results");
    auto roots = std::vector<fs::path>{saves, root / "not-installed", saves / "notes.txt", {}};
    const auto errors = discover_frontend_saves(roots);
    need(errors.saves == result.saves && diagnostic(errors, "not a directory") && diagnostic(errors, "Empty save root"),
         "Invalid optional roots hid valid saves or lost diagnostics");
    const auto missing = discover_frontend_saves({root / "absent/a/b"});
    need(missing.saves.empty() && missing.diagnostics.empty(), "Missing optional roots were treated as errors");
    const auto empty = discover_frontend_saves({});
    need(empty.saves.empty() && empty.diagnostics.empty(), "Empty root list scanned a fallback location");
    const auto filesystem_root = discover_frontend_saves({fs::absolute(root).root_path()});
    need(filesystem_root.saves.empty() && diagnostic(filesystem_root, "Filesystem root"),
         "Save discovery accepted an entire filesystem as a search root");
}
void bounds(const fs::path &root) {
    const auto saves = root / "bounded";
    write(saves / "direct.Civ5Save");
    write(saves / "single/child.CivisSave");
    write(saves / "single/auto/deep.Civ5Save");
    FrontendSaveDiscoveryOptions options;
    options.maximum_depth = 0;
    const auto direct = discover_frontend_saves({saves}, options);
    need(direct.saves.size() == 1 && contains(direct, saves / "direct.Civ5Save") && diagnostic(direct, "depth limit"),
         "Depth zero traversed a child directory or hid a root file");
    options.maximum_depth = 1;
    const auto child = discover_frontend_saves({saves}, options);
    need(child.saves.size() == 2 && contains(child, saves / "single/child.CivisSave") && diagnostic(child, "depth limit"),
         "Configured depth limit was not enforced");
    const auto explicit_child = discover_frontend_saves({saves, saves / "single"}, options);
    need(explicit_child.saves.size() == 3 && !diagnostic(explicit_child, "depth limit"),
         "Overlapping explicit root did not start at depth zero");
    const auto budget = root / "entry-budget";
    for (unsigned i = 0; i != 8; ++i) write(budget / (std::to_string(i) + ".Civ5Save"));
    options = {};
    options.maximum_entries = 3;
    const auto limited = discover_frontend_saves({budget}, options);
    need(limited.saves.size() == 3 && diagnostic(limited, "entry limit"),
         "Aggregate entry limit did not bound inspection or report incomplete results");
    options.maximum_entries = 0;
    const auto zero = discover_frontend_saves({budget}, options);
    need(zero.saves.empty() && diagnostic(zero, "entry limit"), "Zero entry budget inspected files");
    options.maximum_entries = 1;
    const auto one = root / "one";
    write(one / "only.Civ5Save");
    const auto exact = discover_frontend_saves({one}, options);
    need(exact.saves.size() == 1 && exact.diagnostics.empty(), "Exact entry budget falsely reported truncation");
    const auto other = root / "other";
    write(other / "another.CivisSave");
    const auto aggregate = discover_frontend_saves({one, other}, options);
    need(aggregate.saves.size() == 1 && diagnostic(aggregate, "entry limit"),
         "Entry budget was reset between roots");
}
void links_and_cycles(const fs::path &root) {
    const auto saves = root / "links";
    const auto outside = root / "outside";
    write(saves / "own.Civ5Save");
    write(outside / "external.CivisSave");
    std::error_code error;
    fs::create_directory_symlink(outside, saves / "escape", error);
    if (error) {
        std::cout << "Directory symlink checks skipped: " << error.message() << '\n';
    } else {
        fs::create_directory_symlink(saves, saves / "cycle", error);
        if (error) throw fs::filesystem_error("Cannot create cycle fixture", saves / "cycle", error);
        const auto result = discover_frontend_saves({saves});
        need(result.saves.size() == 1 && contains(result, saves / "own.Civ5Save") && diagnostic(result, "directory link skipped"),
             "Directory link escaped the save root or a cycle was followed");
        const auto root_link = discover_frontend_saves({saves / "escape"});
        need(root_link.saves.empty() && diagnostic(root_link, "directory link skipped"),
             "Explicit directory symlink root was followed");
    }
    error.clear();
    fs::create_symlink(saves / "own.Civ5Save", saves / "alias.Civ5Save", error);
    if (error) {
        std::cout << "File alias checks skipped: " << error.message() << '\n';
    } else {
        const auto result = discover_frontend_saves({saves});
        need(result.saves.size() == 1 && contains(result, saves / "own.Civ5Save"),
             "File aliases were not deduplicated by canonical target path");
    }
#ifndef _WIN32
    error.clear();
    fs::create_symlink(saves / "missing.Civ5Save", saves / "dangling.Civ5Save", error);
    if (!error) {
        const auto result = discover_frontend_saves({saves});
        need(result.saves.size() == 1 && diagnostic(result, "Cannot inspect save link target"),
             "Unreadable/dangling entry lost its diagnostic or hid usable saves");
    }
#endif
}
} // namespace
int main() {
    const auto root = fs::current_path() / "fixtures" /
        ("frontend-save-discovery-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        fs::create_directories(root);
        hierarchy_and_metadata(root);
        bounds(root);
        links_and_cycles(root);
        fs::remove_all(root);
        std::cout << "Frontend save discovery checks passed: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        std::error_code cleanup_error;
        fs::remove_all(root, cleanup_error);
        return 1;
    }
}
