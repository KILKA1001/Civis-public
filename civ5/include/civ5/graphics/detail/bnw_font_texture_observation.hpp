#pragma once
#include <memory>
namespace civ5::assets {struct CivisLoadedResource;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;namespace detail {class BnwFontResourceLoan;}}
namespace civ5::ui {class UiScreenGlRenderer;namespace detail {class BnwFontTextureRequest;class NotificationContextGuiPort;}}
namespace civ5::graphics::detail {
class BnwFontTextureObservation;
class BnwFontTextureBorrower final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::detail::NotificationContextGuiPort;
    friend class BnwFontTextureObservation;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit BnwFontTextureBorrower(std::shared_ptr<const Impl>);
    struct SourceViewPin;
    static void require_metadata_current_owned(const std::shared_ptr<const Impl>&,const SourceViewPin&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const runtime::detail::BnwFontResourceLoan&,
        const ui::detail::BnwFontTextureRequest&);
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const runtime::detail::BnwFontResourceLoan&,
        const ui::detail::BnwFontTextureRequest&)const;
    assets::CivisLoadedResource captured_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const runtime::detail::BnwFontResourceLoan&,
        const ui::detail::BnwFontTextureRequest&)const;
public:
    BnwFontTextureBorrower(const BnwFontTextureBorrower&)=delete;
    ~BnwFontTextureBorrower();
};
class BnwFontTextureObservation final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::detail::NotificationContextGuiPort;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    struct Impl;
    const std::unique_ptr<Impl> impl_;
    explicit BnwFontTextureObservation(std::unique_ptr<Impl>);
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const runtime::detail::BnwFontResourceLoan&,
        const ui::detail::BnwFontTextureRequest&)const;
    bool observed_null_owned()const;
    std::shared_ptr<const BnwFontTextureBorrower> borrower_owned()const;
public:
    BnwFontTextureObservation(const BnwFontTextureObservation&)=delete;
    ~BnwFontTextureObservation();
};
}
