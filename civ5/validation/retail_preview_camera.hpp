#pragma once
#include "civ5/terrain/camera_frame.hpp"
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

// Diagnostic-only import of a passive retail DX9 camera read and the TGA it
// brackets. The TGA header supplies the actual client raster, which may differ
// from the configured window size. Neither source is inferred from a save.
struct RetailPreviewCamera {
    int width = 0, height = 0;
    civ5::terrain::PerspectiveCamera camera{};
};

namespace retail_preview_camera_detail {
inline std::string read_text(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open passive retail camera record");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
template <std::size_t N>
std::array<float, N> array(const std::string &source, std::string_view key) {
    const auto quoted = '"' + std::string(key) + '"';
    const auto found = source.find(quoted);
    if (found == std::string::npos || source.find(quoted, found + quoted.size()) != std::string::npos)
        throw std::runtime_error("Missing or duplicate retail camera field: " + std::string(key));
    auto cursor = source.find(':', found + quoted.size());
    if (cursor == std::string::npos) throw std::runtime_error("Missing retail camera field colon");
    cursor = source.find('[', cursor + 1);
    if (cursor == std::string::npos) throw std::runtime_error("Missing retail camera array");
    ++cursor;
    const auto whitespace = [&] {
        while (cursor < source.size() && (source[cursor] == ' ' || source[cursor] == '\t' ||
                                          source[cursor] == '\r' || source[cursor] == '\n')) ++cursor;
    };
    std::array<float, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
        whitespace();
        const auto *begin = source.data() + cursor;
        const auto parsed = std::from_chars(begin, source.data() + source.size(), result[i],
                                            std::chars_format::general);
        if (parsed.ec != std::errc{} || parsed.ptr == begin || !std::isfinite(result[i]))
            throw std::runtime_error("Invalid retail camera number");
        cursor = static_cast<std::size_t>(parsed.ptr - source.data());
        whitespace();
        if (cursor >= source.size() || source[cursor++] != (i + 1 == N ? ']' : ','))
            throw std::runtime_error("Invalid retail camera array length");
    }
    return result;
}
} // namespace retail_preview_camera_detail

inline RetailPreviewCamera load_retail_preview_camera(const std::filesystem::path &pose,
                                                      const std::filesystem::path &tga) {
    const auto source = retail_preview_camera_detail::read_text(pose);
    if (source.find("\"schema\": \"civ5-retail-dx9-passive-camera-v1\"") == std::string::npos ||
        source.find("\"readOnlyProcessAccess\": true") == std::string::npos ||
        source.find("\"debuggerAttached\": false") == std::string::npos ||
        source.find("\"injectedHook\": false") == std::string::npos)
        throw std::runtime_error("Retail camera source was not a passive DX9 read");
    const auto lens = retail_preview_camera_detail::array<2>(source, "lens");
    const auto world = retail_preview_camera_detail::array<16>(source, "cameraWorld");
    const auto clipping = retail_preview_camera_detail::array<2>(source, "clipping");
    std::ifstream image(tga, std::ios::binary);
    std::array<std::uint8_t, 18> header{};
    if (!image || !image.read(reinterpret_cast<char *>(header.data()), header.size()) ||
        header[2] != 2 || header[16] != 32)
        throw std::runtime_error("Retail reference must be an uncompressed 32-bit TGA");
    const int width = header[12] | (int(header[13]) << 8);
    const int height = header[14] | (int(header[15]) << 8);
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
        std::abs(float(width) / float(height) - lens[1]) > 1e-6f ||
        lens[0] <= 0 || lens[0] >= 180 || clipping[0] <= 0 || clipping[1] <= clipping[0] ||
        world[15] != 1.f)
        throw std::runtime_error("Retail camera lens and TGA client raster disagree");
    RetailPreviewCamera result;
    result.width = width;
    result.height = height;
    result.camera = {lens[0], lens[1], clipping[0], clipping[1], world};
    return result;
}
