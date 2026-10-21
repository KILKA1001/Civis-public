#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <utility>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutBaseline;
class NotificationLayoutRuntimeDimensions;
class BnwNormalButtonSubobject118Backing;
class NotificationLayoutBoxState;
class NotificationLayoutParentReceiver;
// OWN actual common58 storage: only the proved native564300 issuer constructs.
class NotificationLayoutControlID58 final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutBoxState;
    friend class NotificationLayoutParentReceiver;
    const std::array<char,32> bytes;
    const std::uint32_t length;
    NotificationLayoutControlID58(std::array<char,32> data,std::uint32_t size):bytes(data),length(size){}
public:
    ~NotificationLayoutControlID58()=default;
};
// Actual canonical receiver identity, not a node ID, XML DTO or serial receipt.
class NotificationLayoutOrdinaryReceiver final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutParentReceiver;
    friend class BnwNormalButtonSubobject118Backing;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutOrdinaryReceiver(std::shared_ptr<Impl> value):impl_(std::move(value)){}
public:
    ~NotificationLayoutOrdinaryReceiver();
    NotificationLayoutOrdinaryReceiver(const NotificationLayoutOrdinaryReceiver&)=delete;
    NotificationLayoutOrdinaryReceiver& operator=(const NotificationLayoutOrdinaryReceiver&)=delete;
};
// Native495E10/78 owner. Non-NULL even for the genuine zero-child branch.
class NotificationLayoutOrdinaryChildren78 final {
    friend class NotificationLayoutStandaloneLabelState;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutParentReceiver;
    struct Impl;
    std::shared_ptr<Impl> impl_; //Actual storage owner; only Tree listv0 may withdraw it.
    explicit NotificationLayoutOrdinaryChildren78(std::shared_ptr<Impl> value):impl_(std::move(value)){}
public:
    ~NotificationLayoutOrdinaryChildren78();
};
// Full common suffix return, produced AFTER real ordered factories/attach,
// OR8 and temporary cleanup. No producer accepts a caller-ready boolean.
class NotificationLayoutCommonChildFactoryReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutParentReceiver;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutCommonChildFactoryReturn(std::shared_ptr<Impl> value):impl_(std::move(value)){}
public:
    ~NotificationLayoutCommonChildFactoryReturn();
    NotificationLayoutCommonChildFactoryReturn(const NotificationLayoutCommonChildFactoryReturn&)=delete;
    NotificationLayoutCommonChildFactoryReturn& operator=(const NotificationLayoutCommonChildFactoryReturn&)=delete;
};
}
