#include "civ5/ui/glyphgen.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <limits>
#include <string>
#include <stdexcept>
#include <unordered_map>

namespace civ5::ui {
namespace {
using Attributes = std::vector<std::pair<std::string, std::string>>;
void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
}
std::string attr(const Attributes &attributes, std::string_view key, const std::string &owner) {
    const auto found = std::find_if(attributes.begin(), attributes.end(),
                                    [&](const auto &entry) { return entry.first == key; });
    require(found != attributes.end(), owner + " is missing " + std::string(key));
    return found->second;
}
std::int64_t integer(std::string_view text, const std::string &owner) {
    if (text.empty())
        throw std::runtime_error(owner + " has an empty integer");
    std::int64_t value = 0;
    const auto *first = text.data();
    const auto *last = first + text.size();
    const auto result = std::from_chars(first, last, value);
    require(result.ec == std::errc{} && result.ptr == last, owner + " has an invalid integer");
    return value;
}
std::int32_t integer32(std::string_view text, const std::string &owner) {
    const auto value = integer(text, owner);
    require(value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max(),
            owner + " integer exceeds int32");
    return static_cast<std::int32_t>(value);
}
std::uint32_t nonnegative(std::string_view text, const std::string &owner) {
    const auto value = integer(text, owner);
    require(value >= 0 && value <= std::numeric_limits<std::uint32_t>::max(), owner + " must be nonnegative");
    return static_cast<std::uint32_t>(value);
}
const content::XmlNode &exact_child(const content::XmlNode &parent, std::string_view name,
                                    std::size_t ordinal = 0) {
    const auto found = std::find_if(parent.children.begin(), parent.children.end(),
                                    [&](const auto &child) { return child.name == name; });
    require(found != parent.children.end(), "GlyphGen source is missing " + std::string(name));
    const auto first = static_cast<std::size_t>(found - parent.children.begin());
    require(first == ordinal, "GlyphGen source has an unexpected direct-child order");
    require(std::count_if(parent.children.begin(), parent.children.end(),
                          [&](const auto &child) { return child.name == name; }) == 1,
            "GlyphGen source must contain exactly one " + std::string(name));
    return *found;
}
void require_count(const content::XmlNode &node, std::size_t actual, const std::string &owner) {
    const auto declared = nonnegative(attr(node.attributes, "count", owner), owner + ".count");
    require(declared == actual, owner + " declares a count that differs from its children");
}
std::uint32_t required_dimension(const Attributes &attributes, std::string_view key, const std::string &owner) {
    return nonnegative(attr(attributes, key, owner), owner + "." + std::string(key));
}
std::uint32_t next_utf8_code_point(std::string_view text, std::size_t &cursor) {
    require(cursor < text.size(), "GlyphGen UTF-8 cursor exceeds text");
    const auto first = static_cast<std::uint8_t>(text[cursor++]);
    if (first < 0x80)
        return first;
    std::size_t continuation_count = 0;
    std::uint32_t code_point = 0;
    std::uint8_t second_minimum = 0x80, second_maximum = 0xbf;
    if (first >= 0xc2 && first <= 0xdf) {
        continuation_count = 1;
        code_point = first & 0x1f;
    } else if (first >= 0xe0 && first <= 0xef) {
        continuation_count = 2;
        code_point = first & 0x0f;
        if (first == 0xe0)
            second_minimum = 0xa0;
        if (first == 0xed)
            second_maximum = 0x9f;
    } else if (first >= 0xf0 && first <= 0xf4) {
        continuation_count = 3;
        code_point = first & 0x07;
        if (first == 0xf0)
            second_minimum = 0x90;
        if (first == 0xf4)
            second_maximum = 0x8f;
    } else {
        throw std::runtime_error("GlyphGen text contains invalid UTF-8");
    }
    require(cursor + continuation_count <= text.size(), "GlyphGen text contains truncated UTF-8");
    for (std::size_t index = 0; index < continuation_count; ++index) {
        const auto byte = static_cast<std::uint8_t>(text[cursor++]);
        require(byte >= (index == 0 ? second_minimum : 0x80) &&
                    byte <= (index == 0 ? second_maximum : 0xbf),
                "GlyphGen text contains invalid UTF-8");
        code_point = (code_point << 6) | (byte & 0x3f);
    }
    return code_point;
}
} // namespace

std::string GlyphGenTexture::name() const { return attr(attributes, "name", "GlyphGen texture"); }
std::string GlyphGenTexture::source() const { return attr(attributes, "src", "GlyphGen texture"); }
std::string GlyphGenGlyph::attribute(std::string_view key) const {
    const auto found = std::find_if(attributes.begin(), attributes.end(),
                                    [&](const auto &entry) { return entry.first == key; });
    return found == attributes.end() ? std::string{} : found->second;
}

GlyphGenFont GlyphGenFont::from_xml(const content::XmlNode &root) {
    require(root.name == "glyphgen", "GlyphGen root must be glyphgen");
    require(root.children.size() >= 3, "GlyphGen root must contain textures, styles and glyphs");
    const auto &textures_node = exact_child(root, "textures", 0);
    const auto &styles_node = exact_child(root, "styles", 1);
    const auto &glyphs_node = exact_child(root, "glyphs", 2);

    GlyphGenFont result{root.attributes, textures_node.attributes, {}, {}, glyphs_node.attributes, 0, {}};
    require(!textures_node.children.empty(), "GlyphGen source must contain textures");
    std::unordered_map<std::string, bool> texture_names;
    for (const auto &node : textures_node.children) {
        require(node.name == "texture", "GlyphGen textures may contain only texture elements");
        (void)attr(node.attributes, "name", "GlyphGen texture");
        (void)attr(node.attributes, "src", "GlyphGen texture");
        const auto name = node.attribute("name");
        require(texture_names.emplace(name, true).second, "GlyphGen source has duplicate texture names");
        result.textures.push_back({node.attributes});
    }
    std::unordered_map<std::string, bool> style_names;
    for (const auto &node : styles_node.children) {
        require(node.name == "style", "GlyphGen styles may contain only style elements");
        const auto name = attr(node.attributes, "name", "GlyphGen style");
        require(style_names.emplace(name, true).second, "GlyphGen source has duplicate style names");
        require(node.children.size() == 1 && node.children.front().name == "layers",
                "GlyphGen style must contain exactly one layers element");
        const auto &layers = node.children.front();
        require_count(layers, layers.children.size(), "GlyphGen layers for style '" + name + "'");
        GlyphGenStyle style{name, {}, node.attributes};
        for (const auto &layer : layers.children) {
            require(layer.name == "layer", "GlyphGen layers may contain only layer elements");
            const auto layer_name = attr(layer.attributes, "tex", "GlyphGen layer");
            require(texture_names.contains(layer_name), "GlyphGen style layer refers to an undeclared texture");
            style.layers.push_back(layer_name);
        }
        require(!style.layers.empty(), "GlyphGen style must contain at least one layer");
        result.styles.push_back(std::move(style));
    }
    require_count(styles_node, result.styles.size(), "GlyphGen styles");
    result.declared_glyph_count = integer32(attr(glyphs_node.attributes, "count", "GlyphGen glyphs"),
                                            "GlyphGen glyphs.count");
    require(result.declared_glyph_count >= 0, "GlyphGen glyph count must be nonnegative");
    std::unordered_map<std::uint32_t, bool> code_points;
    for (const auto &node : glyphs_node.children) {
        require(node.name == "glyph", "GlyphGen glyphs may contain only glyph elements");
        const auto value = integer(attr(node.attributes, "ch", "GlyphGen glyph"), "GlyphGen glyph.ch");
        require(value >= 0 && value <= 65535, "GlyphGen glyph code point is outside UTF-16");
        const auto code_point = static_cast<std::uint32_t>(value);
        require(code_points.emplace(code_point, true).second, "GlyphGen source has duplicate glyph code points");
        result.glyphs.push_back({code_point, node.attributes});
    }
    return result;
}

GlyphGenFont read_glyphgen(const std::filesystem::path &path) {
    return GlyphGenFont::from_xml(content::read_xml(path));
}

GlyphGenFont read_glyphgen(const assets::ResourceMountTable &resources, std::string_view key) {
    const auto mounted = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
    if (!handle)
        throw std::runtime_error("GlyphGen VFS XML is missing or ambiguous: " + std::string(key));
    constexpr std::uint64_t maximum = 64ULL * 1024 * 1024;
    const auto bytes = handle->read_bytes(maximum);
    return GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(), bytes.end())));
}

const GlyphGenGlyph *GlyphGenFont::glyph(std::uint32_t code_point) const {
    const auto found = std::find_if(glyphs.begin(), glyphs.end(),
                                    [&](const auto &entry) { return entry.code_point == code_point; });
    return found == glyphs.end() ? nullptr : &*found;
}
std::uint32_t GlyphGenFont::atlas_width() const { return required_dimension(texture_atlas_attributes, "width", "GlyphGen atlas"); }
std::uint32_t GlyphGenFont::atlas_height() const { return required_dimension(texture_atlas_attributes, "height", "GlyphGen atlas"); }

GlyphGenGeometry glyphgen_geometry(const GlyphGenFont &font) {
    GlyphGenGeometry result;
    result.atlas_width = font.atlas_width();
    result.atlas_height = font.atlas_height();
    require(result.atlas_width <= 4096 && result.atlas_height <= 4096, "GlyphGen atlas exceeds 4096 dimensions");
    const auto &container = font.glyph_container_attributes;
    const auto value = [&](std::string_view key) {
        return integer32(attr(container, key, "GlyphGen glyph container"),
                         "GlyphGen glyph container." + std::string(key));
    };
    result.line_height = value("height");
    result.ascent = value("ascent");
    result.descent = value("descent");
    result.ascent_adjustment = value("ascadj");
    result.spacing = value("spacing");
    result.advance_adjustment = value("fudgeadv");
    for (const auto &source : font.glyphs) {
        const auto value_for = [&](std::string_view key) {
            return integer32(source.attribute(key), "GlyphGen glyph metric");
        };
        GlyphGenGlyphGeometry glyph{source.code_point, value_for("u"), value_for("v"), value_for("width"),
                                    value_for("height"), value_for("a"), value_for("b"), value_for("c"),
                                    value_for("originx"), value_for("originy")};
        require(glyph.atlas_x >= 0 && glyph.atlas_y >= 0 && glyph.width >= 0 && glyph.height >= 0 &&
                    std::uint64_t{static_cast<std::uint32_t>(glyph.atlas_x)} + glyph.width <= result.atlas_width &&
                    std::uint64_t{static_cast<std::uint32_t>(glyph.atlas_y)} + glyph.height <= result.atlas_height,
                "GlyphGen glyph lies outside the declared atlas");
        result.glyphs.push_back(glyph);
    }
    return result;
}

GlyphGenLine project_glyphgen_line(const GlyphGenGeometry &geometry, std::string_view text) {
    require(geometry.advance_adjustment == 0, "GlyphGen nonzero fudgeadv is not supported");
    require(text.find_first_of("\r\n") == std::string_view::npos, "GlyphGen line projection is single-line only");
    std::unordered_map<std::uint32_t, const GlyphGenGlyphGeometry *> by_code_point;
    for (const auto &glyph : geometry.glyphs)
        by_code_point.emplace(glyph.code_point, &glyph);
    GlyphGenLine result;
    std::int64_t pen = 0;
    for (std::size_t cursor = 0; cursor < text.size();) {
        const auto code_point = next_utf8_code_point(text, cursor);
        const auto found = by_code_point.find(code_point);
        require(found != by_code_point.end(), "GlyphGen line contains a missing glyph");
        const auto &glyph = *found->second;
        const auto left = pen + glyph.origin_x;
        const auto next = pen + glyph.advance();
        require(left >= std::numeric_limits<std::int32_t>::min() && left <= std::numeric_limits<std::int32_t>::max() &&
                    next >= std::numeric_limits<std::int32_t>::min() && next <= std::numeric_limits<std::int32_t>::max(),
                "GlyphGen line geometry overflows int32");
        if (glyph.width > 0 && glyph.height > 0)
            result.glyphs.push_back({glyph.code_point, glyph.atlas_x, glyph.atlas_y, glyph.width, glyph.height,
                                     static_cast<std::int32_t>(left), glyph.origin_y});
        pen = next;
    }
    require(pen >= std::numeric_limits<std::int32_t>::min() && pen <= std::numeric_limits<std::int32_t>::max(),
            "GlyphGen line advance overflows int32");
    result.advance_width = static_cast<std::int32_t>(pen);
    return result;
}

GlyphGenAlphaAtlas reconstruct_glyphgen_alpha(const GlyphGenFont &font, const assets::DdsData &index,
                                              const assets::DdsData &packed) {
    require(index.face_count() == 1 && index.mip_count() == 1 && index.format() == assets::DdsFormat::luminance &&
                index.bits_per_pixel() == 16 && index.channel_masks() == std::array<std::uint32_t, 4>{0xffff, 0, 0, 0},
            "GlyphGen index is not the observed L16 profile");
    require(packed.face_count() == 1 && packed.mip_count() == 1 && packed.format() == assets::DdsFormat::alpha8 &&
                packed.bits_per_pixel() == 8,
            "GlyphGen packed atlas is not the observed alpha-only profile");
    const auto virtual_width = font.atlas_width(), virtual_height = font.atlas_height();
    require(virtual_width % index.width() == 0 && virtual_height % index.height() == 0,
            "GlyphGen virtual atlas is not divisible by its index");
    const auto block_width = virtual_width / index.width(), block_height = virtual_height / index.height();
    require(block_width > 0 && block_height > 0 && packed.width() % block_width == 0 &&
                packed.height() % block_height == 0,
            "GlyphGen packed atlas is not block-aligned");
    const auto tiles_per_row = packed.width() / block_width, tile_rows = packed.height() / block_height;
    const auto capacity = std::uint64_t{tiles_per_row} * tile_rows;
    require(capacity <= std::numeric_limits<std::uint32_t>::max(), "GlyphGen packed capacity overflows uint32");
    const auto index_bytes = index.payload(0, 0), packed_bytes = packed.payload(0, 0);
    require(index_bytes.size() == std::uint64_t{index.width()} * index.height() * 2 &&
                packed_bytes.size() == std::uint64_t{packed.width()} * packed.height(),
            "GlyphGen DDS payload sizes do not match dimensions");
    const auto output_size = std::uint64_t{virtual_width} * virtual_height;
    require(output_size <= std::numeric_limits<std::size_t>::max(), "GlyphGen virtual alpha atlas is too large");
    GlyphGenAlphaAtlas result{virtual_width, virtual_height, block_width, block_height,
                              static_cast<std::uint32_t>(capacity), 0, assets::Bytes(static_cast<std::size_t>(output_size))};
    for (std::uint32_t y = 0; y < index.height(); ++y) {
        for (std::uint32_t x = 0; x < index.width(); ++x) {
            const auto at = (std::uint64_t{y} * index.width() + x) * 2;
            const auto tile = std::uint32_t(index_bytes[at]) | (std::uint32_t(index_bytes[at + 1]) << 8);
            result.maximum_index = std::max(result.maximum_index, tile);
            require(tile < capacity, "GlyphGen index sample exceeds packed tile capacity");
            const auto tile_x = tile % tiles_per_row, tile_y = tile / tiles_per_row;
            for (std::uint32_t local_y = 0; local_y < block_height; ++local_y)
                for (std::uint32_t local_x = 0; local_x < block_width; ++local_x) {
                    const auto source_x = tile_x * block_width + local_x;
                    const auto source_y = tile_y * block_height + local_y;
                    const auto destination_x = x * block_width + local_x;
                    const auto destination_y = y * block_height + local_y;
                    result.alpha[std::uint64_t{destination_y} * virtual_width + destination_x] =
                        packed_bytes[std::uint64_t{source_y} * packed.width() + source_x];
                }
        }
    }
    return result;
}

GlyphGenRgbaAtlas reconstruct_glyphgen_rgba(const GlyphGenFont &font, const assets::DdsData &index,
                                            const assets::DdsData &packed) {
    require(index.face_count() == 1 && index.mip_count() == 1 && index.format() == assets::DdsFormat::luminance &&
                index.bits_per_pixel() == 16 && index.channel_masks() == std::array<std::uint32_t, 4>{0xffff, 0, 0, 0},
            "GlyphGen index is not the observed L16 profile");
    require(packed.face_count() == 1 && packed.mip_count() == 1 && packed.format() == assets::DdsFormat::masked_rgb &&
                packed.bits_per_pixel() == 32 &&
                (packed.channel_masks() == std::array<std::uint32_t, 4>{0xff0000, 0xff00, 0xff, 0xff000000} ||
                 packed.channel_masks() == std::array<std::uint32_t, 4>{0xff, 0xff00, 0xff0000, 0xff000000}),
            "GlyphGen packed atlas is not the observed 32-bit RGBA profile");
    const auto virtual_width = font.atlas_width(), virtual_height = font.atlas_height();
    require(virtual_width % index.width() == 0 && virtual_height % index.height() == 0,
            "GlyphGen virtual atlas is not divisible by its index");
    const auto block_width = virtual_width / index.width(), block_height = virtual_height / index.height();
    require(block_width > 0 && block_height > 0 && packed.width() % block_width == 0 &&
                packed.height() % block_height == 0,
            "GlyphGen packed atlas is not block-aligned");
    const auto tiles_per_row = packed.width() / block_width, tile_rows = packed.height() / block_height;
    const auto capacity = std::uint64_t{tiles_per_row} * tile_rows;
    require(capacity <= std::numeric_limits<std::uint32_t>::max(), "GlyphGen packed capacity overflows uint32");
    const auto index_bytes = index.payload(0, 0), packed_bytes = packed.payload(0, 0);
    require(index_bytes.size() == std::uint64_t{index.width()} * index.height() * 2 &&
                packed_bytes.size() == std::uint64_t{packed.width()} * packed.height() * 4,
            "GlyphGen DDS payload sizes do not match dimensions");
    const auto output_size = std::uint64_t{virtual_width} * virtual_height * 4;
    require(output_size <= std::numeric_limits<std::size_t>::max(), "GlyphGen virtual RGBA atlas is too large");
    GlyphGenRgbaAtlas result{virtual_width, virtual_height, block_width, block_height,
                             static_cast<std::uint32_t>(capacity), 0,
                             assets::Bytes(static_cast<std::size_t>(output_size))};
    const auto masks = packed.channel_masks();
    const auto channel = [&](std::uint32_t raw, std::uint32_t mask) {
        if (!mask)
            return static_cast<std::uint8_t>(255);
        const auto shift = static_cast<unsigned>(std::countr_zero(mask));
        const auto maximum = mask >> shift;
        const auto value = (raw & mask) >> shift;
        return static_cast<std::uint8_t>((std::uint64_t{value} * 255 + maximum / 2) / maximum);
    };
    for (std::uint32_t y = 0; y < index.height(); ++y) {
        for (std::uint32_t x = 0; x < index.width(); ++x) {
            const auto at = (std::uint64_t{y} * index.width() + x) * 2;
            const auto tile = std::uint32_t(index_bytes[at]) | (std::uint32_t(index_bytes[at + 1]) << 8);
            result.maximum_index = std::max(result.maximum_index, tile);
            require(tile < capacity, "GlyphGen index sample exceeds packed tile capacity");
            const auto tile_x = tile % tiles_per_row, tile_y = tile / tiles_per_row;
            for (std::uint32_t local_y = 0; local_y < block_height; ++local_y)
                for (std::uint32_t local_x = 0; local_x < block_width; ++local_x) {
                    const auto source_x = tile_x * block_width + local_x;
                    const auto source_y = tile_y * block_height + local_y;
                    const auto source = (std::uint64_t{source_y} * packed.width() + source_x) * 4;
                    const auto raw = std::uint32_t(packed_bytes[source]) |
                                     (std::uint32_t(packed_bytes[source + 1]) << 8) |
                                     (std::uint32_t(packed_bytes[source + 2]) << 16) |
                                     (std::uint32_t(packed_bytes[source + 3]) << 24);
                    const auto destination_x = x * block_width + local_x;
                    const auto destination_y = y * block_height + local_y;
                    const auto destination = (std::uint64_t{destination_y} * virtual_width + destination_x) * 4;
                    result.rgba[destination] = channel(raw, masks[0]);
                    result.rgba[destination + 1] = channel(raw, masks[1]);
                    result.rgba[destination + 2] = channel(raw, masks[2]);
                    result.rgba[destination + 3] = channel(raw, masks[3]);
                }
        }
    }
    return result;
}

namespace {
std::array<std::uint8_t, 4> inline_named_color(std::string_view name) {
    // These named spans appear in the shipped TopPanel locale strings. Their
    // RGB values come from Assets/Gameplay/XML/Interface/CIV5Colors.xml.
    if (name == "COLOR_GREEN")
        return {0, 255, 0, 255};
    if (name == "COLOR_RED")
        return {255, 0, 0, 255};
    if (name == "COLOR_YELLOW")
        return {255, 255, 0, 255};
    if (name == "COLOR_POSITIVE_TEXT")
        return {60, 255, 60, 255};
    if (name == "COLOR_WARNING_TEXT" || name == "COLOR_NEGATIVE_TEXT")
        return {255, 60, 60, 255};
    if (name == "COLOR_REVERT")
        return {255, 255, 200, 255};
    throw std::runtime_error("Unknown GlyphGen named color: " + std::string(name));
}
std::array<std::uint8_t, 4> inline_numeric_color(std::string_view tag) {
    require(tag.size() > 7 && tag.substr(0, 6) == "COLOR:", "Malformed GlyphGen numeric color");
    std::array<std::uint8_t, 4> result{};
    std::size_t cursor = 6;
    for (std::size_t component = 0; component != result.size(); ++component) {
        const auto end = tag.find(':', cursor);
        const auto stop = end == std::string_view::npos ? tag.size() : end;
        const auto value = integer(tag.substr(cursor, stop - cursor), "GlyphGen color component");
        require(value >= 0 && value <= 255, "GlyphGen color component exceeds byte");
        result[component] = static_cast<std::uint8_t>(value);
        cursor = stop + 1;
        require(component + 1 == result.size() || end != std::string_view::npos,
                "GlyphGen numeric color requires four components");
    }
    require(cursor == tag.size() + 1 || cursor == tag.size(), "GlyphGen numeric color has trailing data");
    return result;
}
GlyphGenInlineLine project_markup_run(
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry, std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point,
    std::vector<std::array<std::uint8_t, 4>> &colors) {
    require(icon_code_point != nullptr, "GlyphGen markup requires an icon resolver");
    require(text_geometry.advance_adjustment == 0 && icon_geometry.advance_adjustment == 0,
            "GlyphGen markup requires zero fudgeadv fonts");
    std::unordered_map<std::uint32_t, const GlyphGenGlyphGeometry *> text_by_code, icon_by_code;
    for (const auto &glyph : text_geometry.glyphs)
        text_by_code.emplace(glyph.code_point, &glyph);
    for (const auto &glyph : icon_geometry.glyphs)
        icon_by_code.emplace(glyph.code_point, &glyph);
    GlyphGenInlineLine result;
    const auto emit = [&](const GlyphGenGlyphGeometry &glyph, bool icon) {
        const auto pen = static_cast<std::int64_t>(result.advance_width);
        const auto left = pen + glyph.origin_x;
        const auto next = pen + glyph.advance();
        require(left >= std::numeric_limits<std::int32_t>::min() && left <= std::numeric_limits<std::int32_t>::max() &&
                    next >= std::numeric_limits<std::int32_t>::min() && next <= std::numeric_limits<std::int32_t>::max(),
                "GlyphGen markup geometry overflows int32");
        if (glyph.width > 0 && glyph.height > 0)
            result.glyphs.push_back({icon, glyph.code_point, glyph.atlas_x, glyph.atlas_y, glyph.width, glyph.height,
                                     static_cast<std::int32_t>(left),
                                     glyph.origin_y + (icon ? icon_geometry.ascent_adjustment
                                                             : text_geometry.ascent_adjustment),
                                     colors.back()});
        result.advance_width = static_cast<std::int32_t>(next);
    };
    for (std::size_t cursor = 0; cursor < text.size();) {
        if (text[cursor] == '[') {
            const auto close = text.find(']', cursor + 1);
            if (close != std::string_view::npos) {
                const auto tag = text.substr(cursor + 1, close - cursor - 1);
                if (tag.rfind("ICON_", 0) == 0) {
                    const auto code = icon_code_point(tag);
                    require(code.has_value(), "GlyphGen markup contains an unknown icon");
                    const auto found = icon_by_code.find(*code);
                    require(found != icon_by_code.end(), "GlyphGen markup icon has no glyph");
                    emit(*found->second, true);
                    cursor = close + 1;
                    continue;
                }
                if (tag == "NEWLINE" || tag == "TAB")
                    throw std::runtime_error("GlyphGen markup line contains a line control");
                if (tag.rfind("COLOR:", 0) == 0) {
                    colors.push_back(inline_numeric_color(tag));
                    cursor = close + 1;
                    continue;
                }
                if (tag == "COLOR_POSITIVE_TEXT" || tag == "COLOR_WARNING_TEXT" ||
                    tag == "COLOR_NEGATIVE_TEXT" || tag == "COLOR_GREEN" ||
                    tag == "COLOR_RED" || tag == "COLOR_YELLOW") {
                    colors.push_back(inline_named_color(tag));
                    cursor = close + 1;
                    continue;
                }
                if (tag == "ENDCOLOR" || tag == "/COLOR" || tag == "COLOR_REVERT") {
                    require(colors.size() > 1, "GlyphGen markup color stack underflow");
                    colors.pop_back();
                    cursor = close + 1;
                    continue;
                }
            }
        }
        const auto code = next_utf8_code_point(text, cursor);
        const auto found = text_by_code.find(code);
        require(found != text_by_code.end(), "GlyphGen markup text contains a missing glyph");
        emit(*found->second, false);
    }
    return result;
}
} // namespace

GlyphGenInlineLine project_glyphgen_markup(
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry, std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point) {
    std::vector<std::array<std::uint8_t, 4>> colors{{255, 255, 200, 255}};
    auto result = project_markup_run(text_geometry, icon_geometry, text, icon_code_point, colors);
    require(colors.size() == 1, "GlyphGen markup has an unbalanced color stack");
    return result;
}

std::vector<GlyphGenInlineLine> project_glyphgen_markup_block(
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry, std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point,
    std::int32_t wrap_width) {
    require(wrap_width > 0, "GlyphGen markup wrap width must be positive");
    std::vector<std::array<std::uint8_t, 4>> colors{{255, 255, 200, 255}};
    std::vector<GlyphGenInlineLine> lines(1);
    GlyphGenInlineLine spaces;
    const auto append = [](GlyphGenInlineLine &line, const GlyphGenInlineLine &run) {
        const auto advance = std::int64_t{line.advance_width} + run.advance_width;
        require(advance >= 0 && advance <= std::numeric_limits<std::int32_t>::max(),
                "GlyphGen wrapped line advance overflows int32");
        for (auto glyph : run.glyphs) {
            const auto left = std::int64_t{line.advance_width} + glyph.left_from_line_origin;
            require(left >= std::numeric_limits<std::int32_t>::min() &&
                        left <= std::numeric_limits<std::int32_t>::max(),
                    "GlyphGen wrapped glyph position overflows int32");
            glyph.left_from_line_origin = static_cast<std::int32_t>(left);
            line.glyphs.push_back(glyph);
        }
        line.advance_width = static_cast<std::int32_t>(advance);
    };
    for (std::size_t cursor = 0; cursor < text.size();) {
        if (text.substr(cursor).starts_with("[NEWLINE]")) {
            spaces = {};
            lines.emplace_back();
            cursor += std::string_view("[NEWLINE]").size();
            continue;
        }
        const auto begin = cursor;
        const bool whitespace = text[cursor] == ' ';
        do {
            if (text[cursor] == '[') {
                const auto close = text.find(']', cursor + 1);
                cursor = close == std::string_view::npos ? cursor + 1 : close + 1;
            } else {
                ++cursor;
            }
        } while (cursor < text.size() && (text[cursor] == ' ') == whitespace &&
                 !text.substr(cursor).starts_with("[NEWLINE]"));
        const auto run = project_markup_run(text_geometry, icon_geometry, text.substr(begin, cursor - begin),
                                             icon_code_point, colors);
        if (whitespace) {
            append(spaces, run);
            continue;
        }
        // Color-only runs do not force a wrap or consume leading whitespace.
        if (run.glyphs.empty() && run.advance_width == 0)
            continue;
        if (std::int64_t{lines.back().advance_width} + spaces.advance_width + run.advance_width > wrap_width &&
            lines.back().advance_width != 0) {
            lines.emplace_back();
            spaces = {};
        }
        append(lines.back(), spaces);
        append(lines.back(), run);
        spaces = {};
    }
    require(colors.size() == 1, "GlyphGen markup has an unbalanced color stack");
    return lines;
}
} // namespace civ5::ui
