#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include "civ5/assets/detail/civis_loaded_bytes_physical_path_loan.hpp"
#include "civ5/ui/detail/bnw_original_generated_resource_image_loan.hpp"
#include "civ5/ui/detail/bnw_original_resource_sampler_descriptor_loan.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_setup_loan.hpp"
#include <cstring>
#include <atomic>
#include <utility>
#include "civ5/ui/detail/bnw_original_nonfont_device_initializer.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include <thread>
#include <vector>
#include <stdexcept>
#include "civ5/ui/notification_layout_parent_receiver.hpp"
#include "civ5/ui/detail/notification_layout_texture_keeper.hpp"
#include <exception>
#include <mutex>
#include "civ5/ui/detail/notification_layout_baseline.hpp"
#include "civ5/ui/detail/notification_layout_scroll_receiver.hpp"
#include "civ5/ui/detail/notification_layout_image_state.hpp"
#include "civ5/ui/detail/notification_layout_context_realm.hpp"
#include "civ5/ui/detail/notification_layout_embedded_label_state.hpp"
#include "civ5/ui/detail/notification_layout_standalone_label_state.hpp"
#include "civ5/ui/detail/bnw_label_current_text_loan.hpp"
#include "civ5/ui/detail/bnw_positioned_label_run_loan.hpp"
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include "civ5/ui/detail/bnw_label_paint_prepared_flush_loan.hpp"
#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"
#include "civ5/graphics/detail/bnw_owned_font_paint_effect_fence.hpp"
#include "civ5/ui/detail/bnw_label_system_font_prepared_run.hpp"
#include "civ5/ui/detail/notification_layout_embedded_label_factory_return.hpp"
#include "civ5/ui/bnw_scroll_animation_update_loan.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/ui/detail/notification_context_lua_controls_return.hpp"
#include "civ5/ui/detail/notification_context_postread_globals_reference.hpp"
#include "civ5/ui/detail/notification_context_postread_root_cdata_return.hpp"
#include "civ5/ui/detail/notification_context_ui_manager_value_loan.hpp"
#include "civ5/ui/detail/notification_context_tooltip_manager_operand.hpp"
#include "civ5/ui/detail/notification_context_loader_cleanup_return.hpp"
#include "civ5/runtime/bnw_unitflag_health_instance_loan.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "../../validation/ui_screen_gl.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cctype>
#include <cmath>
#include <charconv>
#include <limits>
#include <variant>
#include <type_traits>
#include <sstream>
#include <typeinfo>
#include <map>
#include <cstddef>
#include "detail/bnw_native_font_registry_key.hpp"
#include "civ5/content/info_types.hpp"
#include "bnw_native_color_load_kernel.hpp"
#include <xmmintrin.h>
#include "detail/bnw_native_font_integer.hpp"
#include "detail/bnw_native_font_optional_metadata.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/ui/detail/bnw_completed_font_loan.hpp"
#include "civ5/ui/detail/bnw_label_system_font_source.hpp"
#include "civ5/ui/detail/bnw_label_system_font_borrower.hpp"
#include "civ5/ui/detail/bnw_label_system_font_prepared_run.hpp"
#include "civ5/ui/detail/bnw_label_current_text_loan.hpp"
#include "civ5/ui/detail/bnw_system_font_native_run_data.hpp"
#include <optional>
#include "civ5/ui/detail/bnw_label_system_font_language_data.hpp"
#include "civ5/ui/detail/bnw_label_system_font_request.hpp"
#include "civ5/graphics/detail/bnw_label_system_font_atlas_metadata.hpp"
#include "civ5/ui/bnw_system_font_cpu_owner.hpp"
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include <set>
#include <utility>
#include <variant>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "detail/bnw_label_native_utf8_converter_core.inc"
namespace civ5::ui {
namespace {
void require(bool value,const char* why){if(!value)throw runtime::ScriptError(why);}
}
struct BnwWorldViewControlTree::Core {
    struct LayoutReceiverOccurrence;
#include "detail/notification_context_default_font_style_fields.inc"
    struct LayoutFourCReturn {
        std::array<float,2> actual_offset;
        std::uint64_t baseline_sequence,source_serial,topology_revision,offset_store_sequence;
    };
    // Unissued until genuine distinct5761E0 typed completion/DTOR publisher.
    std::shared_ptr<const LayoutInitializationReturn> layout_initialization_return;
    std::shared_ptr<const LayoutContextInitializationOrigin> layout_context_initialization_origin;
    std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> context_construction_return;
    std::shared_ptr<detail::NotificationContextGuiPort> context_gui_port;
    // Unissued until genuine8E1990 bank completion; readers never populate this field.
    std::weak_ptr<const detail::BnwNormalNonFontResourceBank> normal_nonfont_current_resource_bank;
    std::shared_ptr<const detail::NotificationLayoutContextRealmRef> context_realm_identity;
    // Partial constructor Base custody; independently distinct from canonical id0 parser occurrence.
    std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> context_ordinary_state;
    std::weak_ptr<ContextRootReadFileFrame> context_root_read_file_frame;
    std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> context_root_read_pending_completion;
    std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> context_root_distinct_attach_return;
    std::weak_ptr<ContextRootAttachEvidence> context_root_attach_evidence;
    //Only a genuine731370 input producer may publish this live lexical identity.
    std::weak_ptr<detail::NotificationContextNativeLoadNameInputLoan::Impl> context_load_name_frame;
    std::weak_ptr<ContextDirectLoadFrame> context_direct_load_frame;
    std::weak_ptr<const detail::NotificationContextDirectLuaTailInput> context_direct_lua_tail_input;
    std::shared_ptr<const detail::NotificationContextDirectLoadReturn> context_direct_load_pending_return;
    std::shared_ptr<const detail::NotificationContextDirectLoadReturn> context_direct_load_return;
    // Issued by the genuine child+10 publisher after its Frame DTOR.
    std::shared_ptr<const detail::NotificationLayoutChildSlot10Completion> context_child_slot10_return;
    std::weak_ptr<ContextXmlWrapperFrame> context_xml_wrapper_frame;
    std::weak_ptr<ContextRootChildrenEvidence> context_root_children_evidence;
    std::vector<std::shared_ptr<ContextRootChildAttempt>> context_root_child_attempts;
    std::weak_ptr<ContextChildControlsFrame> context_child_controls_frame;
    std::shared_ptr<const ContextRootStyleCursorValue> context_root_styles_value;
    std::shared_ptr<const LayoutReceiverOccurrence> context_ordinary_occurrence;
    struct SelectedContextColorSets final {
        const content::XmlNode selected_context;
        const std::uint64_t construction_serial;
        const std::map<std::uint32_t,detail::NotificationLabelColorSetKnownBytes> values{};
        const detail::NotificationLabelColorSetKnownBytes default_53c;
        static detail::NotificationLabelColorSetKnownBytes construct_default(){
            native_color_load_detail::ColorSetBytes scratch{};
            native_color_load_detail::default_color_set(scratch);
            detail::NotificationLabelColorSetKnownBytes result{};
            for(std::size_t i=0;i<63;++i)result[i]=scratch[i]; //540EB0 never writes3F.
            return result;
        }
        SelectedContextColorSets(content::XmlNode xml,std::uint64_t serial)
            :selected_context(std::move(xml)),construction_serial(serial),default_53c(construct_default()){
            require(serial&&selected_context.name=="Context","Local ColorSet setup needs actual Context construction");
            const auto scan=[](auto&& self,const content::XmlNode& row)->void{
                require(row.name!="ColorAtlas"&&row.name!="ColorSet"&&row.name!="ColorSets"&&row.name!="Color"&&row.name!="Colors",
                    "Local/dynamic ColorSet declarations need separately implemented genuine registration owner");
                for(const auto& child:row.children)self(self,child);
            };scan(scan,selected_context);
        }
    };
    std::shared_ptr<const SelectedContextColorSets> selected_context_color_sets;
    struct SelectedContextStyles final {
        const content::XmlNode selected_context;
        const std::uint64_t construction_serial;
        // Genuine portable Context-local registry allocation. This selected
        // profile has no local registration API; dynamic/local styles STOP.
        const std::map<std::string,content::XmlNode,std::less<>> values{};
        SelectedContextStyles(content::XmlNode xml,std::uint64_t serial):selected_context(std::move(xml)),construction_serial(serial){
            require(serial&&selected_context.name=="Context","Local Style setup requires actual Context construction");
            const auto scan=[](auto&& self,const content::XmlNode& row)->void{
                require(row.name!="Styles"&&row.name!="StyleSheet"&&row.name!="Style","Local/dynamic style declarations require their real registration owner");
                for(const auto& child:row.children)self(self,child);
            };scan(scan,selected_context);
        }
    };
    std::shared_ptr<const SelectedContextStyles> selected_context_styles;
    std::shared_ptr<const detail::NotificationContextSlot10LoadOrigin> context_load_origin;
    std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn> context_cursor_load_prefix;
    std::shared_ptr<const detail::NotificationContextLuaStatePreparation> context_lua_preparation;
    struct ContextNamedReceiver {std::weak_ptr<Core> core;std::uint64_t control=0;};
    std::map<std::string,ContextNamedReceiver,std::less<>> context_named_receivers;
    struct SelectedClientContextRegistry {
        struct Declaration {std::string stem;std::uint64_t parent,order;content::XmlNode layer;};
        struct Attachment {std::string id;std::uint64_t parent,order,serial;};
        std::map<std::string,Declaration,std::less<>> declarations;
        std::map<std::string,std::shared_ptr<BnwWorldViewControlTree>,std::less<>> children;
        std::vector<Attachment> history;
        std::uint64_t serial=0;
    };
    std::unique_ptr<SelectedClientContextRegistry> selected_client_registry;
    bool context_initial_lua_started=false;
    std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn> context_initial_lua_gc_return;
    std::shared_ptr<const detail::NotificationContextCursorRestoreReturn> context_cursor_restore_return;
    std::shared_ptr<const detail::NotificationContextLoaderCleanupReturn> context_loader_cleanup_return;
    bool context_initialization_active=false;
    std::optional<LayoutFourCReturn> context_layout_four_c_return;
    std::uint64_t context_layout_offset_store_sequence=0;
    struct LayoutScrollCallback {
        enum class Kind {up,down,thumb};
        std::weak_ptr<Core> source_core;
        std::uint64_t scroll_id;
        Kind kind;
    };
    struct LayoutReceiverOccurrence final {
        const std::uint64_t id,instance,constructor_serial;
        LayoutReceiverOccurrence(std::uint64_t i,std::uint64_t n,std::uint64_t s):id(i),instance(n),constructor_serial(s){}
    };
    // Sealed OWN occurrence, minted only while genuine Source construction or
    // instance cloning walks the selected XML. Nodes remain locators.
    struct FullXmlOccurrence final {
        const content::XmlNode value;
        const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
        const std::weak_ptr<const FullXmlOccurrence> parent;
        const std::size_t ordinal;
        const bool dynamic;
        FullXmlOccurrence(const content::XmlNode& xml,
            const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner>& selected,
            const std::shared_ptr<const FullXmlOccurrence>& actual_parent,
            std::size_t direct_ordinal,bool cloned)
            :value(xml),source(selected),parent(actual_parent),ordinal(direct_ordinal),dynamic(cloned){}
    };
    std::shared_ptr<const FullXmlOccurrence> root_full_xml_occurrence;
    std::shared_ptr<const FullXmlOccurrence> context_root_parsed_xml_occurrence;
    RootOrdinaryParserFrame* layout_parser_root_child_frame=nullptr;
    const CommonChildParserFrame* layout_parser_child_frame=nullptr;
    const std::map<std::string,content::XmlNode,std::less<>>* layout_parser_styles=nullptr;
    std::optional<std::uint64_t> layout_parser_control_id;
    const BnwWorldViewControlTree* layout_parser_facade=nullptr;
    const detail::NotificationLayoutRuntimeDimensions* layout_parser_borrower=nullptr;
    std::shared_ptr<const LayoutReceiverOccurrence> layout_parser_occurrence,context_receiver_occurrence;
    struct StackPending {
        struct Child {std::weak_ptr<Core> core;std::uint64_t id,order,parent_id,parser_serial,constructor_serial;
            std::weak_ptr<const LayoutReceiverOccurrence> occurrence;
            std::array<float,2> derived_offset,native_offset,native_size;bool derived_returned=false;};
        std::weak_ptr<Core> core;std::uint64_t id,parser_serial,constructor_serial,baseline_sequence;
        std::weak_ptr<const LayoutReceiverOccurrence> occurrence;
        std::array<float,2> size,extent,offset,anchor;
        std::array<float,4> bounds;
        std::vector<Child> children;
        bool size_returned=false,anchor_returned=false,empty_initializer=false,member_finished=false;
        std::weak_ptr<Core> cohort_owner;
        std::weak_ptr<const detail::NotificationLayoutContextInitializationReturn> cohort;
    };
    static std::vector<StackPending> capture_stack_view_owned(detail::NotificationLayoutParentReceiver::TreeView&);
    static void merge_stack_results_owned(std::vector<StackPending>&,std::vector<StackPending>);
    static void require_stack_pending_owned(const StackPending&,bool expected_busy=false);
    struct Node {
        std::uint64_t id,parent,instance;
        content::XmlNode authored;
        double alpha=1;
        bool hidden=false,alive=true,dynamic=false;
        bool disabled=false;
        std::optional<std::string> text,tooltip;
        std::uint64_t attached_order=0;
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
        std::optional<detail::NotificationLayoutBaseline> layout_baseline;
        std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> root_context_attach_return;
        std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn> root_ordinary_controls_return;
        std::shared_ptr<const LayoutReceiverOccurrence> layout_receiver_occurrence;
        std::shared_ptr<const detail::NotificationLayoutChildSlot10Completion> button_slot10_return;
        std::shared_ptr<const StackPending> stack_initializer_result;
        // No issuer is supplied here: common parser capture is not vslot authority.
        std::optional<detail::NotificationLayoutParentReceiver::BaseDispatch> layout_base_dispatch;
        std::optional<LayoutFourCReturn> layout_four_c_return;
        std::optional<LayoutScrollCallback> layout_scroll_callback;
        std::uint64_t layout_offset_store_sequence=0;
        std::optional<content::XmlNode> layout_parser_authored;
        std::shared_ptr<const FullXmlOccurrence> full_xml_occurrence;
        std::optional<bool> layout_hidden_command;
        // Only an actual protected SetConsumeMouseOver command earns bit800.
        // No constructor/full native flag word or hover membership is inferred.
        struct MouseConsumeCommand {std::uint32_t bit800;std::uint64_t source_serial,effect_revision;};
        std::optional<MouseConsumeCommand> mouse_consume_command;
        // Actual base ctor/parser exclusion bits, not a ready flag.
        std::uint32_t native_update_exclusion=0;
        std::uint64_t native_initial_parent=0;
        std::optional<detail::NotificationCpuGeometry> cpu_geometry;
    };
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::string context;
    std::vector<Node> nodes;
    std::uint64_t topology_revision=0;
    std::uint64_t effect_revision=0,next_attached_order=0;
    std::uint64_t layout_sequence=0;
    std::weak_ptr<const detail::BnwNormalRootContextEntry::Scope> normal_context_root_scope;
#include "detail/notification_context_initial_lua_journal_core.inc"
    std::weak_ptr<detail::NotificationLayoutResourceScope> layout_resource_scope;
    std::weak_ptr<const BnwWorldViewControlTree> layout_resource_tree;
    const BnwWorldViewControlTree* layout_resource_actual_tree=nullptr;
    const runtime::BnwWorldViewSourceOperationLoan* layout_resource_operation=nullptr;
    std::uint64_t layout_resource_serial=0;
    bool layout_resource_bound=false;
    void resource_first()const;
    void require_layout_resources_owned()const;
    // Absent until authentic Context construction/retained parser capture.
    // Only root bit2 is represented; other native flags are not invented.
    std::optional<std::uint32_t> context_hidden_bit;
    const content::XmlNode root_authored;
    std::optional<detail::NotificationLayoutBaseline> context_layout_baseline;
    std::optional<std::map<std::string,content::XmlNode,std::less<>>> layout_styles;
    std::weak_ptr<Core> context_parent_core;
    std::uint64_t context_parent_control_id=0;
    enum class ContextParentKind {canonical_context,canonical_control,native_null};
    // Disengaged is UNCAPTURED, never a native NULL observation. This producer
    // earns only Notification -> actual ActionInfoPanel context-root membership.
    std::optional<ContextParentKind> context_parent_kind;
    struct ContextLayoutChild {
        std::weak_ptr<Core> core;
        std::uint64_t parent_control_id,attached_order;
    };
    // UNCAPTURED by default. Only actual ordered Context construction history
    // can publish this complete vector; selected-context count/upward links cannot.
    std::optional<std::vector<ContextLayoutChild>> context_layout_children;
    std::map<std::string,std::shared_ptr<BnwWorldViewControlTree>,std::less<>> constructed_nested_contexts;
    struct ScalarParentAttachment {
        const std::weak_ptr<BnwWorldViewControlTree> child_tree,parent_tree;
        const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> child_ctor,parent_ctor;
        const std::string child_name,parent_name;
        const std::uint64_t parent_control_id,source_order,constructor_serial;
        ScalarParentAttachment(const std::shared_ptr<BnwWorldViewControlTree>& child,
            const std::shared_ptr<BnwWorldViewControlTree>& parent,std::uint64_t control,std::uint64_t order,std::uint64_t serial)
            :child_tree(child),parent_tree(parent),child_ctor(child->core_->context_construction_return),
             parent_ctor(parent?parent->core_->context_construction_return:nullptr),child_name(child->core_->context),
             parent_name(parent?parent->core_->context:std::string{}),parent_control_id(control),source_order(order),constructor_serial(serial){}
    };
    std::shared_ptr<const ScalarParentAttachment> scalar_parent_attachment;
    std::vector<std::shared_ptr<const ScalarParentAttachment>> scalar_child_attachment_history;
    // Issued only by the actual existing native factory finalizer, AFTER its
    // exhaustive authored traversal and every real child attachment command.
    // An empty commands vector is real only for a completed no-child factory.
    struct NativeContextChildVectorCompletion {
        const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> constructor;
        const content::XmlNode selected_document;
        const std::vector<std::shared_ptr<const ScalarParentAttachment>> commands;
        const std::uint64_t constructor_source_serial;
        NativeContextChildVectorCompletion(
            std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> ctor,
            content::XmlNode document,std::vector<std::shared_ptr<const ScalarParentAttachment>> actual_commands,
            std::uint64_t serial)
            :constructor(std::move(ctor)),selected_document(std::move(document)),
             commands(std::move(actual_commands)),constructor_source_serial(serial){}
    };
    std::shared_ptr<const NativeContextChildVectorCompletion> native_context_child_vector_completion;
    void require_native_context_child_vector_history_owned(bool expected_busy)const;
    std::map<std::string,content::XmlNode,std::less<>> templates;
    std::map<std::string,content::XmlNode,std::less<>> authored_template_declarations;
    mutable std::atomic<bool> failed{false};mutable bool busy=false;
    std::atomic<bool> facade_alive{true};
    Core(std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> value,std::string name,content::XmlNode authored):context_default_font_style(this),source(std::move(value)),context(std::move(name)),root_authored(std::move(authored)){
        context_layout_baseline.emplace(detail::NotificationLayoutBaseline::construct_owned(root_authored));
    }
    void creator_check()const {
        if(creator!=std::this_thread::get_id()){failed.store(true);throw runtime::ScriptError("WorldView controls foreign thread");}
        if(failed.load()||busy){failed.store(true);throw runtime::ScriptError("WorldView controls failed or reentered");}
    }
    void external_current(const std::vector<Node>& values)const {
            for(const auto& n:values)if(n.alive&&n.external_parent_id){
                require(context=="UnitFlagManager"&&n.dynamic&&!n.parent,"WorldView external parent child topology invalid");
                const auto actual=n.external_parent_tree.lock();require(static_cast<bool>(actual),"WorldView external parent tree retired");
                const auto& target=*actual->core_;
                require(target.creator==creator&&!target.failed.load()&&target.context=="InGame"&&
                    target.source.get()==source.get()&&!target.source.owner_before(source)&&!source.owner_before(target.source),
                    "WorldView external parent canonical source/creator failed");
                const auto& parent=node(target.nodes,n.external_parent_id);
                require(parent.authored.name=="Container"&&parent.authored.attribute("ID")=="SelectedUnitContainer"&&!parent.dynamic,
                    "WorldView external parent control substituted");
            }
    }
    void layout_current_owned()const {
        layout_origin();busy=true;struct Exit{bool&v;~Exit(){v=false;}}exit{busy};
        try {
            source->require_sources_current();layout_origin(true);
            external_current(nodes);layout_origin(true);
        }catch(...){failed.store(true);throw;}
    }
    void layout_origin(bool expected_busy=false)const {
        if(creator!=std::this_thread::get_id()){
            failed.store(true);require(false,"Layout foreign creator before source/node access");
        }
        if(!facade_alive.load()){
            failed.store(true);require(false,"Layout Tree facade retired before source/node access");
        }
        if(failed.load()||busy!=expected_busy){
            failed.store(true);require(false,"Layout failed or reentered");
        }
        try{source->current_operation_loan().require_render_operation_owned();}
        catch(...){failed.store(true);throw;}
    }
    void current()const {
        creator_check();busy=true;struct Exit{bool& busy;~Exit(){busy=false;}} exit{busy};
        try{source->require_sources_current();require(!failed.load(),"WorldView controls poisoned during source validation");
            external_current(nodes);
        }
        catch(...){failed.store(true);throw;}
    }
    void scroll_origin(bool expected_busy=false)const {
        // Saved Core is independent of the facade and survives a callback that
        // drops the Tree. Check retirement FIRST before source/node access.
        require(creator==std::this_thread::get_id(),"Scroll effects foreign creator");
        require(facade_alive.load(),"Scroll effects Tree facade retired");
        require(!failed.load()&&busy==expected_busy,"Scroll effects failed or reentered");
        try{source->current_operation_loan().require_render_operation_owned();}
        catch(...){failed.store(true);throw;}
    }
    void scroll_current_owned()const {
        // Unlike ordinary current(), this new lane checks saved facade life
        // FIRST after callback return, before external topology/node reads.
        scroll_origin();busy=true;struct Exit{bool& value;~Exit(){value=false;}} exit{busy};
        try {
            source->require_sources_current();scroll_origin(true);
            external_current(nodes);scroll_origin(true);
        }catch(...){failed.store(true);throw;}
    }
    template<class F,class P> void mutate_scroll_owned(F&& effect,P&& postflight){
        try {
            scroll_origin();scroll_current_owned();scroll_origin();
            busy=true;struct Exit{bool& value;~Exit(){value=false;}} exit{busy};
            require(effect_revision<std::numeric_limits<std::uint64_t>::max(),"Scroll effect revision exhausted");
            initial_lua_preflight_store_owned();
            auto next=nodes;effect(next);scroll_origin(true);
            source->require_sources_current();scroll_origin(true);
            external_current(next);scroll_origin(true);postflight();scroll_origin(true);
            nodes.swap(next);++effect_revision;initial_lua_record_store_owned(InitialLuaWriter::scroll_command);
        }catch(...){failed.store(true);throw;}
    }
    bool selected_scroll_eligible_without_callbacks(std::uint64_t id)const {
        const auto& n=node(nodes,id);
        require(n.authored.name=="ScrollAnim"&&context=="NotificationPanel"&&!n.dynamic&&!n.instance&&n.parent,
            "Scroll native eligibility outside selected static Notification context");
        const auto& button=node(nodes,n.parent);
        require(button.authored.name=="Button"&&
            (button.authored.attribute("ID")=="TechButton"||button.authored.attribute("ID")=="FreeTechButton"),
            "Scroll selected ordinary parent differs from native source contract");
        bool excluded=false;std::set<std::uint64_t> seen;
        for(auto current=n.id;current;){
            require(seen.insert(current).second,"Scroll eligibility parent cycle");
            const auto& p=node(nodes,current);
            require(p.parent==p.native_initial_parent&&!p.dynamic&&!p.external_parent_id&&!p.world_position,
                "Scroll native eligibility requires unmodified actual static parent path");
            require(p.authored.name!="ShowOnMouseOver"&&p.authored.name!="HideOnMouseOver",
                "Scroll native mouseover list eligibility producer not represented");
            require((p.native_update_exclusion&~0x2002u)==0&&bool(p.native_update_exclusion&2)==p.hidden,
                "Scroll native exclusion state malformed");
            excluded=excluded||bool(p.native_update_exclusion&0x2002u);current=p.parent;
        }
        return !excluded;
    }
    static bool same_layout_xml(const content::XmlNode& a,const content::XmlNode& b){
        if(a.name!=b.name||a.text!=b.text||a.attributes!=b.attributes||a.children.size()!=b.children.size())return false;
        for(std::size_t i=0;i<a.children.size();++i)if(!same_layout_xml(a.children[i],b.children[i]))return false;
        return true;
    }
    static bool same_layout_styles(const std::map<std::string,content::XmlNode,std::less<>>& a,
                                   const std::map<std::string,content::XmlNode,std::less<>>& b){
        if(a.size()!=b.size())return false;
        auto x=a.begin(),y=b.begin();for(;x!=a.end();++x,++y)
            if(x->first!=y->first||!same_layout_xml(x->second,y->second))return false;
        return true;
    }
    static std::vector<content::XmlNode> layout_parser_layers(const content::XmlNode& authored,
        const std::map<std::string,content::XmlNode,std::less<>>& styles){
        std::vector<const content::XmlNode*> chain;std::set<std::string> seen;const auto* selected=&authored;
        while(!selected->attribute("Style").empty()){
            const auto name=selected->attribute("Style");require(seen.insert(name).second,"Layout retained style cycle");
            const auto found=styles.find(name);require(found!=styles.end(),"Layout retained actual style missing");
            selected=&found->second;chain.push_back(selected);
        }
        std::vector<content::XmlNode> result;result.reserve(chain.size()+1);
        for(auto it=chain.rbegin();it!=chain.rend();++it)result.push_back(**it);
        result.push_back(authored);return result;
    }
    static void clone_layout_states(std::vector<Node>& values){
        for(auto& value:values)if(value.layout_baseline){
            auto copy=detail::NotificationLayoutBaseline::clone_owned(*value.layout_baseline);
            value.layout_baseline.emplace(std::move(copy));
        }
    }
    // Canonical parser construction is defined by the actual Tree issuer below.
    template<class F> auto mutate_layout_baseline_owned(F&& mutation){
        try{
            layout_origin();layout_current_owned();layout_origin();
            const auto& operation=source->current_operation_loan();layout_origin();
            const auto serial=operation.operation_serial();layout_origin();
            busy=true;struct Exit{bool&v;~Exit(){v=false;}}exit{busy};
            require(effect_revision<UINT64_MAX,"Layout baseline effect revision exhausted");
            initial_lua_preflight_store_owned();
            auto next=nodes;clone_layout_states(next);auto result=mutation(next);layout_origin(true);
            source->require_sources_current();layout_origin(true);
            external_current(next);layout_origin(true);
            operation.require_operation(serial);layout_origin(true);
            require(&source->current_operation_loan()==&operation,"Layout command SAME source operation replaced");layout_origin(true);
            nodes.swap(next);++effect_revision;initial_lua_record_store_owned(InitialLuaWriter::layout_command);return result;
        }catch(...){failed.store(true);throw;}
    }
    static float scroll_number(const content::XmlNode& xml,std::string_view name,float fallback){
        const auto text=xml.attribute(std::string(name));if(text.empty())return fallback;
        // Original TechTree legend authors Speed="0.07    ". Numeric
        // boundary whitespace is formatting, not a different speed/default.
        // Keep full token consumption, finite and selected nonnegative profile.
        std::string_view token{text};
        const auto whitespace=[](char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\f'||c=='\v';};
        while(!token.empty()&&whitespace(token.front()))token.remove_prefix(1);
        while(!token.empty()&&whitespace(token.back()))token.remove_suffix(1);
        float value=0;const auto parsed=std::from_chars(token.data(),token.data()+token.size(),value);
        require(parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&std::isfinite(value)&&value>=0,
            "Scroll authored numeric value unsupported");return value;
    }
    static ScrollAnimationState authored_scroll(const content::XmlNode& xml){
        require(xml.attribute("Cycle").empty()&&xml.attribute("Function").empty()&&xml.attribute("Stopped").empty(),
            "Scroll explicit Cycle/Function/Stopped profile awaits actual native mapping");
        ScrollAnimationState value;
        value.speed=scroll_number(xml,"Speed",1);value.pause=scroll_number(xml,"Pause",0);
        value.end_pause=scroll_number(xml,"EndPause",0);
        // Native ctor zeroes +84/+94; parser Pause updates +8C, NOT +94.
        // Data defaults do not issue the native constructor clock/sample.
        return value;
    }
    static void reset_scroll(ScrollAnimationState& value){
        require(value.command_sequence<std::numeric_limits<std::uint64_t>::max(),"Scroll reset sequence exhausted");
        ++value.command_sequence;value.progress=0;value.sample=0;
        value.pause_remaining=value.pause;value.reversed=false;value.stopped=false;value.sample_issued=true;
    }
    static void advance_scroll(ScrollAnimationState& value,float delta,std::uint64_t tick){
        require(std::isfinite(delta)&&delta>=0&&tick>value.tick_sequence,"Scroll tick invalid/replayed");
        value.tick_sequence=tick;
        if(value.stopped)return;
        if(value.pause_remaining>0){
            value.pause_remaining=value.pause_remaining-delta;
            require(std::isfinite(value.pause_remaining),"Scroll pause arithmetic overflow");
            return; // Entire native tick is consumed, including pause overshoot.
        }
        // MULSS then ADDSS: force separate float product, no analytic modulo.
        volatile float product=value.speed*delta;
        value.progress=value.progress+product;
        require(std::isfinite(value.progress),"Scroll progress arithmetic overflow");
        if(value.progress<1){value.sample=value.progress;value.sample_issued=true;return;}
        // Native default cycle1 (LOOP): threshold sample1 then discard overshoot.
        value.sample=1;value.sample_issued=true;value.progress=0;
        if(value.end_pause>0)value.pause_remaining=value.end_pause;
        else{value.sample=0;value.pause_remaining=value.pause;}
    }
    static const Node& node(const std::vector<Node>& nodes,std::uint64_t id){
        require(id>0&&id<=nodes.size()&&nodes[id-1].alive,"WorldView control removed or unknown");return nodes[id-1];
    }
    // Owning Tree mutation lane; distinct name preserves all historical const reads.
    static Node& mutable_node_owned(std::vector<Node>& nodes,std::uint64_t id){
        require(id>0&&id<=nodes.size()&&nodes[id-1].alive,"WorldView control removed or unknown");return nodes[id-1];
    }
    std::uint64_t checked(const Control& control)const {
        const auto owner=control.owner_.lock();
        require(owner&&owner.get()==this,"WorldView control belongs to another canonical tree");
        (void)node(nodes,control.id_);return control.id_;
    }
    static std::uint64_t append(std::vector<Node>& output,const content::XmlNode& authored,
            std::uint64_t parent,std::uint64_t instance,bool dynamic,std::set<std::string>& ids,std::uint64_t& order,
        const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner>& source,
        const std::shared_ptr<const FullXmlOccurrence>& parent_occurrence,std::size_t ordinal){
        require(output.size()<16384,"WorldView canonical control budget exhausted");
        const auto name=authored.attribute("ID");
        require(name.empty()||ids.insert(name).second,"WorldView XML control ID ambiguous within one instance");
        const auto id=static_cast<std::uint64_t>(output.size()+1);
        if(dynamic&&!instance)instance=id;
        // Preserve wrappers, unsupported elements and complete authored attrs.
        // InstanceData remains a template, not a fabricated live PullDown entry.
        Node value{id,parent,instance,authored};value.dynamic=dynamic;
        // Retain genuine nested parser children (not fabricated live handles).
        // Slider Track/Shuttle parser needs their actual style/source layers.
        if(authored.name=="ScrollBar")value.layout_parser_authored=authored;
        const auto occurrence=std::make_shared<const FullXmlOccurrence>(authored,source,parent_occurrence,ordinal,dynamic);
        value.full_xml_occurrence=occurrence;
        value.native_initial_parent=parent;
        require(order<std::numeric_limits<std::uint64_t>::max(),"WorldView attachment order exhausted");
        value.attached_order=++order;
        // The source owner/templates retain complete XML. Canonical nodes keep
        // their own attrs/text and explicit parent, avoiding repeated subtrees.
        value.authored.children.clear();
        const auto hidden=authored.attribute("Hidden");
        require(hidden.empty()||hidden=="True"||hidden=="False"||hidden=="true"||hidden=="false"||hidden=="0"||hidden=="1",
                "WorldView XML Hidden spelling unsupported");
        value.hidden=hidden=="True"||hidden=="true"||hidden=="1";
        if(value.hidden)value.native_update_exclusion|=2;
        if(parent&&output.at(parent-1).authored.name=="ShowOnMouseOver")value.native_update_exclusion|=0x2000;
        if(authored.name=="ScrollAnim")value.scroll_animation=authored_scroll(authored);
        output.push_back(std::move(value));
        for(std::size_t child_ordinal=0;child_ordinal<authored.children.size();++child_ordinal){
            const auto& child=authored.children[child_ordinal];
            if(child.name!="Instance"&&child.name!="InstanceData"&&child.name!="LuaContext")
                append(output,child,id,instance,dynamic,ids,order,source,occurrence,child_ordinal);
        }
        return id;
    }
    template<class F> auto mutate(F&& mutation){
        layout_origin();current();busy=true;struct Exit{bool& busy;~Exit(){busy=false;}} exit{busy};
        try {
            require(effect_revision<std::numeric_limits<std::uint64_t>::max(),"WorldView effect revision exhausted");
            initial_lua_preflight_store_owned();
            auto next=nodes;auto result=mutation(next);
            source->require_sources_current();require(!failed.load(),"WorldView controls poisoned before effect commit");
            external_current(next);
            nodes.swap(next);++effect_revision;initial_lua_record_store_owned(InitialLuaWriter::ordinary_command);return result;
        }catch(...){failed.store(true);throw;}
    }
    double alpha(std::uint64_t id)const {
        double result=1;
        while(id){const auto& value=node(nodes,id);result*=value.alpha;
            require(std::isfinite(result),"WorldView inherited alpha overflow");
            if(value.external_parent_id){
                const auto target=value.external_parent_tree.lock();require(static_cast<bool>(target),"WorldView external alpha parent retired");
                const auto& actual=*target->core_;require(actual.creator==creator&&!actual.failed.load(),"WorldView external alpha parent failed");
                result*=actual.alpha(value.external_parent_id);require(std::isfinite(result),"WorldView external alpha overflow");
            }
            id=value.parent;
        }
        return result;
    }
    bool hidden(std::uint64_t id)const {
        while(id){const auto& value=node(nodes,id);if(value.hidden)return true;
            if(value.external_parent_id){
                const auto target=value.external_parent_tree.lock();require(static_cast<bool>(target),"WorldView external hidden parent retired");
                const auto& actual=*target->core_;require(actual.creator==creator&&!actual.failed.load(),"WorldView external hidden parent failed");
                if(actual.hidden(value.external_parent_id))return true;
            }
            id=value.parent;
        }return false;
    }
};
#include "detail/notification_context_construction_core.inc"
#include "detail/notification_context_font_registry_fields.inc"
#include "detail/bnw_label_system_font_registry_fields.inc"
#include "detail/notification_context_gui_port_core.inc"
#include "detail/bnw_original_nonfont_resource_scalars.inc"
#include "detail/bnw_original_nonfont_device_initializer_core.inc"
#include "detail/bnw_original_generated_resource_image_core.inc"
#include "detail/bnw_original_resource_sampler_descriptor_core.inc"
#include "detail/bnw_original_vertex_declaration_setup_core.inc"
#include "detail/bnw_original_vertex_declaration_init_phase_core.inc"
#include "detail/bnw_font_texture_request_core.inc"
#include "detail/bnw_completed_font_loan_core.inc"
#include "detail/notification_context_font_registry_lookup_core.inc"
#include "detail/bnw_label_system_font_source_core.inc"
#include "detail/bnw_label_system_font_request_guard_core.inc"
#include "detail/bnw_label_system_font_borrower_core.inc"
#include "detail/notification_gui_system_font_lookup_core.inc"
#include "detail/notification_layout_runtime_dimensions_core.inc"
#include "detail/notification_scalar_parent_chain_core.inc"
#include "detail/notification_context_cursor_load_core.inc"
#include "detail/notification_context_lua_state_prepare_core.inc"
#include "notification_layout_parent_core.inc"
#include "detail/notification_layout_box_parent_return_core.inc"
#include "detail/notification_layout_resize_98_core.inc"
#include "detail/notification_layout_native_offset_pair_source28_core.inc"
#include "detail/notification_layout_native_offset_getter_source12_core.inc"
#include "detail/notification_layout_native_flags_children_source28_core.inc"
#include "detail/notification_layout_native_anchor_source14_core.inc"
#include "detail/notification_layout_native_reprocess_size_source15_core.inc"
#include "detail/notification_layout_native_setsize_source16_core.inc"
#include "detail/notification_layout_native_color_alpha_source17_core.inc"
#include "detail/notification_layout_native_branch_id_source28_core.inc"
#include "detail/notification_layout_native_disabled_visibility_source21_core.inc"
#include "detail/notification_layout_native_tooltip_source22_core.inc"
#include "detail/notification_layout_native_parent_release_source28_core.inc"
#include "notification_layout_resource_core.inc"
#include "detail/notification_layout_scroll_core.inc"
#include "detail/notification_stack_result_capture_core.inc"
#include "detail/notification_context_initializer_core.inc"
#include "detail/notification_context_direct_initializer_core.inc"
#include "detail/notification_context_native_lua_fields_core.inc"
#include "detail/notification_context_child_slot10_core.inc"
#include "detail/notification_context_initial_lua_core.inc"
#include "detail/notification_context_lookup_control_relative_core.inc"
#include "detail/notification_context_showhide_core.inc"
#include "detail/notification_context_post_lua_suffix_core.inc"
#include "detail/notification_context_direct_postlua_tail_core.inc"
#include "detail/notification_context_initial_graph_origin_core.inc"
#include "detail/bnw_selected_client_context_registry_core.inc"
#include "detail/bnw_selected_nested_context_construction_core.inc"
#include "detail/bnw_selected_retained_context_enumeration_core.inc"
#include "detail/notification_container_slot10_core.inc"
#include "detail/notification_layout_grid_initializer_core.inc"
#include "detail/notification_layout_embedded_label_initializer_core.inc"
#include "detail/notification_layout_standalone_label_initializer_prefix_core.inc"
#include "detail/notification_layout_label_realm_live_run_core.inc"
#include "detail/notification_layout_positioned_label_run_core.inc"
#include "detail/notification_layout_live_settext_core.inc"
#include "detail/notification_layout_entered_paint_run_core.inc"
#include "detail/bnw_label_paint_text_loan_core.inc"
#include "detail/bnw_label_paint_cpu_numeric_core.inc"
#include "detail/bnw_label_paint_cpu_text_records_core.inc"
#include "detail/bnw_label_paint_positioned_data_core.inc"
#include "detail/bnw_label_system_font_prepared_run_core.inc"
#include "detail/notification_layout_button_initializer_core.inc"
#include "detail/notification_layout_button_visual_core.inc"
#include "detail/notification_stack_result_publish_core.inc"
BnwWorldViewControlTree::Control::Control(const std::shared_ptr<Core>& owner,std::uint64_t id):owner_(owner),id_(id){}
BnwWorldViewControlTree::BnwWorldViewControlTree(std::shared_ptr<Core> core):core_(std::move(core)){}
BnwWorldViewControlTree::~BnwWorldViewControlTree(){
    core_->facade_alive.store(false); //FIRST before any destructor-specific child access
    if(core_->context_ordinary_state)core_->context_ordinary_state->retire_owned();
    detail::NotificationLayoutRuntimeDimensions::retire_tree_labels_owned(*this);
    if(core_->context_realm_identity)core_->context_realm_identity->retire_owned();
}
std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> BnwWorldViewControlTree::metadata_source_owner()const {
    // Factory-only metadata access. It does not borrow/renew a source operation
    // or call NativeContextIssuer; the adapter will request its actual active loan.
    core_->creator_check();return core_->source;
}
bool BnwWorldViewControlTree::context_update_hidden_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan& operation)const {
    const auto saved=core_;
    try {
        saved->layout_origin(); // creator/facade/poison/reentry FIRST
        const auto& actual=saved->source->current_operation_loan();
        saved->layout_origin();
        require(&actual==&operation,"Context visibility SAME actual operation substituted");
        const auto serial=actual.operation_serial();saved->layout_origin();
        require(saved->context_hidden_bit.has_value(),"Context visibility construction not captured");
        require((*saved->context_hidden_bit&~std::uint32_t(2))==0,"Context visibility bit history corrupt");
        const bool result=(*saved->context_hidden_bit&2)!=0;
        const auto& after=saved->source->current_operation_loan();saved->layout_origin();
        require(&after==&operation&&after.operation_serial()==serial,"Context visibility source scope changed");
        saved->layout_origin();return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<BnwWorldViewControlTree> BnwWorldViewControlTree::capture_owned(
    std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source,std::string_view context,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<detail::NotificationContextGuiPort>& gui_port){
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    require(bool(gui_port),"Context capture actual canonical GUI port absent");gui_port->require_current_owned(operation);
    require(source && &source->current_operation_loan()==&operation,"Context capture SAME entered Source substituted");
    require(source&&(context=="InGame"||context=="WorldView"||context=="UnitFlagManager"||context=="ActionInfoPanel"||context=="NotificationPanel"||source->prepared_source(context)),"WorldView control owner source unavailable");
    source->require_sources_current();const auto& layout=source->authored_layout(context);
    require(layout.name=="Context","WorldView actual retained layout root is not Context");
    auto root_authored=layout;root_authored.children.clear();
    // Retain only actual direct child-context declarations as value metadata.
    // Canonical Controls append below still skips LuaContext; no handle or grant.
    for(const auto& child:layout.children)
        if(child.name=="LuaContext")root_authored.children.push_back(child);
    // Stage the native ctor0 -> actual Hidden parser state ONCE, during
    // genuine source/context construction. Runtime getters never reread XML.
    if(context!="InGame"){
        const auto parent=source->selected_context_declared_parent_owned(context);
        const auto parent_layout=source->authored_layout(parent);
        source->require_sources_current();
        std::size_t matches=0;
        const auto declaration=[&](auto&& self,const content::XmlNode& xml)->void {
            auto declared_id=xml.attribute("ID");
            if(xml.name=="LuaContext"&&declared_id.empty()){
                const auto stem=xml.attribute("FileName");const auto slash=stem.find_last_of("/\\");
                declared_id=stem.substr(slash==std::string::npos?0:slash+1);
            }
            if(xml.name=="LuaContext"&&declared_id==context){
                const auto descriptor=source->prepared_source(context);
                require(descriptor&&descriptor->context()==context,
                    "Context selected constructor descriptor differs");
                const auto key=descriptor->asset().requested_key().original_name();
                require(assets::AssetKey(key).normalized_name()==assets::AssetKey(xml.attribute("FileName")+".lua").normalized_name(),
                    "Context selected actual declaration source differs");
                // Selected own constructor retains and parses declaration
                // overrides over the real VFS root. No hidden/default skip.
                for(const auto& attribute:xml.attributes){
                    if(attribute.first=="FileName"||attribute.first=="ID")continue;
                    auto found=std::find_if(root_authored.attributes.begin(),root_authored.attributes.end(),
                        [&](const auto& a){return a.first==attribute.first;});
                    if(found==root_authored.attributes.end())root_authored.attributes.push_back(attribute);
                    else found->second=attribute.second;
                }
                ++matches;
            }
            for(const auto& child:xml.children)self(self,child);
        };
        declaration(declaration,parent_layout);
        require(matches==1,"Context visibility actual parent declaration missing or ambiguous");
    }
    const auto root_hidden=root_authored.attribute("Hidden");
    require(root_hidden.empty()||root_hidden=="True"||root_hidden=="False"||root_hidden=="true"||root_hidden=="false"||
        root_hidden=="0"||root_hidden=="1","Context selected Hidden spelling unsupported");
    const std::uint32_t captured_hidden=!root_hidden.empty()&&
        (root_hidden[0]=='t'||root_hidden[0]=='T'||root_hidden[0]=='1')?2:0;
    source->require_sources_current();
    auto core=std::make_shared<Core>(source,std::string(context),std::move(root_authored));std::set<std::string> ids;
    const auto full_root=std::make_shared<const Core::FullXmlOccurrence>(layout,source,nullptr,0,false);
    source->require_sources_current();operation.require_origin_without_callbacks();
    require(&source->current_operation_loan()==&operation,"Full selected XML Source operation changed");
    core->root_full_xml_occurrence=full_root;
    core->context_gui_port=gui_port;
    core->context_construction_return=capture_context_constructor_owned(core,operation);
    core->layout_origin();operation.require_origin_without_callbacks();
    const auto staged_context_color_sets=std::make_shared<const Core::SelectedContextColorSets>(core->root_authored,operation.operation_serial());
    core->layout_origin();operation.require_origin_without_callbacks();
    core->selected_context_color_sets=staged_context_color_sets; //Nonalloc sealed-owner publication.
    core->layout_origin();operation.require_origin_without_callbacks();
    core->layout_origin();operation.require_origin_without_callbacks();
    core->selected_context_styles=std::make_shared<const Core::SelectedContextStyles>(core->root_authored,operation.operation_serial());
    core->layout_origin();operation.require_origin_without_callbacks();
    core->context_hidden_bit=captured_hidden; // private staged state; published only after existing full Source POST
    for(std::size_t child_ordinal=0;child_ordinal<layout.children.size();++child_ordinal){
        const auto& child=layout.children[child_ordinal];
        if(child.name=="Instance"){
            const auto name=child.attribute("Name");
            require(std::any_of(child.attributes.begin(),child.attributes.end(),[](const auto& a){return a.first=="Name";})&&
                core->authored_template_declarations.emplace(name,child).second,
                "Context actual template Name field missing or declaration repeated");
            // Retain actual unused declarations, including an empty Name or
            // no root. They are data, never dummy live controls. Cloning still
            // requires a real single root and its physical constructor path.
            if(child.children.size()==1)
                require(core->templates.emplace(name,child.children.front()).second,
                    "Context actual single-root template repeated");
        }else if(child.name!="LuaContext")Core::append(core->nodes,child,0,0,false,ids,core->next_attached_order,source,full_root,child_ordinal);
    }
    // Hold actual Source/Core VALUES before facade allocation. Create the
    // genuine portable40F Context field owner before any wrapper/full parse.
    // This is not a VM creator or successful4FEE80/RootRead return.
    const auto held_source=source;const auto*const held_operation=&operation;
    auto tree=std::shared_ptr<BnwWorldViewControlTree>(new BnwWorldViewControlTree(core));
    const auto constructor_current=[&]{
        held_source->require_sources_current();
        require(&held_source->current_operation_loan()==held_operation,
            "Context creation canonical operation replaced before old raw operand");
        core->layout_origin();
        require(core->source.get()==held_source.get()&&!core->source.owner_before(held_source)&&
            !held_source.owner_before(core->source),"Context creation SAME Source control changed");
    };
    constructor_current();held_operation->require_current();constructor_current();
    tree->publish_context_ordinary_receiver_projection_owned(*held_operation);constructor_current();
    tree->construct_context_native_prefix_owned(*held_operation);constructor_current();
    tree->construct_context_native_suffix_owned(*held_operation);constructor_current();
    held_operation->require_current();constructor_current();return tree;
}
void BnwWorldViewControlTree::initialize_notification_layout_baselines_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::map<std::string,content::XmlNode,std::less<>>& actual_styles,
    const BnwWorldViewControlTree& actual_action_context){
    const auto saved=core_,parent=actual_action_context.core_;
    try{
        saved->layout_origin();parent->layout_origin();
        // Copy borrowed source values BEFORE any callback. Saved Cores retain
        // storage only: facade_alive FIRST still rejects last owner retirement.
        auto styles=actual_styles;
        initialize_scalar_parent_baselines_owned(operation,styles);
        const auto pure=[&](bool busy=false){
            saved->layout_origin(busy);parent->layout_origin();
            require(saved.get()!=parent.get()&&saved->context=="NotificationPanel"&&parent->context=="ActionInfoPanel"&&
                saved->source.get()==parent->source.get()&&!saved->source.owner_before(parent->source)&&
                !parent->source.owner_before(saved->source),"Layout canonical context/source parent substituted");
        };
        pure();saved->layout_current_owned();pure();parent->layout_current_owned();pure();
        operation.require_current();pure();const auto serial=operation.operation_serial();pure();
        require(&saved->source->current_operation_loan()==&operation&&
                &parent->source->current_operation_loan()==&operation,"Layout parser SAME source operation replaced");pure();
        const auto selected=saved->source->prepared_source(saved->context);pure();
        operation.require_same_source(*selected);pure();
        const content::XmlNode* declaration=nullptr;
        for(const auto& child:parent->root_authored.children)
            if(child.name=="LuaContext"&&child.attribute("ID")=="NotificationPanel"){
                require(!declaration&&child.attribute("FileName")=="Assets/UI/InGame/WorldView/NotificationPanel",
                    "Layout actual direct Notification declaration changed");declaration=&child;
            }
        require(declaration,"Layout actual ActionInfoPanel context declaration absent");
        // Selected current declaration has no extra geometry. Do not create a
        // guessed synthetic context layer or map the client viewport to it.
        for(const auto key:{"Style","Size","Offset","Anchor","AnchorSide","Hidden","Parent"})
            require(declaration->attribute(key).empty(),"Layout direct context parser layer needs genuine new supplier");
        if(saved->layout_styles){
            const auto previous=saved->context_parent_core.lock();
            require(saved->context_parent_kind==Core::ContextParentKind::canonical_context&&previous&&
                previous.get()==parent.get()&&!previous.owner_before(parent)&&!parent.owner_before(previous)&&
                Core::same_layout_styles(*saved->layout_styles,styles),"Layout retained styles/context changed");
            // previous storage lock is memory-only and ends before callback POST.
        }else{
            saved->busy=true;struct Exit{bool&v;~Exit(){v=false;}}exit{saved->busy};pure(true);
            require(saved->context_construction_return&&saved->context_layout_baseline&&
                !saved->context_layout_baseline->parsed_source_serial_,"Context parser genuine retained constructor absent or replayed");
            // Actual constructor-owned root and actual Core.nodes are the
            // receivers BEFORE Size/register. No staged vector may issue it.
            capture_layout_node_canonical_owned(0,styles,serial);
            for(std::size_t i=0;i<saved->nodes.size();++i)
                if(saved->nodes[i].alive)capture_layout_node_canonical_owned(saved->nodes[i].id,styles,serial);
            saved->source->require_sources_current();pure(true);
            operation.require_operation(serial);pure(true);
            require(&saved->source->current_operation_loan()==&operation,"Layout operation replaced before parser publication");pure(true);
            saved->layout_styles.emplace(std::move(styles));
            saved->context_parent_core=parent;saved->context_parent_control_id=0;
            saved->context_parent_kind=Core::ContextParentKind::canonical_context;
        }
        operation.require_operation(serial);pure();
        if(!saved->context_load_origin)prepare_context_slot10_load_origin_owned(operation);
        pure();
    }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::require_current()const {core_->current();}
void BnwWorldViewControlTree::poison()const noexcept {core_->failed.store(true);}
std::string BnwWorldViewControlTree::context()const {core_->current();return core_->context;}
std::shared_ptr<const runtime::ScriptUiChildSource> BnwWorldViewControlTree::actual_source()const {
    core_->current();return core_->source->prepared_source(core_->context);
}
std::vector<std::pair<std::string,BnwWorldViewControlTree::Control>> BnwWorldViewControlTree::static_controls()const {
    core_->current();std::vector<std::pair<std::string,Control>> result;
    for(const auto& node:core_->nodes)if(node.alive&&!node.dynamic){
        const auto name=node.authored.attribute("ID");
        if(!name.empty())result.emplace_back(name,Control(core_,node.id));
    }
    core_->current();return result;
}
BnwWorldViewControlTree::ControlQuote BnwWorldViewControlTree::inspect_control(const Control& control)const {
    core_->current();const auto& node=Core::node(core_->nodes,core_->checked(control));
    //Read the actual earned canonical Image parser field for input admission;
    //authored metadata alone cannot disable a node. Existing command storage
    //and Button/GridButton DynamicSetDisabled behavior stay unchanged.
    const auto& baseline=node.layout_baseline;
    const bool disabled=node.disabled||(baseline&&baseline->parsed_source_serial_&&baseline->image_&&
        baseline->kind_==detail::NotificationLayoutBaseline::Kind::image&&baseline->actual_type_=="Image"&&
        (baseline->base_.flags_40&0x20000u));
    ControlQuote quote{node.id,node.parent,node.authored,node.hidden,node.alive,node.dynamic,disabled,node.alpha,node.text,node.tooltip,
        node.instance,core_->effect_revision,node.world_position,node.offset,node.color,node.fg_color,node.texture,node.texture_offset,node.percent,node.animation,
        node.external_parent_tree,node.external_parent_id,node.layout,node.size_y_override,node.scroll_animation,node.cpu_geometry};
    core_->current();return quote;
}
BnwWorldViewControlTree::Control BnwWorldViewControlTree::static_control(std::string_view name)const {
    core_->current();std::uint64_t id=0;
    for(const auto& value:core_->nodes)if(value.alive&&!value.dynamic&&value.authored.attribute("ID")==name){
        require(!id,"WorldView static control ID ambiguous");id=value.id;
    }
    require(id,"WorldView static control unavailable");return Control(core_,id);
}
BnwWorldViewControlTree::Instance BnwWorldViewControlTree::build_instance(std::string_view name,const Control& parent){
    const auto saved=core_;saved->layout_origin();saved->initial_lua_require_existing_topology_owned();
    const auto declaration=saved->authored_template_declarations.find(name);
    require(declaration!=saved->authored_template_declarations.end(),
        "GetInstance actual template declaration absent");
    require(declaration->second.children.size()==1,
        "GetInstance actual template has no supported single-root physical constructor");
    const auto selected=saved->templates.find(name);
    require(selected!=saved->templates.end()&&selected->second.name==declaration->second.children.front().name&&
        selected->second.attributes==declaration->second.children.front().attributes,
        "GetInstance actual retained single-root template differs from its declaration");
    saved->layout_origin();
    if(saved->layout_styles){
        const auto actual=parent;const std::string requested(name);
        saved->layout_current_owned();saved->layout_origin();const auto pid=saved->checked(actual);
        const auto found=saved->templates.find(requested);require(found!=saved->templates.end(),"Layout actual dynamic template missing");
        const auto authored=found->second;auto order=saved->next_attached_order;
        const auto& operation=saved->source->current_operation_loan();saved->layout_origin();
        const auto serial=operation.operation_serial();saved->layout_origin();
        require(saved->topology_revision<UINT64_MAX,"Layout dynamic topology exhausted");
        require(saved->effect_revision<UINT64_MAX,"Layout dynamic effect history exhausted");
        saved->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{saved->busy};
        try{
            saved->layout_origin(true);const auto first=saved->nodes.size();std::set<std::string> ids;
            const auto parent_xml=pid?Core::node(saved->nodes,pid).full_xml_occurrence:saved->root_full_xml_occurrence;
            const auto rid=Core::append(saved->nodes,authored,pid,0,true,ids,order,saved->source,parent_xml,0);
            saved->next_attached_order=order;++saved->topology_revision;++saved->effect_revision;
            // Publish actual construction storage first; failed parser keeps
            // physical prefix quarantined instead of reverting a false copy.
            for(std::size_t i=first;i<saved->nodes.size();++i)
                capture_layout_node_canonical_owned(saved->nodes[i].id,*saved->layout_styles,serial);
            Instance out{Control(saved,rid),{}};
            for(const auto& value:saved->nodes)if(value.alive&&value.instance==rid){
                const auto id=value.authored.attribute("ID");if(!id.empty())out.controls.emplace_back(id,Control(saved,value.id));
            }
            saved->source->require_sources_current();saved->layout_origin(true);
            saved->external_current(saved->nodes);saved->layout_origin(true);
            operation.require_operation(serial);saved->layout_origin(true);
            require(&saved->source->current_operation_loan()==&operation,"Layout dynamic parser SAME Source replaced");
            return out;
        }catch(...){saved->failed.store(true);throw;}
    }
    core_->current();const auto pid=core_->checked(parent);const auto found=core_->templates.find(name);
    require(found!=core_->templates.end(),"WorldView actual instance template unavailable");
    auto order=core_->next_attached_order;
    auto result=core_->mutate([&](auto& nodes){
        const auto parent_xml=Core::node(nodes,pid).full_xml_occurrence;
        std::set<std::string> ids;const auto rid=Core::append(nodes,found->second,pid,0,true,ids,order,core_->source,parent_xml,0);
        Instance result{Control(core_,rid),{}};
        for(const auto& value:nodes)if(value.alive&&value.instance==rid){
            const auto id=value.authored.attribute("ID");if(!id.empty())result.controls.emplace_back(id,Control(core_,value.id));
        }
        return result;
    });
    core_->next_attached_order=order;
    ++core_->topology_revision;
    return result;
}
BnwWorldViewControlTree::Control BnwWorldViewControlTree::get_button(const Control& pulldown)const {
    core_->current();const auto pid=core_->checked(pulldown);const auto& parent=Core::node(core_->nodes,pid);
    require(parent.authored.name=="PullDown","WorldView GetButton target is not actual PullDown");
    std::uint64_t wrapper=0,button=0;
    for(const auto& n:core_->nodes)if(n.alive&&n.parent==pid&&n.authored.name=="ButtonData"){
        require(!wrapper,"WorldView PullDown ButtonData ambiguous");wrapper=n.id;}
    require(wrapper,"WorldView PullDown ButtonData unavailable");
    for(const auto& n:core_->nodes)if(n.alive&&n.parent==wrapper&&n.authored.name=="Button"){
        require(!button,"WorldView PullDown Button ambiguous");button=n.id;}
    require(button,"WorldView PullDown native button source unavailable");return Control(core_,button);
}
void BnwWorldViewControlTree::set_alpha(const Control& control,double alpha){
    core_->current();const auto id=core_->checked(control);
    require(std::isfinite(alpha)&&alpha>=0,"WorldView SetAlpha argument unsupported");
    core_->mutate([&](auto& nodes){nodes[id-1].alpha=alpha;return 0;});
}
void BnwWorldViewControlTree::set_hidden(const Control& control,bool value){
    const auto saved=core_;saved->layout_origin();
    if(saved->layout_styles){
        const auto actual=control;saved->layout_current_owned();saved->layout_origin();const auto id=saved->checked(actual);
        saved->mutate_layout_baseline_owned([&](auto& nodes){
            auto& node=nodes[id-1];require(node.layout_baseline.has_value(),"Layout hide target lacks earned parser state");
            node.layout_baseline->hide_owned(value);node.hidden=value;node.native_update_exclusion=(node.native_update_exclusion&~2u)|(value?2u:0u);node.layout_hidden_command=value;return 0;
        });return;
    }
    core_->current();const auto id=core_->checked(control);
    core_->mutate([&](auto& nodes){nodes[id-1].hidden=value;nodes[id-1].native_update_exclusion=(nodes[id-1].native_update_exclusion&~2u)|(value?2u:0u);nodes[id-1].layout_hidden_command=value;return 0;});
}
void BnwWorldViewControlTree::require_mouse_consume_backend_origin_without_callbacks(
 const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=core_;const auto actual=control;
 try{
  saved->layout_origin();const auto& entered=saved->source->current_operation_loan();saved->layout_origin();
  require(&entered==&operation&&entered.operation_serial()!=0,"ConsumeMouseOver SAME entered source changed");
  entered.require_render_operation_owned();saved->layout_origin();
  require(saved->context=="UnitFlagManager","ConsumeMouseOver actual UnitFlag context differs");
  (void)Core::node(saved->nodes,saved->checked(actual));saved->layout_origin();
 }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::set_consume_mouse_over_owned(const Control& control,bool value,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 const auto saved=core_;const auto actual=control;
 try{
  saved->layout_origin();require_mouse_consume_backend_origin_without_callbacks(actual,operation);
  const auto serial=operation.operation_serial();saved->layout_origin();const auto id=saved->checked(actual);
  const auto post=[&]{
   saved->layout_origin(true);const auto& entered=saved->source->current_operation_loan();saved->layout_origin(true);
   require(&entered==&operation&&entered.operation_serial()==serial,"ConsumeMouseOver entered source occurrence changed");
   entered.require_render_operation_owned();saved->layout_origin(true);
   (void)Core::node(saved->nodes,saved->checked(actual));saved->layout_origin(true);
  };
  saved->mutate_scroll_owned([&](auto& next){
   (void)Core::node(next,id);auto& n=next.at(id-1);
   if(n.layout_baseline){
    auto& state=*n.layout_baseline;
    state.record_owned(detail::NotificationLayoutBaseline::Mutation::consume_mouse_over);
    state.base_.flags_40=(state.base_.flags_40&~0x800u)|(value?0x800u:0u);
   }
   n.mouse_consume_command=Core::Node::MouseConsumeCommand{value?0x800u:0u,serial,saved->effect_revision+1};
  },post);
  saved->layout_origin();require_mouse_consume_backend_origin_without_callbacks(actual,operation);
 }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::require_unitflag_health_control_without_callbacks(
 const Control& control,std::uint64_t expected_id,std::uint64_t expected_instance_root,
 const runtime::BnwSelectedWorldViewContextOwner& metadata)const {
 const auto saved=core_;
 if(saved->creator!=std::this_thread::get_id())throw runtime::ScriptError("UnitFlag physical member foreign creator");
 require(saved->facade_alive.load()&&!saved->failed.load(),"UnitFlag physical Tree retired/failed before borrow");
 try{
  require(expected_id!=0&&expected_instance_root!=0,"UnitFlag original returned member identity absent");
  const auto owner=control.owner_.lock();
  require(owner&&owner.get()==saved.get()&&!owner.owner_before(saved)&&!saved.owner_before(owner)&&
      control.id_==expected_id,"UnitFlag returned Control canonical Core/id substituted");
  const auto canonical=metadata.canonical_owner_without_callbacks();
  require(saved->source.get()==&metadata&&!saved->source.owner_before(canonical)&&!canonical.owner_before(saved->source),
      "UnitFlag physical member metadata allocation/control substituted");
  const auto& node=Core::node(saved->nodes,expected_id);
  const auto& root=Core::node(saved->nodes,expected_instance_root);
  require(node.dynamic&&node.instance==expected_instance_root&&root.dynamic&&root.instance==expected_instance_root,
      "UnitFlag physical member is not in actual original dynamic GraphInstance root");
  require(saved->facade_alive.load()&&!saved->failed.load(),"UnitFlag physical Tree retired during pure member check");
 }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::Control::require_same_unitflag_health_control_without_callbacks(const Control& expected)const {
 const auto a=owner_.lock(),b=expected.owner_.lock();
 require(a&&b,"UnitFlag health Control owner retired");
 if(a->creator!=std::this_thread::get_id()||b->creator!=std::this_thread::get_id())
  throw runtime::ScriptError("UnitFlag health Control foreign creator");
 require(a->facade_alive.load()&&b->facade_alive.load()&&!a->failed.load()&&!b->failed.load(),
  "UnitFlag health canonical Tree retired or failed");
 require(a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a)&&id_==expected.id_,
  "UnitFlag health Control get/id/control differs from original returned member");
 (void)Core::node(a->nodes,id_);
}
void BnwWorldViewControlTree::require_unitflag_health_backend_origin_without_callbacks(
 const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation,
 runtime::BnwUnitFlagHealthCommand command)const {
 const auto saved=core_;const auto actual=control;
 try{
  if(saved->creator!=std::this_thread::get_id())throw runtime::ScriptError("UnitFlag health Tree foreign creator");
  require(saved->facade_alive.load()&&!saved->failed.load(),"UnitFlag health Tree retired or failed");
  const auto& entered=saved->source->current_operation_loan();
  require(saved->facade_alive.load()&&!saved->failed.load(),"UnitFlag health Tree retired during origin");
  require(&entered==&operation,"UnitFlag health active Source replaced");
  entered.require_unitflag_health_control_owned(actual,command);
  require(saved->facade_alive.load()&&!saved->failed.load(),"UnitFlag health Tree retired during membership check");
  (void)Core::node(saved->nodes,saved->checked(actual));
 }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::mutate_unitflag_health_from_storage_owned(
 const std::shared_ptr<Core>& saved,const Control& actual,
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 runtime::BnwUnitFlagHealthCommand command,std::array<double,4> value){
 using Command=runtime::BnwUnitFlagHealthCommand;
 const auto first=[&](bool active){
  if(saved->creator!=std::this_thread::get_id()){
   saved->failed.store(true);throw runtime::ScriptError("UnitFlag health mutation foreign creator");
  }
  require(saved->facade_alive.load()&&!saved->failed.load()&&saved->busy==active,
   "UnitFlag health Tree retired/failed/reentered before borrowed access");
 };
 try{
  first(false);saved->busy=true;struct Exit{bool&v;~Exit(){v=false;}}exit{saved->busy};
  first(true);const auto& entered=saved->source->current_operation_loan();first(true);
  require(&entered==&operation,"UnitFlag health mutation Source substituted");
  const auto serial=entered.operation_serial();first(true);
  const auto pure=[&]{
   first(true);const auto& current=saved->source->current_operation_loan();first(true);
   require(&current==&operation,"UnitFlag health mutation Source replaced");
   require(current.operation_serial()==serial,"UnitFlag health Source serial replaced");first(true);
   current.require_unitflag_health_control_owned(actual,command);first(true);
   (void)Core::node(saved->nodes,saved->checked(actual));first(true);
  };
  const auto full=[&]{pure();operation.require_operation(serial);first(true);pure();};
  full();const auto id=saved->checked(actual);
  require(saved->effect_revision<std::numeric_limits<std::uint64_t>::max(),"UnitFlag health effect revision exhausted");
  auto next=saved->nodes;auto& node=next.at(id-1);
  switch(command){
   case Command::hide:
    require(value[0]==0||value[0]==1,"UnitFlag health hide value malformed");
    if(node.layout_baseline)node.layout_baseline->hide_owned(value[0]!=0);
    node.hidden=value[0]!=0;node.native_update_exclusion=(node.native_update_exclusion&~2u)|(node.hidden?2u:0u);
    node.layout_hidden_command=node.hidden;break;
   case Command::fg_color:
    require(std::all_of(value.begin(),value.end(),[](double x){return std::isfinite(x)&&x>=0&&x<=1;}),
     "UnitFlag health foreground colour unsupported");node.fg_color=value;break;
   case Command::percent:
    require(std::isfinite(value[0])&&value[0]>=0&&value[0]<=1,"UnitFlag health percent unsupported");
    node.percent=value[0];break;
   case Command::offset_x:{
    require(std::isfinite(value[0]),"UnitFlag health offset X not finite");
    auto offset=node.offset;
    if(node.layout_baseline)offset=std::array<double,2>{value[0],static_cast<double>(node.layout_baseline->base_.margin_70_74[1])};
    if(!offset){
     const auto authored=node.authored.attribute("Offset");const auto comma=authored.find(',');
     require(comma!=authored.npos&&authored.find(',',comma+1)==authored.npos,
      "UnitFlag health offset lacks genuine retained Y supplier");
     const auto parse=[](std::string_view s){
      while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.remove_prefix(1);
      while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.remove_suffix(1);
      double x=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),x);
      require(!s.empty()&&r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(x),
       "UnitFlag health retained offset component unsupported");return x;
     };
     offset=std::array<double,2>{parse(authored.substr(0,comma)),parse(authored.substr(comma+1))};
    }
    (*offset)[0]=value[0];if(node.layout_baseline)node.layout_baseline->offset_prefix_owned(*offset);
    node.offset=*offset;break;
   }
   default:throw runtime::ScriptError("UnitFlag health command unsupported");
  }
  // No renderer, external function target or native/GUI call occurs in stores.
  full();first(true);require(!saved->initial_lua_mutation_journal,"InitialLua cannot borrow post-new health writer");saved->nodes.swap(next);++saved->effect_revision;
  full();first(true);
 }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::set_unitflag_health_hidden_owned(const Control& control,bool value,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 const auto saved=core_;const auto actual=control;
 mutate_unitflag_health_from_storage_owned(saved,actual,operation,runtime::BnwUnitFlagHealthCommand::hide,{value?1.0:0.0,0,0,0});
}
void BnwWorldViewControlTree::set_unitflag_health_fg_color_owned(const Control& control,std::array<double,4> value,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 const auto saved=core_;const auto actual=control;
 mutate_unitflag_health_from_storage_owned(saved,actual,operation,runtime::BnwUnitFlagHealthCommand::fg_color,value);
}
void BnwWorldViewControlTree::set_unitflag_health_percent_owned(const Control& control,double value,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 const auto saved=core_;const auto actual=control;
 mutate_unitflag_health_from_storage_owned(saved,actual,operation,runtime::BnwUnitFlagHealthCommand::percent,{value,0,0,0});
}
void BnwWorldViewControlTree::set_unitflag_health_offset_x_owned(const Control& control,double value,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 const auto saved=core_;const auto actual=control;
 mutate_unitflag_health_from_storage_owned(saved,actual,operation,runtime::BnwUnitFlagHealthCommand::offset_x,{value,0,0,0});
}
void BnwWorldViewControlTree::set_disabled(const Control& control,bool value){
    core_->current();const auto id=core_->checked(control);
    const auto& kind=Core::node(core_->nodes,id).authored.name;
    require(kind=="Button"||kind=="GridButton","WorldView SetDisabled requires authored button");
    core_->mutate([&](auto& nodes){nodes[id-1].disabled=value;return 0;});
}
void BnwWorldViewControlTree::set_text(const Control& control,std::string value){
    const auto saved=core_;saved->layout_origin();
    if(saved->layout_styles){
        //Button enters its genuine live setter BEFORE callback-capable layout
        //currentness. The setter withdraws old Run at its pure actual entry.
        const auto actual=control;saved->layout_origin();const auto id=saved->checked(actual);
        if(Core::node(saved->nodes,id).authored.name=="Button"){set_embedded_label_text_live_owned(actual,std::move(value));return;}
        saved->layout_current_owned();saved->layout_origin();
        saved->failed.store(true);
        throw std::runtime_error("Standalone Label SetText requires genuine owned Label object/reinit issuer; flat authored metadata is insufficient");
        require(Core::node(saved->nodes,id).authored.name=="Label","Layout text target is not actual Label");
        saved->mutate_layout_baseline_owned([&](auto& nodes){
            auto& target=nodes[id-1];require(target.layout_baseline.has_value(),"Layout text target lacks earned parser state");
            target.layout_baseline->text_prefix_owned(value);target.text=std::move(value);return 0;
        });return;
    }
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="Label"&&value.find('\0')==value.npos,
            "WorldView SetText requires authored Label and supported text");
    core_->mutate([&](auto& nodes){nodes[id-1].text=std::move(value);return 0;});
}
void BnwWorldViewControlTree::set_tooltip(const Control& control,std::string value){
    core_->current();const auto id=core_->checked(control);
    require(value.find('\0')==value.npos,"WorldView tooltip contains unsupported NUL");
    const auto& target=Core::node(core_->nodes,id);
    require(!(target.layout_baseline&&
        ((target.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button&&target.layout_baseline->actual_type_=="Button")||
         (target.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::image&&target.layout_baseline->actual_type_=="Image"))),
        "Canonical Button/Image dynamic ToolTip requires genuine54/flags4000/GUI524CC0 mutation issuer");
    core_->mutate([&](auto& nodes){nodes[id-1].tooltip=std::move(value);return 0;});
}
void BnwWorldViewControlTree::set_world_position(const Control& control,std::array<double,3> value){
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="WorldAnchor","WorldView world position target is not actual WorldAnchor");
    require(std::all_of(value.begin(),value.end(),[](double n){return std::isfinite(n);}),"WorldView world position is not finite");
    core_->mutate([&](auto& nodes){nodes[id-1].world_position=value;return 0;});
}
void BnwWorldViewControlTree::set_offset(const Control& control,std::array<double,2> value){
    const auto saved=core_;saved->layout_origin();
    if(saved->layout_styles){
        const auto actual=control;saved->layout_current_owned();saved->layout_origin();const auto id=saved->checked(actual);
        saved->mutate_layout_baseline_owned([&](auto& nodes){
            auto& node=nodes[id-1];require(node.layout_baseline.has_value(),"Layout offset target lacks earned parser state");
            node.layout_baseline->offset_prefix_owned(value);node.offset=value;return 0;
        });return;
    }
    core_->current();const auto id=core_->checked(control);
    require(std::all_of(value.begin(),value.end(),[](double n){return std::isfinite(n);}),"WorldView control offset is not finite");
    core_->mutate([&](auto& nodes){nodes[id-1].offset=value;return 0;});
}
void BnwWorldViewControlTree::set_offset_x(const Control& control,double value){
    const auto saved=core_;saved->layout_origin();
    if(saved->layout_styles){
        const auto actual=control;saved->layout_current_owned();saved->layout_origin();const auto id=saved->checked(actual);
        const auto& node=Core::node(saved->nodes,id);require(node.layout_baseline.has_value(),"Layout offsetX target lacks earned parser state");
        const auto y=node.layout_baseline->base_.margin_70_74[1];
        saved->mutate_layout_baseline_owned([&](auto& nodes){
            auto& target=nodes[id-1];const std::array<double,2> next{value,static_cast<double>(y)};
            target.layout_baseline->offset_prefix_owned(next);target.offset=next;return 0;
        });return;
    }
    core_->current();const auto id=core_->checked(control);const auto& node=Core::node(core_->nodes,id);
    require(std::isfinite(value),"WorldView offset X is not finite");
    auto offset=node.offset;
    if(!offset){
        const auto authored=node.authored.attribute("Offset");const auto comma=authored.find(',');
        require(comma!=authored.npos&&authored.find(',',comma+1)==authored.npos,"WorldView offset Y has no exact authored/source value");
        const auto parse=[](std::string_view s){
            while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.remove_prefix(1);
            while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.remove_suffix(1);
            double n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
            require(!s.empty()&&r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(n),"WorldView authored offset component unsupported");return n;
        };
        offset=std::array<double,2>{parse(authored.substr(0,comma)),parse(authored.substr(comma+1))};
    }
    (*offset)[0]=value;core_->mutate([&](auto& nodes){nodes[id-1].offset=*offset;return 0;});
}
void BnwWorldViewControlTree::set_color(const Control& control,std::array<double,4> value){
    core_->current();const auto id=core_->checked(control);
    require(std::all_of(value.begin(),value.end(),[](double n){return std::isfinite(n)&&n>=0&&n<=1;}),"WorldView actual colour components outside normalized finite domain");
    core_->mutate([&](auto& nodes){
        auto& target=nodes[id-1];
        if(target.layout_baseline&&target.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::image){
            require(static_cast<bool>(target.layout_baseline->image_),"Image SetColor actual embedded constructor absent");
            // The generic mutation initially copies shared subtype pointers.
            // Deep-clone ONLY this actual Image before changing its fields;
            // failed POST must not publish into the old canonical Image.
            auto next=detail::NotificationLayoutBaseline::clone_owned(*target.layout_baseline);
            for(std::size_t i=0;i<4;++i)next.image_->color_98_a4[i]=static_cast<float>(value[i]);
            target.layout_baseline.emplace(std::move(next));
        }
        if(target.layout_baseline&&target.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button){
            require(static_cast<bool>(target.layout_baseline->button_),"Button SetColor actual embedded constructor absent");
            auto next=detail::NotificationLayoutBaseline::clone_owned(*target.layout_baseline);
            for(std::size_t i=0;i<4;++i)next.button_->color_12c_138[i]=static_cast<float>(value[i]);
            target.layout_baseline.emplace(std::move(next));
        }
        // Existing noncanonical/data API and other non-Image behavior retained.
        // No extra bounds/dirty callback: original6CBAF0 only stores4floats.
        target.color=value;return 0;
    });
}
void BnwWorldViewControlTree::set_fg_color(const Control& control,std::array<double,4> value){
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="Bar","WorldView foreground colour requires actual Bar");
    require(std::all_of(value.begin(),value.end(),[](double n){return std::isfinite(n)&&n>=0&&n<=1;}),"WorldView actual foreground colour outside normalized finite domain");
    core_->mutate([&](auto& nodes){nodes[id-1].fg_color=value;return 0;});
}
void BnwWorldViewControlTree::set_texture(const Control& control,std::string value){
    core_->current();const auto id=core_->checked(control);const auto kind=Core::node(core_->nodes,id).authored.name;
    require(kind=="Image"||kind=="AlphaAnim"||kind=="ScrollAnim","WorldView texture target is not an actual textured control");
    require(!value.empty()&&value.find('\0')==value.npos,"WorldView actual texture path absent/unsupported");
    core_->mutate([&](auto& nodes){nodes[id-1].texture=std::move(value);return 0;});
}
void BnwWorldViewControlTree::set_texture_offset(const Control& control,std::array<double,2> value){
    core_->current();const auto id=core_->checked(control);const auto kind=Core::node(core_->nodes,id).authored.name;
    require(kind=="Image"||kind=="AlphaAnim"||kind=="ScrollAnim","WorldView texture offset target differs");
    require(std::all_of(value.begin(),value.end(),[](double n){return std::isfinite(n)&&n>=0;}),"WorldView actual texture offset invalid");
    core_->mutate([&](auto& nodes){nodes[id-1].texture_offset=value;return 0;});
}
void BnwWorldViewControlTree::set_percent(const Control& control,double value){
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="Bar"&&std::isfinite(value)&&value>=0&&value<=1,"WorldView Bar percentage invalid");
    core_->mutate([&](auto& nodes){nodes[id-1].percent=value;return 0;});
}
void BnwWorldViewControlTree::animation_set_to_beginning(const Control& control){
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="AlphaAnim","WorldView reset target is not actual AlphaAnim");
    core_->mutate([&](auto& nodes){auto& n=nodes[id-1];auto command=n.animation.value_or(AnimationCommands{});
        require(command.sequence<std::numeric_limits<std::uint64_t>::max(),"WorldView animation command exhausted");
        command.reset_sequence=++command.sequence;command.reset_at=std::chrono::steady_clock::now();n.animation=command;return 0;});
}
void BnwWorldViewControlTree::animation_play(const Control& control){
    core_->current();const auto id=core_->checked(control);
    require(Core::node(core_->nodes,id).authored.name=="AlphaAnim","WorldView Play target is not actual AlphaAnim");
    core_->mutate([&](auto& nodes){auto& n=nodes[id-1];auto command=n.animation.value_or(AnimationCommands{});
        require(command.sequence<std::numeric_limits<std::uint64_t>::max(),"WorldView animation command exhausted");
        // Native Play clears stopped without resetting progress. Preserve the
        // existing own-clock origin until an actual SetToBeginning command.
        const bool playing=command.play_at&&command.play_sequence>command.reset_sequence;
        command.play_sequence=++command.sequence;
        if(!playing)command.play_at=std::chrono::steady_clock::now();
        n.animation=command;return 0;});
}
BnwWorldViewControlTree::ScrollClockLoan::ScrollClockLoan(const Control& control,
        const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwScrollAnimationUpdateLoan& update)
    :tree_(control.owner_),control_id_(control.id_),source_serial_(operation.operation_serial()),
     tick_sequence_(update.tick_sequence_without_callbacks()),operation_(&operation),
     delta_seconds_(update.delta_seconds_without_callbacks()),update_(&update){
    const auto saved=tree_.lock();require(bool(saved),"Scroll clock actual Tree retired");
    update.require_current_without_callbacks();saved->scroll_origin();
    require_for_core_without_callbacks(saved,control_id_);
}
BnwWorldViewControlTree::ScrollClockLoan::~ScrollClockLoan()noexcept {alive_=false;}
void BnwWorldViewControlTree::ScrollClockLoan::require_for_core_without_callbacks(
        const std::shared_ptr<Core>& saved,std::uint64_t id)const {
    require(creator_==std::this_thread::get_id()&&alive_,"Scroll clock loan retired/foreign");
    require(update_!=nullptr,"Scroll actual active update loan absent");
    update_->require_current_without_callbacks();saved->scroll_origin(saved->busy);
    {const auto actual=tree_.lock();require(actual&&actual.get()==saved.get()&&
        !actual.owner_before(saved)&&!saved.owner_before(actual)&&control_id_==id,
        "Scroll clock actual Tree/control substituted");}
    require(saved->selected_scroll_eligible_without_callbacks(id),"Scroll selected target excluded before phase store");
    require(operation_&&source_serial_&&tick_sequence_&&std::isfinite(delta_seconds_)&&delta_seconds_>=0,
        "Scroll clock actual tick/source missing");
    const auto& active=saved->source->current_operation_loan();saved->scroll_origin(saved->busy);
    require(&active==operation_&&active.operation_serial()==source_serial_,"Scroll clock SAME operation changed");
    saved->scroll_origin(saved->busy);
}
void BnwWorldViewControlTree::advance_selected_scroll_update_owned(
        const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwScrollAnimationUpdateLoan& update){
    const auto saved=core_;
    try {
        update.require_current_without_callbacks();saved->scroll_origin();
        saved->scroll_current_owned();update.require_current_without_callbacks();saved->scroll_origin();
        const auto& actual=saved->source->current_operation_loan();
        update.require_current_without_callbacks();saved->scroll_origin();
        require(&actual==&operation,"Scroll issuer SAME source operation substituted");
        // Admission is source-specific. Unsupported flags/list commands are
        // never silently treated as default zero or a caller eligibility bool.
        std::vector<Control> eligible;
        for(const auto& n:saved->nodes)if(n.alive&&n.authored.name=="ScrollAnim"){
            if(saved->selected_scroll_eligible_without_callbacks(n.id))eligible.push_back(Control(saved,n.id));
        }
        for(const auto& control:eligible){
            update.require_current_without_callbacks();saved->scroll_origin();
            const ScrollClockLoan clock(control,operation,update);
            advance_scroll_animation_owned(control,clock);
            update.require_current_without_callbacks();saved->scroll_origin();
        }
    }catch(...){saved->failed=true;throw;}
}
void BnwWorldViewControlTree::branch_reset_animation_owned(const Control& root){
    const auto saved=core_;
    try {
        saved->scroll_origin();saved->scroll_current_owned();saved->scroll_origin();
        const auto id=saved->checked(root);
        saved->mutate_scroll_owned([&](auto& nodes){
            // Own canonical subtree walk, not an assertion about native flags
            // 0x2002/4B9E80 eligibility. Parent dispatch must close that gate.
            std::vector<std::vector<std::uint64_t>> children(nodes.size()+1);
            for(const auto& n:nodes)if(n.alive){
                require(n.parent<=nodes.size(),"BranchReset canonical parent invalid");
                children[n.parent].push_back(n.id);
            }
            std::vector<std::uint64_t> pending{id};std::set<std::uint64_t> seen;
            const auto own_alpha_now=std::chrono::steady_clock::now();
            while(!pending.empty()){
                const auto current=pending.back();pending.pop_back();
                require(seen.insert(current).second,"BranchReset canonical subtree cyclic");
                auto& n=nodes[current-1];require(n.alive,"BranchReset child retired");
                if(n.authored.name=="ScrollAnim"){
                    require(n.scroll_animation.has_value(),"BranchReset Scroll state absent");
                    Core::reset_scroll(*n.scroll_animation);
                }else if(n.authored.name=="AlphaAnim"){
                    // Preserve the existing OWN Alpha Reset/Play representation.
                    // This timestamp is not a Scroll/native ctor clock grant.
                    auto command=n.animation.value_or(AnimationCommands{});
                    require(command.sequence<std::numeric_limits<std::uint64_t>::max()-1,"BranchReset Alpha sequence exhausted");
                    command.reset_sequence=++command.sequence;command.reset_at=own_alpha_now;
                    command.play_sequence=++command.sequence;command.play_at=own_alpha_now;n.animation=command;
                }else require(!n.authored.name.ends_with("Anim"),"BranchReset animation kind unsupported");
                for(auto it=children[current].rbegin();it!=children[current].rend();++it)pending.push_back(*it);
            }
        },[]{});
        saved->scroll_origin();
    }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::advance_scroll_animation_owned(const Control& target,const ScrollClockLoan& clock){
    const auto saved=core_;
    try {
        saved->scroll_origin();const auto id=saved->checked(target);
        clock.require_for_core_without_callbacks(saved,id);
        saved->mutate_scroll_owned([&](auto& nodes){
            clock.require_for_core_without_callbacks(saved,id);
            auto& n=nodes[id-1];require(n.authored.name=="ScrollAnim"&&n.scroll_animation.has_value(),"Scroll tick target differs");
            Core::advance_scroll(*n.scroll_animation,clock.delta_seconds_,clock.tick_sequence_);
        },[&]{clock.require_for_core_without_callbacks(saved,id);});
        // Mutation has authentic source POST; clock origin is checked again
        // before this tick can become a current ControlQuote sample.
        saved->scroll_origin();clock.require_for_core_without_callbacks(saved,id);saved->scroll_origin();
    }catch(...){saved->failed.store(true);throw;}
}

void BnwWorldViewControlTree::change_parent_selected_container_owned(const Control& child,
        const std::shared_ptr<BnwWorldViewControlTree>& target,const Control& parent){
    core_->current();core_->initial_lua_require_existing_topology_owned();require(target&&target.get()!=this,"WorldView actual external parent owner absent");target->core_->current();
    const auto id=core_->checked(child),pid=target->core_->checked(parent);
    require(core_->context=="UnitFlagManager"&&target->core_->context=="InGame"&&
        core_->source.get()==target->core_->source.get()&&!core_->source.owner_before(target->core_->source)&&
        !target->core_->source.owner_before(core_->source),"WorldView cross-parent actual source context differs");
    const auto& parent_node=Core::node(target->core_->nodes,pid);
    require(Core::node(core_->nodes,id).dynamic&&parent_node.authored.name=="Container"&&
        parent_node.authored.attribute("ID")=="SelectedUnitContainer"&&!parent_node.dynamic,
        "WorldView cross-parent target is outside genuine selected container");
    require(core_->next_attached_order<std::numeric_limits<std::uint64_t>::max(),"WorldView cross attachment exhausted");
    const auto order=core_->next_attached_order+1;
    target->core_->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}} exit{target->core_->busy};
    try {
        core_->mutate([&](auto& nodes){auto& n=nodes[id-1];n.parent=0;n.external_parent_tree=target;n.external_parent_id=pid;n.attached_order=order;return 0;});
        require(!target->core_->failed.load(),"WorldView external parent poisoned during cross commit");
        core_->next_attached_order=order;++core_->topology_revision;
    }catch(...){core_->failed.store(true);target->core_->failed.store(true);throw;}
}
void BnwWorldViewControlTree::change_parent(const Control& child,const Control& parent){
    core_->current();core_->initial_lua_require_existing_topology_owned();const auto id=core_->checked(child),pid=core_->checked(parent);
    require(Core::node(core_->nodes,id).dynamic,"WorldView ChangeParent requires dynamic actual instance");
    for(auto p=pid;p;p=Core::node(core_->nodes,p).parent)require(p!=id,"WorldView ChangeParent cycle");
    require(core_->next_attached_order<std::numeric_limits<std::uint64_t>::max(),"WorldView attachment order exhausted");
    const auto order=core_->next_attached_order+1;
    core_->mutate([&](auto& nodes){nodes[id-1].parent=pid;nodes[id-1].external_parent_tree.reset();nodes[id-1].external_parent_id=0;nodes[id-1].attached_order=order;return 0;});
    core_->next_attached_order=order;
    ++core_->topology_revision;
}
void BnwWorldViewControlTree::destroy_child(const Control& parent,const Control& child){
    core_->current();core_->initial_lua_require_existing_topology_owned();const auto pid=core_->checked(parent),id=core_->checked(child);
    require(Core::node(core_->nodes,id).dynamic&&Core::node(core_->nodes,id).parent==pid,"WorldView DestroyChild parent differs");
    core_->mutate([&](auto& nodes){
        std::set<std::uint64_t> removed{id};bool changed=true;
        while(changed){changed=false;for(const auto& n:nodes)if(n.alive&&removed.contains(n.parent)&&removed.insert(n.id).second)changed=true;}
        for(auto value:removed)nodes[value-1].alive=false;return 0;
    });
    ++core_->topology_revision;
}
double BnwWorldViewControlTree::inherited_alpha(const Control& control)const {
    core_->current();return core_->alpha(core_->checked(control));
}
bool BnwWorldViewControlTree::inherited_hidden(const Control& control)const {
    core_->current();return core_->hidden(core_->checked(control));
}
std::uint64_t BnwWorldViewControlTree::render_revision_owned()const {core_->layout_origin();core_->current();return core_->effect_revision;}
content::XmlNode BnwWorldViewControlTree::render_context_owned()const {core_->layout_origin();
    core_->current();auto result=core_->root_authored;core_->current();return result;
}
void BnwWorldViewControlTree::require_render_revision_without_callbacks(std::uint64_t expected,
        const runtime::BnwWorldViewSourceOperationLoan& expected_loan,std::uint64_t expected_serial)const {
    // Publication.paint holds Core.busy itself. Only the creator, failure,
    // actual canonical operation and revision predicates are needed here.
    if(core_->creator!=std::this_thread::get_id()){
        core_->failed.store(true);throw runtime::ScriptError("WorldView render fence foreign creator");
    }
    try {
        require(!core_->failed.load()&&core_->effect_revision==expected,"WorldView render effects changed before provider/use");
        core_->external_current(core_->nodes);
        const auto& actual=core_->source->current_operation_loan();
        actual.require_render_operation_owned();
        require(&actual==&expected_loan,"WorldView render active source loan substituted");
        // operation_serial() uses genuine private origin checks only. Public
        // require_operation() has real callbacks and MUST NOT be used here.
        require(actual.operation_serial()==expected_serial,"WorldView render source serial substituted");
        core_->external_current(core_->nodes);
        const auto& after=core_->source->current_operation_loan();
        require(&after==&actual&&after.operation_serial()==expected_serial&&
            !core_->failed.load()&&core_->effect_revision==expected,"WorldView render canonical origin/effects changed");
    }catch(...){core_->failed.store(true);throw;}
}
std::vector<BnwWorldViewControlTree::Control> BnwWorldViewControlTree::render_controls_owned()const {core_->layout_origin();
    core_->current();const auto revision=core_->effect_revision;
    std::vector<std::vector<std::uint64_t>> children(core_->nodes.size()+1);
    for(const auto& node:core_->nodes)if(node.alive){
        require(!node.parent||(node.parent<=core_->nodes.size()&&core_->nodes[node.parent-1].alive),"WorldView render parent retired");
        children[node.parent].push_back(node.id);
    }
    for(auto& group:children)std::sort(group.begin(),group.end(),[&](auto a,auto b){return core_->nodes[a-1].attached_order<core_->nodes[b-1].attached_order;});
    std::vector<std::uint64_t> pending;std::vector<Control> result;std::vector<bool> seen(core_->nodes.size()+1);
    for(auto i=children[0].rbegin();i!=children[0].rend();++i)pending.push_back(*i);
    while(!pending.empty()){
        const auto id=pending.back();pending.pop_back();require(!seen[id],"WorldView render topology repeated/cyclic");seen[id]=true;
        result.push_back(Control(core_,id));for(auto i=children[id].rbegin();i!=children[id].rend();++i)pending.push_back(*i);
    }
    for(const auto& node:core_->nodes)require(!node.alive||seen[node.id],"WorldView live node unreachable from canonical roots");
    core_->current();require(revision==core_->effect_revision,"WorldView controls changed during render traversal");return result;
}
BnwWorldViewControlRenderPublication::BnwWorldViewControlRenderPublication(
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> life,std::shared_ptr<BnwWorldViewControlTree> tree,
    BnwWorldViewControlTree::Control control,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame,std::vector<std::size_t> indices,
    std::shared_ptr<const detail::BnwNormalLabelPaintEmission> emission):
    renderer_(&renderer),renderer_life_(std::move(life)),tree_(std::move(tree)),control_(std::move(control)),
    gui_(gui),window_(window),frame_(emission?emission->frame_origin_for_publication_owned():std::move(frame)),
    paint_indices_(std::move(indices)),normal_label_emission_(std::move(emission)),
    topology_revision_(tree_?(tree_->require_current(),tree_->core_->topology_revision):0),
    effect_revision_(tree_?tree_->core_->effect_revision:0){
    require(tree_&&!renderer_life_.expired()&&control_&&!root_context_entry_,
        "WorldView actual renderer publication requires its genuine ordinary Control");
    tree_->core_->layout_origin();
    tree_->require_current();gui_.require_current();window_.require_current();
    std::set<std::size_t> seen;
    require(!paint_indices_.empty(),"WorldView control paint span unavailable");
    for(auto index:paint_indices_)require(index<frame_.paints.size()&&seen.insert(index).second,"WorldView actual control paint span invalid");
    (void)tree_->inherited_alpha(*control_);
    if(normal_label_emission_)normal_label_emission_->require_publication_owned(*this,tree_->core_->source->current_operation_loan());
}
std::uint64_t BnwWorldViewControlRenderPublication::actual_checked_control_id_for_paint_without_callbacks()const{
    //Private DATA read surrounded by actual renderer Scope FIRST. It never
    //calls Core.current or creates a Control/Frame/paint/source authority.
    if(enclosing_context_entry_)enclosing_context_entry_->require_current_without_callbacks();
    require(control_&&!root_context_entry_,"Indexed paint requires actual ordinary Control, not Context entry");
    require(creator_==std::this_thread::get_id()&&renderer_,"Paint publication foreign/unbound");
    const auto tree=tree_;const auto life=renderer_life_.lock();
    require(tree&&life,"Paint publication physical Tree/renderer lifetime absent");
    if(enclosing_context_entry_)enclosing_context_entry_->require_same_tree_without_callbacks(*tree);
    const auto owner=tree->core_;owner->layout_origin(true);
    require(owner->topology_revision==topology_revision_&&owner->effect_revision==effect_revision_,
        "Paint publication original checked-control revision changed");
    const auto id=owner->checked(*control_);
    require(id==control_->id_,"Paint publication is not SAME actual checked Control");
    owner->layout_origin(true);
    if(enclosing_context_entry_){enclosing_context_entry_->require_current_without_callbacks();
        enclosing_context_entry_->require_same_tree_without_callbacks(*tree);}
    return id;
}
std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::capture_original_owned(const OriginalOwnerIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> renderer_life,
    std::shared_ptr<BnwWorldViewControlTree> actual_tree,BnwWorldViewControlTree::Control control,
    const BnwOwnedCameraGuiScreen::Loan& gui,const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    ComposedUiFrame frame,std::vector<std::size_t> indices,
    std::shared_ptr<const detail::BnwNormalLabelPaintEmission> emission){
    //This constructs the original Publication BEFORE any Frame issuance. It
    //does not accept an old Publication or caller shared-pointer alias owner.
    const auto tree=std::move(actual_tree);
    require(tree&&!renderer_life.expired(),"Original Publication factory has no actual Tree/renderer");
    const auto core=tree->core_;core->layout_origin(false);
    const auto source=core->source;const auto& entry=source->current_operation_loan();
    entry.require_origin_without_callbacks();const auto serial=entry.operation_serial();
    const auto topology=core->topology_revision,effects=core->effect_revision;
    const auto pure=[&]{
        require(tree->core_==core&&core->source==source&&!renderer_life.expired()&&!core->failed.load(),
            "Original Publication actual owner retired during factory allocation");
        core->layout_origin(false);const auto& fresh=source->current_operation_loan();
        require(&fresh==&entry,"Original Publication fresh Source differs before saved operand");
        fresh.require_origin_without_callbacks();
        require(fresh.operation_serial()==serial&&core->topology_revision==topology&&core->effect_revision==effects,
            "Original Publication factory immutable Source/control revisions changed");
    };
    const auto full=[&]{pure();gui.require_current();pure();window.require_current();pure();
        entry.require_current();pure();};
    try{
        pure();full();pure();
        //The original closed constructor receives OWN frame/index/emission
        //VALUES. No second object/copy constructor or stack-publication alias.
        auto owned=std::shared_ptr<BnwWorldViewControlRenderPublication>(
            new BnwWorldViewControlRenderPublication(renderer,renderer_life,tree,std::move(control),gui,window,
                std::move(frame),std::move(indices),std::move(emission)));
        pure();full();pure();
        require(owned&&owned->creator_==std::this_thread::get_id()&&owned->renderer_==&renderer&&
            owned->tree_==tree&&owned->topology_revision_==topology&&owned->effect_revision_==effects&&
            owned->original_owner_.expired()&&!owned->original_owner_operation_&&!owned->original_owner_source_serial_,
            "Original Publication newly created allocation/control differs");
        //Only the original factory publishes this exact new control. Weak
        //self avoids a cycle and consumers cannot provide an ownership alias.
        owned->original_owner_operation_=&entry;owned->original_owner_source_serial_=serial;
        owned->original_owner_=owned;
        pure();full();pure();
        const auto result=owned->original_owner_before_paint_without_callbacks();
        require(result.get()==owned.get()&&!result.owner_before(owned)&&!owned.owner_before(result),
            "Original Publication canonical control differs at factory return");
        return result;
    }catch(...){core->failed.store(true);throw;}
}
std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::original_owner_without_callbacks()const{
    require_actual_receiver_for_paint_without_callbacks();
    require(creator_==std::this_thread::get_id()&&renderer_&&!renderer_life_.expired()&&tree_,
        "Original Publication canonical owner retired or foreign");
    const auto core=tree_->core_;core->layout_origin(true);
    const auto& fresh=core->source->current_operation_loan();
    require(original_owner_operation_ && &fresh==original_owner_operation_,
        "Original Publication Source replaced before saved operation identity");
    fresh.require_origin_without_callbacks();
    const auto owned=original_owner_.lock();
    require(owned&&owned.get()==this&&original_owner_source_serial_&&
        fresh.operation_serial()==*original_owner_source_serial_&&
        core->topology_revision==topology_revision_&&core->effect_revision==effect_revision_&&!core->failed.load(),
        "Original Publication factory control/Source/revisions retired or changed");
    require_actual_receiver_for_paint_without_callbacks();
    core->layout_origin(true);require(&core->source->current_operation_loan()==original_owner_operation_,
        "Original Publication fresh Source changed before canonical owner return");
    fresh.require_origin_without_callbacks();
    return owned;
}

void BnwWorldViewControlRenderPublication::require_adjusted_frame_owner_without_callbacks(
    const std::shared_ptr<const ComposedUiFrame>& actual)const{
    require_actual_receiver_for_paint_without_callbacks();
    require(creator_==std::this_thread::get_id()&&renderer_&&!renderer_life_.expired()&&tree_,
        "Adjusted Frame original Publication/renderer/Tree retired or foreign");
    const auto core=tree_->core_;core->layout_origin(true);
    const auto& fresh=core->source->current_operation_loan();
    require(adjusted_frame_operation_ && &fresh==adjusted_frame_operation_,
        "Adjusted Frame Source replaced before saved operation identity");
    fresh.require_origin_without_callbacks();
    require(adjusted_frame_source_serial_&&fresh.operation_serial()==*adjusted_frame_source_serial_&&
        actual&&adjusted_frame_owner_&&actual.get()==adjusted_frame_owner_.get()&&
        !actual.owner_before(adjusted_frame_owner_)&&!adjusted_frame_owner_.owner_before(actual)&&
        core->topology_revision==topology_revision_&&core->effect_revision==effect_revision_&&!core->failed.load(),
        "Adjusted Frame SAME original owned backing/Source/revisions changed");
    require_actual_receiver_for_paint_without_callbacks();
    core->layout_origin(true);require(&core->source->current_operation_loan()==adjusted_frame_operation_,
        "Adjusted Frame Source changed before pure owner return");
    fresh.require_origin_without_callbacks();
}
std::shared_ptr<const ComposedUiFrame>
BnwWorldViewControlRenderPublication::actual_adjusted_frame_owner_for_paint_without_callbacks(
    const UiScreenGlRenderer& renderer,const ComposedUiFrame& actual)const{
    require(creator_==std::this_thread::get_id()&&renderer_==&renderer&&adjusted_frame_painting_,
        "Adjusted Frame owner capture is outside its actual original renderer paint");
    const auto owned=adjusted_frame_owner_;
    require_adjusted_frame_owner_without_callbacks(owned);
    require(owned.get()==&actual,"Renderer Frame is not the SAME original owned adjusted object");
    require_adjusted_frame_owner_without_callbacks(owned);
    return owned; //Copy SAME immutable VALUE owner; never create another frame.
}

UiGlPaintReport BnwWorldViewControlRenderPublication::paint(UiScreenGlRenderer& renderer)const {
    if(context_child_body_)return paint_context_child_body_owned(renderer);
    if(normal_caption_emission_)return paint_context_caption_owned(renderer);
    if(root_context_entry_)return paint_context_root_owned(renderer);
    const auto canonical_owner=original_owner_before_paint_without_callbacks();
    require(canonical_owner&&canonical_owner.get()==this,
        "Ordinary paint requires SAME canonical original Publication control");
    require(control_&&!root_context_entry_,"Ordinary paint requires its genuine Control receiver");
    require(creator_==std::this_thread::get_id(),"WorldView renderer loan foreign thread");
    require(renderer_==&renderer,"WorldView publication belongs to another renderer");
    const auto life=renderer_life_.lock();require(static_cast<bool>(life),"WorldView renderer publication expired");
    if(enclosing_context_entry_){
        enclosing_context_entry_->require_current_without_callbacks();
        enclosing_context_entry_->require_same_tree_without_callbacks(*tree_);
        tree_->core_->layout_origin(true);
    }else{tree_->core_->layout_origin(false);tree_->require_current();}
    const auto owner=tree_->core_;auto& core=*owner;
    require(core.topology_revision==topology_revision_&&core.effect_revision==effect_revision_,
        "WorldView control topology/anchor/effects changed after layout publication");
    require(!adjusted_frame_painting_&&!adjusted_frame_owner_&&!adjusted_frame_operation_&&
        !adjusted_frame_source_serial_,"Original adjusted Frame paint replay or reentry");
    const auto source=core.source;const auto& entry=source->current_operation_loan();
    entry.require_origin_without_callbacks();const auto serial=entry.operation_serial();
    const bool owns_busy=!enclosing_context_entry_.has_value();
    if(owns_busy)core.busy=true;
    adjusted_frame_painting_=true;
    struct Exit{
        std::shared_ptr<const BnwWorldViewControlRenderPublication> owner;
        bool& busy;bool& painting;const bool owns_busy;
        ~Exit(){painting=false;if(owns_busy)busy=false;}
    }exit{canonical_owner,core.busy,adjusted_frame_painting_,owns_busy};
    const auto physical=[&,canonical_owner]{
        const auto current_owner=original_owner_without_callbacks();
        require(current_owner.get()==canonical_owner.get()&&!current_owner.owner_before(canonical_owner)&&
            !canonical_owner.owner_before(current_owner),"Ordinary paint original control was substituted");
        require(creator_==std::this_thread::get_id()&&renderer_==&renderer&&!renderer_life_.expired()&&
            adjusted_frame_painting_&&core.busy&&!core.failed.load()&&tree_->core_==owner&&core.source==source,
            "Adjusted Frame actual original paint owner changed during allocation/Source");
        core.layout_origin(true);const auto& fresh=source->current_operation_loan();
        require(&fresh==&entry,"Adjusted Frame fresh Source differs before original saved operand");
        fresh.require_origin_without_callbacks();
        require(fresh.operation_serial()==serial&&core.topology_revision==topology_revision_&&
            core.effect_revision==effect_revision_,"Adjusted Frame immutable paint serial/revisions changed");
        require_actual_receiver_for_paint_without_callbacks();
        if(adjusted_frame_owner_)require_adjusted_frame_owner_without_callbacks(adjusted_frame_owner_);
    };
    const auto full=[&]{physical();gui_.require_current();physical();window_.require_current();physical();
        source->require_sources_current();physical();entry.require_current();physical();};
    try {
        physical();full();physical();
        const auto id=core.checked(*control_);const auto alpha=core.alpha(id);const auto hidden=core.hidden(id);
        const auto text=BnwWorldViewControlTree::Core::node(core.nodes,id).text;
        //Create the ONE original adjusted frame under allocation PRE/POST,
        //before any Frame/Item/Matrix is issued from its physical address.
        auto adjusted=std::make_shared<ComposedUiFrame>(frame_);
        physical();full();physical();
        require(core.checked(*control_)==id&&core.alpha(id)==alpha&&core.hidden(id)==hidden&&
            BnwWorldViewControlTree::Core::node(core.nodes,id).text==text,
            "Adjusted Frame allocating copy POST changed actual control values");
        const auto scale=[&](std::uint32_t rgba){const auto channel=static_cast<double>(rgba&255u);
            return (rgba&0xffffff00u)|static_cast<std::uint32_t>(std::clamp(channel*alpha,0.0,255.0));};
        for(auto index:paint_indices_){
            auto& paint=adjusted->paints[index];
            if(hidden){paint.destination.width=0;paint.destination.height=0;}
            if(text){require(paint.kind==WorldViewPaintKind::text,"WorldView Label paint span kind differs");paint.text=*text;}
            paint.rgba=scale(paint.rgba);
            if(paint.tint)(*paint.tint)[3]=static_cast<float>(std::clamp(static_cast<double>((*paint.tint)[3])*alpha,0.0,1.0));
            if(paint.font_style){
                for(auto& color:paint.font_style->colors)if(color)color=scale(*color);
                if(paint.font_style->float_colors)for(auto& color:*paint.font_style->float_colors)
                    color[3]=static_cast<float>(std::clamp(static_cast<double>(color[3])*alpha,0.0,1.0));
            }
        }
        physical();full();physical();
        require(core.checked(*control_)==id&&core.alpha(id)==alpha&&core.hidden(id)==hidden&&
            BnwWorldViewControlTree::Core::node(core.nodes,id).text==text,
            "Adjusted Frame post-adjustment Source changed actual control values");
        //No mutable owner remains when this actual backing is published. The
        //same address is later handed to the original renderer Frame issuer.
        std::shared_ptr<const ComposedUiFrame> immutable=std::move(adjusted);
        require(immutable&&!adjusted,"Adjusted Frame immutable transfer was not earned");
        adjusted_frame_operation_=&entry;adjusted_frame_source_serial_=serial;
        adjusted_frame_owner_=std::move(immutable);
        physical();full();physical();
        const auto width=gui_.width(),height=gui_.height();
        window_.require_current();physical();
        const auto actual=adjusted_frame_owner_;
        require_adjusted_frame_owner_without_callbacks(actual);
        auto result=renderer.paint_worldview_control_publication(*this,*actual,width,height);
        physical();full();physical();
        return result;
    }catch(...){core.failed.store(true);throw;}
}
}

namespace civ5::ui {
namespace {
// Recovered scalar FP32 instruction boundaries. Exceptional states are not
// clamped or treated as successful original outcomes by the callers below.
float scroll_sub(float a,float b){volatile float r=a-b;return r;}
float scroll_mul(float a,float b){volatile float r=a*b;return r;}
float scroll_div(float a,float b){volatile float r=a/b;return r;}
float stack_add(float a,float b){volatile float r=a+b;require(std::isfinite(r),"Stack native ADDSS overflow");return r;}
float stack_mul(float a,float b){volatile float r=a*b;require(std::isfinite(r),"Stack native MULSS overflow");return r;}
float stack_abs_delta(float a,float b){const auto r=static_cast<float>(std::fabs(static_cast<long double>(a)-static_cast<long double>(b)));
    require(std::isfinite(r),"Stack native x87 bounds difference overflow");return r;}
float stack_authored_float(const content::XmlNode& xml,std::string_view key,float native_default){
    const auto text=xml.attribute(std::string(key));if(text.empty())return native_default;float v=0;
    const auto p=std::from_chars(text.data(),text.data()+text.size(),v);
    require(p.ec==std::errc{}&&p.ptr==text.data()+text.size()&&std::isfinite(v),"Stack actual authored float unsupported");return v;
}
std::array<double,2> stack_authored_offset(const content::XmlNode&xml){
    const auto text=xml.attribute("Offset");const auto comma=text.find(',');require(!text.empty()&&comma!=text.npos&&text.find(',',comma+1)==text.npos,
        "Actual effective offset requires native base getter; no authored/default viewport guess");
    std::array<double,2> result;for(std::size_t n=0;n<2;++n){const auto part=n?std::string_view(text).substr(comma+1):std::string_view(text).substr(0,comma);
        double v=0;const auto p=std::from_chars(part.data(),part.data()+part.size(),v);require(p.ec==std::errc{}&&p.ptr==part.data()+part.size()&&std::isfinite(v),"Actual Offset spelling unsupported");result[n]=v;}return result;
}
}
void BnwWorldViewControlTree::publish_notification_cpu_geometry_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 std::vector<detail::NotificationCpuGeometry> values){
 const auto saved=core_;try{
  saved->layout_origin();saved->layout_current_owned();saved->layout_origin();
  require(saved->context=="NotificationPanel","CPU geometry publisher requires actual Notification context");
  const auto& actual=saved->source->current_operation_loan();saved->layout_origin();
  require(&actual==&operation,"CPU geometry actual Source operation substituted");
  const auto serial=operation.operation_serial();operation.require_operation(serial);saved->layout_origin();
  saved->busy=true;struct End{bool& v;~End(){v=false;}} end{saved->busy};
  auto staged=saved->nodes;std::set<std::uint64_t> selected;
  for(auto& value:values){
   const auto& node=Core::node(staged,value.id);
   require(value.parent==node.parent&&!node.external_parent_id&&value.source_serial==serial&&
     value.effect_revision==saved->effect_revision&&selected.insert(value.id).second,
     "CPU geometry canonical epoch/topology differs");
   for(float v:value.rect)require(std::isfinite(v),"CPU geometry rect nonfinite");
   for(float v:value.offset)require(std::isfinite(v),"CPU geometry offset nonfinite");
   for(float v:value.intrinsic_size)require(std::isfinite(v),"CPU geometry intrinsic nonfinite");
   value.topology_revision=saved->topology_revision;staged[value.id-1].cpu_geometry=std::move(value);
  }
  std::size_t count=0;for(const auto& n:staged)if(n.alive)++count;
  require(selected.size()==count,"CPU geometry must cover complete actual live Notification tree");
  saved->source->require_sources_current();saved->layout_origin(true);
  require(&saved->source->current_operation_loan()==&operation,"CPU geometry Source operation changed before commit");saved->layout_origin(true);
  operation.require_operation(serial);saved->layout_origin(true);saved->nodes.swap(staged);
  // No effect revision or LayoutState returned/readiness field is changed.
 }catch(...){saved->failed.store(true);throw;}
}

struct BnwWorldViewControlTree::LayoutOperandLoan::Capture {
    std::shared_ptr<Core> tree;
    const runtime::BnwWorldViewSourceOperationLoan* operation;
    std::uint64_t id,serial,topology,effects;
    Operand target;
    std::vector<Operand> children;
    std::array<float,2> zero,margin;
    std::optional<std::array<float,2>> anchor;
    std::optional<ScrollOperand> scroll;
    std::vector<Domain> domains;
};
void BnwWorldViewControlTree::LayoutOperandLoan::require_initialization_owned(
    const std::shared_ptr<Core>& core,const std::shared_ptr<const LayoutInitializationReturn>& seal){
    core->layout_origin(core->busy);
    require(bool(seal)&&bool(seal->native_return_),"Layout genuine native5761E0 post-DTOR completion uncaptured");
    const auto actual=seal->tree_.lock();
    require(actual&&actual.get()==core.get()&&!actual.owner_before(core)&&!core.owner_before(actual)&&
        seal->topology_==core->topology_revision,"Layout typed initializer SAME Core/topology differs");
    require(core->context_layout_baseline&&core->context_parent_kind&&core->context_layout_children,
        "Layout typed initializer complete genuine Context topology absent");
    std::set<std::uint64_t> seen;
    for(const auto& member:seal->members_){
        require(seen.insert(member.id).second,"Layout initializer receipt duplicated canonical member");
        const auto& state=member.id==0?*core->context_layout_baseline:
            *([&]() -> const detail::NotificationLayoutBaseline* {
                const auto& node=Core::node(core->nodes,member.id);
                require(node.layout_baseline.has_value(),"Layout initialized member baseline absent");
                return &*node.layout_baseline;
            }());
        const auto origin=seal->native_return_->impl_->origin;
        if(member.id==0&&origin&&origin->direct_read_){
            require(origin->direct_constructor_&&origin->parser_serial_==0&&member.parser_serial==0&&
                !state.parsed_source_serial_&&member.initialized_sequence!=0&&state.sequence_>=member.initialized_sequence,
                "Layout direct initialized root body/constructor history changed");
        }else {
        require(member.parser_serial!=0&&state.parsed_source_serial_==member.parser_serial&&
            member.initialized_sequence!=0&&state.sequence_>=member.initialized_sequence,
            "Layout initialized parser/body occurrence history changed");
        }
    }
    require(seen.contains(0),"Layout native Context initializer receipt missing");
    std::size_t count=1;
    for(const auto& node:core->nodes)if(node.alive){++count;require(seen.contains(node.id),"Layout actual initialized child omitted");}
    require(seen.size()==count,"Layout initializer receipt member set changed");
    for(float value:seal->native_zero_)require(std::isfinite(value),"Layout earned native zero operand nonfinite");
    core->require_layout_resources_owned();core->layout_origin(core->busy);
}
BnwWorldViewControlTree::LayoutOperandLoan::Prefix
BnwWorldViewControlTree::LayoutOperandLoan::capture_parent_prefix_owned(const std::shared_ptr<Core>& core,std::uint64_t id){
    if(core->creator!=std::this_thread::get_id()){core->failed.store(true);require(false,"Layout parent prefix foreign creator");}
    core->layout_origin(core->busy);
    const detail::NotificationLayoutBaseline* baseline=nullptr;
    const Core::LayoutFourCReturn* stamp=nullptr;std::uint64_t offset_sequence=0;
    if(id==0){require(core->context_layout_baseline&&core->context_layout_four_c_return,
        "Layout ancestor actual own4C prefix absent");baseline=&*core->context_layout_baseline;
        stamp=&*core->context_layout_four_c_return;offset_sequence=core->context_layout_offset_store_sequence;
    }else{const auto& node=Core::node(core->nodes,id);
        require(node.layout_baseline&&node.layout_four_c_return&&!node.external_parent_id,
            "Layout actual parent control own4C prefix absent");
        baseline=&*node.layout_baseline;stamp=&*node.layout_four_c_return;offset_sequence=node.layout_offset_store_sequence;
    }
    require(baseline->parsed_source_serial_&&*baseline->parsed_source_serial_!=0&&stamp->source_serial!=0&&
        stamp->topology_revision==core->topology_revision&&stamp->baseline_sequence!=0&&
        stamp->baseline_sequence<=baseline->sequence_&&stamp->offset_store_sequence==offset_sequence&&offset_sequence!=0,
        "Layout ancestor genuine parser/own4C/offset occurrence changed");
    for(std::size_t i=0;i<2;++i)require(std::isfinite(baseline->base_.offset_28_2c[i])&&
        std::bit_cast<std::uint32_t>(stamp->actual_offset[i])==std::bit_cast<std::uint32_t>(baseline->base_.offset_28_2c[i]),
        "Layout ancestor current actual28/2c differs from own4C prefix");
    return {id,*baseline->parsed_source_serial_,stamp->source_serial,stamp->baseline_sequence,offset_sequence,stamp->actual_offset};
}
void BnwWorldViewControlTree::LayoutOperandLoan::repeat_parent_prefix_owned(
    const std::shared_ptr<Core>& core,const Prefix& expected){
    const auto current=capture_parent_prefix_owned(core,expected.id);
    require(current.parser_serial==expected.parser_serial&&current.source_serial==expected.source_serial&&
        current.baseline_sequence==expected.baseline_sequence&&current.offset_store_sequence==expected.offset_store_sequence,
        "Layout actual ancestor own4C prefix replaced");
    for(std::size_t i=0;i<2;++i)require(std::bit_cast<std::uint32_t>(current.offset[i])==std::bit_cast<std::uint32_t>(expected.offset[i]),
        "Layout captured ancestor offset prefix changed");
}
BnwWorldViewControlTree::LayoutOperandLoan::Operand
BnwWorldViewControlTree::LayoutOperandLoan::capture_operand_owned(const std::shared_ptr<Core>& core,std::uint64_t id){
    const auto& node=Core::node(core->nodes,id);
    require(node.layout_baseline&&!node.external_parent_id,"Layout canonical baseline/parent operand absent");
    const auto& state=*node.layout_baseline;
    return {id,node.parent,node.attached_order,state.base_.flags_40,
        state.base_.offset_28_2c,state.base_.size_30_34,state.base_.extent_20_24,state.base_.bounds_10_1c};
}
BnwWorldViewControlTree::LayoutOperandLoan::Capture
BnwWorldViewControlTree::LayoutOperandLoan::capture_owned(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
    const auto saved=control.owner_.lock();require(bool(saved),"Layout canonical Control owner expired");
    saved->layout_origin();std::shared_ptr<View::Binding> binding;
    try{
        const auto id=saved->checked(control);const auto serial=operation.operation_serial();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation&&serial!=0,
            "Layout Loan mint lacks SAME entered actual Source operation");saved->layout_origin();
        // Refuse BEFORE any receiver numerical prefix. XML/parser/4C/default
        // arithmetic alone never constructs this distinct native initializer seal.
        require_initialization_owned(saved,saved->layout_initialization_return);
        Capture out;out.tree=saved;out.operation=&operation;out.id=id;out.serial=serial;
        out.topology=saved->topology_revision;out.effects=saved->effect_revision;
        saved->busy=true;struct Exit{bool& v;~Exit(){v=false;}}exit{saved->busy};
        binding=std::make_shared<View::Binding>(saved,operation);
        {
            View view(binding,binding->captured);view.require_pure();
            require(binding->operation==&operation&&binding->serial==serial,"Layout capture actual Source serial changed");
            for(const auto& domain:binding->domains)
                out.domains.push_back({domain.core,domain.topology,domain.effects,
                    domain.core.get()==saved.get()?saved->layout_initialization_return:nullptr,{}});
            // Actual target pertype members are proved by the selected genuine
            // completed5761 seal above. Native ancestor scalar reads need the
            // actual own4C prefix, while that ancestor may still run child Lua.
            auto parent=Parent::node(view,Parent::NodeKey{0,id}).parent;
            std::set<std::pair<std::uint64_t,std::uint64_t>> visited;
            while(parent){require(visited.emplace(parent->domain,parent->id).second,"Layout actual parent prefix cycle");
                if(parent->domain!=0)out.domains.at(parent->domain).parent_prefixes.push_back(
                    capture_parent_prefix_owned(binding->domains.at(parent->domain).core,parent->id));
                parent=Parent::node(view,*parent).parent;
            }
            out.target=capture_operand_owned(saved,id);
            std::vector<std::uint64_t> ids;
            for(const auto& node:saved->nodes)if(node.alive&&node.parent==id)ids.push_back(node.id);
            std::sort(ids.begin(),ids.end(),[&](auto a,auto b){return Core::node(saved->nodes,a).attached_order<Core::node(saved->nodes,b).attached_order;});
            for(auto child:ids)out.children.push_back(capture_operand_owned(saved,child));
            const auto& node=Core::node(saved->nodes,id);const auto& state=*node.layout_baseline;
            require(bool(saved->context_gui_port),"Layout mutable arithmetic namespace absent");
            out.zero=saved->context_gui_port->arithmetic_pair_without_callbacks(operation);view.require_pure();out.margin=state.base_.margin_70_74;
            // Optional observation of completed4C, never a substituted anchor.
            if(node.layout_four_c_return){const auto& stamp=*node.layout_four_c_return;
                require(stamp.topology_revision==saved->topology_revision&&stamp.offset_store_sequence==node.layout_offset_store_sequence&&
                    stamp.baseline_sequence<=state.sequence_&&stamp.actual_offset==state.base_.offset_28_2c,
                    "Layout completed4C actual anchor history differs");out.anchor=state.base_.offset_28_2c;
            }
            if(state.scroll_){
                const auto& native=*state.scroll_;
                require(native.handles_c8_cc_d0.has_value(),"Layout actual c8/cc/d0 resolver observations unearned");
                ScrollOperand scroll;
                scroll.native.viewport_30_34=state.base_.size_30_34;scroll.native.flags_40=state.base_.flags_40;
                scroll.native.calculated_84_88=native.calculated_84_88;scroll.native.internal_8c_90_94_98=native.internal_8c_98;
                scroll.native.offset_9c_a0=native.offset_9c_a0;scroll.native.fraction_a4=native.fraction_a4;
                scroll.native.ratio_a8=native.ratio_a8;scroll.native.range_ac=native.range_ac;
                scroll.native.range_b0=native.range_b0;scroll.native.step_b4=native.step_b4;
                for(std::size_t i=0;i<3;++i)if(const auto handle=(*native.handles_c8_cc_d0)[i])
                    scroll.handles[i]=capture_operand_owned(saved,handle);
                out.scroll=std::move(scroll);
            }
            view.require_full();view.require_pure();
        }
        binding->pure(false);require(binding->destruction_passed,"Layout capture scope destructor refused");
        binding->finished=true;return out;
    }catch(...){if(binding)binding->poison();saved->failed.store(true);throw;}
}
BnwWorldViewControlTree::LayoutOperandLoan::LayoutOperandLoan(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation):
    LayoutOperandLoan(capture_owned(control,operation)){}
BnwWorldViewControlTree::LayoutOperandLoan::LayoutOperandLoan(Capture capture):
    tree_(capture.tree),operation_(capture.operation),control_id_(capture.id),source_serial_(capture.serial),
    topology_revision_(capture.topology),effect_revision_(capture.effects),target_(capture.target),
    children_(std::move(capture.children)),native_zero_(capture.zero),native_margin_(capture.margin),
    actual_base_anchor_(capture.anchor),scroll_(std::move(capture.scroll)),domains_(std::move(capture.domains)){
    require_for_core_without_callbacks(capture.tree,control_id_);
}
void BnwWorldViewControlTree::dispatch_layout_operand_owned(const Control& control,
    const runtime::BnwWorldViewSourceOperationLoan& operation,LayoutOperandCommand command,double value){
    const auto saved=core_;saved->layout_origin();const auto actual=control;
    try{
        const auto pure=[&]{saved->layout_origin();
            require(&saved->source->current_operation_loan()==&operation&&operation.operation_serial()!=0,
                "Layout dispatch SAME Source operation substituted");saved->layout_origin();
            require(saved->context=="NotificationPanel","Layout dispatch context is outside actual selected owner");
            saved->checked(actual);
        };
        pure();operation.require_current();pure();
        require(command==LayoutOperandCommand::calculate_size||command==LayoutOperandCommand::reprocess_anchoring||
            command==LayoutOperandCommand::set_size_y||command==LayoutOperandCommand::calculate_internal_size,
            "Layout dispatch command unissued");
        require(command!=LayoutOperandCommand::set_size_y||(std::isfinite(value)&&std::isfinite(static_cast<float>(value))),
            "Layout dispatch actual SetSizeY FP32 argument unsupported");
        {
            const LayoutOperandLoan loan(actual,operation);pure();
            switch(command){
            case LayoutOperandCommand::calculate_size:calculate_size_owned(actual,loan);break;
            case LayoutOperandCommand::reprocess_anchoring:reprocess_anchoring_owned(actual,loan);break;
            case LayoutOperandCommand::set_size_y:set_size_y_owned(actual,value,loan);break;
            case LayoutOperandCommand::calculate_internal_size:calculate_internal_size_owned(actual,loan);break;
            }
            // Receiver Binding tracks only its own stored revisions. Do not
            // compare old Loan numerical preimage AFTER genuine owned writes.
            pure();
        } // Nonthrowing closed Loan destruction retires its sole call lifetime.
        pure();operation.require_current();pure();
    }catch(...){saved->failed.store(true);throw;}
}
BnwWorldViewControlTree::LayoutOperandLoan::~LayoutOperandLoan()noexcept{alive_=false;}
void BnwWorldViewControlTree::LayoutOperandLoan::require_for_core_without_callbacks(const std::shared_ptr<Core>&saved,std::uint64_t id)const{
    require(saved&&creator_==std::this_thread::get_id()&&alive_,"Layout loan retired/foreign");
    saved->layout_origin(saved->busy);
    const auto actual=tree_.lock();require(actual&&actual.get()==saved.get()&&!actual.owner_before(saved)&&!saved.owner_before(actual)&&id==control_id_,
        "Layout SAME canonical Tree/control replaced");
    require(operation_&&source_serial_&&saved->topology_revision==topology_revision_&&saved->effect_revision==effect_revision_,
        "Layout actual source/topology/effects differ");
    const auto&operation=saved->source->current_operation_loan();saved->layout_origin(saved->busy);require(&operation==operation_&&operation.operation_serial()==source_serial_,"Layout SAME actual Source operation substituted");
    const auto&target=Core::node(saved->nodes,id);require(target_.id==id&&target_.parent==target.parent&&target_.attached_order==target.attached_order&&!target.external_parent_id,
        "Layout actual target topology differs");
    const auto finite=[](const auto&values){for(float v:values)require(std::isfinite(v),"Layout real native operand nonfinite");};
    finite(target_.offset);finite(target_.size);finite(target_.extent);finite(target_.bounds);finite(native_zero_);finite(native_margin_);
    if(actual_base_anchor_)finite(*actual_base_anchor_);
    // No hidden-only eligibility approximation: native masks0x240 remain
    // authentic layout operands. Owned SetHide qualifies only bit2.
    require(bool(target_.native_flags&2)==target.hidden,"Layout target native SetHide bit2 differs");
    std::size_t count=0;std::uint64_t previous=0;
    for(const auto&child:children_){const auto&n=Core::node(saved->nodes,child.id);
        require(n.parent==id&&child.parent==id&&n.attached_order==child.attached_order&&child.attached_order>previous&&!n.external_parent_id&&
            bool(child.native_flags&2)==n.hidden,"Layout actual ordered native child/flags differs");previous=child.attached_order;
        finite(child.offset);finite(child.size);finite(child.extent);finite(child.bounds);++count;
    }
    std::size_t actual_count=0;for(const auto&n:saved->nodes)if(n.alive&&n.parent==id)++actual_count;
    require(count==actual_count,"Layout genuine immediate children incomplete");
    // Repeated only BEFORE the actual receiver prefix; Binding thereafter
    // tracks its own real stores. Foreign entry already failed above.
    for(const auto& domain:domains_){
        const auto core=domain.core.lock();require(bool(core),"Layout captured initialization domain retired");
        if(core->creator!=creator_){core->failed.store(true);require(false,"Layout captured domain foreign creator");}
        core->layout_origin(core->busy);
        require(core->topology_revision==domain.topology&&core->effect_revision==domain.effects,
            "Layout captured reached domain/history/effect epoch substituted");
        if(domain.seal){
            require(core.get()==saved.get()&&core->layout_initialization_return.get()==domain.seal.get()&&
                !core->layout_initialization_return.owner_before(domain.seal)&&!domain.seal.owner_before(core->layout_initialization_return),
                "Layout selected genuine complete initialization receipt replaced");
            require_initialization_owned(core,domain.seal);
        }else{
            require(core.get()!=saved.get()&&!domain.parent_prefixes.empty(),"Layout ancestor actual own4C history absent");
            for(const auto& prefix:domain.parent_prefixes)repeat_parent_prefix_owned(core,prefix);
            core->require_layout_resources_owned();core->layout_origin(core->busy);
        }
    }
    const auto same=[](const auto& a,const auto& b){
        for(std::size_t i=0;i<a.size();++i)if(std::bit_cast<std::uint32_t>(a[i])!=std::bit_cast<std::uint32_t>(b[i]))return false;
        return true;
    };
    const auto equal_operand=[&](const Operand& before){
        const auto now=capture_operand_owned(saved,before.id);
        require(now.id==before.id&&now.parent==before.parent&&now.attached_order==before.attached_order&&
            now.native_flags==before.native_flags&&same(now.offset,before.offset)&&same(now.size,before.size)&&
            same(now.extent,before.extent)&&same(now.bounds,before.bounds),
            "Layout canonical captured fields changed before receiver prefix");
    };
    equal_operand(target_);for(const auto& child:children_)equal_operand(child);
    const auto& baseline=*target.layout_baseline;
    require(same(native_margin_,baseline.base_.margin_70_74)&&
        saved->context_gui_port&&same(native_zero_,saved->context_gui_port->arithmetic_pair_without_callbacks(operation)),
        "Layout earned native margin/current mutable arithmetic carrier differs");
    require(bool(scroll_)==bool(baseline.scroll_),"Layout target typed Scroll state changed");
    if(scroll_){const auto& state=*baseline.scroll_;const auto& captured=scroll_->native;
        require(state.handles_c8_cc_d0.has_value(),"Layout Scroll actual resolver observations lost");
        require(same(captured.calculated_84_88,state.calculated_84_88)&&same(captured.internal_8c_90_94_98,state.internal_8c_98)&&
            same(captured.offset_9c_a0,state.offset_9c_a0),"Layout Scroll captured FP32 array changed");
        const std::array<float,5> before{captured.fraction_a4,captured.ratio_a8,captured.range_ac,captured.range_b0,captured.step_b4};
        const std::array<float,5> now{state.fraction_a4,state.ratio_a8,state.range_ac,state.range_b0,state.step_b4};
        require(same(before,now),"Layout Scroll captured FP32 scalar changed");
        for(std::size_t i=0;i<3;++i){const auto id=(*state.handles_c8_cc_d0)[i];const auto& handle=scroll_->handles[i];
            require((id==0)==!handle.has_value()&&(!handle||handle->id==id),
                "Layout earned c8/cc/d0 NULL/handle binding changed");
            if(handle)equal_operand(*handle);
        }
    }
    if(scroll_){
        const auto&s=scroll_->native;
        require(target.authored.name=="ScrollPanel"&&s.viewport_30_34==target_.size&&s.flags_40==target_.native_flags,
            "Scroll actual native viewport/flags differ from SAME target binding");
        finite(s.viewport_30_34);finite(s.calculated_84_88);finite(s.offset_9c_a0);finite(s.internal_8c_90_94_98);
        require(std::isfinite(s.fraction_a4)&&std::isfinite(s.ratio_a8)&&std::isfinite(s.range_ac)&&std::isfinite(s.range_b0)&&std::isfinite(s.step_b4),
            "Scroll actual native input nonfinite environment unsupported");
        for(const auto&handle:scroll_->handles)if(handle){
            const auto&actual_handle=Core::node(saved->nodes,handle->id);
            require(actual_handle.parent==handle->parent&&actual_handle.attached_order==handle->attached_order&&
                !actual_handle.external_parent_id&&bool(handle->native_flags&2)==actual_handle.hidden,
                "Scroll actual c8/cc/d0 canonical handle/topology changed");
            finite(handle->offset);finite(handle->size);finite(handle->extent);finite(handle->bounds);
        }
    }
}
std::array<double,2> BnwWorldViewControlTree::get_offset_val(const Control&control)const{
    const auto saved=core_;const auto actual=control;
    try{
        saved->layout_origin();saved->layout_current_owned();saved->layout_origin();
        const auto id=saved->checked(actual);const auto&n=Core::node(saved->nodes,id);
        const auto&op=saved->source->current_operation_loan();saved->layout_origin();
        require(n.layout_baseline&&n.layout_four_c_return&&n.layout_offset_store_sequence!=0,
            "GetOffsetVal requires actual complete4C native offset history; initializer phase uncaptured");
        const auto& stamp=*n.layout_four_c_return;const auto& baseline=*n.layout_baseline;
        require(stamp.source_serial==op.operation_serial()&&stamp.topology_revision==saved->topology_revision&&
            stamp.offset_store_sequence==n.layout_offset_store_sequence&&
            stamp.baseline_sequence!=0&&stamp.baseline_sequence<=baseline.sequence_,
            "GetOffsetVal complete4C source/topology/actual offset store history changed");
        // Read the actual canonical28/2c fields. XMLmargin, command inputs,
        // constructor zero and generic receiver_return never issue this getter.
        const auto value=baseline.base_.offset_28_2c;
        require(std::bit_cast<std::uint32_t>(value[0])==std::bit_cast<std::uint32_t>(stamp.actual_offset[0])&&
            std::bit_cast<std::uint32_t>(value[1])==std::bit_cast<std::uint32_t>(stamp.actual_offset[1])&&
            std::isfinite(value[0])&&std::isfinite(value[1]),
            "GetOffsetVal actual native fields differ from completed4C history");
        const std::array<double,2> result{value[0],value[1]};
        saved->layout_origin();return result;
    }catch(...){saved->failed.store(true);throw;}
}
double BnwWorldViewControlTree::get_size_y(const Control&control)const{
    const auto saved=core_;const auto actual=control;try{saved->layout_origin();saved->layout_current_owned();saved->layout_origin();const auto id=saved->checked(actual);const auto&n=Core::node(saved->nodes,id);const auto&op=saved->source->current_operation_loan();saved->layout_origin();
    require(n.layout&&n.layout->calculate_returned&&n.layout->source_serial==op.operation_serial()&&n.layout->topology_revision==saved->topology_revision&&
        n.layout->effect_revision==saved->effect_revision,"GetSizeY requires returned actual CalculateSize SAME source/model epoch");return n.layout->calculated_size[1];
}catch(...){saved->failed.store(true);throw;}
}
double BnwWorldViewControlTree::get_ratio(const Control&control)const{
    const auto saved=core_;const auto actual=control;try{saved->layout_origin();saved->layout_current_owned();saved->layout_origin();const auto&n=Core::node(saved->nodes,saved->checked(actual));const auto&op=saved->source->current_operation_loan();saved->layout_origin();
    require(n.authored.name=="ScrollPanel"&&n.layout&&n.layout->internal_returned&&n.layout->ratio&&n.layout->source_serial==op.operation_serial()&&
        n.layout->effect_revision==saved->effect_revision&&n.layout->topology_revision==saved->topology_revision,
        "GetRatio requires genuine returned ScrollPanel internal/viewport owner; no ratio1 fallback");return *n.layout->ratio;
}catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::require_layout_backend_origin_without_callbacks(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=core_;const auto actual=control;
    try{
        saved->layout_origin();
        const auto& entered=saved->source->current_operation_loan();saved->layout_origin();
        require(&entered==&operation&&entered.operation_serial()!=0,
            "Layout backend does not have SAME active Source operation");
        const auto& node=Core::node(saved->nodes,saved->checked(actual));
        require(saved->context=="NotificationPanel"&&node.layout_baseline&&
            node.layout_baseline->parsed_source_serial_!=0,
            "Layout backend lacks authentic canonical Notification parser state");
        saved->layout_origin();
    }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::calculate_size_owned(const Control&control,const LayoutOperandLoan&loan){
    run_layout_parent_receiver_owned(control,loan,LayoutParentCommand::calculate_stack);
}
void BnwWorldViewControlTree::set_size_y_owned(const Control& control,double value,const LayoutOperandLoan& loan){
    run_layout_scroll_receiver_owned(control,loan,LayoutScrollCommand::size_y,value);
}
void BnwWorldViewControlTree::reprocess_anchoring_owned(const Control&control,const LayoutOperandLoan&loan){
    run_layout_parent_receiver_owned(control,loan,LayoutParentCommand::reprocess_stack);
}
void BnwWorldViewControlTree::calculate_internal_size_owned(const Control& control,const LayoutOperandLoan& loan){
    run_layout_scroll_receiver_owned(control,loan,LayoutScrollCommand::internal_size,0.0);
}
//Requires complete Context constructor/initializer and standalone owners above.
#include "detail/bnw_normal_root_batch_history_core.inc"
#include "detail/bnw_normal_item_occurrence_core.inc"
#include "detail/bnw_normal_root_batch_execution_core.inc"
#include "detail/bnw_normal_item_historical_atlas_attachment_core.inc"
#include "detail/bnw_normal_root_bootstrap_selection_core.inc"
#include "detail/bnw_normal_root_context_entry_core.inc"
#include "detail/bnw_normal_root_context_scope_core.inc"
#include "detail/bnw_normal_root_context_publication_core.inc"
#include "detail/bnw_normal_context_child_publication_core.inc"
#include "detail/bnw_normal_root_context_batch_bridge_core.inc"
#include "detail/bnw_normal_publication_prepaint_owner_core.inc"
#include "detail/bnw_normal_context_common_kernel_core.inc"
#include "detail/bnw_normal_common_child_dispatch_core.inc"
#include "detail/bnw_normal_visible_label_call_core.inc"
#include "detail/bnw_normal_root_bootstrap_selection_association_core.inc"
#include "detail/bnw_normal_payload_pending_label_core.inc"
#include "detail/bnw_normal_font_payload_arena_core.inc"
#include "detail/bnw_normal_label_material_core.inc"
#include "detail/bnw_normal_root_outer_manager_core.inc"
#include "detail/bnw_normal_manager_selected_entry_core.inc"
#include "detail/bnw_normal_label_selected_shader_access_core.inc"
#include "detail/bnw_normal_root_visible_label_traversal_core.inc"
#include "detail/bnw_normal_root_completed_owner_handoff_core.inc"
#include "detail/bnw_normal_common_null_child_return_core.inc"
#include "detail/bnw_normal_common_recursive78_core.inc"
#include "detail/bnw_normal_button_positive_caption_call_core.inc"
#include "detail/bnw_normal_button_caption_append_emission_core.inc"
#include "detail/bnw_normal_button_caption_publication_core.inc"
#include "detail/bnw_normal_button_caption_item_history_core.inc"
#include "detail/bnw_normal_button_caption_payload_core.inc"
#include "detail/bnw_normal_button_caption_material_core.inc"
#include "detail/bnw_normal_embedded_label_common56_origin_core.inc"
#include "detail/bnw_normal_common_manager_receiver_origin_core.inc"
#include "detail/bnw_normal_button_caption_common_callback_core.inc"
#include "detail/bnw_normal_button_caption_fp32_rect_core.inc"
#include "detail/bnw_normal_button_caption_fp32_association_core.inc"
#include "detail/bnw_normal_common_child_stage_core.inc"
#include "detail/bnw_normal_context_child_body_publication_core.inc"
#include "detail/bnw_normal_nonfont_current_button_receiver_core.inc"
#include "detail/bnw_selected_direct_root_read_registry_core.inc"
}
