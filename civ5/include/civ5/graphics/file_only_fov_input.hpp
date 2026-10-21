// Staged own C++ mathematical port. Never calls an original instruction.
// Input is the exact pinned original FILE, borrowed immutable through this
// synchronous call. This does not read or authenticate loaded original memory.
#pragma once
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::graphics::file_only_fov {
inline constexpr std::string_view original_exe_sha256 =
    "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
enum class Refusal { wrong_pinned_file };
class InputError : public std::runtime_error {
  public:
    const Refusal refusal=Refusal::wrong_pinned_file;
    InputError():std::runtime_error("Exact pinned original file bytes required; no current lens authority") {}
};
class Quote {
    std::uint32_t bits_;
    explicit Quote(std::uint32_t bits):bits_(bits) {}
    friend Quote calculate(std::span<const std::uint8_t>);
  public:
    std::uint32_t file_crc_bits()const noexcept{return bits_;}
    float file_ieee_float()const noexcept{return std::bit_cast<float>(bits_);}
    static constexpr bool loaded_memory_observed=false;
    static constexpr bool native_current_fov_issued=false;
    static constexpr bool native_lens_issuer_ready=false;
    static constexpr bool changed_extent_drawable=false;
    static constexpr std::uint32_t source_preferred_base=0x400000;
    static constexpr std::uint32_t source_adapter_return_rva=0x1a8507;
    static constexpr std::size_t source_ordered_bytes=616756;
};
namespace detail {
struct Range {std::size_t file_offset,bytes;};
inline constexpr std::array<Range,87> ordered_ranges{{
    {330752, 4096}, // source index0 VA0x451800
    {2302976, 2048}, // source index1 VA0x633000
    {1734912, 16}, // source index2 VA0x5a8500
    {3513344, 8192}, // source index3 VA0x75a800
    {3509248, 8192}, // source index4 VA0x759800
    {3507200, 6144}, // source index5 VA0x759000
    {3496960, 6144}, // source index6 VA0x756800
    {3492864, 8192}, // source index7 VA0x755800
    {3490816, 6144}, // source index8 VA0x755000
    {3480576, 6144}, // source index9 VA0x752800
    {3476480, 8192}, // source index10 VA0x751800
    {3474432, 6144}, // source index11 VA0x751000
    {3460096, 6144}, // source index12 VA0x74d800
    {3456000, 8192}, // source index13 VA0x74c800
    {3451904, 8192}, // source index14 VA0x74b800
    {3447808, 8192}, // source index15 VA0x74a800
    {3445760, 6144}, // source index16 VA0x74a000
    {3435520, 6144}, // source index17 VA0x747800
    {3431424, 8192}, // source index18 VA0x746800
    {3427328, 8192}, // source index19 VA0x745800
    {3423232, 8192}, // source index20 VA0x744800
    {3419136, 8192}, // source index21 VA0x743800
    {3417088, 6144}, // source index22 VA0x743000
    {3402752, 6144}, // source index23 VA0x73f800
    {3398656, 8192}, // source index24 VA0x73e800
    {3394560, 8192}, // source index25 VA0x73d800
    {3390464, 8192}, // source index26 VA0x73c800
    {3386368, 8192}, // source index27 VA0x73b800
    {3382272, 8192}, // source index28 VA0x73a800
    {6901760, 12580}, // source index29 VA0xa96000
    {3517440, 8192}, // source index30 VA0x75b800
    {3521536, 8192}, // source index31 VA0x75c800
    {3525632, 8192}, // source index32 VA0x75d800
    {3529728, 6144}, // source index33 VA0x75e800
    {3539968, 6144}, // source index34 VA0x761000
    {3542016, 8192}, // source index35 VA0x761800
    {3546112, 6144}, // source index36 VA0x762800
    {3556352, 6144}, // source index37 VA0x765000
    {3558400, 8192}, // source index38 VA0x765800
    {3562496, 8192}, // source index39 VA0x766800
    {3566592, 8192}, // source index40 VA0x767800
    {3570688, 8192}, // source index41 VA0x768800
    {3574784, 8192}, // source index42 VA0x769800
    {3578880, 8192}, // source index43 VA0x76a800
    {3582976, 8192}, // source index44 VA0x76b800
    {3587072, 8192}, // source index45 VA0x76c800
    {3591168, 6144}, // source index46 VA0x76d800
    {3609600, 6144}, // source index47 VA0x772000
    {3611648, 8192}, // source index48 VA0x772800
    {3615744, 8192}, // source index49 VA0x773800
    {3619840, 8192}, // source index50 VA0x774800
    {3623936, 8192}, // source index51 VA0x775800
    {3628032, 6144}, // source index52 VA0x776800
    {3642368, 6144}, // source index53 VA0x77a000
    {3644416, 8192}, // source index54 VA0x77a800
    {3648512, 6144}, // source index55 VA0x77b800
    {3666944, 4096}, // source index56 VA0x780000
    {3675136, 6144}, // source index57 VA0x782000
    {3677184, 6144}, // source index58 VA0x782800
    {3687424, 4096}, // source index59 VA0x785000
    {3695616, 6144}, // source index60 VA0x787000
    {3697664, 8192}, // source index61 VA0x787800
    {3701760, 8192}, // source index62 VA0x788800
    {3705856, 6144}, // source index63 VA0x789800
    {3716096, 6144}, // source index64 VA0x78c000
    {3718144, 8192}, // source index65 VA0x78c800
    {3722240, 8192}, // source index66 VA0x78d800
    {3726336, 8192}, // source index67 VA0x78e800
    {3730432, 8192}, // source index68 VA0x78f800
    {3734528, 8192}, // source index69 VA0x790800
    {3738624, 8192}, // source index70 VA0x791800
    {3742720, 8192}, // source index71 VA0x792800
    {3746816, 8192}, // source index72 VA0x793800
    {3750912, 6144}, // source index73 VA0x794800
    {3761152, 6144}, // source index74 VA0x797000
    {3763200, 8192}, // source index75 VA0x797800
    {3767296, 6144}, // source index76 VA0x798800
    {3777536, 6144}, // source index77 VA0x79b000
    {3779584, 8192}, // source index78 VA0x79b800
    {3783680, 6144}, // source index79 VA0x79c800
    {3793920, 4096}, // source index80 VA0x79f000
    {3802112, 6144}, // source index81 VA0x7a1000
    {3804160, 8192}, // source index82 VA0x7a1800
    {3808256, 8192}, // source index83 VA0x7a2800
    {3812352, 6144}, // source index84 VA0x7a3800
    {3822592, 4096}, // source index85 VA0x7a6000
    {3830784, 6144}, // source index86 VA0x7a8000
}};
inline std::uint32_t little32(std::span<const std::uint8_t> b,std::size_t o) {
    return std::uint32_t{b[o]}|(std::uint32_t{b[o+1]}<<8)|
        (std::uint32_t{b[o+2]}<<16)|(std::uint32_t{b[o+3]}<<24);
}
}
inline Quote calculate(std::span<const std::uint8_t> file) {
    if(file.size()!=10602040 || runtime::identify_bytes(file).sha256!=original_exe_sha256)
        throw InputError();
    // At preferred image base 433980 takes its direct CRC branch; relocation
    // descriptors are NOT consumed. Preserve87 entries, overlaps and ordering.
    std::uint32_t crc=0xffffffffu;
    const auto append=[&](std::uint8_t byte) {
        const auto index=(crc^byte)&255u;
        crc=(crc>>8)^detail::little32(file,6930672+std::size_t{index}*4);
    };
    for(const auto range:detail::ordered_ranges)
        for(const auto byte:file.subspan(range.file_offset,range.bytes))append(byte);
    for(unsigned shift=0;shift<32;shift+=8)
        append(static_cast<std::uint8_t>(Quote::source_adapter_return_rva>>shift));
    return Quote(~crc);
}
} // namespace civ5::graphics::file_only_fov
