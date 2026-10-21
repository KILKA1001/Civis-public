#include "civ5/graphics/detail/bnw_nonfont_native_constant_upload_projection.hpp"

#include <algorithm>

namespace civ5::graphics::detail {
namespace {

std::uint32_t read_dword(const std::array<std::byte, 0x38>& bytes,
                         const std::size_t offset) noexcept {
    return std::to_integer<std::uint32_t>(bytes[offset]) |
        (std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8) |
        (std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16) |
        (std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint32_t shift_blocks_native32(const std::uint32_t blocks) noexcept {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(blocks) << 4);
}

} // namespace

BnwNonFontNativeConstantUploadProjection
BnwNonFontNativeConstantUploadProjection::decode_without_callbacks(
    const std::array<std::byte, 0x38>& original_owned_descriptor,
    const std::uint32_t actual_byte_limit,
    const bool actual_data_present) noexcept {
    BnwNonFontNativeConstantUploadProjection result;
    result.descriptor_ = original_owned_descriptor;
    result.raw_byte_limit_ = actual_byte_limit;
    result.data_present_ = actual_data_present;
    result.capacity_bytes_native32_ =
        shift_blocks_native32(read_dword(result.descriptor_, 0x10));

    // 8E2703..8E2717: unsigned caller choice after native32 SHL4.
    const bool bounded = actual_byte_limit != 0u &&
        actual_byte_limit < result.capacity_bytes_native32_;
    result.helper_ = bounded ? Helper::bounded_8ef0a0 : Helper::full_8ef040;

    // 8EF0A0 returns zero on NULL before borrowing any helper count/start.
    // The caller's choice still used descriptor+10 above. The complete owned
    // image is metadata here, not an actual native descriptor dereference.
    if (bounded && !actual_data_present) {
        return result;
    }

    const auto first_count = read_dword(result.descriptor_, 0x1C);
    const auto second_count = read_dword(result.descriptor_, 0x28);
    if (!bounded) {
        // Full helper returns the native32 sum even with NULL/no upload.
        // Neither device HRESULT nor the number of calls affects this sum.
        result.helper_return_native32_ = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(first_count) + second_count);
        if (!actual_data_present) {
            return result;
        }
        if (first_count != 0u) {
            result.upload_ranges_[result.upload_range_count_++] = {
                0x178u, read_dword(result.descriptor_, 0x18), 0u, 0u, first_count};
        }
        if (second_count != 0u) {
            const auto source_blocks = read_dword(result.descriptor_, 0x20);
            result.upload_ranges_[result.upload_range_count_++] = {
                0x1B4u, read_dword(result.descriptor_, 0x24), source_blocks,
                shift_blocks_native32(source_blocks), second_count};
        }
        return result;
    }

    // 8EF0BC..8EF0C8: wrap the original uint32 byteLimit+15, then logical
    // SHR4 and unsigned min. Do not replace this with wide ceiling division.
    const auto rounded_native32 = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(actual_byte_limit) + 15u);
    const auto rounded_blocks = rounded_native32 >> 4;
    result.rounded_bound_blocks_ = rounded_blocks;
    const auto first_bounded_count = std::min(rounded_blocks, first_count);
    // Bounded return is EBX (first stage only), even when stage two uploads.
    result.helper_return_native32_ = first_bounded_count;
    if (first_bounded_count != 0u) {
        result.upload_ranges_[result.upload_range_count_++] = {
            0x178u, read_dword(result.descriptor_, 0x18), 0u, 0u,
            first_bounded_count};
    }
    if (second_count != 0u) {
        const auto source_blocks = read_dword(result.descriptor_, 0x20);
        // 8EF0EE..8EF0FC: unsigned rounded total must exceed source offset.
        if (rounded_blocks > source_blocks) {
            const auto remaining_blocks = rounded_blocks - source_blocks;
            const auto second_bounded_count = std::min(remaining_blocks, second_count);
            if (second_bounded_count != 0u) {
                result.upload_ranges_[result.upload_range_count_++] = {
                    0x1B4u, read_dword(result.descriptor_, 0x24), source_blocks,
                    shift_blocks_native32(source_blocks), second_bounded_count};
            }
        }
    }
    // At most the original first and second virtual-upload ranges, in order.
    // These are numbers only: no device/table access, payload read, cache skip,
    // GL upload, shader mapping, memory readiness or completion is issued.
    return result;
}

} // namespace civ5::graphics::detail
