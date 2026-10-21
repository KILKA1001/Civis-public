#pragma once
#include "civ5/ui/detail/notification_context_gui_port.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
// Actual457652 GUI value loan. The sole issuer is the entered Tree postread
// caller after returned root CData. Its FIRST/FULL closures are used only in
// this constructor; no direct-load frame or retired operation is retained.
class NotificationContextUiManagerValueLoan final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::weak_ptr<NotificationContextGuiPort> gui_;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source_;
    NotificationContextUiManagerValueLoan(
        const std::shared_ptr<NotificationContextGuiPort>& actual_gui,
        const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner>& actual_source,
        const runtime::BnwWorldViewSourceOperationLoan& current_operation,
        const std::function<void()>& first,const std::function<void()>& full)
        :gui_(actual_gui),source_(actual_source){
        if(!actual_gui||!actual_source||!first||!full)
            throw std::runtime_error("UIManager value requires actual GUI/Source and postread caller");
        first();full();
        if(&actual_source->current_operation_loan()!=&current_operation)
            throw std::runtime_error("UIManager value actual Source operation replaced during capture");
        current_operation.require_current();first();
        actual_gui->require_current_owned(current_operation);first();full();
        if(&actual_source->current_operation_loan()!=&current_operation)
            throw std::runtime_error("UIManager value actual Source operation replaced at capture POST");
        current_operation.require_current();first();
    }
    // Called only in Host's outer C++ lender, before/after its protected Lua
    // body. The operation is genuinely fresh; purpose/render/GUI/window guards
    // remain in the canonical GUI port and are never renewed by this loan.
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        if(creator_!=std::this_thread::get_id())
            throw std::runtime_error("UIManager value foreign creator before owner pins");
        const auto source=source_.lock();const auto gui=gui_.lock();
        if(!source||!gui||
            &source->current_operation_loan()!=&operation)
            throw std::runtime_error("UIManager actual GUI/Source value retired or foreign");
        operation.require_current();gui->require_current_owned(operation);operation.require_current();
        if(&source->current_operation_loan()!=&operation)
            throw std::runtime_error("UIManager actual Source operation replaced at value POST");
    }
    std::array<std::int32_t,2> screen_size_owned(
        const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        require_current_owned(operation);const auto gui=gui_.lock();
        if(!gui)throw std::runtime_error("UIManager GUI value retired before644/648 read");
        const auto result=gui->dimensions_without_callbacks(operation);
        require_current_owned(operation);return result;
    }
public:
    ~NotificationContextUiManagerValueLoan()=default;
    NotificationContextUiManagerValueLoan(const NotificationContextUiManagerValueLoan&)=delete;
    NotificationContextUiManagerValueLoan&operator=(const NotificationContextUiManagerValueLoan&)=delete;
};
}}
