#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include "civ5/ui/civis_input_policy_owner.hpp"
#include "lua_worldview_original_input_enum_data.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/graphics/bnw_clear_hex_highlight_owner.hpp"
#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include "civ5/ui/detail/notification_context_loader_cleanup_return.hpp"
#include "civ5/ui/detail/notification_context_cursor_restore_return.hpp"
#include "civ5/ui/detail/notification_context_lua_state_preparation.hpp"
#include "civ5/ui/detail/notification_context_postread_globals_reference.hpp"
#include "civ5/ui/detail/notification_context_postread_root_cdata_return.hpp"
#include "civ5/ui/detail/notification_context_direct_lua_tail_input.hpp"
#include "civ5/ui/detail/notification_context_direct_lua_tail_input.hpp"
#include "civ5/ui/detail/notification_context_ui_manager_value_loan.hpp"
#include "civ5/ui/detail/notification_context_lua_state_name_prefix_return.hpp"
#include "civ5/ui/detail/notification_context_lua_debug_config_prefix.hpp"
#include "civ5/ui/detail/notification_context_lua_profile_return.hpp"
#include "civ5/ui/detail/notification_context_lua_controls_return.hpp"
#include "civ5/ui/detail/notification_context_child_controls_export_loan.hpp"
#include "civ5/ui/detail/notification_context_child_controls_export_return.hpp"
#include "civ5/ui/detail/notification_context_root_ordinary_controls_export_loan.hpp"
#include "civ5/ui/detail/notification_context_root_ordinary_controls_export_return.hpp"
#include "civ5/ui/detail/notification_context_lua_load_setup_return.hpp"
#include "civ5/ui/detail/notification_context_root_default_fields_return.hpp"
#include "civ5/runtime/detail/bnw_context_lua_profile_config_loan.hpp"
#include "civ5/runtime/bnw_context_lua_include_collection_loan.hpp"
#include "civ5/ui/detail/notification_context_load_name_loan.hpp"
#include "civ5/ui/detail/notification_context_gui_port.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/detail/bnw_civis_application_state_owner.hpp"
#include "civ5/runtime/bnw_worldview_script_ui_adapter.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/ui/bnw_tech_award_context_owner.hpp"
#include "civ5/runtime/bnw_owned_team_set_has_tech_lua_dispatch.hpp"
#include "civ5/runtime/bnw_selected_production_action_loan.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/script_ui_control_graph.hpp"
#include "civ5/runtime/bnw_unitflag_health_instance_loan.hpp"
#include "civ5/runtime/bnw_unitflag_health_fifo_loan.hpp"
#include "civ5/runtime/bnw_unitflag_living_health_read.hpp"
#include "civ5/runtime/bnw_selected_unitflag_context_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/bnw_gameinfo_actions_owner.hpp"
#include "../content/bnw_notification_entries_source_owner.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/locale_text_provider.hpp"
#include "civ5/runtime/civis_lua_table_cardinality.hpp"
#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "lua_events_bootstrap.hpp"
#include "lua_custom_events_bootstrap.hpp"
#include "lua_ui_events_bootstrap.hpp"
#include "lua_include_bootstrap.hpp"
#include "lua_modding_bootstrap.hpp"
#include "lua_gameplay_bootstrap.hpp"
#include "lua_gameinfo_bootstrap.hpp"
#include "lua_native_gameinfo_ctor_bootstrap.hpp"
#include "lua_native_ui_enum_data.hpp"
#include "lua_pregame_bootstrap.hpp"
#include "lua_map_bootstrap.hpp"
#include "lua_map_setup_bootstrap.hpp"
#include "lua_content_bootstrap.hpp"
#include "lua_locale_bootstrap.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <charconv>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <array>
#include "civ5/graphics/bnw_unit_setup_graphical_transport.hpp"
#include <cstdlib>
#include <cstdio>
#include <exception>
#include <limits>
#include <set>
#include <deque>
#include <thread>
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "llimits.h"
#include "lobject.h"
#include "lstate.h"
}
namespace civ5::runtime {
static_assert(sizeof(void*)==8&&sizeof(lua_Integer)==8&&sizeof(lu_mem)==8&&sizeof(l_mem)==8&&sizeof(LUA_INTFRM_T)==8,
              "Civis Lua requires wide pointers, integer API and memory counters");
static_assert(sizeof(lua_Number)==8,"Lua compatibility arithmetic requires binary64");
struct LuaScriptHost::Impl : std::enable_shared_from_this<LuaScriptHost::Impl> {
    ScriptBudget budget;lua_State *state=nullptr;std::uint64_t bytes=0,remaining=0;
    // Genuine canonical master initialization; no Source-derived config authority.
    bool profile_debug_master_alive=false,profile_gc_stopped=false;
    int profile_debug_master_ref=LUA_NOREF;const void*profile_debug_master_table=nullptr;
    struct NativeContextIncludeChildControl;
    struct NativeContextControlsFrame;NativeContextControlsFrame* controls_frame=nullptr;
    struct NativeContextCDataCreator;struct NativeChildContextControlsResult;struct NativeChildContextControlsFrame;
    std::shared_ptr<NativeContextCDataCreator> native_context_cdata_creator;
    NativeChildContextControlsFrame* child_controls_export_frame=nullptr;
    struct NativeEnteredControlsFrame;NativeEnteredControlsFrame* entered_controls_frame=nullptr;
    struct NativeControlsNamedRetireFrame;NativeControlsNamedRetireFrame* controls_named_retire_frame=nullptr;
    std::vector<std::shared_ptr<NativeChildContextControlsResult>> child_controls_export_results;
    struct NativeOrdinaryCDataCreator;struct NativeRootOrdinaryControlsResult;struct NativeRootOrdinaryControlsFrame;
    std::array<std::shared_ptr<NativeOrdinaryCDataCreator>,6> native_ordinary_cdata_creators;
    NativeRootOrdinaryControlsFrame* root_ordinary_controls_export_frame=nullptr;
    std::vector<std::shared_ptr<NativeRootOrdinaryControlsResult>> root_ordinary_controls_export_results;
    class NativeOrdinary517CFunction;class NativeOrdinaryTypeDescriptor;class NativeOrdinaryDescriptorPrefixReturn;
    class NativeOrdinaryNestedDescriptorReturn;class NativeOrdinaryPointerFormatReturn;class NativeOrdinary548FieldReturn;
    class NativeOrdinaryReflectionResult;struct NativeOrdinaryMasterTypesResult;struct NativeOrdinaryMetatypeResult;
    std::array<std::shared_ptr<NativeOrdinaryTypeDescriptor>,6> native_ordinary_type_descriptors;
    std::array<std::shared_ptr<const NativeOrdinaryDescriptorPrefixReturn>,6> native_ordinary_descriptor_prefixes;
    std::array<std::shared_ptr<const NativeOrdinaryNestedDescriptorReturn>,2> native_ordinary_nested_descriptors;
    std::array<std::shared_ptr<const NativeOrdinary517CFunction>,3> native_ordinary_517c_functions;
    std::vector<std::shared_ptr<NativeOrdinaryMasterTypesResult>> native_ordinary_master_types_results;
    std::vector<std::shared_ptr<NativeOrdinaryMetatypeResult>> native_ordinary_metatype_results;
    std::shared_ptr<const NativeOrdinaryPointerFormatReturn> native_ordinary_pointer_format;
    std::shared_ptr<const NativeOrdinary548FieldReturn> native_ordinary_548_field;
    std::vector<std::shared_ptr<NativeOrdinaryReflectionResult>> native_ordinary_reflection_results;
    class NativeOrdinaryCodeBuddyCallback;class NativeOrdinaryCodeBuddyGlobalReturn;class NativeOrdinaryCodeBuddyTableReturn;
    std::shared_ptr<const NativeOrdinaryCodeBuddyCallback> native_ordinary_codebuddy_callback;
    std::vector<std::shared_ptr<NativeOrdinaryCodeBuddyGlobalReturn>> native_ordinary_codebuddy_globals;
    std::vector<std::shared_ptr<NativeOrdinaryCodeBuddyTableReturn>> native_ordinary_codebuddy_tables;
    struct NativeContextEventsPrefixFrame;NativeContextEventsPrefixFrame* events_prefix_frame=nullptr;
    struct NativeContextLuaEventsFrame;NativeContextLuaEventsFrame* lua_events_frame=nullptr;
    struct NativeContextApiRegistrationFrame;NativeContextApiRegistrationFrame* api_registration_frame=nullptr;
    struct NativeContextDbRegistrationFrame;NativeContextDbRegistrationFrame* db_registration_frame=nullptr;
    struct NativeContextDatabaseNamespacesFrame;NativeContextDatabaseNamespacesFrame* database_namespaces_frame=nullptr;
    struct NativeContextUiEnumsFrame;NativeContextUiEnumsFrame* ui_enums_frame=nullptr;
    struct NativeContextNetworkFrame;NativeContextNetworkFrame* network_frame=nullptr;
    struct NativeContextSteamFrame;NativeContextSteamFrame* steam_frame=nullptr;
    struct NativeContextLocaleFrame;NativeContextLocaleFrame* locale_frame=nullptr;
    struct NativeContextContentFrame;NativeContextContentFrame* content_frame=nullptr;
    struct NativeContextModdingFrame;NativeContextModdingFrame* modding_frame=nullptr;
    struct NativeContextFLuaFrame;NativeContextFLuaFrame* flua_frame=nullptr;
    struct NativeContextGameEventsFrame;NativeContextGameEventsFrame* game_events_frame=nullptr;
    struct NativeGameEventsStorage;std::unique_ptr<NativeGameEventsStorage> native_game_events_storage;
    struct NativeContextGameCoreFrame;NativeContextGameCoreFrame* gamecore_frame=nullptr;
    class NativeGameCoreBootstrapOwner;std::unique_ptr<NativeGameCoreBootstrapOwner> native_gamecore_bootstrap;
    struct NativeSelectedRegistrarProtectedCall;std::unique_ptr<NativeSelectedRegistrarProtectedCall> native_selected_registrar_call;
    struct NativeContextOptionsFrame;NativeContextOptionsFrame* options_frame=nullptr;
    int native_options_registration_keys_ref=LUA_NOREF;const void* native_options_registration_keys_pointer=nullptr;
    struct NativeContextUiRegistrationFrame;NativeContextUiRegistrationFrame* ui_registration_frame=nullptr;
    struct NativeContextUiNamespaceReturn;std::vector<std::unique_ptr<const NativeContextUiNamespaceReturn>> native_context_ui_namespace_returns;
    int native_context_ui_registration_keys_ref=LUA_NOREF;const void* native_context_ui_registration_keys_pointer=nullptr;
    struct NativeContextPathRegistrationFrame;NativeContextPathRegistrationFrame* path_registration_frame=nullptr;
    struct NativeContextProfilerRegistrationFrame;NativeContextProfilerRegistrationFrame* profiler_registration_frame=nullptr;
    int native_path_registration_keys_ref=LUA_NOREF;const void* native_path_registration_keys_pointer=nullptr;
    struct NativeContextPreGameFrame;NativeContextPreGameFrame* pregame_frame=nullptr;
    class NativePreGameBackendBinding;std::vector<std::unique_ptr<NativePreGameBackendBinding>> native_pregame_backend_bindings;
    class NativePreGameNamespaceReturn;std::vector<std::unique_ptr<NativePreGameNamespaceReturn>> native_pregame_namespace_returns;
    class NativeFLuaBootstrapOwner;std::unique_ptr<NativeFLuaBootstrapOwner> native_flua_bootstrap;
    class NativeFLuaTypedRegistryOwner;std::unique_ptr<NativeFLuaTypedRegistryOwner> native_flua_typed_registry;
    class NativeFLuaNamedRegistryOwner;std::unique_ptr<NativeFLuaNamedRegistryOwner> native_flua_named_registry;
    class NativeFLuaBuiltinRegistryOwner;std::unique_ptr<NativeFLuaBuiltinRegistryOwner> native_flua_builtin_registry;
    struct NativeFLuaMetatypeReturn;std::vector<std::shared_ptr<NativeFLuaMetatypeReturn>> native_flua_metatype_results;
    struct NativeFLuaMethodLibraryReturn;std::vector<std::shared_ptr<NativeFLuaMethodLibraryReturn>> native_flua_method_libraries;
    class NativeFLuaExposureRegistrationOwner;
    std::unique_ptr<NativeFLuaExposureRegistrationOwner> native_flua_exposure_registration;
    class NativeMasterThreadBootstrapOwner;std::unique_ptr<NativeMasterThreadBootstrapOwner> native_master_thread_bootstrap;
    class NativeModdingBackendBinding;std::vector<std::unique_ptr<NativeModdingBackendBinding>> native_modding_backend_bindings;
    int native_modding_registration_keys_ref=LUA_NOREF;const void* native_modding_registration_keys_pointer=nullptr;
    class NativeContentBackendBinding;std::vector<std::unique_ptr<NativeContentBackendBinding>> native_content_backend_bindings;
    int native_content_registration_keys_ref=LUA_NOREF;const void* native_content_registration_keys_pointer=nullptr;
    class NativeLocaleBackendBinding;std::vector<std::unique_ptr<NativeLocaleBackendBinding>> native_locale_backend_bindings;
    int native_locale_registration_keys_ref=LUA_NOREF;const void* native_locale_registration_keys_pointer=nullptr;
    std::string native_locale_key_text;const void* native_locale_operation_entry=nullptr;std::uint32_t native_locale_operation_function=0;
    class NativeSteamBackendBinding;std::vector<std::unique_ptr<NativeSteamBackendBinding>> native_steam_backend_bindings;
    int native_steam_registration_keys_ref=LUA_NOREF;const void* native_steam_registration_keys_pointer=nullptr;
    class NativeNetworkBackendBinding;std::vector<std::unique_ptr<NativeNetworkBackendBinding>> native_network_backend_bindings;
    int native_network_registration_keys_ref=LUA_NOREF;const void* native_network_registration_keys_pointer=nullptr;
    struct NativeDatabaseQueryCell;std::vector<std::unique_ptr<NativeDatabaseQueryCell>> native_db_query_cells;
    int native_db_registration_keys_ref=LUA_NOREF;const void* native_db_registration_keys_pointer=nullptr;
    std::int64_t native_db_count_scratch=0;std::optional<std::string> native_db_sql_scratch;
    bool lua_events_factory_alive=false;int lua_events_factory_ref=LUA_NOREF;
    const void* lua_events_factory_pointer=nullptr;const Closure* lua_events_factory_index=nullptr;
    const UpVal* lua_events_factory_groups=nullptr;const void* lua_events_factory_groups_pointer=nullptr;
    int lua_events_keys_ref=LUA_NOREF;const void* lua_events_keys_pointer=nullptr;
    std::shared_ptr<const NativeContextIncludeChildControl> profile_include_child_pending;
    bool include_configuration_alive=false;int include_configuration_ref=LUA_NOREF;
    const void* include_configuration_pointer=nullptr;
    struct NativeContextIncludeTableFrame;struct NativeContextIncludeTableControl;
    NativeContextIncludeTableFrame* profile_include_frame=nullptr;
    std::shared_ptr<const NativeContextIncludeTableControl> profile_include_table_control,profile_include_table_pending;
    std::shared_ptr<const BnwContextLuaIncludeCollectionLoan> profile_include_collection;
    std::vector<std::string> profile_include_names;
    bool profile_include_master_alive=false,profile_include_refreshing=false;
    int profile_include_refresh_ref=LUA_NOREF,profile_include_table_ref=LUA_NOREF;
    const void* profile_include_refresh_pointer=nullptr;const void* profile_include_table_pointer=nullptr;
    bool profile_print_sink_alive=false,profile_print_publishing=false;
    int profile_print_function_ref=LUA_NOREF;const void*profile_print_function=nullptr;
    std::shared_ptr<const BnwContextLuaProfileConfigLoan> profile_print_input;
    std::vector<ScriptMessage> profile_print_log_messages;
    ScriptMessage profile_print_console_scratch,profile_print_log_scratch;
    ScriptSourceProvider sources;std::vector<ScriptSourceError> source_errors;
    ScriptModdingProvider modding;std::vector<ScriptRow> mod_rows;
    std::optional<ScriptEvaluatedFile> mod_file;
    ScriptGameplayProvider gameplay;std::vector<ScriptValue> query_arguments;
    std::vector<std::shared_ptr<const ScriptUnitReadReference>> unit_references;
    std::shared_ptr<const ScriptUnitReadReference> unit_read_scratch;
    static constexpr std::size_t unit_reference_limit=65536;
    std::vector<std::int64_t> unit_ids;ScriptValue unit_value;
    std::vector<std::shared_ptr<const ScriptUnitRosterQuote>> unit_rosters;
    std::shared_ptr<const ScriptUnitRosterQuote> unit_roster_scratch;std::size_t unit_roster_rows=0;
    ScriptSaveDataProvider save_data;ScriptUserDataProvider user_data;ScriptValue saved_value;
    ScriptPregameProvider pregame;
    ScriptMapProvider map;ScriptValue plot_result;
    std::shared_ptr<BnwLuaCalculatedPlotYieldOwner::VmLease> calculated_plot_vm;
    ScriptContentProvider content;
    ScriptEnumProvider enums;
    ScriptLocaleProvider locale;ScriptLanguage locale_language;std::vector<ScriptValue> locale_arguments;
    std::string locale_text;
    std::string plain_lookup_context;std::vector<std::string> plain_lookup_tags;bool plain_lookup_bound=false;
    std::string locale_text_context;ScriptLocaleTextProvider locale_operations;bool locale_text_bound=false;
    std::string raw_count_context;bool raw_count_bound=false;
    std::string selected_option_context;ScriptSelectedGameOptionProvider selected_option;bool selected_option_bound=false;
    std::string units_query_context;ScriptCivisUnitsQueryProvider units_query;bool units_query_bound=false;
    ScriptCivisUnitsQueryObservations units_query_observed;
    std::string_view units_query_active_context;bool units_query_source_loading=false;
    ScriptUiEventProvider ui_events;std::set<std::string> ui_notification_names,ui_command_names;
    ScriptUiLayoutProvider ui_layout;bool ui_layout_bound=false;
    struct ControlCallback {std::string id;std::int64_t event;int function;};
    std::vector<ControlCallback> control_callbacks;
    ScriptUiTextEntryProvider text_entry;std::string captured_text;
    int text_function=LUA_NOREF,text_receiver=LUA_NOREF;
    ScriptUiContextProvider ui_context;
    int input_function=LUA_NOREF,visibility_function=LUA_NOREF;
    ScriptUiTooltipProvider tooltip;int tooltip_controls=LUA_NOREF;
    bool ui_context_closed=false,calling=false;
    const std::thread::id ui_thread=std::this_thread::get_id();
    std::vector<ScriptValue> ui_command_args;
    std::optional<ScriptRow> query_row;std::vector<std::int64_t> object_ids;
    std::pair<bool,ScriptValue> query_key;
    std::vector<std::pair<std::string,std::int64_t>> type_entries;
    std::vector<ScriptMessage> messages;
    // Native providers run before allocating Lua calls. Persistent scratch
    // storage keeps C++ destructors outside Lua's longjmp boundary, including
    // OOM while pushing a returned source or filename list into the VM.
    std::vector<std::string> source_names;std::string source_text,include_chunk_name;
    const Proto *include_bootstrap_proto=nullptr;int include_bootstrap_reference=LUA_NOREF;
    std::exception_ptr source_exception;const char *source_error=nullptr;
    // Boolean behavior is unchanged. Every true writer is privately observed;
    // only the actual pre-sentinel child-loader loan may issue its own sentinel.
    class FailureFlag final {
        friend struct Impl;
        friend class LuaScriptHost;
        friend class BnwUnitFlagHealthInstanceLoan;
        friend class ::civ5::ui::detail::NotificationContextChildControlsExportReturn;
        friend class ::civ5::ui::detail::NotificationContextRootOrdinaryControlsExportReturn;
        bool value_=false,exhausted_=false;std::uint64_t writes_=0;
        void set_loader_sentinel()noexcept{value_=true;}
      public:
        operator bool()const noexcept{return value_;}
        FailureFlag& operator=(bool value)noexcept{
            if(value){if(writes_==UINT64_MAX)exhausted_=true;else ++writes_;}
            value_=value;return *this;
        }
        FailureFlag()=default;
        FailureFlag(const FailureFlag& other)noexcept:value_(other.value_){} // bool value snapshot only
        FailureFlag& operator=(const FailureFlag& other)noexcept{return operator=(static_cast<bool>(other));}
    };
    int functions=LUA_NOREF,ui_functions=LUA_NOREF,base_environment=LUA_NOREF;bool ready=false,exhausted=false;
    FailureFlag failed;
    std::uint64_t failure_epoch=0;
    class NativeMasterDebugSocketOwner;struct NativeMasterDebugSocketCtorFrame;
    std::unique_ptr<NativeMasterDebugSocketOwner> native_master_debug_socket;
    NativeMasterDebugSocketCtorFrame* native_master_debug_socket_ctor_frame=nullptr;
    class NativeMasterLuaCriticalSection;class NativeMasterProtectedLuaBoundary;class NativeMasterProtectedCallReturn;
    std::shared_ptr<NativeMasterLuaCriticalSection> native_master_lua_critical_section;
    NativeMasterProtectedLuaBoundary* native_master_protected_boundary=nullptr;std::uint64_t native_master_protected_boundary_serial=0;
    std::vector<std::unique_ptr<NativeMasterProtectedCallReturn>> native_master_protected_call_results;
    class NativeMasterReferenceBankOwner;class NativeMasterDebugSubscriptionRegistry;struct NativeMasterReferenceBankCtorFrame;
    std::unique_ptr<NativeMasterDebugSubscriptionRegistry> native_master_debug_subscriptions;
    std::unique_ptr<NativeMasterReferenceBankOwner> native_master_reference_bank;NativeMasterReferenceBankCtorFrame* native_master_reference_bank_ctor_frame=nullptr;
    class NativeMasterThreadLedgerOwner;struct NativeMasterThreadLedgerCtorFrame;
    std::unique_ptr<NativeMasterThreadLedgerOwner> native_master_thread_ledger;NativeMasterThreadLedgerCtorFrame* native_master_thread_ledger_ctor_frame=nullptr;
    void record_failure(){++failure_epoch;failed=true;ready=false;}
    std::vector<ScriptLuaExecutionRecord> execution_records;
    std::uint64_t execution_serial=0,execution_uncertainty=0;
    // Native provider/check calls, not Lua compilation or chunk execution.
    // Each actual compiler invocation lends a nonescaping stack registration;
    // no callback can install it or retain it through a Lua allocating API.
    struct EndTurnChildPrepareScope;
    EndTurnChildPrepareScope* end_turn_child_prepare=nullptr;
    struct EndTurnSourceListScope;
    EndTurnSourceListScope* end_turn_source_list=nullptr;
    struct EndTurnIncludeCompilerScope;
    EndTurnIncludeCompilerScope* end_turn_compiler_scope=nullptr;
    struct GenericChildIncludeCompilerScope;
    GenericChildIncludeCompilerScope* generic_child_compiler_scope=nullptr;
    struct IncludeProviderScope;
    IncludeProviderScope* include_provider_scope=nullptr;
    struct IncludeProviderScope final {
        Impl& owner;
        lua_State* const issuer;
        lua_State* const vm;
        const char* const context;
        const std::size_t context_size;
        const char* const path;
        const std::size_t path_size;
        const void* const environment;
        const std::uint64_t epoch,uncertainty;
        const bool prior_failed;
        const EndTurnIncludeCompilerScope* const child_compiler;
        const GenericChildIncludeCompilerScope* const generic_compiler;
        IncludeProviderScope(Impl& actual,lua_State* thread,const char* cn,std::size_t nc,
            const char* pn,std::size_t np,const void* env,std::uint64_t fe,
            std::uint64_t ue,bool failed_before):owner(actual),issuer(thread),vm(actual.state),
            context(cn),context_size(nc),path(pn),path_size(np),environment(env),
            epoch(fe),uncertainty(ue),prior_failed(failed_before&&!actual.end_turn_compiler_proves(thread,cn,nc,pn,np,env,fe,ue)&&
                !actual.generic_child_compiler_proves(thread,cn,nc,pn,np,env,fe,ue)),
            child_compiler(actual.end_turn_compiler_scope),generic_compiler(actual.generic_child_compiler_scope) {
            if(std::this_thread::get_id()!=owner.ui_thread)
                throw ScriptError("Include provider lender foreign thread");
            if(owner.include_provider_scope||!owner.calling||prior_failed||!issuer||
               !vm||!context||!path||!environment||owner.failure_epoch!=epoch||
               owner.execution_uncertainty!=uncertainty||!owner.failed||owner.ready||
               owner.source_error||owner.exhausted) {
                owner.record_failure();
                throw ScriptError("Include provider lender has no same compiler operation");
            }
            // owner(thread) authenticates the real Lua allocator/Impl identity.
            if(&Impl::owner(issuer)!=&owner) {
                owner.record_failure();throw ScriptError("Foreign include compiler VM");
            }
            owner.include_provider_scope=this;
        }
        IncludeProviderScope(const IncludeProviderScope&)=delete;
        IncludeProviderScope& operator=(const IncludeProviderScope&)=delete;
        void require_current()const {
            if(std::this_thread::get_id()!=owner.ui_thread)
                throw ScriptError("Include provider stamp foreign thread");
            if(owner.include_provider_scope!=this||owner.state!=vm||!owner.calling||
               prior_failed||owner.failure_epoch!=epoch||owner.execution_uncertainty!=uncertainty||
               !owner.failed||owner.ready||owner.source_error||owner.exhausted||
               !context||!path||!environment||&Impl::owner(issuer)!=&owner||
               (child_compiler&&(child_compiler!=owner.end_turn_compiler_scope||!owner.end_turn_compiler_proves(issuer,context,context_size,path,path_size,environment,epoch,uncertainty)))||
               (generic_compiler&&(generic_compiler!=owner.generic_child_compiler_scope||!owner.generic_child_compiler_proves(issuer,context,context_size,path,path_size,environment,epoch,uncertainty))))
                throw ScriptError("Include provider compiler/source operation retired");
        }
        ~IncludeProviderScope()noexcept {
            // Clear registration before this stack storage goes away, including
            // a native callback exception. No provider or Lua API runs here.
            if(owner.include_provider_scope==this)owner.include_provider_scope=nullptr;
            else owner.record_failure();
        }
    };

    std::size_t execution_begin(ScriptLuaAction action,std::string_view context,
        std::string_view name,std::string_view mode={},std::int64_t count=0) {
        // Record storage and strings are reserved BEFORE the observed action.
        // No owning local crosses a subsequent allocating Lua API.
        if(execution_serial==UINT64_MAX||execution_records.size()>=9007199254740991ULL)
            throw ScriptError("Lua execution journal identity exhausted");
        ScriptLuaExecutionRecord record;
        record.begin=execution_serial+1;record.failure_epoch=failure_epoch;
        record.uncertainty_epoch=execution_uncertainty;record.action=action;
        record.context=context;record.name=name;record.mode=mode;record.count_before=count;
        // Parent is the active context execution, not an arbitrary open event
        // or a strict include whose error a script may have caught itself.
        for(auto i=execution_records.size();i>0;--i)if(execution_records[i-1].outcome==ScriptLuaActionOutcome::open&&
            execution_records[i-1].action==ScriptLuaAction::context_execute)
            {record.parent=execution_records[i-1].begin;break;}
        execution_records.push_back(std::move(record));++execution_serial;
        return execution_records.size()-1;
    }
    void execution_source(std::size_t index,std::string_view source) {
        const auto identity=identify_bytes({reinterpret_cast<const std::uint8_t*>(source.data()),source.size()});
        execution_records.at(index).source_sha256=identity.sha256;
        execution_records[index].source_bytes=identity.bytes;
    }
    void execution_finish(std::size_t index,ScriptLuaActionOutcome outcome,
        std::int64_t count=0,std::int64_t removed=0) noexcept {
        if(index>=execution_records.size()||execution_records[index].outcome!=ScriptLuaActionOutcome::open)return;
        // Abrupt nested Lua exits cannot erase their already reached actions.
        for(auto i=execution_records.size();i>index+1;--i) {
            auto &nested=execution_records[i-1];
            if(nested.outcome==ScriptLuaActionOutcome::open) {
                nested.outcome=ScriptLuaActionOutcome::incomplete;
                if(execution_serial!=UINT64_MAX)nested.end=++execution_serial;
            }
        }
        auto &record=execution_records[index];record.count_after=count;record.removed=removed;
        if(outcome==ScriptLuaActionOutcome::succeeded&&
           (record.failure_epoch!=failure_epoch||record.uncertainty_epoch!=execution_uncertainty))
            outcome=ScriptLuaActionOutcome::uncertain;
        record.outcome=outcome;if(execution_serial!=UINT64_MAX)record.end=++execution_serial;
    }
    void execution_abandon_from(std::size_t first) noexcept {
        for(auto i=execution_records.size();i>first;--i)
            execution_finish(i-1,ScriptLuaActionOutcome::incomplete);
    }
    // Private bootstrap closure only; all C++ failures terminate inside this
    // protected C ABI. No provider, VM getter or public lifecycle grant.
    static int execution_log(lua_State *L) {
        auto &self=owner(L);const auto operation=luaL_checkinteger(L,1);
        std::size_t token=0;
        self.source_exception={};self.source_error=nullptr;
        if(operation==0) {
            const auto action=luaL_checkinteger(L,2);
            if(action<1||action>6)return luaL_error(L,"Invalid execution journal action");
            std::size_t cn=0,nn=0,mn=0;
            const char *context=luaL_checklstring(L,3,&cn),*name=luaL_checklstring(L,4,&nn),*mode=luaL_checklstring(L,5,&mn);
            const auto count=luaL_checkinteger(L,6);
            try {token=self.execution_begin(static_cast<ScriptLuaAction>(action),{context,cn},{name,nn},{mode,mn},count);}
            catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="Cannot retain execution journal action";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
            lua_pushinteger(L,static_cast<lua_Integer>(token+1));return 1;
        }
        if(operation!=1)return luaL_error(L,"Invalid execution journal operation");
        const auto value=luaL_checkinteger(L,2);
        if(value<=0||static_cast<std::uint64_t>(value)>self.execution_records.size())
            return luaL_error(L,"Invalid execution journal token");
        token=static_cast<std::size_t>(value-1);
        const auto count=luaL_checkinteger(L,3),removed=luaL_checkinteger(L,4);
        self.execution_finish(token,lua_toboolean(L,5)?ScriptLuaActionOutcome::error:ScriptLuaActionOutcome::succeeded,count,removed);
        return 0;
    }
    struct NativePostreadGlobalsControl;struct NativePostreadGlobalsFrame;
    NativePostreadGlobalsFrame* postread_globals_frame=nullptr;
    std::vector<std::shared_ptr<NativePostreadGlobalsControl>> postread_globals_controls;
    struct NativePostreadLookupControlReturn;struct NativePostreadLookupCallScope;
    NativePostreadLookupCallScope* postread_lookup_call=nullptr;
    std::vector<std::unique_ptr<NativePostreadLookupControlReturn>> postread_lookup_returns;
    struct NativePostreadRootCDataControl;struct NativePostreadRootCDataFrame;
    struct NativeDirectLuaTailControl;
    NativePostreadRootCDataFrame* postread_root_cdata_frame=nullptr;
    std::vector<std::shared_ptr<NativePostreadRootCDataControl>> postread_root_cdata_controls;
    struct NativePostreadLoadTailControl;struct NativePostreadLoadTailFrame;
    struct NativePostreadLookupControlReturn;
    NativePostreadLoadTailFrame* postread_load_tail_frame=nullptr;
    std::vector<std::shared_ptr<NativePostreadLoadTailControl>> postread_load_tail_controls;
    struct NativePostreadTTManagerReturn;
    std::vector<std::unique_ptr<NativePostreadTTManagerReturn>> native_postread_tt_manager_returns;
    struct NativeContextDefinition;struct NativeContextMetatypeResult;
    std::array<std::shared_ptr<const NativeContextDefinition>,2> native_context_definitions;
    std::vector<std::shared_ptr<NativeContextMetatypeResult>> native_context_metatype_results;
    struct NativeDefinitionDeferredQueue;struct NativeControlBaseConstructor;
    std::shared_ptr<NativeDefinitionDeferredQueue> native_definition_deferred_queue;
    std::shared_ptr<NativeControlBaseConstructor> native_control_base_constructor;
    struct NativeControlBaseMutableLibraryPrefix;
    std::vector<std::shared_ptr<NativeControlBaseMutableLibraryPrefix>> native_control_base_mutable_library_prefixes;
    //SAME Host/VM lifetime: records real entry into the current unsupported
    //canonical hover input route, not a cached hover result. Never reset.
    bool worldview_native_input_entered=false;

    struct NativeControlBaseSetOffsetDecodeCall;
    std::vector<std::unique_ptr<NativeControlBaseSetOffsetDecodeCall>> native_control_base_setoffset_decode_calls;
    struct NativeControlBaseSortCall;
    std::vector<std::unique_ptr<NativeControlBaseSortCall>> native_control_base_sort_calls;
    struct NativeControlBaseTooltipCall;
    std::vector<std::unique_ptr<NativeControlBaseTooltipCall>> native_control_base_tooltip_calls;
    struct NativeControlBaseColorDecodeCall;
    std::vector<std::unique_ptr<NativeControlBaseColorDecodeCall>> native_control_base_color_decode_calls;
    enum class Operation {native_worldview_load,initialize,validate_context,validate_loading,execute,test_all,test_any,call_hook,accumulate_int,accumulate,emit_ui,engine_end_turn_dirty,control,update,map_init_query,team_set_has_tech,native_context_state_prepare,native_context_state_query,native_context_setup_state_name,native_context_setup_state_name_query,native_context_debug_config_prefix,native_context_debug_config_query,native_context_profile_restart,native_context_profile_completed_query,native_context_events_factory_prefix,native_context_lua_events_registration,native_context_api_registration_prefix,native_context_db_registration,native_context_database_namespaces,native_context_ui_enums_registration,native_context_network_registration,native_context_steam_registration,native_context_locale_registration,native_context_content_registration,native_context_modding_registration,native_context_flua_registration_prefix,native_context_options_registration,native_context_ui_registration,native_context_matchmaking_registration,native_context_path_registration,native_context_profiler_registration,native_context_pregame_registration,native_context_game_events_registration,native_context_gamecore_registration,native_context_controls_statement,native_context_controls_table_query,native_context_controls_named_lookup,native_context_entered_controls_named_lookup,native_context_controls_named_retire,native_child_context_controls_export,native_root_ordinary_controls_export,native_context_register_gc,native_context_initial_state_query,native_context_direct_lua_execute,native_context_direct_lua_showhide,native_context_direct_lua_cleanup,native_context_loader_temporary_query,native_context_loader_receiver_unref,native_context_loader_environment_unref,native_context_postread_globals_acquire,native_context_postread_globals_query,native_context_postread_root_cdata_create,native_context_postread_root_cdata_query,native_context_postread_contextptr_acquire,native_context_postread_contextptr_query,native_context_postread_registrations_acquire};
    struct Job {
        Impl *owner;Operation operation;std::string_view context,source,name;
        std::span<const ScriptValue> args;ScriptTestResult result;std::exception_ptr exception;
        std::optional<std::int64_t> integer={};std::optional<double> number={};
        int control_function=LUA_NOREF;
        int control_receiver=LUA_NOREF;
        bool input_result=false,handled=false;
        const ScriptUiContextSourceOwner *map_source_owner=nullptr;
        std::size_t map_slot=std::numeric_limits<std::size_t>::max();
        std::uint64_t map_epoch=0;
        std::int32_t map_world_id=0;
        ScriptMapInitQueryResult map_result;
        std::size_t worldview_child=std::numeric_limits<std::size_t>::max();
        NativePostreadGlobalsControl* context_postread_globals=nullptr;
        NativePostreadRootCDataControl* context_postread_root_cdata=nullptr;
        NativeDirectLuaTailControl* context_direct_lua_tail=nullptr;
        NativePostreadLoadTailControl* context_postread_load_tail=nullptr;
        lua_State* context_state_thread=nullptr;
        const void* context_state_environment=nullptr;
        std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> context_setup_name_loan;
        std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> context_setup_preparation;
        std::string context_state_name;bool context_state_name_matched=false;
        std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> context_profile_state_name;
        std::shared_ptr<const BnwContextLuaProfileConfigLoan> context_profile_config;
        std::int32_t context_profile_debug=0; // Not read before captured genuine config.
        const void* context_profile_debug_table=nullptr;bool context_profile_debug_matched=false;
        const void* context_controls_table=nullptr;
        const void* context_lua_events_table=nullptr;int context_lua_events_reference=LUA_NOREF;
        int context_game_events_reference=LUA_NOREF,context_game_events_factory_reference=LUA_NOREF;
        const void* context_game_events_table=nullptr;const void* context_game_events_factory=nullptr;
        int context_gamecore_reference=LUA_NOREF;const void* context_gamecore_table=nullptr;
        std::size_t context_gamecore_copied=0;
        const void* context_events_table=nullptr;int context_events_reference=LUA_NOREF;
        int context_api_art_info_reference=LUA_NOREF,context_api_art_info_type=LUA_TNONE;
        int context_db_master_reference=LUA_NOREF,context_db_proxy_reference=LUA_NOREF;
        const void* context_db_table=nullptr;const void* context_db_proxy=nullptr;
        std::array<int,2> context_database_master_references{LUA_NOREF,LUA_NOREF},context_database_proxy_references{LUA_NOREF,LUA_NOREF};
        std::array<const void*,2> context_database_masters{},context_database_proxies{};
        int context_ui_enums_reference=LUA_NOREF;const void* context_ui_enums_table=nullptr;std::size_t context_ui_enums_copied=0;
        int context_network_master_reference=LUA_NOREF,context_network_proxy_reference=LUA_NOREF;const void* context_network_table=nullptr;const void* context_network_proxy=nullptr;
        int context_steam_master_reference=LUA_NOREF,context_steam_proxy_reference=LUA_NOREF;const void* context_steam_table=nullptr;const void* context_steam_proxy=nullptr;
        int context_locale_master_reference=LUA_NOREF,context_locale_proxy_reference=LUA_NOREF;const void* context_locale_table=nullptr;const void* context_locale_proxy=nullptr;
        int context_content_master_reference=LUA_NOREF,context_content_proxy_reference=LUA_NOREF;const void* context_content_table=nullptr;const void* context_content_proxy=nullptr;
        int context_content_enums_reference=LUA_NOREF;const void* context_content_enums_table=nullptr;std::size_t context_content_enums_copied=0;
        int context_modding_master_reference=LUA_NOREF,context_modding_proxy_reference=LUA_NOREF;const void* context_modding_table=nullptr;const void* context_modding_proxy=nullptr;
        int context_controls_reference=LUA_NOREF,context_controls_named_reference=LUA_NOREF;
        int context_flua_reference=LUA_NOREF;const void*context_flua_table=nullptr;std::size_t context_flua_copied=0;
        int context_options_master_reference=LUA_NOREF,context_options_proxy_reference=LUA_NOREF;const void*context_options_table=nullptr;const void*context_options_proxy=nullptr;
        int context_ui_master_reference=LUA_NOREF,context_ui_proxy_reference=LUA_NOREF;const void* context_ui_master_table=nullptr;const void* context_ui_proxy_table=nullptr;
        int context_matchmaking_master_reference=LUA_NOREF,context_matchmaking_proxy_reference=LUA_NOREF;const void* context_matchmaking_master_table=nullptr;const void* context_matchmaking_proxy_table=nullptr;
        int context_path_master_reference=LUA_NOREF,context_path_proxy_reference=LUA_NOREF;const void* context_path_table=nullptr;const void* context_path_proxy=nullptr;
        int context_profiler_reference=LUA_NOREF,context_profiler_status=-1;const void* context_profiler_table=nullptr;
        int context_pregame_reference=LUA_NOREF;const void* context_pregame_table=nullptr;std::size_t context_pregame_fields=0;int context_pregame_status=-1;

    };
    static Impl& owner(lua_State *L) {void *p=nullptr;lua_getallocf(L,&p);return *static_cast<Impl*>(p);}
    static void validate_value(const ScriptValue &value) {
        if(const auto *n=std::get_if<std::int64_t>(&value);n&&(*n>9007199254740992LL||*n<-9007199254740992LL))
            throw ScriptError("Integer cannot be represented exactly by Lua 5.1 binary64 numbers");
        if(const auto *n=std::get_if<double>(&value);n&&!std::isfinite(*n))throw ScriptError("Nonfinite Lua value");
    }
    static int row_newindex(lua_State *L){return luaL_error(L,"Database row is read-only from Lua.");}
    static int row_index(lua_State *L){
        const char *name=lua_tostring(L,2);
        lua_pushvalue(L,2);lua_rawget(L,lua_upvalueindex(1));
        if(lua_isnil(L,-1)){
            luaL_where(L,1);const char *where=lua_tostring(L,-1);lua_getglobal(L,"print");
            if(lua_isfunction(L,-1)){lua_pushfstring(L,"%s Cannot find key - %s",where,name);lua_call(L,1,0);}
        }
        lua_pushnil(L);return 1;
    }
    static void validate_row(const ScriptRow &row){
        for(std::size_t i=0;i<row.size();++i){
            bool overwritten=false;
            for(std::size_t j=i+1;j<row.size();++j)overwritten|=row[i].first==row[j].first;
            if(!overwritten)validate_value(row[i].second);
        }
    }
    static void push_row(lua_State *L,const ScriptRow &row){
        lua_newtable(L);lua_newtable(L);lua_newtable(L);
        for(const auto &field:row){lua_pushlstring(L,field.first.data(),field.first.size());lua_pushboolean(L,1);lua_rawset(L,-3);}
        lua_pushcclosure(L,row_index,1);lua_setfield(L,-2,"__index");
        lua_pushcfunction(L,row_newindex);lua_setfield(L,-2,"__newindex");lua_setmetatable(L,-2);
        for(const auto &field:row){lua_pushlstring(L,field.first.data(),field.first.size());push(L,field.second);lua_rawset(L,-3);}
    }
    static int query_gc(lua_State *L){
        auto **cursor=static_cast<ScriptQuery**>(lua_touserdata(L,1));
        delete *cursor;*cursor=nullptr;return 0;
    }
    static int query_next(lua_State *L){
        auto &self=owner(L);auto **cursor=static_cast<ScriptQuery**>(lua_touserdata(L,lua_upvalueindex(1)));
        if(!*cursor){self.failed=true;self.ready=false;return luaL_error(L,"Database query is invalid.");}
        self.source_exception={};self.source_error=nullptr;
        try {
            self.query_row=(*cursor)->next();
            if(!(*cursor)->error().empty()){
                self.failed=true;self.ready=false;
                self.source_errors.push_back({lua_tostring(L,lua_upvalueindex(2)),"DB.Query",std::string((*cursor)->error())});
            }
            if(self.query_row)validate_row(*self.query_row);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Database query step failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!self.query_row){lua_pushnil(L);return 1;}
        const bool prior_failure=self.failed;self.failed=true;
        push_row(L,*self.query_row);self.failed=prior_failure;self.query_row.reset();return 1;
    }
    static int database_query(lua_State *L){
        const int count=lua_gettop(L);const char *sql=luaL_checkstring(L,1);auto &self=owner(L);
        auto **cursor=static_cast<ScriptQuery**>(lua_newuserdata(L,sizeof(ScriptQuery*)));*cursor=nullptr;
        luaL_newmetatable(L,"Civis.SQLCursor");lua_pushcfunction(L,query_gc);lua_setfield(L,-2,"__gc");lua_setmetatable(L,-2);
        // Establish GC ownership before asking a C++ provider to prepare SQL.
        self.source_exception={};self.source_error=nullptr;self.query_arguments.clear();
        try {
            for(int i=2;i<=count;++i){
                switch(lua_type(L,i)){
                case LUA_TNIL:self.query_arguments.emplace_back(std::monostate{});break;
                case LUA_TBOOLEAN:self.query_arguments.emplace_back(lua_toboolean(L,i)!=0);break;
                case LUA_TNUMBER:self.query_arguments.emplace_back(static_cast<double>(lua_tonumber(L,i)));break;
                case LUA_TSTRING:self.query_arguments.emplace_back(std::string(lua_tostring(L,i)));break;
                default:throw ScriptError("Invalid argument for database query.");
                }
            }
            if(lua_isuserdata(L,lua_upvalueindex(2))){
                auto *connection=*static_cast<ScriptSaveDataConnection**>(lua_touserdata(L,lua_upvalueindex(2)));
                if(!connection)throw ScriptError("Saved database connection is closed");
                *cursor=connection->query(sql,self.query_arguments).release();
            }else *cursor=self.gameplay.query(sql,self.query_arguments).release();
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Database query preparation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        self.query_arguments.clear();
        lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,query_next,2);return 1;
    }
    static int make_database_query(lua_State *L){luaL_checkstring(L,1);lua_pushvalue(L,1);lua_pushcclosure(L,database_query,1);return 1;}
    static int saved_gc(lua_State *L){
        auto **connection=static_cast<ScriptSaveDataConnection**>(lua_touserdata(L,lua_upvalueindex(1)));
        delete *connection;*connection=nullptr;return 0;
    }
    static int saved_value_access(lua_State *L){
        const char *key=luaL_checkstring(L,1);const int count=lua_gettop(L);
        if(count>2)return luaL_error(L,"SetValue takes 2 arguments; Name and Value");
        const bool write=lua_toboolean(L,lua_upvalueindex(2))!=0;
        // Retail pushes its default false result before inspecting argument 2.
        // Consequently SetValue(name) binds false; explicit nil binds NULL.
        if(write&&count<2)lua_pushboolean(L,0);
        const int kind=lua_type(L,2);
        if(write&&kind!=LUA_TNIL&&kind!=LUA_TBOOLEAN&&kind!=LUA_TNUMBER&&kind!=LUA_TSTRING)
            return luaL_argerror(L,2,"Must be of type boolean, nil, number or string");
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;self.saved_value={};
        auto *connection=*static_cast<ScriptSaveDataConnection**>(lua_touserdata(L,lua_upvalueindex(1)));
        if(!connection)return luaL_error(L,"Saved database connection is closed");
        bool success=false;
        try {
            self.query_arguments={std::string(key)};
            if(write){
                switch(kind){
                case LUA_TNIL:self.query_arguments.emplace_back(std::monostate{});break;
                case LUA_TBOOLEAN:self.query_arguments.emplace_back(lua_toboolean(L,2)!=0);break;
                case LUA_TNUMBER:self.query_arguments.emplace_back(static_cast<double>(lua_tonumber(L,2)));break;
                case LUA_TSTRING:self.query_arguments.emplace_back(std::string(lua_tostring(L,2)));break;
                }
            }
            auto cursor=connection->query(write?"INSERT OR REPLACE Into SimpleValues(Name, Value) VALUES(?,?)":
                "SELECT Value from SimpleValues where Name = ?",self.query_arguments);
            if(cursor){
                if(cursor->step()&&!write){const auto row=cursor->row();if(!row.empty())self.saved_value=row.front().second;}
                success=cursor->error().empty();
            }
            validate_value(self.saved_value);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Saved database value operation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        self.query_arguments.clear();
        if(write)lua_pushboolean(L,success);else push(L,self.saved_value);
        self.saved_value={};return 1;
    }
    static int open_save_data(lua_State *L){
        luaL_checkstring(L,1);auto &self=owner(L);
        const bool user=lua_gettop(L)==3;
        const char *user_name=user?luaL_checkstring(L,2):nullptr;
        const auto user_version=user?source_integer(L,3):std::int64_t{0};
        auto **connection=static_cast<ScriptSaveDataConnection**>(lua_newuserdata(L,sizeof(ScriptSaveDataConnection*)));
        *connection=nullptr;
        lua_newtable(L);lua_pushvalue(L,-2);lua_pushcclosure(L,saved_gc,1);lua_setfield(L,-2,"__gc");
        lua_newtable(L);
        for(const auto *name:{"GetValue","SetValue"}){
            lua_pushvalue(L,-3);lua_pushboolean(L,name[0]=='S');lua_pushcclosure(L,saved_value_access,2);lua_setfield(L,-2,name);
        }
        lua_pushvalue(L,1);lua_pushvalue(L,-4);lua_pushcclosure(L,database_query,2);lua_setfield(L,-2,"Query");
        lua_pushvalue(L,-1);lua_setfield(L,-3,"__newindex");lua_setfield(L,-2,"__index");lua_setmetatable(L,-2);
        self.source_exception={};self.source_error=nullptr;
        try {*connection=(user?self.user_data.open(user_name,user_version):self.save_data.open()).release();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Cannot open Lua data database";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return *connection?1:0;
    }
    static int delete_user_data(lua_State *L){
        luaL_checkstring(L,1);
        const char *name=luaL_checkstring(L,2);
        const auto version=source_integer(L,3);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.user_data.erase(name,version);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Cannot delete mod user data";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int configure_enums(lua_State *L){
        luaL_checktype(L,1,LUA_TTABLE);
        const auto &tables=owner(L).enums.tables;
        for(std::size_t ordinal=0;ordinal<tables.size();++ordinal){
            const auto&table=tables[ordinal];
            if(worldview_configured_enum_preserve_owned(L,1,table.name.c_str()))continue;
            lua_newtable(L);
            for(const auto &[name,value]:table.members){
                lua_pushinteger(L,value);
                lua_setfield(L,-2,name.c_str());
            }
            lua_setfield(L,1,table.name.c_str());
            worldview_capture_configured_enum_publication_owned(L,1,ordinal);
        }
        return 0;
    }
    static int locale_current(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;self.locale_language={};
        try {self.locale_language=self.locale.current();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Current language query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_newtable(L);
        lua_pushlstring(L,self.locale_language.type.data(),self.locale_language.type.size());lua_setfield(L,-2,"Type");
        lua_pushlstring(L,self.locale_language.display_name.data(),self.locale_language.display_name.size());
        lua_setfield(L,-2,"DisplayName");
        self.locale_language={};return 1;
    }
    static int locale_select(lua_State *L){
        std::size_t size=0;const char *name=luaL_checklstring(L,1,&size);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool selected=false;
        try {selected=self.locale.select(std::string_view(name,size));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Language selection failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,selected);return 1;
    }
    static int locale_convert(lua_State *L){
        std::size_t key_size=0;const char *key=luaL_checklstring(L,1,&key_size);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        self.locale_arguments.clear();self.locale_text.clear();
        try {
            for(int i=2;i<=lua_gettop(L);++i){
                switch(lua_type(L,i)){
                case LUA_TNIL:self.locale_arguments.emplace_back(std::monostate{});break;
                case LUA_TBOOLEAN:self.locale_arguments.emplace_back(lua_toboolean(L,i)!=0);break;
                case LUA_TNUMBER:self.locale_arguments.emplace_back(static_cast<double>(lua_tonumber(L,i)));break;
                case LUA_TSTRING:self.locale_arguments.emplace_back(std::string(lua_tostring(L,i)));break;
                default:throw ScriptError("Unsupported Locale.ConvertTextKey argument type");
                }
            }
            self.locale_text=self.locale.convert_text_key(std::string_view(key,key_size),self.locale_arguments);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Localized text lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushlstring(L,self.locale_text.data(),self.locale_text.size());
        self.locale_arguments.clear();self.locale_text.clear();return 1;
    }
    // Internal prepared-cursor operations for GameInfo. The closure owns its
    // GC userdata; none of these operations are exposed as a global Lua API.
    static int info_query_operation(lua_State *L){
        static const char *const operations[]={"valid","step","row","id","type","reset","bind","release",nullptr};
        const int operation=luaL_checkoption(L,1,nullptr,operations);auto &self=owner(L);
        auto **cursor=static_cast<ScriptQuery**>(lua_touserdata(L,lua_upvalueindex(1)));
        if(operation==0){lua_pushboolean(L,*cursor!=nullptr);return 1;}
        if(!*cursor){self.failed=true;self.ready=false;return luaL_error(L,"Database query is invalid.");}
        self.source_exception={};self.source_error=nullptr;bool stepped=false;
        // Argument conversion occurs before provider calls; all owned scratch
        // survives any Lua allocation failure or conversion error.
        if(operation==6){
            luaL_checktype(L,2,LUA_TTABLE);
            const auto count=source_integer(L,3);
            if(count<0)return luaL_error(L,"Negative query binding count");
            self.query_arguments.clear();
            try {
                for(std::int64_t i=1;i<=count;++i){
                    lua_pushinteger(L,i);lua_rawget(L,2);
                    switch(lua_type(L,-1)){
                    case LUA_TNIL:self.query_arguments.emplace_back(std::monostate{});break;
                    case LUA_TBOOLEAN:case LUA_TNUMBER:self.query_arguments.emplace_back(source_integer(L,-1));break;
                    case LUA_TSTRING:self.query_arguments.emplace_back(std::string(lua_tostring(L,-1)));break;
                    default:throw ScriptError("Invalid GameInfo query parameter");
                    }
                    lua_pop(L,1);
                }
            }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="GameInfo argument conversion failed";}
        }
        if(!self.source_error)try {
            switch(operation){
            case 1:
                stepped=(*cursor)->step();
                if(!(*cursor)->error().empty()){
                    self.failed=true;self.ready=false;
                    self.source_errors.push_back({lua_tostring(L,lua_upvalueindex(2)),"GameInfo",std::string((*cursor)->error())});
                }
                break;
            case 2:self.query_row=(*cursor)->row();validate_row(*self.query_row);break;
            case 3:case 4:self.query_key=(*cursor)->cache_key(operation==3);validate_value(self.query_key.second);break;
            case 5:(*cursor)->reset();break;
            case 6:(*cursor)->bind(self.query_arguments);self.query_arguments.clear();break;
            case 7:(*cursor)->release();break;
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="GameInfo query operation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const bool prior_failure=self.failed;self.failed=true;
        if(operation==1)lua_pushboolean(L,stepped);
        else if(operation==2){push_row(L,*self.query_row);self.query_row.reset();}
        else if(operation==3||operation==4){lua_pushboolean(L,self.query_key.first);push(L,self.query_key.second);self.query_key={};}
        self.failed=prior_failure;
        return operation==3||operation==4?2:operation==1||operation==2?1:0;
    }
    static int make_info_query(lua_State *L){
        luaL_checkstring(L,1);const char *sql=luaL_checkstring(L,2);auto &self=owner(L);
        auto **cursor=static_cast<ScriptQuery**>(lua_newuserdata(L,sizeof(ScriptQuery*)));*cursor=nullptr;
        luaL_newmetatable(L,"Civis.SQLCursor");lua_pushcfunction(L,query_gc);lua_setfield(L,-2,"__gc");lua_setmetatable(L,-2);
        self.source_exception={};self.source_error=nullptr;
        try {*cursor=self.gameplay.query(sql,{}).release();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="GameInfo query preparation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushvalue(L,1);lua_pushcclosure(L,info_query_operation,2);return 1;
    }
    static int make_info_object(lua_State *L){
        luaL_checktype(L,1,LUA_TTABLE);lua_newuserdata(L,1);lua_pushvalue(L,1);lua_setmetatable(L,-2);return 1;
    }
    static int gameplay_info_types(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.type_entries=self.gameplay.info_types();for(const auto &entry:self.type_entries)validate_value(entry.second);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="GameInfoTypes snapshot failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const auto* const actual_loader=self.end_turn_child_loader;
        const auto* const actual_native=self.worldview_native_host;
        const bool loader_sentinel=actual_loader&&actual_native;
        if(loader_sentinel){
            try {
                self.worldview_require_callback_origin();
                if(self.end_turn_child_loader!=actual_loader||self.worldview_native_host!=actual_native||
                   actual_loader->entry!=L||!actual_loader->current())
                    throw ScriptError("Gameplay allocation lacks SAME current native child-loader sentinel");
            }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="Gameplay native child-loader allocation preflight failed";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        }
        const bool prior_failure=self.failed;
        if(loader_sentinel)self.failed.set_loader_sentinel();else self.failed=true;lua_newtable(L);
        for(const auto &[name,id]:self.type_entries){lua_pushinteger(L,id);lua_setfield(L,-2,name.c_str());}
        if(loader_sentinel){
            try {
                self.worldview_require_callback_origin();
                if(self.end_turn_child_loader!=actual_loader||self.worldview_native_host!=actual_native||
                   actual_loader->entry!=L||!actual_loader->current())
                    throw ScriptError("Gameplay allocation lost SAME current native child-loader sentinel");
            }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="Gameplay native child-loader allocation postflight failed";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        }
        if(loader_sentinel&&prior_failure)self.failed.set_loader_sentinel();else self.failed=prior_failure;self.type_entries.clear();return 1;
    }
    static const char *object_type(bool team){return team?"Civis.TeamReference":"Civis.PlayerReference";}
    static std::int64_t object_id(lua_State *L,bool team){
        luaL_checktype(L,1,LUA_TTABLE);lua_getfield(L,1,"__instance");
        const auto id=*static_cast<std::int64_t*>(luaL_checkudata(L,-1,object_type(team)));lua_pop(L,1);return id;
    }
    struct UnitSlot {Impl* host;std::size_t slot;};
    static int unit_read(lua_State *L){
        if(owner(L).worldview_native_host)return worldview_unit_reference_owned(L,false);
        auto &self=owner(L);
        luaL_checktype(L,1,LUA_TTABLE);lua_pushliteral(L,"__instance");lua_rawget(L,1);
        if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(UnitSlot))
            return luaL_error(L,"Unit retained reference storage differs");
        const auto *slot=static_cast<const UnitSlot*>(luaL_checkudata(L,-1,"Civis.UnitReadReference"));lua_pop(L,1);
        const auto method=lua_tostring(L,lua_upvalueindex(1));
        self.source_exception={};self.source_error=nullptr;
        try{
            if(!slot||slot->host!=&self||slot->slot>=self.unit_references.size()||!self.gameplay.units)
                throw ScriptError("Foreign retained unit reference");
            self.gameplay.units->require_current();
            self.unit_references[slot->slot]->require_current();
            self.unit_value=self.unit_references[slot->slot]->read(method);
            self.unit_references[slot->slot]->require_current();
            self.gameplay.units->require_current();
        }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Actual unit read failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const bool failed=self.failed;self.failed=true;push(L,self.unit_value);self.failed=failed;return 1;
    }
    static int player_get_unit(lua_State *L){
        if(owner(L).worldview_native_host)return worldview_unit_reference_owned(L,true);
        const auto player=object_id(L,false),id=source_integer(L,2);auto &self=owner(L);
        self.source_exception={};self.source_error=nullptr;std::size_t slot=0;bool found=false;
        try{
            if(!self.gameplay.units)throw ScriptError("Actual unit read owner unavailable");
            self.gameplay.units->require_current();
            // Transfer ownership to Impl before entering any allocating Lua API.
            self.unit_read_scratch=self.gameplay.units->find(player,id);
            found=static_cast<bool>(self.unit_read_scratch);
            if(found){
                self.unit_read_scratch->require_current();
                if(self.unit_references.size()>=unit_reference_limit)throw ScriptError("Retained actual unit reference budget exhausted");
                self.unit_references.push_back(std::move(self.unit_read_scratch));slot=self.unit_references.size()-1;
            }
            self.gameplay.units->require_current();
        }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Actual unit lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!found){lua_pushnil(L);return 1;}
        const bool failed=self.failed;self.failed=true;
        lua_newtable(L);
        auto *reference=static_cast<UnitSlot*>(lua_newuserdata(L,sizeof(UnitSlot)));*reference={&self,slot};
        luaL_newmetatable(L,"Civis.UnitReadReference");lua_setmetatable(L,-2);lua_setfield(L,-2,"__instance");
        for(const auto method:{"GetID","GetOwner","GetX","GetY","MovesLeft","GetDamage","GetMaxHitPoints","IsDead","IsCargo","GetCurrHitPoints","IsCombatUnit","IsEmbarked","CargoSpace","GetUnitType","GetDomainType","IsTrade"}){
            lua_pushstring(L,method);lua_pushcclosure(L,unit_read,1);lua_setfield(L,-2,method);
        }
        self.failed=failed;return 1;
    }
    static int player_unit_ids(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::size_t slot=0;const bool captured=lua_isnoneornil(L,2);
        if(!captured){
            if(lua_type(L,2)!=LUA_TUSERDATA||lua_objlen(L,2)!=sizeof(UnitSlot)){
                self.record_failure();return luaL_error(L,"Unit roster quote storage differs");
            }
            const auto* token=static_cast<const UnitSlot*>(luaL_checkudata(L,2,"Civis.UnitRosterQuote"));
            if(token->host!=&self||token->slot>=self.unit_rosters.size()){
                self.record_failure();return luaL_error(L,"Foreign actual unit roster quote");
            }
            slot=token->slot;
        }
        try{
            if(!self.gameplay.units)throw ScriptError("Actual unit roster unavailable");
            self.gameplay.units->require_current();
            if(captured){
                if(self.unit_rosters.size()>=4096)throw ScriptError("Retained actual unit roster budget exhausted");
                self.unit_roster_scratch=self.gameplay.units->capture_roster(player);
                if(!self.unit_roster_scratch)throw ScriptError("Actual native unit roster quote absent");
                self.unit_roster_scratch->require_current(player);self.unit_ids=self.unit_roster_scratch->ids();
                if(self.unit_ids.size()>unit_reference_limit-self.unit_roster_rows)throw ScriptError("Retained actual unit roster row budget exhausted");
                self.unit_rosters.push_back(std::move(self.unit_roster_scratch));slot=self.unit_rosters.size()-1;
                self.unit_roster_rows+=self.unit_ids.size();
            }else{
                self.unit_rosters[slot]->require_current(player);self.unit_ids=self.unit_rosters[slot]->ids();
            }
            std::set<std::int64_t> seen;
            for(const auto id:self.unit_ids)if(id<0||id>INT32_MAX||!seen.insert(id).second)throw ScriptError("Actual full unit roster differs");
            self.unit_rosters[slot]->require_current(player);self.gameplay.units->require_current();
        }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Actual unit roster failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const bool failed=self.failed;self.failed=true;lua_newtable(L);
        for(std::size_t i=0;i<self.unit_ids.size();++i){lua_pushinteger(L,self.unit_ids[i]);lua_rawseti(L,-2,static_cast<int>(i+1));}
        if(captured){
            auto* reference=static_cast<UnitSlot*>(lua_newuserdata(L,sizeof(UnitSlot)));*reference={&self,slot};
            luaL_newmetatable(L,"Civis.UnitRosterQuote");lua_setmetatable(L,-2);
        }else lua_pushvalue(L,2);
        self.failed=failed;self.unit_ids.clear();return 2;
    }

    static int gameplay_objects(lua_State *L){
        const bool team=lua_toboolean(L,1)!=0;auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {
            self.object_ids=self.gameplay.ids(team);std::set<std::int64_t> seen;
            for(const auto id:self.object_ids){validate_value(id);if(id<0||!seen.insert(id).second)throw ScriptError("Invalid or duplicate Lua object ID");}
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua object enumeration failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const auto* const actual_loader=self.end_turn_child_loader;
        const auto* const actual_native=self.worldview_native_host;
        const bool loader_sentinel=actual_loader&&actual_native;
        if(loader_sentinel){
            try {
                self.worldview_require_callback_origin();
                if(self.end_turn_child_loader!=actual_loader||self.worldview_native_host!=actual_native||
                   actual_loader->entry!=L||!actual_loader->current())
                    throw ScriptError("Gameplay allocation lacks SAME current native child-loader sentinel");
            }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="Gameplay native child-loader allocation preflight failed";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        }
        const bool prior_failure=self.failed;
        if(loader_sentinel)self.failed.set_loader_sentinel();else self.failed=true;
        lua_newtable(L);
        for(const auto id:self.object_ids){
            lua_pushinteger(L,id);lua_newtable(L);
            auto *reference=static_cast<std::int64_t*>(lua_newuserdata(L,sizeof(std::int64_t)));*reference=id;
            luaL_newmetatable(L,object_type(team));lua_setmetatable(L,-2);lua_setfield(L,-2,"__instance");lua_rawset(L,-3);
        }
        if(loader_sentinel){
            try {
                self.worldview_require_callback_origin();
                if(self.end_turn_child_loader!=actual_loader||self.worldview_native_host!=actual_native||
                   actual_loader->entry!=L||!actual_loader->current())
                    throw ScriptError("Gameplay allocation lost SAME current native child-loader sentinel");
            }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="Gameplay native child-loader allocation postflight failed";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        }
        if(loader_sentinel&&prior_failure)self.failed.set_loader_sentinel();else self.failed=prior_failure;self.object_ids.clear();return 1;
    }

    // Private native callback scope. Public call()/calling reentry remains intact.
    std::shared_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> owned_team_tech;
    bool team_tech_inflight=false;
    bool production_test_inflight=false;
    std::shared_ptr<const ui::BnwTechAwardContextOwner> tech_award_context;
    std::vector<std::pair<std::string,std::string>> tech_award_native_controls;
    std::string tech_award_locale_result;
    std::int32_t tech_award_native_popup_type=0,tech_award_native_priority=0;
    static int tech_award_locale(lua_State* L){
        auto& self=owner(L);const int count=lua_gettop(L);
        if(count<1||count>65)return luaL_error(L,"TECH_AWARD Locale parameter count unsupported");
        std::size_t tag_length=0;const char* tag=luaL_checklstring(L,1,&tag_length);
        // All erroring Lua argument checks precede C++ owner/value lifetimes.
        for(int n=2;n<=count;++n){
            const int type=lua_type(L,n);
            if(type==LUA_TNUMBER)(void)source_integer(L,n);
            else if(type!=LUA_TSTRING)return luaL_error(L,"TECH_AWARD Locale requires native string/integer arguments");
        }
        self.source_exception={};self.source_error=nullptr;
        try{
            if(!self.tech_award_context)throw ScriptError("TECH_AWARD actual Locale context absent");
            const auto held=self.tech_award_context->retain_canonical(self.tech_award_context);held->require_current();
            std::vector<ScriptValue> values;values.reserve(static_cast<std::size_t>(count-1));
            for(int n=2;n<=count;++n){
                if(lua_type(L,n)==LUA_TNUMBER)values.emplace_back(static_cast<std::int64_t>(lua_tointeger(L,n)));
                else{std::size_t size=0;const char* text=lua_tolstring(L,n,&size);values.emplace_back(std::string(text,size));}
            }
            self.tech_award_locale_result=held->convert_text_key(std::string_view(tag,tag_length),values);
            held->require_current();
        }catch(const std::exception& e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="TECH_AWARD genuine Locale query failed";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        // Result storage belongs to the retained VM across allocating Lua APIs.
        lua_pushlstring(L,self.tech_award_locale_result.data(),self.tech_award_locale_result.size());return 1;
    }
    // Only this exact context's closed native owner serves these methods.
    int tech_award_continue=LUA_NOREF,tech_award_close=LUA_NOREF;
    int tech_award_input=LUA_NOREF,tech_award_showhide=LUA_NOREF;
    int tech_award_context_table=LUA_NOREF,tech_award_popup_info=LUA_NOREF;
    std::int32_t tech_award_native_left_click=0;
    static int tech_award_register(lua_State* L){
        auto& self=owner(L);const int op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(1)));
        const char* id=lua_tostring(L,lua_upvalueindex(2));
        if(!lua_rawequal(L,1,lua_upvalueindex(3)))return luaL_error(L,"TECH_AWARD registration receiver substituted");
        const int fn=op==17?3:2;std::int64_t mouse=0;
        if(op==17)mouse=source_integer(L,2);
        if(!lua_isfunction(L,fn)||lua_iscfunction(L,fn))return luaL_error(L,"TECH_AWARD original Lua handler required");
        int* slot=nullptr;
        if(op==18)slot=&self.tech_award_input;
        else if(op==19)slot=&self.tech_award_showhide;
        else if(op==17&&std::string_view(id)=="ContinueButton")slot=&self.tech_award_continue;
        else if(op==17&&std::string_view(id)=="CloseButton")slot=&self.tech_award_close;
        if(!slot||*slot!=LUA_NOREF)return luaL_error(L,"TECH_AWARD repeated/unknown original registration");
        // The protected stock context owns this registry closure. Any later
        // native slot failure quarantines the VM; a ref is never a readiness token.
        lua_pushvalue(L,fn);*slot=luaL_ref(L,LUA_REGISTRYINDEX);
        self.source_exception={};self.source_error=nullptr;
        try{
            const auto held=self.tech_award_context->retain_canonical(self.tech_award_context);
            held->require_current();
            if(op==17)held->register_click(id,mouse);
            else if(op==18)held->register_input_handler();
            else held->register_showhide_handler();
            held->require_current();
        }catch(const std::exception& e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="TECH_AWARD actual original registration failed";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}return 0;
    }
    static void tech_award_registration(lua_State* L,int table,const char* name,int op,const char* id=""){
        lua_pushinteger(L,op);lua_pushstring(L,id);lua_pushvalue(L,table);
        lua_pushcclosure(L,tech_award_register,3);lua_setfield(L,table,name);
    }
    static void check_tech_award_original_registration(lua_State* L,int env,const char* name,int reference){
        if(reference==LUA_NOREF)throw ScriptError("TECH_AWARD original native registration absent");
        lua_getfield(L,env,name);lua_rawgeti(L,LUA_REGISTRYINDEX,reference);
        const bool same=lua_rawequal(L,-1,-2)!=0;lua_pop(L,2);
        if(!same)throw ScriptError("TECH_AWARD registered original Lua closure substituted");
    }
    static int tech_award_native(lua_State* L){
        auto& self=owner(L);const int op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(1)));
        const char* id=lua_tostring(L,lua_upvalueindex(2));
        if(((op>=2&&op<=12)||op==17)&&!lua_rawequal(L,1,lua_upvalueindex(3)))return luaL_error(L,"TECH_AWARD native receiver substituted");
        std::size_t length=0;const char* text=nullptr;std::int64_t x=0,y=0;bool hidden=false;
        // All potentially longjmp argument checks precede C++ owner pins.
        if(op>=2&&op<=4)text=luaL_checklstring(L,2,&length);
        if(op==5){x=source_integer(L,2);y=source_integer(L,3);}
        if(op==6){luaL_checktype(L,2,LUA_TBOOLEAN);hidden=lua_toboolean(L,2)!=0;}
        if(op==16){x=source_integer(L,1);y=source_integer(L,2);}
        if(op==10||op==11){
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_context_table);
            const bool same=lua_rawequal(L,2,-1)!=0;lua_pop(L,1);
            if(!same)return luaL_error(L,"TECH_AWARD Queue/Dequeue ContextPtr substituted");
            if(op==10&&source_integer(L,3)!=self.tech_award_native_priority)
                return luaL_error(L,"TECH_AWARD actual native popup priority differs");
        }
        if(op==15){
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_popup_info);
            const bool same=lua_rawequal(L,1,-1)!=0;lua_pop(L,1);
            if(!same)return luaL_error(L,"TECH_AWARD PopupShown source table substituted");
        }
        std::int32_t scalar=0;bool result=false,showhide=false,is_hide=false;self.source_exception={};self.source_error=nullptr;
        try{
            if(!self.tech_award_context)throw ScriptError("TECH_AWARD actual native context absent");
            const auto held=self.tech_award_context->retain_canonical(self.tech_award_context);held->require_current();
            switch(op){
             case 0:scalar=held->active_player();break;
             case 1:scalar=held->active_team();break;
             case 2:held->set_text(id,std::string_view(text,length));break;
             case 3:held->set_tooltip(id,std::string_view(text,length));break;
             case 4:held->set_texture(id,std::string_view(text,length));break;
             case 5:held->set_texture_offset(id,x,y);break;
             case 6:held->set_hide(id,hidden);break;
             case 7:held->calculate_bonus_stack();break;
             case 8:held->reprocess_bonus_stack();break;
             case 9:held->auto_size();break;
             case 10:held->queue_popup();showhide=held->requires_showhide();break;
             case 11:held->dequeue_popup();showhide=held->requires_showhide();is_hide=true;break;
             case 12:result=held->hidden();break;
             case 13:held->increment_timer_semaphore();break;
             case 14:held->decrement_timer_semaphore();break;
             case 15:held->popup_shown();break;
             case 16:held->popup_processed(x,y);break;
             default:throw ScriptError("TECH_AWARD native click/input/showhide registration ingress unavailable");
            }
            held->require_current();
        }catch(const std::exception& e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="TECH_AWARD actual context effect failed";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        if(showhide){
            // No automatic shared owner or C++ exception object crosses Lua's
            // allocating protected caller. Native QueuePopup synchronously
            // runs the retained original handler, exactly before returning.
            if(self.tech_award_showhide==LUA_NOREF)return luaL_error(L,"TECH_AWARD original ShowHide registration absent");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_showhide);
            lua_pushboolean(L,is_hide);lua_pushboolean(L,0);lua_call(L,2,0);
            try{
                const auto held=self.tech_award_context->retain_canonical(self.tech_award_context);
                held->require_current();held->finish_actual_showhide();held->require_current();
            }catch(const std::exception& e){self.source_exception=std::current_exception();self.source_error=e.what();}
            catch(...){self.source_exception=std::current_exception();self.source_error="TECH_AWARD actual ShowHide completion failed";}
            if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        }
        if(op<=1){lua_pushinteger(L,scalar);return 1;}
        if(op==12){lua_pushboolean(L,result);return 1;}return 0;
    }
    static void tech_award_method(lua_State* L,int table,const char* name,int op,const char* id=""){
        lua_pushinteger(L,op);lua_pushstring(L,id);lua_pushvalue(L,table);
        lua_pushcclosure(L,tech_award_native,3);lua_setfield(L,table,name);
    }
    static void install_tech_award_context(lua_State* L,int env){
        auto& self=owner(L);
        if(!self.tech_award_context)return; // actual script will refuse missing mandatory APIs
        // Selected native constants come from the genuine sealed owner and its
        // exact retail/SDK proofs; neither value grants modal or paint readiness.
        lua_newtable(L);lua_pushinteger(L,self.tech_award_native_popup_type);lua_setfield(L,-2,"BUTTONPOPUP_TECH_AWARD");lua_setfield(L,env,"ButtonPopupTypes");
        lua_newtable(L);lua_pushinteger(L,self.tech_award_native_priority);lua_setfield(L,-2,"TechAward");lua_setfield(L,env,"PopupPriority");
        lua_newtable(L);lua_pushinteger(L,self.tech_award_native_left_click);lua_setfield(L,-2,"eLClick");lua_setfield(L,env,"Mouse");
        // Context-local closures use actual selected localization/rules and
        // canonical language scope. Other Locale grammar APIs remain absent.
        lua_newtable(L);lua_pushcfunction(L,tech_award_locale);lua_setfield(L,-2,"ConvertTextKey");
        lua_pushcfunction(L,tech_award_locale);lua_setfield(L,-2,"Lookup");lua_setfield(L,env,"Locale");
        // Descriptor storage is in the retained VM, not a C++ automatic object
        // whose destructor could be bypassed by allocating Lua APIs.
        lua_createtable(L,0,static_cast<int>(self.tech_award_native_controls.size()));const int list=lua_gettop(L);
        // Source control identities come from the selected original XML.
        for(const auto& [id,kind]:self.tech_award_native_controls){
            lua_createtable(L,0,11);const int control=lua_gettop(L);
            lua_pushlstring(L,id.data(),id.size());lua_setfield(L,control,"id");
            lua_pushlstring(L,kind.data(),kind.size());lua_setfield(L,control,"kind");
            tech_award_method(L,control,"SetText",2,id.c_str());tech_award_method(L,control,"SetToolTipString",3,id.c_str());
            tech_award_method(L,control,"SetTexture",4,id.c_str());tech_award_method(L,control,"SetTextureOffsetVal",5,id.c_str());
            tech_award_method(L,control,"SetHide",6,id.c_str());
            if(id=="BonusStack"){tech_award_method(L,control,"CalculateSize",7,id.c_str());tech_award_method(L,control,"ReprocessAnchoring",8,id.c_str());}
            if(id=="OuterGrid")tech_award_method(L,control,"DoAutoSize",9,id.c_str());
            // Actual owner slot and original VM closure are retained together.
            tech_award_registration(L,control,"RegisterCallback",17,id.c_str());lua_setfield(L,list,id.c_str());
        }
        lua_setfield(L,env,"Controls");
        lua_getfield(L,env,"Game");if(!lua_istable(L,-1)){lua_pop(L,1);lua_newtable(L);}
        tech_award_method(L,lua_gettop(L),"GetActivePlayer",0);tech_award_method(L,lua_gettop(L),"GetActiveTeam",1);lua_setfield(L,env,"Game");
        lua_newtable(L);const int context=lua_gettop(L);
        lua_pushvalue(L,context);self.tech_award_context_table=luaL_ref(L,LUA_REGISTRYINDEX);
        tech_award_method(L,context,"IsHidden",12);tech_award_registration(L,context,"SetInputHandler",18);
        tech_award_registration(L,context,"SetShowHideHandler",19);lua_setfield(L,env,"ContextPtr");
        lua_newtable(L);const int manager=lua_gettop(L);
        tech_award_method(L,manager,"QueuePopup",10);tech_award_method(L,manager,"DequeuePopup",11);lua_setfield(L,env,"UIManager");
        lua_getfield(L,env,"UI");if(!lua_istable(L,-1)){lua_pop(L,1);lua_newtable(L);}
        // These are dot calls in the stock script, unlike control colon calls.
        lua_pushinteger(L,13);lua_pushliteral(L,"");lua_pushvalue(L,lua_gettop(L)-2);lua_pushcclosure(L,tech_award_native,3);lua_setfield(L,-2,"incTurnTimerSemaphore");
        lua_pushinteger(L,14);lua_pushliteral(L,"");lua_pushvalue(L,lua_gettop(L)-2);lua_pushcclosure(L,tech_award_native,3);lua_setfield(L,-2,"decTurnTimerSemaphore");lua_setfield(L,env,"UI");
        // Only the exact native popup lifecycle members are context-local;
        // other event registrations still use the genuine VM event registry.
        lua_getfield(L,env,"Events");const int base_events=lua_gettop(L);
        lua_newtable(L);const int events=lua_gettop(L);lua_newtable(L);
        lua_pushvalue(L,base_events);lua_setfield(L,-2,"__index");lua_setmetatable(L,events);
        tech_award_method(L,events,"SerialEventGameMessagePopupShown",15);
        lua_newtable(L);tech_award_method(L,lua_gettop(L),"CallImmediate",16);
        lua_setfield(L,events,"SerialEventGameMessagePopupProcessed");lua_setfield(L,env,"Events");lua_pop(L,1);
    }
    bool tech_award_inflight=false;
    int tech_award_popup=LUA_NOREF,tech_award_refresh=LUA_NOREF,tech_award_queue=LUA_NOREF;
    struct TechAwardJob {
        Impl* owner;std::string_view source;bool activate;
        std::int64_t data1=0,data2=0,data3=0;std::int32_t flags=0;bool option1=false,option2=false;
        std::exception_ptr exception;
    };
    static int selected_tech_award_job(lua_State* L){
        auto& job=*static_cast<TechAwardJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,24))return luaL_error(L,"TECH_AWARD protected stack unavailable");
            if(job.activate)execute_context(L,"TechAwardPopup",job.source,"@Assets/UI/InGame/Popups/TechAwardPopup.lua");
            lua_settop(L,1);
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"create");
            lua_pushliteral(L,"TechAwardPopup");lua_pushnil(L);lua_call(L,2,1);
            const int env=lua_gettop(L);
            check_tech_award_original_registration(L,env,"OnContinueButtonClicked",self.tech_award_continue);
            check_tech_award_original_registration(L,env,"OnClose",self.tech_award_close);
            check_tech_award_original_registration(L,env,"InputHandler",self.tech_award_input);
            check_tech_award_original_registration(L,env,"ShowHideHandler",self.tech_award_showhide);
            lua_getfield(L,env,"OnPopup");
            if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1))throw ScriptError("TECH_AWARD full stock OnPopup has not initialized");
            if(job.activate)self.tech_award_popup=luaL_ref(L,LUA_REGISTRYINDEX);
            else{lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_popup);if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD original handler replaced");lua_pop(L,2);}
            lua_getfield(L,env,"RefreshDisplay");
            if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1))throw ScriptError("TECH_AWARD stock RefreshDisplay absent");
            if(job.activate)self.tech_award_refresh=luaL_ref(L,LUA_REGISTRYINDEX);
            else{lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_refresh);if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD RefreshDisplay replaced");lua_pop(L,2);}
            lua_getfield(L,env,"UIManager");lua_getfield(L,-1,"QueuePopup");
            if(!lua_iscfunction(L,-1))throw ScriptError("TECH_AWARD actual native QueuePopup/control owner unavailable");
            if(job.activate){self.tech_award_queue=luaL_ref(L,LUA_REGISTRYINDEX);return 0;}
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_queue);
            if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD native popup owner replaced");lua_pop(L,3);
            // Type is the actual VM's source-installed native enum, never a guessed integer.
            lua_getfield(L,env,"ButtonPopupTypes");lua_getfield(L,-1,"BUTTONPOPUP_TECH_AWARD");
            if(!lua_isnumber(L,-1))throw ScriptError("TECH_AWARD native ButtonPopupTypes absent");
            const auto type=source_integer(L,-1);lua_pop(L,2);
            lua_getfield(L,env,"GameInfo");lua_getfield(L,-1,"Technologies");lua_pushinteger(L,job.data3);lua_gettable(L,-2);
            if(!lua_istable(L,-1))throw ScriptError("TECH_AWARD actual selected technology row absent");lua_pop(L,3);
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_popup);lua_createtable(L,0,7);
            lua_pushinteger(L,type);lua_setfield(L,-2,"Type");
            lua_pushinteger(L,job.data1);lua_setfield(L,-2,"Data1");
            lua_pushinteger(L,job.data2);lua_setfield(L,-2,"Data2");
            lua_pushinteger(L,job.data3);lua_setfield(L,-2,"Data3");
            lua_pushinteger(L,job.flags);lua_setfield(L,-2,"Flags");
            lua_pushboolean(L,job.option1);lua_setfield(L,-2,"Option1");
            lua_pushboolean(L,job.option2);lua_setfield(L,-2,"Option2");
            if(self.tech_award_popup_info!=LUA_NOREF)throw ScriptError("TECH_AWARD popup table already issued");
            lua_pushvalue(L,-1);self.tech_award_popup_info=luaL_ref(L,LUA_REGISTRYINDEX);
            lua_call(L,1,0); // real original OnPopup -> RefreshDisplay -> Queue -> ShowHide
            check_tech_award_original_registration(L,env,"ShowHideHandler",self.tech_award_showhide);
            lua_getfield(L,env,"OnPopup");lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_popup);
            if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD handler changed during native dispatch");lua_pop(L,2);
            lua_getfield(L,env,"RefreshDisplay");lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_refresh);
            if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD display changed during native dispatch");lua_pop(L,2);
            lua_getfield(L,env,"UIManager");lua_getfield(L,-1,"QueuePopup");lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_queue);
            if(!lua_rawequal(L,-1,-2))throw ScriptError("TECH_AWARD popup owner changed during native dispatch");lua_pop(L,3);
        }catch(...){job.exception=std::current_exception();}return 0;
    }
    struct TechAwardInputJob {
        Impl* owner;ui::BnwTechAwardInput target;
        std::int32_t key_down=0,escape_key=0,return_key=0,input_key=0;
        std::exception_ptr exception;
    };
    static int selected_tech_award_input_job(lua_State* L){
        auto& job=*static_cast<TechAwardInputJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,24))return luaL_error(L,"TechAward input protected stack unavailable");
            lua_settop(L,1);
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"create");
            lua_pushliteral(L,"TechAwardPopup");lua_pushnil(L);lua_call(L,2,1);
            const int env=lua_gettop(L);
            check_tech_award_original_registration(L,env,"OnContinueButtonClicked",self.tech_award_continue);
            check_tech_award_original_registration(L,env,"OnClose",self.tech_award_close);
            check_tech_award_original_registration(L,env,"InputHandler",self.tech_award_input);
            check_tech_award_original_registration(L,env,"ShowHideHandler",self.tech_award_showhide);
            switch(job.target){
             case ui::BnwTechAwardInput::continue_button:
              lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_continue);lua_call(L,0,0);break;
             case ui::BnwTechAwardInput::close_button:
              lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_close);lua_call(L,0,0);break;
             case ui::BnwTechAwardInput::escape:
             case ui::BnwTechAwardInput::return_key:
              // Constants come from the executing opaque source issuer and
              // actual retail enum proof, not a caller numeric provider.
              lua_createtable(L,0,2);lua_pushinteger(L,job.escape_key);lua_setfield(L,-2,"VK_ESCAPE");
              lua_pushinteger(L,job.return_key);lua_setfield(L,-2,"VK_RETURN");lua_setfield(L,env,"Keys");
              lua_createtable(L,0,1);lua_pushinteger(L,job.key_down);lua_setfield(L,-2,"KeyDown");lua_setfield(L,env,"KeyEvents");
              lua_rawgeti(L,LUA_REGISTRYINDEX,self.tech_award_input);
              lua_pushinteger(L,job.key_down);lua_pushinteger(L,job.input_key);lua_pushinteger(L,0);
              lua_call(L,3,1);
              if(!lua_isboolean(L,-1)||!lua_toboolean(L,-1))throw ScriptError("Original TechAward InputHandler did not consume input");
              lua_pop(L,1);break;
             default:throw ScriptError("Unknown opaque TechAward input target");
            }
            // Real Dequeue invokes original ShowHide(true,false) synchronously.
            check_tech_award_original_registration(L,env,"OnContinueButtonClicked",self.tech_award_continue);
            check_tech_award_original_registration(L,env,"OnClose",self.tech_award_close);
            check_tech_award_original_registration(L,env,"InputHandler",self.tech_award_input);
            check_tech_award_original_registration(L,env,"ShowHideHandler",self.tech_award_showhide);
        }catch(...){job.exception=std::current_exception();}return 0;
    }
    struct ProductionHookJob {
        Impl* owner;const char* event;std::int32_t player,city,item;bool city_event;
        ScriptTestResult result;std::exception_ptr exception;
    };
    static int nested_production_hook(lua_State* L){
        auto& job=*static_cast<ProductionHookJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,12))return luaL_error(L,"Production TestAll nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"TestAll");lua_pushstring(L,job.event);lua_pushinteger(L,job.player);
            if(job.city_event)lua_pushinteger(L,job.city);lua_pushinteger(L,job.item);
            lua_call(L,job.city_event?5:4,2);
            if(!lua_isnil(L,-2))job.result.value=lua_toboolean(L,-2)!=0;
            const auto count=lua_objlen(L,-1);if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("Production TestAll error list exceeds Lua array API");
            for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,-1,static_cast<int>(i));std::size_t size=0;const auto* message=lua_tolstring(L,-1,&size);if(message)job.result.errors.emplace_back(message,size);lua_pop(L,1);}
        }catch(...){job.exception=std::current_exception();}return 0;
    }
    bool unit_founder_inflight=false;
    bool unit_setxy_inflight=false;
    struct UnitSetXYJob {
        Impl* owner;std::int32_t player,unit,x,y;
        ScriptTestResult result;std::exception_ptr exception;
    };
    static int nested_unit_setxy(lua_State* L){
        auto& job=*static_cast<UnitSetXYJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,12))return luaL_error(L,"UnitSetXY nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"Call");lua_pushliteral(L,"UnitSetXY");
            lua_pushinteger(L,job.player);lua_pushinteger(L,job.unit);lua_pushinteger(L,job.x);lua_pushinteger(L,job.y);
            lua_call(L,6,2); // exact native owner, full unit ID, current X, current Y
            if(!lua_isnil(L,-2))job.result.value=lua_toboolean(L,-2)!=0;
            const auto count=lua_objlen(L,-1);
            if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("UnitSetXY errors exceed Lua array API");
            for(std::size_t n=1;n<=count;++n){
                lua_rawgeti(L,-1,static_cast<int>(n));std::size_t size=0;const char* text=lua_tolstring(L,-1,&size);
                if(text)job.result.errors.emplace_back(text,size);lua_pop(L,1);
            }
        }catch(...){job.exception=std::current_exception();}return 0;
    }
    struct CityTrainedHookJob {
        Impl* owner;std::int32_t player,city,unit;
        ScriptTestResult result;std::exception_ptr exception;
    };
    static int nested_city_trained_hook(lua_State* L){
        auto& job=*static_cast<CityTrainedHookJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,12))return luaL_error(L,"CityTrained nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"Call");lua_pushliteral(L,"CityTrained");
            lua_pushinteger(L,job.player);lua_pushinteger(L,job.city);lua_pushinteger(L,job.unit);
            lua_pushboolean(L,0);lua_pushboolean(L,0);lua_call(L,7,2);
            if(!lua_isnil(L,-2))job.result.value=lua_toboolean(L,-2)!=0;
            const auto count=lua_objlen(L,-1);if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("CityTrained error list exceeds Lua array API");
            for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,-1,static_cast<int>(i));std::size_t size=0;const auto* message=lua_tolstring(L,-1,&size);if(message)job.result.errors.emplace_back(message,size);lua_pop(L,1);}
        }catch(...){job.exception=std::current_exception();}return 0;
    }
    struct TeamHookJob {
        Impl* owner;
        std::int64_t team,technology;
        ScriptTestResult result;
        std::exception_ptr exception;
    };
    static int nested_team_hook(lua_State* L) {
        auto& job=*static_cast<TeamHookJob*>(lua_touserdata(L,1));
        auto& self=*job.owner;
        // All allocating Lua calls stay inside lua_cpcall. Only trivial C++
        // locals cross Lua longjmp; vectors/exception storage belong to the job.
        try {
            if(!lua_checkstack(L,12))return luaL_error(L,"TeamSetHasTech nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"Call");lua_pushliteral(L,"TeamSetHasTech");
            lua_pushinteger(L,job.team);lua_pushinteger(L,job.technology);lua_pushboolean(L,1);
            lua_call(L,5,2);
            if(!lua_isnil(L,-2))job.result.value=lua_toboolean(L,-2)!=0;
            const auto count=lua_objlen(L,-1);
            if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("TeamSetHasTech error list exceeds Lua array API");
            for(std::size_t i=1;i<=count;++i) {
                lua_rawgeti(L,-1,static_cast<int>(i));
                std::size_t size=0;const char* message=lua_tolstring(L,-1,&size);
                if(message)job.result.errors.emplace_back(message,size);
                lua_pop(L,1);
            }
        }catch(...){job.exception=std::current_exception();}
        return 0;
    }

    struct TeamResearchedJob {
        Impl* owner;
        std::int64_t team,technology;
        ScriptTestResult result;
        std::exception_ptr exception;
    };
    static int nested_team_researched(lua_State* L) {
        auto& job=*static_cast<TeamResearchedJob*>(lua_touserdata(L,1));
        auto& self=*job.owner;
        try {
            if(!lua_checkstack(L,12))return luaL_error(L,"TeamTechResearched nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"Call");lua_pushliteral(L,"TeamTechResearched");
            // SDK processTech: team, technology, iChange (integer +1).
            lua_pushinteger(L,job.team);lua_pushinteger(L,job.technology);lua_pushinteger(L,1);
            lua_call(L,5,2);
            if(!lua_isnil(L,-2))job.result.value=lua_toboolean(L,-2)!=0;
            const auto count=lua_objlen(L,-1);
            if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("TeamTechResearched error list exceeds Lua array API");
            for(std::size_t i=1;i<=count;++i){
                lua_rawgeti(L,-1,static_cast<int>(i));
                std::size_t size=0;const char* message=lua_tolstring(L,-1,&size);
                if(message)job.result.errors.emplace_back(message,size);
                lua_pop(L,1);
            }
        }catch(...){job.exception=std::current_exception();}
        return 0;
    }
    struct TeamFounderJob {
        Impl* owner;std::int64_t player;
        ScriptIntegerResult result;std::exception_ptr exception;
    };
    static int nested_team_founder(lua_State* L){
        auto& job=*static_cast<TeamFounderJob*>(lua_touserdata(L,1));auto& self=*job.owner;
        try{
            if(!lua_checkstack(L,12))return luaL_error(L,"Founder happiness nested stack unavailable");
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch_owned_nested");
            lua_pushliteral(L,"AccumulateINT");lua_pushliteral(L,"GetFounderBenefitsReligion");
            lua_pushinteger(L,job.player);lua_call(L,3,2);
            // Nil means the actual registry had no listener. Initial native
            // value is zero and is NOT passed as an argument to callbacks.
            if(!lua_isnil(L,-2))job.result.value=source_integer(L,-2);
            const auto count=lua_objlen(L,-1);
            if(count>static_cast<std::size_t>(INT_MAX))throw ScriptError("Founder error list exceeds Lua array API");
            for(std::size_t i=1;i<=count;++i){
                lua_rawgeti(L,-1,static_cast<int>(i));std::size_t size=0;
                const char* message=lua_tolstring(L,-1,&size);if(message)job.result.errors.emplace_back(message,size);
                lua_pop(L,1);
            }
        }catch(...){job.exception=std::current_exception();}
        return 0;
    }
    static int team_set_has_tech(lua_State* L) {
        // Use genuine metatable SID; no table-supplied team/before-state proof.
        const auto team=object_id(L,true),technology=source_integer(L,2);
        const bool value=lua_toboolean(L,3)!=0;
        const auto player=source_integer(L,4);
        // Preserve installed Expansion2 lSetHasTech's actual index4/5 ABI.
        const bool first=lua_toboolean(L,4)!=0,announce=lua_toboolean(L,5)!=0;
        auto& self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {
            if(std::this_thread::get_id()!=self.ui_thread||!self.calling||self.failed||
               !self.owned_team_tech||self.team_tech_inflight)
                throw ScriptError("Team:SetHasTech actual current issuer unavailable/reentrant");
            self.team_tech_inflight=true;
            struct End {Impl& self;~End(){self.team_tech_inflight=false;}} end{self};
            const auto actual=self.owned_team_tech;
            actual->require_current();
            const auto epoch=self.failure_epoch,uncertainty=self.execution_uncertainty;
            const auto pending=actual->begin({team,technology,player,value,first,announce});
            actual->before_hook(pending);
            TeamHookJob job{&self,team,technology,{}, {}};
            const int top=lua_gettop(L);
            const int status=lua_cpcall(L,nested_team_hook,&job);
            // lua_cpcall restores its arguments; remove any actual error object
            // before source callbacks. Never stringify arbitrary Lua userdata.
            lua_settop(L,top);
            if(status)throw ScriptError("TeamSetHasTech SAME-VM protected nested dispatch failed");
            if(job.exception)std::rethrow_exception(job.exception);
            if(self.failed||self.failure_epoch!=epoch||self.execution_uncertainty!=uncertainty||self.exhausted)
                throw ScriptError("TeamSetHasTech SAME-VM failure/reentry/budget changed");
            actual->require_current();
            if(!job.result.errors.empty())throw ScriptError(job.result.errors.front());
            // False/nil/no listeners never veto writes; only genuine dispatch
            // above may advance the original city/cache/dirty-queue interval.
            actual->after_hook(pending);
            actual->require_current();
            if(pending->phase()==BnwMajorTeamHasTechPhase::team_tech_researched_pending){
                actual->before_researched(pending);
                TeamResearchedJob researched{&self,team,technology,{}, {}};
                const int researched_top=lua_gettop(L);
                const int researched_status=lua_cpcall(L,nested_team_researched,&researched);
                lua_settop(L,researched_top);
                if(researched_status)throw ScriptError("TeamTechResearched SAME-VM protected nested dispatch failed");
                if(researched.exception)std::rethrow_exception(researched.exception);
                if(self.failed||self.failure_epoch!=epoch||self.execution_uncertainty!=uncertainty||self.exhausted)
                    throw ScriptError("TeamTechResearched SAME-VM failure/reentry/budget changed");
                actual->require_current();
                if(!researched.result.errors.empty())throw ScriptError(researched.result.errors.front());
                // False/nil/no-listeners do not veto CallHook. Genuine dispatch
                // precedes EndsGame and the actual setHasTech continuation.
                actual->after_researched(pending);actual->require_current();
            }
            // The mandatory resource/processTech suffix is still unfinished.
            // Do not return Lua's void-success and resume the caller past it.
            throw BnwMajorTeamHasTechUnavailable(actual->mandatory_stop(pending));
        }catch(const std::exception& error){self.source_exception=std::current_exception();self.source_error=error.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Team:SetHasTech native operation failed";}
        // All receipt/shared ownership/C++ temporaries have unwound before Lua
        // longjmp. A script catching this error cannot restore host readiness.
        self.record_failure();return luaL_error(L,"%s",self.source_error);
    }
    static int team_has_tech(lua_State *L){
        const auto team=object_id(L,true),technology=source_integer(L,2);auto &self=owner(L);
        self.source_exception={};self.source_error=nullptr;bool result=false;
        try {result=self.gameplay.has_tech(team,technology);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua team technology query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,result);return 1;
    }
    static int player_get_team(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t result=0;
        try {result=self.gameplay.player_team(player);validate_value(result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player team query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,result);return 1;
    }
    static int player_is_alive(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool result=false;
        try {result=self.gameplay.player_alive(player);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player alive query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,result);return 1;
    }
    static int player_was_alive(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool result=false;
        try {result=self.gameplay.player_ever_alive(player);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player ever-alive query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,result);return 1;
    }
    static int player_is_minor(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool result=false;
        try {result=self.gameplay.player_minor(player);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player minor-civilization query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,result);return 1;
    }
    static int player_get_minor_type(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t result=-1;
        try {result=self.gameplay.player_minor_type(player);validate_value(result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player minor-civilization type query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,result);return 1;
    }
    static int player_get_civilization(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t result=-1;
        try {result=self.gameplay.player_civilization(player);validate_value(result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player civilization query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,result);return 1;
    }
    static int player_get_leader(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t result=-1;
        try {result=self.gameplay.player_leader(player);validate_value(result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player leader query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,result);return 1;
    }
    static bool content_id_equal(std::string_view stored,const char *requested,std::size_t size){
        if(stored.size()!=size)return false;
        for(std::size_t i=0;i<size;++i){
            const auto lower=[](unsigned char c){return c>='A'&&c<='Z'?static_cast<unsigned char>(c-'A'+'a'):c;};
            if(lower(static_cast<unsigned char>(stored[i]))!=lower(static_cast<unsigned char>(requested[i])))return false;
        }
        return true;
    }
    static int content_package_ids(lua_State *L){
        const auto &packages=owner(L).content.packages;
        lua_newtable(L);
        for(std::size_t i=0;i<packages.size();++i){
            lua_pushinteger(L,static_cast<lua_Integer>(i+1));
            lua_pushlstring(L,packages[i].id.data(),packages[i].id.size());
            lua_rawset(L,-3);
        }
        return 1;
    }
    static int content_is_active(lua_State *L){
        std::size_t length=0;const char *id=luaL_checklstring(L,1,&length);
        const auto type=source_integer(L,2);
        const auto &provider=owner(L).content;
        bool active=false;
        if(type==provider.gameplay_type)for(const auto &package:provider.packages)
            if(content_id_equal(package.id,id,length)){active=package.gameplay_active;break;}
        lua_pushboolean(L,active);return 1;
    }
    static int content_description(lua_State *L){
        std::size_t length=0;const char *id=luaL_checklstring(L,1,&length);
        for(const auto &package:owner(L).content.packages)if(content_id_equal(package.id,id,length)){
            lua_pushlstring(L,package.description.data(),package.description.size());return 1;
        }
        lua_pushnil(L);return 1;
    }
    static int player_is_human(lua_State *L){
        const auto player=object_id(L,false);auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool result=false;
        const auto reason=self.gameplay.human_reason?source_integer(L,2):0;
        try {result=self.gameplay.player_human(player,reason);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua player human query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,result);return 1;
    }
    static int map_query(lua_State *L){
        const auto operation=lua_tointeger(L,lua_upvalueindex(1));
        std::int64_t args[4]{};
        const int argc=operation<4?0:operation==5?1:operation==4?2:operation==6?3:4;
        for(int i=0;i<argc;++i)args[i]=source_integer(L,i+1);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t first=0,second=0;bool boolean=false;std::optional<std::int64_t> plot;
        try {
            switch(operation){
                case 0:case 1:{const auto grid=self.map.grid_size();first=grid.first;second=grid.second;
                    if(first<=0||second<=0)throw ScriptError("Invalid Lua map dimensions");
                    if(operation==1)first=wide::multiply_divide(first,second,1);
                    validate_value(first);if(operation==0)validate_value(second);break;}
                case 2:case 3:{const auto wraps=self.map.wraps();boolean=operation==2?wraps.first:wraps.second;break;}
                case 4:plot=self.map.plot(args[0],args[1]);break;
                case 5:plot=self.map.plot_by_index(args[0]);break;
                case 6:plot=self.map.direction(args[0],args[1],args[2]);break;
                case 7:first=self.map.distance(args[0],args[1],args[2],args[3]);validate_value(first);break;
            }
            if(plot){if(*plot<0)throw ScriptError("Invalid Lua plot identity");validate_value(*plot);}
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(operation==0){lua_pushinteger(L,first);lua_pushinteger(L,second);return 2;}
        if(operation==1||operation==7){lua_pushinteger(L,first);return 1;}
        if(operation==2||operation==3){lua_pushboolean(L,boolean);return 1;}
        if(plot)lua_pushinteger(L,*plot);else lua_pushnil(L);return 1;
    }
    static int map_random(lua_State *L){
        const auto raw=luaL_checkinteger(L,1);
        std::size_t length=0;const char *label=luaL_checklstring(L,2,&length);
        if(raw<std::numeric_limits<std::int32_t>::min()||raw>std::numeric_limits<std::int32_t>::max())
            return luaL_error(L,"BNW Map.Rand range exceeds source int32");
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {
            value=self.map.random(raw,std::string_view(label,length));
            validate_value(value);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map random draw failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int map_setup_world_size(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {value=self.map.setup_world_size();validate_value(value);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map world size query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int map_setup_custom_option(lua_State *L){
        // CvLuaMap::lGetCustomOption uses luaL_checkstring and passes the
        // resulting NUL-terminated key to CvPreGame::GetMapOption.
        const char *name=luaL_checkstring(L,1);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<std::int64_t> value;
        try {value=self.map.setup_custom_option(std::string_view(name));
            if(value)validate_value(*value);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map option query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!value)return luaL_error(L,"CIVIS_MISSING_BNW_MAP_OPTION:%s",name);
        lua_pushinteger(L,*value);return 1;
    }
    static int map_setup_wraps(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::pair<bool,bool> value{};
        try {value=self.map.setup_wraps();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map wrap query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,value.first);lua_pushboolean(L,value.second);return 2;
    }
    static int map_setup_fractal_create(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t id=0;
        try {
            id=self.map.setup_fractal_create(source_integer(L,1),source_integer(L,2),
                source_integer(L,3),source_integer(L,4),source_integer(L,5),source_integer(L,6));
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua fractal creation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,id);return 1;
    }
    static int map_setup_fractal_create_rifts(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t id=0;
        try {
            id=self.map.setup_fractal_create_rifts(source_integer(L,1),source_integer(L,2),
                source_integer(L,3),source_integer(L,4),source_integer(L,5),
                source_integer(L,6),source_integer(L,7));
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua rift fractal creation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,id);return 1;
    }
    static int map_setup_fractal_height(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t height=0;
        try {
            std::optional<std::int64_t> y;
            if(!lua_isnoneornil(L,3))y=source_integer(L,3);
            height=self.map.setup_fractal_height(source_integer(L,1),source_integer(L,2),y);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua fractal height query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,height);return 1;
    }
    static int map_setup_fractal_ridges(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {
            self.map.setup_fractal_ridges(source_integer(L,1),source_integer(L,2),
                source_integer(L,3),source_integer(L,4),source_integer(L,5));
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua ridge generation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_plot_count(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t count=0;
        try {count=self.map.setup_plot_count();validate_value(count);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map plot count query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,count);return 1;
    }
    static int map_setup_plot_by_index(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<std::int64_t> id;
        try {id=self.map.setup_plot_by_index(source_integer(L,1));if(id)validate_value(*id);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup plot lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!id)return 0;
        lua_pushinteger(L,*id);return 1;
    }
    static int map_setup_plot_value(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {value=self.map.setup_plot_value(source_integer(L,1),
                static_cast<ScriptMapSetupPlotQuery>(source_integer(L,2)));validate_value(value);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup plot query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int map_setup_plot_adjacent_land(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool adjacent=false;
        try {adjacent=self.map.setup_plot_adjacent_land(source_integer(L,1));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup adjacency query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,adjacent);return 1;
    }
    static int map_setup_plot_set_type(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_plot_set_type(source_integer(L,1),source_integer(L,2),
                lua_toboolean(L,3)!=0,lua_toboolean(L,4)!=0,lua_toboolean(L,5)!=0);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup plot setter failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_plot_set_area(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_plot_set_area(source_integer(L,1),source_integer(L,2));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup area setter failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_recalculate_areas(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_recalculate_areas();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup recalculate areas failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_calculate_areas(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_calculate_areas();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup calculate areas failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_plot_adjacent_shallow_water(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool adjacent=false;
        try {adjacent=self.map.setup_plot_adjacent_shallow_water(source_integer(L,1));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup shallow-water adjacency query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,adjacent);return 1;
    }
    static int map_setup_plot_coastal_land(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool coastal=false;
        try {coastal=self.map.setup_plot_coastal_land(source_integer(L,1));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup coastal-land query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,coastal);return 1;
    }
    static int map_setup_find_water(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool found=false;
        try {found=self.map.setup_find_water(source_integer(L,1),source_integer(L,2),
                                              lua_toboolean(L,3)!=0);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup water query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,found);return 1;
    }
    static int map_setup_plot_area_id(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t id=0;
        try {id=self.map.setup_plot_area_id(source_integer(L,1));validate_value(id);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup area handle failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,id);return 1;
    }
    static int map_setup_area_value(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {value=self.map.setup_area_value(source_integer(L,1),
                static_cast<ScriptMapSetupAreaQuery>(source_integer(L,2)));validate_value(value);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup area query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int map_setup_find_biggest_area(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<std::int64_t> id;
        try {id=self.map.setup_find_biggest_area(lua_toboolean(L,1)!=0);
             if(id)validate_value(*id);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup biggest-area query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!id)return 0;
        lua_pushinteger(L,*id);return 1;
    }
    static int map_setup_plot_can_have_feature(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool eligible=false;
        try {eligible=self.map.setup_plot_can_have_feature(
                 source_integer(L,1),source_integer(L,2));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup feature eligibility failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,eligible);return 1;
    }
    static int map_setup_plot_set_feature_type(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_plot_set_feature_type(
                 source_integer(L,1),source_integer(L,2),source_integer(L,3));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup feature setter failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_plot_inland_corner(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<std::int64_t> corner;
        try {corner=self.map.setup_plot_inland_corner(source_integer(L,1));
             if(corner)validate_value(*corner);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua inland-corner query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!corner)return 0;
        lua_pushinteger(L,*corner);return 1;
    }
    static int map_setup_plot_direction(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<std::int64_t> adjacent;
        try {adjacent=self.map.setup_plot_direction(source_integer(L,1),
                source_integer(L,2),source_integer(L,3));
             if(adjacent)validate_value(*adjacent);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup plotDirection failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!adjacent)return 0;
        lua_pushinteger(L,*adjacent);return 1;
    }
    static int map_setup_plot_river_edge(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        bool present=false;
        try {present=self.map.setup_plot_river_edge(source_integer(L,1),
                static_cast<ScriptMapSetupRiverEdge>(source_integer(L,2)));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup river-edge query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,present);return 1;
    }
    static int map_setup_plot_set_river_edge(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_plot_set_river_edge(source_integer(L,1),
                static_cast<ScriptMapSetupRiverEdge>(source_integer(L,2)),
                lua_toboolean(L,3)!=0,source_integer(L,4));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup river-edge write failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_plot_set_terrain(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.map.setup_plot_set_terrain(source_integer(L,1),source_integer(L,2),
                lua_toboolean(L,3)!=0,lua_toboolean(L,4)!=0);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua setup terrain setter failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int map_setup_define(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::optional<ScriptValue> value;
        try {
            const auto *name=lua_tostring(L,1);
            if(!name)throw ScriptError("BNW map define name must be text");
            value=self.map.setup_define(name);
            if(value)validate_value(*value);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map define lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(value)push(L,*value);else lua_pushnil(L);
        return 1;
    }
    static int map_setup_terrain_info_type(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {
            const char *name=lua_tostring(L,1);
            if(!name)throw ScriptError("BNW map Terrain Info type must be text");
            value=self.map.setup_terrain_info_type(name);
            validate_value(value);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map Terrain Info lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int map_setup_feature_info_type(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t value=0;
        try {
            const char *name=lua_tostring(L,1);
            if(!name)throw ScriptError("BNW map Feature Info type must be text");
            value=self.map.setup_feature_info_type(name);
            validate_value(value);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua map Feature Info lookup failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,value);return 1;
    }
    static int plot_reference(lua_State *L){
        const auto id=source_integer(L,1);auto &self=owner(L);const bool prior_failure=self.failed;self.failed=true;
        auto *value=static_cast<std::int64_t*>(lua_newuserdata(L,sizeof(std::int64_t)));*value=id;
        luaL_newmetatable(L,"Civis.PlotReference");lua_setmetatable(L,-2);self.failed=prior_failure;return 1;
    }
    static int plot_query(lua_State *L){
        const auto operation=static_cast<ScriptPlotQuery>(lua_tointeger(L,lua_upvalueindex(1)));
        luaL_checktype(L,1,LUA_TTABLE);lua_getfield(L,1,"__instance");
        const auto id=*static_cast<std::int64_t*>(luaL_checkudata(L,-1,"Civis.PlotReference"));lua_pop(L,1);
        auto &self=owner(L);std::int64_t team=-1;
        if(operation==ScriptPlotQuery::impassable&&self.map.capabilities.team_impassability_argument&&!lua_isnoneornil(L,2)){
            luaL_checknumber(L,2);team=source_integer(L,2);
        }
        self.source_exception={};self.source_error=nullptr;
        try {self.plot_result=self.map.plot_value(id,operation,team);validate_value(self.plot_result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua plot query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const bool prior_failure=self.failed;self.failed=true;push(L,self.plot_result);self.failed=prior_failure;return 1;
    }
    static int plot_calculated_yield(lua_State *L){
        auto &self=owner(L);
        const bool prior_failed=self.failed,prior_ready=self.ready;
        // BEFORE Lua checks/conversions/OOM; a caught longjmp cannot publish
        // VM readiness. Only complete success after the final push restores it.
        self.failed=true;self.ready=false;self.source_exception={};self.source_error=nullptr;
        try {
            if(!self.map.calculated_yield)throw ScriptError("Lua calculated plot owner absent");
            self.map.calculated_yield->require_current();
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua calculated plot guard failed";}
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        luaL_checktype(L,1,LUA_TTABLE);
        // Bounded shipped raw-instance shape: do not execute a receiver's
        // Lua __index callback while native Game ownership is borrowed.
        lua_pushliteral(L,"__instance");lua_rawget(L,1);
        auto *reference=static_cast<std::int64_t*>(luaL_checkudata(L,-1,"Civis.PlotReference"));
        if(lua_objlen(L,-1)!=sizeof(std::int64_t))return luaL_error(L,"Lua calculated plot reference storage differs");
        const auto id=*reference;lua_pop(L,1);
        const auto yield=source_integer(L,2);const bool display=lua_toboolean(L,3)!=0;
        std::int32_t result=0;
        try {result=self.map.calculated_yield->value(id,yield,display);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua calculated plot query failed";}
        // All native query RAII has unwound before any Lua allocation/longjmp.
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        lua_pushinteger(L,result);self.failed=prior_failed;self.ready=prior_ready;return 1;
    }
    static int game_active_read(lua_State *L){
        auto &self=owner(L);
        if(self.worldview_native_host)return worldview_game_active_read(L);
        const bool prior_failed=self.failed,prior_ready=self.ready;
        self.failed=true;self.ready=false;self.source_exception={};self.source_error=nullptr;
        std::int32_t result=-1;
        try {
            if(!self.map.calculated_yield)throw ScriptError("Lua active team actual Game owner absent");
            self.map.calculated_yield->require_current();
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua active Game guard failed";}
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        // Internal closure selector only, AFTER the genuine constructor-thread
        // and SAME VM guard; caller arguments cannot select another source.
        if(lua_type(L,lua_upvalueindex(1))!=LUA_TNUMBER)return luaL_error(L,"Lua active Game selector type differs");
        const auto op=lua_tointeger(L,lua_upvalueindex(1));
        if(op!=0&&op!=1)return luaL_error(L,"Lua active Game selector domain differs");
        try {
            result=op==0?self.map.calculated_yield->active_player():self.map.calculated_yield->active_team();
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua active team query failed";}
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        lua_pushinteger(L,result);self.failed=prior_failed;self.ready=prior_ready;return 1;
    }
    static int plot_is_revealed(lua_State *L){
        auto &self=owner(L);const bool prior_failed=self.failed,prior_ready=self.ready;
        self.failed=true;self.ready=false;self.source_exception={};self.source_error=nullptr;
        try {
            if(!self.map.calculated_yield)throw ScriptError("Lua reveal actual Game/plot owner absent");
            self.map.calculated_yield->require_current();
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua reveal guard failed";}
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        luaL_checktype(L,1,LUA_TTABLE);
        lua_pushliteral(L,"__instance");lua_rawget(L,1);
        auto *reference=static_cast<std::int64_t*>(luaL_checkudata(L,-1,"Civis.PlotReference"));
        if(lua_objlen(L,-1)!=sizeof(std::int64_t))return luaL_error(L,"Lua reveal reference storage differs");
        const auto id=*reference;lua_pop(L,1);
        const auto team=source_integer(L,2);
        const bool debug=!lua_isnoneornil(L,3)&&lua_toboolean(L,3)!=0; // native luaL_optbool(false)
        bool result=false;
        try {result=self.map.calculated_yield->is_revealed(id,team,debug);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua reveal query failed";}
        // Native ownership locals have unwound before Lua publication/longjmp.
        if(self.source_error)return luaL_error(L,"%s",self.source_error);
        lua_pushboolean(L,result);self.failed=prior_failed;self.ready=prior_ready;return 1;
    }
    static int plot_resource(lua_State *L){
        // Native binding: omitted team is NO_TEAM; explicit nil converts to 0.
        // VP's C++ ignore-technology argument is not exposed by Lua.
        const auto team=lua_gettop(L)>=2?source_integer(L,2):std::int64_t{-1};
        luaL_checktype(L,1,LUA_TTABLE);lua_getfield(L,1,"__instance");
        const auto id=*static_cast<std::int64_t*>(luaL_checkudata(L,-1,"Civis.PlotReference"));lua_pop(L,1);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.plot_result=self.map.resource_type(id,team);validate_value(self.plot_result);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua resource query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        const bool prior_failure=self.failed;self.failed=true;push(L,self.plot_result);self.failed=prior_failure;return 1;
    }
    static int record_message(lua_State *L){
        std::size_t cn=0,tn=0;const char *context=luaL_checklstring(L,1,&cn),*text=luaL_checklstring(L,2,&tn);
        auto &self=owner(L);bool recorded=false;
        try {self.messages.push_back({std::string(context,cn),std::string(text,tn)});recorded=true;}catch(...){}
        if(!recorded){self.failed=true;self.ready=false;return luaL_error(L,"Cannot record Lua output");}return 0;
    }
    static int game_get_turn(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t turn=0;
        try {turn=self.gameplay.game_turn();validate_value(turn);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Game turn query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,turn);return 1;
    }
    static int game_get_speed(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t speed=0;
        try {speed=self.gameplay.game_speed();validate_value(speed);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Game speed query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,speed);return 1;
    }
    static int pregame_query(lua_State *L){
        const auto operation=lua_tointeger(L,lua_upvalueindex(1));
        const auto id=operation==0?0:source_integer(L,1);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;std::int64_t result=0;
        try {
            result=operation==0?self.pregame.active_player():operation==1?self.pregame.slot_status(id):
                   operation==2?self.pregame.civilization(id):self.pregame.player_color(id);
            validate_value(result);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Pregame query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,result);return 1;
    }
    static int pregame_set_leader(lua_State *L){
        const auto player=source_integer(L,1),leader=source_integer(L,2);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {validate_value(player);validate_value(leader);self.pregame.set_leader_type(player,leader);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Pregame leader update failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int pregame_set_color(lua_State *L){
        const auto player=source_integer(L,1),color=source_integer(L,2);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {validate_value(player);validate_value(color);self.pregame.set_player_color(player,color);}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Pregame player color update failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int process_clock(lua_State *L){
        FILETIME created{},exited{},kernel{},user{};
        if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return luaL_error(L,"Process clock unavailable");
        const auto seconds=[](FILETIME value){return static_cast<double>((std::uint64_t{value.dwHighDateTime}<<32)|value.dwLowDateTime)/10000000.0;};
        lua_pushnumber(L,seconds(kernel)+seconds(user));return 1;
    }
    static int mod_entry_points(lua_State *L) {
        std::size_t size=0;const char *kind=luaL_checklstring(L,1,&size);auto &self=owner(L);
        self.source_exception={};self.source_error=nullptr;
        try {
            // Retail binds C strings; embedded NUL ends the query parameter.
            self.mod_rows=self.modding.entry_points(std::string_view(kind));
            for(const auto &row:self.mod_rows)validate_row(row);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Mod entrypoint query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        // If a mod catches OOM during conversion, content is still incomplete.
        const bool prior_failure=self.failed;self.failed=true;
        lua_newtable(L);
        for(std::size_t i=0;i<self.mod_rows.size();++i){
            lua_pushnumber(L,static_cast<lua_Number>(i+1));push_row(L,self.mod_rows[i]);
            lua_rawset(L,-3);
        }
        self.failed=prior_failure;self.mod_rows.clear();return 1;
    }
    static std::int64_t checked_version(lua_State *L) {
        // Decimal text can carry an exact int64 even beyond binary64's range.
        if(lua_type(L,2)==LUA_TSTRING){
            std::size_t size=0;const char *text=lua_tolstring(L,2,&size);std::int64_t result=0;
            const auto parsed=std::from_chars(text,text+size,result);
            if(parsed.ec==std::errc{}&&parsed.ptr==text+size)return result;
        }
        const auto value=luaL_checknumber(L,2);
        if(!std::isfinite(value)||value>9007199254740992.0||value<-9007199254740992.0)
            luaL_error(L,"Mod version requires an exact integer (use decimal text for full int64)");
        return static_cast<std::int64_t>(value);
    }
    static int evaluated_file(lua_State *L) {
        std::size_t mn=0,pn=0;const char *mod=luaL_checklstring(L,1,&mn);
        const auto version=checked_version(L);const char *path=luaL_checklstring(L,3,&pn);
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {self.mod_file=self.modding.evaluated_file(std::string_view(mod),version,std::string_view(path));}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Mod file query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!self.mod_file)return 0; // Native returns zero values, not one nil.
        const bool prior_failure=self.failed;self.failed=true;
        lua_newtable(L);
        lua_pushvalue(L,1);lua_setfield(L,-2,"ModID");
        lua_pushvalue(L,2);lua_setfield(L,-2,"Version");
        lua_pushvalue(L,3);lua_setfield(L,-2,"Path");
        if(self.mod_file->md5){lua_pushlstring(L,self.mod_file->md5->data(),self.mod_file->md5->size());lua_setfield(L,-2,"md5");}
        if(self.mod_file->path){lua_pushlstring(L,self.mod_file->path->data(),self.mod_file->path->size());lua_setfield(L,-2,"EvaluatedPath");}
        self.failed=prior_failure;self.mod_file.reset();return 1;
    }
    static int list_sources(lua_State *L) {
        std::size_t size=0;const char *context=luaL_checklstring(L,1,&size);auto &self=owner(L);
        self.source_exception={};self.source_error=nullptr;
        bool owned_list=false;
        if(self.end_turn_child_loader&&self.selected_loading_context_matches(std::string_view(context,size))){
            lua_Debug caller{};
            if(!lua_getstack(L,1,&caller)||!lua_getinfo(L,"f",&caller))return luaL_error(L,"Selected source list has no include bootstrap caller");
            const auto* closure=lua_isfunction(L,-1)&&!lua_iscfunction(L,-1)?static_cast<const Closure*>(lua_topointer(L,-1)):nullptr;
            owned_list=closure&&self.include_bootstrap_proto&&graph_contains_proto(self.include_bootstrap_proto,closure->l.p);lua_pop(L,1);
            if(!owned_list){self.record_failure();return luaL_error(L,"Selected source list differs from actual retained include bootstrap");}
        }
        try {
            if(owned_list){
                EndTurnSourceListScope scope(self,L,context,size);
                std::function<std::vector<std::string>(std::string_view)> target;
                try{scope.require_current();target=self.sources.files;scope.require_current();
                    self.source_names=target({context,size});scope.require_current();target={};scope.require_current();
                }catch(...){target={};throw;}
            }else self.source_names=self.sources.files({context,size});
        }
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua source enumeration failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_newtable(L);
        for(std::size_t i=0;i<self.source_names.size();++i){
            lua_pushnumber(L,static_cast<lua_Number>(i+1));
            const auto &name=self.source_names[i];lua_pushlstring(L,name.data(),name.size());lua_rawset(L,-3);
        }
        self.source_names.clear();return 1;
    }
    static int compile_include_source_body(lua_State *L) {
        auto &self=owner(L);const bool ready=self.ready,failed=self.failed;
        const auto epoch=self.failure_epoch;
        const auto uncertainty=self.execution_uncertainty;
        // Only this authentic compiler can stage the same issued loader sentinel.
        self.ready=false;
        if((self.end_turn_compiler_scope&&self.end_turn_compiler_scope->current())||
           (self.generic_child_compiler_scope&&self.generic_child_compiler_scope->current()))self.failed.set_loader_sentinel();
        else self.failed=true;
        if(lua_gettop(L)!=3)return luaL_error(L,"Include compiler argument count differs");
        std::size_t cn=0,pn=0;
        const char *context=luaL_checklstring(L,1,&cn),*path=luaL_checklstring(L,2,&pn);
        luaL_checktype(L,3,LUA_TTABLE);
        const void* const environment=lua_topointer(L,3);
        // These lambdas own no application target and perform no Lua APIs.
        // The real callback and all its native bookends stay inside this lender.
        const auto with_provider=[&](auto&& actual) {
            IncludeProviderScope scope(self,L,context,cn,path,pn,environment,epoch,uncertainty,failed);
            scope.require_current();actual();scope.require_current();
        };
        self.source_exception={};self.source_error=nullptr;
        try {
            if(std::string_view(path,pn).find('\0')!=std::string_view::npos)
                throw ScriptError("NUL in Lua source name");
            if(!self.control_graphs.empty()||self.context_sources) {
                if(self.context_sources) {
                    bool found=false;
                    for(std::size_t slot=0;slot<self.child_contexts.size();++slot)
                        if(self.child_contexts[slot].name==std::string_view(context,cn)) {
                            with_provider([&]{self.child_require_current(slot);});
                            lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[slot].environment);
                            const bool same=lua_rawequal(L,3,-1);lua_pop(L,1);
                            if(!same)throw ScriptError("Include differs from owned child environment");
                            found=true;break;
                        }
                    if(!found)throw ScriptError("Include has no owned context source");
                }
                for(auto &entry:self.control_graphs)if(entry.graph->context()==std::string_view(context,cn)) {
                    entry.touched=true;
                    if(!entry.bound)throw ScriptError("Include context graph has not been bound");
                    lua_rawgeti(L,LUA_REGISTRYINDEX,entry.environment);
                    const bool same=lua_rawequal(L,3,-1);lua_pop(L,1);
                    if(!same)throw ScriptError("Include environment differs from authenticated owner");
                    with_provider([&]{entry.graph->require_current();});
                }
                if(self.graph_execution_scope==graph_no_owner ||
                   self.graph_compiled_sources[self.graph_execution_scope].context!=std::string_view(context,cn))
                    throw ScriptError("Include has no authenticated execution owner");
                // Skip only this exact retained bootstrap, never a filename,
                // mutable fenv or an arbitrary dynamically compiled function.
                std::size_t caller_owner=graph_no_owner;
                lua_Debug caller{};
                for(int level=1;lua_getstack(L,level,&caller);++level) {
                    if(!lua_getinfo(L,"f",&caller))break;
                    if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
                    if(!lua_isfunction(L,-1)){lua_pop(L,1);break;}
                    const auto *closure=static_cast<const Closure*>(lua_topointer(L,-1));
                    if(self.include_bootstrap_proto&&graph_contains_proto(self.include_bootstrap_proto,closure->l.p))
                        {lua_pop(L,1);continue;}
                    caller_owner=self.graph_compiled_owner(L,-1);lua_pop(L,1);break;
                }
                if(caller_owner==graph_no_owner ||
                   self.graph_compiled_sources[caller_owner].context!=std::string_view(context,cn))
                    throw ScriptError("Include caller has no authenticated compiled owner");
                // The Lua-visible IncludeFileList is a matcher, not provenance.
                with_provider([&]{
                    std::function<std::vector<std::string>(std::string_view)> target;
                    try{self.include_provider_scope->require_current();target=self.sources.files;self.include_provider_scope->require_current();
                        self.source_names=target({context,cn});self.include_provider_scope->require_current();
                        target={};self.include_provider_scope->require_current();
                    }catch(...){target={};throw;}
                });
                if(std::find(self.source_names.begin(),self.source_names.end(),std::string_view(path,pn))==self.source_names.end())
                    throw ScriptError("Include source is outside the selected provider list");
                self.source_names.clear();
            }
            with_provider([&]{
                std::function<std::string(std::string_view,std::string_view)> target;
                try{self.include_provider_scope->require_current();target=self.sources.read;self.include_provider_scope->require_current();
                    self.source_text=target({context,cn},{path,pn});self.include_provider_scope->require_current();
                    target={};self.include_provider_scope->require_current();
                }catch(...){target={};throw;}
            });
            self.include_chunk_name.assign("@");self.include_chunk_name.append(path,pn);
            for(auto i=self.execution_records.size();i>0;--i) {
                const auto &record=self.execution_records[i-1];
                if(record.action==ScriptLuaAction::include&&record.outcome==ScriptLuaActionOutcome::open&&
                   record.context==std::string_view(context,cn)&&record.name==std::string_view(path,pn))
                    {
                        std::string_view compiled_source=self.source_text;
                        if(compiled_source.starts_with("\xef\xbb\xbf"))compiled_source.remove_prefix(3);
                        self.execution_source(i-1,compiled_source);break;
                    }
            }
            if(!self.control_graphs.empty()||self.context_sources) {
                graph_require_epoch(L,epoch);
                for(auto &entry:self.control_graphs)if(entry.graph->context()==std::string_view(context,cn))with_provider([&]{entry.graph->require_current();});
                if(self.context_sources)for(std::size_t slot=0;slot<self.child_contexts.size();++slot)
                    if(self.child_contexts[slot].name==std::string_view(context,cn))with_provider([&]{self.child_require_current(slot);});
                graph_require_epoch(L,epoch);
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua include compilation provider failed";}
        if(self.source_error){if(self.context_sources)self.record_failure();lua_pushnil(L);lua_pushstring(L,self.source_error);lua_pushliteral(L,"io");return 3;}
        // No local C++ owner lives across the allocating/protected Lua APIs.
        const char *source=self.source_text.data();std::size_t bytes=self.source_text.size();
        if(bytes>=3&&std::memcmp(source,"\xef\xbb\xbf",3)==0){source+=3;bytes-=3;}
        if(bytes&&source[0]=='\x1b') {
            if(self.context_sources)self.record_failure();
            lua_pushnil(L);lua_pushliteral(L,"Civis loads Lua source, not platform-specific bytecode");
            lua_pushliteral(L,"bytecode");return 3;
        }
        if(luaL_loadbuffer(L,source,bytes,self.include_chunk_name.c_str())) {
            if(self.context_sources)self.record_failure();
            lua_pushnil(L);lua_insert(L,-2);lua_pushliteral(L,"compile");return 3;
        }
        lua_pushvalue(L,3);lua_setfenv(L,-2);
        self.source_exception={};self.source_error=nullptr;
        try {
            if(!self.control_graphs.empty()||self.context_sources) {
                self.graph_retain_compiled(L,{context,cn});
                graph_require_epoch(L,epoch);
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Lua include source retention failed";}
        if(self.source_error){if(self.context_sources)self.record_failure();lua_pop(L,1);lua_pushnil(L);lua_pushstring(L,self.source_error);lua_pushliteral(L,"compile");return 3;}
        self.source_text.clear();self.include_chunk_name.clear();
        // Generic include keeps reached execution after a caught native
        // completion attempt. Authenticated graphs still refuse above.
        // Never restore readiness/failure over the retained failure epoch.
        if(self.failure_epoch==epoch){
            if(self.end_turn_compiler_scope){
                if(!self.end_turn_compiler_scope->current()){self.record_failure();return luaL_error(L,"Event69 compiler success lost actual loader provenance");}
                self.ready=ready;
                if(failed)self.failed.set_loader_sentinel();else self.failed=false;
                self.end_turn_compiler_scope->succeeded=true;
            }else if(self.generic_child_compiler_scope){
                if(!self.generic_child_compiler_scope->current()){self.record_failure();return luaL_error(L,"Generic compiler success lost actual loader provenance");}
                self.ready=ready;
                if(failed)self.failed.set_loader_sentinel();else self.failed=false;
                self.generic_child_compiler_scope->succeeded=true;
            }else{self.ready=ready;self.failed=failed;}
        }
        return 1;
    }
    static int compile_include_source(lua_State* L){
        auto& self=owner(L);
        if(!self.end_turn_lifetime&&self.generic_child_loader)return compile_generic_child_include(L);
        if(!self.end_turn_child_loader)return compile_include_source_body(L);
        if(lua_gettop(L)!=3)return luaL_error(L,"Include compiler argument count differs");
        std::size_t cn=0,pn=0;const char* context=luaL_checklstring(L,1,&cn),*path=luaL_checklstring(L,2,&pn);
        luaL_checktype(L,3,LUA_TTABLE);const void* env=lua_topointer(L,3);
        self.source_exception={};self.source_error=nullptr;bool qualified=false;
        try{qualified=self.end_turn_qualify_compiler(L,context,cn,path,pn,env);}
        catch(...){self.source_exception=std::current_exception();self.source_error="Event69 include compiler provenance refused";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        if(!qualified)return compile_include_source_body(L);
        lua_pushcfunction(L,compile_include_source_body);lua_insert(L,1);
        int status=0;
        try{
            {EndTurnIncludeCompilerScope scope(self,L,context,cn,path,pn,env);
             status=lua_pcall(L,3,LUA_MULTRET,0);
             if(!status&&scope.succeeded&&!scope.current())throw ScriptError("Event69 compiler return retired before publication");
            } // every provider/compiler Lua allocation is protected; retire first
        }catch(...){self.record_failure();return luaL_error(L,"Event69 compiler lender refused");}
        if(status)return lua_error(L);
        return lua_gettop(L);
    }
    static int report_source_error(lua_State *L) {
        std::size_t cn=0,pn=0,en=0;const char *context=luaL_checklstring(L,1,&cn),*path=luaL_checklstring(L,2,&pn),
            *message=luaL_checklstring(L,3,&en);
        auto &self=owner(L);if(self.context_sources)self.record_failure();else {self.failed=true;self.ready=false;}
        bool recorded=false;
        try {self.source_errors.push_back({std::string(context,cn),std::string(path,pn),std::string(message,en)});recorded=true;}
        catch(...){}
        if(!recorded)return luaL_error(L,"Cannot record Lua source error");
        return 0;
    }
    static void *allocate(void *user,void *old,std::size_t old_size,std::size_t size) noexcept {
        auto &self=*static_cast<Impl*>(user);if(!old)old_size=0;
        if(!size){std::free(old);self.bytes-=old_size;return nullptr;}
        if(size>UINT64_MAX-(self.bytes-old_size))return nullptr;
        const auto next=self.bytes-old_size+size;
        if(self.budget.memory_bytes&&next>self.budget.memory_bytes)return nullptr;
        void *p=std::realloc(old,size);if(p)self.bytes=next;return p;
    }
    static void hook(lua_State *L,lua_Debug*) {
        void *user=nullptr;lua_getallocf(L,&user);auto &self=*static_cast<Impl*>(user);
        if(self.remaining<=1000){
            self.remaining=0;self.exhausted=true;
            // A script may catch errors with pcall. Once exhausted, interrupt
            // every subsequent instruction so a retry loop cannot keep running.
            lua_sethook(L,hook,LUA_MASKCOUNT,1);
            luaL_error(L,"Civis Lua instruction budget exhausted");
        }
        self.remaining-=1000;
    }
    static int invoke(lua_State *L) {
        const int count=lua_gettop(L);
        child_require_bindings(L);
        if(count<1)return luaL_error(L,"Missing Lua callback");
        if(!lua_checkstack(L,count+3))return luaL_error(L,"Lua callback stack unavailable");
        lua_State *thread=lua_newthread(L);
        if(!lua_checkstack(thread,count+2))return luaL_error(L,"Lua callback thread stack unavailable");
        if(lua_isfunction(L,1)){lua_getfenv(L,1);lua_xmove(L,thread,1);lua_replace(thread,LUA_GLOBALSINDEX);}
        void *user=nullptr;lua_getallocf(L,&user);const auto *self=static_cast<Impl*>(user);
        if(self->budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
        for(int i=1;i<=count;++i)lua_pushvalue(L,i);
        lua_xmove(L,thread,count);
        auto &host=owner(L);const auto previous_scope=host.graph_execution_scope;
        host.graph_execution_scope=host.graph_compiled_owner(thread,1);
        const int status=lua_pcall(thread,count-1,1,0);
        host.graph_execution_scope=previous_scope;
        child_require_bindings(L);
        lua_pushboolean(L,status==0);lua_xmove(thread,L,1);return 2;
    }
    static std::int64_t source_integer(lua_State *L,int index){
        // The shipped Lua DLL uses FISTP, not a C truncating cast. Preserve
        // rounding under the process floating-point mode, with a wide result.
        const auto value=std::nearbyint(lua_tonumber(L,index));
        if(!std::isfinite(value)||value>9007199254740992.0||value<-9007199254740992.0)
            luaL_error(L,"Lua integer conversion exceeds the exact binary64 range");
        return static_cast<std::int64_t>(value);
    }
    static int invoke_lua_event(lua_State *L){
        // Original518410 calls listeners on the same thread, aborting the
        // synchronous group on the first error. Catch only to restore the
        // native graph scope, then rethrow that same error object.
        const int count=lua_gettop(L);auto& self=owner(L);
        const auto epoch=self.failure_epoch,writes=self.failed.writes_,uncertainty=self.execution_uncertainty;
        const auto*const actual=self.worldview_native_host;
        child_require_bindings(L);
        if(actual)worldview_require_lua_origin(L);
        if(self.failure_epoch!=epoch||self.failed.writes_!=writes||self.execution_uncertainty!=uncertainty||
           self.worldview_native_host!=actual||!native_lua_events_factory_without_callbacks(L))
            return luaL_error(L,"LuaEvents genuine synchronous listener entry changed");
        const auto previous_scope=self.graph_execution_scope;
        if(!self.control_graphs.empty()||self.context_sources)self.graph_execution_scope=self.graph_compiled_owner(L,1);
        const int status=lua_pcall(L,count-1,0,0);self.graph_execution_scope=previous_scope;
        if(status)return lua_error(L);
        // Lua may catch a native operation failure and return normally. This
        // POST cut rejects that changed channel before the next listener.
        child_require_bindings(L);
        if(actual)worldview_require_lua_origin(L);
        if(self.failure_epoch!=epoch||self.failed.writes_!=writes||self.execution_uncertainty!=uncertainty||
           self.worldview_native_host!=actual||!native_lua_events_factory_without_callbacks(L))
            return luaL_error(L,"LuaEvents genuine listener state changed after callback");
        return 0;
    }
    static int ui_command(lua_State *L){
        auto &self=owner(L);
        std::size_t length=0;
        const char *name=luaL_checklstring(L,1,&length);
        const std::string_view command_name(name,length);
        if(!self.ui_command_names.contains(std::string(command_name)))
            return luaL_error(L,"UI command is not declared by the active graphics layer");
        self.source_exception={};self.source_error=nullptr;self.ui_command_args.clear();
        try {
            for(int i=2;i<=lua_gettop(L);++i){
                switch(lua_type(L,i)){
                case LUA_TNIL:self.ui_command_args.emplace_back(std::monostate{});break;
                case LUA_TBOOLEAN:self.ui_command_args.emplace_back(lua_toboolean(L,i)!=0);break;
                case LUA_TNUMBER:{
                    const auto number=lua_tonumber(L,i);
                    if(!std::isfinite(number))throw ScriptError("Nonfinite UI command argument");
                    if(std::floor(number)==number&&number>=-9007199254740992.0&&
                       number<=9007199254740992.0)
                        self.ui_command_args.emplace_back(static_cast<std::int64_t>(number));
                    else self.ui_command_args.emplace_back(number);
                    break;
                }
                case LUA_TSTRING:{
                    std::size_t size=0;const char *value=lua_tolstring(L,i,&size);
                    self.ui_command_args.emplace_back(std::string(value,size));break;
                }
                default:throw ScriptError("Structured UI command argument requires an explicit bridge");
                }
            }
            self.ui_events.command(command_name,self.ui_command_args);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="UI command failed";}
        self.ui_command_args.clear();
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int ui_screen_size(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::pair<std::int64_t,std::int64_t> size{};
        try {size=self.ui_layout.screen_size();validate_value(size.first);validate_value(size.second);
             if(size.first<=0||size.second<=0)throw ScriptError("UI viewport is not measured");}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="UI viewport query failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,size.first);lua_pushinteger(L,size.second);return 2;
    }
    static int ui_layout_control(lua_State *L){
        auto &self=owner(L);const char *id=lua_tostring(L,lua_upvalueindex(1));
        const int operation=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
        luaL_checktype(L,1,LUA_TTABLE);lua_getfield(L,1,"id");
        const char *actual=lua_tostring(L,-1);
        if(!actual||std::strcmp(actual,id)!=0)return luaL_error(L,"UI control receiver differs from selected XML");
        lua_pop(L,1);self.source_exception={};self.source_error=nullptr;
        std::pair<std::int64_t,std::int64_t> size{};
        try {
            if(operation==0){size=self.ui_layout.get_size(id);validate_value(size.first);validate_value(size.second);}
            else if(operation==1){
                luaL_checktype(L,2,LUA_TTABLE);
                lua_getfield(L,2,"x");const auto x=source_integer(L,-1);lua_pop(L,1);
                lua_getfield(L,2,"y");const auto y=source_integer(L,-1);lua_pop(L,1);
                self.ui_layout.set_size(id,x,y);
            }else if(operation==2)
                self.ui_layout.set_offset(id,source_integer(L,2),source_integer(L,3));
            else if(operation==3)self.ui_layout.calculate_internal_size(id);
            else throw ScriptError("Unknown UI layout operation");
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="UI control operation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(operation!=0)return 0;
        lua_createtable(L,0,2);lua_pushinteger(L,size.first);lua_setfield(L,-2,"x");
        lua_pushinteger(L,size.second);lua_setfield(L,-2,"y");return 1;
    }
    static void install_ui_layout(lua_State *L,int environment){
        auto &self=owner(L);
        if(!native_postread_ui_manager_preserve_layout_owned(L,environment)){
            lua_createtable(L,0,1);lua_pushcfunction(L,ui_screen_size);
            lua_setfield(L,-2,"GetScreenSizeVal");lua_setfield(L,environment,"UIManager");
        }
        lua_createtable(L,0,static_cast<int>(self.ui_layout.controls.size()));
        for(const auto &control:self.ui_layout.controls){
            lua_createtable(L,0,control.has_geometry?6:2);
            lua_pushlstring(L,control.id.data(),control.id.size());lua_setfield(L,-2,"id");
            lua_pushlstring(L,control.kind.data(),control.kind.size());lua_setfield(L,-2,"kind");
            if(control.has_geometry){
                constexpr const char *methods[]={"GetSize","SetSize","SetOffsetVal","CalculateInternalSize"};
                for(int operation=0;operation<4;++operation){
                    lua_pushlstring(L,control.id.data(),control.id.size());
                    lua_pushinteger(L,operation);lua_pushcclosure(L,ui_layout_control,2);
                    lua_setfield(L,-2,methods[operation]);
                }
            }
            if(control.has_callbacks){
                constexpr const char *methods[]={"RegisterCallback","ClearCallback"};
                for(int operation=0;operation<2;++operation){
                    lua_pushlstring(L,control.id.data(),control.id.size());
                    lua_pushinteger(L,operation);lua_pushvalue(L,-3);
                    lua_pushcclosure(L,ui_control_callback,3);
                    lua_setfield(L,-2,methods[operation]);
                }
            }
            if(control.id==self.text_entry.control_id){
                lua_pushvalue(L,-1);self.text_receiver=luaL_ref(L,LUA_REGISTRYINDEX);
                constexpr const char *methods[]={"GetText","RegisterCallback","ClearCallback"};
                for(int operation=0;operation<3;++operation){
                    lua_pushinteger(L,operation);lua_pushvalue(L,-2);
                    lua_pushcclosure(L,ui_text_entry,2);lua_setfield(L,-2,methods[operation]);
                }
            }
            lua_setfield(L,-2,control.id.c_str());
        }
        lua_setfield(L,environment,"Controls");
        if(!self.tooltip.context_id.empty()){
            lua_createtable(L,0,static_cast<int>(self.tooltip.controls.size()));
            for(const auto &control:self.tooltip.controls){
                lua_createtable(L,0,5);
                lua_pushlstring(L,control.id.data(),control.id.size());lua_setfield(L,-2,"id");
                lua_pushlstring(L,control.kind.data(),control.kind.size());lua_setfield(L,-2,"kind");
                constexpr const char *methods[]={"SetTexture","SetTextureOffset","SetHide"};
                for(int operation=0;operation<3;++operation){
                    lua_pushlstring(L,control.id.data(),control.id.size());lua_pushinteger(L,operation);lua_pushvalue(L,-3);
                    lua_pushcclosure(L,ui_tooltip_control,3);lua_setfield(L,-2,methods[operation]);
                }
                lua_setfield(L,-2,control.id.c_str());
            }
            lua_pushvalue(L,-1);self.tooltip_controls=luaL_ref(L,LUA_REGISTRYINDEX);
            lua_createtable(L,0,1);lua_pushvalue(L,-2);lua_pushvalue(L,-2);
            lua_pushcclosure(L,ui_tooltip_table,2);lua_setfield(L,-2,"GetTypeControlTable");
            lua_setfield(L,environment,"TTManager");lua_pop(L,1);
        }
        if(!self.ui_context.context_id.empty()){
            lua_createtable(L,0,3);
            constexpr const char *methods[]={"IsHidden","SetInputHandler","SetShowHideHandler"};
            for(int operation=0;operation<3;++operation){
                lua_pushinteger(L,operation);lua_pushvalue(L,-2);
                lua_pushcclosure(L,ui_context_handler,2);lua_setfield(L,-2,methods[operation]);
            }
            lua_setfield(L,environment,"ContextPtr");
            lua_createtable(L,0,2);
            lua_pushinteger(L,static_cast<int>(ScriptUiKeyEvent::key_down));lua_setfield(L,-2,"KeyDown");
            lua_pushinteger(L,static_cast<int>(ScriptUiKeyEvent::key_up));lua_setfield(L,-2,"KeyUp");
            lua_setfield(L,environment,"KeyEvents");
            lua_createtable(L,0,2);
            lua_pushinteger(L,static_cast<int>(ScriptUiKey::escape));lua_setfield(L,-2,"VK_ESCAPE");
            lua_pushinteger(L,static_cast<int>(ScriptUiKey::enter));lua_setfield(L,-2,"VK_RETURN");
            lua_setfield(L,environment,"Keys");
        }
    }
    #include "lua_ui_control_graph_bindings.inc"
    #include "lua_ui_context_loader.inc"
    #include "lua_engine_ui_end_turn_stage.inc"
#include "lua_worldview_native_context_stage.inc"
#include "lua_worldview_created_registration.inc"
#include "lua_worldview_damage_registration.inc"
std::deque<std::shared_ptr<UnitDamageEventReturn>>worldview_damage_returns;
std::shared_ptr<BnwUnitFlagHealthInstanceLoan::State>worldview_health_membership_state;
#include "lua_worldview_loaded_child_call.inc"
#include "lua_worldview_game_active_read.inc"
#include "lua_worldview_player_turn_active.inc"
#include "lua_worldview_pregame_multiplayer.inc"
#include "lua_worldview_sp_client_scalars.inc"
#include "lua_worldview_owned_control_callbacks.inc"
#include "lua_worldview_unit_reference_reads.inc"
#include "lua_worldview_owned_context_callbacks.inc"
#include "lua_context_showhide_callbacks.inc"
#include "lua_worldview_created_listener.inc"
#include "lua_worldview_damage_listener.inc"
#include "lua_worldview_health_damage_protected.inc"
#include "lua_worldview_health_return_registry.inc"
#include "lua_worldview_context_visibility.inc"
#include "lua_worldview_screen_size.inc"
#include "lua_worldview_layout_control_bindings.inc"
#include "lua_worldview_mouse_consume_control.inc"
#include "lua_selected_map_init_query.inc"
#include "lua_native_context_state_prepare.inc"
#include "lua_native_context_postread_globals_ref.inc"
#include "lua_native_context_setup_state_name.inc"
#include "lua_native_context_debug_config_prefix.inc"
#include "lua_native_context_profile_restricted_libraries.inc"
#include "lua_native_context_master_include_table_prefix.inc"
#include "lua_native_context_child_print_delivery.inc"
#include "lua_native_context_include_traceback_prefix.inc"
#include "lua_native_context_profile_restart.inc"
#include "lua_native_context_event_app_init.inc"
#include "lua_native_context_event_camera_view.inc"
#include "lua_native_context_event_camera_projection.inc"
#include "lua_native_context_event_animation_sampling.inc"
#include "lua_native_context_event_dont_record_commands.inc"
#include "lua_native_context_event_record_commands.inc"
#include "lua_native_context_event_city_handle.inc"
#include "lua_native_context_event_unit_handle.inc"
#include "lua_native_context_event_unit_type.inc"
#include "lua_native_context_event_wonder_type.inc"
#include "lua_native_context_event_wonder_state.inc"
#include "lua_native_context_event_wonder_placement.inc"
#include "lua_native_context_event_display_units.inc"
#include "lua_native_context_event_unit_scale.inc"
#include "lua_native_context_event_landmark_swap.inc"
#include "lua_native_context_event_unit_swap.inc"
#include "lua_native_context_event_building_swap.inc"
#include "lua_native_context_event_sequence_init.inc"
#include "lua_native_context_event_state_dump.inc"
#include "lua_native_context_event_state_request.inc"
#include "lua_native_context_event_strategic_view.inc"
#include "lua_native_context_event_ai_started.inc"
#include "lua_native_context_event_ai_ended.inc"
#include "lua_native_context_event_ai_step.inc"
#include "lua_native_context_event_game_view.inc"
#include "lua_native_context_events_ordered_common.inc"
#include "lua_clear_hex_highlight_owned_stage.inc"
#include "lua_native_context_events_lua_event.inc"
#include "lua_native_context_events_prefix_frame.inc"
#include "lua_native_context_events_factory_prefix.inc"
#include "lua_native_child_context_controls_export.inc"
#include "lua_native_root_ordinary_controls_export.inc"
#include "lua_native_root_ordinary_metatype.inc"
#include "lua_native_context_postread_root_cdata.inc"
#include "lua_native_context_postread_load_tail_owned.inc"
#include "lua_native_context_definition_owner_source28.inc"
#include "lua_native_control_base_constructor_source28.inc"
#include "lua_native_control_base_setoffset_record_source10.inc"
#include "lua_native_control_base_geometry_source25_owned.inc"
#include "lua_native_control_base_setoffset_decoder_owned.inc"
#include "lua_native_control_base_setoffset_callable_owned.inc"
#include "lua_native_control_base_offset_axis_callable_owned.inc"
#include "lua_native_control_base_offset_getter_callable_owned.inc"
#include "lua_native_control_base_setanchor_callable_owned.inc"
#include "lua_native_control_base_hide_clip_childcount_callable_owned.inc"
#include "lua_native_control_base_reprocess_size_callable_owned.inc"
#include "lua_native_control_base_setsize_callable_owned.inc"
#include "lua_native_control_base_color_alpha_success_source19_owned.inc"
#include "lua_native_control_base_sort_branch_id_callable_owned.inc"
#include "lua_native_control_base_mouse_source25_callable_owned.inc"
#include "lua_native_control_base_disabled_modal_visibility_callable_owned.inc"
#include "lua_native_control_base_tooltip_callable_owned.inc"
#include "lua_native_control_base_parent_release_source28_callable_owned.inc"
#include "lua_native_control_base_mutable_library_lifecycle49_owned.inc"
#include "lua_native_ordinary_method_table_owner.inc"
#include "lua_native_ordinary_codebuddy_callback.inc"
#include "lua_native_context_entered_controls.inc"
#include "lua_native_context_controls_named_retire.inc"
#include "lua_native_context_lua_events_registrar.inc"
#include "lua_native_context_lua_events_kernel.inc"
#include "lua_native_context_lua_events_body.inc"
#include "lua_native_context_api_registration_prefix.inc"
#include "lua_native_context_db_query.inc"
#include "lua_native_gameinfo_types_info_owned.inc"
#include "lua_native_context_db_registration.inc"
#include "lua_native_context_gameinfo_query.inc"
#include "lua_native_context_gameinfo_defines_registration.inc"
#include "lua_native_context_ui_enums_registration.inc"
#include "lua_native_context_network_registration.inc"
#include "lua_native_context_steam_registration.inc"
#include "lua_native_context_locale_registration.inc"
#include "lua_native_context_content_registration.inc"
#include "lua_native_modding_activated_entrypoints_owned.inc"
#include "lua_native_context_modding_registration.inc"
#include "lua_native_master_print_core.inc"
#include "lua_native_master_extensions.inc"
#include "lua_native_master_threads.inc"
#include "lua_native_master_thread_environment_retire.inc"
#include "lua_native_master_thread_lookup.inc"
#include "lua_native_master_debug_socket.inc"
#include "lua_native_master_protected_call.inc"
#include "lua_native_context_selected_registrar_scope_owned.inc"
#include "lua_native_master_reference_bank.inc"
#include "lua_native_master_thread_ledger.inc"
#include "lua_native_flua_exposure_registration_scope.inc"
#include "lua_native_flua_typed_registry.inc"
#include "lua_native_flua_named_registry.inc"
#include "lua_native_flua_known_control_bindings_owned.inc"
#include "lua_native_context_postread_lookup_control_owned.inc"
#include "lua_native_builtin31_registration_owned.inc"
#include "lua_native_flua_builtin_signatures.inc"
#include "lua_native_flua_builtin_supplier_owned.inc"
#include "lua_native_context_postread_ui_manager_owned.inc"
#include "lua_native_context_postread_tt_manager_owned.inc"
#include "lua_native_context_flua_registration_prefix.inc"
#include "lua_native_context_options_registration.inc"
#include "lua_native_context_ui_registration_owned.inc"
#include "lua_native_context_matchmaking_registration.inc"
#include "lua_native_context_path_registration.inc"
#include "lua_native_context_profiler_registration.inc"
#include "lua_native_context_pregame_registration.inc"
#include "lua_native_context_game_events_registration.inc"
#include "lua_native_context_gamecore_registration.inc"
#include "lua_native_gamecore_map_game_registration_owned.inc"
#include "lua_native_gamecore_player_team_instances_owned.inc"
#include "lua_native_context_controls_table.inc"
#include "lua_native_context_initial_state.inc"
#include "lua_native_context_direct_initial_lua_owned.inc"
#include "lua_native_context_direct_fields_owned.inc"
    #include "lua_context_loader_temporary_state.inc"
    static int execute_context(lua_State *L,std::string_view context,std::string_view source,std::string_view name) {
        auto &self=owner(L);
        lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"create");
        std::size_t retained_slot=graph_no_owner;
        if(self.context_sources)for(std::size_t slot=0;slot<self.child_contexts.size();++slot)
            if(self.child_contexts[slot].name==context){retained_slot=slot;break;}
        lua_pushlstring(L,context.data(),context.size());lua_rawgeti(L,LUA_REGISTRYINDEX,self.base_environment);
        if(retained_slot!=graph_no_owner&&self.child_contexts[retained_slot].state_prepared)
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[retained_slot].environment);
        else lua_pushnil(L);
        // Genuine selected loading scope consumes the retained actual prepared
        // environment/function. A partial/missing profile refuses before create.
        const bool selected_native_print=self.worldview_native_host&&retained_slot!=graph_no_owner;
        if(selected_native_print)push_native_context_print_delivery(L,retained_slot,lua_gettop(L));
        else lua_pushnil(L);
        if(selected_native_print)push_native_context_include_delivery(L,retained_slot,lua_gettop(L)-1);
        else lua_pushnil(L);
        lua_call(L,5,1);
        if(selected_native_print){
            native_context_print_delivery_check(L,retained_slot,lua_gettop(L));
            native_context_include_delivery_full(L,retained_slot);
            native_include_child_physical_query(L,lua_gettop(L),retained_slot);
        }
        if(!self.ui_layout_bound && !self.ui_layout.context_id.empty() &&
           context==self.ui_layout.context_id){
            install_ui_layout(L,lua_gettop(L));self.ui_layout_bound=true;
        }
        for(std::size_t slot=0;slot<self.control_graphs.size();++slot)
            if(context==self.control_graphs[slot].graph->context()&&!self.control_graphs[slot].bound)
                install_control_graph(L,lua_gettop(L),slot);
        if(!self.plain_lookup_bound&&context==self.plain_lookup_context){
            lua_getfield(L,-1,"Locale");
            if(!lua_istable(L,-1))return luaL_error(L,"Selected Locale.Lookup requires an actual locale provider");
            lua_pushcfunction(L,plain_locale_lookup);lua_setfield(L,-2,"Lookup");lua_pop(L,1);
            self.plain_lookup_bound=true;
        }
        if(!self.locale_text_bound&&context==self.locale_text_context){
            lua_getfield(L,-1,"Locale");
            if(!lua_istable(L,-1))return luaL_error(L,"Selected text operations require an actual locale provider");
            lua_pushboolean(L,0);lua_pushcclosure(L,locale_text_operation,1);lua_setfield(L,-2,"ToLower");
            lua_pushboolean(L,1);lua_pushcclosure(L,locale_text_operation,1);lua_setfield(L,-2,"Compare");lua_pop(L,1);
            self.locale_text_bound=true;
        }
        if(!self.raw_count_bound&&context==self.raw_count_context){
            self.require_ui_callback_owner();
            install_civis_raw_table_count(L,lua_gettop(L));self.raw_count_bound=true;
        }
        if(!self.selected_option_bound&&context==self.selected_option_context){
            self.require_ui_callback_owner();
            install_civis_selected_game_option(L,lua_gettop(L));self.selected_option_bound=true;
        }
        if(!self.units_query_bound&&context==self.units_query_context){
            self.require_ui_callback_owner();
            install_civis_units_query(L,lua_gettop(L));self.units_query_bound=true;
        }
        worldview_install_native_enums(L,lua_gettop(L),context);
        worldview_install_player_turn_active(L,lua_gettop(L),context);
        const int pregame_environment=lua_gettop(L);
        lua_pushcfunction(L,native_pregame_preserve_prepared_namespace_owned);
        lua_pushvalue(L,pregame_environment);lua_call(L,1,1);
        const bool genuine_pregame_prepared=lua_toboolean(L,-1)!=0;lua_pop(L,1);
        if(!genuine_pregame_prepared)worldview_install_pregame_multiplayer(L,pregame_environment,context);
        worldview_install_sp_client_scalars(L,lua_gettop(L),context);
        if(context=="TechAwardPopup")install_tech_award_context(L,lua_gettop(L));
        install_child_context(L,lua_gettop(L),context);
        // First generic execution installs its genuine named context above.
        // Resolve that new SAME slot before retaining the actual coroutine
        // and compiled closure; pre-created native context slots stay intact.
        if(self.context_sources&&retained_slot==graph_no_owner) {
            for(std::size_t slot=0;slot<self.child_contexts.size();++slot)
                if(self.child_contexts[slot].name==context){retained_slot=slot;break;}
            if(retained_slot==graph_no_owner)
                return luaL_error(L,"Installed child context has no retained slot");
        }
        lua_State *thread=nullptr;
        if(retained_slot!=graph_no_owner&&self.child_contexts[retained_slot].execution_thread!=LUA_NOREF) {
            lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[retained_slot].execution_thread);
            thread=lua_tothread(L,-1);
            if(!thread||lua_status(thread)||lua_gettop(thread))return luaL_error(L,"Retained context thread is not idle");
        }else {
            thread=lua_newthread(L);
            if(retained_slot!=graph_no_owner) {
                lua_pushvalue(L,-1);self.child_contexts[retained_slot].execution_thread=luaL_ref(L,LUA_REGISTRYINDEX);
            }
        }
        lua_pushvalue(L,-2);lua_xmove(L,thread,1);lua_replace(thread,LUA_GLOBALSINDEX);
        if(self.budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
        const int status=luaL_loadbuffer(thread,source.data(),source.size(),name.data());
        if(status){lua_sethook(thread,nullptr,0,0);lua_xmove(thread,L,1);return lua_error(L);}
        if(!self.control_graphs.empty()||self.context_sources) {
            // Retain/allocate registry refs on the currently protected caller
            // stack; the exact compiled closure is moved back unchanged.
            lua_xmove(thread,L,1);self.graph_execution_scope=self.graph_retain_compiled(L,context);lua_xmove(L,thread,1);
            if(retained_slot!=graph_no_owner)self.child_contexts[retained_slot].execution_source=self.graph_execution_scope;
        }
        const bool end_turn_load=self.end_turn_lifetime&&context==self.end_turn_context;
        if(end_turn_load){
            if(self.end_turn_load_active){self.record_failure();return luaL_error(L,"Event69 selected child load reentry");}
            self.end_turn_load_epoch=self.failure_epoch;self.end_turn_load_source=self.graph_execution_scope;
            self.end_turn_load_owner=self.end_turn_child_loader;
            self.end_turn_load_prior_failed=!self.end_turn_child_loader||!self.end_turn_child_loader->healthy_origin;
            self.end_turn_load_active=true;
        }
        const int execution_status=lua_pcall(thread,0,0,0);lua_sethook(thread,nullptr,0,0);
        if(end_turn_load){self.end_turn_load_active=false;self.end_turn_load_source=graph_no_owner;self.end_turn_load_owner=nullptr;}

        if(execution_status){lua_xmove(thread,L,1);return lua_error(L);}
        return 0;
    }

    static int plain_locale_lookup(lua_State *L){
        auto &self=owner(L);
        if(lua_gettop(L)!=1)return luaL_error(L,"Selected Locale.Lookup supports one verified bare tag only");
        std::size_t length=0;const char *tag=luaL_checklstring(L,1,&length);
        if(std::find(self.plain_lookup_tags.begin(),self.plain_lookup_tags.end(),std::string_view(tag,length))==self.plain_lookup_tags.end())
            return luaL_error(L,"Locale.Lookup tag is outside the selected bare-tag capability");
        return locale_convert(L);
    }
    static int locale_text_operation(lua_State *L){
        const bool compare=lua_toboolean(L,lua_upvalueindex(1))!=0;
        if(lua_gettop(L)!=(compare?2:1))return luaL_error(L,"Selected locale text operation has invalid argument count");
        // Lua 5.1 checkstring accepts numbers; these source-backed operations
        // require text and preserve embedded bytes for UTF-8 validation.
        luaL_checktype(L,1,LUA_TSTRING);if(compare)luaL_checktype(L,2,LUA_TSTRING);
        std::size_t first_size=0,second_size=0;
        const char *first=lua_tolstring(L,1,&first_size);
        const char *second=compare?lua_tolstring(L,2,&second_size):nullptr;
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;self.locale_text.clear();
        std::int64_t order=0;
        try {
            self.require_ui_callback_owner();
            if(compare){
                order=self.locale_operations.compare(std::string_view(first,first_size),std::string_view(second,second_size));
                if(order < -1 || order > 1)throw ScriptError("Selected locale comparator must return -1, 0, or 1");
            }else self.locale_text=self.locale_operations.lower(std::string_view(first,first_size));
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Selected locale text operation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(compare)lua_pushinteger(L,order);else lua_pushlstring(L,self.locale_text.data(),self.locale_text.size());
        self.locale_text.clear();return 1;
    }
    static int civis_raw_table_count(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        std::int64_t count=0;
        try {
            self.require_ui_callback_owner();
            if(lua_gettop(L)!=1)throw ScriptError("Civis raw table count requires exactly one table");
            count=count_civis_raw_table_keys(L,1);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis raw table count failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushinteger(L,count);return 1;
    }
    static void install_civis_raw_table_count(lua_State *L,int environment){
        if(!lua_checkstack(L,8))luaL_error(L,"Civis raw table library stack unavailable");
        lua_getfield(L,environment,"table");
        if(!lua_istable(L,-1))luaL_error(L,"Civis raw table count requires the inherited table library");
        const int inherited=lua_gettop(L);
        lua_pushliteral(L,"count");lua_rawget(L,inherited);
        if(!lua_isnil(L,-1))luaL_error(L,"Civis raw table count would replace an existing binding");
        lua_pop(L,1);lua_newtable(L);
        const int copied=lua_gettop(L);
        lua_pushnil(L);
        while(lua_next(L,inherited)){
            lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_rawset(L,copied);lua_pop(L,1);
        }
        lua_pushcfunction(L,civis_raw_table_count);lua_setfield(L,copied,"count");
        lua_setfield(L,environment,"table");lua_pop(L,1);
    }
    static int civis_selected_game_option(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;bool enabled=false;
        try {
            self.require_ui_callback_owner();
            if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TNUMBER||
               lua_tonumber(L,1)!=static_cast<lua_Number>(self.selected_option.no_religion_token))
                throw ScriptError("Civis selected Game.IsOption requires the one declared numeric NO_RELIGION token");
            enabled=self.selected_option.is_option(self.selected_option.no_religion_token);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis selected Game.IsOption failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        lua_pushboolean(L,enabled);return 1;
    }
    static void install_civis_selected_game_option(lua_State *L,int environment){
        if(!lua_checkstack(L,10))luaL_error(L,"Civis selected Game option binding stack unavailable");
        lua_getfield(L,environment,"GameOptionTypes");
        if(!lua_isnil(L,-1))luaL_error(L,"Civis selected Game option would replace an existing enum binding");
        lua_pop(L,1);lua_getfield(L,environment,"Game");
        if(!lua_istable(L,-1))luaL_error(L,"Civis selected Game option requires the actual gameplay Game owner");
        const int inherited=lua_gettop(L);
        lua_pushliteral(L,"IsOption");lua_rawget(L,inherited);
        if(!lua_isnil(L,-1))luaL_error(L,"Civis selected Game option would replace an existing getter");
        lua_pop(L,1);lua_newtable(L);const int copied=lua_gettop(L);lua_pushnil(L);
        while(lua_next(L,inherited)){
            lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_rawset(L,copied);lua_pop(L,1);
        }
        lua_pushcfunction(L,civis_selected_game_option);lua_setfield(L,copied,"IsOption");
        lua_setfield(L,environment,"Game");lua_pop(L,1);
        lua_newtable(L);lua_pushinteger(L,owner(L).selected_option.no_religion_token);
        lua_setfield(L,-2,"GAMEOPTION_NO_RELIGION");lua_setfield(L,environment,"GameOptionTypes");
    }
    void require_civis_units_call(lua_State *L)const {
        require_ui_callback_owner();
        if(failed||!calling||units_query_active_context!=units_query_context||
           (!units_query_source_loading&&!ready))
            throw ScriptError("Civis Units query requires its current selected source load or ready control callback");
        std::size_t size=0;const auto *context=lua_tolstring(L,lua_upvalueindex(2),&size);
        if(!context||std::string_view(context,size)!=units_query_context)
            throw ScriptError("Civis Units query closure has a different context identity");
    }
    static int civis_units_query_gc(lua_State *L){
        auto **query=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsQuery>**>(lua_touserdata(L,1));
        delete *query;*query=nullptr;return 0;
    }
    static int civis_units_iterator_gc(lua_State *L){
        auto **iterator=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsIterator>**>(lua_touserdata(L,1));
        delete *iterator;*iterator=nullptr;return 0;
    }
    static int civis_units_iterator_next(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {
            self.require_civis_units_call(L);
            // Generic-for supplies state/control arguments; no engine ABI is
            // inferred from them, and they never affect the bound era.
            if(lua_gettop(L)!=0&&lua_gettop(L)!=2)
                throw ScriptError("Civis Units iterator accepts no arguments or the two generic-for slots");
            auto **iterator=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsIterator>**>(lua_touserdata(L,lua_upvalueindex(1)));
            if(!iterator||!*iterator||!**iterator)throw ScriptError("Civis Units iterator is invalid");
            self.query_row=(**iterator)->next();
            if(self.query_row){validate_row(*self.query_row);++self.units_query_observed.rows;}
            else ++self.units_query_observed.eof;
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis Units iterator failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!self.query_row){lua_pushnil(L);return 1;}
        self.failed=true;push_row(L,*self.query_row);self.query_row.reset();self.failed=false;return 1;
    }
    static int civis_units_query_call(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;self.query_arguments.clear();
        try {
            self.require_civis_units_call(L);
            if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TSTRING)
                throw ScriptError("Civis Units query expects exactly one string era");
            std::size_t size=0;const auto *text=lua_tolstring(L,1,&size);
            self.query_arguments.emplace_back(std::string(text,size));
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis Units bind conversion failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!lua_checkstack(L,8)){self.failed=true;self.ready=false;return luaL_error(L,"Civis Units iterator stack unavailable");}
        // Establish a null GC-owned handle before invoking any C++ backend.
        // Allocation failure cannot leave an unowned cursor or a ready VM.
        self.failed=true;
        auto **iterator=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsIterator>**>(lua_newuserdata(L,sizeof(void*)));
        *iterator=nullptr;luaL_newmetatable(L,"Civis.DiagnosticUnitsIterator");
        lua_pushcfunction(L,civis_units_iterator_gc);lua_setfield(L,-2,"__gc");lua_setmetatable(L,-2);self.failed=false;
        try {
            auto **query=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsQuery>**>(lua_touserdata(L,lua_upvalueindex(1)));
            if(!query||!*query||!**query)throw ScriptError("Civis Units prepared query is invalid");
            *iterator=new std::shared_ptr<ui::CivisCivilopediaUnitsIterator>((**query)->iterate(self.query_arguments));
            if(!**iterator)throw ScriptError("Civis Units query returned an invalid iterator");
            self.require_civis_units_call(L);++self.units_query_observed.invocations;
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis Units query invocation failed";}
        self.query_arguments.clear();
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        self.failed=true;lua_pushvalue(L,lua_upvalueindex(2));lua_pushcclosure(L,civis_units_iterator_next,2);self.failed=false;return 1;
    }
    static int civis_units_query_prepare(lua_State *L){
        auto &self=owner(L);self.source_exception={};self.source_error=nullptr;
        try {
            self.require_civis_units_call(L);
            if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TSTRING)
                throw ScriptError("Civis Units CreateQuery expects exactly one declared SQL string");
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis Units query factory guard failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(!lua_checkstack(L,8)){self.failed=true;self.ready=false;return luaL_error(L,"Civis Units factory stack unavailable");}
        self.failed=true;
        auto **query=static_cast<std::shared_ptr<ui::CivisCivilopediaUnitsQuery>**>(lua_newuserdata(L,sizeof(void*)));
        *query=nullptr;luaL_newmetatable(L,"Civis.DiagnosticUnitsQuery");
        lua_pushcfunction(L,civis_units_query_gc);lua_setfield(L,-2,"__gc");lua_setmetatable(L,-2);self.failed=false;
        try {
            std::size_t size=0;const auto *sql=lua_tolstring(L,1,&size);
            *query=new std::shared_ptr<ui::CivisCivilopediaUnitsQuery>(self.units_query.prepare(std::string_view(sql,size)));
            if(!**query)throw ScriptError("Civis Units query factory returned an invalid handle");
            self.require_civis_units_call(L);++self.units_query_observed.prepared;
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Civis Units query preparation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        self.failed=true;lua_pushvalue(L,lua_upvalueindex(2));lua_pushcclosure(L,civis_units_query_call,2);self.failed=false;return 1;
    }
    static void install_civis_units_query(lua_State *L,int environment){
        if(!lua_checkstack(L,10))luaL_error(L,"Civis Units DB binding stack unavailable");
        lua_getfield(L,environment,"DB");
        if(!lua_istable(L,-1))luaL_error(L,"Civis Units CreateQuery requires an actual gameplay DB");
        const int inherited=lua_gettop(L);
        lua_pushliteral(L,"CreateQuery");lua_rawget(L,inherited);
        if(!lua_isnil(L,-1))luaL_error(L,"Civis Units CreateQuery would replace an existing binding");
        lua_pop(L,1);lua_newtable(L);const int copied=lua_gettop(L);lua_pushnil(L);
        while(lua_next(L,inherited)){
            lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_rawset(L,copied);lua_pop(L,1);
        }
        lua_pushnil(L);lua_pushlstring(L,owner(L).units_query_context.data(),owner(L).units_query_context.size());
        lua_pushcclosure(L,civis_units_query_prepare,2);lua_setfield(L,copied,"CreateQuery");
        lua_setfield(L,environment,"DB");lua_pop(L,1);
    }

    void require_ui_callback_owner()const{
        if(std::this_thread::get_id()!=ui_thread)throw ScriptError("UI callback must run on its owner thread");
        if(ui_context_closed)throw ScriptError("UI callback context is closed");
        if(!ui_layout.callback_owner_current||!ui_layout.callback_owner_current())
            throw ScriptError("UI callback context owner is stale");
    }
    void release_control_callbacks(){
        for(const auto &callback:control_callbacks)luaL_unref(state,LUA_REGISTRYINDEX,callback.function);
        control_callbacks.clear();
        for(auto *reference:{&text_function,&text_receiver,&input_function,&visibility_function,&tooltip_controls}){
            if(*reference!=LUA_NOREF)luaL_unref(state,LUA_REGISTRYINDEX,*reference);
            *reference=LUA_NOREF;
        }
    }
    static int ui_tooltip_table(lua_State *L){
        auto &self=owner(L);
        if(!lua_rawequal(L,1,lua_upvalueindex(2)))return luaL_error(L,"TTManager receiver differs from selected context");
        if(lua_gettop(L)!=3)return luaL_error(L,"Selected tooltip registration requires type and destination table");
        luaL_checktype(L,2,LUA_TSTRING);luaL_checktype(L,3,LUA_TTABLE);
        std::size_t length=0;const char *name=lua_tolstring(L,2,&length);
        if(std::string_view(name,length)!=self.tooltip.type_name)return luaL_error(L,"Tooltip type differs from selected XML");
        self.source_exception={};self.source_error=nullptr;
        try{self.require_ui_callback_owner();self.tooltip.registered();}
        catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Tooltip type registration failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        for(const auto &control:self.tooltip.controls){lua_getfield(L,lua_upvalueindex(1),control.id.c_str());lua_setfield(L,3,control.id.c_str());}
        return 0;
    }
    static int ui_tooltip_control(lua_State *L){
        auto &self=owner(L);const char *id=lua_tostring(L,lua_upvalueindex(1));
        const auto operation=lua_tointeger(L,lua_upvalueindex(2));
        if(!lua_rawequal(L,1,lua_upvalueindex(3)))return luaL_error(L,"Tooltip receiver differs from selected XML");
        if(lua_gettop(L)!=2)return luaL_error(L,"Tooltip setter overload differs from selected source");
        const char *texture=nullptr;std::size_t length=0;std::int64_t x=0,y=0;bool hidden=false;
        if(operation==0){luaL_checktype(L,2,LUA_TSTRING);texture=lua_tolstring(L,2,&length);}
        else if(operation==1){
            luaL_checktype(L,2,LUA_TTABLE);lua_getfield(L,2,"x");x=source_integer(L,-1);lua_pop(L,1);
            lua_getfield(L,2,"y");y=source_integer(L,-1);lua_pop(L,1);
        }else{luaL_checktype(L,2,LUA_TBOOLEAN);hidden=lua_toboolean(L,2)!=0;}
        self.source_exception={};self.source_error=nullptr;
        try{
            self.require_ui_callback_owner();
            if(operation==0)self.tooltip.set_texture(id,std::string_view(texture,length));
            else if(operation==1)self.tooltip.set_texture_offset(id,x,y);
            else self.tooltip.set_hidden(id,hidden);
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Tooltip control mutation failed";}
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int ui_text_entry(lua_State *L){
        auto &self=owner(L);const auto operation=lua_tointeger(L,lua_upvalueindex(1));
        if(!lua_rawequal(L,1,lua_upvalueindex(2)))return luaL_error(L,"EditBox receiver differs from selected XML");
        if(lua_gettop(L)!=(operation==1?2:1))return luaL_error(L,"Selected EditBox callback overload differs");
        if(operation==1)luaL_checktype(L,2,LUA_TFUNCTION);
        self.source_exception={};self.source_error=nullptr;int previous=LUA_NOREF;
        try{
            self.require_ui_callback_owner();
            if(operation==0)self.captured_text=self.text_entry.get_text();
            else{
                previous=self.text_function;self.text_function=LUA_NOREF;
                if(operation==1){lua_pushvalue(L,2);self.text_function=luaL_ref(L,LUA_REGISTRYINDEX);}
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Selected EditBox service failed";}
        if(previous!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,previous);
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(operation!=0)return 0;
        lua_pushlstring(L,self.captured_text.data(),self.captured_text.size());return 1;
    }
    static int ui_context_handler(lua_State *L){
        auto &self=owner(L);const auto operation=lua_tointeger(L,lua_upvalueindex(1));
        if(!lua_rawequal(L,1,lua_upvalueindex(2)))return luaL_error(L,"ContextPtr receiver differs from selected context");
        if(lua_gettop(L)!=(operation==0?1:2))return luaL_error(L,"Selected ContextPtr handler overload differs");
        if(operation!=0)luaL_checktype(L,2,LUA_TFUNCTION);
        self.source_exception={};self.source_error=nullptr;int previous=LUA_NOREF;bool hidden=false;
        try{
            self.require_ui_callback_owner();
            if(operation==0)hidden=self.ui_context.is_hidden();
            else{
                auto &reference=operation==1?self.input_function:self.visibility_function;
                previous=reference;reference=LUA_NOREF;
                lua_pushvalue(L,2);reference=luaL_ref(L,LUA_REGISTRYINDEX);
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="Selected ContextPtr service failed";}
        if(previous!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,previous);
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        if(operation!=0)return 0;
        lua_pushboolean(L,hidden);return 1;
    }
    static int ui_control_callback(lua_State *L){
        auto &self=owner(L);
        if(!lua_rawequal(L,1,lua_upvalueindex(3)))
            return luaL_error(L,"UI callback receiver differs from selected XML control");
        const char *id=lua_tostring(L,lua_upvalueindex(1));
        const auto operation=lua_tointeger(L,lua_upvalueindex(2));
        const auto event=source_integer(L,2);
        if(operation==0)luaL_checktype(L,3,LUA_TFUNCTION);
        self.source_exception={};self.source_error=nullptr;
        int previous=LUA_NOREF;
        try{
            self.require_ui_callback_owner();
            if(std::find(self.ui_layout.callback_events.begin(),self.ui_layout.callback_events.end(),event)==
               self.ui_layout.callback_events.end())throw ScriptError("UI control event is not declared by its owner");
            auto selected=std::find_if(self.control_callbacks.begin(),self.control_callbacks.end(),
                [&](const auto &callback){return callback.id==id&&callback.event==event;});
            if(operation==0){
                // Reserve the C++ slot before allocating a Lua reference. The
                // enclosing protected VM call handles a Lua allocation error.
                if(selected==self.control_callbacks.end()){
                    self.control_callbacks.push_back({id,event,LUA_NOREF});
                    selected=std::prev(self.control_callbacks.end());
                }
                previous=selected->function;
                lua_pushvalue(L,3);selected->function=luaL_ref(L,LUA_REGISTRYINDEX);
            }else if(selected!=self.control_callbacks.end()){
                previous=selected->function;self.control_callbacks.erase(selected);
            }
        }catch(const std::exception &e){self.source_exception=std::current_exception();self.source_error=e.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="UI callback registration failed";}
        if(previous!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,previous);
        if(self.source_error){self.failed=true;self.ready=false;return luaL_error(L,"%s",self.source_error);}
        return 0;
    }
    static int sum_values(lua_State *L){
        if(lua_toboolean(L,1)){
            const auto a=source_integer(L,2),b=source_integer(L,3);
            // Each input is at most 2^53, so the sum cannot overflow int64.
            const auto sum=a+b;
            if(sum>9007199254740992LL||sum<-9007199254740992LL)
                return luaL_error(L,"Lua accumulator exceeds the exact binary64 range");
            lua_pushinteger(L,sum);
        }else{
            // Widen the original float accumulator; no per-callback float32
            // narrowing or float32 range ceiling is inherited by Civis.
            const auto sum=lua_tonumber(L,2)+lua_tonumber(L,3);
            if(!std::isfinite(sum))return luaL_error(L,"Lua accumulator is nonfinite");
            lua_pushnumber(L,sum);
        }
        return 1;
    }
    static void push(lua_State *L,const ScriptValue &value) {
        switch(value.index()){
        case 0:lua_pushnil(L);break;
        case 1:lua_pushboolean(L,std::get<bool>(value));break;
        case 2:lua_pushinteger(L,std::get<std::int64_t>(value));break;
        case 3:lua_pushnumber(L,std::get<double>(value));break;
        case 4:lua_pushlstring(L,std::get<std::string>(value).data(),std::get<std::string>(value).size());break;
        }
    }
    static int perform(lua_State *L) {
        auto &job=*static_cast<Job*>(lua_touserdata(L,1));auto &self=*job.owner;
        // All allocating Lua calls stay inside lua_cpcall. Job owns nontrivial
        // C++ state outside that longjmp boundary; local Lua-call data is POD.
        try {
            child_require_bindings(L);
            if(job.operation==Operation::initialize){
                if(!lua_checkstack(L,32))return luaL_error(L,"Lua initialization stack unavailable");
                lua_pushcfunction(L,luaopen_base);lua_call(L,0,0);
                for(const auto open:{luaopen_table,luaopen_string,luaopen_math}){lua_pushcfunction(L,open);lua_call(L,0,0);}
                // Native startup5CE746 luaL_openlibs includes precisely debug too.
                // Already inside call(initialize)'s lua_cpcall; no unprotected open.
                lua_pushcfunction(L,luaopen_debug);lua_call(L,0,0);
                lua_getglobal(L,LUA_DBLIBNAME);
                if(!lua_istable(L,-1)||self.profile_debug_master_ref!=LUA_NOREF)
                    return luaL_error(L,"Actual master debug namespace initialization failed");
                self.profile_debug_master_table=lua_topointer(L,-1);
                self.profile_debug_master_ref=luaL_ref(L,LUA_REGISTRYINDEX);
                self.profile_debug_master_alive=true;
                install_native_master_extensions(L);
                install_native_master_threads(L,job);
                native_flua_exposure_bootstrap_owned(L,job); // original477C49/4F/57 typed,named,builtin
                for(const char *name:{"dofile","loadfile"}){lua_pushnil(L);lua_setglobal(L,name);}
                // Lua's CPU-time API, with 64-bit Windows process counters
                // instead of the CRT's wrapping 32-bit clock_t.
                // Original master startup opens actual os through luaL_openlibs.
                // Protected initialize supplies real date/difftime/time/tmpname.
                // Retain the established Civis wide process-clock policy.
                lua_pushcfunction(L,luaopen_os);lua_call(L,0,0);
                lua_getglobal(L,LUA_OSLIBNAME);
                if(!lua_istable(L,-1))return luaL_error(L,"Actual os master library absent");
                lua_pushcfunction(L,process_clock);lua_setfield(L,-2,"clock");lua_pop(L,1);
                lua_pushvalue(L,LUA_GLOBALSINDEX);self.base_environment=luaL_ref(L,LUA_REGISTRYINDEX);
                if(luaL_loadbuffer(L,lua_events_bootstrap,sizeof(lua_events_bootstrap)-1,"=Civis events"))return lua_error(L);
                lua_pushcfunction(L,invoke);
                if(self.sources.files){
                    if(luaL_loadbuffer(L,lua_include_bootstrap,sizeof(lua_include_bootstrap)-1,"=Civis include"))return lua_error(L);
                    // Retain the exact bootstrap Proto tree before its factory
                    // creates closures. Scripts cannot forge this identity.
                    self.include_bootstrap_proto=static_cast<const Closure*>(lua_topointer(L,-1))->l.p;
                    lua_pushvalue(L,-1);self.include_bootstrap_reference=luaL_ref(L,LUA_REGISTRYINDEX);
                    lua_pushcfunction(L,list_sources);lua_pushcfunction(L,compile_include_source);lua_pushcfunction(L,report_source_error);
                    lua_pushboolean(L,self.sources.strict_map_includes);
                    lua_pushcfunction(L,execution_log);lua_call(L,5,1);
                    // Protected producer returns actual same bootstrap factory.
                    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||
                       !graph_contains_proto(self.include_bootstrap_proto,static_cast<const Closure*>(lua_topointer(L,-1))->l.p))
                        return luaL_error(L,"Actual include configuration factory absent");
                    self.include_configuration_pointer=lua_topointer(L,-1);
                    lua_pushvalue(L,-1);self.include_configuration_ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self.include_configuration_alive=true;
                }else lua_pushnil(L);
                if(self.modding.entry_points||self.save_data.open||self.user_data.open){
                    if(luaL_loadbuffer(L,lua_modding_bootstrap,sizeof(lua_modding_bootstrap)-1,"=Civis Modding"))return lua_error(L);
                    if(self.modding.entry_points){lua_pushcfunction(L,mod_entry_points);lua_pushcfunction(L,evaluated_file);}
                    else {lua_pushnil(L);lua_pushnil(L);}
                    if(self.save_data.open)lua_pushcfunction(L,open_save_data);else lua_pushnil(L);
                    if(self.user_data.open)lua_pushcfunction(L,open_save_data);else lua_pushnil(L);
                    if(self.user_data.erase)lua_pushcfunction(L,delete_user_data);else lua_pushnil(L);
                    lua_call(L,5,1);
                    lua_pushcclosure(L,worldview_configure_api_coexisting_owned<0>,1);
                }else lua_pushnil(L);
                if(self.gameplay.query){
                    if(luaL_loadbuffer(L,lua_gameplay_bootstrap,sizeof(lua_gameplay_bootstrap)-1,"=Civis gameplay"))return lua_error(L);
                    lua_pushcfunction(L,make_database_query);lua_pushcfunction(L,gameplay_objects);
                    lua_pushcfunction(L,team_has_tech);lua_pushcfunction(L,player_get_team);
                    if(luaL_loadbuffer(L,lua_gameinfo_bootstrap,sizeof(lua_gameinfo_bootstrap)-1,"=Civis GameInfo"))return lua_error(L);
                    lua_pushcfunction(L,make_info_query);lua_pushcfunction(L,make_info_object);lua_pushcfunction(L,gameplay_info_types);lua_call(L,3,1);
                    lua_pushcclosure(L,worldview_configure_api_coexisting_owned<1>,1);
                    if(self.gameplay.game_turn)lua_pushcfunction(L,game_get_turn);else lua_pushnil(L);
                    if(self.gameplay.player_human)lua_pushcfunction(L,player_is_human);else lua_pushnil(L);
                    if(self.gameplay.player_alive)lua_pushcfunction(L,player_is_alive);else lua_pushnil(L);
                    if(self.gameplay.player_leader)lua_pushcfunction(L,player_get_leader);else lua_pushnil(L);
                    if(self.gameplay.game_speed)lua_pushcfunction(L,game_get_speed);else lua_pushnil(L);
                    if(self.gameplay.player_ever_alive)lua_pushcfunction(L,player_was_alive);else lua_pushnil(L);
                    if(self.gameplay.player_minor)lua_pushcfunction(L,player_is_minor);else lua_pushnil(L);
                    if(self.gameplay.player_minor_type)lua_pushcfunction(L,player_get_minor_type);else lua_pushnil(L);
                    if(self.gameplay.player_civilization)lua_pushcfunction(L,player_get_civilization);else lua_pushnil(L);
                    if(self.gameplay.units)lua_pushcfunction(L,player_get_unit);else lua_pushnil(L);
                    if(self.gameplay.units)lua_pushcfunction(L,player_unit_ids);else lua_pushnil(L);
                    lua_pushcfunction(L,team_set_has_tech);
                    lua_pushcfunction(L,native_gamecore_gameplay_preserve_callback_owned);lua_call(L,18,1);
                }else lua_pushnil(L);
                lua_pushcfunction(L,sum_values);
                if(luaL_loadbuffer(L,lua_custom_events_bootstrap,sizeof(lua_custom_events_bootstrap)-1,"=Civis LuaEvents"))return lua_error(L);
                lua_pushcfunction(L,invoke_lua_event);lua_call(L,1,1);
                retain_native_lua_events_factory(L);
                if(self.pregame.active_player){
                    if(luaL_loadbuffer(L,lua_pregame_bootstrap,sizeof(lua_pregame_bootstrap)-1,"=Civis pregame"))return lua_error(L);
                    for(int operation=0;operation<3;++operation){lua_pushinteger(L,operation);lua_pushcclosure(L,pregame_query,1);}
                    if(self.pregame.set_leader_type)lua_pushcfunction(L,pregame_set_leader);else lua_pushnil(L);
                    if(self.pregame.player_color){lua_pushinteger(L,3);lua_pushcclosure(L,pregame_query,1);}else lua_pushnil(L);
                    if(self.pregame.set_player_color)lua_pushcfunction(L,pregame_set_color);else lua_pushnil(L);
                    lua_pushcfunction(L,native_pregame_preserve_prepared_namespace_owned);
                    lua_call(L,7,1);
                }else lua_pushnil(L);
                if(self.map.grid_size){
                    if(self.map.plot){
                        if(!lua_checkstack(L,24))return luaL_error(L,"BNW live map argument stack is exhausted");
                        if(luaL_loadbuffer(L,lua_map_bootstrap,sizeof(lua_map_bootstrap)-1,"=Civis map"))return lua_error(L);
                        for(int operation=0;operation<8;++operation){lua_pushinteger(L,operation);lua_pushcclosure(L,map_query,1);}
                        lua_pushcfunction(L,plot_reference);
                        for(int operation=0;operation<6;++operation){lua_pushinteger(L,operation);lua_pushcclosure(L,plot_query,1);}
                        lua_pushboolean(L,self.map.capabilities.plot_index_alias);
                        if(self.map.resource_type)lua_pushcfunction(L,plot_resource);else lua_pushnil(L);
                        if(self.map.calculated_yield)lua_pushcfunction(L,plot_calculated_yield);else lua_pushnil(L);
                        if(self.map.calculated_yield)lua_pushcfunction(L,plot_is_revealed);else lua_pushnil(L);
                        if(self.map.calculated_yield){lua_pushinteger(L,1);lua_pushcclosure(L,game_active_read,1);}else lua_pushnil(L);
                        if(self.map.calculated_yield){lua_pushinteger(L,0);lua_pushcclosure(L,game_active_read,1);}else lua_pushnil(L);
                        lua_pushcfunction(L,native_gamecore_bootstrap_preserve_callback_owned);lua_call(L,22,1);
                    }else{
                        if(luaL_loadbuffer(L,lua_map_setup_bootstrap,sizeof(lua_map_setup_bootstrap)-1,
                                           "=Civis map setup"))return lua_error(L);
                        if(!lua_checkstack(L,40))return luaL_error(L,"BNW map setup argument stack is exhausted");
                        lua_pushinteger(L,0);lua_pushcclosure(L,map_query,1);
                        if(self.map.random)lua_pushcfunction(L,map_random);else lua_pushnil(L);
                        if(self.map.setup_world_size)lua_pushcfunction(L,map_setup_world_size);else lua_pushnil(L);
                        if(self.map.setup_custom_option)lua_pushcfunction(L,map_setup_custom_option);else lua_pushnil(L);
                        if(self.map.setup_worlds){
                            lua_createtable(L,static_cast<int>(self.map.setup_worlds->size()),0);
                            for(std::size_t i=0;i<self.map.setup_worlds->size();++i){
                                const auto &row=(*self.map.setup_worlds)[i];
                                lua_createtable(L,0,4);
                                lua_pushinteger(L,row.rowid);lua_setfield(L,-2,"ROWID");
                                lua_pushinteger(L,row.id);lua_setfield(L,-2,"ID");
                                lua_pushlstring(L,row.type.data(),row.type.size());lua_setfield(L,-2,"Type");
                                lua_pushinteger(L,row.feature_grain_change);lua_setfield(L,-2,"FeatureGrainChange");
                                lua_rawseti(L,-2,static_cast<int>(i+1));
                            }
                        }else lua_pushnil(L);
                        if(self.map.setup_wraps)lua_pushcfunction(L,map_setup_wraps);else lua_pushnil(L);
                        if(self.map.setup_fractal_create)lua_pushcfunction(L,map_setup_fractal_create);else lua_pushnil(L);
                        if(self.map.setup_fractal_height)lua_pushcfunction(L,map_setup_fractal_height);else lua_pushnil(L);
                        if(self.map.setup_fractal_ridges)lua_pushcfunction(L,map_setup_fractal_ridges);else lua_pushnil(L);
                        if(self.map.setup_plot_count)lua_pushcfunction(L,map_setup_plot_count);else lua_pushnil(L);
                        if(self.map.setup_plot_by_index)lua_pushcfunction(L,map_setup_plot_by_index);else lua_pushnil(L);
                        if(self.map.setup_plot_value)lua_pushcfunction(L,map_setup_plot_value);else lua_pushnil(L);
                        if(self.map.setup_plot_set_type)lua_pushcfunction(L,map_setup_plot_set_type);else lua_pushnil(L);
                        if(self.map.setup_define)lua_pushcfunction(L,map_setup_define);else lua_pushnil(L);
                        if(self.map.setup_plot_adjacent_land)lua_pushcfunction(L,map_setup_plot_adjacent_land);else lua_pushnil(L);
                        if(self.map.setup_plot_set_terrain)lua_pushcfunction(L,map_setup_plot_set_terrain);else lua_pushnil(L);
                        if(self.map.setup_plot_adjacent_shallow_water)lua_pushcfunction(L,map_setup_plot_adjacent_shallow_water);else lua_pushnil(L);
                        if(self.map.setup_terrain_info_type)lua_pushcfunction(L,map_setup_terrain_info_type);else lua_pushnil(L);
                        if(self.map.setup_plot_coastal_land)lua_pushcfunction(L,map_setup_plot_coastal_land);else lua_pushnil(L);
                        if(self.map.setup_find_water)lua_pushcfunction(L,map_setup_find_water);else lua_pushnil(L);
                        if(self.map.setup_plot_area_id)lua_pushcfunction(L,map_setup_plot_area_id);else lua_pushnil(L);
                        if(self.map.setup_area_value)lua_pushcfunction(L,map_setup_area_value);else lua_pushnil(L);
                        if(self.map.setup_plot_inland_corner)lua_pushcfunction(L,map_setup_plot_inland_corner);else lua_pushnil(L);
                        if(self.map.setup_plot_direction)lua_pushcfunction(L,map_setup_plot_direction);else lua_pushnil(L);
                        if(self.map.setup_plot_river_edge)lua_pushcfunction(L,map_setup_plot_river_edge);else lua_pushnil(L);
                        if(self.map.setup_plot_set_river_edge)lua_pushcfunction(L,map_setup_plot_set_river_edge);else lua_pushnil(L);
                        if(self.map.setup_plot_set_area)lua_pushcfunction(L,map_setup_plot_set_area);else lua_pushnil(L);
                        if(self.map.setup_calculate_areas)lua_pushcfunction(L,map_setup_calculate_areas);else lua_pushnil(L);
                        if(self.map.setup_feature_info_type)lua_pushcfunction(L,map_setup_feature_info_type);else lua_pushnil(L);
                        if(self.map.setup_find_biggest_area)lua_pushcfunction(L,map_setup_find_biggest_area);else lua_pushnil(L);
                        if(self.map.setup_plot_can_have_feature)lua_pushcfunction(L,map_setup_plot_can_have_feature);else lua_pushnil(L);
                        if(self.map.setup_climate)lua_pushinteger(L,*self.map.setup_climate);else lua_pushnil(L);
                        if(self.map.setup_climates){
                            lua_createtable(L,static_cast<int>(self.map.setup_climates->size()),0);
                            for(std::size_t i=0;i<self.map.setup_climates->size();++i){
                                const auto &row=(*self.map.setup_climates)[i];
                                lua_createtable(L,0,3);
                                lua_pushinteger(L,row.rowid);lua_setfield(L,-2,"ROWID");
                                lua_pushinteger(L,row.id);lua_setfield(L,-2,"ID");
                                lua_pushlstring(L,row.type.data(),row.type.size());lua_setfield(L,-2,"Type");
                                lua_rawseti(L,-2,static_cast<int>(i+1));
                            }
                        }else lua_pushnil(L);
                        if(self.map.setup_plot_set_feature_type)lua_pushcfunction(L,map_setup_plot_set_feature_type);else lua_pushnil(L);
                        if(self.map.setup_fractal_create_rifts)lua_pushcfunction(L,map_setup_fractal_create_rifts);else lua_pushnil(L);
                        if(self.map.setup_recalculate_areas)lua_pushcfunction(L,map_setup_recalculate_areas);else lua_pushnil(L);
                        if(self.map.setup_features){
                            lua_createtable(L,static_cast<int>(self.map.setup_features->size()),0);
                            for(std::size_t i=0;i<self.map.setup_features->size();++i){
                                const auto &row=(*self.map.setup_features)[i];
                                lua_createtable(L,0,3);
                                lua_pushinteger(L,row.rowid);lua_setfield(L,-2,"ROWID");
                                lua_pushinteger(L,row.id);lua_setfield(L,-2,"ID");
                                lua_pushlstring(L,row.type.data(),row.type.size());lua_setfield(L,-2,"Type");
                                lua_rawseti(L,-2,static_cast<int>(i+1));
                            }
                        }else lua_pushnil(L);
                        lua_call(L,37,1);
                    }
                }else lua_pushnil(L);
                if(self.content.available){
                    if(luaL_loadbuffer(L,lua_content_bootstrap,sizeof(lua_content_bootstrap)-1,
                                       "=Civis content"))return lua_error(L);
                    lua_pushcfunction(L,content_package_ids);
                    lua_pushcfunction(L,content_is_active);
                    lua_pushcfunction(L,content_description);
                    lua_pushinteger(L,self.content.gameplay_type);
                    lua_call(L,4,1);
                    lua_pushcclosure(L,worldview_configure_api_coexisting_owned<2>,1);
                }else lua_pushnil(L);
                if(!self.ui_notification_names.empty()||!self.ui_command_names.empty()){
                    if(luaL_loadbuffer(L,lua_ui_events_bootstrap,sizeof(lua_ui_events_bootstrap)-1,
                                       "=Civis UI events"))return lua_error(L);
                    lua_pushcfunction(L,invoke);lua_pushcfunction(L,ui_command);
                    lua_newtable(L);
                    for(const auto &name:self.ui_notification_names){lua_pushboolean(L,1);lua_setfield(L,-2,name.c_str());}
                    lua_newtable(L);
                    for(const auto &name:self.ui_command_names){lua_pushboolean(L,1);lua_setfield(L,-2,name.c_str());}
                    lua_pushcfunction(L,end_turn_event);lua_pushcfunction(L,worldview_created_event);lua_pushcfunction(L,worldview_damage_event);lua_call(L,7,1);
                    lua_pushvalue(L,-1);self.ui_functions=luaL_ref(L,LUA_REGISTRYINDEX);
                    lua_getfield(L,-1,"configure");lua_remove(L,-2);
                }else lua_pushnil(L);
                if(!self.enums.tables.empty())lua_pushcfunction(L,configure_enums);else lua_pushnil(L);
                if(self.locale.current){
                    if(luaL_loadbuffer(L,lua_locale_bootstrap,sizeof(lua_locale_bootstrap)-1,
                                       "=Civis locale"))return lua_error(L);
                    lua_pushcfunction(L,locale_current);lua_pushcfunction(L,locale_select);
                    if(self.locale.convert_text_key)lua_pushcfunction(L,locale_convert);else lua_pushnil(L);
                    lua_call(L,3,1);
                    lua_pushcclosure(L,worldview_configure_locale_coexisting_owned,1); // actual saved Lua configure closure
                }else lua_pushnil(L);
                lua_pushcfunction(L,record_message);
                lua_pushcfunction(L,execution_log);lua_call(L,14,1);self.functions=luaL_ref(L,LUA_REGISTRYINDEX);
            }else if(job.operation==Operation::validate_context){
                // Currentness bookends are performed by call on this VM thread.
            }else if(job.operation==Operation::validate_loading){
                // Only call's idle owner-thread job may consume this private
                // history. A public diagnostic copy cannot authorize completion.
                self.ready=false;
                if(self.failed)throw ScriptError("Lua content has a failed chunk; rebuild its context set");
                if(self.ui_context_closed)throw ScriptError("Lua UI context is closed; rebuild its context set");
                if(self.execution_uncertainty!=0)
                    throw ScriptError("Lua loading has retained execution uncertainty");
                for(const auto &record:self.execution_records) {
                    const bool load=record.action==ScriptLuaAction::context_execute||
                        record.action==ScriptLuaAction::include||record.action==ScriptLuaAction::child_load;
                    if(load&&(record.outcome!=ScriptLuaActionOutcome::succeeded||record.end==0))
                        throw ScriptError("Lua loading has an incomplete or failed source execution");
                }
            }else if(job.operation==Operation::native_context_direct_lua_execute||job.operation==Operation::native_context_direct_lua_showhide||job.operation==Operation::native_context_direct_lua_cleanup){
                perform_native_context_direct_lua_owned(L,job);
            }else if(job.operation==Operation::native_context_register_gc){
                perform_native_context_register_gc(L,job);
            }else if(job.operation==Operation::native_context_initial_state_query){
                perform_native_context_initial_state_query(L,job);
            }else if(job.operation==Operation::native_context_loader_temporary_query){
                perform_context_loader_temporary_query(L,job);
            }else if(job.operation==Operation::native_context_loader_receiver_unref||job.operation==Operation::native_context_loader_environment_unref){
                perform_context_loader_temporary_unref(L,job);
            }else if(job.operation==Operation::native_context_postread_contextptr_acquire||job.operation==Operation::native_context_postread_contextptr_query){
                perform_native_postread_contextptr_owned(L,job);
            }else if(job.operation==Operation::native_context_postread_registrations_acquire){
                perform_native_postread_registrations_owned(L,job);
            }else if(job.operation==Operation::native_context_postread_root_cdata_create||job.operation==Operation::native_context_postread_root_cdata_query){
                perform_native_postread_root_cdata(L,job);
            }else if(job.operation==Operation::native_context_postread_globals_acquire||job.operation==Operation::native_context_postread_globals_query){
                perform_native_postread_globals(L,job);
            }else if(job.operation==Operation::native_context_state_prepare){
                perform_native_context_state_prepare(L,job);
            }else if(job.operation==Operation::native_context_debug_config_prefix){
                perform_native_context_debug_config_prefix(L,job);
            }else if(job.operation==Operation::native_context_profile_restart){
                perform_native_context_profile_restart(L,job);
            }else if(job.operation==Operation::native_context_profile_completed_query){
                perform_native_context_profile_completed_query(L,job);
            }else if(job.operation==Operation::native_context_debug_config_query){
                perform_native_context_debug_config_query(L,job);
            }else if(job.operation==Operation::native_context_events_factory_prefix){
                perform_native_context_events_factory_prefix(L,job);
            }else if(job.operation==Operation::native_context_modding_registration){
                perform_native_context_modding_registration(L,job);
            }else if(job.operation==Operation::native_context_flua_registration_prefix){
                perform_native_context_flua_registration_prefix(L,job);
            }else if(job.operation==Operation::native_context_options_registration){
                perform_native_context_options_registration(L,job);
            }else if(job.operation==Operation::native_context_ui_registration){
                perform_native_context_ui_registration(L,job);
            }else if(job.operation==Operation::native_context_matchmaking_registration){
                perform_native_context_matchmaking_registration(L,job);
            }else if(job.operation==Operation::native_context_path_registration){
                perform_native_context_path_registration(L,job);
            }else if(job.operation==Operation::native_context_profiler_registration){
                perform_native_context_profiler_registration(L,job);
            }else if(job.operation==Operation::native_context_pregame_registration){
                perform_native_context_pregame_registration(L,job);
            }else if(job.operation==Operation::native_context_content_registration){
                perform_native_context_content_registration(L,job);
            }else if(job.operation==Operation::native_context_locale_registration){
                perform_native_context_locale_registration(L,job);
            }else if(job.operation==Operation::native_context_steam_registration){
                perform_native_context_steam_registration(L,job);
            }else if(job.operation==Operation::native_context_network_registration){
                perform_native_context_network_registration(L,job);
            }else if(job.operation==Operation::native_context_ui_enums_registration){
                perform_native_context_ui_enums_registration(L,job);
            }else if(job.operation==Operation::native_context_database_namespaces){
                perform_native_context_database_namespaces(L,job);
            }else if(job.operation==Operation::native_context_db_registration){
                perform_native_context_db_registration(L,job);
            }else if(job.operation==Operation::native_context_api_registration_prefix){
                perform_native_context_api_registration_prefix(L,job);
            }else if(job.operation==Operation::native_context_game_events_registration){
                perform_native_context_game_events_registration(L,job);
            }else if(job.operation==Operation::native_context_gamecore_registration){
                perform_native_context_gamecore_registration(L,job);
            }else if(job.operation==Operation::native_context_lua_events_registration){
                perform_native_context_lua_events_registration(L,job);
            }else if(job.operation==Operation::native_context_controls_statement){
                perform_native_context_controls_statement(L,job);
            }else if(job.operation==Operation::native_context_controls_table_query){
                native_controls_full(L);require_native_precontrols_registration_return(L);
                native_controls_physical_query(L,job);
            }else if(job.operation==Operation::native_context_controls_named_lookup){
                perform_native_context_controls_named_lookup(L,job);
            }else if(job.operation==Operation::native_context_entered_controls_named_lookup){
                perform_native_entered_controls_named_lookup(L,job);
            }else if(job.operation==Operation::native_context_controls_named_retire){
                perform_native_controls_named_retire(L,job);
            }else if(job.operation==Operation::native_child_context_controls_export){
                perform_native_child_context_controls_export(L,job);
            }else if(job.operation==Operation::native_root_ordinary_controls_export){
                perform_native_root_ordinary_controls_export(L,job);
            }else if(job.operation==Operation::native_context_setup_state_name){
                perform_native_context_setup_state_name(L,job);
            }else if(job.operation==Operation::native_context_setup_state_name_query){
                perform_native_context_setup_state_name_query(L,job);
            }else if(job.operation==Operation::native_context_state_query){
                perform_native_context_state_query(L,job);
            }else if(job.operation==Operation::native_worldview_load){
                perform_worldview_context_load(L,job);
            }else if(job.operation==Operation::execute){
                execute_context(L,job.context,job.source,job.name);
            }else if(job.operation==Operation::map_init_query){
                perform_map_init_query(L,job);
                if(job.exception)return 0; // exception rethrow belongs outside Lua C frames

            }else if(job.operation==Operation::team_set_has_tech){
                if(job.args.size()!=5||!self.executed_contexts.contains(std::string(job.context)))
                    throw ScriptError("Team:SetHasTech native command requires actual retained context");
                lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"create");
                lua_pushlstring(L,job.context.data(),job.context.size());lua_pushnil(L);lua_call(L,2,1);
                lua_getfield(L,-1,"Teams");push(L,job.args[0]);lua_rawget(L,-2);
                if(!lua_istable(L,-1))throw ScriptError("Team:SetHasTech genuine retained Team SID absent");
                const int object=lua_gettop(L);
                lua_getfield(L,object,"SetHasTech");
                if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=team_set_has_tech)
                    throw ScriptError("Team:SetHasTech retained method is not actual native dispatcher");
                lua_pushvalue(L,object);
                for(std::size_t i=1;i<job.args.size();++i)push(L,job.args[i]);
                lua_call(L,5,0);
            }else if(job.operation==Operation::update){
                job.integer=0;self.update_epoch=self.failure_epoch;
                self.update_require_current(false);
                // One consume for the whole batch, never caller context/delta.
                self.update_frame=self.update_sources->consume_frame();
                self.update_require_current(false);
                if(self.update_frame) {
                    if(&self.update_frame->issuer()!=self.update_sources.get())throw ScriptError("Foreign update frame issuer");
                    self.update_frame->require_current(*self.context_sources);
                    self.update_sequence=self.update_frame->sequence();self.update_delta=self.update_frame->delta_seconds();
                    if(!std::isfinite(self.update_delta)||self.update_delta<0)throw ScriptError("Update delta must be finite and nonnegative");
                    if(self.update_last_sequence&&self.update_sequence<=*self.update_last_sequence)
                        throw ScriptError("Update frame sequence is stale or already consumed");
                    self.update_require_current(true);
                    // Reserve freshness before Lua can cause partial effects.
                    self.update_last_sequence=self.update_sequence;
                    const auto count=self.child_contexts.size();
                    for(std::size_t slot=0;slot<count;++slot) {
                        auto &entry=self.child_contexts[slot];
                        if(entry.update_function==LUA_NOREF)continue;
                        self.update_require_context_current(slot);self.update_require_current(true);child_require_bindings(L);
                        lua_State *thread=lua_newthread(L);
                        lua_rawgeti(thread,LUA_REGISTRYINDEX,entry.update_function);
                        if(!lua_isfunction(thread,-1))return luaL_error(L,"Update callback retired");
                        lua_getfenv(thread,-1);lua_replace(thread,LUA_GLOBALSINDEX);
                        if(self.budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
                        lua_pushnumber(thread,self.update_delta);
                        const auto previous_scope=self.graph_execution_scope;
                        self.graph_execution_scope=self.graph_compiled_owner(thread,1);
                        self.units_query_active_context=entry.name;self.units_query_source_loading=false;
                        *job.integer+=1;
                        const int status=lua_pcall(thread,1,0,0);
                        self.graph_execution_scope=previous_scope;self.units_query_active_context={};
                        if(status){lua_xmove(thread,L,1);return lua_error(L);}
                        lua_pop(L,1); // completed callback thread; VM/closures remain shared
                        child_require_bindings(L);self.update_require_context_current(slot);self.update_require_current(true);
                    }
                    self.update_require_current(true);
                }
                if(self.update_epoch!=self.failure_epoch)throw ScriptError("Update frame owner caught host reentry");
            }else if(job.operation==Operation::control){
                if(!lua_checkstack(L,static_cast<int>(job.args.size())+8))return luaL_error(L,"Lua control argument stack unavailable");
                lua_rawgeti(L,LUA_REGISTRYINDEX,job.control_function);
                if(!lua_isfunction(L,-1))return luaL_error(L,"UI control callback was unregistered");
                lua_State *thread=lua_newthread(L);
                lua_pushvalue(L,-2);lua_xmove(L,thread,1);
                lua_getfenv(thread,-1);lua_replace(thread,LUA_GLOBALSINDEX);
                if(!lua_checkstack(thread,static_cast<int>(job.args.size())+8))
                    return luaL_error(L,"Lua control callback thread stack unavailable");
                if(self.budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
                for(const auto &arg:job.args)push(thread,arg);
                if(job.control_receiver!=LUA_NOREF)lua_rawgeti(thread,LUA_REGISTRYINDEX,job.control_receiver);
                self.graph_execution_scope=self.graph_compiled_owner(thread,1);
                // Reserve the attempt before Lua executes any external UI
                // command. A failing closure may already have caused effects.
                job.integer=1;
                if(lua_pcall(thread,static_cast<int>(job.args.size())+(job.control_receiver!=LUA_NOREF),job.input_result?1:0,0)){
                    lua_xmove(thread,L,1);return lua_error(L);
                }
                if(job.input_result){
                    if(!lua_isnil(thread,-1)&&!lua_isboolean(thread,-1))return luaL_error(L,"UI input handler must return boolean or nil");
                    job.handled=lua_toboolean(thread,-1)!=0;
                }
            }else if(job.operation==Operation::engine_end_turn_dirty){
                self.perform_end_turn_dirty(L);
            }else if(job.operation==Operation::emit_ui){
                if(!lua_checkstack(L,static_cast<int>(job.args.size())+8))return luaL_error(L,"Lua UI argument stack unavailable");
                lua_rawgeti(L,LUA_REGISTRYINDEX,self.ui_functions);lua_getfield(L,-1,"dispatch");
                lua_pushlstring(L,job.name.data(),job.name.size());
                for(const auto &arg:job.args)push(L,arg);
                lua_call(L,static_cast<int>(job.args.size())+1,2);
                job.integer=source_integer(L,-2);
                const auto count=lua_objlen(L,-1);
                for(std::size_t i=1;i<=count;++i){
                    lua_pushnumber(L,static_cast<lua_Number>(i));lua_rawget(L,-2);
                    std::size_t size=0;const char *message=lua_tolstring(L,-1,&size);
                    if(message)job.result.errors.emplace_back(message,size);lua_pop(L,1);
                }
            }else{
                if(!lua_checkstack(L,static_cast<int>(job.args.size())+8))return luaL_error(L,"Lua argument stack unavailable");
                lua_rawgeti(L,LUA_REGISTRYINDEX,self.functions);lua_getfield(L,-1,"dispatch");
                const char *mode=job.operation==Operation::call_hook?"Call":job.operation==Operation::test_any?"TestAny":
                    job.operation==Operation::accumulate_int?"AccumulateINT":job.operation==Operation::accumulate?"Accumulate":"TestAll";
                lua_pushstring(L,mode);
                lua_pushlstring(L,job.name.data(),job.name.size());for(const auto &arg:job.args)push(L,arg);
                lua_call(L,static_cast<int>(job.args.size())+2,2);
                if(!lua_isnil(L,-2)){
                    if(job.operation==Operation::accumulate_int)job.integer=source_integer(L,-2);
                    else if(job.operation==Operation::accumulate)job.number=lua_tonumber(L,-2);
                    else job.result.value=lua_toboolean(L,-2)!=0;
                }
                const auto count=lua_objlen(L,-1);
                for(std::size_t i=1;i<=count;++i){
                    lua_pushnumber(L,static_cast<lua_Number>(i));lua_rawget(L,-2);
                    std::size_t size=0;const char *message=lua_tolstring(L,-1,&size);
                    if(message)job.result.errors.emplace_back(message,size);lua_pop(L,1);
                }
            }
            child_require_bindings(L);
        }catch(...){job.exception=std::current_exception();}
        return 0;
    }
    explicit Impl(ScriptBudget limit,ScriptSourceProvider provider,ScriptModdingProvider mods,ScriptGameplayProvider world,ScriptSaveDataProvider saved,ScriptPregameProvider setup,ScriptMapProvider plots,ScriptUiEventProvider ui,ScriptContentProvider packages,ScriptUserDataProvider user,ScriptEnumProvider gamecore_enums,ScriptLocaleProvider language,ScriptUiLayoutProvider layout)
        :budget(limit),sources(std::move(provider)),modding(std::move(mods)),gameplay(std::move(world)),save_data(std::move(saved)),user_data(std::move(user)),pregame(std::move(setup)),map(std::move(plots)),content(std::move(packages)),enums(std::move(gamecore_enums)),locale(std::move(language)),ui_events(std::move(ui)),ui_layout(std::move(layout)) {
        initialize_native_definition_deferred_queue_owned(); // genuine fresh canonical Host queue; no Original process/default count
        initialize_native_context_cdata_creator_owned(); // actual own static476380/9BAE20 equivalent
        initialize_native_ordinary_cdata_creators_owned(); // six proven native static name/creator registrations
        initialize_native_ordinary_type_descriptors_owned(); // six actual descriptor constructor/name setter returns
        initialize_native_ordinary_descriptor_prefixes_owned(); // genuine always54C then nonNULL548 initializers
        initialize_native_ordinary_517c_functions_owned(); // actual native closure bodies and __tostring/__eq/__index/BaseType keys
        initialize_native_context_definition_owned(); // actual Context prefix/15 records + distinct ControlBase name/49 records; no library/current Base548 grant
        initialize_native_control_base_constructor_owned(); // actual new5210F0 live stores/callback cells/queued7A4DF0/name/49 records; no library return
        initialize_native_ordinary_pointer_formatter_owned(); // actual505B40 fixed9DFDC0 format and typed Windows CRT body
        initialize_native_ordinary_codebuddy_callback_owned(); // actual8F7C80 GETTABLE/protected1,1/A8E420 body
        initialize_native_master_protected_call_before_vm_owned();
        initialize_native_flua_typed_registry_before_vm_owned();
        initialize_native_flua_named_registry_before_vm_owned();
        initialize_native_flua_builtin_registry_before_vm_owned();
        if(!ui_layout.context_id.empty()){
            if(!ui_layout.screen_size||!ui_layout.get_size||!ui_layout.set_size||
               !ui_layout.set_offset||!ui_layout.calculate_internal_size)
                throw ScriptError("Lua UI layout requires a complete frame-owned provider");
            std::set<std::string> control_ids;
            for(const auto &control:ui_layout.controls)
                if(control.id.empty()||control.kind.empty()||
                   control.id.find('\0')!=control.id.npos||
                   !control_ids.insert(control.id).second)
                    throw ScriptError("Lua UI layout control ID is missing or duplicated");
        }else if(!ui_layout.controls.empty()||ui_layout.screen_size||ui_layout.get_size||
                 ui_layout.set_size||ui_layout.set_offset||ui_layout.calculate_internal_size||
                 !ui_layout.callback_events.empty()||ui_layout.callback_owner_current)
            throw ScriptError("Lua UI layout has no context owner");
        const bool control_callbacks_enabled=std::any_of(ui_layout.controls.begin(),ui_layout.controls.end(),
            [](const auto &control){return control.has_callbacks;});
        if(control_callbacks_enabled&&(!ui_layout.callback_owner_current||ui_layout.callback_events.empty()))
            throw ScriptError("Lua UI control callbacks require declared events and a current context owner");
        if(!control_callbacks_enabled&&(!ui_layout.callback_events.empty()||ui_layout.callback_owner_current))
            throw ScriptError("Lua UI callback provider has no declared controls");
        std::set<std::int64_t> callback_events;
        for(const auto event:ui_layout.callback_events){
            validate_value(event);
            if(!callback_events.insert(event).second)throw ScriptError("Duplicate Lua UI control callback event");
        }
        if(static_cast<bool>(locale.current)!=static_cast<bool>(locale.select))
            throw ScriptError("Lua locale requires both language query and selection");
        if(locale.convert_text_key&&!locale.current)
            throw ScriptError("Lua text conversion requires a selected locale");
        std::set<std::string> enum_names;
        for(const auto &table:enums.tables){
            if(table.name.empty()||table.name.find('\0')!=table.name.npos||!enum_names.insert(table.name).second)
                throw ScriptError("Invalid or duplicate Lua GameCore enum table");
            std::set<std::string> member_names;
            for(const auto &[name,value]:table.members){
                if(name.empty()||name.find('\0')!=name.npos||!member_names.insert(name).second)
                    throw ScriptError("Invalid or duplicate Lua GameCore enum member");
                validate_value(value);
            }
        }
        if(static_cast<bool>(user_data.open)!=static_cast<bool>(user_data.erase))
            throw ScriptError("Lua user data requires both open and delete capabilities");
        if(!content.available&&!content.packages.empty())throw ScriptError("Lua content inventory is unavailable");
        validate_value(content.gameplay_type);
        std::set<std::string> content_ids;
        for(const auto &package:content.packages){
            if(package.id.empty()||package.id.find('\0')!=package.id.npos||package.description.find('\0')!=package.description.npos)
                throw ScriptError("Invalid Lua content package");
            auto id=package.id;
            for(auto &c:id)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
            if(!content_ids.insert(std::move(id)).second)throw ScriptError("Duplicate Lua content package");
        }
        if(static_cast<bool>(sources.files)!=static_cast<bool>(sources.read))throw ScriptError("Lua source provider requires both enumeration and reading");
        if(static_cast<bool>(modding.entry_points)!=static_cast<bool>(modding.evaluated_file))throw ScriptError("Lua Modding provider requires both queries");
        if(static_cast<bool>(pregame.active_player)!=static_cast<bool>(pregame.slot_status)||
           static_cast<bool>(pregame.active_player)!=static_cast<bool>(pregame.civilization))
            throw ScriptError("Lua pregame provider requires active player, slot status and civilization");
        if(pregame.set_leader_type&&!pregame.active_player)
            throw ScriptError("Lua pregame leader mutation requires the shared roster");
        if((pregame.player_color||pregame.set_player_color)&&!pregame.active_player)
            throw ScriptError("Lua pregame player color requires the shared roster");
        const auto map_fields=static_cast<int>(!!map.grid_size)+!!map.wraps+!!map.plot+!!map.plot_by_index+!!map.direction+!!map.distance+!!map.plot_value;
        if(map_fields!=0&&map_fields!=1&&map_fields!=7)
            throw ScriptError("Lua map provider requires setup grid or complete geometry and plot getters");
        if(map.random&&map_fields!=1)
            throw ScriptError("Lua map random requires a setup-only map provider");
        if((map.setup_world_size||map.setup_custom_option||map.setup_worlds||map.setup_wraps||
            map.setup_fractal_create||map.setup_fractal_height||map.setup_fractal_ridges||map.setup_fractal_create_rifts||
            map.setup_plot_count||map.setup_plot_by_index||map.setup_plot_value||
            map.setup_plot_set_type||map.setup_define||map.setup_plot_adjacent_land||
            map.setup_plot_set_terrain||map.setup_plot_adjacent_shallow_water||
            map.setup_terrain_info_type||map.setup_plot_coastal_land||
            map.setup_find_water||map.setup_plot_area_id||map.setup_area_value||
            map.setup_plot_inland_corner||map.setup_plot_direction||
            map.setup_plot_river_edge||map.setup_plot_set_river_edge||
            map.setup_plot_set_area||map.setup_calculate_areas||map.setup_recalculate_areas||
            map.setup_feature_info_type||map.setup_find_biggest_area||
            map.setup_plot_can_have_feature||map.setup_climate||
            map.setup_climates||map.setup_features||map.setup_plot_set_feature_type)&&map_fields!=1)
            throw ScriptError("Lua map setup services require a setup-only map provider");
        if(map.setup_plot_adjacent_shallow_water&&!map.setup_plot_value)
            throw ScriptError("Lua setup shallow-water adjacency requires plot state queries");
        if(map.setup_plot_adjacent_land&&!map.setup_plot_value)
            throw ScriptError("Lua setup adjacency requires plot state queries");
        if((map.setup_plot_coastal_land||map.setup_find_water||
            map.setup_plot_area_id||map.setup_area_value||
            map.setup_plot_inland_corner||map.setup_plot_direction||
            map.setup_plot_river_edge||map.setup_plot_set_river_edge)&&!map.setup_plot_value)
            throw ScriptError("Lua setup river queries require plot state queries");
        if(static_cast<bool>(map.setup_plot_area_id)!=static_cast<bool>(map.setup_area_value))
            throw ScriptError("Lua setup area lookup and value queries require both callbacks");
        if(map.setup_plot_set_terrain&&!map.setup_plot_value)
            throw ScriptError("Lua setup terrain setter requires plot state queries");
        if(map.setup_plot_set_type&&!map.setup_plot_value)
            throw ScriptError("Lua setup plot setter requires plot state queries");
        if(map.setup_plot_set_area&&(!map.setup_plot_value||!map.setup_plot_area_id||
                                     !map.setup_area_value))
            throw ScriptError("Lua setup SetArea requires selected plot and area state");
        if(map.setup_recalculate_areas&&(!map.setup_plot_value||!map.setup_plot_area_id||!map.setup_area_value))
            throw ScriptError("Lua setup RecalculateAreas requires selected plot and area state");
        if(map.setup_calculate_areas&&(!map.setup_plot_value||!map.setup_plot_area_id||
                                       !map.setup_area_value))
            throw ScriptError("Lua setup CalculateAreas requires selected plot and area state");
        if(map.setup_find_biggest_area&&(!map.setup_plot_value||!map.setup_plot_area_id||
                                         !map.setup_area_value))
            throw ScriptError("Lua setup FindBiggestArea requires selected plot and area state");
        if(map.setup_plot_can_have_feature&&(!map.setup_plot_value||
                                             !map.setup_plot_area_id||
                                             !map.setup_area_value||
                                             !map.setup_plot_coastal_land))
            throw ScriptError("Lua setup CanHaveFeature requires selected plot and area state");
        if(map.setup_plot_set_feature_type&&(!map.setup_plot_value||
                                             !map.setup_plot_can_have_feature||
                                             !map.setup_feature_info_type))
            throw ScriptError("Lua setup SetFeatureType requires selected feature and plot state");
        if(map.setup_features){
            if(!map.setup_worlds||!map.setup_feature_info_type||map.setup_features->size()>128)
                throw ScriptError("Lua setup Features iterator requires selected feature and world rules");
            std::set<std::int64_t> ids;std::set<std::string> types;
            for(const auto &row:*map.setup_features)
                if(row.rowid!=row.id||row.id<0||row.id>127||row.type.empty()||
                    !ids.insert(row.id).second||!types.insert(row.type).second)
                    throw ScriptError("Lua setup Features iterator rows differ from selected Info slots");
        }
        if(map.setup_climate&&!map.setup_climates)
            throw ScriptError("Lua setup climate choice requires selected Climates rows");
        if(map.setup_climates&&!map.setup_worlds)
            throw ScriptError("Lua setup Climates rows require selected BNW world rules");
        if(static_cast<bool>(map.setup_plot_by_index)!=static_cast<bool>(map.setup_plot_value))
            throw ScriptError("Lua setup plot lookup and state queries require both callbacks");
        if(static_cast<bool>(map.setup_fractal_create)!=static_cast<bool>(map.setup_fractal_height))
            throw ScriptError("Lua map fractal creation and height queries require both callbacks");
        if(map.setup_fractal_ridges&&!map.setup_fractal_create)
            throw ScriptError("Lua map ridge generation requires fractal creation");
        if(map.setup_fractal_create_rifts&&!map.setup_fractal_create)
            throw ScriptError("Lua map rift generation requires fractal creation and height queries");
        if(map.resource_type&&map_fields!=7)throw ScriptError("Lua resource query requires the shared map provider");
        if(map.calculated_yield&&map_fields!=7)throw ScriptError("Lua calculated yield requires the real live map provider");
        if((map.capabilities.team_impassability_argument||map.capabilities.plot_index_alias)&&map_fields!=7)
            throw ScriptError("Lua map compatibility capabilities require the shared map provider");
        const auto add_ui_name=[](std::set<std::string> &names,const std::string &name){
            if(name.empty()||name.find('\0')!=name.npos||!names.insert(name).second)
                throw ScriptError("Invalid or duplicate UI event capability");
        };
        for(const auto &name:ui_events.notifications)add_ui_name(ui_notification_names,name);
        for(const auto &name:ui_events.commands){
            add_ui_name(ui_command_names,name);
            if(ui_notification_names.contains(name))throw ScriptError("UI notification and command names overlap");
        }
        if(static_cast<bool>(ui_events.command)!=!ui_command_names.empty())
            throw ScriptError("UI commands require a graphics command handler");
        if(static_cast<bool>(gameplay.query)!=static_cast<bool>(gameplay.ids)||static_cast<bool>(gameplay.query)!=static_cast<bool>(gameplay.has_tech)||
           static_cast<bool>(gameplay.query)!=static_cast<bool>(gameplay.player_team)||static_cast<bool>(gameplay.query)!=static_cast<bool>(gameplay.info_types))
            throw ScriptError("Lua gameplay provider requires database, type catalog, roster and live getters");
        initialize_native_master_reference_bank_before_vm_owned();
        initialize_native_master_thread_ledger_before_vm_owned();
        state=lua_newstate(allocate,this);if(!state)throw ScriptError("Cannot allocate Lua state");
        try {
            bind_native_master_reference_bank_actual_vm_owned();
            bind_native_master_thread_ledger_actual_vm_owned();
            bind_native_flua_typed_registry_actual_vm_owned();
            bind_native_flua_named_registry_actual_vm_owned();
            bind_native_flua_builtin_registry_actual_vm_owned();
            if(map.calculated_yield){
                calculated_plot_vm=BnwLuaCalculatedPlotYieldOwner::make_vm_lease(state);
                map.calculated_yield->bind_vm_lease(calculated_plot_vm);
            }
            Job job{this,Operation::initialize,{},{},{},{},{},{}};call(job);
        }catch(...){BnwLuaCalculatedPlotYieldOwner::retire_vm_lease(calculated_plot_vm);native_master_thread_ledger_retire();native_master_reference_bank_retire();lua_close(state);state=nullptr;throw;}
    }
    void require_ui_ready(std::string_view context)const{
        if(context.empty()||context!=ui_layout.context_id)throw ScriptError("UI input context differs from selected owner");
        require_ui_callback_owner();
        if(calling)throw ScriptError("UI input dispatch is reentrant");
        if(!ready||failed)throw ScriptError("UI input VM has not completed loading");
    }
    ScriptUiInputResult invoke_ui(int function,std::span<const ScriptValue> args,int receiver=LUA_NOREF,bool input=false){
        if(function==LUA_NOREF)throw ScriptError("UI input callback is not registered");
        Job job{this,Operation::control,{},{},{},args,{},{}};
        job.control_function=function;job.control_receiver=receiver;job.input_result=input;
        ScriptUiInputResult result;
        try{call(job);result.listeners=job.integer.value_or(0);require_ui_callback_owner();result.handled=job.handled;}
        catch(const std::exception &error){
            result.listeners=job.integer.value_or(0);result.errors.emplace_back(error.what());failed=true;ready=false;release_control_callbacks();
        }catch(...){
            result.listeners=job.integer.value_or(0);result.errors.emplace_back("UI input callback failed");failed=true;ready=false;release_control_callbacks();
        }
        return result;
    }
    ~Impl(){native_selected_registrar_call.reset();native_pregame_registration_retire();profiler_registration_frame=nullptr;path_registration_frame=nullptr;matchmaking_frame=nullptr;native_context_ui_registration_retire();options_frame=nullptr;native_flua_prefix_retire();native_flua_builtin_registry_retire();native_flua_named_registry_retire();native_flua_typed_registry_retire();native_flua_exposure_registration_retire();native_control_base_tooltip_retire();native_control_base_sort_retire();native_control_base_color_decoder_retire();native_control_base_setoffset_decoder_retire();native_control_base_mutable_library_prefix_retire();native_control_base_constructor_retire();native_ordinary_codebuddy_retire();native_ordinary_metatype_retire();for(const auto& type:native_ordinary_cdata_creators)if(type)type->alive.store(false);root_ordinary_controls_export_frame=nullptr;if(native_context_cdata_creator)native_context_cdata_creator->alive.store(false);child_controls_export_frame=nullptr;entered_controls_frame=nullptr;controls_named_retire_frame=nullptr;native_lua_event_retire();native_ordered_retire();native_game_view_retire();native_ai_step_retire();native_ai_ended_retire();native_ai_started_retire();native_strategic_view_retire();native_state_request_retire();native_state_dump_retire();native_sequence_init_retire();native_building_swap_retire();native_unit_swap_retire();native_landmark_swap_retire();native_unit_scale_retire();native_display_units_retire();native_wonder_placement_retire();native_wonder_state_retire();native_wonder_type_retire();native_unit_type_retire();native_unit_handle_retire();native_city_handle_retire();native_record_commands_retire();native_dont_record_commands_retire();native_animation_sampling_retire();native_camera_projection_retire();native_camera_view_retire();native_app_init_retire();events_prefix_frame=nullptr;api_registration_frame=nullptr;db_registration_frame=nullptr;database_namespaces_frame=nullptr;ui_enums_frame=nullptr;network_frame=nullptr;steam_frame=nullptr;locale_frame=nullptr;content_frame=nullptr;modding_frame=nullptr;native_master_thread_ledger_retire();native_master_reference_bank_retire();native_master_debug_socket_retire();native_master_protected_calls_retire();native_master_threads_retire();native_locale_operation_entry=nullptr;lua_events_factory_alive=false;lua_events_frame=nullptr;controls_frame=nullptr;include_configuration_alive=false;profile_include_frame=nullptr;profile_include_master_alive=false;profile_include_refreshing=false;profile_print_sink_alive=false;profile_debug_master_alive=false;BnwLuaCalculatedPlotYieldOwner::retire_vm_lease(calculated_plot_vm);if(state){worldview_retire_callback_registrations();worldview_retire_context_callback_registrations();(void)lua_cpcall(state,worldview_release_retired_callbacks,nullptr);worldview_retire_created_registrations();worldview_retire_damage_registrations();(void)lua_cpcall(state,worldview_release_retired_context_callbacks,nullptr);(void)lua_cpcall(state,worldview_release_retired_created,nullptr);(void)lua_cpcall(state,worldview_release_retired_damage,nullptr);native_postread_ui_manager_close_owned();native_postread_tt_manager_close();native_postread_load_tail_close_owned();native_postread_root_cdata_close();native_context_definition_close();native_postread_globals_close();lua_close(state);}}
    void call(Job &job) {
        const auto actual_storage=weak_from_this().lock();
        if(!actual_storage&&job.operation!=Operation::initialize)throw ScriptError("Lua operation lost actual shared Impl");
        if(std::this_thread::get_id()!=ui_thread)throw ScriptError("Lua VM must run on its owner thread");
        if(calling||end_turn_native_active){++execution_uncertainty;if(!control_graphs.empty()||context_sources||job.operation==Operation::validate_loading)record_failure();throw ScriptError("Lua VM invocation is reentrant");}
        // f_Ccall allocates its C closure before perform can validate loading.
        // Revoke only after owner-thread/idle guards, before any such allocation.
        if(job.operation==Operation::validate_loading)ready=false;
        NativeMasterProtectedLuaBoundary native_protected_boundary(*this,job);
        calling=true;
        for(auto &entry:control_graphs)entry.touched=false;
        const auto lifetime_current=[&]{if(end_turn_lifetime&&(!end_turn_lifetime->alive||end_turn_lifetime->failed))throw ScriptError("Configured Lua loading facade retired");};
        try {lifetime_current();for(auto &entry:control_graphs)if(entry.bound){lifetime_current();entry.graph->require_current();lifetime_current();}
            if(context_sources){lifetime_current();child_require_all();lifetime_current();}}
        catch(...) {calling=false;failed=true;ready=false;throw;}
        units_query_source_loading=job.operation==Operation::execute&&job.context==units_query_context;
        units_query_active_context=(job.operation==Operation::execute||job.operation==Operation::map_init_query)?job.context:
            job.operation==Operation::control?std::string_view(ui_layout.context_id):std::string_view{};
        const int top=lua_gettop(state);remaining=budget.instructions;exhausted=false;
        lua_sethook(state,budget.instructions?hook:nullptr,budget.instructions?LUA_MASKCOUNT:0,1000);
        const auto journal_before=execution_records.size();
        lifetime_current();
        native_protected_boundary.begin_cpcall();
        const int status=lua_cpcall(state,perform,&job);native_protected_boundary.after_cpcall();lua_sethook(state,nullptr,0,0);
        // Preserve the authentic primary result before callbackful graph cleanup.
        // This diagnostic does not restore flags, counters, stack or readiness.
        if(worldview_native_host&&worldview_native_host->creator==std::this_thread::get_id()&&
           (status||job.exception||failure_epoch!=worldview_native_host->epoch||
            execution_uncertainty!=worldview_native_host->uncertainty||failed.exhausted_||
            failed.writes_!=worldview_native_host->writes)){
            std::fprintf(stderr,"[WORLDVIEW_PRIMARY_FAILURE] operation=%d status=%d epoch=%llu writes=%llu uncertainty=%llu exhausted=%d context=",
                static_cast<int>(job.operation),status,static_cast<unsigned long long>(failure_epoch),
                static_cast<unsigned long long>(failed.writes_),static_cast<unsigned long long>(execution_uncertainty),
                failed.exhausted_?1:0);
            if(!job.context.empty())std::fwrite(job.context.data(),1,job.context.size(),stderr);
            std::fputs(" lua_error=",stderr);
            if(status&&lua_type(state,-1)==LUA_TSTRING){
                std::size_t length=0;const char* text=lua_tolstring(state,-1,&length);
                if(length)std::fwrite(text,1,length,stderr);
            }else std::fputs(status?"<non-string error; no coercion>":"<no cpcall error>",stderr);
            std::fputs(" exception=",stderr);
            if(job.exception){
                try{std::rethrow_exception(job.exception);}
                catch(const std::exception& error){std::fputs(error.what(),stderr);}
                catch(...){std::fputs("<non-standard exception>",stderr);}
            }else std::fputs("<none>",stderr);
            std::fputc('\n',stderr);std::fflush(stderr);
        }
        execution_abandon_from(journal_before);
        graph_execution_scope=graph_no_owner;
        units_query_active_context={};units_query_source_loading=false;
        if(ui_context_closed)release_control_callbacks();
        try {
            lifetime_current();
            if(context_sources&&!status&&!job.exception){child_require_all();lifetime_current();}
            for(auto &entry:control_graphs) {
                if(entry.touched&&(status||job.exception||exhausted||failed))entry.graph->poison();
                else if(entry.bound){lifetime_current();entry.graph->require_current();lifetime_current();}
            }
            if(job.operation==Operation::map_init_query&&!status&&!job.exception&&!exhausted)map_init_require_current(job);
            if(job.operation==Operation::update) {
                update_frame.reset(); // destructor callbacks still fenced
                update_require_current(false);child_require_all();
                if(update_epoch!=failure_epoch)throw ScriptError("Update frame release caught host reentry");
            }
        }catch(...) {
            if(job.operation==Operation::update)update_frame.reset();
            for(auto &entry:control_graphs)if(entry.touched)entry.graph->poison();
            calling=false;failed=true;ready=false;lua_settop(state,top);throw;
        }
        lifetime_current();
        calling=false;
        if(status){
            worldview_retire_callback_registrations();
            worldview_retire_created_registrations();worldview_retire_damage_registrations();worldview_retire_context_callback_registrations();
            std::string message="Lua operation failed";
            if(lua_type(state,-1)==LUA_TSTRING){std::size_t size=0;const char *text=lua_tolstring(state,-1,&size);message.assign(text,size);}
            lua_settop(state,top);throw ScriptError(message);
        }
        lua_settop(state,top);if(job.exception)std::rethrow_exception(job.exception);
        if(exhausted)throw ScriptError("Civis Lua instruction budget exhausted");
    }
    Job dispatch(Operation operation,std::string_view event,std::span<const ScriptValue> args){
        const auto actual_storage=shared_from_this();
        if(event.empty()||event.find('\0')!=event.npos)throw ScriptError("Invalid Lua event name");
        if(args.size()>static_cast<std::size_t>(INT_MAX-16))throw ScriptError("Lua call argument count exceeds VM stack API");
        for(const auto &arg:args)validate_value(arg);
        const auto errors_before=source_errors.size();
        Job job{this,operation,{},{},event,args,{},{}};call(job);
        for(std::size_t i=errors_before;i<source_errors.size();++i){
            const auto &error=source_errors[i];job.result.errors.push_back(error.path+": "+error.message);
        }
        return job;
    }
};
LuaScriptHost::LuaScriptHost(ScriptBudget budget,ScriptSourceProvider sources,ScriptModdingProvider modding,ScriptGameplayProvider gameplay,ScriptSaveDataProvider save_data,ScriptPregameProvider pregame,ScriptMapProvider map,ScriptUiEventProvider ui_events,ScriptContentProvider content,ScriptUserDataProvider user_data,ScriptEnumProvider enums,ScriptLocaleProvider locale)
    :LuaScriptHost(budget,std::move(sources),std::move(modding),std::move(gameplay),
        std::move(save_data),std::move(pregame),std::move(map),std::move(ui_events),
        std::move(content),std::move(user_data),std::move(enums),std::move(locale),{}){}
LuaScriptHost::LuaScriptHost(ScriptBudget budget,ScriptSourceProvider sources,ScriptModdingProvider modding,ScriptGameplayProvider gameplay,ScriptSaveDataProvider save_data,ScriptPregameProvider pregame,ScriptMapProvider map,ScriptUiEventProvider ui_events,ScriptContentProvider content,ScriptUserDataProvider user_data,ScriptEnumProvider enums,ScriptLocaleProvider locale,ScriptUiLayoutProvider ui_layout)
    :impl_(std::make_unique<Impl>(budget,std::move(sources),std::move(modding),std::move(gameplay),std::move(save_data),std::move(pregame),std::move(map),std::move(ui_events),std::move(content),std::move(user_data),std::move(enums),std::move(locale),std::move(ui_layout))){}
LuaScriptHost::~LuaScriptHost(){if(impl_->end_turn_lifetime){impl_->end_turn_lifetime->alive=false;impl_->end_turn_lifetime->failed=true;impl_->end_turn_lifetime->host=nullptr;}}
void LuaScriptHost::bind_owned_team_set_has_tech(
    std::shared_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> supplied) {
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->calling||impl_->failed||impl_->team_tech_inflight)
        throw ScriptError("Team:SetHasTech actual VM binding outside idle source operation");
    if(!supplied)throw ScriptError("Team:SetHasTech canonical issuer absent");
    // Authenticate control before callbacks; retain canonical storage only.
    const auto canonical=supplied->self_.lock();
    if(!canonical||canonical.get()!=supplied.get()||canonical.owner_before(supplied)||supplied.owner_before(canonical))
        throw ScriptError("Team:SetHasTech issuer control substituted");
    canonical->require_current();
    auto old=std::move(impl_->owned_team_tech);impl_->owned_team_tech=canonical;
    old.reset();canonical->require_current();
}
void LuaScriptHost::invoke_owned_team_set_has_tech(std::string_view context,
    std::int64_t team,std::int64_t technology,bool value,std::int64_t player,bool announce) {
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->calling||impl_->failed||!impl_->owned_team_tech)
        throw ScriptError("Team:SetHasTech actual native command VM unavailable/reentrant");
    impl_->owned_team_tech->require_current();
    if(!impl_->executed_contexts.contains(std::string(context)))
        throw ScriptError("Team:SetHasTech cannot create/reexecute a context");
    const ScriptValue args[]={team,technology,value,player,announce};
    Impl::Job job{impl_.get(),Impl::Operation::team_set_has_tech,context,{},{},args,{}, {}};
    // Genuine protected outer job; C callback invokes the same nested registry.
    // No new VM/context/chunk or loading completion is issued here.
    impl_->call(job);
    impl_->owned_team_tech->require_current();
}

void LuaScriptHost::execute(std::string_view context,std::string_view source,std::string_view chunk_name) {
    const auto p=impl_;
    p->ready=false;
    std::size_t action=std::numeric_limits<std::size_t>::max();
    try {
        if(context.empty()||context.find('\0')!=context.npos||chunk_name.find('\0')!=chunk_name.npos)throw ScriptError("Invalid Lua context/chunk name");
        if(p->ui_context_closed&&context==p->ui_layout.context_id)
            throw ScriptError("UI callback context is closed");
        if(source.starts_with("\xef\xbb\xbf"))source.remove_prefix(3);
        if(!source.empty()&&source.front()=='\x1b')throw ScriptError("Civis loads Lua source, not platform-specific bytecode");
        const std::string name(chunk_name);
        if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Lua VM must run on its owner thread");
        action=p->execution_begin(ScriptLuaAction::context_execute,context,chunk_name);
        p->execution_source(action,source);
        p->executed_contexts.emplace(context);
        Impl::Job job{p.get(),Impl::Operation::execute,context,source,name,{},{},{}};p->call(job);
        if(p->end_turn_lifetime&&(!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed))throw ScriptError("Lua selected loading facade retired");
        p->execution_finish(action,ScriptLuaActionOutcome::succeeded);
    }catch(...){p->execution_finish(action,ScriptLuaActionOutcome::error);p->record_failure();throw;}
}
ScriptMapInitQueryResult LuaScriptHost::query_map_init_data(std::string_view context,
    const ScriptUiContextSourceOwner& same_source_owner,std::int32_t world_id,ScriptMapInitFields initial) {
    try {
        if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Map init query requires VM owner thread");
        if(impl_->calling){impl_->record_failure();throw ScriptError("Map init query is reentrant");}
        if(context.empty()||context.find('\0')!=context.npos||impl_->failed||
           !impl_->context_sources||impl_->context_sources.get()!=&same_source_owner)
            throw ScriptError("Map init query lacks the SAME current source owner");
        const std::string retained_context(context); // owns caller bytes outside Lua protection
        Impl::Job job{impl_.get(),Impl::Operation::map_init_query,retained_context,{},{},{},{},{}};
        job.map_source_owner=&same_source_owner;job.map_epoch=impl_->failure_epoch;
        job.map_world_id=world_id;job.map_result.fields=initial;
        for(std::size_t slot=0;slot<impl_->child_contexts.size();++slot)
            if(impl_->child_contexts[slot].name==context){job.map_slot=slot;break;}
        impl_->call(job); // currentness and receiver bookends before returning data
        if(impl_->failed||job.map_epoch!=impl_->failure_epoch)throw ScriptError("Map init query failed before publication");
        return job.map_result;
    }catch(...){impl_->record_failure();throw;}
}
ScriptTestResult LuaScriptHost::test_all(std::string_view event,std::span<const ScriptValue> args) {
    return impl_->dispatch(Impl::Operation::test_all,event,args).result;
}
ScriptTestResult LuaScriptHost::test_any(std::string_view event,std::span<const ScriptValue> args) {
    return impl_->dispatch(Impl::Operation::test_any,event,args).result;
}
ScriptTestResult LuaScriptHost::call_hook(std::string_view event,std::span<const ScriptValue> args) {
    return impl_->dispatch(Impl::Operation::call_hook,event,args).result;
}
ScriptUiEventResult LuaScriptHost::emit_ui_event(std::string_view event,std::span<const ScriptValue> args){
    if(event=="SerialEventEndTurnDirty")throw ScriptError("Event69 requires the actual private queue pump");
    if(!impl_->ui_notification_names.contains(std::string(event)))
        throw ScriptError("UI notification is not declared by the active graphics layer");
    auto job=impl_->dispatch(Impl::Operation::emit_ui,event,args);
    return {static_cast<std::uint64_t>(job.integer.value_or(0)),std::move(job.result.errors)};
}
ScriptUiEventResult LuaScriptHost::dispatch_ui_control(std::string_view context,
    std::string_view control,std::int64_t event,std::span<const ScriptValue> args){
    if(context!=impl_->ui_layout.context_id||context.empty())
        throw ScriptError("UI callback context differs from its selected owner");
    impl_->require_ui_callback_owner();
    if(impl_->calling)throw ScriptError("UI control callback dispatch is reentrant");
    if(!loading_complete())throw ScriptError("UI control callback VM has not completed loading");
    if(args.size()>static_cast<std::size_t>(INT_MAX-16))throw ScriptError("Lua call argument count exceeds VM stack API");
    for(const auto &arg:args)Impl::validate_value(arg);
    const auto selected=std::find_if(impl_->control_callbacks.begin(),impl_->control_callbacks.end(),
        [&](const auto &callback){return callback.id==control&&callback.event==event;});
    if(selected==impl_->control_callbacks.end())throw ScriptError("UI control callback is not registered");
    Impl::Job job{impl_.get(),Impl::Operation::control,{},{},{},args,{},{}};
    job.control_function=selected->function;
    ScriptUiEventResult result;
    try{
        impl_->call(job);result.listeners=job.integer.value_or(0);
        impl_->require_ui_callback_owner();
    }catch(const std::exception &error){
        result.listeners=job.integer.value_or(0);
        result.errors.emplace_back(error.what());impl_->failed=true;impl_->ready=false;
        impl_->release_control_callbacks();
    }catch(...){
        result.listeners=job.integer.value_or(0);
        result.errors.emplace_back("UI control callback failed");impl_->failed=true;impl_->ready=false;
        impl_->release_control_callbacks();
    }
    return result;
}
std::size_t LuaScriptHost::registered_ui_control_callbacks(std::string_view context)const{
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("UI callback must run on its owner thread");
    if(context!=impl_->ui_layout.context_id||impl_->ui_context_closed)return 0;
    return static_cast<std::size_t>(std::count_if(impl_->control_callbacks.begin(),impl_->control_callbacks.end(),
        [](const auto &callback){return callback.function!=LUA_NOREF;}))+(impl_->text_function!=LUA_NOREF);
}
std::size_t LuaScriptHost::registered_ui_control_callbacks(std::string_view context,std::span<const std::string> controls)const{
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("UI callback must run on its owner thread");
    if(context!=impl_->ui_layout.context_id||impl_->ui_context_closed)return 0;
    return static_cast<std::size_t>(std::count_if(impl_->control_callbacks.begin(),impl_->control_callbacks.end(),
        [&](const auto &callback){return callback.function!=LUA_NOREF&&std::find(controls.begin(),controls.end(),callback.id)!=controls.end();}))+
        (impl_->text_function!=LUA_NOREF&&std::find(controls.begin(),controls.end(),impl_->text_entry.control_id)!=controls.end());
}
void LuaScriptHost::enable_ui_text_entry(ScriptUiTextEntryProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->ui_layout_bound||!impl_->text_entry.context_id.empty()||
       provider.context_id!=impl_->ui_layout.context_id||provider.control_id.empty()||!provider.get_text||!provider.replace_text)
        throw ScriptError("UI text entry capability is invalid, duplicated, or declared after context creation");
    const auto control=std::find_if(impl_->ui_layout.controls.begin(),impl_->ui_layout.controls.end(),
        [&](const auto &entry){return entry.id==provider.control_id;});
    if(control==impl_->ui_layout.controls.end()||control->kind!="EditBox"||control->has_callbacks)
        throw ScriptError("UI text entry requires an independently declared selected EditBox");
    impl_->text_entry=std::move(provider);impl_->ready=false;
}
void LuaScriptHost::replace_ui_text(std::string_view context,std::string_view control,std::string_view text){
    impl_->require_ui_ready(context);
    if(control!=impl_->text_entry.control_id||!impl_->text_entry.replace_text)throw ScriptError("UI text entry differs from selected control");
    impl_->text_entry.replace_text(text);impl_->require_ui_callback_owner();
}
ScriptUiEventResult LuaScriptHost::submit_ui_text(std::string_view context,std::string_view control){
    impl_->require_ui_ready(context);
    if(control!=impl_->text_entry.control_id||!impl_->text_entry.get_text||impl_->text_receiver==LUA_NOREF)
        throw ScriptError("UI text entry differs from selected control");
    if(impl_->ui_context.is_hidden&&impl_->ui_context.is_hidden())throw ScriptError("Hidden UI context cannot accept text submission");
    const ScriptValue args[]={impl_->text_entry.get_text()};
    const auto result=impl_->invoke_ui(impl_->text_function,args,impl_->text_receiver);
    return {result.listeners,result.errors};
}
void LuaScriptHost::enable_ui_context_handlers(ScriptUiContextProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->ui_layout_bound||!impl_->ui_context.context_id.empty()||
       provider.context_id!=impl_->ui_layout.context_id||!provider.is_hidden||!provider.set_hidden)
        throw ScriptError("UI context handler capability is invalid, duplicated, or declared after context creation");
    for(const auto &table:impl_->enums.tables)if(table.name=="Keys"||table.name=="KeyEvents")
        throw ScriptError("Explicit Civis key semantics conflict with already declared engine key enums");
    impl_->ui_context=std::move(provider);impl_->ready=false;
}
ScriptUiInputResult LuaScriptHost::dispatch_ui_input(std::string_view context,ScriptUiInputEvent event){
    impl_->require_ui_ready(context);
    if(!impl_->ui_context.is_hidden||impl_->ui_context.is_hidden())throw ScriptError("UI input context is unavailable or hidden");
    if((event.event!=ScriptUiKeyEvent::key_down&&event.event!=ScriptUiKeyEvent::key_up)||
       (event.key!=ScriptUiKey::other&&event.key!=ScriptUiKey::escape&&event.key!=ScriptUiKey::enter))
        throw ScriptError("UI input semantic token is not declared");
    const ScriptValue args[]={static_cast<std::int64_t>(event.event),static_cast<std::int64_t>(event.key),std::int64_t(0)};
    return impl_->invoke_ui(impl_->input_function,args,LUA_NOREF,true);
}
ScriptUiEventResult LuaScriptHost::dispatch_ui_visibility(std::string_view context,bool hidden){
    impl_->require_ui_ready(context);
    if(!impl_->ui_context.set_hidden||impl_->visibility_function==LUA_NOREF)throw ScriptError("UI visibility handler is not registered");
    impl_->ui_context.set_hidden(hidden);
    const ScriptValue args[]={hidden};const auto result=impl_->invoke_ui(impl_->visibility_function,args);
    return {result.listeners,result.errors};
}
std::size_t LuaScriptHost::registered_ui_context_handlers(std::string_view context)const{
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("UI handler query requires owner thread");
    if(context!=impl_->ui_layout.context_id||impl_->ui_context_closed)return 0;
    return (impl_->input_function!=LUA_NOREF)+(impl_->visibility_function!=LUA_NOREF);
}
void LuaScriptHost::enable_ui_tooltip(ScriptUiTooltipProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->ui_layout_bound||!impl_->tooltip.context_id.empty()||
       provider.context_id!=impl_->ui_layout.context_id||provider.type_name.empty()||provider.type_name.find('\0')!=provider.type_name.npos||
       provider.controls.empty()||!provider.registered||!provider.set_texture||!provider.set_texture_offset||!provider.set_hidden)
        throw ScriptError("Selected tooltip capability is invalid, duplicated, or declared after context creation");
    std::set<std::string> ids;
    for(const auto &control:provider.controls)if(control.id.empty()||control.id.find('\0')!=control.id.npos||
        control.kind!="Image"||!ids.insert(control.id).second)throw ScriptError("Selected tooltip Image identity is invalid or duplicated");
    impl_->tooltip=std::move(provider);impl_->ready=false;
}
void LuaScriptHost::enable_plain_locale_lookup(std::string context,std::vector<std::string> tags){
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->calling||impl_->ui_context_closed)
        throw ScriptError("Selected Locale.Lookup must be declared on the idle current owner thread");
    if(!impl_->plain_lookup_context.empty()||context.empty()||context.find('\0')!=context.npos||tags.empty()||!impl_->locale.convert_text_key)
        throw ScriptError("Selected Locale.Lookup capability is invalid, duplicated, or has no real locale provider");
    std::set<std::string> unique;
    for(const auto &tag:tags)if(tag.empty()||tag.find('\0')!=tag.npos||!unique.insert(tag).second)
        throw ScriptError("Selected Locale.Lookup tag is invalid or duplicated");
    impl_->plain_lookup_context=std::move(context);impl_->plain_lookup_tags=std::move(tags);impl_->ready=false;
}
void LuaScriptHost::enable_locale_text_operations(std::string context,ScriptLocaleTextProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->ui_layout_bound||!impl_->locale_text_context.empty()||context!=impl_->ui_layout.context_id||
       context.empty()||!impl_->locale.current||!provider.lower||!provider.compare)
        throw ScriptError("Selected locale text capability is invalid, duplicated, or declared after context creation");
    impl_->locale_text_context=std::move(context);impl_->locale_operations=std::move(provider);impl_->ready=false;
}
void LuaScriptHost::enable_civis_raw_table_count(std::string context){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->failed||impl_->ui_layout_bound||!impl_->raw_count_context.empty()||
       context.empty()||context!=impl_->ui_layout.context_id||context.find('\0')!=context.npos)
        throw ScriptError("Civis raw table count capability is invalid, duplicated, or declared after context creation");
    impl_->raw_count_context=std::move(context);impl_->ready=false;
}
void LuaScriptHost::enable_civis_selected_game_option(std::string context,ScriptSelectedGameOptionProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->failed||impl_->ui_layout_bound||!impl_->selected_option_context.empty()||
       context.empty()||context!=impl_->ui_layout.context_id||context.find('\0')!=context.npos||
       provider.no_religion_token!=21||!provider.is_option)
        throw ScriptError("Civis selected Game option capability is invalid, duplicated, or declared after context creation");
    impl_->selected_option_context=std::move(context);impl_->selected_option=std::move(provider);impl_->ready=false;
}
void LuaScriptHost::enable_civis_units_query(std::string context,ScriptCivisUnitsQueryProvider provider){
    impl_->require_ui_callback_owner();
    if(impl_->calling||impl_->failed||impl_->ui_layout_bound||!impl_->units_query_context.empty()||
       context.empty()||context!=impl_->ui_layout.context_id||context.find('\0')!=context.npos||!provider.prepare)
        throw ScriptError("Civis Units CreateQuery capability is invalid, duplicated, or declared after context creation");
    impl_->units_query_context=std::move(context);impl_->units_query=std::move(provider);impl_->ready=false;
}
ScriptCivisUnitsQueryObservations LuaScriptHost::civis_units_query_observations(std::string_view context)const{
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Civis Units query observations require the owner thread");
    return context==impl_->units_query_context?impl_->units_query_observed:ScriptCivisUnitsQueryObservations{};
}
void LuaScriptHost::enable_ui_control_graph(std::shared_ptr<ScriptUiControlGraph> graph){
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->calling||impl_->failed||!graph)
        throw ScriptError("UI graph registration thread/state refused");
    impl_->calling=true;
    try {graph->require_current();}catch(...) {impl_->calling=false;throw;}
    impl_->calling=false;
    if(graph->context()==impl_->ui_layout.context_id||impl_->executed_contexts.contains(graph->context())||
       std::any_of(impl_->control_graphs.begin(),impl_->control_graphs.end(),
           [&](const auto &entry){return entry.graph->context()==graph->context();}))
        throw ScriptError("UI graph context already owned or executed");
    if(impl_->control_graphs.size()>=64)throw ScriptError("UI graph context budget exhausted");
    impl_->control_graphs.push_back({std::move(graph),false,false,LUA_NOREF});
}
void LuaScriptHost::enable_ui_context_sources(std::shared_ptr<ScriptUiContextSourceOwner> owner){
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Child owner binding requires VM thread");
    if(impl_->calling&&impl_->context_sources){impl_->record_failure();throw ScriptError("Child owner binding is reentrant");}
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->calling||impl_->failed||!owner||
       impl_->context_sources||!impl_->executed_contexts.empty())throw ScriptError("Child source owner binding refused");
    impl_->context_sources=std::move(owner);impl_->ready=false;
}
void LuaScriptHost::enable_ui_updates(std::shared_ptr<ScriptUiUpdateSourceOwner> owner){
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Update owner binding requires VM thread");
    if(impl_->calling){impl_->record_failure();throw ScriptError("Update owner binding is reentrant");}
    if(!owner||!impl_->context_sources||impl_->update_sources||impl_->failed||!impl_->executed_contexts.empty())
        throw ScriptError("Update owner requires idle SAME source channel before execution");
    impl_->update_sources=std::move(owner);impl_->calling=true;
    try {impl_->update_require_current(false);}
    catch(...) {impl_->calling=false;impl_->record_failure();throw;}
    impl_->calling=false;impl_->ready=false;
}
ScriptUiEventResult LuaScriptHost::dispatch_ui_update(){
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Update dispatch requires VM thread");
    if(impl_->calling){impl_->record_failure();throw ScriptError("Update dispatch is reentrant");}
    if(!impl_->update_sources||!impl_->context_sources)throw ScriptError("Update source owner unavailable");
    if(!loading_complete())throw ScriptError("Update VM has not completed generic loading");
    Impl::Job job{impl_.get(),Impl::Operation::update,{},{},{},{},{},{}};
    ScriptUiEventResult result;
    try {impl_->call(job);result.listeners=job.integer.value_or(0);}
    catch(const std::exception &error){impl_->record_failure();result.listeners=job.integer.value_or(0);result.errors.emplace_back(error.what());}
    catch(...){impl_->record_failure();result.listeners=job.integer.value_or(0);result.errors.emplace_back("Update callback failed");}
    return result;
}
std::size_t LuaScriptHost::registered_ui_updates()const{
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Update observation requires VM thread");
    return static_cast<std::size_t>(std::count_if(impl_->child_contexts.begin(),impl_->child_contexts.end(),
        [](const auto &entry){return !entry.retired&&entry.update_function!=LUA_NOREF;}));
}
void LuaScriptHost::close_ui_context(std::string_view context){
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("UI close requires VM owner thread");
    if(impl_->calling&&impl_->context_sources){impl_->record_failure();throw ScriptError("Child close during VM invocation refused");}
    bool child_retired=false;
    for(auto &entry:impl_->child_contexts)if(entry.name==context) {
        impl_->record_failure();
        entry.retired=true;child_retired=true;break;
    }
    if(child_retired) {
        const int top=lua_gettop(impl_->state);
        const int status=lua_cpcall(impl_->state,Impl::release_retired_map_contexts,nullptr);
        lua_settop(impl_->state,top);
        if(status)throw ScriptError("Retired update closure release failed");
    }
    for(auto &entry:impl_->control_graphs)if(entry.graph->context()==context) {
        if(impl_->calling)throw ScriptError("UI graph close during VM invocation refused");
        entry.graph->retire();impl_->ready=false;impl_->failed=true;return;
    }
    if(child_retired&&context!=impl_->ui_layout.context_id)return;
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("UI callback must close on its owner thread");
    if(context!=impl_->ui_layout.context_id||context.empty())throw ScriptError("UI callback context differs from its selected owner");
    impl_->ui_context_closed=true;impl_->ready=false;
    if(!impl_->calling)impl_->release_control_callbacks();
}
ScriptIntegerResult LuaScriptHost::accumulate_int(std::string_view event,std::span<const ScriptValue> args,std::int64_t initial){
    auto job=impl_->dispatch(Impl::Operation::accumulate_int,event,args);
    if(job.integer)job.integer=wide::add(initial,*job.integer);
    return {job.integer,std::move(job.result.errors)};
}
ScriptNumberResult LuaScriptHost::accumulate(std::string_view event,std::span<const ScriptValue> args,double initial){
    if(!std::isfinite(initial))throw ScriptError("Nonfinite accumulator initial value");
    auto job=impl_->dispatch(Impl::Operation::accumulate,event,args);
    if(job.number){
        job.number=initial+*job.number;
        if(!std::isfinite(*job.number))throw ScriptError("Lua accumulator result is nonfinite");
    }
    return {job.number,std::move(job.result.errors)};
}
void LuaScriptHost::complete_loading(){
    Impl::Job job{impl_.get(),Impl::Operation::validate_loading,{},{},{},{},{},{}};impl_->call(job);
    if(impl_->failed)throw ScriptError("Loading owner validation failed");
    // Set only after the actual same-thread source/graph postflight. This
    // necessary check cannot discover omitted selected roots or attest their
    // load schedule, script-system binding or absence of listeners.
    impl_->ready=true;
}
bool LuaScriptHost::loading_complete()const{return impl_->ready&&!impl_->failed;}
std::uint64_t LuaScriptHost::memory_bytes()const{return impl_->bytes;}
LuaScriptHost::StockOperationStamp LuaScriptHost::stock_operation_stamp()const {
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Stock VM stamp foreign thread");
    return {impl_->state,impl_->failure_epoch,impl_->failed};
}
std::optional<LuaScriptHost::StockOperationStamp> LuaScriptHost::stock_include_provider_stamp()const {
    if(std::this_thread::get_id()!=impl_->ui_thread)
        throw ScriptError("Include provider stamp foreign thread");
    const auto* const scope=impl_->include_provider_scope;
    if(!scope){
        if(const auto* child=impl_->end_turn_child_prepare){
            child->require_current();return StockOperationStamp{impl_->state,child->epoch,false};
        }
        if(const auto* list=impl_->end_turn_source_list){
            list->require_current();
            return StockOperationStamp{impl_->state,list->epoch,false}; // actual healthy-origin selected loader provider only
        }
        return std::nullopt; // Ordinary checks keep the conservative stamp.
    }
    scope->require_current();
    // Only this actual compiler's own provisional sentinel is accounted here.
    // Any caught native failure/reentry changes the exact epochs above and refuses.
    return StockOperationStamp{scope->vm,scope->epoch,scope->prior_failed};
}
const std::vector<ScriptSourceError>& LuaScriptHost::source_errors()const{return impl_->source_errors;}
const std::vector<ScriptMessage>& LuaScriptHost::messages()const{return impl_->messages;}
const std::vector<ScriptMessage>& LuaScriptHost::native_lua_log_messages()const{return impl_->profile_print_log_messages;}
ScriptLuaExecutionSnapshot LuaScriptHost::execution_snapshot()const {
    if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("Execution snapshot requires VM thread");
    if(impl_->calling)throw ScriptError("Execution snapshot cannot borrow an active VM");
    return {impl_.get(),impl_->failure_epoch,impl_->execution_uncertainty,impl_->failed,impl_->execution_records};
}
std::string_view LuaScriptHost::version(){return LUA_RELEASE;}
} // namespace civ5::runtime

namespace civ5::runtime {

ScriptIntegerResult LuaScriptHost::accumulate_owned_team_happiness_founder(std::int64_t player){
    try{
        if(std::this_thread::get_id()!=impl_->ui_thread||!impl_->calling||!impl_->team_tech_inflight||impl_->failed||
           !impl_->owned_team_tech||player<0||player>=22)
            throw ScriptError("Founder happiness requires actual current nested team operation");
        const auto owner=impl_->owned_team_tech;owner->require_current();
        const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
        Impl::TeamFounderJob job{impl_.get(),player,{}, {}};const int top=lua_gettop(impl_->state);
        const int status=lua_cpcall(impl_->state,Impl::nested_team_founder,&job);lua_settop(impl_->state,top);
        if(status)throw ScriptError("Founder happiness SAME-VM protected dispatch failed");
        if(job.exception)std::rethrow_exception(job.exception);
        if(impl_->failed||impl_->failure_epoch!=failure||impl_->execution_uncertainty!=uncertainty||impl_->exhausted)
            throw ScriptError("Founder happiness SAME-VM failure/reentry/budget changed");
        owner->require_current();
        if(!job.result.errors.empty())throw ScriptError("Founder happiness actual callback failed: "+job.result.errors.front());
        return std::move(job.result);
    }catch(...){impl_->record_failure();throw;}
}

}

namespace civ5::runtime {
ScriptTestResult LuaScriptHost::test_selected_production_owned(BnwSelectedProductionLuaEvent event,
 std::int32_t player,std::int32_t city,std::int32_t item,const BnwStockLuaAttemptLease& lease){
    const auto serial=lease.operation_serial();
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->production_test_inflight||player<0||city<0||item<0)
        throw ScriptError("Production TestAll SAME VM/thread/issuer unavailable");
    const char* name=nullptr;bool city_event=false;
    switch(event){
     case BnwSelectedProductionLuaEvent::player_can_maintain:name="PlayerCanMaintain";break;
     case BnwSelectedProductionLuaEvent::city_can_maintain:name="CityCanMaintain";city_event=true;break;
     case BnwSelectedProductionLuaEvent::player_can_construct:name="PlayerCanConstruct";break;
     case BnwSelectedProductionLuaEvent::city_can_construct:name="CityCanConstruct";city_event=true;break;
     case BnwSelectedProductionLuaEvent::player_can_train:name="PlayerCanTrain";break;
     case BnwSelectedProductionLuaEvent::city_can_train:name="CityCanTrain";city_event=true;break;
     default:throw ScriptError("Production TestAll unsupported typed event");
    }
    lease.require_operation(serial);
    if(!impl_->calling){
        const ScriptValue args[]={std::int64_t{player},std::int64_t{city},std::int64_t{item}};
        const ScriptValue player_args[]={std::int64_t{player},std::int64_t{item}};
        auto actual=impl_->dispatch(Impl::Operation::test_all,name,city_event?std::span<const ScriptValue>{args}:std::span<const ScriptValue>{player_args}).result;
        lease.require_operation(serial);return actual;
    }
    impl_->production_test_inflight=true;
    struct End{Impl& impl;~End(){impl.production_test_inflight=false;}} end{*impl_};
    const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
    Impl::ProductionHookJob job{impl_.get(),name,player,city,item,city_event,{}, {}};
    const auto top=lua_gettop(impl_->state);const auto status=lua_cpcall(impl_->state,Impl::nested_production_hook,&job);lua_settop(impl_->state,top);
    if(status!=0||job.exception||impl_->failed||failure!=impl_->failure_epoch||uncertainty!=impl_->execution_uncertainty){
        impl_->record_failure();if(job.exception)std::rethrow_exception(job.exception);throw ScriptError("Actual nested production TestAll failed/uncertain");
    }
    lease.require_operation(serial);return std::move(job.result);
}
}

namespace civ5::runtime {
ScriptTestResult LuaScriptHost::call_selected_city_trained_owned(
 std::int32_t player,std::int32_t city,std::int32_t unit,const BnwStockLuaAttemptLease& lease){
    const auto serial=lease.operation_serial();
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->production_test_inflight||player<0||city<0||unit<0)
        throw ScriptError("CityTrained SAME VM/thread/issuer unavailable");
    lease.require_operation(serial);
    if(!impl_->calling){
        const ScriptValue args[]={std::int64_t{player},std::int64_t{city},std::int64_t{unit},false,false};
        auto result=impl_->dispatch(Impl::Operation::call_hook,"CityTrained",args).result;
        lease.require_operation(serial);return result;
    }
    impl_->production_test_inflight=true;
    struct End{Impl& impl;~End(){impl.production_test_inflight=false;}} end{*impl_};
    const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
    Impl::CityTrainedHookJob job{impl_.get(),player,city,unit,{}, {}};
    const auto top=lua_gettop(impl_->state);const auto status=lua_cpcall(impl_->state,Impl::nested_city_trained_hook,&job);lua_settop(impl_->state,top);
    if(status!=0||job.exception||impl_->failed||failure!=impl_->failure_epoch||uncertainty!=impl_->execution_uncertainty){
        impl_->record_failure();if(job.exception)std::rethrow_exception(job.exception);throw ScriptError("Actual nested CityTrained failed/uncertain");
    }
    lease.require_operation(serial);return std::move(job.result);
}
}

namespace civ5::runtime {
void LuaScriptHost::activate_selected_tech_award_owned(std::string_view source,const BnwStockLuaAttemptLease& lease){
 const auto serial=lease.operation_serial();lease.require_operation(serial);
 if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->tech_award_inflight||source.empty()||
    impl_->tech_award_popup!=LUA_NOREF||impl_->executed_contexts.contains("TechAwardPopup"))
  throw ScriptError("TECH_AWARD actual activation is unavailable/repeated");
 impl_->tech_award_inflight=true;struct End{Impl& p;~End(){p.tech_award_inflight=false;}}end{*impl_};
 const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
 const auto action=impl_->execution_begin(ScriptLuaAction::context_execute,"TechAwardPopup","Assets/UI/InGame/Popups/TechAwardPopup.lua");
 impl_->execution_source(action,source);impl_->executed_contexts.emplace("TechAwardPopup");
 Impl::TechAwardJob job{impl_.get(),source,true,0,0,0,0,false,false,{}};const int top=lua_gettop(impl_->state);
 try{
  const bool nested=impl_->calling;
  if(!nested){impl_->calling=true;impl_->remaining=impl_->budget.instructions;impl_->exhausted=false;lua_sethook(impl_->state,impl_->budget.instructions?Impl::hook:nullptr,impl_->budget.instructions?LUA_MASKCOUNT:0,1000);}
  const int status=lua_cpcall(impl_->state,Impl::selected_tech_award_job,&job);
  const char* actual_error=status?lua_tostring(impl_->state,-1):nullptr;lua_settop(impl_->state,top);
  if(!nested){lua_sethook(impl_->state,nullptr,0,0);impl_->calling=false;}
  if(status||job.exception||impl_->failed||impl_->exhausted||impl_->failure_epoch!=failure||impl_->execution_uncertainty!=uncertainty){
   if(job.exception)std::rethrow_exception(job.exception);throw ScriptError(actual_error?actual_error:"TECH_AWARD selected full source/native context initialization failed");
  }
  impl_->tech_award_context->require_current();lease.require_operation(serial);impl_->execution_finish(action,ScriptLuaActionOutcome::succeeded);
 }catch(...){lua_settop(impl_->state,top);impl_->execution_finish(action,ScriptLuaActionOutcome::error);impl_->record_failure();throw;}
}
void LuaScriptHost::dispatch_selected_tech_award_owned(std::int64_t data1,std::int64_t data2,std::int64_t data3,
 std::int32_t flags,bool option1,bool option2,const BnwStockLuaAttemptLease& lease){
 const auto serial=lease.operation_serial();lease.require_operation(serial);
 if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->tech_award_inflight||impl_->tech_award_popup==LUA_NOREF||data1<0||data3<0)
  throw ScriptError("TECH_AWARD actual initialized receiver/issuer unavailable");
 Impl::validate_value(data1);Impl::validate_value(data2);Impl::validate_value(data3);
 impl_->tech_award_inflight=true;struct End{Impl& p;~End(){p.tech_award_inflight=false;}}end{*impl_};
 const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;const bool nested=impl_->calling;
 Impl::TechAwardJob job{impl_.get(),{},false,data1,data2,data3,flags,option1,option2,{}};
 const int top=lua_gettop(impl_->state);
 try{
  if(!nested){impl_->calling=true;impl_->remaining=impl_->budget.instructions;impl_->exhausted=false;lua_sethook(impl_->state,impl_->budget.instructions?Impl::hook:nullptr,impl_->budget.instructions?LUA_MASKCOUNT:0,1000);}
  const int status=lua_cpcall(impl_->state,Impl::selected_tech_award_job,&job);
  const char* actual_error=status?lua_tostring(impl_->state,-1):nullptr;lua_settop(impl_->state,top);
  if(!nested){lua_sethook(impl_->state,nullptr,0,0);impl_->calling=false;}
  if(status||job.exception||impl_->failed||impl_->exhausted||failure!=impl_->failure_epoch||uncertainty!=impl_->execution_uncertainty){
   if(job.exception)std::rethrow_exception(job.exception);throw ScriptError(actual_error?actual_error:"TECH_AWARD actual OnPopup/RefreshDisplay/native QueuePopup failed");
  }
  impl_->tech_award_context->require_current();lease.require_operation(serial);
 }catch(...){lua_settop(impl_->state,top);impl_->record_failure();throw;}
}
}

namespace civ5::runtime {
void LuaScriptHost::bind_selected_tech_award_context_owned(
 std::shared_ptr<const ui::BnwTechAwardContextOwner> supplied,const BnwStockLuaAttemptLease& lease){
 if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->tech_award_context||impl_->tech_award_inflight||!supplied)
  throw ScriptError("TECH_AWARD actual context binding unavailable/repeated");
 const auto serial=lease.operation_serial();lease.require_operation(serial);
 const auto actual=supplied->retain_canonical(supplied);actual->require_current();lease.require_operation(serial);
 std::vector<std::pair<std::string,std::string>> controls;
 for(const auto& id:actual->control_ids())controls.emplace_back(id,actual->control_kind(id));
 if(controls.size()>static_cast<std::size_t>(INT_MAX))throw ScriptError("TECH_AWARD XML control count exceeds Lua API");
 actual->require_current();lease.require_operation(serial);
 const auto popup_type=actual->native_popup_type();const auto priority=actual->popup_priority();
 actual->require_current();lease.require_operation(serial);
 const auto left_click=actual->native_mouse_left_click();actual->require_current();lease.require_operation(serial);
 impl_->tech_award_native_left_click=left_click;
 impl_->tech_award_native_popup_type=popup_type;impl_->tech_award_native_priority=priority;
 impl_->tech_award_native_controls=std::move(controls);
 impl_->tech_award_context=actual; // genuine canonical control only; VM owns context storage
}
}

namespace civ5::runtime {
ScriptTestResult LuaScriptHost::call_initial_unit_setxy_owned(
 std::int32_t player,std::int32_t full_id,std::int32_t x,std::int32_t y,const BnwStockLuaAttemptLease& lease){
    // Bootstrap derives operands only while retaining Core's private genuine
    // dispatch lease; this method is private, not a caller ID/ready API.
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->unit_setxy_inflight||
       player<0||player>=64||full_id<0||x<0||y<0)
        throw ScriptError("UnitSetXY SAME VM/thread/issuer unavailable");
    const auto serial=lease.operation_serial();lease.require_operation(serial);
    impl_->unit_setxy_inflight=true;
    struct End{Impl& p;~End(){p.unit_setxy_inflight=false;}}end{*impl_};
    try{
        if(!impl_->calling){
            const ScriptValue args[]={std::int64_t{player},std::int64_t{full_id},std::int64_t{x},std::int64_t{y}};
            auto result=impl_->dispatch(Impl::Operation::call_hook,"UnitSetXY",args).result;
            lease.require_operation(serial);
            if(!result.errors.empty())throw ScriptError("Actual UnitSetXY callback failed");
            return result; // Real no-listener result remains absent, not manufactured.
        }
        const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
        Impl::UnitSetXYJob job{impl_.get(),player,full_id,x,y,{}, {}};
        const int top=lua_gettop(impl_->state);const int status=lua_cpcall(impl_->state,Impl::nested_unit_setxy,&job);
        lua_settop(impl_->state,top);
        if(status!=0||job.exception||impl_->failed||failure!=impl_->failure_epoch||
           uncertainty!=impl_->execution_uncertainty||!job.result.errors.empty()){
            if(job.exception)std::rethrow_exception(job.exception);
            throw ScriptError("Actual nested UnitSetXY callback failed/uncertain");
        }
        lease.require_operation(serial);return std::move(job.result);
    }catch(...){impl_->record_failure();throw;} // Never pretend partial Lua side effects rolled back.
}
}

namespace civ5::runtime {
ScriptIntegerResult LuaScriptHost::accumulate_initial_unit_founder_owned(
 std::int32_t player,const BnwStockLuaAttemptLease& lease){
    // Bootstrap reads player from a genuine earned Core lease. This private
    // method deliberately has no fabricated owned_team_tech parent: it uses
    // the same actual VM registry under the real unit operation instead.
    if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||
       impl_->unit_founder_inflight||player<0||player>=64)
        throw ScriptError("Initial unit founder SAME VM/thread/issuer unavailable");
    const auto serial=lease.operation_serial();lease.require_operation(serial);
    impl_->unit_founder_inflight=true;
    struct End{Impl& p;~End(){p.unit_founder_inflight=false;}}end{*impl_};
    try{
        if(!impl_->calling){
            const ScriptValue args[]={std::int64_t{player}};
            auto job=impl_->dispatch(Impl::Operation::accumulate_int,"GetFounderBenefitsReligion",args);
            lease.require_operation(serial);
            if(!job.result.errors.empty())throw ScriptError("Initial unit actual founder callback failed");
            return {job.integer,std::move(job.result.errors)};
        }
        const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
        // Exact protected AccumulateINT registry job already used by native
        // team happiness. Only admission is the genuine unit operation lease.
        Impl::TeamFounderJob job{impl_.get(),player,{}, {}};
        const int top=lua_gettop(impl_->state);
        const int status=lua_cpcall(impl_->state,Impl::nested_team_founder,&job);lua_settop(impl_->state,top);
        if(status!=0||job.exception||impl_->failed||failure!=impl_->failure_epoch||
           uncertainty!=impl_->execution_uncertainty||impl_->exhausted||!job.result.errors.empty()){
            if(job.exception)std::rethrow_exception(job.exception);
            throw ScriptError("Initial unit nested founder callback failed/uncertain");
        }
        lease.require_operation(serial);return std::move(job.result);
    }catch(...){impl_->record_failure();throw;}
}
}

namespace civ5::runtime {
ScriptIntegerResult LuaScriptHost::accumulate_engine_founder_owned(std::int32_t actor,const BnwStockLuaAttemptLease& lease){
 // Reuse the actual protected AccumulateINT registry implementation and shared
 // reentry fence. This low-level private VM call grants no Unit/Tech receipt;
 // the genuine Engine cursor derives actor and Bootstrap owns its operation.
 return accumulate_initial_unit_founder_owned(actor,lease);
}
}

namespace civ5::runtime {
void LuaScriptHost::require_selected_tech_award_input_owner_owned(
 const std::shared_ptr<const ui::BnwTechAwardContextOwner>& context,
 const BnwStockLuaAttemptLease& lease)const{
 if(std::this_thread::get_id()!=impl_->ui_thread||impl_->failed||impl_->tech_award_inflight||
    !context||!impl_->tech_award_context||impl_->tech_award_popup==LUA_NOREF||
    context.get()!=impl_->tech_award_context.get()||
    context.owner_before(impl_->tech_award_context)||impl_->tech_award_context.owner_before(context))
  throw ScriptError("TechAward input differs from SAME VM original retained context");
 const auto serial=lease.operation_serial();lease.require_operation(serial);
 // Canonical storage only. Old popup Request operation is not renewed here.
}
void LuaScriptHost::dispatch_selected_tech_award_input_owned(
 std::shared_ptr<const ui::BnwTechAwardInputDispatch> supplied,const BnwStockLuaAttemptLease& lease){
 if(std::this_thread::get_id()!=impl_->ui_thread||!supplied)throw ScriptError("TechAward input foreign creator/token absent");
 const auto token=supplied->retain_canonical(supplied);
 const auto context=token->retain_context_for_dispatch();
 require_selected_tech_award_input_owner_owned(context,lease);
 token->require_active_dispatch();token->require_same_actual_context(*impl_->tech_award_context);
 const auto serial=lease.operation_serial();lease.require_operation(serial);
 Impl::TechAwardInputJob job{impl_.get(),token->target(),0,0,0,0,{}};
 if(job.target==ui::BnwTechAwardInput::escape||job.target==ui::BnwTechAwardInput::return_key){
  job.key_down=token->native_key_down();job.escape_key=token->native_escape_key();
  job.return_key=token->native_return_key();job.input_key=token->native_input_key();
 }
 token->require_active_dispatch();lease.require_operation(serial);
 impl_->tech_award_inflight=true;const bool nested=impl_->calling;
 struct End{Impl& p;bool nested;~End(){p.tech_award_inflight=false;if(!nested){lua_sethook(p.state,nullptr,0,0);p.calling=false;}}}end{*impl_,nested};
 const auto failure=impl_->failure_epoch,uncertainty=impl_->execution_uncertainty;
 const int top=lua_gettop(impl_->state);
 try{
  if(!nested){impl_->calling=true;impl_->remaining=impl_->budget.instructions;impl_->exhausted=false;
   lua_sethook(impl_->state,impl_->budget.instructions?Impl::hook:nullptr,impl_->budget.instructions?LUA_MASKCOUNT:0,1000);}
  const int status=lua_cpcall(impl_->state,Impl::selected_tech_award_input_job,&job);
  std::string error;
  if(status){const char* actual=lua_tostring(impl_->state,-1);error=actual?actual:"TechAward original input Lua error";}
  lua_settop(impl_->state,top);
  if(status||job.exception||impl_->failed||impl_->exhausted||failure!=impl_->failure_epoch||uncertainty!=impl_->execution_uncertainty){
   if(job.exception)std::rethrow_exception(job.exception);throw ScriptError(error.empty()?"TechAward original native input failed":error);
  }
  token->require_active_dispatch();token->require_same_actual_context(*impl_->tech_award_context);
  impl_->tech_award_context->require_current();lease.require_operation(serial);
 }catch(...){lua_settop(impl_->state,top);token->fail_actual_dispatch();impl_->record_failure();throw;}
}
}

namespace civ5::runtime {
void LuaScriptHost::prepare_end_turn_consumer_owned(const ScriptUiContextSourceOwner& same,std::shared_ptr<const ScriptUiChildSource> source){
 const auto p=impl_;
 try{
  if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Event69 activation foreign thread");
  if(p->calling||p->end_turn_native_active){p->record_failure();throw ScriptError("Event69 activation reentry");}
  if(p->failed||!source||!p->context_sources||p->context_sources.get()!=&same||
   !p->ui_notification_names.contains("SerialEventEndTurnDirty")||
   p->executed_contexts.contains("ActionInfoPanel"))throw ScriptError("Event69 genuine selected ActionInfoPanel activation is unjoined");
  std::optional<Impl::WorldViewNativeHostScope> native;
  if(p->end_turn_lifetime){
   // Reuse only the lifetime issued by the actual current native WorldView
   // binding. The old arbitrary-lifetime and repeated-activation paths refuse.
   const auto actual=p->worldview_sources.lock();
   if(!p->worldview_sources_bound||!actual||actual.get()!=&same||
      actual.get()!=p->context_sources.get()||actual.owner_before(p->context_sources)||
      p->context_sources.owner_before(actual)||!p->end_turn_lifetime->alive||
      p->end_turn_lifetime->failed||p->end_turn_lifetime->host!=this||
      p->end_turn_lifetime->vm!=p->state||p->end_turn_sealed||p->end_turn_registering||
      p->end_turn_registration||p->end_turn_registered_state||
      p->end_turn_registered_thread!=LUA_NOREF||!p->end_turn_functions.empty()||
      !p->end_turn_context.empty()||!p->end_turn_expected_source.expired()||
      !p->end_turn_expected_sources.expired())
    throw ScriptError("Event69 existing lifetime is not fresh native SAME WorldView/VM storage");
   const auto& loan=actual->current_operation_loan();
   native.emplace(*p,*this,loan);p->worldview_require_callback_origin();
   loan.require_same_vm(*this);p->worldview_require_callback_origin();
   loan.require_same_source(*source);p->worldview_require_callback_origin();
  }else{
   p->end_turn_lifetime=std::make_shared<ui::BnwEngineUiEndTurnQueue::VmLifetime>();
   p->end_turn_lifetime->host=this;p->end_turn_lifetime->vm=p->state;
  }
  p->end_turn_native_active=true;
  const auto epoch=p->failure_epoch;
  const auto post=[&]{if(epoch!=p->failure_epoch||p->failed||p->calling||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
    p->end_turn_lifetime->host!=this||p->end_turn_lifetime->vm!=p->state)
   throw ScriptError("Event69 activation source callback failed/retired facade");
   if(native)p->worldview_require_callback_origin();};
  const std::string context(source->context());post();
  if(context!="ActionInfoPanel")throw ScriptError("Event69 selected ActionInfoPanel source activation is unjoined");
  source->require_current();post();same.require_current(context,source.get());post();
  p->end_turn_context=context;p->end_turn_expected_source=source;p->end_turn_expected_sources=p->context_sources;
  post();p->end_turn_native_active=false;if(native)native->returned=true;
 }catch(...){p->end_turn_native_active=false;p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
std::shared_ptr<const ui::BnwEngineUiEndTurnQueue::Consumer> LuaScriptHost::seal_end_turn_consumer_owned(){
 const auto p=impl_;
 try{
  if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Event69 sealing foreign thread");
  if(p->calling||p->end_turn_native_active){p->record_failure();throw ScriptError("Event69 sealing reentry");}
  p->end_turn_native_active=true;p->end_turn_current(true);
  if((!p->ready&&!p->worldview_sources_bound)||p->end_turn_sealed||p->end_turn_registering||p->end_turn_functions.size()!=1)
   throw ScriptError("Event69 actual selected single consumer did not finish loading");
  auto c=std::shared_ptr<ui::BnwEngineUiEndTurnQueue::Consumer>(new ui::BnwEngineUiEndTurnQueue::Consumer);
  c->lifetime=p->end_turn_lifetime;c->sources=p->end_turn_expected_sources;c->source=p->end_turn_expected_source;
  for(;c->child<p->child_contexts.size();++c->child)if(p->child_contexts[c->child].name==p->end_turn_context)break;
  c->registration=p->end_turn_registration;c->epoch=p->failure_epoch;c->vm=p->state;c->registered_state=p->end_turn_registered_state;
  p->end_turn_consumer=c;p->end_turn_sealed=true;p->end_turn_native_active=false;return c;
 }catch(...){p->end_turn_native_active=false;p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_end_turn_consumer_owned(const ui::BnwEngineUiEndTurnQueue::Consumer& c)const{
 const auto p=impl_;
 if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Event69 consumer foreign thread");
 p->end_turn_require_consumer(c);
}
std::unique_ptr<ui::BnwEngineUiEndTurnQueue::Completion> LuaScriptHost::dispatch_end_turn_dirty_owned(
 const ui::BnwEngineUiEndTurnQueue::Consumer& c,const ui::BnwEngineUiEndTurnQueue::Event& event){
 const auto p=impl_;
 try{
  if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Event69 dispatch foreign thread");
  if(p->calling||p->end_turn_native_active){p->record_failure();throw ScriptError("Event69 dispatch reentry");}
  p->end_turn_require_consumer(c);
  Impl::Job job{p.get(),Impl::Operation::engine_end_turn_dirty,{},{},{},{},{},{}};
  p->call(job); // real registered closure, protected VM boundary and source POST
  p->end_turn_require_consumer(c);
  if(p->calling||p->failed||(!p->ready&&!p->worldview_sources_bound))throw ScriptError("Event69 protected callback did not return to idle current VM");
  return std::unique_ptr<ui::BnwEngineUiEndTurnQueue::Completion>(new ui::BnwEngineUiEndTurnQueue::Completion(event,c));
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}

namespace civ5::runtime {
std::optional<LuaScriptHost::StockOperationStamp> LuaScriptHost::stock_worldview_provider_stamp(
 const BnwWorldViewSourceOperationLoan& loan)const {
 const auto p=impl_;
 if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("WorldView provider stamp foreign creator");
 const auto* scope=p->worldview_native_host;
 if(!scope)return std::nullopt;
 scope->pure();
 if(scope->host!=this||scope->loan!=&loan)throw ScriptError("WorldView provider stamp foreign native scope");
 // No loan callback here: the Bootstrap caller already checked the actual
 // source scope. This pure check cannot recursively enter its own VM guard.
 return StockOperationStamp{p->state,scope->epoch,false};
}
void LuaScriptHost::bind_worldview_sources_owned(
 std::shared_ptr<BnwSelectedWorldViewContextOwner> source,const BnwWorldViewSourceOperationLoan& loan){
 const auto p=impl_;
 if(std::this_thread::get_id()!=p->ui_thread||p->calling||p->failed||!source)
  throw ScriptError("WorldView native source binding thread/state refused");
 loan.require_same_vm(*this);source->require_sources_current();loan.require_same_vm(*this);
 if(p->worldview_sources_bound){
  const auto actual=p->worldview_sources.lock();
  if(!actual||actual.get()!=source.get()||actual.owner_before(source)||source.owner_before(actual)||
     p->context_sources.get()!=source.get())throw ScriptError("WorldView native source member changed");
  return;
 }
 // Existing Civilopedia execution is retained unchanged. Its root did not
 // install a child-source channel; this closed native binding cannot replace
 // any existing source owner or retained child/environment.
 if(p->context_sources||!p->child_contexts.empty()||p->end_turn_lifetime)
  throw ScriptError("WorldView cannot replace an existing source/VM lifetime channel");
 p->context_sources=source;p->worldview_sources=source;p->worldview_sources_bound=true;
 p->worldview_mouse_events=source->native_mouse_callback_events();
 p->end_turn_lifetime=std::make_shared<ui::BnwEngineUiEndTurnQueue::VmLifetime>();
 p->end_turn_lifetime->host=this;p->end_turn_lifetime->vm=p->state;
 loan.require_same_vm(*this);source->require_sources_current();loan.require_same_vm(*this);
}
void LuaScriptHost::bind_worldview_input_policy_owned(const std::shared_ptr<ui::CivisInputPolicyOwner>& policy,
 const BnwWorldViewSourceOperationLoan& loan){
 if(std::this_thread::get_id()!=impl_->ui_thread)throw ScriptError("WorldView input policy binding foreign creator");
 const auto p=impl_;
 if(p->calling||p->failed||!policy||!p->worldview_sources_bound)
  throw ScriptError("WorldView input policy binding lacks healthy actual native Host");
 loan.require_same_vm(*this);policy->require_for_source(loan);loan.require_same_vm(*this);
 if(!p->end_turn_lifetime||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
    p->end_turn_lifetime->host!=this||p->end_turn_lifetime->vm!=p->state||p->worldview_input_policy_bound)
  throw ScriptError("WorldView input policy cannot replace another Host/controller lifetime");
 p->worldview_input_policy=policy;p->worldview_input_policy_bound=true;
 policy->require_for_source(loan);loan.require_same_vm(*this);
 if(!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed)
  throw ScriptError("WorldView input policy Host retired during publication");
}
void LuaScriptHost::bind_worldview_locale_owned(ScriptLocaleProvider provider,const BnwWorldViewSourceOperationLoan& loan){
 const auto p=impl_;
 if(std::this_thread::get_id()!=p->ui_thread||p->calling||p->failed||!provider.current||!provider.select||!provider.convert_text_key)
  throw ScriptError("WorldView authentic Locale bind state/provider missing");
 loan.require_same_vm(*this);
 if(p->locale.current||p->locale.select||p->locale.convert_text_key)throw ScriptError("WorldView cannot replace existing selected Locale issuer");
 p->locale=std::move(provider);loan.require_same_vm(*this);
}
void LuaScriptHost::load_worldview_context_owned(const BnwWorldViewSourceOperationLoan& loan,
 std::shared_ptr<const ScriptUiChildSource> source,std::shared_ptr<ScriptUiControlGraph> graph){
 const auto p=impl_;
 try{
  Impl::WorldViewNativeHostScope native(*p,*this,loan);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);
  if(!source||!graph)throw ScriptError("WorldView native canonical source/control graph missing");
  loan.require_same_source(*source);p->worldview_require_callback_origin();
  const std::string context(source->context());p->worldview_require_callback_origin();
  if(context!=graph->context()||(context!="ActionInfoPanel"&&context!="WorldView"&&context!="UnitFlagManager"))
   throw ScriptError("Native selected child load is outside authenticated stock contexts");
  for(const auto& child:p->child_contexts)if(child.name==context)
   throw ScriptError("WorldView actual context cannot be loaded/replayed twice");
  if(p->executed_contexts.contains(context))throw ScriptError("WorldView selected context already executed");
  bool registered=false;for(const auto& entry:p->control_graphs)
   if(entry.graph.get()==graph.get()&&!entry.graph.owner_before(graph)&&!graph.owner_before(entry.graph))registered=true;
  if(!registered)throw ScriptError("WorldView native load graph is not actual registered member");
  const auto slot=p->child_contexts.size();p->child_contexts.emplace_back();auto& child=p->child_contexts.back();
  child.name=context;child.source=source;child.loading=true;native.child=slot;native.child_source=source;
  source->require_current();p->worldview_require_callback_origin();
  child.asset=&source->asset();p->worldview_require_callback_origin();
  child.bytes=child.asset->read_bytes(Impl::child_source_limit);p->worldview_require_callback_origin();
  source->require_captured_bytes(child.bytes);p->worldview_require_callback_origin();
  child.chunk="@"+child.asset->candidate().original_name;
  if(child.chunk.find('\0')!=std::string::npos)throw ScriptError("WorldView source chunk contains NUL");
  p->child_require_current(slot);p->worldview_require_callback_origin();
  Impl::Job job{p.get(),Impl::Operation::native_worldview_load,context,{},{},{},{},{}};job.worldview_child=slot;
  {
   Impl::EndTurnChildLoaderScope loader(*p,p->state);loader.child=slot;
   p->call(job);p->worldview_require_callback_origin();
   if(!child.loaded||child.loading||!p->executed_contexts.contains(context))
    throw ScriptError("WorldView genuine selected Lua load never returned");
   loader.returned=true;
  }
  p->worldview_require_callback_origin();loan.require_same_vm(*this);
  // Source4's global ready flag remains partial. Loading this real context
  // does not mark other stock contexts or the complete screen epoch ready.
  native.returned=true;
 }catch(...){p->worldview_retire_callback_registrations();p->worldview_retire_context_callback_registrations();p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}

namespace civ5::runtime {
#include "lua_unitflag_button_native_unit.inc"
void LuaScriptHost::note_worldview_native_input_entry_owned(){
 const auto p=impl_;
 if(!p||!p->state||p->ui_thread!=std::this_thread::get_id()||p->failed)
  throw ScriptError("Native input entry requires SAME live Host/VM creator");
 //Only closes a read admission. Does not issue input/hover/callback completion.
 p->worldview_native_input_entered=true;
}
bool LuaScriptHost::dispatch_worldview_input_owned(const BnwWorldViewSourceOperationLoan& loan,
 std::int64_t message,std::int64_t w,std::int64_t l){
 const auto p=impl_;
 try{
  Impl::WorldViewNativeHostScope scope(*p,*this,loan);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
  std::size_t child=0;for(;child<p->child_contexts.size();++child)if(p->child_contexts[child].name=="WorldView")break;
  if(child==p->child_contexts.size()||!p->child_contexts[child].loaded||p->child_contexts[child].loading||p->child_contexts[child].retired)
   throw ScriptError("WorldView input needs genuinely loaded selected child");
  scope.child=child;scope.child_source=p->child_contexts[child].source;
  p->worldview_require_callback_origin();loan.require_same_source(*p->child_contexts[child].source);p->worldview_require_callback_origin();
  //The current route does not produce canonical enter/exit flags40. Mark
  //the actual entry BEFORE callbacks;40 cannot admit old pre-input storage.
  p->worldview_native_input_entered=true;
  const bool consumed=p->worldview_dispatch_owned_context_input(child,message,w,l);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
  scope.returned=true;return consumed;
 }catch(...){p->worldview_retire_context_callback_registrations();p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::dispatch_worldview_shutdown_owned(const BnwWorldViewSourceOperationLoan& loan){
 const auto p=impl_;
 try{
  Impl::WorldViewNativeHostScope scope(*p,*this,loan);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
  std::size_t child=0;for(;child<p->child_contexts.size();++child)if(p->child_contexts[child].name=="UnitFlagManager")break;
  if(child==p->child_contexts.size()||!p->child_contexts[child].loaded||p->child_contexts[child].loading||p->child_contexts[child].retired)
   throw ScriptError("UnitFlag shutdown needs genuinely loaded selected child");
  scope.child=child;scope.child_source=p->child_contexts[child].source;
  p->worldview_require_callback_origin();loan.require_same_source(*p->child_contexts[child].source);p->worldview_require_callback_origin();
  p->worldview_dispatch_owned_context_shutdown(child);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
  scope.returned=true;
 }catch(...){p->worldview_retire_context_callback_registrations();p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}

#include "lua_worldview_created_dispatch.inc"
#include "lua_unitflag_health_membership_body.inc"
#include "lua_unitflag_health_unit_reference.inc"
#include "lua_worldview_health_damage_return.inc"

#include "lua_native_context_state_api.inc"
#include "lua_native_context_setup_state_name_api.inc"
#include "lua_native_context_debug_config_api.inc"
#include "lua_native_context_profile_restart_api.inc"
#include "lua_native_context_controls_table_api.inc"
#include "lua_native_child_context_controls_export_api.inc"
#include "lua_native_child_context_controls_v64_return_api.inc"
#include "lua_native_root_ordinary_controls_export_api.inc"
#include "lua_native_root_ordinary_controls_v64_return_api.inc"
#include "lua_native_context_entered_controls_api.inc"
#include "lua_native_context_postread_globals_api.inc"
#include "lua_native_context_postread_root_cdata_api.inc"
#include "lua_native_context_postread_load_tail_api.inc"
#include "lua_native_context_controls_named_retire_api.inc"
#include "lua_native_context_load_setup_prefix_api.inc"
#include "lua_clear_hex_highlight_owned_api.inc"
#include "lua_native_context_graph_registration_api.inc"
#include "lua_worldview_unitflag_inflight_api.inc"
#include "lua_worldview_unitflag_inflight_ids.inc"

#include "lua_context_loader_temporary_api.inc"
#include "lua_context_initial_showhide_api.inc"
#include "lua_native_context_initial_api.inc"
#include "lua_native_context_direct_initial_lua_api.inc"
