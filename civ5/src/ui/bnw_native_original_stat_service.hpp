#pragma once
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <cstdint>
#include <memory>
#include <optional>

namespace civ5::ui {class BnwNativePackageDiscoveryOwner;class BnwNativeInstallationInputOwner;}
namespace civ5::ui::detail {
class BnwNativeCommonSourceValidationContext;
class BnwNativeCompletedColdSourceLoan;
class BnwNativeOriginalStatService;
struct BnwNativeOriginalStatFields {
    int return_value = -1, error_number = 0;
    std::optional<std::int32_t> size;
    std::optional<std::int64_t> mtime;
    bool operator==(const BnwNativeOriginalStatFields&) const = default;
};
struct BnwNativeOriginalStatSource {
    std::filesystem::path loaded_crt;
    std::string loaded_crt_sha256;
    std::uint32_t process_id = 0, thread_id = 0, code_page = 0;
};
// Private source read, not a cached archive or stage receipt. The sole producer
// is the actual service. Expired service, changed metadata/lease/ambient state
// refuse even if copied scalar fields happen to equal the original observation.
class BnwNativeOriginalStatRead final {
  public:
    ~BnwNativeOriginalStatRead();
    BnwNativeOriginalStatRead(const BnwNativeOriginalStatRead&) = delete;
    BnwNativeOriginalStatRead& operator=(const BnwNativeOriginalStatRead&) = delete;
    const BnwNativeOriginalStatFields& current() const;
    const std::filesystem::path& relative_source() const;
    std::uint64_t sequence() const;
    void require_same_service(const BnwNativeOriginalStatService&) const;
  private:
    friend class BnwNativeOriginalStatService;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    const BnwNativeOriginalStatFields& current_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& relative_source_common_source(BnwNativeCommonSourceValidationContext&)const;
    void require_same_service_common_source(const BnwNativeOriginalStatService&,BnwNativeCommonSourceValidationContext&)const;
    const BnwNativeOriginalStatFields& current_completed(const BnwNativeCompletedColdSourceLoan&)const;
    const std::filesystem::path& relative_source_completed(const BnwNativeCompletedColdSourceLoan&)const;
    void require_same_service_completed(const BnwNativeOriginalStatService&,const BnwNativeCompletedColdSourceLoan&)const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeOriginalStatRead(std::unique_ptr<Impl>);
};
// Scoped finite VC90x86/UCRTx64 field port: exact loaded UCRT, ACP1251, recorded
// 172-byte Windows timezone, absent TZ; ASCII ORIGINAL paths only. No package,
// Color, SQL, default archive constructor, cached zero or public ready authority.
class BnwNativeOriginalStatService final {
  public:
    static std::shared_ptr<const BnwNativeOriginalStatService> acquire(
        std::shared_ptr<const BnwNativeEarlyInputLease>);
    ~BnwNativeOriginalStatService();
    BnwNativeOriginalStatService(const BnwNativeOriginalStatService&) = delete;
    BnwNativeOriginalStatService& operator=(const BnwNativeOriginalStatService&) = delete;
    void require_current() const;
    void require_same_input(const BnwNativeEarlyInputLease&) const;
    const BnwNativeOriginalStatSource& source() const;
    std::shared_ptr<const BnwNativeOriginalStatRead> read_original_relative(
        const std::filesystem::path&) const;
  private:
    static std::shared_ptr<const BnwNativeOriginalStatService> acquire(
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>);
    void require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner&)const;
    friend class BnwNativeOriginalStatRead;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_input_common_source(const BnwNativeEarlyInputLease&,BnwNativeCommonSourceValidationContext&)const;
    void require_current_read_common_source(const BnwNativeOriginalStatRead&,BnwNativeCommonSourceValidationContext&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&)const;
    void require_same_input_completed(const BnwNativeEarlyInputLease&,const BnwNativeCompletedColdSourceLoan&)const;
    void require_current_read_completed(const BnwNativeOriginalStatRead&,const BnwNativeCompletedColdSourceLoan&)const;
    void require_current_read(const BnwNativeOriginalStatRead&) const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeOriginalStatService(std::unique_ptr<Impl>);
};
}
