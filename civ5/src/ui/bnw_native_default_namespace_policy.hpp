#pragma once
namespace civ5::ui::detail {class BnwNativeCommonSourceValidationContext;}
#include "bnw_native_cold_filesystem_owner.hpp"
#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace civ5::ui::detail {
class BnwNativeArchiveEntryAdmissionArena;
// Pure constructor storage policy, also used by the private owner. No argument
// can inject flags/filters into the owner. This is NOT CURRENT VFS activation.
class BnwDefaultNamespaceInitialPolicyDiagnostic final {
    const std::uint8_t archive_priority_ = 1, enabled_ = 1;
    const std::vector<std::string> filters_{};
  public:
    std::uint8_t constructor_archive_priority() const noexcept { return archive_priority_; }
    std::uint8_t constructor_enabled() const noexcept { return enabled_; }
    std::span<const std::string> constructor_prefix_filters() const noexcept { return filters_; }
};

struct BnwDefaultNamespaceAsciiQueryDiagnostic {
    std::string raw_basename;
    std::uint32_t crc = 0;
};
// Finite ASCII-only83AF90/81F3C0/83B4E0 formula. Unsupported input refuses;
// no diagnostic key/query may manufacture a native lookup or Config request.
inline BnwDefaultNamespaceAsciiQueryDiagnostic
bnw_default_namespace_ascii_query_diagnostic(std::u16string_view query) {
    if (query.empty() || query.size() > 259)
        throw std::runtime_error("Finite native query requires nonempty UTF16ASCII path <=259");
    std::string normalized;
    normalized.reserve(query.size());
    for (const char16_t c : query) {
        if (c < 32 || c > 126)
            throw std::runtime_error("Query lacks finite UTF16ASCII/CP_ACP proof");
        normalized.push_back(c == u'/' ? '\\' : static_cast<char>(c));
    }
    const auto slash = normalized.rfind('\\');
    const auto colon = slash == std::string::npos ? normalized.find(':') : std::string::npos;
    const auto delimiter = slash != std::string::npos ? slash : colon;
    auto raw = delimiter == std::string::npos ? normalized : normalized.substr(delimiter + 1);
    if (raw.empty() || raw.size() > 127)
        throw std::runtime_error("Empty/long basename branch not owned by finite query helper");
    std::uint32_t crc = 0xffffffffu;
    for (const char byte : raw) {
        auto c = static_cast<unsigned char>(byte);
        if (c >= 'A' && c <= 'Z') c = static_cast<unsigned char>(c + ('a' - 'A'));
        crc ^= c;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0u);
    }
    return {std::move(raw), crc}; // raw CRC, no final complement or filename equality.
}
template<class Map> const typename Map::mapped_type*
bnw_default_namespace_crc_selection_diagnostic(const Map& records,
                                                const BnwDefaultNamespaceAsciiQueryDiagnostic& query) {
    const auto selected = records.find(query.crc);
    return selected == records.end() ? nullptr : &selected->second;
}

struct BnwNativeDefaultNamespaceInitialReceiptDiagnostic {
    std::uint8_t constructor_archive_priority = 0, constructor_enabled = 0;
    std::size_t constructor_prefix_filter_count = 0;
};
// Retains INITIAL constructor state and source identity only. It cannot quote
// CURRENT enable/stack state or issue a snapshot/namespace lookup/read/Config.
class BnwNativeDefaultNamespacePolicyOwner final {
  public:
    ~BnwNativeDefaultNamespacePolicyOwner();
    BnwNativeDefaultNamespacePolicyOwner(const BnwNativeDefaultNamespacePolicyOwner&) = delete;
    BnwNativeDefaultNamespacePolicyOwner& operator=(const BnwNativeDefaultNamespacePolicyOwner&) = delete;
    void require_current() const;
    void require_same_input(const BnwNativeColdFilesystemOwner&) const;
    BnwNativeDefaultNamespaceInitialReceiptDiagnostic initial_receipt() const;
  private:
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;

    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_same_input_completed(const BnwNativeColdFilesystemOwner&,const BnwNativeCompletedColdSourceLoan&)const;
    BnwNativeDefaultNamespaceInitialReceiptDiagnostic initial_receipt_completed(
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    // const-ref avoids a second hidden Cold factory pin. Arena owns ONE by-value
    // Cold input and checks its callback-free early phase before/after this call.
    static std::shared_ptr<const BnwNativeDefaultNamespacePolicyOwner> acquire(
        const std::shared_ptr<const BnwNativeColdFilesystemOwner>&);
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeDefaultNamespacePolicyOwner(std::unique_ptr<Impl>);
};
}
