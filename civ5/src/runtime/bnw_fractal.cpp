#include "civ5/runtime/bnw_fractal.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr int scale = 1000;
int clamp(int value, int low, int high) { return std::clamp(value, low, high); }
int hex_x(int x, int y) { return x - y / 2; } // Ridge coordinates have y >= 0.
int hex_distance(int dx, int dy) {
    const int ax = std::abs(dx), ay = std::abs(dy);
    return (dx >= 0) == (dy >= 0) ? ax + ay : std::max(ax, ay);
}
int estimate_direction(int dx, int dy) {
    // CvGameCoreUtils.cpp duplicates southwest at index five. Preserve that
    // shipped direction table, including strict > tie handling.
    constexpr std::array<std::array<double,2>,6> directions{{
        {0.5,0.866025}, {1.0,0.0}, {0.5,-0.866025},
        {-0.5,-0.866025}, {-1.0,0.0}, {-0.5,-0.866025}
    }};
    double maximum = 0;
    int selected = -1;
    for (int i = 0; i < 6; ++i) {
        const double dot = dx * directions[i][0] + dy * directions[i][1];
        if (dot > maximum) { maximum = dot; selected = i; }
    }
    return selected;
}
struct RidgeSeed {
    int x, y, weakness, direction, strength;
};
}

BnwFractal::BnwFractal(int width, int height, int grain, std::uint32_t flags,
                       int x_exponent, int y_exponent, SimulationRandom &map_random)
    : width_(width), height_(height), frac_x_(0), frac_y_(0), x_inc_(0),
      y_inc_(0), flags_(flags) {
    if (width <= 0 || height <= 0 || width >= 255 || height >= 255 ||
        grain < -128 || grain > 128 ||
        map_random.algorithm() != RandomAlgorithm::bnw)
        throw std::invalid_argument("BNW fractal dimensions or map RNG are invalid");
    if (x_exponent < 0) x_exponent = 7;
    if (y_exponent < 0) y_exponent = 6;
    // The SDK's fixed [256][256] buffer cannot safely hold the 257 entries
    // needed by exponent 8. Constrain this source-port to defined cases.
    if (x_exponent > 7 || y_exponent > 7 ||
        (flags & ~(wrap_x | wrap_y | percent | polar | center_rift | invert_heights)))
        throw std::invalid_argument("BNW fractal exponent or flags exceed the verified SDK path");
    frac_x_ = 1 << x_exponent;
    frac_y_ = 1 << y_exponent;
    values_.resize(static_cast<std::size_t>(frac_x_ + 1) * (frac_y_ + 1));
    x_inc_ = frac_x_ * scale / width_;
    y_inc_ = frac_y_ * scale / height_;
    const int smooth = clamp(std::min(x_exponent, y_exponent) - grain,
                             0, std::min(x_exponent, y_exponent));
    for (int pass = smooth; pass >= 0; --pass) {
        const int screen = (1 << (pass + 1)) - 1;
        if (flags_ & wrap_y)
            for (int x = 0; x <= frac_x_; ++x) at(x, frac_y_) = at(x, 0);
        else if (flags_ & polar)
            for (int x = 0; x <= frac_x_; ++x) at(x, 0) = at(x, frac_y_) = 0;
        if (flags_ & wrap_x)
            for (int y = 0; y <= frac_y_; ++y) at(frac_x_, y) = at(0, y);
        else if (flags_ & polar)
            for (int y = 0; y <= frac_y_; ++y) at(0, y) = at(frac_x_, y) = 0;
        if (flags_ & center_rift) {
            if (flags_ & wrap_y)
                for (int x = 0; x <= frac_x_; ++x)
                    for (int y = 0; y < frac_y_ / 6; ++y) {
                        const int divisor = std::abs(frac_y_ / 12 - y) + 1;
                        at(x, y) /= divisor;
                        at(x, frac_y_ / 2 + y) /= divisor;
                    }
            if (flags_ & wrap_x)
                for (int y = 0; y <= frac_y_; ++y)
                    for (int x = 0; x < frac_x_ / 6; ++x) {
                        const int divisor = std::abs(frac_x_ / 12 - x) + 1;
                        at(x, y) /= divisor;
                        at(frac_x_ / 2 + x, y) /= divisor;
                    }
        }
        for (int x = 0; x < (frac_x_ >> pass) + ((flags_ & wrap_x) ? 0 : 1); ++x)
            for (int y = 0; y < (frac_y_ >> pass) + ((flags_ & wrap_y) ? 0 : 1); ++y) {
                const int gx = x << pass, gy = y << pass;
                if (pass == smooth) {
                    at(gx, gy) = static_cast<int>(map_random.draw(256));
                    continue;
                }
                int sum = 0;
                if (gx & screen) {
                    if (gy & screen) {
                        sum = (at((x-1)<<pass,(y-1)<<pass) +
                               at((x+1)<<pass,(y-1)<<pass) +
                               at((x-1)<<pass,(y+1)<<pass) +
                               at((x+1)<<pass,(y+1)<<pass)) >> 2;
                    } else {
                        sum = (at((x-1)<<pass,gy) + at((x+1)<<pass,gy)) >> 1;
                    }
                } else if (gy & screen) {
                    sum = (at(gx,(y-1)<<pass) + at(gx,(y+1)<<pass)) >> 1;
                } else continue;
                sum += static_cast<int>(map_random.draw(1 << (8 - smooth + pass)));
                sum -= 1 << (7 - smooth + pass);
                at(gx, gy) = clamp(sum, 0, 255);
            }
    }
    if (flags_ & invert_heights)
        for (int x = 0; x < frac_x_; ++x)
            for (int y = 0; y < frac_y_; ++y) at(x, y) = 255 - at(x, y);
}

int BnwFractal::height_at(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0;
    const int low_x = std::min(x_inc_ * x / scale, frac_x_ - 1);
    const int low_y = std::min(y_inc_ * y / scale, frac_y_ - 1);
    const int error_x = x_inc_ * x - low_x * scale;
    const int error_y = y_inc_ * y - low_y * scale;
    int sum = (scale-error_x) * (scale-error_y) * at(low_x,low_y);
    sum += error_x * (scale-error_y) * at(low_x+1,low_y);
    sum += (scale-error_x) * error_y * at(low_x,low_y+1);
    sum += error_x * error_y * at(low_x+1,low_y+1);
    const int height = clamp(sum / (scale * scale), 0, 255);
    return (flags_ & percent) ? (height * 100 >> 8) : height;
}

int BnwFractal::height_from_percent(int percentage) const {
    percentage = clamp(percentage, 0, 100);
    int lower = 0, upper = 255;
    int estimate = 255 * percentage / 100;
    while (estimate != lower) {
        int below = 0;
        for (int x = 0; x < frac_x_; ++x)
            for (int y = 0; y < frac_y_; ++y)
                below += at(x,y) < estimate;
        if ((100 * below / (frac_x_ * frac_y_)) > percentage)
            upper = estimate;
        else lower = estimate;
        estimate = (upper + lower) / 2;
    }
    return estimate;
}

void BnwFractal::build_ridges(int seeds, std::uint32_t flags, int ridge_blend,
                              int fractal_blend, SimulationRandom &map_random,
                              std::uint64_t maximum_seeds, std::uint64_t maximum_retries) {
    seeds = std::max(seeds, 3);
    if (map_random.algorithm() != RandomAlgorithm::bnw ||
        static_cast<std::uint64_t>(seeds) > maximum_seeds || seeds > 256 ||
        ridge_blend < 0 || fractal_blend < 0 ||
        ridge_blend > 1000 || fractal_blend > 1000)
        throw std::invalid_argument("CIVIS_BNW_FRACTAL_BUDGET:ridge request exceeds verified bounds");
    std::vector<RidgeSeed> points;
    points.reserve(static_cast<std::size_t>(seeds));
    std::uint64_t retries = 0;
    for (int i = 0; i < seeds; ++i) {
        RidgeSeed point{};
        point.y = static_cast<int>(map_random.draw(frac_y_));
        point.x = hex_x(static_cast<int>(map_random.draw(frac_x_)), point.y);
        point.weakness = std::max(0, static_cast<int>(map_random.draw(7)) - 3);
        point.direction = static_cast<int>(map_random.draw(6));
        point.strength = std::max(0, static_cast<int>(map_random.draw(8)) - 4);
        bool recheck;
        do {
            recheck = false;
            for (int j = 0; j < i; ++j)
                if (hex_distance(point.x - points[j].x,
                                 point.y - points[j].y) < 7) {
                    if (retries >= maximum_retries)
                        throw std::runtime_error("CIVIS_BNW_FRACTAL_BUDGET:ridge seed retries exhausted");
                    ++retries;
                    // The SDK reroll keeps raw X instead of converting to
                    // hex-space again; this affects both placement and RNG.
                    point.x = static_cast<int>(map_random.draw(frac_x_));
                    point.y = static_cast<int>(map_random.draw(frac_y_));
                    recheck = true;
                    break;
                }
        } while (recheck);
        points.push_back(point);
    }
    for (int x = 0; x < frac_x_; ++x)
        for (int y = 0; y < frac_y_; ++y) {
            const int hx = hex_x(x,y);
            int closest = std::numeric_limits<int>::max();
            int second = std::numeric_limits<int>::max();
            for (const auto &point : points) {
                int distance = hex_distance(hx - point.x, y - point.y);
                if (flags) {
                    distance += point.weakness;
                    distance += static_cast<int>(map_random.draw(3));
                    const int direction = estimate_direction(point.x-hx, point.y-y);
                    if (direction == point.direction) distance -= point.strength;
                    else if (direction == (point.direction + 3) % 6)
                        distance += point.strength;
                    distance = std::max(1,distance);
                }
                if (distance < closest) { second = closest; closest = distance; }
                else if (distance < second) second = distance;
            }
            if (second <= 0 || second == std::numeric_limits<int>::max())
                throw std::runtime_error("CIVIS_BNW_FRACTAL_BUDGET:ridge geometry is degenerate");
            const int ridge_height = 255 * closest / second;
            at(x,y) = (ridge_height * ridge_blend + at(x,y) * fractal_blend) /
                      std::max(ridge_blend + fractal_blend, 1);
        }
}

} // namespace civ5::runtime
