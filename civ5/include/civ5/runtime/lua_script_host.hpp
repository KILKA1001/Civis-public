namespace civ5::ui::detail {class NotificationContextRootOrdinaryControlsExportLoan;class NotificationContextRootOrdinaryControlsExportReturn;}
namespace civ5::ui {class CivisInputPolicyOwner;class BnwWorldViewControlTree;namespace detail {class NotificationContextChildControlsExportLoan;class NotificationContextLuaControlsNamedRetirement;class NotificationContextChildControlsExportReturn;class NotificationContextLuaStatePreparation;class NotificationContextLoadNameLoan;class NotificationContextLuaLoadSetupReturn;class NotificationContextLuaControlsNamedReference;class NotificationContextRootDefaultFieldsReturn;class NotificationContextLoadNameLoan;class NotificationContextLuaStateNamePrefixReturn;class NotificationContextLuaDebugConfigPrefix;class NotificationContextLuaProfileReturn;class NotificationContextLuaControlsTableReturn;class NotificationContextLuaControlsNamedReference;class NotificationContextRootDefaultFieldsReturn;class NotificationContextCursorLoadPrefixReturn;class NotificationContextCursorRestoreReturn;class NotificationContextLoaderCleanupReturn;}}
namespace civ5::ui::detail {class NotificationContextPostreadGlobalsInputLoan;class NotificationContextPostreadGlobalsReference;class NotificationContextPostreadRootCDataReturn;}
#pragma once
#include "civ5/runtime/bnw_unitflag_health_fifo_loan.hpp"
#include "civ5/ui/bnw_engine_ui_end_turn_queue.hpp"
#include "civ5/graphics/bnw_unit_setup_graphical_transport.hpp"
#include "civ5/runtime/locale_text_provider.hpp"
#include "civ5/runtime/selected_game_option_provider.hpp"
#include "civ5/runtime/civis_units_query_provider.hpp"
#include "civ5/assets/asset_source.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
namespace civ5::ui {class BnwTechAwardContextOwner;class BnwTechAwardInputDispatch;}
namespace civ5::runtime {
class BnwWorldViewSourceOperationLoan;
class BnwSelectedWorldViewContextOwner;
enum class BnwSelectedProductionLuaEvent;
using ScriptValue=std::variant<std::monostate,bool,std::int64_t,double,std::string>;
struct ScriptBudget {
    // Caller policies, not x86 capacities. Zero means no host-imposed limit.
    std::uint64_t memory_bytes=0,instructions=0;
};
template<class T> struct ScriptCallResult {std::optional<T> value;std::vector<std::string> errors;};
using ScriptTestResult=ScriptCallResult<bool>;
using ScriptIntegerResult=ScriptCallResult<std::int64_t>;
using ScriptNumberResult=ScriptCallResult<double>;
struct ScriptSourceProvider {
    // Explicit ordered IncludeFileList. Discovery order is a content decision,
    // not an alphabetical order or an arbitrary scan of every installed mod.
    std::function<std::vector<std::string>(std::string_view context)> files;
    std::function<std::string(std::string_view context,std::string_view path)> read;
    // Map generation uses exact selected helper names and explicit
    // cycle/missing errors. Other contexts keep native include matching.
    bool strict_map_includes = false;
};
struct ScriptSourceError {std::string context,path,message;};
using ScriptRow=std::vector<std::pair<std::string,ScriptValue>>;
struct ScriptEvaluatedFile {std::optional<std::string> md5,path;};
struct ScriptModdingProvider {
    // An immutable registry for this host's lifetime. Rows preserve SQL column
    // order (including duplicate names); later columns overwrite earlier ones.
    std::function<std::vector<ScriptRow>(std::string_view type)> entry_points;
    std::function<std::optional<ScriptEvaluatedFile>(std::string_view mod,std::int64_t version,
                                                   std::string_view path)> evaluated_file;
};
class ScriptQuery {
  public:
    virtual ~ScriptQuery()=default;
    virtual std::optional<ScriptRow> next()=0;
    virtual bool step()=0;
    virtual ScriptRow row()const=0;
    virtual std::pair<bool,ScriptValue> cache_key(bool id)const=0;
    virtual void reset()=0;
    virtual void bind(std::span<const ScriptValue>)=0;
    virtual void release()=0;
    virtual std::string_view error()const=0;
};
class ScriptSaveDataConnection {
  public:
    virtual ~ScriptSaveDataConnection()=default;
    virtual std::unique_ptr<ScriptQuery> query(std::string_view,std::span<const ScriptValue>)=0;
};
struct ScriptSaveDataProvider {
    // Each OpenSaveData creates a distinct connection to the session's saved
    // database. Null means unavailable; it must not synthesize lost mod state.
    std::function<std::unique_ptr<ScriptSaveDataConnection>()> open;
};
struct ScriptUserDataProvider {
    // Per-mod named/versioned store. The provider owns the session storage;
    // these callbacks never write the original Civ V ModUserData files.
    std::function<std::unique_ptr<ScriptSaveDataConnection>(std::string_view,std::int64_t)> open;
    std::function<void(std::string_view,std::int64_t)> erase;
};
struct ScriptPregameProvider {
    std::function<std::int64_t()> active_player;
    std::function<std::int64_t(std::int64_t)> slot_status,civilization;
    std::function<void(std::int64_t player,std::int64_t leader)> set_leader_type;
    std::function<std::int64_t(std::int64_t)> player_color;
    std::function<void(std::int64_t player,std::int64_t color)> set_player_color;
};
class BnwLuaCalculatedPlotYieldOwner;
enum class ScriptPlotQuery {x,y,index,water,mountain,impassable};
enum class ScriptMapSetupPlotQuery {x,y,index,plot_type,terrain_type,is_river,feature_type,is_lake};
enum class ScriptMapSetupAreaQuery {num_tiles,num_river_edges};
enum class ScriptMapSetupRiverEdge {north_east,west,north_west};
struct ScriptMapCapabilities {
    // A compatibility profile may expose optional Lua methods/arguments.
    // The common host does not select a GameCore by name.
    bool team_impassability_argument=false,plot_index_alias=false;
};
struct ScriptMapWorldRow {
    std::int64_t rowid = 0, id = 0;
    std::string type;
    std::int64_t feature_grain_change = 0;
};
struct ScriptMapFeatureRow {
    std::int64_t rowid=0,id=0;
    std::string type;
};
struct ScriptMapClimateRow {
    std::int64_t rowid = 0, id = 0;
    std::string type;
};
struct ScriptMapProvider {
    // A setup host may supply only grid_size and optional random. A live map
    // supplies all seven geometry/plot callbacks together. Owners and the
    // source profile must outlive the host.
    std::function<std::pair<std::int64_t,std::int64_t>()> grid_size;
    std::function<std::pair<bool,bool>()> wraps;
    std::function<std::optional<std::int64_t>(std::int64_t,std::int64_t)> plot;
    std::function<std::optional<std::int64_t>(std::int64_t)> plot_by_index;
    std::function<std::optional<std::int64_t>(std::int64_t,std::int64_t,std::int64_t)> direction;
    std::function<std::int64_t(std::int64_t,std::int64_t,std::int64_t,std::int64_t)> distance;
    std::function<ScriptValue(std::int64_t,ScriptPlotQuery,std::int64_t)> plot_value;
    ScriptMapCapabilities capabilities;
    // Optional world service; GetResourceType is exposed only when supplied.
    std::function<std::int64_t(std::int64_t,std::int64_t)> resource_type;
    // BNW Map.Rand uses the session map stream and its required log label.
    // The caller owns narrowing to the original unsigned-short range.
    std::function<std::int64_t(std::int64_t,std::string_view)> random;
    // Fresh-map setup services. They are supplied only from explicit choices;
    // live-map providers continue to use their own geometry callbacks.
    std::function<std::int64_t()> setup_world_size;
    std::function<std::optional<std::int64_t>(std::string_view)> setup_custom_option;
    // Only selected, validated Worlds rows are visible during BNW map setup.
    std::optional<std::vector<ScriptMapWorldRow>> setup_worlds;
    // Read from the selected, privately postprocessed BNW Defines database.
    std::function<std::optional<ScriptValue>(std::string_view)> setup_define;
    // Selected Terrain Info type IDs for fresh map generation only.
    std::function<std::int64_t(std::string_view)> setup_terrain_info_type;
    // CvMapInitData wrap flags become visible after GetMapInitData completes.
    std::function<std::pair<bool,bool>()> setup_wraps;
    // Bounded BNW CvFractal handles are owned by the map setup session.
    std::function<std::int64_t(std::int64_t,std::int64_t,std::int64_t,
                               std::int64_t,std::int64_t,std::int64_t)> setup_fractal_create;
    // Actual paired lifetime: the SAME map session owns each published ID.
    std::function<void(std::int64_t)> setup_fractal_release;
    // Actual owned rift handle is resolved by the SAME map session.
    std::function<std::int64_t(std::int64_t,std::int64_t,std::int64_t,
        std::int64_t,std::int64_t,std::int64_t,std::int64_t)> setup_fractal_create_rifts;
    std::function<std::int64_t(std::int64_t,std::int64_t,
                               std::optional<std::int64_t>)> setup_fractal_height;
    std::function<void(std::int64_t,std::int64_t,std::int64_t,
                       std::int64_t,std::int64_t)> setup_fractal_ridges;
    // CvMap::m_iGridSize, resolved from validated GetMapInitData dimensions.
    std::function<std::int64_t()> setup_plot_count;
    std::function<std::optional<std::int64_t>(std::int64_t)> setup_plot_by_index;
    std::function<std::int64_t(std::int64_t,ScriptMapSetupPlotQuery)> setup_plot_value;
    std::function<bool(std::int64_t)> setup_plot_adjacent_land;
    std::function<bool(std::int64_t)> setup_plot_adjacent_shallow_water;
    std::function<bool(std::int64_t)> setup_plot_coastal_land;
    std::function<bool(std::int64_t,std::int64_t,bool)> setup_find_water;
    std::function<std::int64_t(std::int64_t)> setup_plot_area_id;
    std::function<std::int64_t(std::int64_t,ScriptMapSetupAreaQuery)> setup_area_value;
    std::function<std::optional<std::int64_t>(std::int64_t)> setup_plot_inland_corner;
    std::function<std::optional<std::int64_t>(std::int64_t,std::int64_t,std::int64_t)> setup_plot_direction;
    std::function<bool(std::int64_t,ScriptMapSetupRiverEdge)> setup_plot_river_edge;
    std::function<void(std::int64_t,ScriptMapSetupRiverEdge,bool,std::int64_t)> setup_plot_set_river_edge;
    std::function<void(std::int64_t,std::int64_t,bool,bool,bool)> setup_plot_set_type;
    std::function<void(std::int64_t,std::int64_t,bool,bool)> setup_plot_set_terrain;
    std::function<void(std::int64_t,std::int64_t)> setup_plot_set_area;
    // CvLuaMap::lCalculateAreas during the selected fresh AddLakes phase.
    std::function<void()> setup_calculate_areas;
    // Genuine early full area/landmass rebuild; distinct from AddLakes CalculateAreas.
    std::function<void()> setup_recalculate_areas;
    // Selected Features enum IDs for BNW FeatureGenerator setup only.
    std::function<std::int64_t(std::string_view)> setup_feature_info_type;
    std::function<std::optional<std::int64_t>(bool)> setup_find_biggest_area;
    std::function<bool(std::int64_t,std::int64_t)> setup_plot_can_have_feature;
    std::optional<std::int64_t> setup_climate;
    std::optional<std::vector<ScriptMapClimateRow>> setup_climates;
    std::optional<std::vector<ScriptMapFeatureRow>> setup_features;
    // Detached AddFeatures preflight. This rejects until every native
    // CvPlot::setFeatureType effect can be committed atomically.
    std::function<void(std::int64_t,std::int64_t,std::int64_t)> setup_plot_set_feature_type;
    // LAST optional field: genuine read owner; bDisplay is a native selector.
    std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner> calculated_yield={};
};
// The host retains each issued reference across Lua allocations. Implementations
// validate the actual unit allocation, world/source/rules loan on every getter.
// No copied row or caller-supplied alive/cargo boolean issues this capability.
class ScriptUnitReadReference {
 public:
    virtual ~ScriptUnitReadReference()=default;
    virtual void require_current()const=0;
    virtual ScriptValue read(std::string_view method)const=0;
};
// Bounded native iterator quote: the authentic provider verifies SAME player,
// occupied slot order and allocation/header/operation stability on each next.
class ScriptUnitRosterQuote {
 public:
    virtual ~ScriptUnitRosterQuote()=default;
    virtual void require_current(std::int64_t actual_player_state)const=0;
    virtual const std::vector<std::int64_t>& ids()const=0;
};
class ScriptUnitReadProvider {
 public:
    virtual ~ScriptUnitReadProvider()=default;
    virtual void require_current()const=0;
    virtual std::shared_ptr<const ScriptUnitReadReference> find(std::int64_t owner,std::int64_t full_id)const=0;
    virtual std::shared_ptr<const ScriptUnitRosterQuote> capture_roster(std::int64_t owner)const=0;
};

struct ScriptGameplayProvider {
    std::function<std::unique_ptr<ScriptQuery>(std::string_view sql,std::span<const ScriptValue>)> query;
    // Authoritative object IDs, including inactive slots in an imported game.
    // Getter calls read live shared state; they do not copy teams or players.
    std::function<std::vector<std::int64_t>(bool teams)> ids;
    std::function<bool(std::int64_t team,std::int64_t technology)> has_tech;
    std::function<std::int64_t(std::int64_t player)> player_team;
    std::function<bool(std::int64_t player)> player_alive;
    std::function<std::int64_t(std::int64_t player)> player_leader;
    // Snapshot of the authoritative GC type catalog, copied per Lua context.
    std::function<std::vector<std::pair<std::string,std::int64_t>>()> info_types;
    std::function<std::int64_t()> game_turn;
    std::function<bool(std::int64_t,std::int64_t)> player_human;
    bool human_reason=false;
    std::function<std::int64_t()> game_speed;
    std::function<bool(std::int64_t player)> player_ever_alive;
    std::function<bool(std::int64_t player)> player_minor;
    std::function<std::int64_t(std::int64_t player)> player_minor_type;
    std::function<std::int64_t(std::int64_t player)> player_civilization;
    // Opt-in retained read capability; appended to preserve existing aggregates.
    std::shared_ptr<const ScriptUnitReadProvider> units;
    // Actual selected rules connection diagnostics. Empty callbacks are
    // unissued invocation providers, never an implicit zero-valued report.
    std::function<std::int64_t()> db_statement_count;
    std::function<std::optional<std::string>(std::int64_t)> db_statement_sql;
    // Original Database heap fields in native order, including Uncommited.
    // This must describe the real dedicated database allocator, not SQLite
    // global memory counters or the process CRT heap.
    std::function<ScriptRow()> db_memory_usage;
    std::function<void()> db_collect_memory_usage;
};
struct ScriptUiEventProvider {
    // The active graphics/UI layer declares the notifications it can emit and
    // the commands it can perform. No event name is enabled implicitly by a
    // particular mod or GameCore profile.
    std::vector<std::string> notifications, commands;
    // Commands are synchronous and must reach the actual UI implementation.
    // Scalar arguments are supported here; structured UI values need a
    // separate verified bridge rather than a lossy conversion.
    std::function<void(std::string_view, std::span<const ScriptValue>)> command;
};
struct ScriptUiLayoutControl {
    std::string id, kind;
    bool has_geometry = false;
    bool has_callbacks = false;
};
struct ScriptUiLayoutProvider {
    // Owned by one visible UI context and one measured frame. The host does
    // not manufacture viewport dimensions or control geometry from Lua.
    std::string context_id;
    std::vector<ScriptUiLayoutControl> controls;
    std::function<std::pair<std::int64_t,std::int64_t>()> screen_size;
    std::function<std::pair<std::int64_t,std::int64_t>(std::string_view)> get_size;
    std::function<void(std::string_view,std::int64_t,std::int64_t)> set_size;
    std::function<void(std::string_view,std::int64_t,std::int64_t)> set_offset;
    std::function<void(std::string_view)> calculate_internal_size;
    // Opt-in control callbacks. Values come from the selected source enum
    // profile; they are never guessed by the common host. The owner check is
    // evaluated before registration and invocation on the host's UI thread.
    std::vector<std::int64_t> callback_events;
    std::function<bool()> callback_owner_current;
};
struct ScriptContentPackage {
    std::string id, description;
    bool gameplay_active=false;
};
struct ScriptContentProvider {
    // Read-only installed DLC inventory and this session's selected gameplay
    // activation. Mods are exposed through Modding, not this DLC inventory.
    bool available=false;
    std::vector<ScriptContentPackage> packages;
    std::int64_t gameplay_type=1;
};
struct ScriptEnumTable {
    std::string name;
    std::vector<std::pair<std::string,std::int64_t>> members;
};
struct ScriptEnumProvider {
    // The selected GameCore module supplies its exposed Lua enum tables. The
    // common VM copies them into each context without choosing a profile.
    std::vector<ScriptEnumTable> tables;
};
struct ScriptLanguage {std::string type,display_name;};
struct ScriptLocaleProvider {
    // Selected UI language is session state. The graphics owner can observe a
    // revision change after SetCurrentLanguage and refresh displayed text.
    std::function<ScriptLanguage()> current;
    std::function<bool(std::string_view)> select;
    std::function<std::string(std::string_view,std::span<const ScriptValue>)> convert_text_key;
};
struct ScriptUiEventResult {
    std::uint64_t listeners=0;
    std::vector<std::string> errors;
};
// Independent opt-in capabilities; existing provider layouts stay unchanged.
struct ScriptUiTextEntryProvider {
    std::string context_id,control_id;
    std::function<std::string()> get_text;
    std::function<void(std::string_view)> replace_text;
};
struct ScriptUiContextProvider {
    std::string context_id;
    std::function<bool()> is_hidden;
    std::function<void(bool)> set_hidden;
};
struct ScriptUiTooltipControl {std::string id,kind;};
struct ScriptUiTooltipProvider {
    std::string context_id,type_name;
    std::vector<ScriptUiTooltipControl> controls;
    std::function<void()> registered;
    std::function<void(std::string_view,std::string_view)> set_texture;
    std::function<void(std::string_view,std::int64_t,std::int64_t)> set_texture_offset;
    std::function<void(std::string_view,bool)> set_hidden;
};
// Civis semantic tokens mapped into this requested Lua context. These values
// do not claim original KeyEvents/Keys numeric ABI equivalence.
enum class ScriptUiKeyEvent { key_down=1,key_up=2 };
enum class ScriptUiKey { other=0,escape=1,enter=2 };
struct ScriptUiInputEvent {
    ScriptUiKeyEvent event=ScriptUiKeyEvent::key_down;
    ScriptUiKey key=ScriptUiKey::other;
};
struct ScriptUiInputResult:ScriptUiEventResult {bool handled=false;};
struct ScriptMessage {std::string context,text;};
struct ScriptError:std::runtime_error {using std::runtime_error::runtime_error;};
class ScriptUiControlGraph;
// Generic owned source channel. Implementations validate their genuine lease,
// source byte witness and parent/child membership on every currentness check.
// A Lua path, context name or AssetCandidate DTO cannot issue this capability.
// This interface does not supply a selected InGame activation issuer.
class ScriptUiChildSource {
  public:
    virtual ~ScriptUiChildSource()=default;
    virtual std::string_view context()const=0;
    virtual const assets::AssetHandle& asset()const=0;
    virtual void require_current()const=0;
    // Authenticate the actual captured buffer against this SAME source pin,
    // including transient substitution which before/after file stat misses.
    virtual void require_captured_bytes(std::span<const std::uint8_t>)const=0;
};
class ScriptUiContextSourceOwner {
  public:
    virtual ~ScriptUiContextSourceOwner()=default;
    // child==nullptr is a root explicitly owned by this provider. Otherwise
    // validate the exact retained descriptor, not its copied name or fields.
    virtual void require_current(std::string_view context,const ScriptUiChildSource *child)const=0;
    virtual std::shared_ptr<const ScriptUiChildSource> resolve_child(
        std::string_view parent,const ScriptUiChildSource *parent_source,std::string_view request)=0;
};
class ScriptUiUpdateSourceOwner;
// A retained observation issued by the bound frame owner, never an argument
// supplied to dispatch by context name/delta. Implementations authenticate
// their genuine frame lease, freshness and SAME source-owner channel.
class ScriptUiUpdateFrame {
  public:
    virtual ~ScriptUiUpdateFrame()=default;
    virtual const ScriptUiUpdateSourceOwner& issuer()const=0;
    virtual std::uint64_t sequence()const=0;
    virtual double delta_seconds()const=0;
    virtual void require_current(const ScriptUiContextSourceOwner&)const=0;
};
class ScriptUiUpdateSourceOwner {
  public:
    virtual ~ScriptUiUpdateSourceOwner()=default;
    virtual void require_current(const ScriptUiContextSourceOwner&)const=0;
    // Consume exactly one pending frame, or none. No selected frame/window
    // issuer is provided here; the real integration must supply this owner.
    virtual std::unique_ptr<const ScriptUiUpdateFrame> consume_frame()=0;
};
// Primitive constructor data only. The caller supplies its real initial values.
struct ScriptMapInitFields {
    std::int32_t width=0,height=0;
    bool wrap_x=false,wrap_y=false;
    std::int32_t top_latitude=0,bottom_latitude=0;
    bool operator==(const ScriptMapInitFields&)const=default;
};
struct ScriptMapInitQueryResult {
    bool table_result=false;
    ScriptMapInitFields fields;
};
// Diagnostic action history of this one VM. It never attests the selected
// startup set, absence of listeners, world binding or gameplay admission.
enum class ScriptLuaAction {context_execute,include,child_load,event_add,event_remove,event_clear,event_dispatch};
enum class ScriptLuaActionOutcome {open,succeeded,error,incomplete,uncertain};
struct ScriptLuaExecutionRecord {
    std::uint64_t begin=0,end=0,parent=0,failure_epoch=0,uncertainty_epoch=0;
    ScriptLuaAction action=ScriptLuaAction::context_execute;
    ScriptLuaActionOutcome outcome=ScriptLuaActionOutcome::open;
    std::string context,name,mode,source_sha256;
    std::uint64_t source_bytes=0;
    std::int64_t count_before=0,count_after=0,removed=0;
};
struct ScriptLuaExecutionSnapshot {
    const void* host_allocation=nullptr;std::uint64_t failure_epoch=0,uncertainty_epoch=0;
    bool failed=false;std::vector<ScriptLuaExecutionRecord> records;
};
// Own x64 Lua 5.1 VM. Named contexts have separate globals/event lists.
// No original EXE/DLL, ambient file loader or native extension is used.
class LuaScriptHost {
  public:
    explicit LuaScriptHost(ScriptBudget budget={},ScriptSourceProvider sources={},ScriptModdingProvider modding={},
                           ScriptGameplayProvider gameplay={},ScriptSaveDataProvider save_data={},ScriptPregameProvider pregame={},ScriptMapProvider map={},ScriptUiEventProvider ui_events={},ScriptContentProvider content={},ScriptUserDataProvider user_data={},ScriptEnumProvider enums={},ScriptLocaleProvider locale={});
    LuaScriptHost(ScriptBudget budget,ScriptSourceProvider sources,ScriptModdingProvider modding,
                  ScriptGameplayProvider gameplay,ScriptSaveDataProvider save_data,ScriptPregameProvider pregame,
                  ScriptMapProvider map,ScriptUiEventProvider ui_events,ScriptContentProvider content,
                  ScriptUserDataProvider user_data,ScriptEnumProvider enums,ScriptLocaleProvider locale,
                  ScriptUiLayoutProvider ui_layout);
    ~LuaScriptHost();
    LuaScriptHost(const LuaScriptHost&)=delete;
    LuaScriptHost& operator=(const LuaScriptHost&)=delete;
    void execute(std::string_view context,std::string_view source,std::string_view chunk_name);
    // Protected query of an already retained context in this VM. SAME bound
    // source owner is required; this never creates a context or an activation
    // capability. Missing/nonfunction/nontable returns {false,initial}; all
    // other failures are sticky and publish no result. Integers reuse existing source_integer
    // process-FP rounding; unsupported nonfinite/out-of-int32 cases refuse.
    ScriptMapInitQueryResult query_map_init_data(std::string_view context,
        const ScriptUiContextSourceOwner& same_source_owner,std::int32_t world_id,
        ScriptMapInitFields initial);
    ScriptTestResult test_all(std::string_view event,std::span<const ScriptValue> args={});
    ScriptTestResult test_any(std::string_view event,std::span<const ScriptValue> args={});
    ScriptTestResult call_hook(std::string_view event,std::span<const ScriptValue> args={});
    // UI notifications are separate from gameplay hooks and custom LuaEvents.
    // The graphics/UI owner calls this on the host's thread after supplying a
    // provider. Listener errors are reported, never counted as a successful
    // visual command.
    ScriptUiEventResult emit_ui_event(std::string_view event,std::span<const ScriptValue> args={});
    ScriptUiEventResult dispatch_ui_control(std::string_view context,
        std::string_view control,std::int64_t event,std::span<const ScriptValue> args={});
    std::size_t registered_ui_control_callbacks(std::string_view context) const;
    std::size_t registered_ui_control_callbacks(std::string_view context,
        std::span<const std::string> controls) const;
    // Explicit selected-source capability for known bare-tag Lookup calls.
    // Only these tags with exactly one argument are accepted. Formatting and
    // source/lifetime checks remain the actual declared locale provider's job;
    // this does not enable a general Locale.Lookup alias or native grammar.
    void enable_plain_locale_lookup(std::string context,std::vector<std::string> tags);
    // Explicit owner-provided text operations for this selected context.
    // No engine collation/casing algorithm or numeric ABI is implied.
    void enable_locale_text_operations(std::string context,ScriptLocaleTextProvider provider);
    // Explicit Civis immediate raw-key cardinality in a copied context-local
    // table library. Requires a current declared UI callback owner; no native
    // table.count algorithm/ABI is asserted, and other contexts stay unchanged.
    void enable_civis_raw_table_count(std::string context);
    // Selected diagnostic NO_RELIGION21 only. Copies Game into this context,
    // preserving existing getters; no string/general IsOption ABI is enabled.
    void enable_civis_selected_game_option(std::string context,ScriptSelectedGameOptionProvider provider);
    // Selected first Units SQL only; context-local copied DB preserves Query.
    // Source execution is diagnostic; production callback dispatch still
    // requires complete_loading. Caught service errors reject VM readiness.
    void enable_civis_units_query(std::string context,ScriptCivisUnitsQueryProvider provider);
    ScriptCivisUnitsQueryObservations civis_units_query_observations(std::string_view context)const;
    void enable_ui_text_entry(ScriptUiTextEntryProvider provider);
    void replace_ui_text(std::string_view context,std::string_view control,std::string_view text);
    ScriptUiEventResult submit_ui_text(std::string_view context,std::string_view control);
    void enable_ui_context_handlers(ScriptUiContextProvider provider);
    ScriptUiInputResult dispatch_ui_input(std::string_view context,ScriptUiInputEvent event);
    ScriptUiEventResult dispatch_ui_visibility(std::string_view context,bool hidden);
    std::size_t registered_ui_context_handlers(std::string_view context) const;
    void enable_ui_tooltip(ScriptUiTooltipProvider provider);
    // Closing unregisters retained Lua closures. A closed context cannot be
    // reopened on this VM; create a new context owner and host instead.
    void close_ui_context(std::string_view context);
    // Per-context supporting authored graph, on this same VM. Registration
    // does not activate source or establish native/rendered publication.
    void enable_ui_control_graph(std::shared_ptr<ScriptUiControlGraph> graph);
    // Bind before executing any root. No ambient file resolver is installed.
    // Each child runs on this VM with the existing shared GameEvents groups.
    void enable_ui_context_sources(std::shared_ptr<ScriptUiContextSourceOwner> owner);
    // Before root execution, paired with the SAME context-source owner.
    // SetUpdate(fn) is source-proven; nil unregister is a generic extension.
    void enable_ui_updates(std::shared_ptr<ScriptUiUpdateSourceOwner> owner);
    // One owner-issued frame, context registration order at frame start,
    // callback(delta_seconds). Requires generic loading completion. This does
    // not attest selected startup, actual frame publication or rendered state.
    ScriptUiEventResult dispatch_ui_update();
    std::size_t registered_ui_updates()const;
    // Native CallAccumulator adds the callback sum to the caller's initial
    // value; initial is not passed to Lua. No listeners leaves value absent.
    ScriptIntegerResult accumulate_int(std::string_view event,std::span<const ScriptValue> args={},std::int64_t initial=0);
    ScriptNumberResult accumulate(std::string_view event,std::span<const ScriptValue> args={},double initial=0);
    // The content loader must attest that ALL selected entrypoints ran.
    // Failed chunks cannot be ignored. Another chunk invalidates readiness.
    void complete_loading();
    bool loading_complete() const;
    std::uint64_t memory_bytes() const;
    const std::vector<ScriptSourceError>& source_errors() const;
    const std::vector<ScriptMessage>& messages() const;
    // Actual portable Lua logger output DATA; no native callback registry claim.
    const std::vector<ScriptMessage>& native_lua_log_messages() const;
    // Owner-thread, idle readback; copies actual retained action records even
    // after failure. No reset, completion, listener-absence or issuer API.
    ScriptLuaExecutionSnapshot execution_snapshot() const;
    static std::string_view version();
  private:
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwWorldViewScriptUiAdapter;
    std::pair<std::int32_t,std::int32_t> unitflag_initializing_button_ids_owned(const BnwWorldViewSourceOperationLoan&,
        const std::weak_ptr<const void>&,std::uint64_t,std::uint64_t)const;
    void require_unitflag_initializing_button_owned(const BnwWorldViewSourceOperationLoan&,
        const std::weak_ptr<const void>&,std::uint64_t,std::uint64_t,std::int32_t,std::int32_t)const;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::detail::NotificationContextChildControlsExportReturn;
    friend class ::civ5::ui::detail::NotificationContextRootOrdinaryControlsExportReturn;
    friend class ::civ5::ui::detail::NotificationContextLuaLoadSetupReturn;
    friend class ::civ5::ui::detail::NotificationContextLuaControlsNamedReference;
    friend class ::civ5::ui::detail::NotificationContextLuaControlsNamedRetirement;
    friend class ::civ5::ui::detail::NotificationContextPostreadGlobalsReference;
    friend class ::civ5::ui::detail::NotificationContextPostreadRootCDataReturn;
    std::shared_ptr<const ui::detail::NotificationContextPostreadRootCDataReturn> create_context_postread_root_cdata_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference>&)const;
    void require_context_postread_root_cdata_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const ui::detail::NotificationContextPostreadRootCDataReturn&)const;
    void require_context_postread_root_cdata_owned(const BnwWorldViewSourceOperationLoan&,
        const ui::detail::NotificationContextPostreadRootCDataReturn&)const;
    void require_context_postread_globals_input_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&)const;
    void require_context_postread_globals_input_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&)const;
    std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference> acquire_context_postread_globals_reference_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&)const;
    void require_context_postread_globals_reference_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const ui::detail::NotificationContextPostreadGlobalsReference&)const;
    void require_context_postread_globals_reference_owned(const BnwWorldViewSourceOperationLoan&,
        const ui::detail::NotificationContextPostreadGlobalsReference&)const;
    void require_context_controls_named_retirement_current_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const ui::detail::NotificationContextLuaControlsNamedReference&,
        const ui::detail::NotificationContextLuaControlsNamedRetirement&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedRetirement> retire_context_controls_named_reference_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference>&)const;
    void require_context_lua_load_setup_current_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const ui::detail::NotificationContextLuaLoadSetupReturn&)const;
    void require_context_controls_entered_current_without_callbacks(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&,
        const ui::detail::NotificationContextLuaControlsNamedReference&)const;
    void require_context_controls_entered_current_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&,
        const ui::detail::NotificationContextLuaControlsNamedReference&)const;
    std::shared_ptr<const ui::detail::NotificationContextChildControlsExportReturn> export_context_child_controls_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextChildControlsExportLoan>&)const;
    std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportReturn> export_root_ordinary_controls_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan>&)const;
    // Body owned by Host; only real45731B..47C can issue this return.
    // Required Host-owned actual enteredRead lane. Only completedSetup's own
    // retained StateName/Table controls may be decomposed; no table creation.
    std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> acquire_context_controls_named_reference_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn> prepare_context_lua_load_setup_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepare_context_lua_state_owned(
        const BnwWorldViewSourceOperationLoan&,std::shared_ptr<const ScriptUiChildSource>,const std::weak_ptr<const void>&,
        std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> prepare_context_lua_setup_state_name_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix> prepare_context_lua_debug_config_prefix_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn> complete_context_lua_profile_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix>&)const;
    void require_context_lua_profile_completed_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn>&)const;
    void require_context_lua_debug_config_prefix_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix>&)const;
    void require_context_lua_setup_state_name_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn> prepare_context_lua_controls_table_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&)const;
    void require_context_lua_controls_table_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> acquire_context_controls_named_reference_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&)const;
    void register_context_initial_lua_graph_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<ui::BnwWorldViewControlTree>&)const;
    void invoke_context_initial_lua_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&)const;
    std::shared_ptr<const ui::detail::NotificationContextLoaderCleanupReturn> cleanup_context_loader_temporaries_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextCursorRestoreReturn>&)const;
    void invoke_context_initial_showhide_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&,
        const std::shared_ptr<const ui::detail::NotificationContextCursorRestoreReturn>&,bool)const;
    void require_context_initial_lua_state_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&)const;
    void require_context_lua_state_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&)const;
    void require_context_lua_state_history_owned(const BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&,
        const std::weak_ptr<const void>&,const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>&,
        std::uint64_t original_serial)const;
    friend class BnwUnitFlagHealthInstanceLoan;
    friend class BnwUnitFlagHealthFifoLoan;
    struct UnitDamageEventReturn;
    std::shared_ptr<UnitDamageEventReturn> dispatch_worldview_unit_damage_owned(
        const BnwWorldViewSourceOperationLoan&,const BnwUnitFlagHealthInstanceLoan&,const BnwUnitFlagHealthFifoLoan&);
    BnwUnitFlagHealthFifoLoan::DeliveryReturn complete_unitflag_health_fifo_owned(
        const BnwUnitFlagHealthFifoLoan&,const std::shared_ptr<UnitDamageEventReturn>&);
    std::uint64_t consume_original_unitflag_health_return_without_callbacks(
        const BnwUnitFlagHealthFifoLoan&,const std::shared_ptr<UnitDamageEventReturn>&)const;
    void with_unitflag_health_instance_owned(const class BnwUnitFlagHealthFifoLoan&,
        const class BnwStockLuaAttemptLease&,const class BnwSelectedUnitFlagContextOwner&,
        const std::function<void(const class BnwUnitFlagHealthInstanceLoan&)>&);
    friend class ::civ5::ui::BnwEngineUiEndTurnQueue;
    void prepare_end_turn_consumer_owned(const ScriptUiContextSourceOwner&,std::shared_ptr<const ScriptUiChildSource>);
    std::shared_ptr<const ui::BnwEngineUiEndTurnQueue::Consumer> seal_end_turn_consumer_owned();
    void require_end_turn_consumer_owned(const ui::BnwEngineUiEndTurnQueue::Consumer&)const;
    std::unique_ptr<ui::BnwEngineUiEndTurnQueue::Completion> dispatch_end_turn_dirty_owned(
        const ui::BnwEngineUiEndTurnQueue::Consumer&,const ui::BnwEngineUiEndTurnQueue::Event&);

    ScriptTestResult test_selected_production_owned(BnwSelectedProductionLuaEvent,
        std::int32_t,std::int32_t,std::int32_t,const class BnwStockLuaAttemptLease&);
    ScriptIntegerResult accumulate_engine_founder_owned(std::int32_t,const class BnwStockLuaAttemptLease&);
    ScriptIntegerResult accumulate_initial_unit_founder_owned(std::int32_t,
        const class BnwStockLuaAttemptLease&);
    ScriptTestResult call_initial_unit_setxy_owned(std::int32_t,std::int32_t,
        std::int32_t,std::int32_t,const class BnwStockLuaAttemptLease&);
    ScriptTestResult call_selected_city_trained_owned(std::int32_t,std::int32_t,std::int32_t,const class BnwStockLuaAttemptLease&);
    // Only Bootstrap can load the selected complete source and invoke its
    // retained original OnPopup; a generic scalar UI event cannot supply it.
    void bind_selected_tech_award_context_owned(
        std::shared_ptr<const ui::BnwTechAwardContextOwner>,const class BnwStockLuaAttemptLease&);
    void require_selected_tech_award_input_owner_owned(const std::shared_ptr<const ui::BnwTechAwardContextOwner>&,const class BnwStockLuaAttemptLease&)const;
    void dispatch_selected_tech_award_input_owned(std::shared_ptr<const ui::BnwTechAwardInputDispatch>,const class BnwStockLuaAttemptLease&);
    void activate_selected_tech_award_owned(std::string_view,const class BnwStockLuaAttemptLease&);
    void dispatch_selected_tech_award_owned(std::int64_t,std::int64_t,std::int64_t,
        std::int32_t,bool,bool,const class BnwStockLuaAttemptLease&);
    // Actual SourceOwner-issued one-operation service, never generic gameplay callbacks.
    ScriptIntegerResult accumulate_owned_team_happiness_founder(std::int64_t player);
    void bind_owned_team_set_has_tech(std::shared_ptr<const class BnwOwnedTeamSetHasTechLuaDispatch>);
    void invoke_owned_team_set_has_tech(std::string_view retained_context,std::int64_t team,
        std::int64_t technology,bool value,std::int64_t player,bool announce);
    friend class BnwWorldViewScriptUiAdapter;
    void require_unitflag_button_unit_owned(const BnwWorldViewSourceOperationLoan&,std::int32_t,std::int32_t)const;
    struct StockOperationStamp {const void* vm;std::uint64_t failure_epoch;bool failed;};
    StockOperationStamp stock_operation_stamp()const;
    // Available only during the actual include compiler's native provider
    // lender. This never grants loading completion or changes the general stamp.
    std::optional<StockOperationStamp> stock_include_provider_stamp()const;
    std::optional<StockOperationStamp> stock_worldview_provider_stamp(const BnwWorldViewSourceOperationLoan&)const;
    void bind_worldview_sources_owned(std::shared_ptr<BnwSelectedWorldViewContextOwner>,const BnwWorldViewSourceOperationLoan&);
    void bind_worldview_locale_owned(ScriptLocaleProvider,const BnwWorldViewSourceOperationLoan&);
    void bind_worldview_input_policy_owned(const std::shared_ptr<ui::CivisInputPolicyOwner>&,const BnwWorldViewSourceOperationLoan&);
    friend class BnwWorldViewNativeContextIssuer;
    void drain_clear_hex_highlight_events_owned(const BnwWorldViewSourceOperationLoan&)const;
    void load_worldview_context_owned(const BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const ScriptUiChildSource>,std::shared_ptr<ScriptUiControlGraph>);

    //Closes constructor-only hover reads at actual native capture, no Lua call.
    void note_worldview_native_input_entry_owned();
    bool dispatch_worldview_input_owned(const BnwWorldViewSourceOperationLoan&,
        std::int64_t message,std::int64_t w,std::int64_t l);
    void dispatch_worldview_shutdown_owned(const BnwWorldViewSourceOperationLoan&);
    void dispatch_worldview_unit_created_owned(const BnwWorldViewSourceOperationLoan&,
        const graphics::BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan&);
    struct Impl;std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
