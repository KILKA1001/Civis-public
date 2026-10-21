#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace civ5::graphics {
class GlBnwNonFontPipeline;

namespace detail {

// A private byte-field projection, not a renderer, queue permission or completion.
// The pipeline must earn the SAME selected Item/Material/Drain association before
// calling this reader. Native tables and the GL codec are separate suppliers.
class BnwNonFontNativeRecordProjection final {
public:
    BnwNonFontNativeRecordProjection(const BnwNonFontNativeRecordProjection&) = default;
    BnwNonFontNativeRecordProjection(BnwNonFontNativeRecordProjection&&) = default;
    ~BnwNonFontNativeRecordProjection() = default;

private:
    friend class ::civ5::graphics::GlBnwNonFontPipeline;

    BnwNonFontNativeRecordProjection() = default;
    static BnwNonFontNativeRecordProjection decode_without_callbacks(
        const std::array<std::byte, 0xF0>& original_payload,
        const std::bitset<0xF0>& original_write_coverage,
        const std::array<std::byte, 0x64>& original_fixed_descriptor,
        const std::array<std::byte, 0x20>& original_registration);

    // Retain bytes and coverage exactly. Unwritten payload bytes remain unearned;
    // this reader does not give them a zero/default value or decode them as vertices.
    std::array<std::byte, 0xF0> payload_{};
    std::bitset<0xF0> payload_write_coverage_{};
    std::array<std::byte, 0x64> fixed_descriptor_{};
    std::array<std::byte, 0x20> registration_{};

    std::uint32_t registration_dword00_index_{};
    std::uint16_t registration_word08_subselector_{};
    std::uint32_t registration_dword10_pipeline_selector_{};
    // Opaque original 32-bit address; never convert it to or dereference a pointer.
    std::uint32_t registration_dword14_payload_address_{};
    std::uint16_t registration_word18_payload_size_{};

    // Neutral offsets preserve native meaning until selected table/ABI suppliers
    // establish declaration, stream, index and draw domains. DWORD00=0x41 is not
    // a shader selector, a vertex format or a topology declaration.
    std::uint32_t descriptor_dword00_{};
    std::uint32_t descriptor_dword04_{};
    std::uint32_t descriptor_dword08_{};
    std::uint32_t descriptor_dword0c_{};
    std::uint32_t descriptor_dword10_{};
    std::uint32_t descriptor_dword14_{};
    std::uint8_t primary_stage_count_{};
    std::uint8_t secondary_stage_count_{};
    std::array<std::uint16_t, 4> primary_resource_slots_{};
    std::array<std::uint16_t, 4> primary_state_slots_{};
};

} // namespace detail
} // namespace civ5::graphics
