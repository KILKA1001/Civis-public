#pragma once
#include <memory>
#include <utility>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// A real OWN Context constructor occurrence only. This immutable receipt grants
// neither native GUI dimensions nor parsed parent/topology/load/Lua completion.
class NotificationLayoutContextConstructionReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationLayoutContextConstructionReturn(std::shared_ptr<const Impl> actual)
        :impl_(std::move(actual)){}
    NotificationLayoutContextConstructionReturn(const NotificationLayoutContextConstructionReturn&)=delete;
    NotificationLayoutContextConstructionReturn&operator=(const NotificationLayoutContextConstructionReturn&)=delete;
public:
    ~NotificationLayoutContextConstructionReturn()=default;
};
}
