#pragma once
#include <memory>
#include <string>
namespace civ5::runtime { class BnwWorldViewSourceOperationLoan; }
namespace civ5::ui { class BnwWorldViewControlTree; }
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
// Identity of ONE actually allocated portable Context. It is not a parsed3C
// store, GUI/name alias, constructor-return token or initialization receipt.
class NotificationLayoutContextRealmRef final {
    friend class BnwPositionedLabelRunLoan;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutContextRealmArgumentLoan;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class NotificationLayoutStandaloneLabelState;
    friend class NotificationLayoutEmbeddedLabelState;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutContextRealmRef(std::shared_ptr<Impl>);
    void retire_owned()const noexcept;
    void require_live_run_origin_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::string default_font_style_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
    ~NotificationLayoutContextRealmRef();
    NotificationLayoutContextRealmRef(const NotificationLayoutContextRealmRef&)=delete;
    NotificationLayoutContextRealmRef& operator=(const NotificationLayoutContextRealmRef&)=delete;
};
// The actual second argument borrowed by ONE ordinary canonical parser.
// Independent of any previous/common parentButton3C assignment. No issuer is
// exposed to a caller, XML, GUI facade, baseline clone or readiness flag.
class NotificationLayoutContextRealmArgumentLoan final {
    friend class NotificationLayoutStandaloneLabelState;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutEmbeddedLabelState;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutContextRealmArgumentLoan(std::shared_ptr<Impl>);
    void retire_owned()const noexcept;
public:
    ~NotificationLayoutContextRealmArgumentLoan();
    NotificationLayoutContextRealmArgumentLoan(const NotificationLayoutContextRealmArgumentLoan&)=delete;
    NotificationLayoutContextRealmArgumentLoan& operator=(const NotificationLayoutContextRealmArgumentLoan&)=delete;
};
}
