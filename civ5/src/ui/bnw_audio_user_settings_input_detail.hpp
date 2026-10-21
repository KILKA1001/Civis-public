#pragma once
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <limits>
#include <filesystem>
#include <fstream>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui::audio_settings_detail {
inline void settings_need(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
// Imbue classic before opening: default global codecvt must never transform bytes.
inline std::vector<std::uint8_t> read_settings_raw(const std::filesystem::path& path) {
    settings_need(std::filesystem::is_regular_file(path), "Actual selected UserSettings file unavailable");
    const auto size = std::filesystem::file_size(path);
    settings_need(size <= 1024 * 1024, "BNW UserSettings byte budget exceeded");
    std::ifstream input;
    input.imbue(std::locale::classic());
    input.open(path, std::ios::binary);
    settings_need(static_cast<bool>(input), "Actual selected UserSettings stream unavailable");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    settings_need(input.gcount() == static_cast<std::streamsize>(bytes.size()) && input.peek() == std::char_traits<char>::eof(),
                  "Actual UserSettings stream changed during read");
    return bytes;
}
inline std::string trim_settings(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
    return std::string(value);
}
inline bool same_settings_name(const std::string& a, const std::string& b) {
#if defined(_WIN32)
    return _stricmp(a.c_str(), b.c_str()) == 0;
#else
    throw std::runtime_error("BNW native settings comparison requires Windows CRT");
#endif
}
// Native parser caches atoi(value). Admit representable ASCII decimal prefix;
// arbitrary nonnumeric prefixes yield0; overflow conversion is unsupported.
inline std::int32_t saved_settings_atoi_subset(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    bool negative = false;
    if (!value.empty() && (value.front() == '-' || value.front() == '+')) {
        negative = value.front() == '-'; value.remove_prefix(1);
    }
    std::uint64_t magnitude = 0;
    const std::uint64_t limit = negative ? 2147483648ULL : 2147483647ULL;
    for (const auto ch : value) {
        if (ch < '0' || ch > '9') break;
        const auto digit = static_cast<unsigned>(ch - '0');
        settings_need(magnitude <= (limit - digit) / 10, "BNW settings integer overflow branch unsupported");
        magnitude = magnitude * 10 + digit;
    }
    return static_cast<std::int32_t>(negative ? -static_cast<std::int64_t>(magnitude)
                                            : static_cast<std::int64_t>(magnitude));
}
// Strict source grammar subset: ASCII, conventional single section headers,
// one key per section. Native duplicate/loose-header branches are refused.
inline std::array<std::int32_t, 4> parse_audio_settings_existing_subset(std::string_view bytes) {
    settings_need(bytes.size() <= 1024 * 1024, "BNW UserSettings byte budget exceeded");
    for (const auto ch : bytes) {
        const auto c = static_cast<unsigned char>(ch);
        settings_need((c >= 32 && c < 127) || c == '\r' || c == '\n' || c == '\t',
                      "BNW UserSettings non-ASCII/control parser branch unsupported");
    }
    struct Section { std::string name; std::vector<std::pair<std::string, std::string>> entries; };
    std::vector<Section> sections;
    Section* current = nullptr;
    while (!bytes.empty()) {
        const auto end = bytes.find('\n');
        const auto raw = bytes.substr(0, end);
        settings_need(raw.size() < 1024, "BNW UserSettings long line branch unsupported");
        const auto carriage = raw.find('\r');
        settings_need(carriage == std::string_view::npos || carriage == raw.size() - 1,
                      "BNW UserSettings internal carriage-return branch unsupported");
        auto line = trim_settings(raw);
        if (end == std::string_view::npos) bytes = {}; else bytes.remove_prefix(end + 1);
        if (line.empty() || line.front() == ';') continue;
        if (line.front() == '[') {
            settings_need(line.size() >= 3 && line.back() == ']' && line.find(']', 1) == line.size() - 1,
                          "BNW UserSettings loose section header unsupported");
            const std::string name(std::string_view(line).substr(1, line.size() - 2));
            settings_need(!name.empty() && name.find('[') == std::string::npos,
                          "BNW UserSettings malformed section unsupported");
            for (const auto& s : sections) settings_need(!same_settings_name(s.name, name),
                "BNW UserSettings duplicate section branch unsupported");
            sections.push_back({name, {}}); current = &sections.back(); continue;
        }
        const auto equal = line.find('=');
        if (!current || equal == std::string::npos) continue;
        const auto key = trim_settings(std::string_view(line).substr(0, equal));
        const auto value = trim_settings(std::string_view(line).substr(equal + 1));
        settings_need(!key.empty(), "BNW UserSettings empty key branch unsupported");
        for (const auto& row : current->entries) settings_need(!same_settings_name(row.first, key),
            "BNW UserSettings duplicate key branch unsupported");
        current->entries.emplace_back(key, value);
    }
    constexpr std::array<const char*, 4> keys{"Music.Volume", "Effects.Volume", "Ambience.Volume", "Speech.Volume"};
    const Section* user = nullptr;
    for (const auto& section : sections) if (same_settings_name(section.name, "User Settings")) user = &section;
    settings_need(user, "BNW UserSettings missing section requires native default insertion/save");
    std::array<std::int32_t, 4> result{};
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const std::string* selected = nullptr;
        for (const auto& row : user->entries) if (same_settings_name(row.first, keys[i])) selected = &row.second;
        settings_need(selected, "BNW UserSettings missing volume requires native default insertion/save");
        result[i] = saved_settings_atoi_subset(*selected);
    }
    return result;
}
} // namespace civ5::ui::audio_settings_detail
