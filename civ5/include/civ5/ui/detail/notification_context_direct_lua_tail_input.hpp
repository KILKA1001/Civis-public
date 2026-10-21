#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextPostreadRootCDataReturn;
// Issued only by the direct caller after actual5761 and before original Lua.
// Owns the current private frame and real writer journals, never parser input.
class NotificationContextDirectLuaTailInput final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextDirectLuaTailInput(std::shared_ptr<const Impl>);
public:
    ~NotificationContextDirectLuaTailInput();
    NotificationContextDirectLuaTailInput(const NotificationContextDirectLuaTailInput&)=delete;
    NotificationContextDirectLuaTailInput&operator=(const NotificationContextDirectLuaTailInput&)=delete;
    void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_root_cdata_return_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextPostreadRootCDataReturn&)const;
    std::shared_ptr<::civ5::ui::BnwWorldViewControlTree> retain_actual_tree_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::weak_ptr<const void> actual_core_identity_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
};
}}
