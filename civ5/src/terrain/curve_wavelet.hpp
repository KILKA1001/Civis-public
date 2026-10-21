#pragma once
#include "civ5/terrain/native_math.hpp"
#include <stdexcept>
#include <vector>

namespace civ5::terrain::curve_wavelet {
using Points = std::vector<Point2>;
inline Points reduce(const Points &p) {
    Points out{{p[0].x * 1.5f + p[1].x * -.5f, p[0].y * 1.5f + p[1].y * -.5f}};
    for (std::size_t i = 0; i < p.size() - 3; i += 2) {
        const auto a = p[i], b = p[i + 1], c = p[i + 2], d = p[i + 3];
        out.push_back({((a.x * -.25f + b.x * .75f) + c.x * .75f) + d.x * -.25f,
                       ((a.y * -.25f + b.y * .75f) + c.y * .75f) + d.y * -.25f});
    }
    const auto a = p[p.size() - 2], b = p.back();
    out.push_back({a.x * -.5f + b.x * 1.5f, a.y * -.5f + b.y * 1.5f});
    return out;
}
inline Points detail(const Points &p) {
    Points out;
    for (std::size_t i = 0; i < p.size() - 3; i += 2) {
        const auto a = p[i], b = p[i + 1], c = p[i + 2], d = p[i + 3];
        out.push_back({((a.x * .25f + b.x * -.75f) + c.x * .75f) + d.x * -.25f,
                       ((a.y * .25f + b.y * -.75f) + c.y * .75f) + d.y * -.25f});
    }
    return out;
}
inline Points reconstruct(const Points &coarse, const Points &details) {
    Points out{{details[0].x * -.25f, details[0].y * -.25f}, {details[0].x * -.75f, details[0].y * -.75f}};
    for (std::size_t i = 0; i + 1 < details.size(); ++i) {
        const auto a = details[i], b = details[i + 1];
        out.push_back({a.x * .75f + b.x * -.25f, a.y * .75f + b.y * -.25f});
        out.push_back({a.x * .25f + b.x * -.75f, a.y * .25f + b.y * -.75f});
    }
    out.push_back({details.back().x * .75f, details.back().y * .75f});
    out.push_back({details.back().x * .25f, details.back().y * .25f});
    if (out.size() != coarse.size() * 2 - 2)
        throw std::invalid_argument("Curve wavelet reconstruction size mismatch");
    for (std::size_t i = 0; i + 1 < coarse.size(); ++i) {
        const auto a = coarse[i], b = coarse[i + 1];
        out[i * 2] = {(b.x * .25f + a.x * .75f) + out[i * 2].x, (b.y * .25f + a.y * .75f) + out[i * 2].y};
        out[i * 2 + 1] = {(b.x * .75f + a.x * .25f) + out[i * 2 + 1].x, (b.y * .75f + a.y * .25f) + out[i * 2 + 1].y};
    }
    return out;
}
} // namespace civ5::terrain::curve_wavelet
