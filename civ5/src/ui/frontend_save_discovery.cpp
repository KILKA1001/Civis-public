#include "civ5/ui/frontend_save_discovery.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <system_error>
#include <utility>
#ifdef _WIN32
#include "civ5/platform/windows_path.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace civ5::ui {
namespace fs = std::filesystem;
namespace {
fs::path file_path(const fs::path &path) {
#ifdef _WIN32
    return platform::extended_windows_path(path);
#else
    return path;
#endif
}
fs::path canonical_path(const fs::path &path, std::error_code &error) {
    auto canonical = fs::canonical(file_path(path), error);
#ifdef _WIN32
    canonical = platform::unextended_windows_path(canonical);
#endif
    return canonical;
}
std::string path_text(const fs::path &path) {
    const auto text = path.generic_u8string();
    return {text.begin(), text.end()};
}
struct PathIdentityLess {
    bool operator()(const fs::path &a, const fs::path &b) const {
#ifdef _WIN32
        const auto order = CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE);
        if (order != 0) return order == CSTR_LESS_THAN;
#endif
        return a.native() < b.native();
    }
};
bool path_less(const fs::path &a, const fs::path &b) {
    return a.native() < b.native();
}
bool save_extension(const fs::path &path) {
    auto extension = path.extension().native();
    for (auto &value : extension)
        if (value >= 'A' && value <= 'Z') value += 'a' - 'A';
    return extension == fs::path(".civ5save").native() ||
           extension == fs::path(".civissave").native();
}
bool directory_link(const fs::path &path, fs::file_status status, std::error_code &error) {
    if (fs::is_symlink(status)) return true;
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(file_path(path).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return true;
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    (void)error;
    return false;
#endif
}
bool missing(const std::error_code &error, fs::file_status status) {
    return error == std::errc::no_such_file_or_directory ||
           (!error && status.type() == fs::file_type::not_found);
}
struct SaveMetadata {
    fs::path path;
    fs::file_time_type modified;
};
} // namespace

FrontendSaveDiscoveryResult discover_frontend_saves(
        const std::vector<fs::path> &roots, FrontendSaveDiscoveryOptions options) {
    FrontendSaveDiscoveryResult result;
    const auto report = [&](const fs::path &path, const char *operation, const std::error_code &error) {
        result.diagnostics.push_back(std::string(operation) + ": " + path_text(path) + ": " + error.message());
    };
    const auto usable_directory = [&](const fs::path &path, bool optional) {
        std::error_code error;
        const auto status = fs::symlink_status(file_path(path), error);
        if (optional && missing(error, status)) return false;
        if (error) {
            report(path, "Cannot inspect save directory", error);
            return false;
        }
        const auto linked = directory_link(path, status, error);
        if (error) {
            report(path, "Cannot inspect save directory attributes", error);
            return false;
        }
        if (linked) {
            result.diagnostics.push_back("Save directory link skipped: " + path_text(path));
            return false;
        }
        if (!fs::is_directory(status)) {
            result.diagnostics.push_back("Save root is not a directory: " + path_text(path));
            return false;
        }
        return true;
    };

    std::vector<fs::path> canonical_roots;
    for (const auto &root : roots) {
        if (root.empty()) {
            result.diagnostics.emplace_back("Empty save root skipped");
            continue;
        }
        if (!usable_directory(root, true)) continue;
        std::error_code error;
        auto canonical = canonical_path(root, error);
        if (error) {
            report(root, "Cannot resolve save directory", error);
            continue;
        }
        if (canonical == canonical.root_path()) {
            result.diagnostics.push_back("Filesystem root is not a save directory: " + path_text(canonical));
            continue;
        }
        canonical_roots.push_back(std::move(canonical));
    }
    std::sort(canonical_roots.begin(), canonical_roots.end(), path_less);
    std::map<fs::path, std::size_t, PathIdentityLess> pending;
    for (const auto &root : canonical_roots) pending.emplace(root, 0);
    std::set<fs::path, PathIdentityLess> visited_directories, visited_saves;
    std::vector<SaveMetadata> found;
    std::size_t entries_seen = 0;
    bool entry_limit = false;

    while (!pending.empty() && !entry_limit) {
        auto [directory, depth] = *pending.begin();
        pending.erase(pending.begin());
        if (!visited_directories.insert(directory).second || !usable_directory(directory, false)) continue;
        std::error_code error;
        fs::directory_iterator iterator(file_path(directory), fs::directory_options::none, error), end;
        if (error) {
            report(directory, "Cannot enumerate save directory", error);
            continue;
        }
        std::vector<fs::path> entries;
        while (iterator != end) {
            if (entries_seen == options.maximum_entries) {
                entry_limit = true;
                break;
            }
            ++entries_seen;
            entries.push_back(iterator->path());
            iterator.increment(error);
            if (error) {
                report(directory, "Cannot continue enumerating save directory", error);
                break;
            }
        }
        std::sort(entries.begin(), entries.end(), path_less);
        for (const auto &entry : entries) {
            error.clear();
            auto status = fs::symlink_status(file_path(entry), error);
            if (error) {
                report(entry, "Cannot inspect save entry", error);
                continue;
            }
            if (fs::is_symlink(status)) {
                // File aliases may identify an existing slot; directory aliases
                // are observed only to reject them, never enumerated.
                status = fs::status(file_path(entry), error);
                if (error) {
                    report(entry, "Cannot inspect save link target", error);
                    continue;
                }
                if (fs::is_directory(status)) {
                    result.diagnostics.push_back("Save directory link skipped: " + path_text(entry));
                    continue;
                }
            }
            if (fs::is_directory(status)) {
                const auto linked = directory_link(entry, status, error);
                if (error) report(entry, "Cannot inspect save directory attributes", error);
                else if (linked) result.diagnostics.push_back("Save directory link skipped: " + path_text(entry));
                else if (depth >= options.maximum_depth)
                    result.diagnostics.push_back("Save discovery depth limit reached: " + path_text(entry));
                else {
                    auto canonical = canonical_path(entry, error);
                    if (error) report(entry, "Cannot resolve save directory", error);
                    else {
                        const auto [position, inserted] = pending.emplace(std::move(canonical), depth + 1);
                        if (!inserted) position->second = std::min(position->second, depth + 1);
                    }
                }
                continue;
            }
            if (!fs::is_regular_file(status) || !save_extension(entry)) continue;
            auto canonical = canonical_path(entry, error);
            if (error) {
                report(entry, "Cannot resolve save file", error);
                continue;
            }
            if (visited_saves.contains(canonical)) continue;
            const auto modified = fs::last_write_time(file_path(canonical), error);
            if (error) {
                report(entry, "Cannot read save modification time", error);
                continue;
            }
            visited_saves.insert(canonical);
            found.push_back({std::move(canonical), modified});
        }
    }
    if (entry_limit) result.diagnostics.emplace_back("Save discovery entry limit reached; search is incomplete");
    std::sort(found.begin(), found.end(), [](const auto &a, const auto &b) {
        if (a.modified != b.modified) return a.modified > b.modified;
        return path_less(a.path, b.path);
    });
    for (auto &save : found) result.saves.push_back(std::move(save.path));
    std::sort(result.diagnostics.begin(), result.diagnostics.end());
    result.diagnostics.erase(std::unique(result.diagnostics.begin(), result.diagnostics.end()), result.diagnostics.end());
    return result;
}
} // namespace civ5::ui
