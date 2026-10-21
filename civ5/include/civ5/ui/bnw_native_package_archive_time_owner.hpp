#pragma once
#include "civ5/ui/bnw_native_package_pre_skin_owner.hpp"
namespace civ5::ui {
class BnwNativePackageCategorySourceOwner;
struct BnwNativeStat64Receipt {
    int return_value = -1, error_number = 0;
    // Failed unchecked native loose stat has UNKNOWN outputs, not zero/FindA.
    std::optional<std::int32_t> size;
    std::optional<std::int64_t> mtime;
    bool operator==(const BnwNativeStat64Receipt&) const = default;
};
struct BnwNativePackageArchiveTimeRead {
    std::string descriptor, declared, relative_source;
    unsigned category = 0;
    bool archive_loaded = false;
    BnwNativeStat64Receipt original, canonical_payload;
    // Native checked83C957 failure retains cache constructor zero. Valid only
    // for an actually loaded archive; absence has no cached archive at all.
    std::optional<std::int64_t> native_cached_archive_mtime;
};
struct BnwNativePackageArchiveTimeFrame {
    std::filesystem::path loaded_crt;
    std::string loaded_crt_sha256;
    std::uint32_t process_id = 0, thread_id = 0, code_page = 0;
    std::size_t eligible_categories = 0, archives_loaded = 0;
    bool archive_metadata_source_complete = false;
    bool category_mount_stage_complete = false;
    std::vector<BnwNativePackageArchiveTimeRead> attempts;
};
// Source83C94E/83A9A7 stat64i32 fields, scoped to actual original metadata.
// UCRTx64 field port is pinned to the finite VC90x86/x64 differential receipt
// and actual loaded DLL. Canonical copy times never replace native times.
// This is a metadata/cache predecessor, not category table publication.
class BnwNativePackageArchiveTimeOwner final {
  public:
    static std::shared_ptr<const BnwNativePackageArchiveTimeOwner> create(
        std::shared_ptr<const BnwNativePackagePreSkinOwner>,
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>);
    ~BnwNativePackageArchiveTimeOwner();
    BnwNativePackageArchiveTimeOwner(const BnwNativePackageArchiveTimeOwner&) = delete;
    BnwNativePackageArchiveTimeOwner& operator=(const BnwNativePackageArchiveTimeOwner&) = delete;
    const BnwNativePackageArchiveTimeFrame& current() const;
    void require_same_resources(const BnwNativePackageDiscoveryOwner&) const;
  private:
    friend class BnwNativePackageCategorySourceOwner;
    BnwNativeStat64Receipt original_loose_stat(const std::filesystem::path&) const;
    void require_category_borrow_current() const;
    BnwNativeStat64Receipt category_original_loose_stat(const std::filesystem::path&) const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativePackageArchiveTimeOwner(std::unique_ptr<Impl>);
};
}
