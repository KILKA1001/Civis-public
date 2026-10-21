#pragma once
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::assets {
class CivisResourceBankOwner;
namespace detail {
//One actual winning, already-loaded loose byte payload. Path comes from its
//held SourcePin, never source_recipe/AssetCandidate/prepared Lua filename.
//This resource lender grants no Original821370/CRC/packed-record address,
//native7493B0 caller, script setup/load, Context initializer or listener seal.
class CivisLoadedBytesPhysicalPathLoan final {
    friend class ::civ5::assets::CivisResourceBankOwner;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit CivisLoadedBytesPhysicalPathLoan(std::shared_ptr<Impl>);
    void require_current_without_callbacks()const;
    void require_same_bank_without_callbacks(const CivisResourceBankOwner&)const;
    std::filesystem::path physical_path_owned()const;
    std::shared_ptr<const Bytes> actual_bytes_owned()const;
    runtime::FileIdentity payload_identity_owned()const;
public:
    ~CivisLoadedBytesPhysicalPathLoan();
    CivisLoadedBytesPhysicalPathLoan(const CivisLoadedBytesPhysicalPathLoan&)=delete;
    CivisLoadedBytesPhysicalPathLoan&operator=(const CivisLoadedBytesPhysicalPathLoan&)=delete;
};
}
}
