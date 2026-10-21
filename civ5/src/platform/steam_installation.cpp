#include "civ5/platform/steam_installation.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/binary_reader.hpp"
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <shlobj.h>
#include <stdexcept>

namespace civ5::platform {
namespace fs = std::filesystem;
namespace {
struct Vdf {
    bool object = false;
    std::string text;
    std::map<std::string, Vdf> fields;
};
class VdfReader {
    std::string text_;
    std::size_t at_ = 0;
    void space() {
        while (at_ < text_.size()) {
            if (text_[at_] == ' ' || text_[at_] == '\n' || text_[at_] == '\r' || text_[at_] == '\t') {
                ++at_;
                continue;
            }
            if (text_.compare(at_, 2, "//") == 0) {
                at_ = text_.find('\n', at_);
                if (at_ == std::string::npos)
                    at_ = text_.size();
                continue;
            }
            break;
        }
    }
    std::string token() {
        space();
        std::string result;
        if (at_ == text_.size())
            throw std::runtime_error("Truncated VDF");
        if (text_[at_++] != '"')
            throw std::runtime_error("Expected quoted VDF token");
        while (at_ < text_.size()) {
            char c = text_[at_++];
            if (c == '"')
                return result;
            if (c == '\\') {
                if (at_ == text_.size())
                    throw std::runtime_error("Truncated VDF escape");
                c = text_[at_++];
                if (c != '\\' && c != '"')
                    throw std::runtime_error("Unsupported VDF escape");
            }
            if (c == '\0')
                throw std::runtime_error("NUL in VDF");
            result += c;
        }
        throw std::runtime_error("Unclosed VDF token");
    }
    Vdf object(unsigned depth, bool closing) {
        if (depth > 64)
            throw std::runtime_error("VDF nesting exceeds resource budget");
        Vdf result;
        result.object = true;
        for (;;) {
            space();
            if (at_ == text_.size()) {
                if (closing)
                    throw std::runtime_error("Unclosed VDF object");
                return result;
            }
            if (text_[at_] == '}') {
                if (!closing)
                    throw std::runtime_error("Unexpected VDF brace");
                ++at_;
                return result;
            }
            const auto key = token();
            space();
            Vdf value;
            if (at_ < text_.size() && text_[at_] == '{') {
                ++at_;
                value = object(depth + 1, true);
            } else
                value.text = token();
            if (!result.fields.emplace(key, std::move(value)).second)
                throw std::runtime_error("Duplicate VDF key: " + key);
        }
    }

  public:
    explicit VdfReader(const fs::path &path) {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("Cannot read VDF: " + assets::path_utf8(path));
        const auto size = fs::file_size(path);
        if (size > 16 * 1024 * 1024)
            throw std::runtime_error("VDF exceeds resource budget");
        text_.resize(static_cast<std::size_t>(size));
        stream.read(text_.data(), static_cast<std::streamsize>(text_.size()));
        if (!stream || !runtime::valid_utf8(text_))
            throw std::runtime_error("Invalid VDF text");
        if (text_.starts_with("\xef\xbb\xbf"))
            at_ = 3;
    }
    Vdf read() { return object(0, false); }
};
const Vdf &field(const Vdf &parent, const std::string &key, bool object = false) {
    const auto found = parent.fields.find(key);
    if (found == parent.fields.end() || found->second.object != object)
        throw std::runtime_error("Missing VDF field: " + key);
    return found->second;
}
} // namespace
fs::path default_steam_root() {
    const auto registered = [](HKEY hive, const wchar_t *name, REGSAM flags) -> fs::path {
        struct Key {
            HKEY handle = nullptr;
            ~Key() {
                if (handle)
                    RegCloseKey(handle);
            }
        } key;
        DWORD bytes = 0;
        if (RegOpenKeyExW(hive, L"Software\\Valve\\Steam", 0, KEY_QUERY_VALUE | flags, &key.handle) !=
            ERROR_SUCCESS)
            return {};
        if (RegGetValueW(key.handle, nullptr, name, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) != ERROR_SUCCESS)
            return {};
        std::wstring value(bytes / sizeof(wchar_t), L'\0');
        if (RegGetValueW(key.handle, nullptr, name, RRF_RT_REG_SZ, nullptr, value.data(), &bytes) !=
            ERROR_SUCCESS)
            return {};
        while (!value.empty() && value.back() == L'\0')
            value.pop_back();
        return fs::path(value);
    };
    std::vector<fs::path> candidates{registered(HKEY_CURRENT_USER, L"SteamPath", 0),
                                     registered(HKEY_LOCAL_MACHINE, L"InstallPath", KEY_WOW64_32KEY)};
    PWSTR program_files = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, nullptr, &program_files))) {
        candidates.push_back(fs::path(program_files) / "Steam");
        CoTaskMemFree(program_files);
    }
    for (const auto &candidate : candidates)
        if (!candidate.empty() && fs::is_regular_file(candidate / "steam.exe") &&
            fs::is_regular_file(candidate / "steamapps/libraryfolders.vdf"))
            return candidate;
    throw std::runtime_error("Steam path not found; supply --steam-root or --game-root");
}
void validate_game_root(const fs::path &root) {
    if (!fs::is_directory(root / "Assets") || !fs::is_regular_file(root / "CivilizationV.exe"))
        throw std::runtime_error("Expected an installed Civilization V game root: " +
                                 assets::path_utf8(root));
}
SteamDiscovery discover_civ5(const fs::path &steam_root) {
    SteamDiscovery result;
    try {
        const auto document = VdfReader(steam_root / "steamapps/libraryfolders.vdf").read();
        const auto &folders = field(document, "libraryfolders", true);
        std::set<fs::path> seen;
        for (const auto &[index, entry] : folders.fields) {
            if (index.empty() || index.find_first_not_of("0123456789") != std::string::npos)
                continue;
            try {
                const auto library_text = entry.object ? field(entry, "path").text : entry.text;
                const auto library = fs::path(std::u8string(library_text.begin(), library_text.end()));
                if (!library.is_absolute())
                    throw std::runtime_error("Relative Steam library root");
                const auto manifest_path = library / "steamapps/appmanifest_8930.acf";
                if (!fs::exists(manifest_path))
                    continue;
                const auto manifest = VdfReader(manifest_path).read();
                const auto &app = field(manifest, "AppState", true);
                if (field(app, "appid").text != "8930")
                    throw std::runtime_error("Wrong Steam application ID");
                const auto &directory_text = field(app, "installdir").text;
                const auto directory = fs::path(std::u8string(directory_text.begin(), directory_text.end()));
                if (directory.empty() || directory.has_parent_path() || directory.has_root_path() ||
                    directory == "." || directory == "..")
                    throw std::runtime_error("Unsafe Steam installdir");
                const auto game = fs::canonical(library / "steamapps/common" / directory);
                validate_game_root(game);
                if (seen.insert(game).second)
                    result.installations.push_back({fs::canonical(library), game,
                                                    library / "steamapps/workshop/content/8930",
                                                    field(app, "buildid").text});
            } catch (const std::exception &error) {
                result.diagnostics.push_back("Steam library " + index + ": " + error.what());
            }
        }
    } catch (const std::exception &error) {
        result.diagnostics.push_back(error.what());
    }
    return result;
}
} // namespace civ5::platform
