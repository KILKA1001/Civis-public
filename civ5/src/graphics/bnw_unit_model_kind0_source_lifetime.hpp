#pragma once
#include <bit>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>
namespace civ5::graphics::kind0_lifetime_detail {
// Controlled CPU observations only. IDs are not native objects/cache authority.
using Id = std::uint32_t;
struct FixtureObject {
    std::uint32_t refs = 0;
    bool wrapper = false;
    Id underlying = 0;
    std::uint32_t name = 0, vtable = 0;
};
struct RequiredEffect {
    std::uint32_t source_label = 0;
    Id object = 0;
    std::uint32_t argument = 0;
    bool operator==(const RequiredEffect&) const = default;
};
struct FixtureState {
    std::map<Id, FixtureObject> objects;
    // Core destruction and storage release are mandatory and unexecuted.
    // There is deliberately no acknowledgement/continuation API.
    std::vector<RequiredEffect> pending;
};
inline FixtureObject& object(FixtureState& s, Id id) {
    const auto at = s.objects.find(id);
    if (!id || at == s.objects.end())
        throw std::invalid_argument("Kind0 diagnostic reference identity unavailable");
    return at->second;
}
inline FixtureObject& wrapper(FixtureState& s, Id id) {
    auto& o = object(s, id);
    if (!o.wrapper) throw std::invalid_argument("Kind0 wrapper type required");
    return o;
}
inline void resource(FixtureState& s, Id id) {
    if (id && object(s, id).wrapper)
        throw std::invalid_argument("FGrannyAsset resource type required");
}
inline bool positive(std::uint32_t bits) { return std::bit_cast<std::int32_t>(bits) > 0; }
inline void retain(FixtureState& s, Id id) { if (id) ++object(s, id).refs; }
inline void release(FixtureState& s, Id id);
inline void wrapper_destructor(FixtureState& s, Id id, std::uint32_t deleting) {
    auto& w = wrapper(s, id);
    w.vtable = 0xb70f24;
    if (w.underlying) { release(s, w.underlying); w.underlying = 0; }
    w.vtable = 0xb70f88;
    if (deleting & 1) s.pending.push_back({0x81cd70, id, 0});
}
inline void release(FixtureState& s, Id id) {
    if (!id) return;
    auto& o = object(s, id);
    --o.refs; // Source DWORD wraps; the branch interprets the result as signed.
    if (positive(o.refs)) return;
    if (o.wrapper) wrapper_destructor(s, id, 1);
    else { s.pending.push_back({0x965490, id, 0}); s.pending.push_back({0x81cd70, id, 0}); }
}
inline void swap(FixtureState& s, Id& slot, Id replacement) {
    if (slot == replacement) return;
    retain(s, replacement); release(s, slot); slot = replacement;
}
// 974110's result is an explicit fixture endpoint; no load occurs here.
inline bool diagnostic_load_result(FixtureState& s, Id id, std::uint32_t name, Id modeled_result) {
    auto& w = wrapper(s, id); resource(s, modeled_result); resource(s, w.underlying);
    w.name = name; // Source stores the name before calling the opaque loader.
    swap(s, w.underlying, modeled_result);
    return w.underlying != 0;
}
inline bool diagnostic_copy_out(FixtureState& s, Id id, std::optional<Id>& out) {
    const auto source = wrapper(s, id).underlying; resource(s, source);
    if (out) { resource(s, *out); swap(s, *out, source); }
    return true;
}
inline bool diagnostic_unload(FixtureState& s, Id id) {
    auto& w = wrapper(s, id); resource(s, w.underlying);
    release(s, w.underlying); w.underlying = 0; return true;
}
inline bool diagnostic_still_referenced(FixtureState& s, Id id) {
    const auto& w = wrapper(s, id);
    if (positive(w.refs - 1)) return true;
    resource(s, w.underlying);
    return w.underlying && positive(object(s, w.underlying).refs - 1);
}
// 973A10 has an empty initialized output receiver. The cache/loader is modeled.
inline Id diagnostic_adapter_result(FixtureState& s, Id modeled_wrapper) {
    if (!modeled_wrapper) return 0;
    wrapper(s, modeled_wrapper);
    retain(s, modeled_wrapper); release(s, modeled_wrapper);
    const auto result = object(s, modeled_wrapper).underlying; resource(s, result);
    retain(s, result); release(s, modeled_wrapper); return result;
}
inline void diagnostic_remove_pending(FixtureState& s, std::vector<Id>& slots, Id id) {
    wrapper(s, id);
    for (auto& slot : slots) if (slot == id) { release(s, slot); slot = 0; return; }
}
[[noreturn]] inline void require_native_load() {
    throw std::runtime_error("Native kind0 load unavailable: SAME namespace/allocator/cache, "
        "974110 parser/dependencies/listeners and resource965490/free effects unowned");
}
} // namespace civ5::graphics::kind0_lifetime_detail
