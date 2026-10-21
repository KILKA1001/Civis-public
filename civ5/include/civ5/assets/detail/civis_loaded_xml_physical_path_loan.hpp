#pragma once
#include <filesystem>
#include <memory>
namespace civ5::content {struct XmlNode;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::assets {
class CivisResourceBankOwner;
namespace detail {
// Physical DATA of one already-loaded, winning loose XML. This does not
// grant Original RootRead, watcher, Lua setup, child factory, or activation.
class CivisLoadedXmlPhysicalPathLoan final {
    friend class ::civ5::assets::CivisResourceBankOwner;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit CivisLoadedXmlPhysicalPathLoan(std::shared_ptr<Impl>);
    void require_current_without_callbacks()const;
    void require_same_bank_without_callbacks(const CivisResourceBankOwner&)const;
    std::filesystem::path physical_path_owned()const;
    std::shared_ptr<const content::XmlNode> actual_xml_owned()const;
public:
    ~CivisLoadedXmlPhysicalPathLoan();
    CivisLoadedXmlPhysicalPathLoan(const CivisLoadedXmlPhysicalPathLoan&)=delete;
    CivisLoadedXmlPhysicalPathLoan& operator=(const CivisLoadedXmlPhysicalPathLoan&)=delete;
};
}
}
