#include "civ5/terrain/native_math.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool finite(const Matrix4 &m) {
    return std::all_of(m.begin(), m.end(), [](float x) { return std::isfinite(x); });
}
Vector4 mul(const Vector4 &a, const Vector4 &b) {
    Vector4 r;
    for (int i = 0; i < 4; ++i)
        r[i] = a[i] * b[i];
    return r;
}
Vector4 add(const Vector4 &a, const Vector4 &b) {
    Vector4 r;
    for (int i = 0; i < 4; ++i)
        r[i] = a[i] + b[i];
    return r;
}
Vector4 sub(const Vector4 &a, const Vector4 &b) {
    Vector4 r;
    for (int i = 0; i < 4; ++i)
        r[i] = a[i] - b[i];
    return r;
}
Vector4 shuffle(const Vector4 &x, const Vector4 &y, int mask) {
    return {x[mask & 3], x[(mask >> 2) & 3], y[(mask >> 4) & 3], y[(mask >> 6) & 3]};
}
Vector4 shuffle(const Vector4 &x, int mask) { return shuffle(x, x, mask); }
} // namespace

Point2::Point2(float a, float b) : x(a), y(b) {
    require(std::isfinite(x) && std::isfinite(y), "Non-finite point");
}
Point2 normalize2_sse(Point2 v, const ReciprocalSqrtEstimate &estimate) {
    require(bool(estimate), "Missing RSQRTSS estimate provider");
    const float squared = v.x * v.x + v.y * v.y;
    require(std::isfinite(squared), "SSE Vec2 normalization overflow");
    if (squared < 0x1p-46f)
        return {0, 0};
    const float seed = estimate(squared);
    require(std::isfinite(seed) && seed > 0, "Invalid RSQRTSS estimate");
    const float product = squared * seed, correction = product * seed;
    const float factor = (.5f * seed) * (3.f - correction);
    return {factor * v.x, factor * v.y};
}
Vector3 normalize3_sse(Vector3 v, const ReciprocalSqrtEstimate &estimate) {
    require(bool(estimate), "Missing RSQRTSS estimate provider");
    require(std::all_of(v.begin(), v.end(), [](float f) { return std::isfinite(f); }),
            "Nonfinite SSE Vec3 normalization input");
    const float squared = (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
    require(std::isfinite(squared), "SSE Vec3 normalization overflow");
    if (squared < 0x1p-46f)
        return {0, 0, 0};
    const float seed = estimate(squared);
    require(std::isfinite(seed) && seed > 0, "Invalid RSQRTSS estimate");
    const float product = seed * squared, correction = product * seed;
    const float factor = (3.f - correction) * (seed * .5f);
    const Vector3 result{v[0] * factor, v[1] * factor, v[2] * factor};
    require(std::all_of(result.begin(), result.end(), [](float f) { return std::isfinite(f); }),
            "SSE Vec3 normalization output overflow");
    return result;
}
double round_x87(double value, X87Precision precision) {
    return precision == X87Precision::single ? static_cast<double>(static_cast<float>(value)) : value;
}
Matrix4 identity_matrix() {
    Matrix4 m{};
    for (int i = 0; i < 16; i += 5)
        m[i] = 1.0f;
    return m;
}
IntMatrix4 unshifted_global_map_matrix(const FactoryDescriptor &factory) {
    require(std::all_of(factory.begin(), factory.end(), [](auto x) { return x == 0; }),
            "Unverified global factory descriptor");
    return {0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 1};
}
Matrix4 matrix_multiply(const Matrix4 &a, const Matrix4 &b, X87Precision pc, MatrixPath path) {
    require(finite(a) && finite(b), "Non-finite matrix input");
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        const int row = i / 4, column = i % 4;
        if (path == MatrixPath::sse) {
            Vector4 p;
            for (int k = 0; k < 4; ++k)
                p[k] = a[row * 4 + k] * b[k * 4 + column];
            result[i] =
                (row == 0 || row == 3) ? (p[0] + p[1]) + (p[2] + p[3]) : ((p[0] + p[1]) + p[2]) + p[3];
        } else {
            std::array<double, 4> p;
            for (int k = 0; k < 4; ++k)
                p[k] = round_x87(static_cast<double>(a[row * 4 + k]) * b[k * 4 + column], pc);
            result[i] = static_cast<float>(
                path == MatrixPath::dispatch_x87
                    ? round_x87(round_x87(p[0] + p[2], pc) + round_x87(p[1] + p[3], pc), pc)
                    : round_x87(round_x87(round_x87(p[0] + p[1], pc) + p[2], pc) + p[3], pc));
        }
    }
    return result;
}
Vector4 vector4_transform(const Vector4 &v, const Matrix4 &matrix, X87Precision pc, Vec4Path path) {
    Vector4 result;
    for (int c = 0; c < 4; ++c) {
        if (path == Vec4Path::sse) {
            const float a = v[0] * matrix[c], b = v[1] * matrix[4 + c], d = v[3] * matrix[12 + c],
                        z = v[2] * matrix[8 + c];
            result[c] = (d + z) + (b + a);
        } else {
            std::array<double, 4> p;
            for (int k = 0; k < 4; ++k)
                p[k] = round_x87(static_cast<double>(v[k]) * matrix[k * 4 + c], pc);
            const double first = round_x87(c == 0 ? p[1] + p[0] : p[0] + p[1], pc);
            result[c] = static_cast<float>(round_x87(round_x87(first + p[2], pc) + p[3], pc));
        }
    }
    return result;
}

Matrix4 inverse_sse(const Matrix4 &a, const ReciprocalEstimate &estimate) {
    require(finite(a), "Non-finite SSE inverse input");
    require(static_cast<bool>(estimate), "Missing RCPSS provider");
    Vector4 x1{a[0], a[1], a[4], a[5]}, x2{a[2], a[3], a[6], a[7]};
    Vector4 x3{a[8], a[9], a[12], a[13]}, x4{a[10], a[11], a[14], a[15]};
    auto x5 = shuffle(x2, x4, 0x88);
    x4 = shuffle(x4, x2, 0xdd);
    x2 = shuffle(mul(x4, x5), 0xb1);
    auto x6 = shuffle(x2, 0x4e);
    auto x7 = x3;
    x3 = shuffle(x3, x1, 0xdd);
    x1 = shuffle(x1, x7, 0x88);
    x7 = x3;
    x3 = mul(x3, x6);
    x6 = mul(x6, x1);
    const auto d0 = x6;
    x6 = x7;
    x7 = mul(x7, x2);
    x2 = mul(x2, x1);
    x3 = sub(x3, x7);
    x7 = mul(x6, x5);
    x5 = shuffle(x5, 0x4e);
    x7 = shuffle(x7, 0xb1);
    const auto c0 = x2;
    x2 = add(mul(x4, x7), x3);
    x3 = x7;
    x7 = shuffle(x7, 0x4e);
    x3 = mul(x3, x1);
    const auto b0 = x3;
    x3 = mul(x4, x7);
    x7 = mul(x7, x1);
    x2 = sub(x2, x3);
    x3 = shuffle(mul(shuffle(x6, 0x4e), x4), 0xb1);
    const auto a0 = x7;
    x7 = x5;
    x5 = add(mul(x5, x3), x2);
    x2 = x3;
    x3 = shuffle(x3, 0x4e);
    x2 = mul(x2, x1);
    const auto r90 = x4;
    x4 = x7;
    x7 = mul(x7, x3);
    x3 = mul(x3, x1);
    x5 = sub(x5, x7);
    x3 = sub(x3, x2);
    x2 = x1;
    x1 = mul(x1, x5);
    x3 = shuffle(x3, 0x4e);
    x7 = x1;
    x1 = shuffle(x1, 0x4e);
    const auto e0 = x5;
    x1 = add(x1, x7);
    x5 = x1;
    x1 = shuffle(x1, 0xb1);
    const float determinant = x1[0] + x5[0];
    x5 = shuffle(mul(x6, x2), 0xb1);
    x7 = x5;
    x5 = shuffle(x5, 0x4e);
    const auto r80 = x4;
    x4 = r90;
    const auto r70 = x6;
    x6 = add(mul(x4, x7), x3);
    x3 = sub(mul(x4, x5), x6);
    x6 = shuffle(mul(x4, x2), 0xb1);
    const auto r60 = x5;
    x5 = r70;
    const auto r50 = x7;
    x7 = add(mul(x6, x5), x3);
    x3 = shuffle(x6, 0x4e);
    const auto r40 = x4;
    x4 = x5;
    x5 = mul(x5, x3);
    const auto r30 = x4;
    x4 = x6;
    x6 = sub(x7, x5);
    x5 = shuffle(sub(d0, c0), 0x4e);
    x7 = r80;
    x4 = mul(x4, x7);
    x3 = mul(x3, x7);
    x5 = sub(x5, x4);
    x2 = mul(x2, x7);
    x3 = add(x3, x5);
    x2 = shuffle(x2, 0xb1);
    x4 = shuffle(x2, 0x4e);
    x5 = r40;
    const auto r20 = x6;
    x6 = x5;
    x5 = mul(x5, x2);
    x6 = mul(x6, x4);
    x5 = add(x5, x3);
    x3 = x4;
    x4 = sub(x5, x6);
    x5 = shuffle(sub(a0, b0), 0x4e);
    x6 = sub(mul(r50, x7), x5);
    x5 = r60;
    x7 = mul(x7, x5);
    x6 = sub(x6, x7);
    x5 = r30;
    x2 = mul(x2, x5);
    x5 = mul(x5, x3);
    x6 = sub(x6, x2);
    x2 = add(x5, x6);
    x6 = r20;
    require(std::isfinite(determinant) && determinant != 0.0f, "Unsupported singular SSE inverse");
    const float seed = estimate(determinant);
    require(std::isfinite(seed), "Unprovided finite RCPSS estimate");
    const float square = seed * seed, correction = determinant * square, twice = seed + seed;
    const float reciprocal = twice - correction;
    const Vector4 factor{reciprocal, reciprocal, reciprocal, reciprocal};
    const std::array<Vector4, 4> rows{mul(e0, factor), mul(x4, factor), mul(x6, factor), mul(factor, x2)};
    Matrix4 result;
    for (int row = 0; row < 4; ++row)
        std::copy(rows[row].begin(), rows[row].end(), result.begin() + row * 4);
    require(finite(result), "Unsupported SSE inverse overflow");
    return result;
}
} // namespace civ5::terrain
