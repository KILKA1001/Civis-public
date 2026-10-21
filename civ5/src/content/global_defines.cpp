#include "civ5/content/global_defines.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::content {
namespace {
#define GD_INT(name, value, zero) {name, GlobalDefineKind::integer, value, 0, zero},
#define GD_FLOAT(name, value) {name, GlobalDefineKind::source_float, 0, value, false},
constexpr GlobalDefineSpec bnw[] = {
#include "global_defines_bnw.inc"
};
constexpr GlobalDefineSpec vp542[] = {
#include "global_defines_vp542.inc"
};
#undef GD_INT
#undef GD_FLOAT
} // namespace
std::span<const GlobalDefineSpec> global_define_specs(GlobalDefineProfile profile) {
    switch (profile) {
    case GlobalDefineProfile::bnw: return bnw;
    case GlobalDefineProfile::vp542: return vp542;
    }
    throw std::runtime_error("Unknown global define profile");
}
const CachedGlobalDefine *GlobalDefineCache::find(std::string_view name) const {
    name = name.substr(0, name.find('\0'));
    const auto found = names_.find(name);
    return found == names_.end() ? nullptr : &entries_[found->second];
}
std::uint64_t GlobalDefineCache::missing() const {
    return std::count_if(entries_.begin(), entries_.end(), [](const auto &entry) {
        return !entry.lookup.found && entry.lookup.error.empty();
    });
}
std::uint64_t GlobalDefineCache::errors() const {
    return std::count_if(entries_.begin(), entries_.end(), [](const auto &entry) {
        return !entry.lookup.error.empty();
    });
}
} // namespace civ5::content
