#pragma once
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_default_namespace_policy.hpp"
#include "bnw_native_primary32_loop_journal.hpp"
#include <functional>

namespace civ5::ui::detail {
class BnwNativePrimary32RecordSnapshotOwner;
// Pure record formula only, never a bindings/snapshot issuer.
inline void bnw_primary32_record_shape_diagnostic(const BnwNativeArchiveRecordDiagnostic& record,
                                                 std::string_view first, std::uint32_t actual_prefix) {
    if (actual_prefix >= 4096 || record.offset == 0 || (record.packed_prefix & 0xfffffu) != 0 ||
        (record.packed_prefix >> 20) != actual_prefix || record.symbolic_slot == 0 ||
        record.symbolic_slot > NativeResourceBucketOrder::source_initial_pool ||
        bnw_native_admission_ascii_crc(first) != record.crc)
        throw std::runtime_error("Primary32 selected record differs from finite source mode0 archive branch");
}
// Capture borrowed input BEFORE any owner/client callback. Preflight the entire
// bounded grammar and basename size before allocating owned query storage.
inline BnwDefaultNamespaceAsciiQueryDiagnostic
bnw_primary32_prepare_ascii_query_diagnostic(std::u16string_view raw) {
    if (raw.empty() || raw.size() > 259)
        throw std::runtime_error("Primary32 query requires bounded nonempty UTF16ASCII input");
    for (const char16_t c : raw)
        if (c < 32 || c > 126)
            throw std::runtime_error("Primary32 query grammar lacks finite UTF16ASCII proof");
    const auto slash = raw.find_last_of(u"/\\");
    const auto colon = slash == std::u16string_view::npos ? raw.find(u':') : std::u16string_view::npos;
    const auto delimiter = slash != std::u16string_view::npos ? slash : colon;
    const auto first = delimiter == std::u16string_view::npos ? 0 : delimiter + 1;
    if (raw.size() == first || raw.size() - first > 127)
        throw std::runtime_error("Primary32 query has unsupported empty/long basename");
    return bnw_default_namespace_ascii_query_diagnostic(raw); // owned basename+CRC
}
// CLOSED source capture. Root/Arena alone construct/fill it from their actual
// privately retained nodes/frames. No public callbacks/maps/flags/count inputs.
class BnwNativePrimary32RecordSourceBindings final {
  public:
    ~BnwNativePrimary32RecordSourceBindings() = default;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    BnwNativePrimary32RecordSourceBindings() = default;
    BnwNativePrimary32RecordSourceBindings(BnwNativePrimary32RecordSourceBindings&&) noexcept = default;
    struct Archive {
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
        std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries;
        std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publication;
        std::shared_ptr<const std::uint8_t> actual_step_brand;
    };
    struct Row {
        const BnwNativeArchiveRecordDiagnostic* actual_record_node = nullptr;
        const assets::AssetHandle* actual_handle_node = nullptr;
        BnwNativeArchiveRecordDiagnostic record;
        std::string first_raw_name;
        std::size_t archive_ordinal = 0;
        std::optional<assets::AssetHandle> canonical_handle;
    };
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> package;
    std::shared_ptr<const std::uint8_t> actual_snapshot_brand;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeDefaultNamespacePolicyOwner> initial_policy;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes;
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena;
    std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache;
    std::shared_ptr<const BnwNativePrimary32LoopJournalOwner> journal;
    std::array<Archive,32> archives{};
    std::map<std::uint32_t,Row> selected;
    // Root-owned callback-free authentication of actual scope/brand/node values.
    // nullptr facade denotes unpublished construction ONLY; nonnull must match
    // Root's genuinely retained snapshot. Never call Snapshot.current inside it.
    std::function<void(const BnwNativePrimary32RecordSourceBindings&,
                       const BnwNativePrimary32RecordSnapshotOwner*)> authenticate;
};
struct BnwNativePrimary32RecordQueryDiagnostic {
    bool matched = false;
    std::uint32_t requested_crc = 0;
    BnwNativeArchiveRecordDiagnostic selected;
    std::string first_raw_name;
    std::size_t archive_ordinal = 0;
};
class BnwNativePrimary32RecordSnapshotOwner final {
  public:
    ~BnwNativePrimary32RecordSnapshotOwner();
    BnwNativePrimary32RecordSnapshotOwner(const BnwNativePrimary32RecordSnapshotOwner&) = delete;
    void require_current() const;
    std::size_t selected_record_count_diagnostic() const;
    // Query of this completed archive record set ONLY. Copied facts cannot
    // authorize native namespace miss/read/current activation or Config Load.
    BnwNativePrimary32RecordQueryDiagnostic query_ascii_record_diagnostic(std::u16string_view) const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    static std::shared_ptr<const BnwNativePrimary32RecordSnapshotOwner> acquire(
        BnwNativePrimary32RecordSourceBindings);
    // Only this member owns private installed Cold access; validation helpers
    // retain their existing callback bookends and cannot issue source authority.
    static void require_installed_cold_source(const BnwNativePrimary32RecordSourceBindings&);
    const BnwNativePrimary32RecordSourceBindings& source_identity_without_callbacks()const noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativePrimary32RecordSnapshotOwner(std::unique_ptr<Impl>);
};
}
