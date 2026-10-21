#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <functional>

namespace civ5::terrain {
static_assert(sizeof(void *) == 8, "Civ V runtime must be 64-bit");
using Matrix4 = std::array<float, 16>;
using IntMatrix4 = std::array<std::int32_t, 16>;
using Vector4 = std::array<float, 4>;
using Vector3 = std::array<float, 3>;
using FactoryDescriptor = std::array<std::int32_t, 5>;
using ReciprocalEstimate = std::function<float(float)>;
using ReciprocalSqrtEstimate = std::function<float(float)>;
enum class X87Precision { single, double_precision };
enum class MatrixPath { reference_x87, dispatch_x87, sse };
enum class Vec4Path { legacy_x87, sse };

struct Point2 {
    float x, y;
    Point2(float x, float y);
    bool operator==(const Point2 &) const = default;
};
struct LogicalHex {
    std::int32_t x, y, z;
    bool operator==(const LogicalHex &) const = default;
};

// Original IMUL/ADD/NEG wrap to 32 bits, even in the 64-bit engine. Never
// reproduce them with signed C++ overflow (undefined behavior).
inline std::int32_t wrap_add(std::int32_t a, std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a) + std::bit_cast<std::uint32_t>(b));
}
inline std::int32_t wrap_mul(std::int32_t a, std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a) * std::bit_cast<std::uint32_t>(b));
}
inline std::int32_t wrap_neg(std::int32_t a) {
    return std::bit_cast<std::int32_t>(0u - std::bit_cast<std::uint32_t>(a));
}
double round_x87(double value, X87Precision precision);
Matrix4 identity_matrix();
IntMatrix4 unshifted_global_map_matrix(const FactoryDescriptor &factory);
Matrix4 matrix_multiply(const Matrix4 &a, const Matrix4 &b, X87Precision precision, MatrixPath path);
Vector4 vector4_transform(const Vector4 &v, const Matrix4 &matrix, X87Precision precision, Vec4Path path);
// Original SSE inverse/refinement ordering. The caller supplies RCPSS state;
// division, an assumed processor estimate or a fitted matrix is not a default.
Matrix4 inverse_sse(const Matrix4 &matrix, const ReciprocalEstimate &reciprocal_estimate);
// Original D3DX SSE normalization, with explicit RSQRTSS instruction state.
// Generic division/sqrt is not a substitute for the processor estimate.
Point2 normalize2_sse(Point2 value, const ReciprocalSqrtEstimate &estimate);
Vector3 normalize3_sse(Vector3 value, const ReciprocalSqrtEstimate &estimate);
} // namespace civ5::terrain
