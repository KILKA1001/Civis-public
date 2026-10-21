#include "civ5/ui/glyphgen.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace civ5;

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
fs::path repo_root() {
    auto path = fs::current_path();
    for (unsigned i = 0; i != 8; ++i) {
        if (fs::exists(path / "civ5-assets") && fs::exists(path / "civ5"))
            return path;
        path = path.parent_path();
    }
    throw std::runtime_error("Cannot locate Civ V repository root");
}
std::shared_ptr<assets::FpkSource> archive(const assets::AssetEnvironment &environment, const char *path) {
    const auto opened = environment.open_fpk_archive(path);
    check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(opened), "UI FPK is unavailable");
    return std::get<std::shared_ptr<assets::FpkSource>>(opened);
}
assets::DdsData dds(const std::shared_ptr<assets::FpkSource> &source, const char *name) {
    const auto lookup = source->find(assets::AssetKey(name));
    check(std::holds_alternative<assets::AssetHandle>(lookup), "GlyphGen DDS is unavailable or ambiguous");
    return assets::DdsData::parse(std::get<assets::AssetHandle>(lookup).read_bytes());
}
} // namespace

int main() {
    try {
        const auto root = repo_root();
        const auto font = ui::read_glyphgen(root / "civ5-assets/Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml");
        check(font.atlas_width() == 512 && font.atlas_height() == 512, "TwCenMT16 atlas dimensions");
        check(font.textures.size() == 5 && font.styles.size() == 6 && font.glyphs.size() == 195,
              "TwCenMT16 authored counts");
        check(font.styles.at(1).name == "Base" && font.styles.at(1).layers.size() == 1 &&
                  font.styles.at(1).layers.front() == "TwCenMT16Base",
              "TwCenMT16 Base style");
        const auto *a = font.glyph('A');
        check(a != nullptr && a->attribute("width") == "10", "TwCenMT16 A glyph");
        const auto geometry = ui::glyphgen_geometry(font);
        check(geometry.advance_adjustment == 0 && geometry.line_height == 16, "TwCenMT16 metric profile");
        const auto line = ui::project_glyphgen_line(geometry, "Turn: 1");
        check(line.advance_width == 43 && line.glyphs.size() == 6, "TwCenMT16 Turn projection");
        auto russian_geometry = geometry;
        for (const std::uint32_t code_point : {0x425u, 0x43eu, 0x434u})
            russian_geometry.glyphs.push_back({code_point, 0, 0, 5, 8, 0, 6, 0, 0, 8});
        const auto russian_line = ui::project_glyphgen_line(russian_geometry, "\xd0\xa5\xd0\xbe\xd0\xb4");
        check(russian_line.advance_width == 18 && russian_line.glyphs.size() == 3 &&
                  russian_line.glyphs[0].code_point == 0x425 &&
                  russian_line.glyphs[1].code_point == 0x43e &&
                  russian_line.glyphs[2].code_point == 0x434,
              "GlyphGen UTF-8 Cyrillic line projection");
        bool rejected_missing_glyph = false;
        try {
            (void)ui::project_glyphgen_line(geometry, "\xd0\xa5");
        } catch (const std::runtime_error &) {
            rejected_missing_glyph = true;
        }
        check(rejected_missing_glyph, "GlyphGen did not reject a missing Cyrillic glyph");
        bool rejected_invalid_utf8 = false;
        try {
            (void)ui::project_glyphgen_line(russian_geometry, "\xc0\xaf");
        } catch (const std::runtime_error &) {
            rejected_invalid_utf8 = true;
        }
        check(rejected_invalid_utf8, "GlyphGen accepted overlong UTF-8");

        const assets::AssetEnvironment environment(root / "civ5-assets");
        const auto source = archive(environment, "Resource/DX9/UITextures.fpk");
        const auto index = dds(source, "TwCenMT16Base-index.dds");
        const auto packed = dds(source, "TwCenMT16Base.dds");
        check(index.width() == 128 && index.height() == 128 && index.format() == assets::DdsFormat::luminance &&
                  index.bits_per_pixel() == 16,
              "TwCenMT16 index profile");
        check(packed.width() == 428 && packed.height() == 48 && packed.format() == assets::DdsFormat::alpha8,
              "TwCenMT16 packed alpha profile");
        const auto atlas = ui::reconstruct_glyphgen_alpha(font, index, packed);
        check(atlas.width == 512 && atlas.height == 512 && atlas.block_width == 4 && atlas.block_height == 4 &&
                  atlas.packed_tile_capacity == 1284 && atlas.maximum_index < atlas.packed_tile_capacity &&
                  atlas.alpha.size() == 512u * 512u,
              "TwCenMT16 reconstructed atlas");
        check(std::any_of(atlas.alpha.begin(), atlas.alpha.end(), [](auto value) { return value != 0; }),
              "TwCenMT16 reconstructed alpha is nonempty");
        const auto icon_font = ui::read_glyphgen(root / "civ5-assets/Assets/UI/Fonts/Font Icons/FontIcons.ggxml");
        check(icon_font.atlas_width() == 256 && icon_font.atlas_height() == 256 &&
                  icon_font.glyphs.size() == 79,
              "FontIcons authored counts");
        const auto icon_index = dds(source, "fonticons-index.dds");
        const auto icon_packed = dds(source, "fonticons.dds");
        check(icon_index.width() == 128 && icon_index.height() == 128 &&
                  icon_packed.width() == 246 && icon_packed.height() == 90 &&
                  icon_packed.format() == assets::DdsFormat::masked_rgb &&
                  icon_packed.bits_per_pixel() == 32,
              "FontIcons indexed RGBA profile");
        const auto icon_atlas = ui::reconstruct_glyphgen_rgba(icon_font, icon_index, icon_packed);
        check(icon_atlas.width == 256 && icon_atlas.height == 256 && icon_atlas.block_width == 2 &&
                  icon_atlas.block_height == 2 && icon_atlas.packed_tile_capacity == 5535 &&
                  icon_atlas.maximum_index == 5531 && icon_atlas.rgba.size() == 256u * 256u * 4,
              "FontIcons reconstructed atlas");
        const auto icon_geometry = ui::glyphgen_geometry(icon_font);
        const auto russian_markup = ui::project_glyphgen_markup(
            russian_geometry, icon_geometry, "[COLOR:33:190:247:255]\xd0\xa5\xd0\xbe\xd0\xb4[/COLOR]",
            [](std::string_view) -> std::optional<std::uint32_t> { return std::nullopt; });
        check(russian_markup.advance_width == 18 && russian_markup.glyphs.size() == 3 &&
                  russian_markup.glyphs.front().code_point == 0x425 &&
                  russian_markup.glyphs.back().code_point == 0x434 &&
                  russian_markup.glyphs.back().color == std::array<std::uint8_t, 4>{33, 190, 247, 255},
              "GlyphGen UTF-8 Cyrillic markup projection");
        const auto projected = ui::project_glyphgen_markup(
            geometry, icon_geometry, "[ICON_RESEARCH][COLOR:33:190:247:255]+5[/COLOR]",
            [](std::string_view name) -> std::optional<std::uint32_t> {
                return name == "ICON_RESEARCH" ? std::optional<std::uint32_t>(2) : std::nullopt;
            });
        check(projected.advance_width == 41 && projected.glyphs.size() == 3 && projected.glyphs.front().icon &&
                  projected.glyphs.front().code_point == 2 && !projected.glyphs.back().icon &&
                  projected.glyphs.back().color == std::array<std::uint8_t, 4>{33, 190, 247, 255},
              "FontIcons inline markup projection");
        const auto block = ui::project_glyphgen_markup_block(
            geometry, icon_geometry, "[COLOR:150:255:150:255]A[NEWLINE]B[/COLOR]",
            [](std::string_view name) -> std::optional<std::uint32_t> {
                return name == "ICON_RESEARCH" ? std::optional<std::uint32_t>(2) : std::nullopt;
            },
            100);
        check(block.size() == 2 && block[0].glyphs.size() == 1 && block[1].glyphs.size() == 1 &&
                  block[0].glyphs.front().color == std::array<std::uint8_t, 4>{150, 255, 150, 255} &&
                  block[1].glyphs.front().color == std::array<std::uint8_t, 4>{150, 255, 150, 255},
              "GlyphGen block projection did not retain color across NEWLINE");
        const auto locale_colors = ui::project_glyphgen_markup(
            geometry, icon_geometry,
            "[COLOR_GREEN]A[COLOR_RED]B[ENDCOLOR]C[ENDCOLOR][COLOR_YELLOW]D[ENDCOLOR]",
            [](std::string_view) -> std::optional<std::uint32_t> { return std::nullopt; });
        check(locale_colors.glyphs.size() == 4 &&
                  locale_colors.glyphs[0].color == std::array<std::uint8_t, 4>{0, 255, 0, 255} &&
                  locale_colors.glyphs[1].color == std::array<std::uint8_t, 4>{255, 0, 0, 255} &&
                  locale_colors.glyphs[2].color == std::array<std::uint8_t, 4>{0, 255, 0, 255} &&
                  locale_colors.glyphs[3].color == std::array<std::uint8_t, 4>{255, 255, 0, 255},
              "GlyphGen named locale colors or nested ENDCOLOR changed");
        std::cout << "PASS: native GlyphGen parser, metrics, indexed alpha/RGBA atlases and inline markup checks\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
