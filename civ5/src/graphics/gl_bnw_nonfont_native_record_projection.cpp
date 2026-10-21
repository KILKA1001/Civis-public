#include "civ5/graphics/detail/bnw_nonfont_native_record_projection.hpp"

#include <stdexcept>

namespace civ5::graphics::detail {
namespace {

template <std::size_t Size>
std::uint16_t read_word(const std::array<std::byte, Size>& bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        std::to_integer<std::uint32_t>(bytes[offset]) |
        (std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8));
}

template <std::size_t Size>
std::uint32_t read_dword(const std::array<std::byte, Size>& bytes,
                         const std::size_t offset) noexcept {
    return std::to_integer<std::uint32_t>(bytes[offset]) |
        (std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8) |
        (std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16) |
        (std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

BnwNonFontNativeRecordProjection
BnwNonFontNativeRecordProjection::decode_without_callbacks(
    const std::array<std::byte, 0xF0>& original_payload,
    const std::bitset<0xF0>& original_write_coverage,
    const std::array<std::byte, 0x64>& original_fixed_descriptor,
    const std::array<std::byte, 0x20>& original_registration) {
    // These are the issued ordinary Button stores, not a generic native decoder.
    // 8EC210 selects the simple manager branch with registration DWORD0C=0.
    if (read_dword(original_registration, 0x04) != 0xFFFFFFFFu ||
        read_dword(original_registration, 0x0C) != 0u ||
        read_word(original_registration, 0x18) != 0xF0u ||
        read_dword(original_fixed_descriptor, 0x00) != 0x41u ||
        read_dword(original_fixed_descriptor, 0x0C) != 2u) {
        throw std::logic_error("nonFont native projection: unsupported issued record");
    }
    const auto primary_count =
        std::to_integer<std::uint8_t>(original_fixed_descriptor[0x60]);
    const auto secondary_count =
        std::to_integer<std::uint8_t>(original_fixed_descriptor[0x61]);
    if ((primary_count != 2u && primary_count != 4u) || secondary_count != 0u) {
        throw std::logic_error("nonFont native projection: unsupported stage fields");
    }

    BnwNonFontNativeRecordProjection result;
    result.payload_ = original_payload;
    result.payload_write_coverage_ = original_write_coverage;
    result.fixed_descriptor_ = original_fixed_descriptor;
    result.registration_ = original_registration;
    result.registration_dword00_index_ = read_dword(original_registration, 0x00);
    result.registration_word08_subselector_ = read_word(original_registration, 0x08);
    result.registration_dword10_pipeline_selector_ = read_dword(original_registration, 0x10);
    result.registration_dword14_payload_address_ = read_dword(original_registration, 0x14);
    result.registration_word18_payload_size_ = read_word(original_registration, 0x18);
    result.descriptor_dword00_ = read_dword(original_fixed_descriptor, 0x00);
    result.descriptor_dword04_ = read_dword(original_fixed_descriptor, 0x04);
    result.descriptor_dword08_ = read_dword(original_fixed_descriptor, 0x08);
    result.descriptor_dword0c_ = read_dword(original_fixed_descriptor, 0x0C);
    result.descriptor_dword10_ = read_dword(original_fixed_descriptor, 0x10);
    result.descriptor_dword14_ = read_dword(original_fixed_descriptor, 0x14);
    result.primary_stage_count_ = primary_count;
    result.secondary_stage_count_ = secondary_count;
    for (std::size_t stage = 0; stage < primary_count; ++stage) {
        result.primary_resource_slots_[stage] =
            read_word(original_fixed_descriptor, 0x18 + stage * 2);
        result.primary_state_slots_[stage] =
            read_word(original_fixed_descriptor, 0x38 + stage * 2);
    }
    // No saved-address dereference, table lookup, GL effect, callback or positive
    // completion occurs. The selected wrapper must retain original sealed owners.
    return result;
}

} // namespace civ5::graphics::detail
