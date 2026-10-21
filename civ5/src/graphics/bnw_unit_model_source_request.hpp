#pragma once
#include <bit>
#include <cstdint>
namespace civ5::graphics::model_request_detail {
struct SourceArguments {
    std::uint32_t scale_bits=0,load_flags=2;
    bool modifier_required=false;
    std::uint32_t modifier_scale_bits=0,flags=0,filter=0,filter_count=0;
};
// 6EF0B6 UCOMISS against1: unequal or unordered =>9723E0. These are pure
// source instructions; public production APIs cannot supply their operands.
inline SourceArguments prepare(std::uint32_t scale_bits) {
    const float scale=std::bit_cast<float>(scale_bits);
    SourceArguments out;out.scale_bits=scale_bits;
    out.modifier_required=scale!=1.F;
    if(out.modifier_required)out.modifier_scale_bits=scale_bits;
    return out;
}
struct ResourceFields {std::uint32_t scale_bits=0x3f800000,flags=0;};
// 9721A0 obtains a resource from vtable+10. Only the writes after that modeled
// endpoint are represented here; they do not create native cache authority.
inline ResourceFields apply_modifier(ResourceFields resource,const SourceArguments& source) {
    if(std::bit_cast<float>(source.modifier_scale_bits)>0.F)
        resource.scale_bits=source.modifier_scale_bits;
    resource.flags=source.flags;return resource;
}
// 962AD0 unconditionally stores+BC, returning the old word through x87.
inline std::uint32_t apply_fallback(ResourceFields& resource,std::uint32_t scale_bits) {
    const auto old=resource.scale_bits;resource.scale_bits=scale_bits;return old;
}
} // namespace civ5::graphics::model_request_detail
