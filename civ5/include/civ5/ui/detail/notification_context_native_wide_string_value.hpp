#pragma once
#include <array>
#include <memory>
#include <string>
#include <utility>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextLoadNameLoan;
// Immutable OWN UTF16 VALUE, not a constructor/load/Source authority.
// Empty values share one physical terminator across Context fields, preserving
// the native canonical-empty data-pointer relation under portable helpers.
class NotificationContextNativeWideStringValue final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextLoadNameLoan;
    inline static constexpr std::array<char16_t,1> shared_empty_{{u'\0'}};
    const std::u16string value_;
    NotificationContextNativeWideStringValue():value_(){}
    explicit NotificationContextNativeWideStringValue(std::u16string value):value_(std::move(value)){}
    const char16_t* terminated_data_without_callbacks()const noexcept{
        return value_.empty()?shared_empty_.data():value_.c_str();
    }
public:
    ~NotificationContextNativeWideStringValue()=default;
    NotificationContextNativeWideStringValue(const NotificationContextNativeWideStringValue&)=delete;
    NotificationContextNativeWideStringValue&operator=(const NotificationContextNativeWideStringValue&)=delete;
};
}
