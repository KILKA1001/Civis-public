#include "civ5/terrain/coast_assets.hpp"
#include "../assets/attribute_xml.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace civ5::terrain {
namespace {
constexpr std::size_t text_limit = 8 * 1024 * 1024;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
std::string_view trim(std::string_view s) {
    while (!s.empty() && whitespace(s.front()))
        s.remove_prefix(1);
    while (!s.empty() && whitespace(s.back()))
        s.remove_suffix(1);
    return s;
}
template <class T> T number(std::string_view value) {
    value = trim(value);
    if (!value.empty() && value.front() == '+') {
        value.remove_prefix(1);
        require(!value.empty() && value.front() != '+' && value.front() != '-', "Invalid coast numeric sign");
    }
    require(!value.empty(), "Missing coast numeric value");
    T result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    require(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size(),
            "Invalid coast numeric value");
    if constexpr (std::is_floating_point_v<T>)
        require(std::isfinite(result), "Nonfinite coast numeric value");
    return result;
}
assets::AssetHandle exact_asset(const assets::AssetSource &source, const assets::AssetKey &key) {
    require(key.safe_relative_path(), "Unsafe coast resource key");
    auto lookup = source.find(key);
    auto *found = std::get_if<assets::AssetHandle>(&lookup);
    require(found != nullptr, "Coast resource is missing or ambiguous");
    return std::move(*found);
}
std::string_view as_text(const assets::Bytes &bytes) {
    return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}
} // namespace
CoastStyleDocument parse_coast_styles(std::string_view xml) {
    const auto root = assets::detail::AttributeXml(xml).document();
    require(root.name == "CoastRoot", "Wrong coast XML root");
    std::vector<std::string> characters;
    for (const auto &n : root.child("SplineCharacterList").children) {
        if (n.name != "SplineCharacter")
            continue;
        require(characters.size() < 4096, "Coast Character count exceeded");
        auto filename = n.attribute("Filename");
        require(!filename.empty() && filename.size() <= 1024, "Invalid coast Character filename");
        characters.push_back(std::move(filename));
    }
    std::vector<CoastProfileRecord> profiles;
    for (const auto &n : root.child("CoastStyleList").children) {
        if (n.name != "CoastStyle")
            continue;
        require(profiles.size() < 4096, "Coast style count exceeded");
        const auto &s = n.child("SmoothingOffset"), &h = n.child("HighBeach"), &l = n.child("LowBeach"),
                   &w = n.child("Waterline");
        auto beach = [](const assets::detail::AttributeXmlNode &node) {
            return CoastBeachSource{number<float>(node.attribute("Height")),
                                    number<float>(node.attribute("Offset")),
                                    number<std::int32_t>(node.attribute("Character")),
                                    number<std::int32_t>(node.attribute("TerrainType"))};
        };
        profiles.push_back(compile_coast_profile(
            {n.attribute("type"),
             number<float>(s.attribute("HighSide")),
             number<float>(s.attribute("LowSide")),
             beach(h),
             beach(l),
             {number<float>(w.attribute("Height")), number<float>(w.attribute("Offset")),
              number<std::int32_t>(w.attribute("Character"))}}));
    }
    return {CoastProfiles(profiles), std::move(characters)};
}
std::vector<CoastCharacterSourcePoint> parse_coast_character(std::string_view text) {
    require(text.size() <= text_limit, "Coast Character text exceeds supported bound");
    std::vector<CoastCharacterSourcePoint> result;
    result.reserve(98);
    for (unsigned i = 0; i < 98; ++i) {
        require(!text.empty(), "Coast Character requires 98 source lines");
        const auto end = text.find_first_of("\r\n");
        auto line = text.substr(0, end);
        if (end == text.npos)
            text = {};
        else {
            const auto skip = text[end] == '\r' && end + 1 < text.size() && text[end + 1] == '\n' ? 2 : 1;
            text.remove_prefix(end + skip);
        }
        while (!line.empty() && whitespace(line.back()))
            line.remove_suffix(1);
        require(line.starts_with('[') && line.ends_with(']'), "Invalid coast Character triple");
        line.remove_prefix(1);
        line.remove_suffix(1);
        std::array<float, 3> values{};
        for (unsigned lane = 0; lane < 3; ++lane) {
            const auto comma = line.find(',');
            require((comma == line.npos) == (lane == 2), "Coast Character requires three numbers");
            values[lane] = number<float>(line.substr(0, comma));
            if (lane != 2)
                line.remove_prefix(comma + 1);
        }
        result.push_back({values[0], values[1], values[2]});
    }
    return result;
}
CoastLibrary::CoastLibrary(CoastStyleDocument document, assets::AssetHandle style, X87Precision precision)
    : document_(std::move(document)), style_asset_(std::move(style)), precision_(precision) {}
CoastLibrary CoastLibrary::load(const assets::AssetSource &source, const assets::AssetKey &style_key,
                                X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast Character precision");
    auto style = exact_asset(source, style_key);
    const auto raw = style.read_bytes(text_limit);
    CoastLibrary result(parse_coast_styles(as_text(raw)), std::move(style), precision);
    std::uint64_t total = raw.size();
    for (const auto &filename : result.document_.character_files) {
        auto asset = exact_asset(source, assets::AssetKey(filename));
        total += asset.candidate().payload_size;
        require(total <= 64 * 1024 * 1024, "Coast library source size exceeded");
        const auto bytes = asset.read_bytes(text_limit);
        result.characters_.push_back(
            compile_coast_character(parse_coast_character(as_text(bytes)), precision));
        result.character_assets_.push_back(std::move(asset));
    }
    return result;
}
} // namespace civ5::terrain
