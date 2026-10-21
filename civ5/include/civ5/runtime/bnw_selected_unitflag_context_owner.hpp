#pragma once
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <cstdint>
#include <memory>
#include <vector>
namespace civ5::ui { class BnwWorldViewControlTree; }
namespace civ5::runtime {
class LuaScriptHost;
class ScriptUiChildSource;
class ScriptUiControlGraph;
class BnwSelectedWorldViewContextOwner;
class BnwSelectedStockLuaBootstrap;
// The original InGame/BulkUI/WorldViewControls UnitFlagManager descriptor.
// WorldView lends its native operation; it is not this context's Lua parent.
// This owner neither creates flags nor acknowledges UnitShouldDimFlag delivery.
class BnwSelectedUnitFlagContextOwner final {
 public:
    ~BnwSelectedUnitFlagContextOwner();
    BnwSelectedUnitFlagContextOwner(const BnwSelectedUnitFlagContextOwner&)=delete;
    BnwSelectedUnitFlagContextOwner& operator=(const BnwSelectedUnitFlagContextOwner&)=delete;
    void require_current()const;
 private:
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwUnitFlagHealthInstanceLoan;
    friend class LuaScriptHost;
    struct State;
    struct HealthOrigin {
        std::shared_ptr<State> state;
        std::weak_ptr<BnwSelectedWorldViewContextOwner> owner;
        std::weak_ptr<const ScriptUiChildSource> source;
        std::weak_ptr<ui::BnwWorldViewControlTree> tree;
        std::weak_ptr<ScriptUiControlGraph> graph;
        const LuaScriptHost* vm;
    };
    HealthOrigin retain_health_origin_without_callbacks(const LuaScriptHost&)const;
    static void require_health_origin_without_callbacks(const HealthOrigin&,const LuaScriptHost&);
    const std::shared_ptr<State> state_;
    explicit BnwSelectedUnitFlagContextOwner(std::shared_ptr<State>);
    struct HostBindings {
        std::shared_ptr<const ScriptUiChildSource> source;
        std::shared_ptr<ScriptUiControlGraph> graph;
    };
    static std::shared_ptr<BnwSelectedUnitFlagContextOwner> build_owned(
        std::weak_ptr<BnwSelectedWorldViewContextOwner> actual_metadata_owner,
        std::shared_ptr<const ScriptUiChildSource> actual_unitflag_descriptor,
        std::shared_ptr<ui::BnwWorldViewControlTree> actual_unitflag_tree,
        LuaScriptHost& actual_vm,
        std::vector<std::int64_t> actual_selected_mouse_events);
    HostBindings bindings_for_host_owned(
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&)const;
    // Bootstrap bookends its actual graph registration/source load with this
    // method inside the native lender; a returned binding is no operation loan.
    void require_host_operation_owned(
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&)const;
};
}
