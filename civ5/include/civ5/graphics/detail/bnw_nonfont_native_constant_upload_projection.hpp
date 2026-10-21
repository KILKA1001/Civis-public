#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace civ5::graphics {
class GlBnwNonFontPipeline;

namespace detail {

// Numeric projection of the two saved constant-upload helpers. This value
// neither selects a current native table nor authorizes any memory/GL effect.
class BnwNonFontNativeConstantUploadProjection final {
public:
    BnwNonFontNativeConstantUploadProjection(
        const BnwNonFontNativeConstantUploadProjection&) = default;
    BnwNonFontNativeConstantUploadProjection(
        BnwNonFontNativeConstantUploadProjection&&) = default;
    ~BnwNonFontNativeConstantUploadProjection() = default;

private:
    friend class ::civ5::graphics::GlBnwNonFontPipeline;

    enum class Helper : std::uint8_t {
        full_8ef040,
        bounded_8ef0a0
    };
    struct UploadRange final {
        std::uint32_t device_vtable_byte_offset;
        std::uint32_t start_register;
        std::uint32_t source_block_offset;
        // Native32 SHL4 result only. Adding it to an actual retained data
        // address, checking complete safe extent and uploading are external.
        std::uint32_t source_byte_offset_native32;
        std::uint32_t block_count;
    };

    BnwNonFontNativeConstantUploadProjection() = default;
    static BnwNonFontNativeConstantUploadProjection decode_without_callbacks(
        const std::array<std::byte, 0x38>& original_owned_descriptor,
        std::uint32_t actual_byte_limit,
        bool actual_data_present) noexcept;

    // Preserve the entire owned descriptor, including opaque generation+8.
    // No native address is converted to a pointer or dereferenced here.
    std::array<std::byte, 0x38> descriptor_{};
    std::uint32_t raw_byte_limit_{};
    bool data_present_{};
    Helper helper_{Helper::full_8ef040};
    std::uint32_t capacity_bytes_native32_{};
    // Meaningful only for the bounded helper with present data. Native32
    // addition wraps BEFORE SHR4; this is not a safe memory extent.
    std::uint32_t rounded_bound_blocks_{};
    std::uint32_t helper_return_native32_{};
    // Only [0, upload_range_count_) contains active projected calls. Unused
    // value-initialized slots are not native ranges or upload defaults.
    std::array<UploadRange, 2> upload_ranges_{};
    std::size_t upload_range_count_{};
};

} // namespace detail
} // namespace civ5::graphics
