#pragma once
#include "bnw_native_archive_construction.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace civ5::ui {class BnwNativeInstallationInputOwner;}
namespace civ5::ui::detail {
struct BnwArchiveV6HeaderFields {
    std::uint32_t declared_entry_count=0;
    bool operator==(const BnwArchiveV6HeaderFields&) const=default;
};
// PURE diagnostic source formulas. No lease/current stream/registration/count
// authority follows from these values. Finite supported v6 transform0 scope.
inline BnwArchiveV6HeaderFields bnw_v6_header_fields(std::span<const std::uint8_t> bytes) {
    if(bytes.size()!=14)throw std::runtime_error("Native v6 header requires exact4/4/1/1/4 reads");
    const auto u32=[&](std::size_t at) {
        return static_cast<std::uint32_t>(bytes[at]) | (static_cast<std::uint32_t>(bytes[at+1])<<8) |
               (static_cast<std::uint32_t>(bytes[at+2])<<16) | (static_cast<std::uint32_t>(bytes[at+3])<<24);
    };
    if(u32(0)!=6 || u32(4)!=0x5f4b5046 || bytes[8]!=0 || bytes[9]!=0)
        throw std::runtime_error("Native header requires supported v6 FPK_ bytes8/9 zero source scope");
    return {u32(10)}; // Declared count0 is a valid prefix-stage header, not admission.
}
inline std::u16string bnw_prefix_cstring(std::u16string_view name) {
    return std::u16string(name.substr(0,name.find(u'\0')));
}
class BnwGlobalPrefixKernelDiagnostic final {
  public:
    std::optional<std::uint32_t> lookup(std::u16string_view raw) const {
        const auto value=raw.substr(0,raw.find(u'\0'));
        for(std::size_t n=0;n<names_.size();++n)if(names_[n]==value)return static_cast<std::uint32_t>(n);
        return {};
    }
    std::uint32_t append_successful_allocation(std::u16string_view raw) {
        if(names_.size()>=std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("Native unchecked prefix counter overflow is outside finite port");
        const auto index=static_cast<std::uint32_t>(names_.size());
        names_.push_back(bnw_prefix_cstring(raw));return index;
    }
    std::uint32_t lookup_or_append_successful_allocation(std::u16string_view raw) {
        if(const auto found=lookup(raw))return *found;
        return append_successful_allocation(raw);
    }
    std::uint32_t count() const { return static_cast<std::uint32_t>(names_.size()); }
    const std::u16string& name(std::uint32_t index) const {return names_.at(index);}
    void swap(BnwGlobalPrefixKernelDiagnostic& other) noexcept {names_.swap(other.names_);}
  private:
    std::vector<std::u16string> names_;
};
// Pure full-key observable projection ONLY. Native namespace+38 has its own
// MSVC hash/list container; CRC hash alone is not key equality and native bucket
// iteration/rehash/order are not implemented by this diagnostic map.
class BnwArchiveBasenameCacheKernelDiagnostic final {
  public:
    std::optional<std::uint64_t> lookup(std::string_view raw_ansi) const {
        const auto found=records_.find(archive_cache_basename_ascii(raw_ansi));
        if(found==records_.end())return {};return found->second;
    }
    bool publish_modeled_admission(std::string_view raw_ansi,std::uint64_t modeled_archive,
                                   std::uint32_t modeled_native_return) {
        if(modeled_native_return==0)return false;
        return records_.emplace(archive_cache_basename_ascii(raw_ansi),modeled_archive).second;
    }
  private:
    std::map<std::string,std::uint64_t> records_;
};
// Pure pin-budget formula; genuine producers use only their OWN weak count and
// source-private scoped pin budget, never caller scalars to authenticate a child.
inline bool bnw_prefix_persistent_facade_diagnostic(long actual_count,long internal_pins) noexcept {
    return internal_pins>=1 && actual_count>internal_pins;
}
// Pure transaction policy used by private typed Impl, not a production receipt.
// Caller-created values cannot be injected into Header/Record/Registry owners.
struct BnwArchivePrefixIdentityDiagnostic {
    const void* registry=nullptr;
    const void* record=nullptr;
    const void* header=nullptr;
    const void* construction=nullptr;
    const void* cold=nullptr;
    const void* lease=nullptr;
    const void* service=nullptr;
    const ::civ5::ui::BnwNativeInstallationInputOwner* installation_input=nullptr;
    bool operator==(const BnwArchivePrefixIdentityDiagnostic&) const=default;
};
class BnwArchivePrefixPhaseDiagnostic final {
    enum class Phase {fresh,prepared,prepared_current,published,entry_started,entry_current,entry_finished};
    Phase phase_=Phase::fresh;
    BnwArchivePrefixIdentityDiagnostic identity_{};
    void require_identity(const BnwArchivePrefixIdentityDiagnostic& value) const {
        if(value!=identity_ || !value.registry || !value.record || !value.header ||
           !value.construction || !value.cold || !value.service ||
           (bool(value.lease)==bool(value.installation_input)))
            throw std::runtime_error("Native prefix transaction has different sealed brands");
    }
  public:
    void seal_prepared(const BnwArchivePrefixIdentityDiagnostic& value) {
        if(phase_!=Phase::fresh)throw std::runtime_error("Native prefix transaction already sealed");
        identity_=value;require_identity(value);phase_=Phase::prepared;
    }
    void mark_prepared_current(const BnwArchivePrefixIdentityDiagnostic& value) {
        require_identity(value);
        if(phase_!=Phase::prepared)throw std::runtime_error("Native prefix prepared validation repeated/retired");
        phase_=Phase::prepared_current;
    }
    void require_prepared_current(const BnwArchivePrefixIdentityDiagnostic& value) const {
        require_identity(value);
        if(phase_!=Phase::prepared_current)throw std::runtime_error("Native prefix lacks active prepared currentness");
    }
    // Only the private actual producer calls this after all throwing checks.
    // There is no callback/check/allocation between shared store and this mark.
    void publish_after_source_store() noexcept {phase_=Phase::published;}
    void start_entry(const BnwArchivePrefixIdentityDiagnostic& value) {
        require_identity(value);
        if(phase_!=Phase::published)throw std::runtime_error("Native prefix entry preflight repeated/out of order");
        phase_=Phase::entry_started;
    }
    void mark_entry_current(const BnwArchivePrefixIdentityDiagnostic& value) {
        require_identity(value);
        if(phase_!=Phase::entry_started)throw std::runtime_error("Native prefix entry validation repeated/retired");
        phase_=Phase::entry_current;
    }
    void require_entry_current(const BnwArchivePrefixIdentityDiagnostic& value) const {
        require_identity(value);
        if(phase_!=Phase::entry_current)throw std::runtime_error("Native prefix entry preflight not active/current");
    }
    void retire_entry() noexcept {phase_=Phase::entry_finished;}
};

}
