#include "civ5/content/info_types.hpp"
#include "civ5/runtime/crc32_byte.hpp"
#include <stdexcept>

namespace civ5::content {
std::uint32_t saved_info_type_hash(std::string_view type) {
    std::uint32_t crc = 0xffffffffu;
    for (const char raw_byte : type) {
        const auto byte = static_cast<unsigned char>(raw_byte);
        if (byte == 0)
            throw std::runtime_error("Embedded NUL in saved info type name");
        crc = runtime::crc32_update_byte(crc, byte);
    }
    return crc;
}
const InfoTypeEntry *InfoTypeCatalog::find(std::string_view type) const {
    // Match source C-string lookup, including an embedded terminator.
    type = type.substr(0, type.find('\0'));
    const auto found = names_.find(type);
    return found == names_.end() ? nullptr : &found->second;
}
const InfoTypeEntry *InfoTypeCatalog::find_hash(std::uint64_t hash) const {
    if (hash > UINT32_MAX) return nullptr;
    const auto found = hashes_.find(static_cast<std::uint32_t>(hash));
    return found == hashes_.end() ? nullptr : &found->second;
}
void InfoTypeCatalog::insert(InfoTypeEntry entry, const InfoTypeBudget &budget) {
    const auto sample = [&](const InfoTypeEntry &previous) {
        if (report_.collision_samples.size() < budget.collision_samples)
            report_.collision_samples.push_back({previous, entry});
    };
    if (const auto found = names_.find(entry.type); found != names_.end()) {
        ++report_.name_overwrites;
        sample(found->second);
    }
    if (const auto found = hashes_.find(entry.hash);
        found != hashes_.end() && found->second.type != entry.type) {
        ++report_.hash_collisions;
        sample(found->second);
    }
    names_.insert_or_assign(entry.type, entry);
    hashes_.insert_or_assign(entry.hash, entry);
}
} // namespace civ5::content
