#pragma once
#include "civ5/ui/glyphgen.hpp"
#include "civ5/ui/top_panel.hpp"
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
struct PreviewAlphaFont {
    civ5::ui::GlyphGenAlphaAtlas atlas;
    civ5::ui::GlyphGenGeometry geometry;
    std::size_t extended_glyphs = 0;
};

struct PreviewGdiFont {
    HDC dc = nullptr;
    HFONT font = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ old_font = nullptr;
    HGDIOBJ old_bitmap = nullptr;
    void *pixels = nullptr;
    static constexpr int side = 64;

    PreviewGdiFont(std::string_view face, std::int32_t height) {
        if (face.empty() || face.size() >= LF_FACESIZE || height <= 0 || height > 48)
            throw std::runtime_error("Extended GDI font declaration is invalid");
        LOGFONTW description{};
        description.lfHeight = height;
        description.lfCharSet = DEFAULT_CHARSET;
        description.lfQuality = PROOF_QUALITY;
        for (std::size_t i = 0; i < face.size(); ++i) {
            if (static_cast<unsigned char>(face[i]) >= 0x80)
                throw std::runtime_error("Extended GDI font face must use ASCII");
            description.lfFaceName[i] = static_cast<wchar_t>(face[i]);
        }
        dc = CreateCompatibleDC(nullptr);
        if (!dc)
            throw std::runtime_error("Could not create extended-font GDI context");
        font = CreateFontIndirectW(&description);
        BITMAPINFO information{};
        information.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        information.bmiHeader.biWidth = side;
        information.bmiHeader.biHeight = -side;
        information.bmiHeader.biPlanes = 1;
        information.bmiHeader.biBitCount = 32;
        bitmap = CreateDIBSection(dc, &information, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!font || !bitmap || !pixels) {
            release();
            throw std::runtime_error("Could not create extended-font GDI bitmap");
        }
        old_font = SelectObject(dc, font);
        old_bitmap = SelectObject(dc, bitmap);
        if (!old_font || !old_bitmap || old_font == HGDI_ERROR || old_bitmap == HGDI_ERROR) {
            release();
            throw std::runtime_error("Could not select extended-font GDI objects");
        }
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkMode(dc, TRANSPARENT);
        SetTextAlign(dc, TA_LEFT | TA_BASELINE);
    }
    void release() noexcept {
        if (dc) {
            if (old_font && old_font != HGDI_ERROR)
                SelectObject(dc, old_font);
            if (old_bitmap && old_bitmap != HGDI_ERROR)
                SelectObject(dc, old_bitmap);
        }
        if (font)
            DeleteObject(font);
        if (bitmap)
            DeleteObject(bitmap);
        if (dc)
            DeleteDC(dc);
        dc = nullptr;
        font = nullptr;
        bitmap = nullptr;
    }
    ~PreviewGdiFont() { release(); }
    PreviewGdiFont(const PreviewGdiFont &) = delete;
    PreviewGdiFont &operator=(const PreviewGdiFont &) = delete;
};

std::set<wchar_t> preview_missing_glyphs(const civ5::ui::GlyphGenGeometry &geometry,
                                         const civ5::ui::TopPanelLocalizationCatalog &catalog) {
    std::unordered_set<std::uint32_t> present;
    for (const auto &glyph : geometry.glyphs)
        present.insert(glyph.code_point);
    std::set<wchar_t> missing;
    for (const auto &[key, text] : catalog.rows) {
        (void)key;
        if (text.empty())
            continue;
        if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("Extended-font localization row exceeds Win32 length");
        const auto length = static_cast<int>(text.size());
        const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, nullptr, 0);
        if (count <= 0)
            throw std::runtime_error("Extended-font localization row is not UTF-8");
        std::wstring wide(static_cast<std::size_t>(count), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, wide.data(), count) != count)
            throw std::runtime_error("Extended-font localization conversion changed length");
        for (const wchar_t character : wide) {
            const auto code_point = static_cast<std::uint32_t>(character);
            if (code_point >= 32 && code_point < 0xd800 && !present.contains(code_point))
                missing.insert(character);
            else if (code_point >= 0xd800 && code_point <= 0xdfff)
                throw std::runtime_error("Extended-font localization needs non-BMP glyph handling");
        }
    }
    return missing;
}

PreviewAlphaFont prepare_preview_alpha_font(const civ5::ui::GlyphGenFont &font,
                                             const civ5::assets::DdsData &index,
                                             const civ5::assets::DdsData &packed,
                                             const civ5::ui::TopPanelLocalizationCatalog &catalog,
                                             std::string_view extended_face,
                                             std::uint32_t extra_runtime_rows = 0) {
    PreviewAlphaFont result{civ5::ui::reconstruct_glyphgen_alpha(font, index, packed),
                            civ5::ui::glyphgen_geometry(font), 0};
    if (extra_runtime_rows != 0 && !extended_face.empty()) {
        const auto enlarged_height = std::uint64_t{result.atlas.height} + extra_runtime_rows;
        if (enlarged_height > 4096 ||
            std::uint64_t{result.atlas.width} * enlarged_height >
                std::numeric_limits<std::size_t>::max())
            throw std::runtime_error("Extended GDI atlas exceeds runtime limits");
        // The authored alpha bytes and glyph coordinates remain in the first
        // 512 rows. Only runtime-generated locale glyphs use the added rows.
        result.atlas.alpha.resize(static_cast<std::size_t>(result.atlas.width * enlarged_height), 0);
        result.atlas.height = static_cast<std::uint32_t>(enlarged_height);
        result.geometry.atlas_height = result.atlas.height;
    }
    if (extended_face.empty())
        return result;
    const auto missing = preview_missing_glyphs(result.geometry, catalog);
    if (missing.empty())
        return result;
    PreviewGdiFont gdi(extended_face, result.geometry.line_height);
    std::int32_t atlas_x = 0, atlas_y = 0, row_height = 0;
    for (const auto &glyph : result.geometry.glyphs)
        atlas_y = std::max(atlas_y, glyph.atlas_y + glyph.height);
    ++atlas_y;
    for (const wchar_t character : missing) {
        WORD native_glyph = 0xffff;
        if (GetGlyphIndicesW(gdi.dc, &character, 1, &native_glyph,
                             GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR || native_glyph == 0xffff)
            throw std::runtime_error("Extended GDI face is missing a localization glyph");
        std::fill_n(static_cast<std::uint32_t *>(gdi.pixels), PreviewGdiFont::side * PreviewGdiFont::side,
                    std::uint32_t{0});
        SIZE extent{};
        if (!GetTextExtentPoint32W(gdi.dc, &character, 1, &extent) ||
            !ExtTextOutW(gdi.dc, 16, 32, 0, nullptr, &character, 1, nullptr) || extent.cx <= 0)
            throw std::runtime_error("Extended GDI glyph rasterization failed");
        const auto *pixels = static_cast<const std::uint8_t *>(gdi.pixels);
        int left = PreviewGdiFont::side, top = PreviewGdiFont::side, right = 0, bottom = 0;
        for (int y = 0; y < PreviewGdiFont::side; ++y)
            for (int x = 0; x < PreviewGdiFont::side; ++x)
                if (pixels[(y * PreviewGdiFont::side + x) * 4 + 1]) {
                    left = std::min(left, x);
                    top = std::min(top, y);
                    right = std::max(right, x + 1);
                    bottom = std::max(bottom, y + 1);
                }
        const int width = right - left, height = bottom - top;
        if (width <= 0 || height <= 0)
            throw std::runtime_error("Extended GDI glyph has no ink");
        if (atlas_x + width + 1 > static_cast<std::int32_t>(result.atlas.width)) {
            atlas_x = 0;
            atlas_y += row_height + 1;
            row_height = 0;
        }
        if (atlas_y + height > static_cast<std::int32_t>(result.atlas.height))
            throw std::runtime_error("Extended GDI glyphs exceed the prepared runtime atlas");
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                const auto destination = static_cast<std::size_t>(atlas_y + y) * result.atlas.width + atlas_x + x;
                if (result.atlas.alpha[destination] != 0)
                    throw std::runtime_error("Extended GDI glyph overlaps authored atlas ink");
                result.atlas.alpha[destination] =
                    pixels[((top + y) * PreviewGdiFont::side + left + x) * 4 + 1];
            }
        result.geometry.glyphs.push_back({static_cast<std::uint32_t>(character), atlas_x, atlas_y,
                                          width, height, 0, extent.cx, 0, left - 16, 32 - top});
        atlas_x += width + 1;
        row_height = std::max(row_height, height);
        ++result.extended_glyphs;
    }
    return result;
}
} // namespace
