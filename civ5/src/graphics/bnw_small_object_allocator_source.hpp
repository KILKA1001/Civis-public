#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace civ5::graphics::small_allocator_detail {
// Diagnostic symbolic operands only. This is not a native allocator owner.
using Words = std::array<std::uint32_t, 13>;
using Params = std::array<std::uint32_t, 15>;
struct Effect {
    std::uint32_t source = 0, first = 0, second = 0;
    bool operator==(const Effect&) const = default;
};
struct Observation {
    std::array<std::uint32_t, 7> fields{};
    Params params{};
    // Storage/VM/slab/CRT effects are unexecuted, mandatory endpoints.
    // No method can acknowledge these as completed native effects.
    std::vector<Effect> pending;
};
inline Params default_params(std::uint32_t padding) {
    if (padding > 255) throw std::invalid_argument("Diagnostic padding byte");
    // 824630 preserves padding byte +1B; never assume it is initialized.
    return {0x10000000,0x2000000,0x10000,0x1000,0x100,4,
        1|(padding<<24),0x1000,0x10000,0x10,0x2000,0x200,12,16,0x40};
}
inline Params original_export_params(std::uint32_t padding) {
    if (padding > 255) throw std::invalid_argument("Diagnostic padding byte");
    // Original PE6D37C0 ->8246C0. Not a current-process export binding.
    return {0x2000000,0x2000000,0x10000,0x1000,0x200,0x20,
        1|(padding<<24),0x200,0x2000,0x10,0x2000,0x200,12,16,0x10};
}
inline std::uint32_t class_index(std::uint32_t bytes,std::uint32_t quantum,
                                 std::uint32_t count) {
    if (!quantum) throw std::invalid_argument("Unowned allocator quantum zero");
    auto rounded = bytes ? bytes : 1u;
    const auto mask = quantum - 1;
    if (rounded & mask) rounded = (rounded & ~mask) + quantum;
    rounded = std::max(rounded, 2u);
    const auto index = rounded / quantum - 1;
    return index < count ? index : 0xffffffff;
}
// Exact fixed synthetic pool-map operands used by the independent CPU fixture.
// Real base/extent/page map/mutex must come from a retained context owner.
inline std::uint32_t fixture_pool_tag(std::uint32_t position) {
    constexpr std::array<std::uint32_t,7> pointers{
        0,0x400a000,0x400a0ff,0x400a100,0x400afff,0x4009fff,0x400b000};
    if (position >= pointers.size()) throw std::invalid_argument("Unknown pool pointer fixture");
    const auto p = pointers[position];
    if (p < 0x400a000 || p >= 0x400b000) return 0xbaadf00d;
    return ((p - 0x400a000) >> 8) + 7;
}
inline Observation observe(const Words& c) {
    Observation out;
    switch (c[0]) {
    case 0: {
        const auto cache=c[1],disabled=c[2],blocked=c[3],initialize=c[4],
            exported=c[5],external=c[6],override_kind=c[7],storage=c[8];
        if (cache>2 || exported>2 || override_kind>2)
            throw std::invalid_argument("Unknown diagnostic export/cache identity");
        out.fields[0]=cache;out.fields[2]=disabled;
        if (disabled) break;
        if (cache>1) {out.fields[1]=cache;break;}
        if (blocked) break;
        out.fields[3]=1;out.fields[4]=1;
        if (!exported) {out.fields[2]=1;break;}
        if (exported==2) {
            out.pending.push_back({0x81c87d,0,external});
            out.fields[0]=external?2:0;out.fields[1]=out.fields[0];break;
        }
        if (cache || !initialize) break;
        // Init-params query, then recursive source lookup with cache sentinel1.
        out.fields[3]=3;out.fields[4]=3;
        out.pending.push_back({0x9c97e8,0xbc,storage});
        out.fields[0]=0;
        if (storage) {
            out.fields[0]=2;out.fields[1]=2;
            out.fields[5]=override_kind==2?2:1;
            out.params=override_kind==2?original_export_params(c[10]):default_params(c[9]);
            out.pending.push_back({0x824de0,out.fields[5],2});
        }
        break;
    }
    case 1: {
        const auto present=c[1],disabled=c[2],slab_result=c[3],bytes=c[4],
            alignment=c[5],quantum=c[6],count=c[7],crt_result=c[8];
        // Source permits alignment0 in the bit test; its rounded size is0.
        if (!(alignment & (alignment-1)) && present && !disabled) {
            const auto rounded=(bytes+alignment-1)&~(alignment-1);
            out.fields[1]=1;
            const auto index=class_index(rounded,quantum,count);
            if (index!=0xffffffff) {
                out.fields[2]=1;out.pending.push_back({0x824b40,index,slab_result});
                if (slab_result) {out.fields[0]=1;break;}
                out.fields[6]=1; // Original824F10 sets5CF909C on slab failure.
            }
        }
        out.fields[0]=crt_result?2:0;out.fields[3]=1;
        out.fields[4]=bytes;out.fields[5]=alignment;
        out.pending.push_back({0x9c9870,bytes,alignment});
        break;
    }
    case 2: {
        const auto present=c[1],aligned=c[2],position=c[3];
        const auto tag=fixture_pool_tag(position);
        out.fields={present,0,0xbaadf00d,0,aligned,0};
        if (present && tag!=0xbaadf00d) {
            out.fields[1]=1;out.fields[2]=tag;out.fields[3]=2;
            out.pending.push_back({0x824fb0,position,tag});
        } else out.pending.push_back({aligned?0x9c984cu:0x9c97e4u,position,0});
        break;
    }
    case 3: {
        const auto index=class_index(c[1],c[2],c[3]);
        out.fields={std::uint32_t(index!=0xffffffff),index,0,0,0,0};break;
    }
    case 4: {
        const auto tag=fixture_pool_tag(c[1]);
        const bool owns=tag!=0xbaadf00d;
        out.fields={std::uint32_t(owns),tag,owns?2u:0u,0,0,0};break;
    }
    default: throw std::invalid_argument("Unknown private allocator fixture operation");
    }
    return out;
}
[[noreturn]] inline void require_live_allocator() {
    throw std::runtime_error("Native allocator unavailable: SAME module/export/context/VM, "
        "slab storage, mutex and allocation/free lifetime owners required");
}
} // namespace civ5::graphics::small_allocator_detail
