#pragma once
#include "bnw_native_archive_basename_cache.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_current_source_control.hpp"
#include <array>
#include <functional>
#include <thread>
namespace civ5::ui::detail {
class BnwNativeCommonSourceValidationContext;
// Literal source order only. This helper cannot construct a channel/receipt.
inline constexpr std::array<std::string_view,13> bnw_common13_descriptors={
 "BuildingModels","CityModels","EffectData","FeatureModels","HutsBarbarianFortModels",
 "ImprovementModels","LeaderModels","MiscData","SoundData","TempModels","TerrainModels",
 "UnitModels","WonderModels"};
inline std::string bnw_common13_raw_request(std::size_t ordinal) {
 if(ordinal>=bnw_common13_descriptors.size())throw std::runtime_error("Common source ordinal outside thirteen");
 return std::string("Resource\\Common\\")+std::string(bnw_common13_descriptors[ordinal])+".fpk";
}
class BnwNativeCommon13ArchiveChannel final {
 public:
  BnwNativeCommon13ArchiveChannel(const BnwNativeCommon13ArchiveChannel&)=delete;
  void require_current()const;
 private:
  friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
  friend class BnwNativeColdFilesystemOwner;
  friend class BnwNativeArchiveV6EntryAdmissionOwner;
  friend class BnwNativeArchiveEntryAdmissionArena;
  BnwNativeCommon13ArchiveChannel()=default;
  std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
  std::shared_ptr<const BnwNativeColdFilesystemOwner> cold_;
  std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
  std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
  std::shared_ptr<const BnwNativeCurrentDefaultSource> default_;
  std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena_;
  std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache_;
  std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes_;
  std::shared_ptr<const void> epoch_,brand_;
  const std::thread::id thread_=std::this_thread::get_id();
  std::size_t ordinal_=0;
  // Body belongs to Parent producer; it authenticates the genuine immutable
  // terminal Common step/brand and same completed Cold loan, never scalar ready.
  void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
  void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
  void require_active()const;
};
// Retained actual attempts: absent and zero-admission are distinct from cache publication.
struct BnwNativeCommon13Step final {
 std::shared_ptr<const BnwNativeCommon13ArchiveChannel> channel;
 std::shared_ptr<const BnwNativeColdArchiveRequest> request;
 std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup;
 std::shared_ptr<const BnwNativeArchiveConstruction> construction;
 std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
 std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
 std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries;
 std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publication;
 enum class Phase {issued,lookup,constructor,header,prefix,metadata,entries,terminal};
 Phase phase=Phase::issued;
 bool absent=false;
};
// Source-only private entry to the real Package operations; no caller readiness inputs.
class BnwNativeOwnedStartupCompletion;
class BnwNativeDefaultCommonProduction final {
 public:
  static void primary(const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&);
  // Read-only negative regression checks; these issue/advance no source owner.
  static void require_active_primary_partial_effects(const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
  static void require_failed_primary_quarantine(const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
  static void categories(const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&);
  // SAME genuine pre-DX9 receiver adoption; actual installed admission and
  // category effects remain separate source-owned operations.
  static void adopt_installed_pre_dx9_receivers(
      const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
  // Installed diagnostic only: SAME returned source -> receiver adoption
  // -> first Paks observation -> negative saved-policy domain check.
  // This is not the installed frontend/Original startup policy driver.
  static void with_installed_common_receiver_checkpoint(
      const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
      const std::function<void(int,const std::vector<std::string>&)>&);
  // Genuine normal-return resource result only; downstream UI/SQL/Window
  // producers retain distinct guards and authority.
  static std::shared_ptr<const BnwNativeOwnedStartupCompletion> advance_owned_initial_category_prefix(
      const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&);
};
}
