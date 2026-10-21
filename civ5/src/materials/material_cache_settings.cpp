#include "civ5/materials/material_cache_settings.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
inline bool space(unsigned char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
inline std::string_view trim(std::string_view value) {
    while (!value.empty() && space(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    while (!value.empty() && space(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);
    return value;
}
inline unsigned char lower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
}
inline bool equal(std::string_view left, std::string_view right) {
    left = left.substr(0, left.find('\0'));
    right = right.substr(0, right.find('\0'));
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0; i != left.size(); ++i)
        if (lower(static_cast<unsigned char>(left[i])) != lower(static_cast<unsigned char>(right[i])))
            return false;
    return true;
}
inline std::uint32_t integer(std::string_view value) {
    while (!value.empty() && space(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    bool negative = false;
    if (!value.empty() && (value.front() == '+' || value.front() == '-')) {
        negative = value.front() == '-';
        value.remove_prefix(1);
    }
    const std::uint32_t limit = negative ? 0x80000000u : 0x7fffffffu;
    std::uint32_t result = 0;
    for (const unsigned char c : value) {
        if (c < '0' || c > '9')
            break;
        const std::uint32_t digit = c - '0';
        if (result > (limit - digit) / 10u)
            return negative ? 0x80000000u : 0x7fffffffu;
        result = result * 10u + digit;
    }
    return negative ? 0u - result : result;
}
struct Entry {
    std::string comment, key, value;
    std::uint32_t integer = 0;
};
struct Section {
    std::string name;
    std::vector<Entry> entries;
};
struct Document {
    std::vector<Section> sections;
    static Document parse(std::string_view text) {
        std::vector<std::string> lines;
        while (!text.empty()) {
            const auto end = text.find('\n');
            lines.emplace_back(text.substr(0, end));
            if (end == std::string_view::npos)
                text = {};
            else
                text.remove_prefix(end + 1);
        }
        return from_lines(lines);
    }
    static Document from_lines(const std::vector<std::string> &lines) {
        Document result;
        std::string comment;
        for (const auto &raw_line : lines) {
            auto line = trim(raw_line);
            if (line.empty())
                continue;
            if (line.front() == ';') {
                if (!result.sections.empty())
                    comment = trim(line.substr(1));
                continue;
            }
            if (line.front() == '[' && line.back() == ']') {
                while (!line.empty() && line.front() == '[')
                    line.remove_prefix(1);
                while (!line.empty() && line.back() == ']')
                    line.remove_suffix(1);
                result.sections.push_back({std::string(line), {}});
                comment.clear();
                continue;
            }
            if (result.sections.empty())
                continue;
            const auto separator = line.find('=');
            const auto nul = line.find('\0');
            if (separator == std::string_view::npos || separator == 0 ||
                (nul != std::string_view::npos && nul < separator))
                continue;
            const auto key = trim(line.substr(0, separator));
            const auto value = trim(line.substr(separator + 1));
            result.sections.back().entries.push_back(
                {comment, std::string(key), std::string(value), integer(value)});
            comment.clear();
        }
        return result;
    }
    std::optional<std::size_t> section(std::string_view name) const {
        for (std::size_t i = 0; i != sections.size(); ++i)
            if (equal(sections[i].name, name))
                return i;
        return std::nullopt;
    }
    bool lookup(std::string_view section_name, std::string_view key, std::uint32_t &output) const {
        const auto index = section(section_name);
        if (!index)
            return false;
        for (const auto &entry : sections[*index].entries) {
            if (equal(entry.key, key)) {
                output = entry.integer;
                return true;
            }
        }
        return false;
    }
};
} // namespace

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <bit>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
struct FileDocument {
    bool wide = false;
    std::uint32_t bom_bytes = 0;
    std::string normalized_bytes;
    std::vector<std::string> decoded_lines;
    Document document;
};
inline std::string decode_line(std::string line) {
    if (line.empty() || static_cast<unsigned char>(line.back()) != 0x7f)
        return line;
    line.pop_back();
    std::uint32_t state = static_cast<std::uint32_t>(line.size()) + 1234u;
    constexpr double scale = std::bit_cast<double>(std::uint64_t{0x3df0000301109066});
    for (char &raw : line) {
        const auto value = static_cast<unsigned char>(raw);
        if (value < 32 || value > 126)
            continue;
        state = (state * 0x343fdu + 0x269ec3u) % 0xffffcfefu;
        // Original PC53 rounds each multiplication before FISTP truncation.
        volatile double fraction = static_cast<double>(state) * scale;
        volatile double offset = fraction * 95.0;
        auto decoded = static_cast<int>(value) - static_cast<int>(offset);
        if (decoded < 32)
            decoded += 95;
        raw = static_cast<char>(decoded);
    }
    return line;
}
inline void parse_lines(FileDocument &result, std::string_view text) {
    while (!text.empty()) {
        auto end = text.find('\n');
        const auto nul = text.find('\0');
        if (nul != std::string_view::npos && (end == std::string_view::npos || nul < end))
            end = std::string_view::npos;
        auto raw_line = text.substr(0, end);
        while (!raw_line.empty() && (raw_line.front() == '\r' || raw_line.front() == '\n'))
            raw_line.remove_prefix(1);
        while (!raw_line.empty() && (raw_line.back() == '\r' || raw_line.back() == '\n'))
            raw_line.remove_suffix(1);
        auto line = decode_line(std::string(raw_line));
        if (end == std::string_view::npos)
            text = {};
        else
            text.remove_prefix(end + 1);
        result.decoded_lines.push_back(line);
    }
    result.document = Document::from_lines(result.decoded_lines);
}
// Exact observed original flags0 file path. BOM selects one of two modes;
// every recognized BOM selects UTF16 code units in the fixed little-endian
// stream. This deliberately preserves observed behavior for other BOM forms.
inline FileDocument parse_file_bytes(std::string_view bytes) {
    FileDocument result;
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(bytes[i]); };
    if (bytes.size() >= 3 && byte(0) == 0xef && byte(1) == 0xbb && byte(2) == 0xbf)
        result.bom_bytes = 3;
    else if (bytes.size() >= 2 &&
             ((byte(0) == 0xff && byte(1) == 0xfe) || (byte(0) == 0xfe && byte(1) == 0xff)))
        result.bom_bytes = 2;
    else if (bytes.size() >= 4 && byte(0) == 0 && byte(1) == 0 && byte(2) == 0xfe && byte(3) == 0xff)
        result.bom_bytes = 4;
    result.wide = result.bom_bytes != 0;
    bytes.remove_prefix(result.bom_bytes);
    if (!result.wide) {
        for (char c : bytes)
            if (c != '\r')
                result.normalized_bytes.push_back(c);
        parse_lines(result, result.normalized_bytes);
        return result;
    }
#ifdef _WIN32
    static_assert(sizeof(wchar_t) == 2);
#endif
    std::wstring wide;
    for (std::size_t i = 0; i + 1 < bytes.size(); i += 2) {
        const auto low = static_cast<unsigned char>(bytes[i]);
        const auto high = static_cast<unsigned char>(bytes[i + 1]);
        const auto unit = static_cast<std::uint16_t>(low | (high << 8));
        if (unit == L'\r')
            continue;
        wide.push_back(static_cast<wchar_t>(unit));
        result.normalized_bytes.push_back(bytes[i]);
        result.normalized_bytes.push_back(bytes[i + 1]);
    }
    std::wstring_view remaining = wide;
    while (!remaining.empty()) {
        auto end = remaining.find(L'\n');
        const auto nul = remaining.find(L'\0');
        if (nul != std::wstring_view::npos && (end == std::wstring_view::npos || nul < end))
            end = std::wstring_view::npos;
        auto raw_line = remaining.substr(0, end);
        while (!raw_line.empty() && (raw_line.front() == L'\r' || raw_line.front() == L'\n'))
            raw_line.remove_prefix(1);
        while (!raw_line.empty() && (raw_line.back() == L'\r' || raw_line.back() == L'\n'))
            raw_line.remove_suffix(1);
        const std::wstring line(raw_line);
        if (end == std::wstring_view::npos)
            remaining = {};
        else
            remaining.remove_prefix(end + 1);
        std::string narrow;
        if (!line.empty()) {
#ifdef _WIN32
            const auto length =
                WideCharToMultiByte(CP_ACP, 0, line.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (!length)
                throw std::runtime_error("original CP_ACP conversion failed");
            if (length > 1) {
                std::string converted(static_cast<std::size_t>(length), '\0');
                if (WideCharToMultiByte(CP_ACP, 0, line.c_str(), -1, converted.data(), length, nullptr,
                                        nullptr) != length)
                    throw std::runtime_error("original CP_ACP conversion changed");
                converted.pop_back();
                narrow = std::move(converted);
            }
#else
            throw std::runtime_error("Original wide INI conversion requires a Windows ACP");
#endif
        }
        result.decoded_lines.push_back(decode_line(std::move(narrow)));
    }
    result.document = Document::from_lines(result.decoded_lines);
    return result;
}
} // namespace

namespace civ5::materials {
MaterialCacheSettingsOverrides parse_material_cache_ini(std::string_view bytes) {
    const auto document = parse_file_bytes(bytes).document;
    MaterialCacheSettingsOverrides result;
    const auto lookup = [&](const char *key, std::optional<std::uint32_t> &value) {
        std::uint32_t word = 0;
        if (document.lookup("GraphicsDetailSettings", key, word))
            value = word;
    };
    lookup("TerrainPageinSpeedMoving", result.moving_budget);
    lookup("TerrainPageinSpeedStill", result.still_budget);
    lookup("MinimizeGrayTiles", result.minimize_gray_tiles);
    lookup("BlockOnLoad", result.block_on_load);
    lookup("TerrainDetailLevel", result.detail);
    lookup("TextureQuality", result.texture_quality);
    lookup("TerrainTessLevel", result.tessellation);
    lookup("BicubicTerrainTessellation", result.derived_tessellation);
    lookup("Terrain64Chunks", result.pool_counts[0]);
    lookup("Terrain128Chunks", result.pool_counts[1]);
    lookup("Terrain256Chunks", result.pool_counts[2]);
    lookup("Terrain512Chunks", result.pool_counts[3]);
    return result;
}
MaterialCacheIniLoad load_material_cache_ini(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return {};
    const std::string bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad())
        throw std::runtime_error("Cannot read material INI file");
    return {true, parse_material_cache_ini(bytes)};
}
MaterialLoadSettings parse_material_load_ini(std::string_view bytes) {
    const auto document = parse_file_bytes(bytes).document;
    std::uint32_t minimal = 1;
    document.lookup("GameSettings", "MinimalTerrainBuildOnLoad", minimal);
    return {minimal != 0};
}
MaterialLoadIniResult load_material_load_ini(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return {};
    const std::string bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad())
        throw std::runtime_error("Cannot read material load INI file");
    return {true, parse_material_load_ini(bytes)};
}
bool material_cache_initial_cull(const MaterialLoadSettings &settings, bool renderer_present,
                                 std::uint32_t renderer_mode) {
    return settings.minimal_build_on_load && renderer_present && renderer_mode != 2;
}
} // namespace civ5::materials
