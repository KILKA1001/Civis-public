#pragma once
#include "civ5/runtime/script_ui_control_graph_backend.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
namespace civ5::runtime {
class ScriptUiControlGraph;class BnwSelectedStockLuaBootstrap;
class BnwSelectedUnitFlagContextOwner;
class BnwWorldViewScriptUiAdapter final : public ScriptUiControlGraphBackend {
 public:
    ~BnwWorldViewScriptUiAdapter();
    BnwWorldViewScriptUiAdapter(const BnwWorldViewScriptUiAdapter&)=delete;
    BnwWorldViewScriptUiAdapter& operator=(const BnwWorldViewScriptUiAdapter&)=delete;
    void require_current()const override;
    void retire()override;void poison()override;
    std::vector<ScriptUiGraphBackendNamed> static_controls()const override;
    ScriptUiGraphBackendNode inspect(std::uint64_t)const override;
    std::vector<ScriptUiGraphBackendNamed> build(std::string_view,std::uint64_t)override;
    void set_hidden(std::uint64_t,bool)override;
    void change_parent(std::uint64_t,std::uint64_t)override;
    void destroy_child(std::uint64_t,std::uint64_t)override;
    void set_alpha(std::uint64_t,double)override;
    void set_disabled(std::uint64_t,bool)override;
    void set_void1(std::uint64_t,std::int32_t)override;
    void set_void2(std::uint64_t,std::int32_t)override;
    void set_consume_mouse_over(std::uint64_t,bool)override;
    void set_text(std::uint64_t,std::string)override;
    void set_tooltip(std::uint64_t,std::string)override;
    void set_world_position(std::uint64_t,std::array<double,3>)override;
    void set_offset(std::uint64_t,std::array<double,2>)override;
    void set_offset_x(std::uint64_t,double)override;
    void set_color(std::uint64_t,std::array<double,4>)override;
    void set_fg_color(std::uint64_t,std::array<double,4>)override;
    void set_texture(std::uint64_t,std::string)override;
    void set_texture_offset(std::uint64_t,std::array<double,2>)override;
    void set_percent(std::uint64_t,double)override;
    void animation_set_to_beginning(std::uint64_t)override;
    void animation_play(std::uint64_t)override;
    void calculate_size(std::uint64_t)override;
    void reprocess_anchoring(std::uint64_t)override;
    std::array<double,2> get_offset_val(std::uint64_t)const override;
    double get_size_y(std::uint64_t)const override;
    void set_size_y(std::uint64_t,double)override;
    void calculate_internal_size(std::uint64_t)override;
    double get_ratio(std::uint64_t)const override;
    std::uint64_t lookup_control(std::string_view)const override;
    std::uint64_t get_button(std::uint64_t)const override;
    void register_callback(std::uint64_t,std::int64_t,std::shared_ptr<const ScriptUiOwnedControlCallback>)override;
    ScriptUiEventResult invoke_callback(std::uint64_t,std::int64_t,std::span<const ScriptValue>)override;
 private:
    friend class LuaScriptHost;
    static void require_initializing_button_core_owned(const std::shared_ptr<ScriptUiControlGraph>&,
        const std::weak_ptr<const void>&,const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&,
        std::uint64_t,std::uint64_t);
    static std::shared_ptr<ScriptUiControlGraph> build_context_initial_lua_owned(
        const std::shared_ptr<ui::BnwWorldViewControlTree>&,const std::shared_ptr<const ScriptUiChildSource>&,
        const LuaScriptHost&,std::vector<std::int64_t>,const BnwWorldViewSourceOperationLoan&);
    static void require_initial_lua_core_owned(const std::shared_ptr<ScriptUiControlGraph>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ScriptUiChildSource>&,
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&);
    friend class ScriptUiControlGraph;
    friend class BnwUnitFlagHealthInstanceLoan;
    static ui::BnwWorldViewControlTree::Control unitflag_health_control_owned(
        const std::shared_ptr<ScriptUiControlGraph>&,std::uint64_t,std::uint64_t,
        const BnwSelectedWorldViewContextOwner&);
    void require_unitflag_health_current_owned(std::uint64_t,BnwUnitFlagHealthCommand)const;
    static bool context_hidden_owned(const std::shared_ptr<ScriptUiControlGraph>&,
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&);
    static void register_context_showhide_owned(const std::shared_ptr<ScriptUiControlGraph>&,
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&);
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwWorldViewNativeContextIssuer;
    friend class BnwSelectedUnitFlagContextOwner;
    static void bind_selected_container_owned(const std::shared_ptr<ScriptUiControlGraph>&,const std::shared_ptr<ui::BnwWorldViewControlTree>&);
    struct State;const std::shared_ptr<State> state_;
    explicit BnwWorldViewScriptUiAdapter(std::unique_ptr<State>);
    // Genuine private Bootstrap supplies SAME tree/source/VM and actual selected
    // enum values while the native context owner lends a fresh typed operation.
    // No persisted old lease, arbitrary VM-current closure or readiness boolean.
    static std::shared_ptr<ScriptUiControlGraph> build_owned(
        std::shared_ptr<ui::BnwWorldViewControlTree>,std::shared_ptr<const ScriptUiChildSource>,
        LuaScriptHost&,
        std::vector<std::int64_t> selected_mouse_callback_events);
};
}
