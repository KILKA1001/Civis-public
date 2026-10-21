#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <bit>
namespace civ5::ui {
namespace detail {
// Signed64 CRT _stat64i32 st_mtime seconds, NOT Win32 FILETIME. Formula only. These values are NOT authority to advance a native owner.
constexpr bool native_mode7_replaces(bool old_archived, bool source_cache_present,
    bool cached_archive_present, std::uint64_t incoming_time, std::uint64_t archive_time) {
    if (!old_archived || !source_cache_present || !cached_archive_present) return true;
    const auto incoming_high = std::bit_cast<std::int32_t>(std::uint32_t(incoming_time >> 32));
    const auto archive_high = std::bit_cast<std::int32_t>(std::uint32_t(archive_time >> 32));
    return incoming_high > archive_high ||
           (incoming_high == archive_high && std::uint32_t(incoming_time) >= std::uint32_t(archive_time));
}
}
struct BnwNativeDeclaredPackageProbe {
    std::string selector, declared_path, actual_source_path;
    std::uint32_t attributes = 0xffffffffU, os_error = 0;
    bool exists = false;
};
struct BnwNativePackagePreSkinRow {
    std::string descriptor, guid;
    // Parser/Tablet/file-validation transitions owned here. SAME retained Key
    // equality ORs bit0; mismatch/rejected helpers never clear or invent it.
    // Early bit2=1 closes the query gate independently of Key comparison.
    std::uint32_t known_flags_mask = 0xfffffffeU, known_flags_value = 4;
    std::optional<bool> key_integrity_bit;
    bool early_query_gate_closed = false, declared_inputs_valid = false;
    std::uint32_t category_mask = 0xffffffffU;
    bool category_mask_source_closed = true;
    std::vector<BnwNativeDeclaredPackageProbe> probes;
    std::array<bool,2> category_mount_stage_complete{false,false};
};
struct BnwNativePackagePreSkinFrame {
    bool tablet_guid_transition_complete = false;
    bool declared_input_validation_complete = false;
    bool category_mount_stage_complete = false;
    bool native_skin_loading_complete = false;
    std::vector<BnwNativePackagePreSkinRow> packages;
    std::string next_dependency;
};
// Source54CCD6/64C610 predecessor. Category source construction and native
// cached-archive timestamp provenance are retained as explicit pending work.
// No caller flags, paths, timestamps or selected-save package substitution.
class BnwNativePackagePreSkinOwner {
 public:
    static std::shared_ptr<const BnwNativePackagePreSkinOwner> create(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner> same_early_resources);
    ~BnwNativePackagePreSkinOwner();
    BnwNativePackagePreSkinOwner(const BnwNativePackagePreSkinOwner&) = delete;
    BnwNativePackagePreSkinOwner& operator=(const BnwNativePackagePreSkinOwner&) = delete;
    const BnwNativePackagePreSkinFrame& current() const;
    void require_same_resources(const BnwNativePackageDiscoveryOwner&) const;
    void require_category_mount_stage_complete() const;
 private:
    friend class BnwNativePackageDiscoveryOwner;
    friend class BnwNativePackageCategorySourceOwner;
    friend class BnwNativePackageArchiveTimeOwner;
    const BnwNativePackagePreSkinFrame& category_operation_frame(const BnwNativePackageDiscoveryOwner&) const;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit BnwNativePackagePreSkinOwner(std::shared_ptr<Impl>);
};
}
