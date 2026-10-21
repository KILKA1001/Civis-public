#pragma once
#include "civ5/ui/detail/notification_context_root_default_fields_return.hpp"
#include "civ5/ui/detail/notification_context_lua_controls_named_retirement.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/assets/detail/civis_loaded_xml_physical_path_loan.hpp"
#include "civ5/ui/detail/notification_context_root_file_watcher.hpp"
#include "civ5/ui/detail/notification_context_root_small_ui_policy_loan.hpp"
#include "civ5/ui/detail/notification_context_root_child_attach_return.hpp"
#include "civ5/ui/detail/notification_context_root_child_attach_store_loan.hpp"
#include "civ5/ui/detail/notification_context_root_read_completion_slot.hpp"
#include "civ5/ui/detail/notification_layout_native_child_controls_return.hpp"
#include "civ5/ui/detail/notification_context_lua_load_setup_return.hpp"
#include "civ5/ui/detail/notification_context_load_name_loan.hpp"
#include "civ5/ui/detail/notification_context_root_read_b4_guard.hpp"
#include "civ5/ui/detail/notification_context_native_constructor_return.hpp"
#include "civ5/ui/detail/notification_context_native_constructor_prefix.hpp"
#include "civ5/ui/detail/notification_layout_context_ordinary_state.hpp"
#include "civ5/ui/detail/notification_context_root_read_children_loan.hpp"
#include "civ5/ui/detail/notification_context_root_ordinary_factory_return.hpp"
#include "civ5/ui/detail/notification_context_child_controls_export_loan.hpp"
#include "civ5/ui/detail/notification_context_child_controls_export_return.hpp"
#include "civ5/ui/detail/notification_context_root_ordinary_controls_export_loan.hpp"
#include "civ5/ui/detail/notification_context_root_ordinary_controls_export_return.hpp"
#include "civ5/ui/detail/notification_context_lua_state_preparation.hpp"
#include "civ5/ui/detail/notification_context_initial_lua_gc_return.hpp"
#include "civ5/ui/detail/notification_context_direct_lua_tail_input.hpp"
#include "civ5/ui/detail/notification_context_direct_load_return.hpp"
#include "civ5/ui/detail/notification_context_cursor_restore_return.hpp"
#include "civ5/ui/detail/notification_cpu_geometry.hpp"
#include "civ5/ui/detail/notification_layout_context_initialization_return.hpp"
#include "civ5/ui/detail/notification_layout_context_construction_return.hpp"
#include "civ5/ui/detail/notification_context_gui_port.hpp"
#include "civ5/ui/detail/notification_layout_runtime_dimensions.hpp"
#include "civ5/ui/detail/notification_layout_common_child_state.hpp"
#include "civ5/ui/detail/notification_layout_child_slot10_completion.hpp"
#include "civ5/ui/detail/notification_context_root_child_attach_return.hpp"
#include "civ5/ui/detail/notification_context_root_child_attach_store_loan.hpp"
#include "civ5/ui/detail/bnw_standalone_positioned_label_run_loan.hpp"
#include "civ5/ui/detail/bnw_normal_label_paint_invocation.hpp"
#include "civ5/ui/detail/bnw_normal_gui_batch_prefix.hpp"
#include "civ5/ui/detail/bnw_normal_label_material_loan.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
#include "civ5/ui/detail/bnw_normal_nonfont_pipeline.hpp"
#include "civ5/ui/detail/bnw_normal_button_subobject118_backing.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_execution.hpp"
#include "civ5/ui/detail/bnw_normal_root_bootstrap_selection.hpp"
#include "civ5/ui/detail/bnw_normal_root_context_entry.hpp"
#include "civ5/ui/detail/bnw_normal_font_payload_prefix.hpp"
#include "civ5/ui/detail/bnw_normal_caption_fp32_rect_loan.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <span>
#include <array>
#include <variant>
#include <chrono>
#include <functional>
#include <map>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewScriptUiAdapter;class BnwWorldViewNativeContextIssuer;class BnwUnitFlagHealthInstanceLoan;enum class BnwUnitFlagHealthCommand;}
namespace civ5::ui::detail {class NotificationLayoutTextureKeeper;class NotificationLayoutParentReceiver;class NotificationLayoutResourceScope;class NotificationLayoutContextInitializationReturn;class NotificationContextSlot10LoadOrigin;class NotificationLayoutContextRealmRef;class BnwPositionedLabelRunLoan;class BnwLabelActualPaintCallFrame;class BnwNormalRootPaintCallFrame;}
namespace civ5::ui::detail {class BnwNormalCommonChildCall;class BnwNormalButtonCaptionCall;class BnwNormalButtonCaptionAppendProjection;class BnwNormalButtonCaptionPaintEmission;class BnwCompletedFontLoan;class BnwNormalEmbeddedLabelCommon56Call;class BnwNormalCommonManagerReceiverCall;}
namespace civ5::ui {
class UiScreenGlRenderer;class BnwScrollAnimationUpdateLoan;struct UiGlPaintReport;
// Full selected XML storage and canonical own control effects. No semantic
// equivalence for native PullDown/layout/animation or camera projection is asserted.
class BnwWorldViewControlTree final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class detail::NotificationContextDirectLuaTailInput;
    friend class detail::NotificationContextDirectLoadReturn;
    friend class detail::BnwNormalNonFontCurrentReceiverGuard;
    friend class detail::BnwNormalNonFontResourceBank;
    struct Core;
    struct ContextDirectLoadFrame;
    struct ContextDirectPostLuaFrame;
    class CommonChildParserFrame;
    class RootOrdinaryParserFrame;
    friend class detail::NotificationContextRootOrdinaryFactoryReturn;
    class CommonChildParserReturn;
 public:
    class Control final {
        friend class BnwWorldViewControlTree;
        friend class BnwWorldViewControlRenderPublication;
        friend class ::civ5::runtime::BnwUnitFlagHealthInstanceLoan;
        void require_same_unitflag_health_control_without_callbacks(const Control&)const;
        const std::weak_ptr<Core> owner_;const std::uint64_t id_;
        Control(const std::shared_ptr<Core>&,std::uint64_t);
     public:
        Control()=delete;
    };
    struct Instance {Control root;std::vector<std::pair<std::string,Control>> controls;};
    struct AnimationCommands {
        std::uint64_t sequence=0,reset_sequence=0,play_sequence=0;
        std::optional<std::chrono::steady_clock::time_point> reset_at,play_at;
    };
    // Value quote only: these fields cannot issue a clock or a paint grant.
    // Selected default LINEAR/LOOP profile; explicit Cycle/Function are refused.
    struct ScrollAnimationState {
        float progress=0, sample=0, speed=1, pause=0, end_pause=0, pause_remaining=0;
        bool stopped=false, reversed=false, sample_issued=false;
        std::uint64_t command_sequence=0, tick_sequence=0;
    };
    // Observation only. Only the closed actual-layout loan may publish these
    // canonical native fields; callers cannot submit rectangles/dimensions.
    struct LayoutState {
        // Native observations only. Zero/default quote values do not qualify
        // viewport, NULL handles, geometry, Source provenance or readiness.
        struct ScrollPanelFields {
            std::array<float,2> viewport_30_34{},calculated_84_88{},offset_9c_a0{};
            std::array<float,4> internal_8c_90_94_98{};
            float fraction_a4=0,ratio_a8=0,range_ac=0,range_b0=0,step_b4=0;
            std::uint32_t flags_40=0,store_mask=0;
        };
        std::optional<ScrollPanelFields> scroll_panel;
        std::array<float,2> native_offset{},calculated_size{},native_extent{},anchor_origin{};
        std::array<float,4> native_bounds{};
        std::vector<std::pair<std::uint64_t,std::array<float,2>>> child_offsets;
        std::uint64_t source_serial=0,topology_revision=0,effect_revision=0,command_sequence=0;
        bool calculate_returned=false,anchor_prepass_stored=false,anchoring_returned=false,internal_returned=false;
        std::optional<float> ratio;
    };
    struct ControlQuote {
        std::uint64_t id,parent;content::XmlNode authored;
        bool hidden,alive,dynamic,disabled;double alpha;
        std::optional<std::string> text,tooltip;
        std::uint64_t instance,effect_revision;
        std::optional<std::array<double,3>> world_position;
        std::optional<std::array<double,2>> offset;
        std::optional<std::array<double,4>> color;
        std::optional<std::array<double,4>> fg_color;
        std::optional<std::string> texture;
        std::optional<std::array<double,2>> texture_offset;
        std::optional<double> percent;
        std::optional<AnimationCommands> animation;
        std::weak_ptr<const BnwWorldViewControlTree> external_parent_tree;
        std::uint64_t external_parent_id=0;
        std::optional<LayoutState> layout;
        std::optional<float> size_y_override;
        std::optional<ScrollAnimationState> scroll_animation;
        std::optional<detail::NotificationCpuGeometry> cpu_geometry; // Own observation, not native readiness.
    };
    ~BnwWorldViewControlTree();
    BnwWorldViewControlTree(const BnwWorldViewControlTree&)=delete;
    BnwWorldViewControlTree& operator=(const BnwWorldViewControlTree&)=delete;
    void require_current()const;
    void poison()const noexcept;
    std::string context()const;
    std::shared_ptr<const runtime::ScriptUiChildSource> actual_source()const;
    std::vector<std::pair<std::string,Control>> static_controls()const;
    ControlQuote inspect_control(const Control&)const;
    Control static_control(std::string_view)const;
    Instance build_instance(std::string_view,const Control& actual_parent);
    Control get_button(const Control& actual_pulldown)const;
    void set_alpha(const Control&,double);
    void set_hidden(const Control&,bool);
    void set_disabled(const Control&,bool);
    void set_text(const Control&,std::string);
    void set_tooltip(const Control&,std::string);
    void set_world_position(const Control&,std::array<double,3>);
    void set_offset(const Control&,std::array<double,2>);
    void set_offset_x(const Control&,double);
    void set_color(const Control&,std::array<double,4>);
    void set_fg_color(const Control&,std::array<double,4>);
    void set_texture(const Control&,std::string);
    void set_texture_offset(const Control&,std::array<double,2>);
    void set_percent(const Control&,double);
    void animation_set_to_beginning(const Control&);
    void animation_play(const Control&);
    void change_parent(const Control&,const Control&);
    void destroy_child(const Control& actual_parent,const Control& actual_child);
    std::array<double,2> get_offset_val(const Control&)const;
    double get_size_y(const Control&)const;
    double get_ratio(const Control&)const;
    double inherited_alpha(const Control&)const;
    bool inherited_hidden(const Control&)const;
 private:
    static void require_lookup_context_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>&);
    static std::shared_ptr<const void> lookup_receiver_physical_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&);
    static std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> lookup_context_relative_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>&,const char*);
    friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
    friend class ::civ5::runtime::BnwWorldViewScriptUiAdapter;
    void require_context_initial_lua_graph_origin_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const runtime::ScriptUiChildSource>&,const runtime::LuaScriptHost&)const;
    void require_mouse_consume_backend_origin_without_callbacks(const Control&,
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    void set_consume_mouse_over_owned(const Control&,bool,
        const runtime::BnwWorldViewSourceOperationLoan&);
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::runtime::BnwSelectedWorldViewContextOwner;
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> collect_selected_client_retained_context_trees_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<BnwWorldViewControlTree>&)const;
    void construct_selected_nested_contexts_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    void issue_direct_root_scalar_parent_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<BnwWorldViewControlTree>&);
    static void require_direct_context_load_history_owned(const std::shared_ptr<Core>&,
        const runtime::BnwWorldViewSourceOperationLoan&);
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> borrow_direct_root_read_children_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> collect_direct_root_read_tree_facades_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<BnwWorldViewControlTree>&)const;
    std::array<std::shared_ptr<BnwWorldViewControlTree>,5> borrow_direct_loaded_native_slots_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<BnwWorldViewControlTree>&)const;
    void adopt_direct_selected_client_registry_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    void require_direct_selected_client_registry_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_direct_registry_baselines_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::map<std::string,content::XmlNode,std::less<>>&)const;
    void construct_selected_client_context_registry_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    void require_selected_client_context_registry_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<BnwWorldViewControlTree> lookup_selected_client_context_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::string_view)const;
    friend class BnwWorldViewControlRenderPublication;
    friend class UiScreenGlRenderer;
    friend class detail::NotificationLayoutParentReceiver;
    friend class detail::NotificationLayoutContextInitializationReturn;
    friend class detail::NotificationLayoutContextConstructionReturn;
    friend class detail::NotificationContextSlot10LoadOrigin;
    friend class detail::NotificationContextCursorLoadPrefixReturn;
    friend class detail::NotificationLayoutRuntimeDimensions;
    friend class detail::BnwNormalButtonSubobject118Backing;
    friend class detail::NotificationLayoutContextOrdinaryState;
    friend class detail::NotificationContextNativeLoadNameInputLoan;
    friend class detail::NotificationContextLoadNameLoan;
    friend class detail::NotificationContextRootReadB4Guard;
    friend class detail::NotificationContextRootChildAttachReturn;
    friend class detail::NotificationContextRootChildAttachStoreLoan;
    friend class detail::NotificationContextNativeConstructorPrefix;
    friend class detail::NotificationContextNativeConstructorReturn;
    friend class detail::NotificationLayoutOrdinaryReceiver;
    friend class detail::NotificationLayoutOrdinaryChildren78;
    friend class detail::NotificationLayoutCommonChildFactoryReturn;
    friend class detail::NotificationLayoutContextRealmRef;
    friend class detail::NotificationLayoutEmbeddedLabelState;
    friend class detail::NotificationLayoutStandaloneLabelState;
    friend class detail::BnwStandalonePositionedLabelRunLoan;
    friend class detail::BnwNormalLabelPaintInvocation;
    friend class detail::BnwNormalGuiBatchPrefix;
    friend class detail::BnwNormalFontPayloadPrefix;
    friend class detail::BnwNormalRootBatchHistory;
    friend class detail::BnwNormalItemOccurrence;
    friend class detail::BnwNormalLabelMaterialLoan;
    friend class detail::BnwNormalQueueDrainPermission;
    friend class detail::BnwNormalVisibleLabelCall;
    friend class detail::BnwNormalCommonChildCall;
    friend class detail::BnwNormalCommonManagerReceiverCall;
    friend class detail::BnwNormalButtonCaptionCall;
    friend class detail::BnwNormalCaptionFp32RectLoan;
    friend class detail::BnwNormalEmbeddedLabelCommon56Call;
    friend class detail::BnwNormalButtonCaptionPaintEmission;
    friend class detail::BnwNormalRootBootstrapSelection;
    friend class detail::BnwNormalRootContextEntry;
    friend class detail::BnwNormalLabelPaintEmission;
    friend class detail::BnwPositionedLabelRunLoan;
    friend class detail::NotificationContextGuiPort;
    friend class detail::NotificationLayoutResizeRegistration;
    friend class detail::NotificationLayoutChildSlot10Completion;
    friend class detail::NotificationContextInitialLuaGcReturn;
    friend class detail::NotificationContextCursorRestoreReturn;
    class LayoutResourceBinding final {
        friend class BnwWorldViewControlTree;
        const std::shared_ptr<Core> core_;
        const std::weak_ptr<detail::NotificationLayoutResourceScope> scope_;
        const std::weak_ptr<const BnwWorldViewControlTree> tree_;
        const runtime::BnwWorldViewSourceOperationLoan* const operation_;
        const std::uint64_t serial_,topology_;
        LayoutResourceBinding(std::shared_ptr<Core>,const std::shared_ptr<detail::NotificationLayoutResourceScope>&,
            const std::shared_ptr<const BnwWorldViewControlTree>&,const runtime::BnwWorldViewSourceOperationLoan&);
     public:
        void require_current_without_callbacks()const;
        ~LayoutResourceBinding()noexcept;
    };
    std::unique_ptr<LayoutResourceBinding> enter_layout_resource_scope_owned(
        const std::shared_ptr<detail::NotificationLayoutResourceScope>&,
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const BnwWorldViewControlTree>&);
    void require_layout_resource_operation_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::array<float,4> image_color_rgba_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&)const;
    struct ButtonVisualOwned {
        std::array<float,4> rgba_12c_138;
        std::array<float,2> destination_size_13c_140,pixel_size_144_148,pixel_origin_14c_150,uv_168_16c,uv_170_174;
        std::optional<std::array<std::uint32_t,2>> texture_extent_15c;
        std::shared_ptr<const detail::NotificationLayoutTextureKeeper> texture_keeper;
        std::uint32_t state_104;
        bool earned_constructor_null_178;
    };
    ButtonVisualOwned button_visual_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&)const;
    struct StackLayoutOwned {
        struct Child {
            std::shared_ptr<const BnwWorldViewControlTree> tree;
            std::uint64_t id; //0 means genuine attached Context root, not an invented Control.
            std::optional<Control> control;
            std::array<float,2> derived_offset,native_offset,native_size;
        };
        std::array<float,2> native_size,native_extent,native_offset,anchor_origin;
        std::array<float,4> native_bounds;
        std::vector<Child> children;
        bool actual_size_calculation,earned_empty_initializer;
    };
    StackLayoutOwned stack_layout_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&)const;
    void ensure_selected_root_context_initialized_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void prepare_selected_client_registry_baselines_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::map<std::string,content::XmlNode,std::less<>>& actual_full_styles);
    // Only the real renderer's SAME source/GUI/window style-capture scope may
    // call this. It captures parser/history data, never a LayoutOperandLoan.
    static void attach_existing_native_scalar_parent_chain_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    static void require_scalar_parent_attachment_owned(const std::shared_ptr<Core>&,
        const runtime::BnwWorldViewSourceOperationLoan&,bool child_expected_busy=false,bool parent_expected_busy=false);
    void initialize_scalar_parent_baselines_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::map<std::string,content::XmlNode,std::less<>>&);
    void initialize_notification_layout_baselines_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::map<std::string,content::XmlNode,std::less<>>& actual_styles,
        const BnwWorldViewControlTree& actual_action_context);
    // Root must bind the actual native/update clock and eligible target while
    // SAME source/GUI/window scopes enclose this nontransferable token. Its
    // Issuance consumes only the closed ACTIVE real renderer loop loan.
    class ScrollClockLoan final {
        friend class BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
        const std::thread::id creator_=std::this_thread::get_id();
        const std::weak_ptr<Core> tree_;
        const std::uint64_t control_id_,source_serial_,tick_sequence_;
        const runtime::BnwWorldViewSourceOperationLoan* const operation_;
        const float delta_seconds_;
        const BnwScrollAnimationUpdateLoan*const update_;
        bool alive_=true;
        ScrollClockLoan(const Control&,const runtime::BnwWorldViewSourceOperationLoan&,
            const BnwScrollAnimationUpdateLoan&);
        ~ScrollClockLoan()noexcept;
        void require_for_core_without_callbacks(const std::shared_ptr<Core>&,std::uint64_t)const;
        ScrollClockLoan(const ScrollClockLoan&)=delete;
        ScrollClockLoan& operator=(const ScrollClockLoan&)=delete;
        ScrollClockLoan(ScrollClockLoan&&)=delete;
        ScrollClockLoan& operator=(ScrollClockLoan&&)=delete;
    };
    // Producer origin is minted ONLY by the genuine construction/load owner.
    // Undefined constructor deliberately keeps initial geometry/parent/native
    // zero/subtype provenance out of XML/default/numeric caller submissions.
    class LayoutContextInitializationOrigin final {
        friend class BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
        const std::weak_ptr<Core> tree_;
        const std::uint64_t topology_,parser_serial_;
        const std::array<float,2> initial_size_,native_zero_;
        const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> construction_;
        // Separate genuine direct RootRead lineage; absent in the legacy lane.
        const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> direct_read_;
        const std::shared_ptr<ContextDirectLoadFrame> direct_caller_;
        const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> direct_constructor_;
        const std::uint64_t direct_scope_=0,direct_serial_=0,direct_before_effects_=0,direct_before_sequence_=0;
        LayoutContextInitializationOrigin(const std::shared_ptr<Core>&,
            std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn>);
        LayoutContextInitializationOrigin(const std::shared_ptr<Core>&,
            const std::shared_ptr<ContextDirectLoadFrame>&,
            const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>&);
        LayoutContextInitializationOrigin(const LayoutContextInitializationOrigin&)=delete;
        LayoutContextInitializationOrigin&operator=(const LayoutContextInitializationOrigin&)=delete;
    public:
        ~LayoutContextInitializationOrigin()=default;
    };
    // The actual child+10 dispatcher includes pertype suffixes and, for
    // nondeferred Context, its own initializer followed by original child Lua.
    // Only that dispatcher may construct this post-complete receipt.
    class LayoutChildSlot10Return final {
        friend class BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
        struct Member {std::uint64_t id,parser_serial,initialized_sequence;};
        struct Domain {std::weak_ptr<Core> core;std::uint64_t topology,before_effects,effects;};
        const std::weak_ptr<Core> parent_,child_;
        const std::uint64_t child_id_,child_order_,source_serial_;
        const std::vector<Member> members_;
        const std::vector<Domain> domains_;
        const std::shared_ptr<const detail::NotificationLayoutChildSlot10Completion> completion_;
        LayoutChildSlot10Return(const std::shared_ptr<Core>&,const std::shared_ptr<Core>&,
            std::uint64_t,const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const detail::NotificationLayoutChildSlot10Completion>&);
        LayoutChildSlot10Return(const LayoutChildSlot10Return&)=delete;
        LayoutChildSlot10Return&operator=(const LayoutChildSlot10Return&)=delete;
    public:
        LayoutChildSlot10Return(LayoutChildSlot10Return&&)=default;
        ~LayoutChildSlot10Return()=default;
    };
    static std::shared_ptr<detail::NotificationContextGuiPort> capture_context_gui_port_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    void set_embedded_label_text_live_owned(const Control&,std::string);
    static void run_layout_resize_98_owned(const detail::NotificationLayoutResizeRegistration&,
        const runtime::BnwWorldViewSourceOperationLoan&);
    static void require_box_native_common_return_owned(const std::shared_ptr<Core>&,std::uint64_t,
        const runtime::BnwWorldViewSourceOperationLoan&,bool expected_busy=true);
    static void require_button_native_common_return_owned(const std::shared_ptr<Core>&,std::uint64_t,
        const runtime::BnwWorldViewSourceOperationLoan&,bool expected_busy=true);
    detail::BnwNormalNonFontPaintInvocation capture_normal_common_button_invocation_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&,
        const BnwWorldViewControlRenderPublication&,const detail::BnwNormalRootPaintCallFrame&)const;
    static void require_nonfont_receiver_inputs_without_callbacks(
        const detail::BnwNormalNonFontCurrentReceiverGuard::Impl&);
    static void require_nonfont_bank_inputs_without_callbacks(
        const detail::BnwNormalNonFontResourceBank::Impl&);
    static void require_nonfont_bank_current_without_callbacks(
        const detail::BnwNormalNonFontResourceBank::Impl&);
    void require_normal_nonfont_button_receiver_ready_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&,
        const detail::BnwNormalRootPaintCallFrame::Guard&,const detail::BnwNormalRootBatchHistory::Guard&,
        std::shared_ptr<const BnwWorldViewControlRenderPublication>)const;
    static void require_stack_native_common_return_owned(const std::shared_ptr<Core>&,std::uint64_t,
        const runtime::BnwWorldViewSourceOperationLoan&,bool expected_busy=true);
    std::unique_ptr<CommonChildParserReturn> capture_layout_node_canonical_owned(std::uint64_t,
        const std::map<std::string,content::XmlNode,std::less<>>&,std::uint64_t);
    void prepare_context_cursor_load_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn> require_context_cursor_load_prefix_owned(
        const std::shared_ptr<Core>&,const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<const detail::NotificationContextLuaStatePreparation> prepare_context_lua_state_owned(
        const std::shared_ptr<Core>&,const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>&);
    static void require_context_initial_lua_gc_return_owned(const std::shared_ptr<Core>&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>&);
    static void require_context_lua_preparation_owned(const std::shared_ptr<Core>&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextLuaStatePreparation>&);
    void prepare_context_slot10_load_origin_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<const detail::NotificationContextSlot10LoadOrigin> require_context_slot10_load_origin_owned(
        const std::shared_ptr<Core>&,const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<Core> require_context_construction_origin_owned(const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn>&);
    struct ContextInitializationFrame;
    struct ContextChildSlot10Frame;
    // Root construction/load owner supplies this opaque actual457270 prefix.
    // Its constructor/issuer and original protected initial-Lua body are
    // distinct mandatory providers, not callback/bool or XML substitutes.
    using LayoutContextSlot10LoadOrigin=detail::NotificationContextSlot10LoadOrigin;
    enum class LayoutContextSlot10LoadBranch {deferred,nondeferred};
    static std::shared_ptr<const LayoutContextSlot10LoadOrigin> prepare_context_slot10_load_owned(
        ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    static LayoutContextSlot10LoadBranch require_context_slot10_load_origin_owned(
        ContextChildSlot10Frame&,const std::shared_ptr<const LayoutContextSlot10LoadOrigin>&);
    static std::shared_ptr<BnwWorldViewControlTree> retain_context_slot10_tree_owned(
        ContextChildSlot10Frame&,const std::shared_ptr<const LayoutContextSlot10LoadOrigin>&);
    static void invoke_context_post_lua_showhide_owned(ContextChildSlot10Frame&,const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>&,
        const std::shared_ptr<const detail::NotificationContextCursorRestoreReturn>&);
    static void finish_context_post_initial_lua_owned(ContextChildSlot10Frame&,const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>&);
    static void execute_context_slot10_initial_lua_owned(ContextChildSlot10Frame&,
        const std::shared_ptr<const LayoutContextSlot10LoadOrigin>&,
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&);
    static void bind_scroll_slot10_callback_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t,std::uint64_t,std::uint32_t);
    static void initialize_control_5761_child_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    static void dispatch_context_child_slot10_body_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    // Separate Mouse-authored actual6F6C10 provider; no accepting stub here.
    std::shared_ptr<const detail::BnwPositionedLabelRunLoan> positioned_label_run_owned(Control,const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const detail::BnwPositionedLabelRunLoan> positioned_label_run_for_paint_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,Control,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwLabelActualPaintCallFrame&)const;
    static void initialize_button_slot10_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    //Only actual Root-owned673730 StoreLoan supplies inherited44BF40/ContextTHIS67373E VALUE.
    void store_standalone_root_attached_context_3c_owned(
        const detail::NotificationContextRootChildAttachStoreLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationLayoutOrdinaryReceiver&);
    class NormalGuiLabelDispatchOwned final {
        friend class BnwWorldViewControlTree;friend class UiScreenGlRenderer;
        enum class Outcome {other_control,button_no_caption,standalone_label};
        const Outcome outcome;
        const std::shared_ptr<const detail::BnwNormalLabelPaintInvocation> invocation;
        NormalGuiLabelDispatchOwned(Outcome value,std::shared_ptr<const detail::BnwNormalLabelPaintInvocation> call)
            :outcome(value),invocation(std::move(call)){}
    };
    NormalGuiLabelDispatchOwned dispatch_normal_gui_label_paint_owned(std::shared_ptr<const BnwWorldViewControlTree>,
        Control,const runtime::BnwWorldViewSourceOperationLoan&)const;
    NormalGuiLabelDispatchOwned dispatch_normal_gui_label_paint_owned(std::shared_ptr<const BnwWorldViewControlTree>,
        Control,const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootContextEntry&)const;
    NormalGuiLabelDispatchOwned dispatch_normal_gui_label_paint_impl_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,Control,
        const runtime::BnwWorldViewSourceOperationLoan&,
        std::optional<detail::BnwNormalRootContextEntry>)const;
    void require_normal_paint_no_label_invocation_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> windowed_renderer_matrix_data_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,const runtime::BnwWorldViewSourceOperationLoan&)const;
    //Closed issuer: original ACTIVE Frame/Matrix and executed Prefix are
    //captured once BEFORE Common traversal. Historical health is separate.
    detail::BnwNormalRootBatchHistory::Guard capture_normal_root_batch_history_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,const detail::BnwLabelActualPaintCallFrame&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalGuiBatchPrefix&)const;
    detail::BnwNormalRootBatchHistory::Guard capture_normal_root_batch_history_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,const detail::BnwNormalRootPaintCallFrame&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalGuiBatchPrefix&)const;
    std::unique_ptr<detail::BnwNormalRootBatchExecution> begin_normal_root_batch_execution_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,std::size_t,const detail::BnwLabelActualPaintCallFrame&)const;
    std::unique_ptr<detail::BnwNormalRootBatchExecution> begin_normal_root_batch_execution_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,std::size_t,const detail::BnwNormalRootPaintCallFrame&)const;
    std::unique_ptr<detail::BnwNormalRootBatchExecution> begin_normal_root_batch_execution_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,const detail::BnwNormalRootPaintCallFrame&)const;
    template<class ActualFrameGuard>
    std::unique_ptr<detail::BnwNormalRootBatchExecution> begin_normal_root_batch_execution_from_frame_guard_impl_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,std::optional<std::size_t>,const ActualFrameGuard&)const;
    //Internal captured-Guard delegate preserves the ORIGINAL Frame
    //and avoids raw facade access after Matrix/Prefix callbacks.
    detail::BnwNormalRootBatchHistory::Guard capture_normal_root_batch_history_from_frame_guard_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwWorldViewControlRenderPublication&,
        const detail::BnwLabelActualPaintCallFrame::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalGuiBatchPrefix&)const;
    detail::BnwNormalRootBatchHistory::Guard capture_normal_root_batch_history_from_frame_guard_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwWorldViewControlRenderPublication&,
        const detail::BnwNormalRootPaintCallFrame::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalGuiBatchPrefix&)const;
    template<class ActualFrameGuard>
    detail::BnwNormalRootBatchHistory::Guard capture_normal_root_batch_history_from_frame_guard_impl_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwWorldViewControlRenderPublication&,
        const ActualFrameGuard&,std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalGuiBatchPrefix&)const;
    //Distinct CPU occurrence earned under original ACTIVE item/DATA
    //after actual Binding attach; no Atlas/planes/drain grant here.
    detail::BnwNormalItemOccurrence::Guard capture_normal_item_occurrence_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        const BnwWorldViewControlRenderPublication&,std::size_t,const detail::BnwLabelActualPaintCallFrame&,
        const detail::BnwLabelPaintTextLoan&,const detail::BnwLabelPaintPositionedData&)const;
    detail::BnwNormalItemOccurrence::Guard capture_normal_item_occurrence_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        const BnwWorldViewControlRenderPublication&,std::size_t,const detail::BnwLabelActualPaintCallFrame&,
        const detail::BnwLabelPaintTextLoan&,const detail::BnwLabelPaintPositionedData&,
        const detail::BnwNormalButtonCaptionCall&)const;
    static void require_normal_caption_item_physical_without_callbacks(const detail::BnwNormalItemOccurrence::Impl&);
    detail::BnwNormalRootBootstrapSelection capture_normal_root_bootstrap_selection_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_normal_root_bootstrap_selection_call_without_callbacks(
        const detail::BnwNormalRootBootstrapSelection&,const detail::BnwNormalRootBatchHistory::Guard&,
        const detail::BnwNormalVisibleLabelCall&)const;
    static void require_normal_root_bootstrap_selection_current_without_callbacks(
        const detail::BnwNormalRootBootstrapSelection::Impl&);
    static void require_normal_root_bootstrap_selection_publication_without_callbacks(
        const detail::BnwNormalRootBootstrapSelection::Impl&,const BnwWorldViewControlRenderPublication&);
    static void require_normal_root_receiver_before_common_without_callbacks(
        const std::shared_ptr<Core>&,const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t);
    detail::BnwNormalRootContextEntry capture_normal_root_context_entry_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,const runtime::BnwWorldViewSourceOperationLoan&)const;
    UiGlPaintReport with_normal_root_context_entry_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const std::function<UiGlPaintReport(const detail::BnwNormalRootContextEntry&)>>)const;
    static void require_normal_root_context_entry_inputs_without_callbacks(
        const detail::BnwNormalRootContextEntry::Impl&);
    static void require_normal_gui_batch_receiver_without_callbacks(
        const detail::BnwNormalGuiBatchPrefix::Impl&);
    static void require_normal_root_history_context_without_callbacks(
        const detail::BnwNormalRootBatchHistory::Impl&);
    static std::uint32_t require_normal_root_ordered_child_flags_without_callbacks(
        const std::shared_ptr<Core>&,const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&);
    using NormalVisibleLabelConsumer=std::function<std::unique_ptr<detail::BnwNormalLabelMaterialLoan>(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,const detail::BnwNormalVisibleLabelCall&)>;
    // First child consumes the SAME already-open original paint; no nested paint.
    void bootstrap_normal_root_visible_label_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootBatchExecution&,const detail::BnwLabelActualPaintCallFrame&,
        std::shared_ptr<const NormalVisibleLabelConsumer>)const;
    // Continuation is legal only after that original paint has returned.
    void stage_normal_root_visible_labels_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootBatchExecution&,std::shared_ptr<const NormalVisibleLabelConsumer>)const;
    void execute_normal_visible_label_child_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::BnwNormalRootBatchExecution::Impl>&,
        const std::shared_ptr<const NormalVisibleLabelConsumer>&,
        const detail::BnwLabelActualPaintCallFrame::Guard*)const;
    static void require_normal_visible_label_call_current_without_callbacks(const detail::BnwNormalVisibleLabelCall::Impl&);
    static void require_normal_visible_label_call_publication_without_callbacks(
        const detail::BnwNormalVisibleLabelCall::Impl&,const BnwWorldViewControlRenderPublication&);
    static void require_normal_root_epoch_prefix_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&);
    static void require_normal_root_epoch_prefix_source_owned(
        const detail::BnwNormalGuiBatchPrefix::Impl&,const runtime::BnwWorldViewSourceOperationLoan&);
    static void require_normal_pending_label_item_without_callbacks(
        const detail::BnwNormalGuiBatchPrefix::Impl&,const detail::BnwNormalItemOccurrence::Guard&);
    static void require_normal_payload_pending_label_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&,
        const detail::BnwLabelPaintPositionedData::Guard&,const detail::BnwLabelPaintTextLoan::Guard&);
    static void require_normal_payload_pending_caption_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&,
        const detail::BnwLabelPaintPositionedData::Guard&,const detail::BnwLabelPaintTextLoan::Guard&,
        const detail::BnwNormalButtonCaptionCall&,const detail::BnwNormalItemOccurrence::Guard&);
    static void require_normal_payload_caption_source_owned(const detail::BnwNormalGuiBatchPrefix::Impl&,
        const detail::BnwLabelPaintPositionedData::Guard&,const detail::BnwLabelPaintTextLoan::Guard&,
        const detail::BnwNormalButtonCaptionCall&,const detail::BnwNormalItemOccurrence::Guard&,
        const runtime::BnwWorldViewSourceOperationLoan&);
    friend class detail::BnwNormalRootBatchExecution;
    struct NormalCommonTraversal;
    struct NormalCommonNullChildReturn;
    struct NormalCommonRecursiveReturn;
    struct NormalCommonChild18Return;
    struct NormalContextChildBodyPublicationState;
    static std::shared_ptr<const NormalContextChildBodyPublicationState>
        capture_normal_context_child_body_publication_state_owned(
            const detail::BnwNormalCommonChildCall&,const std::shared_ptr<BnwWorldViewControlTree>&,
            const Control&,const detail::BnwNormalRootContextEntry&,const UiScreenGlRenderer&,
            const std::shared_ptr<const ComposedUiFrame>&);
    using NormalCommonLabelConsumer=std::function<std::unique_ptr<detail::BnwNormalLabelMaterialLoan>(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalCommonChildCall&,const detail::BnwNormalVisibleLabelCall&)>;
    using NormalCommonButtonOwnConsumer=std::function<detail::BnwNormalNonFontOwnReturn(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalCommonChildCall&)>;
    using NormalCommonCaptionConsumer=std::function<std::unique_ptr<detail::BnwNormalLabelMaterialLoan>(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
        const detail::BnwNormalCommonChildCall&,const detail::BnwNormalButtonCaptionCall&)>;
    struct NormalCommonChildConsumers final {
        const NormalCommonLabelConsumer label;
        const NormalCommonButtonOwnConsumer button_own118;
        const NormalCommonCaptionConsumer caption178{};
        NormalCommonChildConsumers(NormalCommonLabelConsumer a,NormalCommonButtonOwnConsumer b)
            :label(std::move(a)),button_own118(std::move(b)){}
        NormalCommonChildConsumers(NormalCommonLabelConsumer a,NormalCommonButtonOwnConsumer b,
            NormalCommonCaptionConsumer caption)
            :label(std::move(a)),button_own118(std::move(b)),caption178(std::move(caption)){}
    };
    [[noreturn]] void execute_normal_common_button_caption178_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&,
        std::shared_ptr<const NormalCommonChildConsumers>)const;
    void stage_normal_root_common_children_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootBatchExecution&,std::shared_ptr<const NormalCommonChildConsumers>)const;
    static std::uint32_t require_normal_common_actual_child40_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&);
    static void require_normal_common_recursive_return_physical_without_callbacks(const NormalCommonRecursiveReturn&);
    static void require_normal_common_child18_return_physical_without_callbacks(const NormalCommonChild18Return&);
    void execute_normal_common56_required_manager_prefix_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&)const;
    static std::array<std::uint32_t,4> read_normal_common_manager_button_rect_without_callbacks(
        const detail::BnwNormalCommonChildCall&);
    static void require_normal_common_manager_receiver_current_without_callbacks(
        const detail::BnwNormalCommonManagerReceiverCall&);
    static void require_normal_common_manager_receiver_source_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonManagerReceiverCall&);
    std::shared_ptr<const detail::BnwNormalCommonManagerReceiverCall> capture_normal_common_manager_receiver_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&)const;
    std::shared_ptr<const detail::BnwNormalCommonManagerReceiverCall> capture_normal_common_manager_receiver_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalEmbeddedLabelCommon56Call&)const;
    void execute_normal_common56_required_manager_prefix_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonManagerReceiverCall&)const;
    void execute_normal_common56_required_manager_prefix_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalEmbeddedLabelCommon56Call&)const;
    std::shared_ptr<const NormalCommonRecursiveReturn> execute_normal_common_button_recursive78_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&,
        std::shared_ptr<const NormalCommonChildConsumers>)const;
    std::shared_ptr<const NormalCommonChild18Return> execute_normal_common_child18_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&,
        std::shared_ptr<const NormalCommonChildConsumers>)const;
    static void require_normal_common_traversal_capture_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&);
    void attach_normal_common_button_invocation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalCommonChildCall&,const detail::BnwNormalNonFontPaintInvocation&,
        const BnwWorldViewControlRenderPublication&,const detail::BnwNormalRootPaintCallFrame::Guard&)const;
    static bool normal_common_button_texture_is_nonnull_without_callbacks(const detail::BnwNormalCommonChildCall&);
    static std::shared_ptr<const NormalCommonNullChildReturn> capture_normal_common_null_child_return_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&);
    static void require_normal_common_null_child_return_physical_without_callbacks(const NormalCommonNullChildReturn&);
    static bool normal_common_child_call_matches_actual_control_without_callbacks(
        const detail::BnwNormalCommonChildCall&,const Control&);
    static void require_normal_common_child_call_current_without_callbacks(const detail::BnwNormalCommonChildCall&);
    static void require_normal_common_child_call_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalCommonChildCall&);
    detail::BnwNormalButtonCaptionCall capture_normal_common_button_caption_call_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCommonChildCall&)const;
    static void require_normal_button_caption_call_current_without_callbacks(const detail::BnwNormalButtonCaptionCall&);
    static void require_normal_button_caption_call_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalButtonCaptionCall&);
    std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> capture_normal_button_caption_fp32_rect_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalButtonCaptionCall&)const;
    static void require_normal_button_caption_fp32_rect_current_without_callbacks(
        const detail::BnwNormalCaptionFp32RectLoan&);
    static void require_normal_button_caption_fp32_rect_source_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalCaptionFp32RectLoan&);
    static void require_normal_button_caption_fp32_rect_same_caption_without_callbacks(
        const detail::BnwNormalCaptionFp32RectLoan&,const detail::BnwNormalButtonCaptionCall&);
    static void require_normal_button_caption_fp32_rect_same_initialized_run_without_callbacks(
        const detail::BnwNormalCaptionFp32RectLoan&,const std::shared_ptr<const detail::BnwPositionedLabelRunLoan>&);
    static void require_normal_button_caption_fp32_rect_same_entered_run_without_callbacks(
        const detail::BnwNormalCaptionFp32RectLoan&,const detail::BnwPositionedLabelRunLoan::Guard&);
    static std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
        normal_caption_fp32_rect_for_actual_paint_without_callbacks(
            const BnwWorldViewControlRenderPublication&,std::size_t,
            const detail::BnwLabelActualPaintCallFrame::Guard&);
    static void require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(
        const BnwWorldViewControlRenderPublication&,const detail::BnwLabelActualPaintCallFrame::Guard&);
    static void require_normal_caption_item_fp32_rect_physical_without_callbacks(
        const detail::BnwNormalItemOccurrence::Impl&);
    static void require_normal_button_caption_same_common_without_callbacks(
        const detail::BnwNormalButtonCaptionCall&,const detail::BnwNormalCommonChildCall&);
    static detail::BnwNormalButtonCaptionAppendProjection project_normal_button_caption_for_append_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalButtonCaptionCall&);
    static void require_normal_button_caption_emission_current_without_callbacks(
        const detail::BnwNormalButtonCaptionPaintEmission&);
    static void require_normal_button_caption_emission_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalButtonCaptionPaintEmission&);
    static void require_normal_button_caption_publication_inputs_without_callbacks(
        const detail::BnwNormalButtonCaptionPaintEmission&,const std::shared_ptr<BnwWorldViewControlTree>&,
        const Control&,const detail::BnwNormalRootContextEntry&);
    static std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission> bind_normal_button_caption_emission_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalButtonCaptionCall&,
        std::shared_ptr<const ComposedUiFrame>,const WorldViewPaint*,std::size_t,std::size_t);
    static void require_normal_button_caption_emission_publication_owned(
        const detail::BnwNormalButtonCaptionPaintEmission&,const BnwWorldViewControlRenderPublication&,
        const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<const detail::BnwPositionedLabelRunLoan> normal_button_caption_run_for_actual_item_owned(
        const detail::BnwNormalButtonCaptionPaintEmission&,const BnwWorldViewControlRenderPublication&,std::size_t,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwLabelActualPaintCallFrame&);


    using NormalQueueConsumer=std::function<detail::BnwNormalQueueRecordCompletion(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&,
        const detail::BnwNormalItemOccurrence::Guard&,const detail::BnwNormalQueueDrainPermission::Guard&,
        const detail::BnwNormalLabelMaterialLoan::ShaderAccess&,
        graphics::detail::BnwNormalLabelDrawRecordBinding::Guard,
        graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity,
        graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt)>;
    void drain_normal_root_batch_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootBatchExecution&,std::shared_ptr<const NormalQueueConsumer>)const;
    // Earned whole Root + actual completed manager drain, once-only owner
    // handoff. Retires CURRENT authority; SAME original Execution/Prefix/CPU
    // storage remains owned until the enclosing Ui scope releases it.
    std::array<std::uint64_t,4> retire_completed_normal_root_batch_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchExecution&)const;
    static void require_normal_root_queue_registration_metadata_without_callbacks(
        const detail::BnwNormalRootBatchHistory::Guard&);
    void open_normal_root_outer_window_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&)const;
    void close_normal_root_outer_window_and_register_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalRootBatchHistory::Guard&)const;
    static void require_normal_root_queue_registration_without_callbacks(
        const detail::BnwNormalRootBatchHistory::Guard&);
    void attach_normal_item_historical_font_planes_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalItemOccurrence::Guard&,
        const detail::BnwLabelPaintPositionedData&,std::shared_ptr<graphics::GlBnwSystemFontAtlas>)const;
    //Declaration only: no implementation/positive Material until earned whole
    //RootRead->slot10->root18, ordered78 and completed selector1 batch append.
    std::unique_ptr<detail::BnwNormalLabelMaterialLoan> normal_label_material_for_actual_item_owned(
        std::shared_ptr<const BnwWorldViewControlTree> same_tree,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,std::size_t actual_index,
        const detail::BnwLabelActualPaintCallFrame&,
        const std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>&,
        const detail::BnwLabelPaintTextLoan&,const detail::BnwLabelPaintPositionedData&,
        const detail::BnwWindowedRendererMatrixDataLoan&)const;
    std::unique_ptr<detail::BnwNormalFontPayloadPrefix> allocate_normal_font_payload_prefix_owned(
        const detail::BnwNormalGuiBatchPrefix&,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwLabelPaintPositionedData&)const;
    void write_normal_font_scalar_prefix_owned(const detail::BnwNormalFontPayloadPrefix&)const;
    std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl> allocate_normal_font_payload_control_owned(
        std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl>,
        detail::BnwLabelPaintPositionedData::Guard,detail::BnwLabelPaintTextLoan::Guard,
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    void write_normal_font_scalar_control_owned(std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl>)const;
    std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl> allocate_normal_font_payload_control_owned(
        std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl>,
        detail::BnwLabelPaintPositionedData::Guard,detail::BnwLabelPaintTextLoan::Guard,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalButtonCaptionCall&)const;
    void write_normal_font_caption_scalar_control_owned(std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl>)const;
    static void require_normal_caption_material_current_without_callbacks(const detail::BnwNormalLabelMaterialLoan::Impl&);
    struct NormalEmbeddedLabelCommon56Storage;
    detail::BnwNormalEmbeddedLabelCommon56Call capture_normal_embedded_label_common56_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<detail::BnwNormalLabelMaterialLoan::Impl>)const;
    static void require_normal_embedded_label_common56_current_without_callbacks(
        const detail::BnwNormalEmbeddedLabelCommon56Call&);
    static void require_normal_embedded_label_common56_source_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwNormalEmbeddedLabelCommon56Call&);
    std::unique_ptr<detail::BnwNormalLabelMaterialLoan> normal_label_material_for_actual_item_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwWorldViewControlRenderPublication&,std::size_t,const detail::BnwLabelActualPaintCallFrame&,
        const std::shared_ptr<const detail::BnwPositionedLabelRunLoan>&,
        const detail::BnwLabelPaintTextLoan&,const detail::BnwLabelPaintPositionedData&,
        const detail::BnwWindowedRendererMatrixDataLoan&,const detail::BnwNormalButtonCaptionCall&)const;
    void execute_normal_context_selected_label_entry_owned(
        std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl>,detail::BnwNormalLabelMaterialLoan::Impl*)const;
    static void enter_normal_selected_label591020_owned(detail::BnwNormalLabelMaterialLoan::Impl&,
        const detail::BnwNormalGuiBatchPrefix::Impl&);
    void continue_normal_context_common56_owned(
        std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl>,detail::BnwNormalLabelMaterialLoan::Impl*)const;
    void resume_normal_context_common56_after_label_owned(
        std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl>)const;
    static void require_normal_label_tail_return_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&);
    void complete_normal_label_tail_owned(detail::BnwNormalLabelMaterialLoan::Impl&)const;
    static void require_normal_root18_return_without_callbacks(const detail::BnwNormalGuiBatchPrefix::Impl&);
    static void require_normal_material_visible_child_without_callbacks(
        const detail::BnwNormalGuiBatchPrefix::Impl&,const detail::BnwNormalItemOccurrence::Guard&);
    std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan> standalone_positioned_label_run_for_paint_owned(
        std::shared_ptr<const BnwWorldViewControlTree>,Control,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwLabelActualPaintCallFrame&)const;
    static void initialize_standalone_label_slot10_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    void require_unitflag_button_slot10_return_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t,std::uint64_t)const;
    // Each other actual subtype must supply its own native+10 body/constructor
    // authority; Base4C/94 alone cannot issue a generic initialization return.
    static void initialize_grid_slot10_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    static void initialize_other_control_slot10_owned(ContextChildSlot10Frame&,std::uint64_t,std::uint64_t);
    // REQUIRED actual private load-owner definition. No callback/boolean
    // adapter is supplied in its place. Both ordinary subtype+10 and actual
    // Context457270/deferred branches must dispatch their real bodies.
    static LayoutChildSlot10Return initialize_context_child_slot10_owned(
        ContextInitializationFrame&,std::uint64_t actual_domain,std::uint64_t actual_id);
    static void require_normal_common_root_prefix_without_callbacks(
        const detail::BnwNormalGuiBatchPrefix::Impl&);
    void execute_normal_context_common56_prefix_owned(const detail::BnwNormalGuiBatchPrefix&)const;
    void initialize_context_5761_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<Core> require_context_direct_initialization_input_owned(
        const std::shared_ptr<Core>&,const std::shared_ptr<ContextDirectLoadFrame>&,
        const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>&);
    static void require_context_direct_initialization_origin_owned(
        const std::shared_ptr<Core>&,const std::shared_ptr<const LayoutContextInitializationOrigin>&);
    std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> initialize_context_direct_5761_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>&);
    void require_context_direct_initialization_history_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&)const;
    static void adopt_stack_child_results_owned(ContextInitializationFrame&,const LayoutChildSlot10Return&);
    static void finalize_stack_results_owned(ContextInitializationFrame&);
    static void publish_stack_results_owned(const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&);
    static std::shared_ptr<Core> require_context_initialization_return_owned(
        const std::shared_ptr<Core>&,const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&);
    class LayoutOperandLoan;
    // A closed completion witness of the DISTINCT native5761E0 initializer.
    // Defined by the private distinct5761 body only after its full scope DTOR.
    // Genuine construction and child+10/load owner joins remain required;
    // parser capture and generic4C receipts cannot construct this seal.
    class LayoutInitializationReturn final {
        friend class LayoutOperandLoan;
        friend class BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
        struct Member {std::uint64_t id,parser_serial,initialized_sequence;};
        const std::weak_ptr<Core> tree_;
        const std::uint64_t topology_;
        const std::vector<Member> members_;
        const std::array<float,2> native_zero_;
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> native_return_;
        LayoutInitializationReturn(const std::shared_ptr<Core>&,
            const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&);
        LayoutInitializationReturn(const LayoutInitializationReturn&)=delete;
        LayoutInitializationReturn&operator=(const LayoutInitializationReturn&)=delete;
        LayoutInitializationReturn(LayoutInitializationReturn&&)=delete;
        LayoutInitializationReturn&operator=(LayoutInitializationReturn&&)=delete;
     public:
        ~LayoutInitializationReturn()=default;
    };
    // Defined immutable consumer; genuine native initializer seal producer
    // remains separate and mandatory. CPU geometry never issues this Loan.
    class LayoutOperandLoan final {
        friend class detail::NotificationLayoutParentReceiver;
        friend class BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
        struct Operand {
            std::uint64_t id=0,parent=0,attached_order=0;
            std::uint32_t native_flags=0;
            std::array<float,2> offset{},size{},extent{};
            std::array<float,4> bounds{};
        };
        const std::thread::id creator_=std::this_thread::get_id();
        const std::weak_ptr<Core> tree_;
        const runtime::BnwWorldViewSourceOperationLoan*const operation_;
        const std::uint64_t control_id_,source_serial_,topology_revision_,effect_revision_;
        const Operand target_;
        const std::vector<Operand> children_;
        const std::array<float,2> native_zero_,native_margin_;
        const std::optional<std::array<float,2>> actual_base_anchor_;
        struct ScrollOperand {
            LayoutState::ScrollPanelFields native;
            // Same canonical actual native c8/cc/d0 bindings, in native order.
            // NULL means genuine producer observedNULL, never missing issuer.
            std::array<std::optional<Operand>,3> handles;
        };
        const std::optional<ScrollOperand> scroll_;
        struct Prefix {
            std::uint64_t id,parser_serial,source_serial,baseline_sequence,offset_store_sequence;
            std::array<float,2> offset;
        };
        struct Domain {
            std::weak_ptr<Core> core;
            std::uint64_t topology,effects;
            // Only selected completed context has a seal. Ancestor actual4C
            // prefixes are different source history, never full init grants.
            std::shared_ptr<const LayoutInitializationReturn> seal;
            std::vector<Prefix> parent_prefixes;
        };
        const std::vector<Domain> domains_;
        struct Capture;
        static Capture capture_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&);
        static Operand capture_operand_owned(const std::shared_ptr<Core>&,std::uint64_t);
        static void require_initialization_owned(const std::shared_ptr<Core>&,
            const std::shared_ptr<const LayoutInitializationReturn>&);
        static Prefix capture_parent_prefix_owned(const std::shared_ptr<Core>&,std::uint64_t);
        static void repeat_parent_prefix_owned(const std::shared_ptr<Core>&,const Prefix&);
        explicit LayoutOperandLoan(Capture);
        bool alive_=true;
        LayoutOperandLoan(const Control&,const runtime::BnwWorldViewSourceOperationLoan&);
        ~LayoutOperandLoan()noexcept;
        void require_for_core_without_callbacks(const std::shared_ptr<Core>&,std::uint64_t)const;
        LayoutOperandLoan(const LayoutOperandLoan&)=delete;
        LayoutOperandLoan&operator=(const LayoutOperandLoan&)=delete;
        LayoutOperandLoan(LayoutOperandLoan&&)=delete;
        LayoutOperandLoan&operator=(LayoutOperandLoan&&)=delete;
    };
    friend class ::civ5::runtime::BnwUnitFlagHealthInstanceLoan;
    void require_unitflag_health_control_without_callbacks(const Control&,std::uint64_t expected_id,
        std::uint64_t expected_instance_root,const runtime::BnwSelectedWorldViewContextOwner&)const;
    void require_unitflag_health_backend_origin_without_callbacks(const Control&,
        const runtime::BnwWorldViewSourceOperationLoan&,runtime::BnwUnitFlagHealthCommand)const;
    static void mutate_unitflag_health_from_storage_owned(const std::shared_ptr<Core>&,const Control&,
        const runtime::BnwWorldViewSourceOperationLoan&,runtime::BnwUnitFlagHealthCommand,std::array<double,4>);
    void set_unitflag_health_hidden_owned(const Control&,bool,const runtime::BnwWorldViewSourceOperationLoan&);
    void set_unitflag_health_fg_color_owned(const Control&,std::array<double,4>,const runtime::BnwWorldViewSourceOperationLoan&);
    void set_unitflag_health_percent_owned(const Control&,double,const runtime::BnwWorldViewSourceOperationLoan&);
    void set_unitflag_health_offset_x_owned(const Control&,double,const runtime::BnwWorldViewSourceOperationLoan&);
    enum class LayoutOperandCommand {calculate_size,reprocess_anchoring,set_size_y,calculate_internal_size};
    void dispatch_layout_operand_owned(const Control&,const runtime::BnwWorldViewSourceOperationLoan&,LayoutOperandCommand,double);
    void publish_notification_cpu_geometry_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::vector<detail::NotificationCpuGeometry>);
    enum class LayoutParentCommand {calculate_stack,reprocess_stack};
    void run_layout_parent_receiver_owned(const Control&,const LayoutOperandLoan&,LayoutParentCommand);
    void initialize_layout_parent_receiver_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void require_layout_backend_origin_without_callbacks(const Control&,const runtime::BnwWorldViewSourceOperationLoan&)const;
    void calculate_size_owned(const Control&,const LayoutOperandLoan&);
    enum class LayoutScrollCommand { size_y,internal_size };
    void run_layout_scroll_receiver_owned(const Control&,const LayoutOperandLoan&,LayoutScrollCommand,double);
    void set_size_y_owned(const Control&,double,const LayoutOperandLoan&);
    void reprocess_anchoring_owned(const Control&,const LayoutOperandLoan&);
    void calculate_internal_size_owned(const Control&,const LayoutOperandLoan&);
    // Pure observation of the actual captured root Context state, never
    // inherited visibility/native2000, XML rereading or a clock issuer.
    bool context_update_hidden_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void register_context_showhide_flag_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void branch_reset_animation_owned(const Control& actual_root);
    void advance_scroll_animation_owned(const Control& actual_target,const ScrollClockLoan&);
    void advance_selected_scroll_update_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwScrollAnimationUpdateLoan&);
    //Private callback-time physical borrower; no expired RootRead/ExportLoan replay.
    static std::uint64_t native_ordinary_geometry_receiver_id_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    void require_native_ordinary_geometry_receiver_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&)const;
    enum class NativeOrdinaryFlagWrite {anchor_51e6a0,hide_v50_4ccec0,no_clip_72c700,consume_mouse_over_4d7210,consume_mouse_button_437ec0,consume_mouse_wheel_6798e0,disabled_member_5fc7c0,disabled_hover_clear_5fc7e0,modal_7a5fc0,tooltip_callback_6dbf40,tooltip_enable_692040,tooltip_type_47cb60_4e3da9,tooltip_text_5f3b90,tooltip_clear_73fe50_69ade0};
    enum class NativeOrdinaryUintRead {hidden_40,children_78_08,mouse_over_42_bit0_669850,consume_mouse_over_40_bit11_75d480,consume_mouse_button_40_bit10_6d8390,consume_mouse_wheel_40_bit12_488050,disabled_40_bit17_6d2930,modal_40_bit19_70c030,tooltip_enabled_40_bit14_453350};
    static std::uint32_t native_ordinary_flag_word_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    static void commit_native_ordinary_flag_word_owned(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryFlagWrite,std::uint32_t);
    void set_native_ordinary_boolean_flag_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryFlagWrite,bool);
    std::uint32_t read_native_ordinary_uint_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryUintRead)const;
    void set_native_ordinary_anchor_51e6a0_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const char*,const std::function<void()>&);
    void reprocess_native_ordinary_anchoring_4a7f10_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    void set_native_ordinary_color58_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::array<float,4>&);
    void set_native_ordinary_alpha9c_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,float);
    double read_native_ordinary_alpha_a0_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&)const;
    static std::uint64_t native_ordinary_common_receiver_id_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&);
    void branch_reset_native_ordinary_animation54_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    struct NativeOrdinaryIDRead {std::array<char,32> bytes;std::uint32_t length;};
    NativeOrdinaryIDRead read_native_ordinary_id58_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&)const;
    void set_native_ordinary_disabled88_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,bool);
    void calculate_native_ordinary_visibility94_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    enum class NativeOrdinaryTooltipWrite {callback_6dbf40,type_47cb60_4e3d80,text_5f3b90,clear_73fe50_69ade0,localization_lookup_69ae5a};
    static std::uint64_t native_ordinary_tooltip_fields_id_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&);
    void refresh_native_ordinary_tooltip524cc0_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::function<void()>&);
    void write_native_ordinary_tooltip_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryTooltipWrite,const char*,const std::function<void()>&);
    void read_native_ordinary_tooltip54_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,std::optional<std::string>&)const;
    //Current78 may be NULL only after the SAME constructor-owned list storage
    //has actually been released. This never renews a historical RootRead loan.
    static void require_native_ordinary_current_children_without_callbacks(
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn>&,
        const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78>&);
    enum class NativeOrdinaryChildrenMember {change_parent_5335e0,release_child_6ab0d0,destroy_all_children_781ff0};
    static std::shared_ptr<const void> native_ordinary_concrete_without_callbacks(
        const std::shared_ptr<Core>&,const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&);
    void execute_native_ordinary_children_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryChildrenMember,const void*);
    enum class NativeOrdinarySizeAxis {x_4e99c0,y_68a240};
    void set_native_ordinary_size_axis_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinarySizeAxis,float);
    void set_native_ordinary_size98_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::array<float,2>&);
    enum class NativeOrdinarySizeRead {virtual_a8_691d40,virtual_ac_428990,
        direct_30_467420,direct_34_467420};
    static float native_ordinary_current_size_cell_without_callbacks(
        const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,std::size_t);
    std::array<float,2> read_native_ordinary_size_a4_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&)const;
    float read_native_ordinary_size_axis_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinarySizeRead)const;
    enum class NativeOrdinaryOffsetRead { virtual_b0_5b5b50, virtual_b4_749a00,
        direct_70_6486f0, direct_74_6486f0 };
    float read_native_ordinary_offset_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryOffsetRead)const;
    enum class NativeInitialLuaField { context_update_90, context_input_51c, context_showhide_524, button_event_84 };
    std::shared_ptr<const void> read_native_initial_lua_function_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeInitialLuaField,std::uint32_t,bool expected_busy=false)const;
    void replace_native_initial_lua_function_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeInitialLuaField,std::uint32_t,
        const std::shared_ptr<const void>& expected,const std::shared_ptr<const void>& value);
    void set_native_context_function_flag_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>&,NativeInitialLuaField);
    void set_native_button_void1_4a9bc0_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,std::uint32_t);
    enum class NativeOrdinaryOffsetAxis { x_4a8380, y_6158f0 };
    enum class NativeOrdinaryOffsetPairMember { vector_4c8fe0, pair_69cd90 };
    void set_native_ordinary_offset_axis_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,NativeOrdinaryOffsetAxis,float);
    void set_native_ordinary_offset_69cd90_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::array<float,2>&);
    void execute_native_ordinary_offset_pair_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::array<float,2>&,NativeOrdinaryOffsetPairMember);
    void set_native_ordinary_offset_4c8fe0_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,const std::array<float,2>&);
    const std::shared_ptr<Core> core_;
    explicit BnwWorldViewControlTree(std::shared_ptr<Core>);
    void change_parent_selected_container_owned(const Control&,const std::shared_ptr<BnwWorldViewControlTree>&,const Control&);
    std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> metadata_source_owner()const;
    std::vector<Control> render_controls_owned()const;
    std::uint64_t render_revision_owned()const;
    content::XmlNode render_context_owned()const;
    void require_render_revision_without_callbacks(std::uint64_t,
        const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t)const;
    struct ContextRootReadFileFrame;
    struct ContextRootWatcherCustody;
    struct ContextRootXml84;
    struct ContextRootDefaultFieldsEvidence;
    struct ContextXmlWrapperFrame;
    struct ContextRootChildAttempt;
    struct ContextChildControlsFrame;
    struct RootOrdinaryControlsFrame;
    struct RecursiveOrdinaryOrigin;
    static void require_recursive_root_ordinary_origin_without_callbacks(const RecursiveOrdinaryOrigin&);
    static void require_root_ordinary_button_cdata_owner_without_callbacks(
        const std::shared_ptr<Core>&,std::uint64_t,
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&,
        const std::shared_ptr<const void>&,std::uint64_t);
    std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn> consume_root_ordinary_78_child_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<RootOrdinaryControlsFrame>&,std::size_t);
    std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn> run_root_ordinary_controls_frame_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<RootOrdinaryControlsFrame>&);
    struct RootOrdinaryName588Registration;
    friend class detail::NotificationContextRootOrdinaryControlsExportLoan;
    static void require_root_ordinary_controls_export_operand_without_callbacks(
        const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>&,
        const runtime::BnwWorldViewSourceOperationLoan&);
    static std::u16string_view root_ordinary_controls_screen_b4_without_callbacks(
        const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>&);
    std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportLoan> make_root_ordinary_controls_export_operand_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<RootOrdinaryControlsFrame>&);
    void register_root_ordinary_name588_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<RootOrdinaryControlsFrame>&,
        const std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportLoan>&,
        const std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportReturn>&);
    struct ContextChildName588Registration;
    struct ContextDistinctChildAttachProgress;
    static void require_context_distinct_attach_progress_without_callbacks(const ContextDistinctChildAttachProgress&);
    static void require_context_distinct_attachment_physical_without_callbacks(const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>&);
    void attach_distinct_context_child_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>&,const std::shared_ptr<ContextRootChildAttempt>&);
    friend class detail::NotificationLayoutNativeChildControlsReturn;
    static void require_context_child_v64_physical_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn::Impl&);
    static void require_root_ordinary_child_v64_current_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn::Impl&);
    static void require_root_ordinary_child_v64_export_current_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn::Impl&);
    static void require_root_ordinary_v64_export_origin_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationContextRootOrdinaryControlsExportReturn&);
    static void require_root_ordinary_v64_export_operands_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationContextRootOrdinaryControlsExportReturn&,
        const detail::NotificationContextLuaControlsNamedReference&,const void*,const BnwWorldViewControlTree&,
        const detail::NotificationLayoutOrdinaryReceiver&,std::string_view,std::string_view);
    static void require_root_ordinary_controls_export_operand_physical_without_callbacks(
        const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>&,
        const runtime::BnwWorldViewSourceOperationLoan&,bool);
    static std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> root_ordinary_child_v64_receiver_without_callbacks(
        const detail::NotificationLayoutNativeChildControlsReturn::Impl&);
    std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn> consume_root_ordinary_child_controls_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>&,
        const std::shared_ptr<const detail::NotificationContextRootOrdinaryFactoryReturn>&);
    static void require_context_child_v64_current_without_callbacks(const std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn::Impl>&);
    static void require_context_child_v64_same_read_without_callbacks(const std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn::Impl>&,
        const detail::NotificationContextRootReadChildrenLoan&);
    static void require_context_child_v64_export_origin_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationContextChildControlsExportReturn&);
    static void require_context_child_v64_export_operands_without_callbacks(const detail::NotificationLayoutNativeChildControlsReturn&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationContextChildControlsExportReturn&,
        const detail::NotificationContextLuaControlsNamedReference&,const detail::NotificationLayoutContextOrdinaryState&,
        const BnwWorldViewControlTree&,const runtime::ScriptUiChildSource&,const detail::NotificationLayoutOrdinaryReceiver&,std::string_view);
    void stage_context_child_v64_return_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<ContextRootChildAttempt>&,const std::shared_ptr<ContextChildControlsFrame>&);
    void publish_context_child_v64_after_scope_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<ContextRootChildAttempt>&);
    friend class detail::NotificationContextRootChildFactoryReturn;
    friend class detail::NotificationContextChildControlsExportLoan;
    std::shared_ptr<const detail::NotificationContextRootChildFactoryReturn> publish_context_root_child_factory_return_owned(const runtime::BnwWorldViewSourceOperationLoan&, const std::shared_ptr<ContextRootChildAttempt>&);
    static void require_context_root_child_factory_physical_without_callbacks(const detail::NotificationContextRootChildFactoryReturn::Impl&);
    void register_root_context_child_name588_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<ContextRootChildAttempt>&,const std::shared_ptr<ContextChildControlsFrame>&);
    void prepare_root_context_child_controls_export_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&, const std::shared_ptr<ContextRootChildAttempt>&);
    void construct_root_context_child_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&, const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>&);
    std::shared_ptr<const detail::NotificationContextRootOrdinaryFactoryReturn> construct_root_ordinary_child_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>&);
    static void require_root_ordinary_factory_current_without_callbacks(
        const detail::NotificationContextRootOrdinaryFactoryReturn::Impl&);
    static void require_ordinary_root_read_origin_without_callbacks(const std::shared_ptr<Core>&);
    void complete_root_context_child_common_wrapper_owned(const runtime::BnwWorldViewSourceOperationLoan&, const std::shared_ptr<ContextXmlWrapperFrame>&);
    struct ContextRootStyleCursorValue;
    struct ContextRootChildrenEvidence;
    struct ContextRootNodeReturn;
    struct ContextRootInstanceMutation;
    static void require_context_root_node_returns_without_callbacks(const ContextRootChildrenEvidence&);
    static void require_context_root_read_staged_before_frame_exit_owned(const std::shared_ptr<ContextRootReadFileFrame>&);
    std::shared_ptr<const ContextRootNodeReturn> register_context_root_instance_a0_owned(const std::shared_ptr<ContextRootChildrenEvidence>&);
    void cleanup_and_stage_context_root_read_owned(const std::shared_ptr<ContextRootChildrenEvidence>&);
    static void require_context_root_read_terminal_without_callbacks(const detail::NotificationContextRootReadCompletionSlot::Impl&);
    void publish_context_root_read_after_scope_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    friend class detail::NotificationContextRootReadCompletionSlot;
    static void require_context_root_read_pending_slot_without_callbacks(const detail::NotificationContextRootReadCompletionSlot::Impl&);
    static void require_context_root_read_completed_slot_without_callbacks(const detail::NotificationContextRootReadCompletionSlot::Impl&);
    static std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> capture_pending_context_root_read_completion_owned(
        const detail::NotificationContextRootReadChildrenLoan::Guard&);
    friend class detail::NotificationContextRootReadChildrenLoan;
    void read_context_root_declarations_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::shared_ptr<ContextRootReadFileFrame>);
    void parse_retained_context_xml_wrapper_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&, const detail::NotificationContextRootReadChildrenLoan* = nullptr);
    void load_selected_context_native_root_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void execute_context_direct_initial_lua_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&);
    void finalize_context_direct_load_return_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<const detail::NotificationContextDirectLoadReturn>
        require_context_direct_load_return_without_callbacks(const std::shared_ptr<Core>&,
            const runtime::BnwWorldViewSourceOperationLoan&);
    std::shared_ptr<const detail::NotificationContextDirectLoadReturn>
        require_context_direct_load_return_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    static bool require_normal_root_or_idle_origin_without_callbacks(const std::shared_ptr<Core>&);
    friend class detail::NotificationContextRootDefaultFieldsReturn;
    void read_context_root_file_prefix_until_required_owner_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const detail::NotificationContextLoadNameLoan>,
        std::shared_ptr<const detail::NotificationContextLuaStatePreparation>,
        std::shared_ptr<const detail::NotificationContextLuaLoadSetupReturn>,
        std::shared_ptr<const detail::NotificationContextRootSmallUiPolicyLoan>);
    void resolve_and_watch_context_root_file_selected_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::shared_ptr<ContextRootReadFileFrame>,
        const detail::NotificationContextRootSmallUiPolicyLoan&);
    std::shared_ptr<const detail::NotificationContextRootDefaultFieldsReturn> read_context_root_default_fields_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::shared_ptr<ContextRootReadFileFrame>);
    void read_context_root_xml84_name_prefix_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::shared_ptr<ContextRootReadFileFrame>,
        std::shared_ptr<const assets::CivisResourceBankOwner>,
        std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan>,assets::AssetKey);
    void publish_context_ordinary_receiver_projection_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void construct_context_native_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void construct_context_native_suffix_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    std::shared_ptr<const detail::NotificationContextLoadNameLoan> store_context_native_load_name_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationContextNativeLoadNameInputLoan&);
    std::unique_ptr<const detail::NotificationContextRootReadB4Guard> capture_context_root_read_b4_guard_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const detail::NotificationContextLuaStatePreparation>&);
    // Actual attachment body requires genuine RootRead/factory/v64 returns.
    struct ContextRootAttachEvidence;
    static void require_context_root_attach_returned_factory_identity_without_callbacks(
        const detail::NotificationContextRootReadChildrenLoan::Guard&,
        const detail::NotificationLayoutNativeChildControlsReturn::Guard&);
    std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> attach_context_returned_child_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::NotificationContextRootReadChildrenLoan&,const detail::NotificationLayoutNativeChildControlsReturn&);
    void store_standalone_root_parent_38_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::NotificationContextRootReadChildrenLoan&,const detail::NotificationLayoutNativeChildControlsReturn&);
    void store_context_root_attach_3c_for_actual_receiver_owned(
        const detail::NotificationContextRootChildAttachStoreLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationLayoutOrdinaryReceiver&);
    std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> publish_context_root_child_attach_return_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<ContextRootAttachEvidence>&);
    static void require_context_root_attachment_physical_without_callbacks(
        const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>&);
    static void require_context_root_ordinary_attachment_physical_without_callbacks(
        const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>&);
    static std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> require_actual_context_root_attachment_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::NotificationLayoutOrdinaryReceiver&);
    static std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn>
        capture_context_constructor_owned(const std::shared_ptr<Core>&,const runtime::BnwWorldViewSourceOperationLoan&);
    static std::shared_ptr<BnwWorldViewControlTree> capture_owned(
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner>,std::string_view,
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<detail::NotificationContextGuiPort>&);
};
// Original renderer-owned lender: its producer encloses its whole retained
// lifetime through queue consumption in the actual GUI/window loans.
// It retains no extra strong GUI/window pin and is never cloned/reissued.
// Only the actual existing renderer can bind control identity to its actual
// composed paint span. No public constructor accepts caller rectangles/frame DTOs.
namespace detail {
// A pending real native child18, not a painted Frame, return or queue permit.
// Only Tree can issue it. Only the actual renderer callback may read its lender.
class BnwNormalCommonChildCall final {
    friend class BnwNormalNonFontCurrentReceiverGuard;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalCommonChildCall(std::shared_ptr<const Impl>,const IssuerKey&);
    enum class Kind {standalone_label,button};
    enum class Phase {child18,button_own118,common56f630,button_caption178};
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    BnwWorldViewControlTree::Control actual_control_without_callbacks()const;
    BnwNormalRootContextEntry captured_root_context_entry_without_callbacks()const;
    BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
    std::size_t actual_ordered_ordinal_without_callbacks()const;
    std::size_t actual_common_depth_without_callbacks()const;
    std::array<std::uint32_t,2> actual_child_offset_words_without_callbacks()const;
    std::uint32_t actual_child_alpha_word_without_callbacks()const;
    bool matches_actual_control_without_callbacks(const BnwWorldViewControlTree::Control&)const;
    bool actual_button_texture_is_nonnull_without_callbacks()const;
    Kind actual_kind_without_callbacks()const;
    Phase actual_phase_without_callbacks()const;
public:
    BnwNormalCommonChildCall(const BnwNormalCommonChildCall&)=default;
    BnwNormalCommonChildCall&operator=(const BnwNormalCommonChildCall&)=delete;
    ~BnwNormalCommonChildCall()=default;
};

// A pending actual Button-owned Label178 call. It is distinct from an
// ordinary standalone Label Control/child18 and from a completed CPU return.
class BnwNormalButtonCaptionCall final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class BnwNormalButtonCaptionPaintEmission;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class BnwNormalCommonChildCall;
    friend class BnwNormalLabelMaterialLoan;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalButtonCaptionCall(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    BnwWorldViewControlTree::Control actual_control_without_callbacks()const;
    BnwNormalRootContextEntry captured_root_context_entry_without_callbacks()const;
    BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
    std::size_t actual_ordered_ordinal_without_callbacks()const;
    std::shared_ptr<const BnwPositionedLabelRunLoan>
        positioned_run_for_actual_caption_without_callbacks()const;
    void require_same_common_call_without_callbacks(const BnwNormalCommonChildCall&)const;
    BnwNormalButtonCaptionAppendProjection projection_for_append_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    class EmissionIssuerKey final {
        friend class BnwNormalButtonCaptionCall;
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<const ComposedUiFrame> frame_;
        const WorldViewPaint*const item_;
        const std::size_t index_,count_;
        EmissionIssuerKey(std::shared_ptr<const ComposedUiFrame>,std::size_t);
        EmissionIssuerKey(const EmissionIssuerKey&)=delete;
        EmissionIssuerKey&operator=(const EmissionIssuerKey&)=delete;
    };
    std::shared_ptr<const BnwNormalButtonCaptionPaintEmission> bind_emitted_index_owned(
        const EmissionIssuerKey&,const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
    BnwNormalButtonCaptionCall(const BnwNormalButtonCaptionCall&)=default;
    BnwNormalButtonCaptionCall&operator=(const BnwNormalButtonCaptionCall&)=delete;
    ~BnwNormalButtonCaptionCall()=default;
};

// Genuine Label540B54 receiver entry, distinct from outer Button Common56.
// Its only issuer follows actual font-pass RETURN and physical spriteNULL.
class BnwNormalEmbeddedLabelCommon56Call final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class BnwNormalCommonManagerReceiverCall;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalEmbeddedLabelCommon56Call(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<NotificationLayoutEmbeddedLabelState> original_label_without_callbacks()const;
    BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
    std::array<std::uint32_t,4> actual_receiver_rect_words_without_callbacks()const;
    std::array<std::uint32_t,3> actual_incoming_caption_words_without_callbacks()const;
    std::array<std::uint32_t,2> actual_root_arg4_arg5_words_without_callbacks()const;
public:
    BnwNormalEmbeddedLabelCommon56Call(const BnwNormalEmbeddedLabelCommon56Call&)=default;
    BnwNormalEmbeddedLabelCommon56Call&operator=(const BnwNormalEmbeddedLabelCommon56Call&)=delete;
    ~BnwNormalEmbeddedLabelCommon56Call()=default;
};

// Genuine typed receiver/caller before the shared required Common56 manager
// prefix. This capsule issues no manager state, return or effect permission.
class BnwNormalCommonManagerReceiverCall final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    using Origin=std::variant<BnwNormalCommonChildCall,BnwNormalEmbeddedLabelCommon56Call>;
    using NativeReceiver=std::variant<std::shared_ptr<NotificationLayoutButtonState>,
        std::shared_ptr<NotificationLayoutEmbeddedLabelState>>;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalCommonManagerReceiverCall(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
    BnwNormalCommonManagerReceiverCall(const BnwNormalCommonManagerReceiverCall&)=default;
    BnwNormalCommonManagerReceiverCall&operator=(const BnwNormalCommonManagerReceiverCall&)=delete;
    ~BnwNormalCommonManagerReceiverCall()=default;
};

// Readonly projection of actual initialized178/C4/CC/ColorSet words. Data
// alone issues no Frame/queue/current capability; only Tree can construct it.
class BnwNormalButtonCaptionAppendProjection final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    BnwNormalButtonCaptionAppendProjection(std::string t,std::string f,std::string s,
        std::shared_ptr<const BnwCompletedFontLoan> owner,std::uint32_t ordinal,
        std::array<std::uint32_t,3> selectors,std::array<std::int32_t,3> metrics,
        std::array<std::uint32_t,10> geometry,std::uint32_t alpha,
        std::array<std::uint32_t,3> packed,std::array<std::uint32_t,12> raw,
        std::array<std::uint32_t,3> arguments,std::array<std::uint32_t,2> incoming,
        std::uint32_t incoming_alpha,std::array<std::uint32_t,2> button,std::uint32_t button_alpha,
        std::array<std::uint32_t,2> controls)
        :text(std::move(t)),font_name(std::move(f)),style_name(std::move(s)),font_owner(std::move(owner)),
         style_ordinal(ordinal),style_pass_selectors(selectors),font_metrics(metrics),label_geometry_10_34(geometry),
         label_alpha_44(alpha),packed_colors_148_14c_150(packed),raw_colors_84_b0(raw),caption_arguments_xy_alpha(arguments),
         incoming_position(incoming),incoming_alpha_word(incoming_alpha),button_position_28_2c(button),
         button_alpha_word_44(button_alpha),numeric_controls(controls){}
public:
    const std::string text,font_name,style_name;
    const std::shared_ptr<const BnwCompletedFontLoan> font_owner;
    const std::uint32_t style_ordinal;
    const std::array<std::uint32_t,3> style_pass_selectors;
    const std::array<std::int32_t,3> font_metrics;
    const std::array<std::uint32_t,10> label_geometry_10_34;
    const std::uint32_t label_alpha_44;
    const std::array<std::uint32_t,3> packed_colors_148_14c_150;
    const std::array<std::uint32_t,12> raw_colors_84_b0;
    const std::array<std::uint32_t,3> caption_arguments_xy_alpha;
    const std::array<std::uint32_t,2> incoming_position;
    const std::uint32_t incoming_alpha_word;
    const std::array<std::uint32_t,2> button_position_28_2c;
    const std::uint32_t button_alpha_word_44;
    const std::array<std::uint32_t,2> numeric_controls;
    BnwNormalButtonCaptionAppendProjection(const BnwNormalButtonCaptionAppendProjection&)=default;
    BnwNormalButtonCaptionAppendProjection&operator=(const BnwNormalButtonCaptionAppendProjection&)=delete;
    ~BnwNormalButtonCaptionAppendProjection()=default;
};

// SAME Ui-only actual append carrier/index. Not a standalone Run or a Label
// body-return/Root-return/queue-completion issuer.
class BnwNormalButtonCaptionPaintEmission final {
    friend class BnwNormalButtonCaptionCall;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit BnwNormalButtonCaptionPaintEmission(std::shared_ptr<const Impl>);
    const ComposedUiFrame& frame_origin_for_publication_owned()const;
    void require_publication_owned(const ::civ5::ui::BnwWorldViewControlRenderPublication&,
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwPositionedLabelRunLoan> run_for_actual_item_owned(
        const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t,
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelActualPaintCallFrame&)const;
public:
    BnwNormalButtonCaptionPaintEmission(const BnwNormalButtonCaptionPaintEmission&)=delete;
    BnwNormalButtonCaptionPaintEmission&operator=(const BnwNormalButtonCaptionPaintEmission&)=delete;
    ~BnwNormalButtonCaptionPaintEmission()=default;
};
}

// The producer hookup for WorldAnchor projection remains a separate missing gate.
class BnwWorldViewControlRenderPublication final {
    friend class detail::BnwNormalLabelPaintEmission;
    friend class detail::BnwNormalButtonCaptionPaintEmission;
    friend class detail::BnwLabelPaintTextLoan;
    friend class detail::BnwNormalRootBatchHistory;
    friend class detail::BnwNormalItemOccurrence;
    friend class detail::BnwLabelActualPaintCallFrame;
    friend class detail::BnwNormalRootPaintCallFrame;
    friend class BnwWorldViewControlTree;
 public:
    BnwWorldViewControlRenderPublication(const BnwWorldViewControlRenderPublication&)=delete;
    BnwWorldViewControlRenderPublication& operator=(const BnwWorldViewControlRenderPublication&)=delete;
 private:
    friend class UiScreenGlRenderer;
    const std::thread::id creator_=std::this_thread::get_id();
    const UiScreenGlRenderer* renderer_;
    std::weak_ptr<const void> renderer_life_;
    const std::shared_ptr<BnwWorldViewControlTree> tree_;
    const std::optional<BnwWorldViewControlTree::Control> control_;
    const std::optional<detail::BnwNormalRootContextEntry> root_context_entry_;
    // An ordinary child retains its enclosing lexical Context, while its
    // actual receiver remains the genuine Control and actual paint span.
    const std::optional<detail::BnwNormalRootContextEntry> enclosing_context_entry_;
    const std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState> context_child_body_;
    const BnwOwnedCameraGuiScreen::Loan& gui_;
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window_;
    const ComposedUiFrame frame_;
    //SAME actual adjusted backing is created before Frame issuance.
    mutable std::shared_ptr<const ComposedUiFrame> adjusted_frame_owner_;
    mutable const runtime::BnwWorldViewSourceOperationLoan* adjusted_frame_operation_=nullptr;
    mutable std::optional<std::uint64_t> adjusted_frame_source_serial_;
    mutable bool adjusted_frame_painting_=false;
    //Closed original allocation control is set only by its factory.
    std::weak_ptr<const BnwWorldViewControlRenderPublication> original_owner_;
    const runtime::BnwWorldViewSourceOperationLoan* original_owner_operation_=nullptr;
    std::optional<std::uint64_t> original_owner_source_serial_;
    const std::vector<std::size_t> paint_indices_;
    const std::shared_ptr<const detail::BnwNormalLabelPaintEmission> normal_label_emission_;
    const std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission> normal_caption_emission_{};
    const std::uint64_t topology_revision_;
    const std::uint64_t effect_revision_;
    class OriginalOwnerIssuerKey final {
        friend class UiScreenGlRenderer;
        OriginalOwnerIssuerKey()=default;
        OriginalOwnerIssuerKey(const OriginalOwnerIssuerKey&)=delete;
    };
    static std::shared_ptr<const BnwWorldViewControlRenderPublication> capture_original_owned(
        const OriginalOwnerIssuerKey&,const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalLabelPaintEmission> = {});
    static std::shared_ptr<const BnwWorldViewControlRenderPublication> capture_context_original_owned(
        const OriginalOwnerIssuerKey&,const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,detail::BnwNormalRootContextEntry,
        const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame);
    BnwWorldViewControlRenderPublication(const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,detail::BnwNormalRootContextEntry,
        const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame);
    static std::shared_ptr<const BnwWorldViewControlRenderPublication> capture_context_child_original_owned(
        const OriginalOwnerIssuerKey&,const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        const detail::BnwNormalRootContextEntry&,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalLabelPaintEmission> = {});
    BnwWorldViewControlRenderPublication(const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        detail::BnwNormalRootContextEntry,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalLabelPaintEmission>);
    class ContextChildBodyIssuerKey final {
        friend class BnwWorldViewControlRenderPublication;
        ContextChildBodyIssuerKey()=default;
        ContextChildBodyIssuerKey(const ContextChildBodyIssuerKey&)=delete;
    };
    static std::shared_ptr<const BnwWorldViewControlRenderPublication> capture_context_child_body_original_owned(
        const OriginalOwnerIssuerKey&,const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,const detail::BnwNormalCommonChildCall&,
        BnwWorldViewControlTree::Control,const detail::BnwNormalRootContextEntry&,
        const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        std::shared_ptr<const ComposedUiFrame>);
    BnwWorldViewControlRenderPublication(const ContextChildBodyIssuerKey&,const UiScreenGlRenderer&,
        std::weak_ptr<const void>,std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        detail::BnwNormalRootContextEntry,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState>);
    static ComposedUiFrame context_child_body_frame_before_constructor_owned(
        const std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState>&);
    UiGlPaintReport paint_context_child_body_owned(UiScreenGlRenderer&)const;
    static std::shared_ptr<const BnwWorldViewControlRenderPublication> capture_context_child_original_owned(
        const OriginalOwnerIssuerKey&,const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        const detail::BnwNormalRootContextEntry&,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>);
    BnwWorldViewControlRenderPublication(const UiScreenGlRenderer&,std::weak_ptr<const void>,
        std::shared_ptr<BnwWorldViewControlTree>,BnwWorldViewControlTree::Control,
        detail::BnwNormalRootContextEntry,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>);
    static ComposedUiFrame context_caption_frame_before_constructor_owned(
        const std::shared_ptr<BnwWorldViewControlTree>&,const BnwWorldViewControlTree::Control&,
        const std::optional<detail::BnwNormalRootContextEntry>&,ComposedUiFrame,
        const std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>&);
    UiGlPaintReport paint_context_caption_owned(UiScreenGlRenderer&)const;
    std::shared_ptr<const detail::BnwPositionedLabelRunLoan> caption_run_for_actual_item_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwLabelActualPaintCallFrame&)const;
    std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> caption_fp32_rect_for_actual_item_without_callbacks(
        std::size_t,const detail::BnwLabelActualPaintCallFrame::Guard&)const;
    void require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(
        const detail::BnwLabelActualPaintCallFrame::Guard&)const;
    static ComposedUiFrame context_child_frame_before_constructor_owned(
        const std::shared_ptr<BnwWorldViewControlTree>&,
        const std::optional<detail::BnwNormalRootContextEntry>&,ComposedUiFrame,
        const std::shared_ptr<const detail::BnwNormalLabelPaintEmission>&);
    static std::uint64_t context_receiver_revision_without_callbacks(
        const std::shared_ptr<BnwWorldViewControlTree>&,
        const std::optional<detail::BnwNormalRootContextEntry>&,bool);
    void require_actual_receiver_for_paint_without_callbacks()const;
    std::optional<detail::BnwNormalRootContextEntry> actual_root_context_entry_for_paint_without_callbacks()const;
    std::optional<detail::BnwNormalRootContextEntry> actual_enclosing_root_context_entry_without_callbacks()const;
    UiGlPaintReport paint_context_root_owned(UiScreenGlRenderer&)const;
    std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner_before_paint_without_callbacks()const;
    std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner_without_callbacks()const;
    BnwWorldViewControlRenderPublication(const UiScreenGlRenderer&,
        std::weak_ptr<const void>,std::shared_ptr<BnwWorldViewControlTree>,
        BnwWorldViewControlTree::Control,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        ComposedUiFrame,std::vector<std::size_t>,
        std::shared_ptr<const detail::BnwNormalLabelPaintEmission> = {});
    std::uint64_t actual_checked_control_id_for_paint_without_callbacks()const;
    std::shared_ptr<const ComposedUiFrame> actual_adjusted_frame_owner_for_paint_without_callbacks(
        const UiScreenGlRenderer&,const ComposedUiFrame&)const;
    void require_adjusted_frame_owner_without_callbacks(const std::shared_ptr<const ComposedUiFrame>&)const;
    std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan> normal_label_run_for_actual_item_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwLabelActualPaintCallFrame&)const;
    void require_normal_label_run_for_actual_item_without_callbacks(std::size_t,
        const runtime::BnwWorldViewSourceOperationLoan&,const detail::BnwLabelActualPaintCallFrame::Guard&,
        const std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>&)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwLabelActualPaintCallFrame&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootPaintCallFrame&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_from_frame_guard_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwLabelActualPaintCallFrame::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_from_frame_guard_owned(
        std::size_t,const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootPaintCallFrame::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootPaintCallFrame&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_from_frame_guard_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwNormalRootPaintCallFrame::Guard&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    template<class ActualFrameGuard>
    std::unique_ptr<detail::BnwNormalGuiBatchPrefix> begin_normal_gui_batch_prefix_from_frame_guard_impl_owned(
        std::optional<std::size_t>,const runtime::BnwWorldViewSourceOperationLoan&,
        const ActualFrameGuard&,std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>)const;
    UiGlPaintReport paint(UiScreenGlRenderer&)const;
};
}
