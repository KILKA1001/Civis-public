#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace civ5::graphics {
// Native 608D40/88FD30 table keys are signed full IDs, independent of the serial
// registry's owner<<24 | (unitId&FFFFFF) representation. This readonly utility
// accepts immutable caller-supplied data; it does not create units or placements.
struct BnwNativeUnitIdentityRow {
    std::int32_t owner;
    std::int32_t unit_id;
    std::uint32_t pool_index;
    friend bool operator==(const BnwNativeUnitIdentityRow &, const BnwNativeUnitIdentityRow &) = default;
};
inline constexpr std::size_t bnw_native_unit_identity_row_bytes=12;
// A capture reader limit, not the native game's capacity.
inline constexpr std::size_t bnw_native_unit_identity_reader_limit=16384;

inline auto bnw_native_unit_identity_key(const BnwNativeUnitIdentityRow &row) noexcept {
    return std::pair{row.owner,row.unit_id};
}

inline void validate_bnw_native_unit_identity_rows(
    std::span<const BnwNativeUnitIdentityRow> rows,std::uint32_t same_frame_pool_count) {
    if(rows.size()>bnw_native_unit_identity_reader_limit ||
       same_frame_pool_count>bnw_native_unit_identity_reader_limit)
        throw std::invalid_argument("BNW native identity capture exceeds reader bound");
    for(std::size_t i=0;i<rows.size();++i) {
        if(rows[i].pool_index>=same_frame_pool_count)
            throw std::invalid_argument("BNW native identity index outside same-frame pool");
        if(i && !(bnw_native_unit_identity_key(rows[i-1])<bnw_native_unit_identity_key(rows[i])))
            throw std::invalid_argument("BNW native identity keys unsorted or duplicate");
    }
}

inline std::vector<BnwNativeUnitIdentityRow> decode_bnw_native_unit_identity_rows(
    std::span<const std::byte> bytes,std::uint32_t same_frame_pool_count) {
    if(bytes.size()%bnw_native_unit_identity_row_bytes ||
       bytes.size()/bnw_native_unit_identity_row_bytes>bnw_native_unit_identity_reader_limit)
        throw std::invalid_argument("BNW native identity byte count invalid");
    const auto u32=[&bytes](std::size_t offset) {
        std::uint32_t value=0;
        for(std::size_t i=0;i<4;++i) value|=std::to_integer<std::uint32_t>(bytes[offset+i])<<(i*8);
        return value;
    };
    std::vector<BnwNativeUnitIdentityRow> rows;
    rows.reserve(bytes.size()/bnw_native_unit_identity_row_bytes);
    for(std::size_t offset=0;offset<bytes.size();offset+=bnw_native_unit_identity_row_bytes)
        rows.push_back({std::bit_cast<std::int32_t>(u32(offset)),
                        std::bit_cast<std::int32_t>(u32(offset+4)),u32(offset+8)});
    validate_bnw_native_unit_identity_rows(rows,same_frame_pool_count);
    return rows;
}

inline std::optional<std::uint32_t> lookup_bnw_native_unit_pool_index(
    std::span<const BnwNativeUnitIdentityRow> rows,std::uint32_t same_frame_pool_count,
    std::int32_t owner,std::int32_t unit_id) {
    validate_bnw_native_unit_identity_rows(rows,same_frame_pool_count);
    const auto target=std::pair{owner,unit_id};
    std::size_t low=0,high=rows.size();
    while(low<high) {
        const auto middle=low+(high-low)/2;
        if(bnw_native_unit_identity_key(rows[middle])<target) low=middle+1;
        else high=middle;
    }
    if(low==rows.size() || bnw_native_unit_identity_key(rows[low])!=target) return std::nullopt;
    return rows[low].pool_index;
}
} // namespace civ5::graphics
