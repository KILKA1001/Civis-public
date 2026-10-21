#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "bnw_native_fresh_find.hpp"

// Private package implementation; no resource owner or public issuer API.
namespace civ5::ui::native_loose_detail {
inline void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
inline std::wstring acp_wide(const std::string& bytes) {
    const int size = MultiByteToWideChar(CP_ACP, 0, bytes.c_str(), -1, nullptr, 0);
    need(size > 0, "Native loose ANSI path conversion failed");
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    need(MultiByteToWideChar(CP_ACP, 0, bytes.c_str(), -1, wide.data(), size) == size,
         "Native loose ANSI path conversion changed");
    wide.pop_back(); return wide;
}
inline std::string acp_path(const std::filesystem::path& path) {
    const auto& wide = path.native();
    const int size = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    need(size > 0, "Native loose source root ANSI conversion failed");
    std::string bytes(static_cast<std::size_t>(size), '\0');
    need(WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, bytes.data(), size, nullptr, nullptr) == size,
         "Native loose source root conversion changed");
    bytes.pop_back();
    need(acp_wide(bytes) == wide, "Native loose source root cannot round-trip current CP_ACP");
    return bytes;
}
struct LooseEntry {
    std::filesystem::path relative;
    std::string native_name;
    DWORD attributes;
    std::uint64_t bytes, written;
    // Exact FindA-derived relative bytes authenticated alongside relative at capture.
    std::string raw_relative;
    bool operator==(const LooseEntry&) const = default;
};
struct LooseWalk {
    std::filesystem::path root;
    bool base_filters = false, exists = false;
    std::vector<LooseEntry> entries;
    UINT code_page = 0;
    bool original_star_dot_star = false;
    bool operator==(const LooseWalk&) const = default;
};
// Optional private diagnosis only: no alternate comparison, retry or authority.
// The first differing record is retained while the SAME traversal continues.
struct LooseWalkDifference {
    enum class Kind { none, record, extra_record, missing_records, root_state };
    struct Observed { std::string raw_relative, native_name; DWORD attributes;
        std::uint64_t bytes, written; };
    Kind kind = Kind::none;
    std::size_t index = 0, observed_count = 0;
    // raw path, name, attributes, bytes, written respectively.
    unsigned fields = 0;
    bool observed_root_exists = false;
    std::optional<LooseEntry> expected;
    std::optional<Observed> observed;
};
struct NativeEnumeration {
    struct Cursor {
        WIN32_FIND_DATAA data{};
        HANDLE h = INVALID_HANDLE_VALUE;
        explicit Cursor(const std::string& search) : h(native_fresh_find_detail::first(search.c_str(), &data)) {}
        Cursor(const Cursor&) = delete;
        ~Cursor() { if (h != INVALID_HANDLE_VALUE) FindClose(h); }
        bool valid() const { return h != INVALID_HANDLE_VALUE; }
        bool next() { return FindNextFileA(h, &data) != 0; }
        DWORD error() const { return GetLastError(); }
    };
    Cursor open(const std::string& search) { return Cursor(search); }
};
// Capture and revalidation consume the SAME traversal. The private enumeration
// parameter lets regression fixtures force native order and enumeration errors;
// production always uses NativeEnumeration and its real FindFirst/NextFileA.
// The filename view is borrowed only during emit; FindNext may overwrite it.
// Keep directory paths owning BEFORE emit, just as the capture traversal did.
// The comparator consumes parent/name segments without constructing file paths.
template<class Enumeration, class Emit, class Folder>
void visit_loose_entry_segments_with_folder(const std::string& root_bytes, bool base_filters,
                          Enumeration& enumeration, Emit&& emit, Folder&& enter_folder,const char* search_suffix="*") {
    const auto visit = [&](auto&& self, const std::string& relative) -> void {
        auto folded = runtime::ascii_lower(relative);
        std::replace(folded.begin(), folded.end(), '/', '\\');
        folded = "\\" + folded + "\\";
        if (base_filters && (folded.find("\\dlc\\") != std::string::npos ||
            folded.find("\\scenarios\\") != std::string::npos ||
            folded.find("\\sounds\\speech\\") != std::string::npos)) return;
        enter_folder(relative); //After actual exclusions, BEFORE this folder's FindA.
        const auto search = root_bytes + "\\" + relative + (relative.empty() ? "" : "\\") + search_suffix;
        auto find = enumeration.open(search);
        need(find.valid(), "Native loose directory enumeration failed");
        do {
            const auto& data = find.data;
            const std::string_view name = data.cFileName;
            const auto attrs = data.dwFileAttributes;
            if (name == "." || name == ".." || (attrs & 6) != 0) continue;
            need(std::all_of(name.begin(), name.end(), [](unsigned char c) { return c < 128; }),
                 "Native loose non-ASCII record bytes require CRT/CRC source owner");
            need((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0,
                 "Native loose reparse source requires target owner");
            const bool directory = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
            const std::string child = !directory ? std::string{} :
                relative.empty() ? std::string(name) : relative + "\\" + std::string(name);
            const auto bytes = (std::uint64_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
            const auto written = (std::uint64_t(data.ftLastWriteTime.dwHighDateTime) << 32) |
                                 data.ftLastWriteTime.dwLowDateTime;
            emit(relative, name, attrs, bytes, written, child);
            if (directory) self(self, child);
        } while (find.next());
        need(find.error() == ERROR_NO_MORE_FILES, "Native loose traversal incomplete");
    };
    visit(visit, {});
}
template<class Enumeration, class Emit, class Folder>
void visit_loose_entries_with_folder(const std::string& root_bytes, bool base_filters,
                          Enumeration& enumeration, Emit&& emit,Folder&& enter_folder,const char* search_suffix="*") {
    visit_loose_entry_segments_with_folder(root_bytes,base_filters,enumeration,
        [&](const std::string& parent,std::string_view observed_name,DWORD attrs,
            std::uint64_t bytes,std::uint64_t written,const std::string& child) {
            // Existing capture/admission callbacks still receive owning strings.
            const std::string name(observed_name);
            if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) emit(child,name,attrs,bytes,written);
            else {
                const auto path=parent.empty()?name:parent+"\\"+name;
                emit(path,name,attrs,bytes,written);
            }
        },std::forward<Folder>(enter_folder),search_suffix);
}
// Compare the literal concatenation, without normalization or basename-only
// equivalence. The length guard also prevents indexing a short forged prefix.
inline bool raw_relative_equals_segments(std::string_view raw,std::string_view parent,
                                        std::string_view name) noexcept {
    if(parent.empty())return raw==name;
    return raw.size()>parent.size()&&raw.starts_with(parent)&&raw[parent.size()]=='\\'&&
           raw.substr(parent.size()+1)==name;
}
template<class Enumeration,class Emit>
void visit_loose_entries(const std::string& root_bytes,bool base_filters,
    Enumeration& enumeration,Emit&& emit) {
    visit_loose_entries_with_folder(root_bytes,base_filters,enumeration,
        std::forward<Emit>(emit),[](const std::string&){});
}
template<class Enumeration>
std::vector<LooseEntry> capture_loose_entries(const std::string& root_bytes, bool base_filters,
                                            Enumeration& enumeration) {
    std::vector<LooseEntry> entries;
    visit_loose_entries(root_bytes, base_filters, enumeration,
        [&](const std::string& path, const std::string& name, DWORD attrs,
            std::uint64_t bytes, std::uint64_t written) {
            entries.push_back({std::filesystem::path(acp_wide(path)), name, attrs, bytes, written, path});
        });
    return entries;
}
inline LooseWalk loose_walk(const std::filesystem::path& root, bool base_filters) {
    LooseWalk out{root, base_filters, std::filesystem::exists(root), {}, GetACP()};
    if (!out.exists) return out;
    need(std::filesystem::is_directory(root), "Native loose source is not a directory");
    const auto root_bytes = acp_path(root);
    NativeEnumeration enumeration;
    out.entries = capture_loose_entries(root_bytes, base_filters, enumeration);
    need(GetACP() == out.code_page, "Native loose CP_ACP changed");
    return out;
}
template<class Enumeration>
bool compare_loose_entries(const LooseWalk& expected, const std::string& root_bytes,
                          Enumeration& enumeration, LooseWalkDifference* difference = nullptr) {
    if (difference) *difference = {};
    std::size_t index = 0;
    bool matched = true;
    visit_loose_entry_segments_with_folder(root_bytes, expected.base_filters, enumeration,
        [&](const std::string& parent, std::string_view name, DWORD attrs,
            std::uint64_t bytes, std::uint64_t written, const std::string& child) {
            const auto owned_observation=[&] {
                // Only the optional first-difference record owns file strings;
                // no view escapes the cursor or survives its next()/destructor.
                const auto path=(attrs & FILE_ATTRIBUTE_DIRECTORY) != 0 ? child :
                    parent.empty()?std::string(name):parent+"\\"+std::string(name);
                return LooseWalkDifference::Observed{path,std::string(name),attrs,bytes,written};
            };
            if (index >= expected.entries.size()) {
                matched = false;
                if (difference && difference->kind == LooseWalkDifference::Kind::none) {
                    difference->kind = LooseWalkDifference::Kind::extra_record;
                    difference->index = index;
                    difference->observed = owned_observation();
                }
            } else {
                const auto& entry = expected.entries[index];
                const bool same_path=raw_relative_equals_segments(entry.raw_relative,parent,name);
                const bool same_name=std::string_view(entry.native_name)==name;
                if (!same_path || !same_name || attrs != entry.attributes ||
                    bytes != entry.bytes || written != entry.written) {
                    matched = false;
                    if (difference && difference->kind == LooseWalkDifference::Kind::none) {
                        difference->kind = LooseWalkDifference::Kind::record;
                        difference->index = index; difference->expected = entry;
                        difference->observed = owned_observation();
                        difference->fields = (!same_path ? 1u : 0u) |
                            (!same_name ? 2u : 0u) | (attrs != entry.attributes ? 4u : 0u) |
                            (bytes != entry.bytes ? 8u : 0u) | (written != entry.written ? 16u : 0u);
                    }
                }
            }
            ++index;
        },[](const std::string&){},expected.original_star_dot_star?"*.*":"*");
    // Do not stop on mismatch: later ASCII/reparse/FindNext failures must still
    // reject exactly as the old full-vector walk did before equality comparison.
    if (difference) {
        difference->observed_count = index;
        if (index < expected.entries.size() && difference->kind == LooseWalkDifference::Kind::none) {
            difference->kind = LooseWalkDifference::Kind::missing_records;
            difference->index = index; difference->expected = expected.entries[index];
        }
    }
    return matched && index == expected.entries.size();
}
inline bool loose_walk_matches(const LooseWalk& expected, LooseWalkDifference* difference = nullptr) {
    if (difference) *difference = {};
    need(GetACP() == expected.code_page, "Native loose CP_ACP changed");
    const bool exists = std::filesystem::exists(expected.root);
    if (!exists) {
        if (difference && (expected.exists || !expected.entries.empty()))
            difference->kind = LooseWalkDifference::Kind::root_state;
        return !expected.exists && expected.entries.empty();
    }
    need(std::filesystem::is_directory(expected.root), "Native loose source is not a directory");
    // Keep actual root conversion/round-trip validation, including non-ASCII root.
    const auto root_bytes = acp_path(expected.root);
    NativeEnumeration enumeration;
    const bool matched = compare_loose_entries(expected, root_bytes, enumeration, difference);
    if (difference) {
        difference->observed_root_exists = true;
        if (!expected.exists && difference->kind == LooseWalkDifference::Kind::none)
            difference->kind = LooseWalkDifference::Kind::root_state;
    }
    need(GetACP() == expected.code_page, "Native loose CP_ACP changed");
    // Same raw relative bytes + same ACP refer to the wide path converted at
    // capture. A changed byte sequence is rejected, never cast or reinterpreted.
    return expected.exists && matched;
}
} // namespace civ5::ui::native_loose_detail
