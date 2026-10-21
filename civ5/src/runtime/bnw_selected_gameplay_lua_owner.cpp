#include "civ5/runtime/bnw_local_client_scalar_owner.hpp"
#include "civ5/runtime/bnw_sp_client_update_loan.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_treasury_same_vm_income_loan.hpp"
#include <map>
#include <deque>
#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/graphics/bnw_initial_center_unit_renderer_loan.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/graphics/bnw_selected_pending_unit_visual_loan.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_player_turn_active_dirty_issuer.hpp"
#include "civ5/runtime/bnw_major_tech_transport_consumer.hpp"
#include "civ5/runtime/bnw_owned_team_set_has_tech_lua_dispatch.hpp"
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/ui/selected_city_production_services.hpp"
#include "civ5/ui/bnw_train_renderer_refresh_receipt.hpp"
#include "civ5/graphics/bnw_train_presented_frame_proof.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/ui/bnw_alpha_progression_backend.hpp"
#include "../ui/bnw_native_completed_color_query.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/ui/bnw_city_production_ui_adapter.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/ui/bnw_civilopedia_scroll_subtree.hpp"
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include "civ5/ui/civis_civilopedia_input_owner.hpp"
#include "civ5/ui/civis_civilopedia_tooltip_owner.hpp"
#include "civ5/ui/civis_locale_text_owner.hpp"
#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/selected_bnw_stock_ui_lua_step.hpp"
#include "civ5/ui/bnw_civilopedia_native_runtime_owner.hpp"
#include <fstream>
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <sstream>
#include <limits>
#include <set>
#include <span>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <exception>
#include <type_traits>
#include <utility>

namespace civ5::runtime {
namespace {
void visit_contexts(const content::XmlNode &node,
                    BnwSelectedStockLuaEvidence &evidence) {
    if(node.name=="LuaContext") {
        if(!evidence.declared_in_game_contexts)
            evidence.first_context_id=node.attribute("ID");
        ++evidence.declared_in_game_contexts;
    }
    for(const auto &child:node.children) visit_contexts(child,evidence);
}

std::string verified_source(const std::filesystem::path &path,
                            std::string *sha=nullptr) {
    const auto before=identify_file(path);
    if(before.bytes>64ULL*1024*1024)
        throw std::runtime_error("Selected BNW Lua source exceeds 64 MiB");
    std::ifstream stream(path,std::ios::binary);
    if(!stream) throw std::runtime_error("Selected BNW Lua source cannot be opened");
    std::string bytes((std::istreambuf_iterator<char>(stream)),
                      std::istreambuf_iterator<char>());
    if(!stream.eof() && stream.fail())
        throw std::runtime_error("Selected BNW Lua source cannot be read");
    if(identify_bytes(std::span<const std::uint8_t>(
           reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size())).sha256
           !=before.sha256)
        throw std::runtime_error("Selected BNW Lua source changed during read");
    if(sha) *sha=before.sha256;
    return bytes;
}
}

struct BnwSelectedStockLuaSourceOwner::StockThreadLease {
    const std::thread::id thread=std::this_thread::get_id();
    mutable std::atomic<bool> poisoned{false};std::atomic<bool> retired{false};
    void check()const {
        if(std::this_thread::get_id()!=thread){poisoned.store(true);throw std::runtime_error("Selected stock owner foreign creation thread");}
        if(poisoned.load()||retired.load())throw std::runtime_error("Selected stock owner retired/poisoned");
    }
};

BnwSelectedStockLuaSourceOwner::BnwSelectedStockLuaSourceOwner(
    std::filesystem::path game_root,content::Catalog catalog,
    SavedContentProfile selected,SessionCheckpointIdentity identity)
    :game_root_(std::move(game_root)),catalog_(std::move(catalog)),
     selected_(std::move(selected)),identity_(std::move(identity)),stock_thread_(std::make_shared<StockThreadLease>()) {
    try {
        if(!selected_.can_activate || !selected_.all_requirements_located ||
           !selected_.enabled_mods.empty() ||
           identity_.payload_sha256.empty() || identity_.rules_sha256.empty())
            throw std::runtime_error("Selected stock BNW Lua profile or identity unavailable");
        const auto step=ui::execute_selected_bnw_first_stock_ui_include(
            game_root_,catalog_,selected_);
        if(step.status!=ui::SelectedBnwStockUiLuaStatus::first_include_executed)
            throw std::runtime_error(step.blocker);
        evidence_.selected_profile=true;
        evidence_.first_include_executed=true;
        evidence_.declaration_sha256=step.declaration_sha256;
        evidence_.first_source_sha256=step.context_source_sha256;
        evidence_.first_include_sha256=step.include_sha256;
        const auto xml=content::read_xml(
            game_root_/"Assets/DLC/Expansion2/UI/InGame/InGame.xml");
        if(xml.name!="Context")
            throw std::runtime_error("Selected BNW InGame XML has no Context root");
        visit_contexts(xml,evidence_);
        if(evidence_.first_context_id!=step.context_id ||
           evidence_.declared_in_game_contexts<2)
            throw std::runtime_error("Selected BNW InGame LuaContext order changed");
        evidence_.blocker=
            "CivilopediaScreen parent Lua chunk and remaining selected contexts/includes/listeners are not activated";
    } catch(const std::exception &error) {
        evidence_.blocker=error.what();
    }
}

struct BnwSelectedStockLuaSourceOwner::StockBootstrap {
    std::shared_ptr<StockThreadLease> thread;
    mutable std::atomic<bool> poisoned{false};
    std::shared_ptr<const ui::SelectedBnwPreviewHost> selected;
    const content::RulesDatabase* rules=nullptr;
    std::shared_ptr<content::RulesDatabase> saved_database;
    std::function<bool()> rules_current;
    std::shared_ptr<const LiveSessionState> state;
    const saves::SavedSession* source=nullptr;
    const std::uint8_t* payload=nullptr;
    std::size_t payload_size=0;
    std::uint64_t generation=0;
    std::int64_t turn=0;
    std::array<std::uint8_t,16> map_guid{};
    SessionCheckpointIdentity identity;
    std::unique_ptr<BnwSelectedStockLuaBootstrap> vm;
    std::shared_ptr<BnwLocalClientScalarOwner> local_client;
    std::weak_ptr<ui::BnwEngineUiTurnOwner> golden_age_event_engine;
 std::weak_ptr<ui::BnwEngineUiTurnOwner> game_turn_event_engine;
    // Once per actual original actor/turn; strong receipt has only weak Source backrefs.
    std::map<std::pair<std::int64_t,std::int64_t>,std::shared_ptr<const BnwLivePlayerGoldPending>> live_gold_components;
    struct LiveGoldCombatScalar {
     const saves::UnitCoreField* field=nullptr;
     std::uint64_t begin=0,end=0;std::int64_t value=0;
    };
    struct LiveGoldCombatRow {
     const saves::SavedUnitObservation* unit=nullptr;
     std::uint64_t index=0,core_begin=0,core_end=0;
     std::size_t field_count=0;LiveGoldCombatScalar id,owner,base;
    };
    static LiveGoldCombatScalar live_gold_combat_scalar(const saves::SavedUnitObservation& unit,const char* name){
     const saves::UnitCoreField* found=nullptr;
     for(const auto& field:unit.core.fields)if(field.name&&std::string_view(field.name)==name){
      if(found)throw std::runtime_error("Deficit raw duplicate native combat operand");found=&field;
     }
     if(!found||found->encoding!=saves::UnitCoreEncoding::i32||found->values.size()!=1||
        !found->bytes.empty()||found->wire_count||found->values.front()<INT32_MIN||found->values.front()>INT32_MAX)
      throw std::runtime_error("Deficit actual source typed combat operand unavailable");
     return {found,found->start_offset,found->end_offset,found->values.front()};
    }
    static std::vector<LiveGoldCombatRow> capture_live_gold_combat_rows(const LiveSessionState& state,std::int64_t actor){
     if(actor<0||actor>=64||!state.world.all_players||state.world.all_players->players.size()!=64)
      throw std::runtime_error("Deficit actual source roster unavailable");
     const auto& units=state.world.all_players->players.at(static_cast<std::size_t>(actor)).units;
     if(units.units.size()>static_cast<std::size_t>(INT32_MAX)||units.header.wire_unit_count!=static_cast<std::int64_t>(units.units.size()))
      throw std::runtime_error("Deficit actual source roster extent unavailable");
     std::vector<LiveGoldCombatRow> rows;rows.reserve(units.units.size());
     for(const auto& unit:units.units){LiveGoldCombatRow row;
      row.unit=&unit;row.index=unit.index;row.core_begin=unit.core.start_offset;row.core_end=unit.core.end_offset;
      row.field_count=unit.core.fields.size();row.id=live_gold_combat_scalar(unit,"m_iID");
      row.owner=live_gold_combat_scalar(unit,"m_eOwner");row.base=live_gold_combat_scalar(unit,"m_iBaseCombat");
      if(row.id.value<0||row.owner.value!=actor)throw std::runtime_error("Deficit actual fullID/owner unavailable");
      rows.push_back(row);
     }return rows;
    }
    static void require_live_gold_combat_rows_without_callbacks(const LiveSessionState& state,std::int64_t actor,
     const std::vector<LiveGoldCombatRow>& rows,std::int32_t returned_count){
     if(actor<0||actor>=64||!state.world.all_players||state.world.all_players->players.size()!=64)
      throw std::runtime_error("Deficit actual source roster retired");
     const auto& units=state.world.all_players->players.at(static_cast<std::size_t>(actor)).units;
     if(units.units.size()!=rows.size()||units.header.wire_unit_count!=static_cast<std::int64_t>(rows.size()))
      throw std::runtime_error("Deficit actual source roster extent changed after count");
     const auto equal=[](const LiveGoldCombatScalar& before,const LiveGoldCombatScalar& now){
      return before.field==now.field&&before.begin==now.begin&&before.end==now.end&&before.value==now.value;
     };
     std::int32_t count=0;
     for(std::size_t i=0;i<rows.size();++i){const auto& unit=units.units[i];const auto& row=rows[i];
      // Compare addresses before reading current fields; saved stale pointers
      // are identities only and are never dereferenced after callbacks.
      if(row.unit!=&unit||row.index!=unit.index||row.core_begin!=unit.core.start_offset||row.core_end!=unit.core.end_offset||
         row.field_count!=unit.core.fields.size()||!equal(row.id,live_gold_combat_scalar(unit,"m_iID"))||
         !equal(row.owner,live_gold_combat_scalar(unit,"m_eOwner"))||!equal(row.base,live_gold_combat_scalar(unit,"m_iBaseCombat")))
       throw std::runtime_error("Deficit actual fullID/owner/BaseCombat/source operands changed after query");
      if(row.base.value>0){if(count==INT32_MAX)throw std::runtime_error("Deficit actual combat count overflow");++count;}
     }
     if(count!=returned_count)throw std::runtime_error("Deficit Core query disagrees with sealed actual combat operands");
    }
    struct LiveGoldDeficitReturn {
     std::weak_ptr<const BnwLivePlayerGoldPending> pending;
     std::weak_ptr<const BnwNativeUnitAllocatorCore> core;
     std::weak_ptr<const LiveSessionState> world;
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> source;
     std::int64_t actor=-1,turn=-1;std::uint64_t serial=0,generation=0,revision=0;
     std::int32_t combat_count=-1,city_count=-1;std::vector<LiveGoldCombatRow> combat_rows;
     SimulationRandomState map_rng{};
     SimulationRandomState game_rng{};
     bool unit_query_returned=false,city_query_returned=false;
    };std::optional<LiveGoldDeficitReturn> live_gold_deficit_return;
    bool running=false;
    std::shared_ptr<BnwStockLuaAttemptLease::Control> operation_control;
    std::uint64_t operation_world_revision=0;
    // Weak Source only: Source -> Services -> stock owner is already strong.
    // Existing history survives expiration, preventing another prefix issue.
    struct TrainAttempt {std::weak_ptr<const ui::SelectedCityTrainCreateSource> source;std::uint64_t before=0,occurrence=0;};
    std::map<std::pair<std::int32_t,std::int32_t>,TrainAttempt> train_attempts;
    // Actual returned core receipt only; strong ownership would cycle through
    // retained Hooks/ActionBinding. Expired association is still a refusal.
    // Weak only: receipt retains Source/Core/Services/stock; strong here cycles.
    // Expired control remains a mandatory barrier until actual disposition.
    std::weak_ptr<const ui::BnwTrainRendererRefreshReceipt> pending_train_renderer;
    std::uint64_t pending_train_renderer_serial=0,pending_train_renderer_revision=0;
    std::weak_ptr<const BnwResearchChoiceCommit> pending_research;
    std::uint64_t pending_research_serial=0,pending_research_revision=0;
    BnwStockLuaOperationState operation_state=BnwStockLuaOperationState::idle_suspended;
    std::vector<BnwStockLuaOperationRecord> journal;
    std::exception_ptr retained_error;
    void quarantine()noexcept {
        poisoned.store(true);
        if(operation_control)operation_control->invalidated.store(true);
        operation_state=BnwStockLuaOperationState::uncertain;
        if(!journal.empty())journal.back().outcome=BnwStockLuaOperationOutcome::uncertain;
    }
    void require_no_pending_train_renderer()const {
        thread->check();
        const std::weak_ptr<const ui::BnwTrainRendererRefreshReceipt> empty;
        if(pending_train_renderer.owner_before(empty)||empty.owner_before(pending_train_renderer))
            throw std::runtime_error("Actual TRAIN prefix awaits presentation; unrelated production preview denied before VM provider/serial");
    }
    void check_active()const {
        thread->check();
        // Active-slot metadata comes BEFORE any DB/session/world borrow.
        if(poisoned.load()||!operation_control||operation_control->invalidated.load()||!running||
           operation_state!=BnwStockLuaOperationState::active||operation_control->vm!=vm.get())
            throw std::runtime_error("Selected stock source has no SAME active operation/VM");
        check(true);
        if(vm)vm->require_vm_current_stamp();
        if(state->ui_source_revision!=operation_world_revision)
            throw std::runtime_error("Selected stock operation world revision changed");
    }
    struct OperationLoan {
        StockBootstrap& owner;
        explicit OperationLoan(StockBootstrap& o):owner(o){}
        OperationLoan(const OperationLoan&)=delete;
        OperationLoan& operator=(const OperationLoan&)=delete;
        ~OperationLoan(){
            owner.running=false;
            if(owner.operation_state==BnwStockLuaOperationState::active)owner.quarantine();
        }
    };
    void record_retained() {
        if(journal.empty())return;
        const auto serial=journal.back().serial;
        const auto kind=journal.back().kind;
        const auto outcome=journal.back().outcome;
        auto record=vm?vm->retained_operation_record():BnwStockLuaOperationRecord{};
        record.serial=serial;record.kind=kind;record.outcome=outcome;
        journal.back()=std::move(record);
    }
    std::weak_ptr<const BnwPolicyAdoptionPending> pending_policy_adoption;
    std::shared_ptr<const BnwPolicyAdoptionPending> retained_policy_adoption_receipt;
    struct PolicyAdoptHookReturn {
      std::weak_ptr<const BnwPolicyAdoptionPending> source;
      std::weak_ptr<const LiveSessionState> world;
      const BnwSelectedStockLuaBootstrap* vm=nullptr;
      std::uint64_t generation=0,revision=0,serial=0;
      std::int64_t actor=-1,policy=-1;
      ScriptTestResult actual;
      bool call_returned=false;
    };
    std::optional<PolicyAdoptHookReturn> policy_adopt_hook_return;
    template<class F>auto run_operation(BnwStockLuaOperationKind kind,F&& actual)
        ->std::invoke_result_t<F> {
        thread->check();
        const std::weak_ptr<const BnwPolicyAdoptionPending> empty_policy;
        if(pending_policy_adoption.owner_before(empty_policy)||empty_policy.owner_before(pending_policy_adoption))
            throw std::runtime_error("Actual incomplete policy adoption awaits mandatory native suffix; VM action denied");
        const std::weak_ptr<const ui::BnwTrainRendererRefreshReceipt> empty_train;
        if((pending_train_renderer.owner_before(empty_train)||empty_train.owner_before(pending_train_renderer))&&
           kind!=BnwStockLuaOperationKind::train_renderer_refresh)
            throw std::runtime_error("Actual incomplete TRAIN awaits genuine post-present refresh; VM action denied");
        const std::weak_ptr<const BnwResearchChoiceCommit> empty_pending;
        if((pending_research.owner_before(empty_pending)||empty_pending.owner_before(pending_research))&&
           kind!=BnwStockLuaOperationKind::research_refresh)
            throw std::runtime_error("Actual research commit awaits post-dispatch presentation; VM action denied");
        try{check();}catch(...){if(!retained_error)retained_error=std::current_exception();quarantine();throw;}
        if(!operation_control||operation_control->invalidated.load()){
            quarantine();throw std::runtime_error("Stock operation control retired; no replay");
        }
        if(operation_control->serial==std::numeric_limits<std::uint64_t>::max()){
            quarantine();throw std::runtime_error("Stock operation serial exhausted");
        }
        // Journal allocation happens BEFORE any source, native or Lua effect.
        operation_world_revision=state->ui_source_revision;
        journal.emplace_back();journal.back().serial=++operation_control->serial;
        journal.back().kind=kind;
        operation_state=BnwStockLuaOperationState::active;running=true;
        OperationLoan loan(*this);
        try {
            auto result=std::forward<F>(actual)();
            check_active();
            if(vm)vm->require_healthy_owned();
            record_retained();
            journal.back().outcome=BnwStockLuaOperationOutcome::incomplete_suspended;
            operation_state=BnwStockLuaOperationState::returned_suspended;
            return result;
        }catch(...){
            if(!retained_error)retained_error=std::current_exception();
            quarantine();
            try{record_retained();}catch(...){/* Keep the original error and attempted record. */}
            throw;
        }
    }
    void check(bool inside_callback=false)const {
        const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
        thread->check();
        require(!poisoned.load(),"Selected stock VM poisoned");
        require(!operation_control||!operation_control->invalidated.load(),"Selected stock operation control invalidated");
        try {
            require(!running||inside_callback,"Selected stock VM reentrant operation");
            // Factory captures the actual sealed DB's SAME Impl guard, never
            // a caller callback or the immutable host identity string alone.
            require(&selected->database()==rules&&rules_current&&rules_current(),
                    "Selected stock VM actual rules scope retired/changed");
            require(selected->saved_database_.get()==saved_database.get(),
                    "Selected stock saved SQL allocation changed");
            const auto& session=selected->session();
            const auto current=session.read_state_lease();
            // source() dereferences an owned pointer: check moved-from state first.
            require(current&&current.get()==state.get()&&session.state_generation()==generation,
                    "Selected stock VM state allocation/generation changed");
            const auto& retained=session.source();
            require(&retained==source&&retained.source.bytes.data()==payload&&retained.source.bytes.size()==payload_size,
                    "Selected stock VM actual source allocation changed");
            require(selected->identity().payload_sha256==identity.payload_sha256&&
                    selected->identity().rules_sha256==identity.rules_sha256&&
                    current->setup.profile==content::TechnologyProfile::bnw&&
                    current->setup.game_turn==turn&&current->world.map_header.guid==map_guid,
                    "Selected stock VM game/rules identity changed");
            // Genuine private Game::Read ownership is invalidated by mutable_state.
            // This prevents copied VM getters surviving arbitrary in-place writes.
            (void)session.bnw_game_debug_mode();
        }catch(...){poisoned.store(true);throw;}
    }
    BnwStockLuaBootstrapProgress advance(){
        return run_operation(BnwStockLuaOperationKind::advance,[this]{
            if(!vm)throw std::runtime_error("Selected stock VM start failed; no replay");
            const auto result=vm->advance_owned();
            if(result.outcome==BnwStockLuaOperationOutcome::uncertain){
                if(result.error)std::rethrow_exception(result.error);
                throw std::runtime_error("Stock VM returned retained uncertainty");
            }
            return *result.progress;
        });
    }
};

#ifdef _WIN32
struct BnwSelectedStockLuaSourceOwner::WorldViewInputQueue {
 const std::thread::id creator=std::this_thread::get_id();
 bool alive=true,failed=false,active=false,shutdown_returned=false;
 std::weak_ptr<const void> source;
 std::weak_ptr<const BnwStockLuaAttemptLease::Control> control;
 const void* source_get=nullptr;
 const BnwStockLuaAttemptLease::Control* control_get=nullptr;
 struct Row {
  std::shared_ptr<const ui::BnwWorldViewWindowInputLoan> native;
  std::weak_ptr<const LiveSessionState> world;
  const LiveSessionState* world_get=nullptr;
  std::uint64_t generation=0,revision=0;
 };
 std::deque<Row> rows;
};
#endif
BnwSelectedStockLuaSourceOwner::~BnwSelectedStockLuaSourceOwner(){
#ifdef _WIN32
 if(worldview_input_queue_){worldview_input_queue_->alive=false;worldview_input_queue_->failed=true;}
#endif
 retire_stock_bootstrap();
}
void BnwSelectedStockLuaSourceOwner::retire_stock_bootstrap()noexcept {
    // This constructor-owned control is immutable for the facade lifetime.
    // Foreign revocation must not read bootstrap_ or any VM/native members:
    // their owner-thread publication is not synchronized by this atomic flag.
    const auto thread=stock_thread_;
    thread->retired.store(true);
    if(std::this_thread::get_id()!=thread->thread)return;
    const auto bound=bootstrap_;
    if(bound){
        if(bound->operation_control)bound->operation_control->invalidated.store(true);
        if(bound->running)bound->quarantine();
        else if(bound->operation_state!=BnwStockLuaOperationState::uncertain)
            bound->operation_state=BnwStockLuaOperationState::retired;
        if(bound->vm)bound->vm->retire_native_client();
    }
}
BnwStockLuaBootstrapProgress BnwSelectedStockLuaSourceOwner::start_stock_bootstrap(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> selected,std::filesystem::path assets){
    const auto thread=stock_thread_;thread->check();
    if(bootstrap_){bootstrap_->check();throw std::runtime_error("Selected stock VM already started/attempted");}
    if(!selected||!evidence_.selected_profile||!evidence_.first_include_executed||
       evidence_.first_context_id!="CivilopediaScreen"||evidence_.declared_in_game_contexts<2)
        throw std::runtime_error("Actual selected stock owner/profile absent");
    if(selected->identity().payload_sha256!=identity_.payload_sha256||selected->identity().rules_sha256!=identity_.rules_sha256)
        throw std::runtime_error("Actual selected stock host identity differs");
    if(civis_sources_){
        civis_sources_->require_current();civis_sources_->require_same_host(*selected);
        const auto actual_host=civis_sources_->retain_host();
        if(actual_host.get()!=selected.get()||actual_host.owner_before(selected)||selected.owner_before(actual_host))
            throw std::runtime_error("Own stock bootstrap Host get/control differs from its factory");
        if(assets.lexically_normal()!=civis_sources_->recipe().canonical_payload_root.lexically_normal())
            throw std::runtime_error("Own stock bootstrap cannot substitute its captured resource root");
    }
    const auto& rules=selected->database();
    auto rules_current=rules.immutable_preview_fingerprint_currentness();
    if(!rules_current||!rules_current()||rules.gameplay_fingerprint_sha256()!=identity_.rules_sha256||!rules_current())
        throw std::runtime_error("Actual selected stock immutable rules owner unavailable/mismatched");
    auto bound=std::make_shared<StockBootstrap>();bound->thread=thread;bound->selected=std::move(selected);
    bound->rules=&rules;bound->rules_current=std::move(rules_current);
    bound->state=bound->selected->session().read_state_lease();
    if(!bound->state)throw std::runtime_error("Actual selected stock session absent");
    if(std::this_thread::get_id()!=bound->selected->saved_database_thread_)
        throw std::runtime_error("Selected saved SQL host belongs to another thread");
    bound->saved_database=bound->selected->saved_database_;
    if(bound->saved_database&&bound->selected->saved_database_claimed_.exchange(true))
        throw std::runtime_error("Selected saved SQL already has an attempted stock owner");
    bound->generation=bound->selected->session().state_generation();
    bound->source=&bound->selected->session().source();
    bound->payload=bound->source->source.bytes.data();bound->payload_size=bound->source->source.bytes.size();
    bound->identity=identity_;bound->turn=bound->state->setup.game_turn;bound->map_guid=bound->state->world.map_header.guid;
    bound->operation_control=std::make_shared<BnwStockLuaAttemptLease::Control>();
    bound->operation_control->world=bound->state;
    bound->operation_control->source_owner=bound;
    bound->operation_control->selected_host=bound->selected;
    const std::weak_ptr<StockBootstrap> weak_attempt=bound;
    bound->operation_control->current=[thread,weak_attempt]{
        thread->check(); // Before weak owner resolution and mutable fields.
        const auto actual=weak_attempt.lock();
        if(!actual)throw std::runtime_error("Stock source attempt expired");
        actual->check_active();
    };
    bound->check();bootstrap_=bound; // One attempted VM, also if construction fails.
    const BnwStockLuaAttemptLease operation(bound->operation_control);
    return bound->run_operation(BnwStockLuaOperationKind::prepare,[&]{
        auto session=std::shared_ptr<const LiveSession>(bound->selected,&bound->selected->session());
        auto yield_rules=std::make_shared<const BnwNormalPlotYieldRules>(
            BnwNormalPlotYieldRules::load(bound->selected->database(),identity_.rules_sha256));
        auto calculated=make_bnw_lua_calculated_plot_yield_owner(std::move(session),std::move(yield_rules));
        calculated->bind_stock_epoch([operation]{operation.require_active();});
        auto live_map=bnw_lua_calculated_plot_services(calculated);
        if(civis_sources_){
            civis_sources_->require_current();civis_sources_->require_same_host(*bound->selected);
            bound->vm=std::unique_ptr<BnwSelectedStockLuaBootstrap>(new BnwSelectedStockLuaBootstrap(
                civis_sources_,identity_,bound->state,bound->selected->database(),
                std::move(live_map),operation,bound->saved_database));
            civis_sources_->require_current();
        }else bound->vm=std::unique_ptr<BnwSelectedStockLuaBootstrap>(new BnwSelectedStockLuaBootstrap(
            std::move(assets),catalog_,selected_,identity_,bound->state,
            bound->selected->database(),std::move(live_map),operation,bound->saved_database));
        bound->operation_control->vm=bound->vm.get();
        bound->vm->require_prepared_owned();
        // Fresh standalone SP client lifetime: real restored BNW mode, SAME
        // newly constructed VM/source/world, then concrete native client reset.
        if(bound->local_client)throw std::runtime_error("SP client birth already attempted");
        if(bound->state->setup.profile!=content::TechnologyProfile::bnw||bound->state->setup.game_type!=0||
           !bound->state->world.pregame||bound->state->world.pregame->layout!=saves::GameStateLayout::expansion2_version1)
            throw std::runtime_error("SP client birth requires actual ordinary BNW pregame");
        const auto& native_types=bound->state->world.pregame->archive.integers;
        const auto native_type=native_types.find("game_type");
        if(native_type==native_types.end()||native_type->second!=bound->state->setup.game_type)
            throw std::runtime_error("SP client birth native pregame/setup mode differs");
        bound->local_client=std::shared_ptr<BnwLocalClientScalarOwner>(new BnwLocalClientScalarOwner(operation,bound->vm.get(),bound->selected->session().bnw_imported_pregame_read_cache_,bound->selected->database()));

        const auto& prepared=bound->vm->progress();
        if(prepared.selected_sources_verified&&
           (prepared.parent_sha256!=evidence_.first_source_sha256||prepared.declaration_sha256!=evidence_.declaration_sha256))
            throw std::runtime_error("Persistent stock parent/declaration differs from selected source owner");
        const auto result=bound->vm->advance_owned();
        if(result.outcome==BnwStockLuaOperationOutcome::uncertain){
            if(result.error)std::rethrow_exception(result.error);
            throw std::runtime_error("Stock VM preparation returned retained uncertainty");
        }
        // The first owned advance allocated the genuine LuaScriptHost and
        // bound its SAME VM incarnation. Attach the already constructed
        // client only after that successful return, before any WorldView use.
        calculated->bind_local_client(bound->local_client);
        return *result.progress;
    });
}

saves::CivisSaveSection BnwSelectedStockLuaSourceOwner::snapshot_stock_save_data(){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;
    if(!bound)throw std::runtime_error("Selected saved SQL has no attempted source owner");
    bound->check();
    if(!bound->vm||!bound->saved_database)
        throw std::runtime_error("Selected saved SQL image/actual VM is absent");
    // A known unfinished SQL transaction/query is a nonmutating export refusal,
    // not an uncertain Lua callback; let genuine subsequent Lua finish it.
    bound->saved_database->require_saved_checkpoint_boundary();
    return bound->run_operation(BnwStockLuaOperationKind::save_data_snapshot,[&]{
        bound->vm->require_healthy_owned();
        bound->saved_database->require_saved_checkpoint_boundary();
        auto image=bound->saved_database->saved_image();
        bound->saved_database->require_saved_checkpoint_boundary();
        // SAME source/rules/world bookends, DB domain ONLY: no world/once grant.
        return saves::CivisSaveSection{static_cast<std::uint32_t>(
            saves::CivisSaveSectionId::saved_database),0,std::move(image)};
    });
}

// NEXT draft; final author must splice from exact frozen Graph14 and pin the
// immutable consumer/GUI packets. No legacy constructor issues action authority.
std::shared_ptr<BnwSelectedStockLuaSourceOwner> BnwSelectedStockLuaSourceOwner::create_selected(
    std::filesystem::path root,content::Catalog catalog,SavedContentProfile selected,
    SessionCheckpointIdentity identity) {
    auto result=std::shared_ptr<BnwSelectedStockLuaSourceOwner>(new BnwSelectedStockLuaSourceOwner(
        std::move(root),std::move(catalog),std::move(selected),std::move(identity)));
    result->stock_thread_->check();
    result->action_self_=result;
    return result;
}
std::shared_ptr<BnwSelectedStockLuaSourceOwner>
BnwSelectedStockLuaSourceOwner::retain_action_factory_owner()const {
    const auto thread=stock_thread_;thread->check(); // BEFORE facade/control fields.
    const auto actual=action_self_.lock();
    if(!actual||actual.get()!=this||actual.owner_before(action_self_)||action_self_.owner_before(actual))
        throw std::runtime_error("Stock action requires actual canonical selected factory owner");
    thread->check();
    return actual; // Genuine factory allocation; caller aliases cannot mint it.
}

std::shared_ptr<BnwSelectedStockLuaSourceOwner>
BnwSelectedStockLuaSourceOwner::retain_production_entry_owner_without_callbacks()const{
 const auto canonical=retain_action_factory_owner(); // creator BEFORE fields
 const auto bound=canonical->bootstrap_;
 if(bound){
  bound->require_no_pending_train_renderer(); // weak membership, even expired
  if(bound->poisoned.load()||(bound->operation_control&&bound->operation_control->invalidated.load()))
   throw std::runtime_error("Production actual stock operation retired");
 }
 return canonical; // no callbackful currentness or world borrow here
}

std::shared_ptr<ui::SelectedCityProductionServices>
BnwSelectedStockLuaSourceOwner::create_selected_city_production_services(){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();
 const auto bound=canonical->bootstrap_;
 canonical->retain_production_entry_owner_without_callbacks();
 if(!bound||!bound->vm||!bound->selected||!bound->selected->session_)
  throw std::runtime_error("Production actual canonical Host/session/stock VM absent");
 bound->check();bound->vm->require_vm_current_stamp();thread->check();
 // SourceOwner is the existing private friend of actual Host/LiveSession.
 // The immutable Host facade owns this real mutable pointee; no const_cast.
 const auto services=ui::SelectedCityProductionServices::from_actual_source_owner(
  bound->selected,*bound->selected->session_,canonical);
 bound->check();bound->vm->require_vm_current_stamp();thread->check();return services;
}

std::shared_ptr<ui::BnwAlphaProgressionBackend>
BnwSelectedStockLuaSourceOwner::create_alpha_progression_backend(
    std::shared_ptr<ui::BnwClientSceneFrame> scene) {
    const auto thread=stock_thread_;thread->check();
    const auto canonical=retain_action_factory_owner();
    const auto bound=canonical->bootstrap_;
    if(!bound||!bound->vm||!scene)
        throw std::runtime_error("Alpha research canonical VM/registered scene absent");
    bound->check();bound->vm->require_vm_current_stamp();
    const auto selected=bound->selected;
    // The sealed backend factory itself authenticates the actual scene
    // allocation/control and its privately retained canonical selected Host.
    const auto result=ui::BnwAlphaProgressionBackend::bind(canonical,selected,std::move(scene));
    bound->check();bound->vm->require_vm_current_stamp();thread->check();
    return result;
}


// Draft for the NEXT immutable supplier. Requires the actual Category research
// GUI Loan child and the peer's immutable core child; not a Graph14 output.

std::shared_ptr<const BnwPolicyAdoptionPending>
BnwSelectedStockLuaSourceOwner::run_same_policy_adoption_prefix(const BnwPolicyAdoptionCommand& borrowed,std::shared_ptr<const ui::LiveUiHost> actual_ui) {
 stock_thread_->check();const auto command=borrowed;const auto canonical=retain_action_factory_owner();
 const auto bound=canonical->bootstrap_;if(!bound||!bound->vm)throw std::runtime_error("Policy canonical SAME VM absent");
 bound->check();std::shared_ptr<const BnwPolicyAdoptionPending> returned;bool native_policy_returned=false;
 try {
  bound->run_operation(BnwStockLuaOperationKind::policy_adoption,[bound,command,&returned,actual_ui,canonical,&native_policy_returned]{
   bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
   const auto serial=lease.operation_serial(),before=bound->operation_world_revision;
   if(command.player!=bound->selected->active_player_id())throw std::runtime_error("Policy actual active player changed");
   auto hooks=BnwPolicyAdoptionHooks::issue(bound->selected,lease,
    [bound,lease,serial,command](std::int64_t player,std::int64_t policy){
     bound->check_active();lease.require_operation(serial);
     if(player!=command.player||policy!=command.policy)throw std::runtime_error("Policy TestAll changed actual operands");
     const auto actual=bound->vm->test_policy_adoption_owned(player,policy,lease);
     bound->check_active();lease.require_operation(serial);return actual;
    },
    [bound,lease,serial,before,command](const BnwPolicyAdoptionPending& actual){
     bound->thread->check();bound->check(true);
     actual.require_stored_current(*bound->selected);
     if(actual.phase()!=BnwPolicyAdoptionPhase::uncertain||actual.player()!=command.player||actual.policy()!=command.policy||
        actual.before_revision()!=before||actual.previous_revision()!=bound->operation_world_revision||actual.after_revision()<=actual.previous_revision()||
        bound->state->ui_source_revision!=actual.after_revision())throw std::runtime_error("Policy opaque SAME-state mutation changed");
     bound->check(true);bound->operation_world_revision=actual.after_revision();
     bound->check_active();lease.require_operation(serial);
    },canonical);
   // The closed no-callback sink captures canonical Pending before the first
   // native/cache store, including failures inside the initial execution.
   bound->retained_policy_adoption_receipt.reset();
   returned=BnwPolicyAdoptionPending::execute_owned(*bound->selected->session_,std::move(hooks),command,returned);
   bound->retained_policy_adoption_receipt=returned;
   if(!returned)throw std::runtime_error("Policy prefix returned no native owner");
   // One genuine Pending/root command. The child finisher uses the SAME
   // original processPolicies pipeline; it never pays/adopts/tests a new root.
   if(returned->phase()==BnwPolicyAdoptionPhase::branch_finisher_pending)returned->advance_branch_finisher();
   const auto drive_inner_process=[&]{
    if(returned->phase()==BnwPolicyAdoptionPhase::process_policies_pending)returned->advance_process_initial_scalars();
    if(returned->phase()==BnwPolicyAdoptionPhase::worker_speed_pending)returned->advance_process_worker_scalars();
    if(returned->phase()==BnwPolicyAdoptionPhase::treasury_route_gold_pending)returned->advance_process_treasury_scalars();
    if(returned->phase()==BnwPolicyAdoptionPhase::city_strength_pending)returned->advance_process_city_scalars();
    if(returned->phase()==BnwPolicyAdoptionPhase::spaceship_hurry_pending)returned->advance_process_extra_scalars();
    if(returned->phase()==BnwPolicyAdoptionPhase::one_shot_pending)returned->advance_process_one_shot();
    while(returned->phase()==BnwPolicyAdoptionPhase::yield_arrays_pending||returned->phase()==BnwPolicyAdoptionPhase::plot_yield_pending||(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)){
     if(returned->phase()==BnwPolicyAdoptionPhase::yield_arrays_pending)returned->advance_process_yield_arrays();
     if(returned->phase()==BnwPolicyAdoptionPhase::plot_yield_pending)returned->advance_process_plot_yields();
     if(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)returned->consume_actual_policy_plot_ui();
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::unit_combat_pending)returned->advance_process_combat_hurry();
    while(returned->phase()==BnwPolicyAdoptionPhase::improvement_yield_pending||returned->phase()==BnwPolicyAdoptionPhase::improvement_plot_yield_pending||(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)){
     if(returned->phase()==BnwPolicyAdoptionPhase::improvement_yield_pending)returned->advance_process_improvement_yields();
     if(returned->phase()==BnwPolicyAdoptionPhase::improvement_plot_yield_pending)returned->advance_process_plot_yields();
     if(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)returned->consume_actual_policy_plot_ui();
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::free_promotions_pending)returned->advance_process_free_promotions();
    if(returned->phase()==BnwPolicyAdoptionPhase::city_effects_pending)returned->advance_process_city_effects();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_disables_pending)returned->advance_process_rewards();
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_pending)returned->advance_process_golden_age();
    while(returned->phase()==BnwPolicyAdoptionPhase::golden_age_map_pending||(returned->phase()==BnwPolicyAdoptionPhase::golden_age_plot_ui_pending&&actual_ui)){
     if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_map_pending)returned->advance_process_plot_yields();
     if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_plot_ui_pending&&actual_ui)returned->consume_actual_policy_plot_ui();
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_history_pending)returned->advance_process_golden_age_history();
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_replay_pending&&actual_ui){
     const auto replay_lease=returned->begin_golden_age_replay_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_golden_age_replay(*bound->selected->session_->state_,returned,*bound->vm,replay_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_gameplay_pending&&actual_ui){
     const auto gameplay_lease=returned->begin_golden_age_gameplay_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     bound->golden_age_event_engine=receiver;
     receiver->consume_golden_age_gameplay(*bound->selected->session_->state_,returned,gameplay_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_notification_pending&&actual_ui){
     const auto notification_lease=returned->begin_golden_age_notification_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_golden_age_notification(*bound->selected->session_->state_,returned,*bound->vm,notification_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::golden_age_dirty_pending&&actual_ui){
     const auto dirty_lease=returned->begin_golden_age_dirty_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_golden_age_dirty(*bound->selected->session_->state_,returned,dirty_lease);
     bound->check_active();lease.require_operation(serial);
    }
    // Policy55 preserves world revision through the sealed Selection/Engine return.
    // Only afterwards does the genuine native reward tail write additional state.
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_disables_pending&&returned->reward_cursor()==3)
     returned->advance_process_reward_tail();
    if(returned->phase()==BnwPolicyAdoptionPhase::free_techs_pending&&actual_ui){
     const auto text_lease=returned->begin_free_tech_text_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_free_tech_text(*bound->selected->session_->state_,returned,*bound->vm,text_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::free_tech_dirty_pending&&actual_ui){
     const auto dirty_lease=returned->begin_free_tech_dirty_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_free_tech_dirty(*bound->selected->session_->state_,returned,dirty_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::free_tech_notification_pending&&actual_ui){
     const auto notification_lease=returned->begin_free_tech_notification_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     bound->golden_age_event_engine=receiver; // SAME existing post-native common publication consumer
     receiver->consume_free_tech_notification(*bound->selected->session_->state_,returned,notification_lease);
     bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_disables_pending&&returned->reward_cursor()==3&&returned->free_tech_choice_returned())
     returned->advance_process_reward_tail();
    if(returned->phase()==BnwPolicyAdoptionPhase::free_policy_notification_pending&&actual_ui){
     const auto source_scope=lease.pin_active_scope_without_callbacks();
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     lease.require_scope_identity_without_callbacks(source_scope); // FIRST after receiver callbacks
     bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
     lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
     bound->golden_age_event_engine=receiver; // SAME canonical Engine for actual post-native FIFO pump
     receiver->consume_free_policy_notification(*bound->selected->session_->state_,returned,*bound->vm);
     lease.require_scope_identity_without_callbacks(source_scope); // FIRST after native/locale callbacks
     bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
     lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::one_shot_free_units_pending)returned->advance_process_one_shot_units();
    if(returned->phase()==BnwPolicyAdoptionPhase::reformation_pending&&actual_ui){
     // Reserve before callbackful receiver/locale retention. Positive Add is
     // distinct from the negative tail already completed by the native owner.
     const auto notification_lease=returned->begin_reformation_notification_dispatch(*bound->selected,*bound->state);
     try{
      const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
      bound->golden_age_event_engine=receiver; // SAME genuine post-unwind notification/audio FIFO pump
      receiver->consume_reformation_notification(*bound->selected->session_->state_,returned,*bound->vm,notification_lease);
      bound->check_active();lease.require_operation(serial);
     }catch(...){returned->fail_reformation_notification_dispatch();throw;}
    }
    // Actual receiver and SAME source/VM bookends must finish before the
    // once-only native suffix. Never replay units or positive notification.
    if(returned->phase()==BnwPolicyAdoptionPhase::reformation_returned)
     returned->advance_process_post_reformation_tail();
    while(returned->phase()==BnwPolicyAdoptionPhase::resource_policy_yield_pending||(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)||
     (returned->phase()==BnwPolicyAdoptionPhase::resource_policy_layout_pending&&actual_ui)||returned->phase()==BnwPolicyAdoptionPhase::resource_policy_layout_returned){
     if(returned->phase()==BnwPolicyAdoptionPhase::resource_policy_yield_pending)returned->advance_process_plot_yields();
     if(returned->phase()==BnwPolicyAdoptionPhase::plot_ui_pending&&actual_ui)returned->consume_actual_policy_plot_ui();
     if(returned->phase()==BnwPolicyAdoptionPhase::resource_policy_layout_pending&&actual_ui){
      // One issued native layout call for this actual plot. The next visible
      // matching plot issues a distinct call through this SAME finite loop.
      const auto layout_lease=returned->begin_resource_policy_layout_dispatch(*bound->selected,*bound->state);
      try{
       const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
       receiver->consume_resource_policy_layout(*bound->selected->session_->state_,returned,layout_lease);
       bound->check_active();lease.require_operation(serial);
      }catch(...){returned->fail_resource_policy_layout_dispatch();throw;}
     }
     // Real Selection PlotData33 then runtime plot-layout stores have returned;
     // resume the map only AFTER receiver and SAME source/VM bookends.
     if(returned->phase()==BnwPolicyAdoptionPhase::resource_policy_layout_returned)
      returned->resume_after_resource_policy_layout();
    }
    // Headless visible plots keep their actual mandatory layout frontier.
    // Runtime layout publication never claims saved state or GPU delivery.
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_pending)returned->advance_process_happiness_prefix();
    while(returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_founder_pending||returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_foreign_founder_pending){
     const auto actor=returned->begin_happiness_founder_dispatch();bound->check_active();lease.require_operation(serial);
     const auto result=bound->vm->accumulate_team_happiness_founder_owned(actor,lease);
     returned->finish_happiness_founder_dispatch(result.value);bound->check_active();lease.require_operation(serial);
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_suffix_pending)returned->advance_process_happiness_suffix();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_dirty_pending&&actual_ui){
     // Original DoUpdateHappiness requires unconditional GameData11 after the
     // native suffix; no active-player predicate or successful headless skip.
     const auto dirty_lease=returned->begin_happiness_dirty_dispatch(*bound->selected,*bound->state);
     try{
      const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
      receiver->consume_happiness_dirty(*bound->selected->session_->state_,returned,dirty_lease);
      bound->check_active();lease.require_operation(serial);
     }catch(...){returned->fail_happiness_dirty_dispatch();throw;}
    }
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_happiness_dirty_returned)
     returned->resume_after_happiness_dirty();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_trade_pending)returned->advance_process_trade_and_great_people_prefix();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_great_people_traits_pending)returned->advance_process_great_people_traits();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_great_people_policies_pending)returned->advance_process_great_people_policy_buildings();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_great_people_friendships_pending)returned->advance_process_great_people_suffix();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_post_gp_cost_pending)returned->advance_process_policy_cost_and_experience();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_camp_visibility_pending)returned->advance_process_camp_visibility();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_process_dirty_pending&&actual_ui){
     const auto dirty_lease=returned->begin_policy_process_dirty_dispatch(*bound->selected,*bound->state);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     receiver->consume_policy_process_dirty(*bound->selected->session_->state_,returned,dirty_lease);
    }
    // A child native return may resume ONLY after actual receiver returns and
    // the final SAME-source/VM bookends. Never resume inside the UI receiver.
    bound->check_active();lease.require_operation(serial);
   };
   drive_inner_process();
   if(returned->phase()==BnwPolicyAdoptionPhase::finisher_process_returned){
    returned->resume_after_finisher_process();
    drive_inner_process();
   }
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_cost_pending)returned->advance_outer_next_policy_cost();
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_next_cost_pending)returned->advance_outer_next_policy_cost();
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_branch_pending)returned->advance_outer_policy_branch();
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_branch_rewards_pending)returned->advance_outer_policy_branch_zero_reward();
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_pending&&actual_ui){
    const auto source_scope=lease.pin_active_scope_without_callbacks();
    try{
     // The actual Policy supplier reserves its once-only slot before its callbacks.
     const auto notification_lease=returned->begin_outer_branch_notification_dispatch(*bound->selected,*bound->state);
     lease.require_scope_identity_without_callbacks(source_scope); // FIRST after reserve/source callbacks
     notification_lease.require_scope_identity_without_callbacks(source_scope);
     bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
     notification_lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
     lease.require_scope_identity_without_callbacks(source_scope); // FIRST after receiver retention
     bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
     notification_lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
     bound->golden_age_event_engine=receiver;
     receiver->consume_outer_branch_notifications(*bound->selected->session_->state_,returned,*bound->vm,notification_lease);
     lease.require_scope_identity_without_callbacks(source_scope); // FIRST after actual Engine dispatch
     bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
     notification_lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
     // This supplier accepts ONLY an earned batch with zero emitted client effects.
     // A publication-pending batch stays pending; the common pump cannot resume it.
     if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_returned){
      returned->finish_outer_branch_notifications_without_publication();
      lease.require_scope_identity_without_callbacks(source_scope); // FIRST after genuine zero-effect tail
      bound->check_active();lease.require_scope_identity_without_callbacks(source_scope);
      notification_lease.require_operation(serial);lease.require_scope_identity_without_callbacks(source_scope);
     }
    }catch(...){returned->fail_outer_branch_notification_dispatch();throw;}
   }
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_dirty_pending&&actual_ui){
    const auto dirty_lease=returned->begin_outer_policy_dirty_dispatch(*bound->selected,*bound->state);
    const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
    receiver->consume_outer_policy_dirty(*bound->selected->session_->state_,returned,dirty_lease);
   }
   if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_hook_pending){
    const auto hook_lease=returned->begin_outer_policy_hook_dispatch(*bound->selected,*bound->state);
    const auto hook_serial=hook_lease.operation_serial();bound->policy_adopt_hook_return.reset();
    try{
     bound->check_active();hook_lease.require_operation(hook_serial);
     returned->require_outer_policy_hook_source(*bound->selected,*bound->state);
     StockBootstrap::PolicyAdoptHookReturn receipt;
     receipt.source=returned;receipt.world=bound->state;receipt.vm=bound->vm.get();receipt.generation=bound->generation;
     receipt.revision=bound->state->ui_source_revision;receipt.serial=hook_serial;receipt.actor=returned->player();receipt.policy=returned->policy();
     receipt.actual=bound->vm->call_policy_adoption_hook_owned(receipt.actor,receipt.policy,hook_lease);receipt.call_returned=true;
     bound->check_active();hook_lease.require_operation(hook_serial);
     returned->require_outer_policy_hook_source(*bound->selected,*bound->state);
     if(!receipt.actual.errors.empty())throw ScriptError(receipt.actual.errors.front());
     if(bound->generation!=receipt.generation||bound->state->ui_source_revision!=receipt.revision||bound->vm.get()!=receipt.vm)
      throw std::runtime_error("PlayerAdoptPolicy actual VM/world source changed on return");
     bound->policy_adopt_hook_return=std::move(receipt);
     returned->accept_actual_outer_policy_hook_return(*canonical);
     hook_lease.require_operation(hook_serial);canonical->require_policy_adopt_hook_return(*returned);
    }catch(...){returned->fail_outer_policy_hook_dispatch();bound->policy_adopt_hook_return.reset();throw;}
   }
   while(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_yield_pending||(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending&&actual_ui)){
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_yield_pending)returned->advance_process_plot_yields();
    if(returned->phase()==BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending&&actual_ui)returned->consume_actual_policy_plot_ui();
   }
   // Preserve genuine native return history before callbackful final source
   // bookends: Policy44 has already released its private busy slot here.
   native_policy_returned=returned->phase()==BnwPolicyAdoptionPhase::adoption_returned&&returned->outer_policy_yield_returned();
   if(returned->phase()!=BnwPolicyAdoptionPhase::denied){
    returned->require_stored_current(*bound->selected);
    if(returned->phase()==BnwPolicyAdoptionPhase::adoption_returned){
     if(!returned->outer_policy_yield_returned())throw std::runtime_error("Policy completion lacks genuine final native yield scan return");
     bound->pending_policy_adoption.reset();
    }
    else bound->pending_policy_adoption=returned;
   }
   bound->check_active();lease.require_operation(serial);return 0;
  });
  return returned;
 }catch(...){
  // The final native map method can earn/release its busy slot and then throw
  // from its own source bookend before our normal latch is reached. Observe
  // only its closed original source/history seal; never invoke a stale provider.
  native_policy_returned=native_policy_returned||(returned&&returned->observed_earned_native_return_for(*canonical));
  if(returned&&actual_ui&&!native_policy_returned)returned->fail_free_policy_notification_dispatch();
  const auto original=std::current_exception();
  // No source/VM/currentness callback in this failure path. A copied issued
  // receipt keeps actual uncertain/earned native history after quarantine.
  if(returned)bound->retained_policy_adoption_receipt=returned;
  if(!bound->retained_error)bound->retained_error=original;
  bound->quarantine();
  if(returned){
   try{std::rethrow_exception(original);}
   catch(const std::exception& failure){throw BnwPolicyAdoptionSourceFailure(returned,failure.what());}
   catch(...){throw BnwPolicyAdoptionSourceFailure(returned,"Policy actual issued receipt retained after nonstandard source failure");}
  }
  std::rethrow_exception(original);
 }
}

std::shared_ptr<const BnwPolicyAdoptionPending>
BnwSelectedStockLuaSourceOwner::pending_same_policy_adoption()const {
 const auto thread=stock_thread_;thread->check();
 // Intentionally diagnostic after quarantine: authenticate immutable creator
 // and allocation control only, never invoke retired source providers.
 const auto canonical=action_self_.lock();
 if(!(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)))
  throw std::runtime_error("Policy retained receipt facade/control retired");
 const auto bound=canonical->bootstrap_;
 if(!(bound&&bound->thread==thread))throw std::runtime_error("Policy retained receipt creator/source allocation differs");
 return bound->retained_policy_adoption_receipt;
}

std::shared_ptr<const BnwResearchChoiceCommit>
BnwSelectedStockLuaSourceOwner::run_same_research_choice(const BnwResearchChoiceCommand& borrowed) {
    const auto thread=stock_thread_;thread->check();
    const auto command=borrowed;
    const auto canonical=retain_action_factory_owner();
    const auto bound=canonical->bootstrap_;
    if(!bound||!bound->vm)throw std::runtime_error("Research choice canonical stock VM absent");
    bound->check();
    std::shared_ptr<const BnwResearchChoiceCommit> returned;
    try {
        const auto result=bound->run_operation(BnwStockLuaOperationKind::research_choice,[bound,command,&returned] {
            bound->check_active();
            const auto serial=bound->operation_control->serial;
            const auto before=bound->operation_world_revision;
            const BnwStockLuaAttemptLease lease(bound->operation_control);
            const auto gui=bound->vm->retain_research_choice_gui_owned();
            bound->check_active();lease.require_operation(serial);
            if(command.player!=bound->selected->active_player_id()||
               !bound->state->setup.players.contains(command.player))
                throw std::runtime_error("Research action actual active player absent");
            const auto team=bound->state->setup.players.at(command.player).team;
            gui->require_same_selected_host(*bound->selected);
            bound->check_active();lease.require_operation(serial);
            gui->with_actual_research_choice_loan([&](const ui::BnwOwnedCameraGuiScreen::ResearchChoiceLoan& actual_ui) {
                actual_ui.require_ready_for_team(team); // Genuine producer before core stores.
                bound->check_active();lease.require_operation(serial);
                struct ActionBinding {
                    std::shared_ptr<StockBootstrap> bound;
                    BnwStockLuaAttemptLease lease;
                    std::uint64_t serial,before;
                    std::int64_t actor,team;
                    const ui::BnwOwnedCameraGuiScreen::ResearchChoiceLoan* ui=nullptr;
                    bool active=true,accepted=false;
                    ActionBinding(std::shared_ptr<StockBootstrap> b,BnwStockLuaAttemptLease l,
                        std::uint64_t s,std::uint64_t r,std::int64_t p,std::int64_t t,
                        const ui::BnwOwnedCameraGuiScreen::ResearchChoiceLoan& u)
                        :bound(std::move(b)),lease(std::move(l)),serial(s),before(r),actor(p),team(t),ui(&u) {}
                    void require_scope_identity()const {
                        bound->thread->check();
                        // This pure opening barrier comes before dereferencing
                        // the borrowed renderer Loan, including retained targets.
                        if(!active||!ui||bound->poisoned.load()||!bound->running||
                           bound->operation_state!=BnwStockLuaOperationState::active||
                           !bound->operation_control||bound->operation_control->invalidated.load()||
                           bound->operation_control->serial!=serial||bound->operation_control->vm!=bound->vm.get())
                            throw std::runtime_error("Research actual scoped action/renderer Loan ended");
                        bound->vm->require_vm_current_stamp();
                    }
                    void require_current()const {
                        require_scope_identity();bound->check_active();lease.require_operation(serial);
                        ui->require_current();
                        require_scope_identity();bound->check_active();lease.require_operation(serial);
                    }
                };
                const auto action=std::make_shared<ActionBinding>(bound,lease,serial,before,command.player,team,actual_ui);
                struct EndActionBinding {
                    std::shared_ptr<ActionBinding> actual;
                    ~EndActionBinding(){actual->active=false;actual->ui=nullptr;}
                } end{action}; // Cleared while the real renderer Loan is entered.
                action->require_current();
                auto hooks=BnwResearchChoiceHooks::issue(bound->selected,lease,
                    [action](BnwResearchChoiceEvent event,std::int64_t player,std::int64_t tech) {
                        action->require_current();
                        if(player!=action->actor)throw std::runtime_error("Research TestAll changed actual actor");
                        auto value=action->bound->vm->test_research_choice_owned(event,player,tech,action->lease);
                        action->require_current();return value;
                    },
                    [action](const BnwResearchChoiceCommit& commit) {
                        action->require_scope_identity();
                        if(action->accepted||action->bound->operation_world_revision!=action->before)
                            throw std::runtime_error("Research opaque commit already accepted/substituted");
                        action->accepted=true;
                        // Actual core stores have made old GUI currentness stale.
                        // Authenticate its opaque stored proof, not old frame.
                        action->bound->check(true);
                        commit.require_stored_current(*action->bound->selected);
                        const auto after=commit.after_revision();
                        if(commit.phase()!=BnwResearchChoicePhase::uncertain||
                           commit.player()!=action->actor||commit.team()!=action->team||
                           commit.before_revision()!=action->before||after<=action->before||
                           action->bound->state->ui_source_revision!=after)
                            throw std::runtime_error("Research opaque stored receipt changed source/revision");
                        action->require_scope_identity();action->bound->check(true);
                        if(action->bound->operation_world_revision!=action->before||
                           action->bound->state->ui_source_revision!=after)
                            throw std::runtime_error("Research actual source changed during commit proof");
                        action->bound->operation_world_revision=after;
                        action->bound->check_active();action->lease.require_operation(action->serial);
                        const auto next=action->ui->accept_commit(commit);
                        action->require_scope_identity();
                        action->bound->check_active();action->lease.require_operation(action->serial);
                        if(!next)throw std::runtime_error("Research renderer did not issue fresh composed GUI");
                        action->bound->vm->bind_gui_screen(next); // Existing private SAME-attempt binder.
                        action->require_current();
                    },
                    [action](BnwResearchChoiceDirty dirty,std::int64_t active_team) {
                        action->require_current();
                        if(!action->accepted||active_team!=action->team)
                            throw std::runtime_error("Research dirty changed actual committed team");
                        const auto next=action->ui->deliver_dirty(dirty,active_team);
                        action->require_scope_identity();
                        action->bound->check_active();action->lease.require_operation(action->serial);
                        if(!next)throw std::runtime_error("Research dirty handler did not issue fresh GUI");
                        action->bound->vm->bind_gui_screen(next);
                        action->require_current();
                    });
                action->require_current();
                returned=BnwResearchChoiceCommit::execute(*bound->selected->session_,std::move(hooks),command);
                // A failed late hook/dirty preserves the opaque native receipt
                // and the stored world. Do not replay, rollback, or claim ready.
                if(!returned||returned->phase()!=BnwResearchChoicePhase::complete||!action->accepted)
                    throw std::runtime_error("Research action returned actual uncertain core receipt");
                returned->require_stored_current(*bound->selected);action->require_current();
            });
            bound->check_active();lease.require_operation(serial);
            if(!returned)throw std::runtime_error("Research action omitted actual core receipt");
            return returned;
        });
        // The whole renderer Loan, including copied targets and their
        // destruction, AND the real operation tail have returned normally.
        // A weak receipt is enough for exact membership and cannot cycle via
        // Commit -> Hooks -> ActionBinding -> StockBootstrap.
        thread->check();bound->check();
        const auto actual_gui=bound->vm->retain_research_choice_gui_storage_without_callbacks();
        actual_gui->require_current();actual_gui->require_same_selected_host(*bound->selected);
        result->require_stored_current(*bound->selected);bound->check();
        if(bound->running||bound->operation_state!=BnwStockLuaOperationState::returned_suspended||
           !bound->operation_control||bound->operation_control->invalidated.load()||
           result.get()!=returned.get()||result.owner_before(returned)||returned.owner_before(result)||
           result->phase()!=BnwResearchChoicePhase::complete||
           result->after_revision()!=bound->state->ui_source_revision)
            throw std::runtime_error("Research actual normal-return operation/receipt retired before pending publication");
        bound->pending_research=result;
        bound->pending_research_serial=bound->operation_control->serial;
        bound->pending_research_revision=result->after_revision();
        return result;
    }catch(const std::exception& failure) {
        // run_operation has already quarantined its genuine VM/source. The
        // native commit's uncertain receipt remains observable by the caller.
        if(returned) {
            if(!bound->retained_error)bound->retained_error=std::current_exception();
            bound->quarantine();
            if(returned->phase()==BnwResearchChoicePhase::uncertain)return returned;
            throw BnwResearchChoiceSourceFailure(returned,failure.what());
        }
        throw;
    }catch(...) {
        if(returned) {
            if(!bound->retained_error)bound->retained_error=std::current_exception();
            bound->quarantine();
            if(returned->phase()==BnwResearchChoicePhase::uncertain)return returned;
            throw BnwResearchChoiceSourceFailure(returned,"Research actual core receipt retained after nonstandard source failure");
        }
        throw;
    }
}

// NEXT draft fields in StockBootstrap:
// weak_ptr<const BnwResearchChoiceCommit> pending_research;
// uint64_t pending_research_serial=0,pending_research_revision=0;
// Registration occurs only after whole actual GUI action borrower returns
// normally with a COMPLETE opaque core receipt. A strong backedge would cycle
// through receipt->hooks->ActionBinding->StockBootstrap.
// The operation entry must deny another VM action while a weak association
// exists (also if expired), except the exact research_refresh operation below.

void BnwSelectedStockLuaSourceOwner::complete_same_research_choice_after_refresh(
    std::shared_ptr<const BnwResearchChoiceCommit> incoming,
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> supplied_gui) {
    const auto thread=stock_thread_;thread->check();
    const auto canonical=retain_action_factory_owner();
    const auto bound=canonical->bootstrap_;
    if(!bound||!bound->vm||!incoming||!supplied_gui)
        throw std::runtime_error("Research final refresh canonical inputs absent");
    // Actual weak receipt control, not an address-only facade or a ready flag.
    const auto receipt=bound->pending_research.lock();
    if(!receipt||receipt.get()!=incoming.get()||receipt.owner_before(incoming)||incoming.owner_before(receipt)||
       !bound->operation_control||bound->pending_research_serial!=bound->operation_control->serial||
       bound->pending_research_revision!=receipt->after_revision())
        throw std::runtime_error("Research final refresh changed its actual pending receipt/operation");
    try {
    const auto old_gui=bound->vm->retain_research_choice_gui_storage_without_callbacks();
    bound->check();receipt->require_stored_current(*bound->selected);
    if(receipt->phase()!=BnwResearchChoicePhase::complete||
       bound->state->ui_source_revision!=bound->pending_research_revision)
        throw std::runtime_error("Research pending native stores retired before final refresh");
    const auto new_gui=supplied_gui->retain_owner();
    bound->check();
    if(!new_gui||new_gui.get()!=supplied_gui.get()||new_gui.owner_before(supplied_gui)||supplied_gui.owner_before(new_gui))
        throw std::runtime_error("Research post-dispatch GUI substituted canonical issuer control");
    new_gui->require_same_research_refresh_origin(*old_gui,*receipt);
    bound->check();receipt->require_stored_current(*bound->selected);
    bound->run_operation(BnwStockLuaOperationKind::research_refresh,[bound,receipt,old_gui,new_gui] {
        bound->check_active();
        const auto serial=bound->operation_control->serial;
        const BnwStockLuaAttemptLease lease(bound->operation_control);
        const auto pending=bound->pending_research.lock();
        if(!pending||pending.get()!=receipt.get()||pending.owner_before(receipt)||receipt.owner_before(pending)||
           bound->pending_research_serial==UINT64_MAX||serial!=bound->pending_research_serial+1||
           bound->pending_research_revision!=receipt->after_revision())
            throw std::runtime_error("Research refresh actual pending source/serial changed on entry");
        receipt->require_stored_current(*bound->selected);bound->check_active();lease.require_operation(serial);
        // Closed provenance proves the SAME Scene/compositor/Host and an actual
        // current Frame serial AND owner revision after the temporary action
        // publication. Deliberately do not ask the retired old GUI for currentness.
        new_gui->require_same_research_refresh_origin(*old_gui,*receipt);
        bound->check_active();lease.require_operation(serial);
        bound->vm->bind_gui_screen(new_gui);
        new_gui->require_current();receipt->require_stored_current(*bound->selected);
        bound->check_active();lease.require_operation(serial);
        return 0;
    });
    // Preserve the actual association through ALL operation-return bookends,
    // including journal publication. Failure quarantines it without replay.
    thread->check();
    new_gui->require_current();receipt->require_stored_current(*bound->selected);
    bound->check();bound->vm->require_vm_current_stamp();thread->check();
    if(bound->running||bound->operation_state!=BnwStockLuaOperationState::returned_suspended||
       !bound->operation_control||bound->operation_control->invalidated.load()||
       bound->poisoned.load()||bound->pending_research_serial==UINT64_MAX||
       bound->operation_control->serial!=bound->pending_research_serial+1)
        throw std::runtime_error("Research refresh operation did not return normally");
    const auto final_pending=bound->pending_research.lock();
    if(!final_pending||final_pending.get()!=receipt.get()||final_pending.owner_before(receipt)||
       receipt.owner_before(final_pending))
        throw std::runtime_error("Research pending control changed after true refresh return");
    bound->pending_research.reset();
    bound->pending_research_serial=0;bound->pending_research_revision=0;
    }catch(const std::exception& failure) {
        if(!bound->retained_error)bound->retained_error=std::current_exception();
        bound->quarantine();
        throw BnwResearchChoiceSourceFailure(receipt,failure.what());
    }catch(...) {
        if(!bound->retained_error)bound->retained_error=std::current_exception();
        bound->quarantine();
        throw BnwResearchChoiceSourceFailure(receipt,"Research actual stored receipt retained after nonstandard final-refresh failure");
    }
}

std::shared_ptr<const BnwResearchChoiceCommit>
BnwSelectedStockLuaSourceOwner::pending_same_research_choice()const {
    const auto thread=stock_thread_;thread->check();
    const auto canonical=retain_action_factory_owner();
    const auto bound=canonical->bootstrap_;
    if(!bound||!bound->vm||!bound->operation_control||bound->poisoned.load()||
       bound->operation_control->invalidated.load()||bound->running||
       bound->operation_state==BnwStockLuaOperationState::uncertain||
       bound->operation_state==BnwStockLuaOperationState::retired)
        throw std::runtime_error("Research pending observation source retired/active");
    const std::weak_ptr<const BnwResearchChoiceCommit> empty;
    if(!bound->pending_research.owner_before(empty)&&!empty.owner_before(bound->pending_research))
        return {};
    // Callback-free weak identity observation only. No VM advance, GUI read,
    // native query or new authority from scalar revision fields.
    const auto pending=bound->pending_research.lock();
    if(!pending||bound->pending_research_serial!=bound->operation_control->serial||
       bound->pending_research_revision!=pending->after_revision()||
       pending->phase()!=BnwResearchChoicePhase::complete)
        throw std::runtime_error("Research pending receipt expired/substituted; no retry");
    thread->check();return pending;
}


BnwCityProductionTurnProgress BnwSelectedStockLuaSourceOwner::run_same_production_city_screen(
    ui::BnwCityProductionUiAdapter& adapter,std::shared_ptr<const BnwCityProductionTurnPending> pending){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Production CityScreen stock source absent");
    bound->check(); // Refuses reentry BEFORE the native adapter can execute.
    const auto key=adapter.require_same_stock_pending(bound->selected.get(),pending);
    try{return bound->run_operation(BnwStockLuaOperationKind::production_city_screen,[&]{
        bound->check_active();const auto serial=bound->operation_control->serial;
        const auto revision=bound->state->ui_source_revision,occurrence=bound->state->city_dirty_next_occurrence;
        const auto queue=bound->state->city_dirty_events;const auto before=pending->progress();
        if(before.phase!=BnwCityProductionTurnPhase::city_screen_predicate_pending&&before.phase!=BnwCityProductionTurnPhase::city_screen_dirty_pending)
            throw std::runtime_error("Production CityScreen requires actual pre-banner native phase");
        try{
            const auto result=adapter.continue_city(pending);
            // Metadata/source/DB guard, not a general revision rebaseline.
            // The SAME adapter already validated its exact registered writer.
            bound->check(true);
            if(bound->operation_control->serial!=serial||bound->operation_control->vm!=bound->vm.get())
                throw std::runtime_error("Production CityScreen substituted stock operation/VM");
            if(adapter.require_same_stock_pending(bound->selected.get(),pending)!=key)
                throw std::runtime_error("Production CityScreen changed actual registered source key");
            ui::detail::require_production_city_screen_delta(before,result,revision,bound->state->ui_source_revision,
                occurrence,bound->state->city_dirty_next_occurrence,bound->generation,queue,bound->state->city_dirty_events);
            // Only a fully checked native0/1 CityScreen producer can update
            // this private loan revision. Existing read-only lease stays exact.
            bound->operation_world_revision=bound->state->ui_source_revision;
            bound->check_active();return result;
        }catch(...){adapter.fail_stock_pending(pending);throw;}
    });}catch(...){
        // Also covers run_operation's post-call VM health/retained journal
        // failure, which occurs after the private native lambda has returned.
        adapter.fail_stock_pending(pending);throw;
    }
}

BnwUnitMovePrefix BnwSelectedStockLuaSourceOwner::run_same_unit_dim(
    std::shared_ptr<const BnwUnitMovePending> pending){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Dim actual stock source absent");
    bound->check(); // Reentry refused before any pending/receiver marker.
    if(!pending)throw std::runtime_error("Dim genuine registered Pending absent");
    pending->require_same_registered_pending(bound->selected.get());
    try{return bound->run_operation(BnwStockLuaOperationKind::unit_dim,[&]{
        bound->check_active();
        if(!bound->vm)throw std::runtime_error("Dim actual SAME stock VM absent");
        const auto serial=bound->operation_control->serial;
        const BnwStockLuaAttemptLease lease(bound->operation_control);
        pending->begin_stock_dim(bound->selected.get(),lease);
        const auto before=pending->stock_dim_operands();
        // GRAPHICS supplies this genuine synchronous receiver. No empty
        // listener success, public completion bool or replacement VM here.
        bound->vm->dispatch_same_unit_dim_owned(*pending,lease);
        bound->check_active();lease.require_operation(serial);
        bound->vm->require_healthy_owned();
        const auto after=pending->stock_dim_operands();
        if(before.owner!=after.owner||before.full_id!=after.full_id||before.moves!=after.moves||
           before.occurrence!=after.occurrence||before.generation!=after.generation||
           before.world_revision!=after.world_revision)
            throw std::runtime_error("Dim callback changed exact canonical operands/lineage");
        pending->accept_stock_dim_return(bound->selected.get());
        // Native setMoves order: actual Dim return THEN IsSelected.
        pending->continue_post_dim_selection(bound->selected.get());
        bound->check_active();lease.require_operation(serial);return pending->prefix();
    });}catch(...){pending->fail_stock_dim();throw;}
}

bool BnwSelectedStockLuaSourceOwner::attach_stock_client_extent(BnwStockLuaClientExtentProvider provider){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Selected stock VM not started");
    bound->check();if(!provider.current)return false;
    const auto callback=std::make_shared<const decltype(provider.current)>(std::move(provider.current));
    std::weak_ptr<StockBootstrap> weak=bound;
    BnwStockLuaClientExtentProvider guarded{[weak,callback]{
        const auto current=weak.lock();if(!current)throw std::runtime_error("Selected stock VM callback owner expired");
        try {
            current->check_active();
            const auto serial=current->operation_control->serial;
            const auto pinned=callback; // Retain the actual callable across self-retirement.
            const auto extent=(*pinned)();
            current->check_active();
            if(current->operation_control->serial!=serial)throw std::runtime_error("Stock callback operation substituted");
            return extent;
         }catch(...){
            current->poisoned.store(true);current->operation_control->invalidated.store(true);
            if(std::this_thread::get_id()==current->thread->thread){
                if(!current->retained_error)current->retained_error=std::current_exception();
                current->quarantine();
            }
            throw;
        }
    }};
    return bound->run_operation(BnwStockLuaOperationKind::attach_client,[&]{
        if(!bound->vm)throw std::runtime_error("Stock VM absent; no client retry");
        return bound->vm->attach_client_extent(std::move(guarded));
    });
}
bool BnwSelectedStockLuaSourceOwner::attach_stock_native_client(std::uintptr_t actual_hwnd,BnwStockLuaClientExtentProvider provider){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Selected stock VM not started");
    bound->check();if(!actual_hwnd||!provider.current)return false;
    const auto callback=std::make_shared<const decltype(provider.current)>(std::move(provider.current));
    std::weak_ptr<StockBootstrap> weak=bound;
    BnwStockLuaClientExtentProvider guarded{[weak,callback]{
        const auto current=weak.lock();if(!current)throw std::runtime_error("Selected stock native callback owner expired");
        try{
            current->check_active();
            const auto serial=current->operation_control->serial;
            const auto pinned=callback;const auto extent=(*pinned)();
            current->check_active();
            if(current->operation_control->serial!=serial)throw std::runtime_error("Stock callback operation substituted");
            return extent;
         }catch(...){
            current->poisoned.store(true);current->operation_control->invalidated.store(true);
            if(std::this_thread::get_id()==current->thread->thread){
                if(!current->retained_error)current->retained_error=std::current_exception();
                current->quarantine();
            }
            throw;
        }
    }};
    return bound->run_operation(BnwStockLuaOperationKind::attach_native,[&]{
        if(!bound->vm)throw std::runtime_error("Stock VM absent; no native retry");
        return bound->vm->attach_native_client(actual_hwnd,std::move(guarded));
    });
}
void BnwSelectedStockLuaSourceOwner::attach_stock_renderer_window_owner(
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> renderer,
 BnwStockLuaClientExtentProvider provider){
 const auto thread=stock_thread_;thread->check();
 const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Selected stock VM not started");
 bound->check();
 if(!renderer||!provider.current)throw std::runtime_error("Actual typed renderer/provider missing");
 const auto callback=std::make_shared<const decltype(provider.current)>(std::move(provider.current));
 std::weak_ptr<StockBootstrap> weak=bound;
 BnwStockLuaClientExtentProvider guarded{[weak,callback]{
  const auto current=weak.lock();if(!current)throw std::runtime_error("Typed renderer stock source owner expired");
  try{
   current->check_active();const auto serial=current->operation_control->serial;
   const auto pinned=callback;const auto extent=(*pinned)();
   current->check_active();
   if(current->operation_control->serial!=serial)throw std::runtime_error("Typed renderer stock operation substituted");
   return extent;
  }catch(...){
   current->poisoned.store(true);current->operation_control->invalidated.store(true);
   if(std::this_thread::get_id()==current->thread->thread){
    if(!current->retained_error)current->retained_error=std::current_exception();
    current->quarantine();
   }
   throw;
  }
 }};
 bound->run_operation(BnwStockLuaOperationKind::attach_renderer_window,[&]{
  if(!bound->vm)throw std::runtime_error("Typed renderer SAME stock VM absent");
  bound->vm->attach_renderer_window_owner(std::move(renderer),std::move(guarded));
  bound->check_active();return 0;
 });
}
void BnwSelectedStockLuaSourceOwner::attach_stock_gui_screen(
 std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> incoming){
 const auto thread=stock_thread_;thread->check();
 const auto bound=bootstrap_;if(!bound||!incoming)throw std::runtime_error("GUI SAME stock owner/source absent");
 bound->check();
 bound->run_operation(BnwStockLuaOperationKind::attach_gui_screen,[&]{
  bound->check_active();if(!bound->vm)throw std::runtime_error("GUI SAME VM absent");
  const auto canonical=incoming->retain_owner();bound->check_active();
  if(canonical.get()!=incoming.get()||canonical.owner_before(incoming)||incoming.owner_before(canonical))
   throw std::runtime_error("GUI source supplied control is not the actual compositor owner");
  canonical->require_same_selected_host(*bound->selected);bound->check_active();
  bound->vm->bind_gui_screen(canonical);bound->check_active();
  // Finish the genuine SAME Scene UI/Engine attachment while this GUI
  // operation is still active. The original Source journal publishes its
  // returned record only after this body and its retained scopes return.
  const BnwStockLuaAttemptLease attachment(bound->operation_control);
  const auto serial=attachment.operation_serial();
  const auto actual_ui=canonical->retain_policy_action_ui();
  bound->check_active();attachment.require_operation(serial);
  const auto engine=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
  bound->check_active();attachment.require_operation(serial);
  // The canonical helper reuses the actual Engine slot; only an empty slot
  // can be bound. It never creates a replacement Selection or resets state.
  const auto selection=actual_ui->city_selection_control_;
  if(!selection)throw std::runtime_error("GUI attachment actual Selection absent");
  const auto selected=selection->retained_host_for_tech_transport();
  bound->check_active();attachment.require_operation(serial);
  if(selected.get()!=bound->selected.get()||selected.owner_before(bound->selected)||bound->selected.owner_before(selected))
   throw std::runtime_error("GUI attachment Selection belongs to different selected Host");
  engine->require_current();bound->check_active();attachment.require_operation(serial);
  canonical->require_same_selected_host(*bound->selected);
  bound->check_active();attachment.require_operation(serial);
  // Final callback-free checks use the saved owners after all full bookends.
  const auto vm_gui=bound->vm->retain_research_choice_gui_storage_without_callbacks();
  const auto ui_control=actual_ui->selected_owner_lifetime_.lock();
  const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
  if(!same(vm_gui,canonical)||!same(ui_control,actual_ui)||
     !same(actual_ui->engine_ui_turn_owner_,engine)||!same(actual_ui->city_selection_control_,selection))
   throw std::runtime_error("GUI attachment canonical VM/UI/Engine/Selection control changed");
  attachment.require_operation(serial);return 0;
 });
}
void BnwSelectedStockLuaSourceOwner::attach_stock_completed_color_query(
 std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery> input){
 const auto thread=stock_thread_;thread->check();
 const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Color SAME stock owner absent");
 bound->check();
 // Existing private actual Host adoption, not a caller Parent/control.
 // Root-owned canonical Parent adoption hardening is a required dependency.
 const auto parent=bound->selected->early_package_;
 if(!parent||!input)throw std::runtime_error("Color source needs actual privately adopted Early Parent");
 bound->run_operation(BnwStockLuaOperationKind::attach_completed_color,[&]{
  bound->check_active();if(!bound->vm)throw std::runtime_error("Color SAME VM absent");
  // Canonical leaf is obtained BEFORE callbackful Parent/client currentness;
  // the physical preflight invokes no external extent-provider callbacks.
  auto canonical=bound->vm->retain_completed_color_query(std::move(input));
  bound->check_active();bound->vm->require_owned_physical_window();
  canonical->require_same_parent(*parent);
  bound->check_active();bound->vm->require_owned_physical_window();
  bound->selected->require_same_early_package(*parent);
  bound->check_active();bound->vm->require_owned_physical_window();
  canonical->require_same_parent(*parent);
  bound->check_active();bound->vm->require_owned_physical_window();
  bound->vm->bind_completed_color_query(std::move(canonical),parent);
  bound->check_active();return 0;
 });
}
std::optional<ui::BnwCivilopediaNativeRuntimeFrame>BnwSelectedStockLuaSourceOwner::stock_native_runtime_frame()const{
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)return {};
    bound->check();if(!bound->vm)return {};
    return bound->run_operation(BnwStockLuaOperationKind::read_native,[&]{return bound->vm->native_runtime_frame();});
}
std::weak_ptr<const void>BnwSelectedStockLuaSourceOwner::stock_native_window_storage()const{
 const auto thread=stock_thread_;thread->check();
 const auto bound=bootstrap_;if(!bound)return {};
 bound->check();if(!bound->vm)return {};
 return bound->run_operation(BnwStockLuaOperationKind::read_native,[&]{return bound->vm->native_window_storage();});
}
BnwStockLuaBootstrapProgress BnwSelectedStockLuaSourceOwner::advance_stock_bootstrap(){
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)throw std::runtime_error("Selected stock VM not started");
    return bound->advance();
}
const BnwStockLuaBootstrapProgress* BnwSelectedStockLuaSourceOwner::stock_bootstrap_progress()const {
    const auto thread=stock_thread_;thread->check();
    const auto bound=bootstrap_;if(!bound)return nullptr;
    bound->check();return bound->vm?&bound->vm->progress():nullptr;
}

BnwStockLuaOperationEvidence BnwSelectedStockLuaSourceOwner::stock_operation_evidence()const {
    const auto thread=stock_thread_;
    if(std::this_thread::get_id()!=thread->thread){
        thread->poisoned.store(true);throw std::runtime_error("Stock journal foreign thread");
    }
    BnwStockLuaOperationEvidence result;
    const auto bound=bootstrap_;if(!bound)return result;
    result.state=bound->operation_state;result.operations=bound->journal;
    if(bound->poisoned.load()||thread->poisoned.load())result.state=BnwStockLuaOperationState::uncertain;
    else if(thread->retired.load())result.state=BnwStockLuaOperationState::retired;
    if(bound->retained_error){
        try{std::rethrow_exception(bound->retained_error);}
        catch(const std::exception& e){result.error=e.what();}
        catch(...){result.error="Non-standard retained stock operation failure";}
    }
    return result;
}

BnwGameplayLuaForkResult BnwSelectedStockLuaSourceOwner::fork_at(
    const LiveSessionState &state) {
    const auto thread=stock_thread_;
    try{thread->check();}catch(const std::exception& error){return {{},error.what()};}
    const auto bound=bootstrap_;
    if(bound){
        try{bound->check();}catch(const std::exception& error){return {{},error.what()};}
    }
    // This owner serves all gameplay hooks. TeamTech-specific ordering is
    // validated by quote_bnw_team_tech_researched at its actual caller boundary.
    // No complete VM is available here yet, so no world is read or forked.
    (void)state;
    if(bound&&bound->vm)return {{},bound->vm->progress().blocker};
    return {{},evidence_.blocker};
}

BnwGameplayLuaPublishResult BnwSelectedStockLuaSourceOwner::publish(
    std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) {
    return {false,false,
        "Selected stock Lua owner has no complete context epoch or atomic VM/world publication"};
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected) const {
    return probe_parent_chunk_impl(selected,nullptr,nullptr);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,
    const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured) const {
    return probe_parent_chunk_impl(selected,&resources,&measured);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,
    const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,
    ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements) const {
    return probe_parent_chunk_impl(selected,&resources,&measured,
                                   std::move(renderer_measurements));
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,
    const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,
    ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
    std::int64_t civis_left_click_event) const {
    return probe_parent_chunk_impl(selected,&resources,&measured,
        std::move(renderer_measurements),civis_left_click_event);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,
    ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
    std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale)const{
    if(!locale)throw ScriptError("Civilopedia requested locale provider is absent");
    return probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer_measurements),
        civis_left_click_event,std::move(locale));
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,
    ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaInputCapability input)const{
    if(!locale||input!=BnwCivilopediaInputCapability::captured_civis_input)
        throw ScriptError("Civilopedia captured input capability is absent or unsupported");
    return probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer_measurements),event,std::move(locale),true);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaTooltipCapability tooltip,
    const assets::ResourceMountTable *textures)const{
    if(!locale||tooltip!=BnwCivilopediaTooltipCapability::source_declared_state)
        throw ScriptError("Civilopedia tooltip capability is absent or unsupported");
    return probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),true,true,textures);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaLocaleTextCapability operations,
    const assets::ResourceMountTable *textures)const{
    if(!locale||operations!=BnwCivilopediaLocaleTextCapability::civis_windows_nls)
        throw ScriptError("Civilopedia locale text capability is absent or unsupported");
    return probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),true,true,textures,true);
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaTableCountCapability count,
    const assets::ResourceMountTable *textures)const{
    if(!locale||count!=BnwCivilopediaTableCountCapability::civis_raw_immediate_keys)
        throw ScriptError("Civilopedia raw table count capability is absent or unsupported");
    return probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),true,true,textures,true,true);
}

BnwCivilopediaGameOptionProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaGameOptionCapability option,
    const assets::ResourceMountTable *textures)const{
    if(!locale||option!=BnwCivilopediaGameOptionCapability::civis_selected_no_religion)
        throw ScriptError("Civilopedia selected Game option capability is absent or unsupported");
    BnwCivilopediaGameOptionProbe result;
    result.parent=probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),
        true,true,textures,true,true,true,&result.option);
    return result;
}

BnwCivilopediaUnitsQueryProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaUnitsQueryCapability query,
    const assets::ResourceMountTable *textures)const{
    if(!locale||query!=BnwCivilopediaUnitsQueryCapability::civis_first_units_statement)
        throw ScriptError("Civilopedia diagnostic first Units query capability is absent or unsupported");
    BnwCivilopediaUnitsQueryProbe result;
    result.parent=probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),
        true,true,textures,true,true,true,&result.option,true,&result.query);
    return result;
}

BnwCivilopediaUnitsQueryProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk(
    const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &resources,
    const ui::CivilopediaMeasuredLayout &measured,ui::CivisCivilopediaScrollMeasurementProvider renderer,
    std::int64_t event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,BnwCivilopediaBuildingsQueryCapability query,
    const assets::ResourceMountTable *textures)const{
    if(!locale||query!=BnwCivilopediaBuildingsQueryCapability::civis_units_and_buildings_statements||
       !locale->buildings_description_first_form_enabled())
        throw ScriptError("Civilopedia Buildings query requires its explicit selected Buildings Description capability");
    BnwCivilopediaUnitsQueryProbe result;
    result.parent=probe_parent_chunk_impl(selected,&resources,&measured,std::move(renderer),event,std::move(locale),
        true,true,textures,true,true,true,&result.option,true,&result.query,true);
    return result;
}

BnwCivilopediaParentProbe BnwSelectedStockLuaSourceOwner::probe_parent_chunk_impl(
    const ui::SelectedBnwPreviewHost &selected,
    const assets::ResourceMountTable *resources,
    const ui::CivilopediaMeasuredLayout *measured,
    ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
    std::optional<std::int64_t> civis_left_click_event,
    std::shared_ptr<ui::SelectedCivilopediaLocale> locale,bool captured_input,bool tooltip_state,
    const assets::ResourceMountTable *texture_resources,bool locale_text_operations,bool raw_table_count,
    bool selected_game_option,std::optional<ui::CivisSelectedGameOptionResolution> *option_observation,
    bool first_units_query,ScriptCivisUnitsQueryObservations *query_observation,bool buildings_query) const {
    BnwCivilopediaParentProbe result;
    try {
        if(!evidence_.selected_profile || !evidence_.first_include_executed ||
           selected.identity().payload_sha256!=identity_.payload_sha256 ||
           selected.identity().rules_sha256!=identity_.rules_sha256)
            throw std::runtime_error("Selected Civilopedia parent owner identity unavailable");
        const auto prefix=ui::execute_selected_bnw_manager_block_binding(
            game_root_,catalog_,selected_,selected);
        if(prefix.status!=ui::SelectedBnwCivilopediaPreambleStatus::manager_block_bound ||
           prefix.bound_manager_count!=33 ||
           prefix.next_native_control!="UIManager.GetScreenSizeVal")
            throw std::runtime_error("Selected Civilopedia manager prefix unavailable: "+
                                     prefix.blocker);
        const auto root=std::filesystem::canonical(game_root_);
        const auto pedia=root/"Assets/DLC/Expansion2/UI/Civilopedia";
        std::string parent_sha;
        const auto parent=verified_source(pedia/"CivilopediaScreen.lua",&parent_sha);
        if(parent_sha!=evidence_.first_source_sha256 ||
           parent_sha!=prefix.context_source_sha256)
            throw std::runtime_error("Selected Civilopedia parent source changed");
        const auto layout=content::read_xml(pedia/"CivilopediaScreen.xml");
        const auto instance=verified_source(root/"Assets/UI/InstanceManager.lua");
        const auto managers=ui::inspect_civilopedia_manager_block(
            parent,layout,instance);
        if(!managers.verified || managers.bindings.size()!=33)
            throw std::runtime_error("Selected Civilopedia manager control contract changed: "+
                                     managers.blocker);
        const auto icon=verified_source(root/"Assets/UI/IconSupport.lua");
        const auto vector=verified_source(root/"Assets/Gameplay/Lua/FLuaVector.lua");
        std::unique_ptr<ui::CivisMeasuredCivilopediaControlTree> control_tree;
        std::optional<ui::SelectedBnwCivilopediaLayoutPreflight> layout_plan;
        std::optional<ui::SelectedBnwCivilopediaLayoutSources> layout_sources;
        ScriptUiLayoutProvider layout_provider;
        std::shared_ptr<ui::CivisCivilopediaCallbackOwner> callback_owner;
        std::shared_ptr<ui::CivisCivilopediaInputOwner> input_owner;
        std::shared_ptr<ui::CivisCivilopediaTooltipOwner> tooltip_owner;
        std::shared_ptr<ui::CivisLocaleTextOwner> text_owner;
        ScriptEnumProvider ui_enums;
        std::vector<std::string> category_ids;
        if(resources || measured) {
            if(!resources || !measured)
                throw std::runtime_error("Selected Civilopedia layout needs both VFS and measured frame");
            layout_sources=ui::inspect_bnw_civilopedia_layout_sources(root);
            layout_plan=ui::preflight_selected_bnw_civilopedia_layout(
                root,*resources,catalog_,selected_,selected,
                measured->frame_number,*measured);
            if(!layout_plan->source_and_owner_verified ||
               layout_plan->instructions.size()!=11)
                throw std::runtime_error("Selected Civilopedia layout preflight blocked: "+
                                         layout_plan->blocker);
            std::string tree_blocker;
            control_tree=ui::CivisMeasuredCivilopediaControlTree::open(
                *resources,*layout_sources,*layout_plan,*measured,
                ui::CivisUiExecutionScope::diagnostic,tree_blocker);
            if(!control_tree)
                throw std::runtime_error("Selected Civilopedia control tree blocked: "+tree_blocker);
            result.viewport_width=measured->viewport_width;
            result.viewport_height=measured->viewport_height;
            layout_provider=ui::bind_civis_measured_civilopedia_layout(
                *control_tree,*layout_sources,*measured,
                std::move(renderer_measurements));
            std::set<std::string> bound_ids;
            for(const auto &control:layout_provider.controls)
                bound_ids.insert(control.id);
            for(const auto &binding:managers.bindings)
                if(bound_ids.insert(binding.parent_control).second)
                    layout_provider.controls.push_back({binding.parent_control,
                                                        binding.parent_kind,false});
            if(civis_left_click_event){
                callback_owner=std::make_shared<ui::CivisCivilopediaCallbackOwner>(layout_plan->owner);
                category_ids=ui::bind_civis_civilopedia_category_callbacks(
                    layout_provider,root,*layout_sources,layout_plan->owner,callback_owner,*civis_left_click_event);
                result.category_callback_controls=category_ids.size();
                // Explicit Civis pointer mapping, not original numeric ABI.
                ui_enums.tables={{"Mouse",{{"eLClick",*civis_left_click_event}}}};
                if(locale){
                    if(!locale->current(layout_plan->owner))throw ScriptError("Civilopedia selected locale owner is stale or different");
                    result.top_level_callback_controls=ui::bind_civis_civilopedia_top_level_button_callbacks(
                        layout_provider,root,*layout_sources,layout_plan->owner,callback_owner,*civis_left_click_event).size();
                    if(captured_input)input_owner=ui::CivisCivilopediaInputOwner::open(root,*layout_sources,layout_plan->owner,callback_owner,true);
                    if(tooltip_state)tooltip_owner=ui::CivisCivilopediaTooltipOwner::open(root,*layout_sources,layout_plan->owner,callback_owner,texture_resources);
                    if(locale_text_operations)text_owner=ui::CivisLocaleTextOwner::open(locale,layout_plan->owner,locale->provider().current().type);
                }
            }
        }
        if(civis_left_click_event&&!control_tree)
            throw std::runtime_error("Civilopedia category callback capability requires a measured context owner");
        auto database=selected.database().clone_gameplay();
        auto types=database->cache_info_types();
        if(!types.report().all_succeeded())
            throw std::runtime_error("Selected Civilopedia GameInfo catalog incomplete");
        auto detached_world=selected.session().state();
        std::shared_ptr<ui::CivisSelectedGameOptionOwner> option_owner;
        if(selected_game_option){
            if(!layout_plan||!callback_owner||!detached_world.world.pregame)
                throw ScriptError("Civilopedia selected Game option lacks its same-context detached Pregame owner");
            option_owner=ui::CivisSelectedGameOptionOwner::open(*detached_world.world.pregame,*database,layout_plan->owner,callback_owner);
            saves::PregameObservation owned_setup;owned_setup.layout=detached_world.world.pregame->layout;
            owned_setup.game_options=detached_world.setup.game_options;
            const auto setup_option=ui::CivisSelectedGameOptionOwner::open(owned_setup,*database,layout_plan->owner,callback_owner);
            if(setup_option->resolution().value!=option_owner->resolution().value)
                throw ScriptError("Civilopedia selected Game option differs between raw Pregame and owned setup semantics");
            if(option_observation)*option_observation=option_owner->resolution();
        }
        std::shared_ptr<ui::CivisCivilopediaUnitsQueryOwner> query_owner;
        if(first_units_query){
            if(!layout_plan||!layout_sources||!callback_owner||!option_owner)
                throw ScriptError("Civilopedia first Units query lacks its same-context selected service owners");
            query_owner=buildings_query?
                ui::CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,*layout_sources,*database,layout_plan->owner,callback_owner):
                ui::CivisCivilopediaUnitsQueryOwner::open(root,*layout_sources,*database,layout_plan->owner,callback_owner);
        }
        ScriptSourceProvider sources;
        sources.files=[](std::string_view) {
            return std::vector<std::string>{
                "Assets/UI/InstanceManager.lua", "Assets/UI/IconSupport.lua",
                "Assets/Gameplay/Lua/FLuaVector.lua"};
        };
        sources.read=[&](std::string_view,std::string_view path) -> std::string {
            result.observed_include_order.emplace_back(path);
            if(path=="Assets/UI/InstanceManager.lua") return instance;
            if(path=="Assets/UI/IconSupport.lua") return icon;
            if(path=="Assets/Gameplay/Lua/FLuaVector.lua") return vector;
            throw ScriptError("Unexpected selected Civilopedia include");
        };
        ScriptUiEventProvider events;
        events.notifications={"AfterModsDeactivate","AfterModsActivate",
            "SearchForPediaEntry","GoToPediaHomePage","GameplaySetActivePlayer",
            "MultiplayerGameLobbyInvite","MultiplayerGameServerInvite"};
        LuaScriptHost host({64ULL*1024*1024,2'000'000},std::move(sources),{},
            lua_gameplay_services(*database,detached_world.setup,
                                  detached_world.technologies,types),
            {},{},{},std::move(events),{},{},std::move(ui_enums),locale?locale->provider():ScriptLocaleProvider{},std::move(layout_provider));
        if(locale)host.enable_plain_locale_lookup("CivilopediaScreen",locale->plain_lookup_tags());
        if(input_owner){host.enable_ui_text_entry(input_owner->text_provider());host.enable_ui_context_handlers(input_owner->context_provider());}
        if(tooltip_owner)host.enable_ui_tooltip(tooltip_owner->provider());
        if(text_owner)host.enable_locale_text_operations("CivilopediaScreen",text_owner->provider());
        if(raw_table_count)host.enable_civis_raw_table_count("CivilopediaScreen");
        if(option_owner)host.enable_civis_selected_game_option("CivilopediaScreen",option_owner->provider());
        if(query_owner){
            const std::weak_ptr<ui::CivisCivilopediaUnitsQueryOwner> weak=query_owner;
            host.enable_civis_units_query("CivilopediaScreen",{[weak](std::string_view sql){
                const auto owner=weak.lock();if(!owner)throw ScriptError("Civilopedia first Units query owner expired");
                return owner->prepare(sql);
            }});
        }
        std::string controls="Controls={";
        std::set<std::string> seen;
        for(const auto &binding:managers.bindings)
            if(seen.insert(binding.parent_control).second)
                controls+=binding.parent_control+"={id='"+
                    binding.parent_control+"',kind='"+binding.parent_kind+"'},";
        controls+='}';
        if(!control_tree)
            host.execute("CivilopediaScreen",controls,
                         "=source-verified-civilopedia-manager-controls");
        result.selected_source_verified=true;
        result.manager_bindings=managers.bindings.size();
        try {
            host.execute("CivilopediaScreen",parent,
                "@Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
            result.full_parent_loaded=true;
        } catch(const ScriptError &error) {
            result.blocker=error.what();
        }
        if(query_observation)*query_observation=host.civis_units_query_observations("CivilopediaScreen");
        result.registered_category_callbacks=host.registered_ui_control_callbacks("CivilopediaScreen",category_ids);
        result.registered_control_callbacks=host.registered_ui_control_callbacks("CivilopediaScreen");
        const std::string text_ids[]={"SearchEditBox"};
        result.registered_text_callbacks=host.registered_ui_control_callbacks("CivilopediaScreen",text_ids);
        result.registered_context_handlers=host.registered_ui_context_handlers("CivilopediaScreen");
        if(tooltip_owner){
            result.tooltip_type_registrations=tooltip_owner->registrations();result.tooltip_control_tables=tooltip_owner->snapshot().size();
            result.tooltip_render_receipt_verified=tooltip_owner->render_receipt_verified();
        }
        if(locale)result.resolved_locale_labels=locale->observations();
        if(control_tree)result.layout_operations_completed=
            control_tree->completed_operations();
        if(control_tree && result.layout_operations_completed>=9) {
            const auto &owner=layout_plan->owner;
            const auto scrollbar=control_tree->get_geometry(owner,"ScrollBar");
            const auto panel=control_tree->get_geometry(owner,"ScrollPanel");
            const auto down=control_tree->get_geometry(owner,"DownButton");
            if(!scrollbar.available || !panel.available || !down.available)
                throw std::runtime_error("Selected Civilopedia geometry disappeared after Lua setters");
            result.scrollbar_width=scrollbar.geometry.width;
            result.scrollbar_height=scrollbar.geometry.height;
            result.scroll_panel_height=panel.geometry.height;
            result.down_button_offset_y=down.geometry.offset_y;
            const auto subtree=ui::inspect_bnw_civilopedia_scroll_subtree(
                root,*resources,*layout_sources,control_tree->geometry_prefix(),*measured);
            if(!subtree.source_and_owner_verified)
                throw std::runtime_error("Civilopedia selected scroll subtree blocked: "+
                                         subtree.blocker);
            result.scroll_subtree_verified=true;
            result.scroll_nodes=subtree.nodes.size();
            result.content_roots=subtree.content_roots.size();
            result.explicit_hidden=subtree.explicit_hidden;
        }
        if(result.observed_include_order!=std::vector<std::string>{
              "Assets/UI/InstanceManager.lua","Assets/UI/IconSupport.lua",
              "Assets/Gameplay/Lua/FLuaVector.lua"} ||
           !host.source_errors().empty())
            throw std::runtime_error("Selected Civilopedia include graph did not execute exactly");
        // Observe retained listener registries without invoking any callback.
        // Source execution with a requested locale can reach additional Add
        // calls; a failed diagnostic VM still cannot be published to gameplay.
        host.execute("CivilopediaScreen",
            "print('__CIVIS_PARENT_LISTENER_COUNTS',Events.AfterModsDeactivate.Count(),"
            "Events.AfterModsActivate.Count(),Events.SearchForPediaEntry.Count(),"
            "Events.GoToPediaHomePage.Count(),Events.GameplaySetActivePlayer.Count(),"
            "Events.MultiplayerGameLobbyInvite.Count(),Events.MultiplayerGameServerInvite.Count())",
            "=Civilopedia-parent-listener-boundary");
        if(host.messages().empty())throw ScriptError("Civilopedia listener count observation is absent");
        std::istringstream count_message(host.messages().back().text);std::string field;
        count_message>>field;if(field!="__CIVIS_PARENT_LISTENER_COUNTS")throw ScriptError("Civilopedia listener count observation changed");
        std::array<std::size_t,7> counts{};
        for(auto &value:counts){
            if(!(count_message>>field))throw ScriptError("Civilopedia listener count observation is incomplete");
            const auto parsed=std::from_chars(field.data(),field.data()+field.size(),value);
            if(parsed.ec!=std::errc{}||parsed.ptr!=field.data()+field.size())throw ScriptError("Civilopedia listener count is not an integer");
        }
        if(counts[0]!=1||counts[1]!=1)throw ScriptError("Civilopedia selected include invalidation registry changed");
        result.invalidation_listeners=counts[0]+counts[1];
        for(std::size_t i=2;i<counts.size();++i)result.civilopedia_listeners+=counts[i];
        if(!result.full_parent_loaded &&
           result.blocker.find("UIManager")!=std::string::npos)
            result.first_missing_api="UIManager.GetScreenSizeVal";
        else if(!result.full_parent_loaded && control_tree &&
                result.layout_operations_completed==9 &&
                result.blocker.find("CalculateInternalSize")!=std::string::npos)
            result.first_missing_api="ScrollPanel.CalculateInternalSize";
        if(!result.full_parent_loaded && control_tree &&
           result.layout_operations_completed==11 &&
           result.blocker.find(":169:")!=std::string::npos &&
           result.blocker.find("nil")!=std::string::npos)
            result.first_missing_api="Controls.CategoryButton1.RegisterCallback";
        if(!result.full_parent_loaded&&civis_left_click_event&&
           result.category_callback_controls>0&&
           result.registered_category_callbacks==result.category_callback_controls&&
           result.blocker.find("Locale")!=std::string::npos){
            const auto call=parent.find("CivilopediaCategory[CategoryHomePage].labelString = Locale.ConvertTextKey(");
            if(call!=parent.npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=std::string::npos)
                    result.first_missing_api="Locale.ConvertTextKey";
            }
        }
        if(text_owner&&!result.full_parent_loaded){
            // Classify only a real failure at a matching installed statement.
            // A formatter rejection is retained as its original resource error;
            // these observations do not publish the incomplete VM.
            for(const auto &api:{std::string("Locale.ConvertTextKey"),std::string("table.count")}){
                if(api=="Locale.ConvertTextKey"&&result.blocker.find("localized")==result.blocker.npos&&
                   result.blocker.find("localization")==result.blocker.npos)continue;
                if(api=="table.count"&&(result.blocker.find("count")==result.blocker.npos||result.blocker.find("nil")==result.blocker.npos))continue;
                std::size_t cursor=0;
                while((cursor=parent.find(api,cursor))!=parent.npos){
                    const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(cursor),'\n');
                    if(result.blocker.find(":"+std::to_string(line)+":")!=result.blocker.npos){result.first_missing_api=api;break;}
                    cursor+=api.size();
                }
            }
        }
        if(raw_table_count&&!result.full_parent_loaded&&result.blocker.find("GameOptionTypes")!=result.blocker.npos&&
           result.blocker.find("nil")!=result.blocker.npos){
            const auto api=std::string("GameOptionTypes.GAMEOPTION_NO_RELIGION");
            const auto call=parent.find(api);
            if(call!=parent.npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=result.blocker.npos)result.first_missing_api=api;
            }
        }
        if(option_owner&&!result.full_parent_loaded&&result.blocker.find("CreateQuery")!=result.blocker.npos&&
           result.blocker.find("nil")!=result.blocker.npos){
            const auto call=parent.find("DB.CreateQuery(sql)");
            if(call!=parent.npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=result.blocker.npos)result.first_missing_api="DB.CreateQuery";
            }
        }
        if(query_owner){
            if(buildings_query&&!result.full_parent_loaded&&
               result.blocker.find("SelectedCategoryTab")!=result.blocker.npos&&
               result.blocker.find("nil")!=result.blocker.npos){
                const auto call=parent.find("Controls.SelectedCategoryTab:SetOffsetVal(");
                if(call!=parent.npos){
                    const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                    if(result.blocker.find(":"+std::to_string(line)+":")!=result.blocker.npos)
                        result.first_missing_api="Controls.SelectedCategoryTab.SetOffsetVal";
                }
            }
            if(!result.full_parent_loaded&&result.blocker.find("CreateQuery only allows")!=result.blocker.npos){
                const auto call=parent.find("local BuildingsByEra = DB.CreateQuery(sql);");
                if(call!=parent.npos){
                    const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                    if(result.blocker.find(":"+std::to_string(line)+":")!=result.blocker.npos)
                        result.first_missing_api="DB.CreateQuery(Buildings statement)";
                }
            }
            query_owner->close();
        }
        if(option_owner)option_owner->close();
        if(text_owner)text_owner->close();
        if(callback_owner){callback_owner->close();host.close_ui_context("CivilopediaScreen");}
        if(tooltip_owner){
            const auto call=parent.find("searchableList[Locale.ToLower(name)] = article;");
            if(!result.full_parent_loaded&&call!=parent.npos&&result.blocker.find("ToLower")!=std::string::npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=std::string::npos)result.first_missing_api="Locale.ToLower";
            }
            tooltip_owner->close();
        }
        if(input_owner){
            const auto call=parent.find("TTManager:GetTypeControlTable( \"TypeRoundImage\", tipControlTable );");
            if(!result.full_parent_loaded&&call!=parent.npos&&result.blocker.find("TTManager")!=std::string::npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=std::string::npos)
                    result.first_missing_api="TTManager.GetTypeControlTable(TypeRoundImage)";
            }
            input_owner->close();
        }
        if(locale){
            const auto call=parent.find("Controls.SearchEditBox:RegisterCallback( OnSearchTextEnter );");
            if(!result.full_parent_loaded&&call!=parent.npos&&
               result.blocker.find("RegisterCallback")!=std::string::npos){
                const auto line=1+std::count(parent.begin(),parent.begin()+static_cast<std::ptrdiff_t>(call),'\n');
                if(result.blocker.find(":"+std::to_string(line)+":")!=std::string::npos)
                    result.first_missing_api="Controls.SearchEditBox.RegisterCallback(OnSearchTextEnter)";
            }
            locale->close();
        }
        if(result.full_parent_loaded)
            result.blocker="Civilopedia parent loaded in diagnostic VM; remaining selected contexts and atomic publication unavailable";
        else if(result.first_missing_api.empty())
            throw std::runtime_error("Civilopedia parent stopped at an unclassified source dependency: "+result.blocker);
    } catch(const std::exception &error) {
        result.full_parent_loaded=false;
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::runtime

namespace civ5::runtime {
namespace {
void production_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct ProductionDirtyStage {
 std::vector<BnwCityDirtyEvent> before_flags,after_flags;
 std::vector<BnwSpecificCityInfoEvent> before_specific,after_specific;
 std::uint64_t before_occurrence=0,after_occurrence=0,notifications=0;
};
ProductionDirtyStage stage_production_dirty(const LiveSessionState& state,std::uint64_t generation,
 std::int32_t owner,std::int32_t city,bool selected,bool help,bool choice){
 ProductionDirtyStage p;p.before_flags=state.city_dirty_events;p.after_flags=p.before_flags;
 p.before_specific=state.specific_city_info_events;p.after_specific=p.before_specific;
 p.before_occurrence=state.city_dirty_next_occurrence;p.after_occurrence=p.before_occurrence;
 auto flag=[&](BnwCityDirtyKind kind){production_need(p.after_occurrence<UINT64_MAX,"Production dirty occurrence exhausted");p.after_flags.push_back({p.after_occurrence++,generation,owner,city,-1,-1,kind,true});++p.notifications;};
 if(help)flag(BnwCityDirtyKind::help);
 if(choice){if(selected){flag(BnwCityDirtyKind::selection_buttons);flag(BnwCityDirtyKind::city_screen);flag(BnwCityDirtyKind::plot_list_buttons);}flag(BnwCityDirtyKind::city_info);
  production_need(p.after_occurrence<UINT64_MAX,"Production specific occurrence exhausted");p.after_specific.push_back({p.after_occurrence++,generation,owner,city,BnwSpecificCityInfoKind::production});++p.notifications;}
 production_need(p.notifications<=UINT64_MAX-state.ui_source_revision&&state.ui_source_revision<UINT64_MAX-p.notifications,
  "Production native prefix/dirty revision exhausted");return p;
}
void store_production_dirty(LiveSessionState& state,ProductionDirtyStage& p){
 production_need(state.city_dirty_events==p.before_flags&&state.specific_city_info_events==p.before_specific&&state.city_dirty_next_occurrence==p.before_occurrence&&p.notifications<=UINT64_MAX-state.ui_source_revision,"Production real dirty queue changed before stores");
 static_assert(noexcept(state.city_dirty_events.swap(p.after_flags)));static_assert(noexcept(state.specific_city_info_events.swap(p.after_specific)));
 state.city_dirty_events.swap(p.after_flags);state.specific_city_info_events.swap(p.after_specific);state.city_dirty_next_occurrence=p.after_occurrence;state.ui_source_revision+=p.notifications;
}
}
struct BnwSelectedProductionActionLoan::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> ended{false};
 std::shared_ptr<BnwSelectedStockLuaSourceOwner> owner;
 std::shared_ptr<BnwSelectedStockLuaSourceOwner::StockThreadLease> creator;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::int32_t actor=-1,city=-1;
 void check()const{if(thread!=std::this_thread::get_id()){creator->poisoned.store(true);throw std::runtime_error("Production loan foreign creator");}production_need(!ended.load(),"Production loan ended");owner->require_production_binding(*host,host->session(),actor,city);}
};
BnwSelectedProductionActionLoan::BnwSelectedProductionActionLoan(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwSelectedProductionActionLoan::~BnwSelectedProductionActionLoan(){const auto p=impl_;if(p->thread!=std::this_thread::get_id())p->creator->poisoned.store(true);p->ended.store(true);}
void BnwSelectedProductionActionLoan::require_same(const ui::SelectedBnwPreviewHost& host,const LiveSession& session,std::int32_t actor,std::int32_t city)const{
 const auto p=impl_;p->check();production_need(p->host.get()==&host&&&host.session()==&session&&p->actor==actor&&p->city==city,"Production loan SAME actual request differs");p->check();
}
SelectedProductionResult BnwSelectedProductionActionLoan::preview_maintain(ui::SelectedMaintainPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Maintain loan different actual city");return p->owner->run_production_maintain(action,false);}
SelectedProductionResult BnwSelectedProductionActionLoan::execute_maintain(ui::SelectedMaintainPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Maintain loan different actual city");return p->owner->run_production_maintain(action,true);}
SelectedProductionResult BnwSelectedProductionActionLoan::preview_construct(ui::SelectedConstructPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Construct loan different actual city");return p->owner->run_production_construct(action,false);}
SelectedProductionResult BnwSelectedProductionActionLoan::execute_construct(ui::SelectedConstructPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Construct loan different actual city");return p->owner->run_production_construct(action,true);}
SelectedProductionResult BnwSelectedProductionActionLoan::preview_train(ui::SelectedTrainPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Train loan different actual city");return p->owner->run_production_train(action,false);}
SelectedProductionResult BnwSelectedProductionActionLoan::execute_train(ui::SelectedTrainPreparedAction& action){const auto p=impl_;p->check();production_need(action.owner()==p->actor&&action.city()==p->city,"Train loan different actual city");return p->owner->run_production_train(action,true);}
void BnwSelectedStockLuaSourceOwner::require_production_binding(const ui::SelectedBnwPreviewHost& host,const LiveSession& session,std::int32_t actor,std::int32_t city)const{
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm,"Production genuine canonical VM absent");bound->require_no_pending_train_renderer();
 production_need(bound->selected.get()==&host&&&host.session()==&session,"Production genuine canonical Host/session absent");bound->check();
 production_need(actor==host.active_player_id()&&detail::bnw_game_active_player_read(*bound->state)==actor&&resolve_bnw_current_city(*bound->state,actor,city),"Production active source city absent");bound->vm->require_vm_current_stamp();bound->check();
}
std::unique_ptr<BnwSelectedProductionActionLoan> BnwSelectedStockLuaSourceOwner::borrow_city_production(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&host&&bound->selected.get()==host.get()&&!bound->selected.owner_before(host)&&!host.owner_before(bound->selected),"Production supplied Host control differs");
 canonical->require_production_binding(*host,host->session(),actor,city);
 auto p=std::make_shared<BnwSelectedProductionActionLoan::Impl>();p->owner=canonical;p->creator=thread;p->host=bound->selected;p->actor=actor;p->city=city;
 auto loan=std::unique_ptr<BnwSelectedProductionActionLoan>(new BnwSelectedProductionActionLoan(std::move(p)));loan->require_same(*host,host->session(),actor,city);return loan;
}
std::unique_ptr<BnwSelectedProductionActionLoan> borrow_selected_city_production(std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city){
 production_need(bool(supplied),"Production actual stock owner absent");const auto canonical=supplied->retain_action_factory_owner();
 production_need(canonical&&canonical.get()==supplied.get()&&!canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"Production supplied stock owner control differs");return canonical->borrow_city_production(std::move(host),actor,city);
}
SelectedProductionResult BnwSelectedStockLuaSourceOwner::run_production_maintain(ui::SelectedMaintainPreparedAction& action,bool execute){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm,"Maintain actual selected VM absent");bound->require_no_pending_train_renderer();bound->check();
 try{return bound->run_operation(BnwStockLuaOperationKind::city_production_choice,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  action.require_for_source_host(*bound->selected,bound->selected->session());bound->check_active();
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::player_can_maintain,action.owner(),action.city(),action.process(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual PlayerCanMaintain veto"};
  action.require_current_native();bound->check_active();lease.require_operation(serial);
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::city_can_maintain,action.owner(),action.city(),action.process(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual CityCanMaintain veto"};
  action.require_current_native();bound->check_active();lease.require_operation(serial);
  if(!execute)return SelectedProductionResult{SelectedProductionDisposition::eligible,false,{}};
  auto& state=*bound->selected->session_->state_;const auto before=state.ui_source_revision;
  bool selected=false;const auto team=detail::bnw_player_team_read(state,action.owner());
  if(team==detail::bnw_game_active_team_read(state)||bound->selected->session().bnw_game_debug_mode()){
   const auto input=bound->selected->session().city_selection_.lock();production_need(bool(input),"Maintain canonical client selection absent");input->require_current();selected=input->is_selected(action.owner(),action.city());input->require_current();}
  auto dirty=stage_production_dirty(state,bound->generation,action.owner(),action.city(),selected,false,true);
  bound->check_active();lease.require_operation(serial);action.require_current_native();action.apply_native_in_place(state);
  bound->check(true);production_need(action.native_committed()&&before<UINT64_MAX&&state.ui_source_revision==before+1,"Maintain genuine native store revision differs");
  bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  store_production_dirty(state,dirty);bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  return SelectedProductionResult{SelectedProductionDisposition::committed,true,{}};
 });}catch(const std::exception& e){return {SelectedProductionDisposition::uncertain,action.native_committed(),e.what()};}
 catch(...){return {SelectedProductionDisposition::uncertain,action.native_committed(),"Maintain SAME VM/native operation failed"};}
}
SelectedProductionResult BnwSelectedStockLuaSourceOwner::run_production_construct(ui::SelectedConstructPreparedAction& action,bool execute){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm,"Construct actual selected VM absent");bound->require_no_pending_train_renderer();bound->check();
 try{return bound->run_operation(BnwStockLuaOperationKind::city_production_choice,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  action.require_for_source_host(*bound->selected,bound->selected->session());action.require_player_native();bound->check_active();
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::player_can_construct,action.owner(),action.city(),action.building(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual PlayerCanConstruct veto"};
  bound->check_active();lease.require_operation(serial);
  if(!action.city_native_allowed())return SelectedProductionResult{SelectedProductionDisposition::blocked,false,"Actual City canConstruct predicate refused"};
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::city_can_construct,action.owner(),action.city(),action.building(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual CityCanConstruct veto"};
  action.require_current_native();bound->check_active();lease.require_operation(serial);
  if(!execute)return SelectedProductionResult{SelectedProductionDisposition::eligible,false,{}};
  auto& state=*bound->selected->session_->state_;const auto before=state.ui_source_revision;
  bool selected=false;const auto team=detail::bnw_player_team_read(state,action.owner());
  if(team==detail::bnw_game_active_team_read(state)||bound->selected->session().bnw_game_debug_mode()){
   const auto input=bound->selected->session().city_selection_.lock();production_need(bool(input),"Construct canonical client selection absent");input->require_current();selected=input->is_selected(action.owner(),action.city());input->require_current();}
  auto dirty=stage_production_dirty(state,bound->generation,action.owner(),action.city(),selected,true,true);
  bound->check_active();lease.require_operation(serial);action.require_current_native();action.apply_native_in_place(state);
  bound->check(true);production_need(action.native_committed()&&before<UINT64_MAX&&state.ui_source_revision==before+1,"Construct genuine native store revision differs");
  bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  store_production_dirty(state,dirty);bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  return SelectedProductionResult{SelectedProductionDisposition::committed,true,{}};
 });}catch(const std::exception& e){return {SelectedProductionDisposition::uncertain,action.native_committed(),e.what()};}
 catch(...){return {SelectedProductionDisposition::uncertain,action.native_committed(),"Construct SAME VM/native operation failed"};}
}
SelectedProductionResult BnwSelectedStockLuaSourceOwner::run_production_train(ui::SelectedTrainPreparedAction& action,bool execute){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm,"Train actual selected VM absent");bound->require_no_pending_train_renderer();bound->check();
 try{return bound->run_operation(BnwStockLuaOperationKind::city_production_choice,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  action.require_for_source_host(*bound->selected,bound->selected->session());action.require_player_native();bound->check_active();
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::player_can_train,action.owner(),action.city(),action.unit(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual PlayerCanTrain veto"};
  bound->check_active();lease.require_operation(serial);
  if(!action.city_native_allowed())return SelectedProductionResult{SelectedProductionDisposition::blocked,false,"Actual City canTrain predicate refused"};
  if(bound->vm->test_production_owned(BnwSelectedProductionLuaEvent::city_can_train,action.owner(),action.city(),action.unit(),lease)==std::optional{false})return SelectedProductionResult{SelectedProductionDisposition::vetoed,false,"Actual CityCanTrain veto"};
  action.require_current_native();bound->check_active();lease.require_operation(serial);
  if(!execute)return SelectedProductionResult{SelectedProductionDisposition::eligible,false,{}};
  auto& state=*bound->selected->session_->state_;const auto before=state.ui_source_revision;
  bool selected=false;const auto team=detail::bnw_player_team_read(state,action.owner());
  if(team==detail::bnw_game_active_team_read(state)||bound->selected->session().bnw_game_debug_mode()){
   const auto input=bound->selected->session().city_selection_.lock();production_need(bool(input),"Train canonical client selection absent");input->require_current();selected=input->is_selected(action.owner(),action.city());input->require_current();}
  auto dirty=stage_production_dirty(state,bound->generation,action.owner(),action.city(),selected,true,true);
  bound->check_active();lease.require_operation(serial);action.require_current_native();action.apply_native_in_place(state);
  bound->check(true);production_need(action.native_committed()&&before<UINT64_MAX&&state.ui_source_revision==before+1,"Train genuine native store revision differs");
  bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  store_production_dirty(state,dirty);bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  return SelectedProductionResult{SelectedProductionDisposition::committed,true,{}};
 });}catch(const std::exception& e){return {SelectedProductionDisposition::uncertain,action.native_committed(),e.what()};}
 catch(...){return {SelectedProductionDisposition::uncertain,action.native_committed(),"Train SAME VM/native operation failed"};}
}
void BnwSelectedStockLuaSourceOwner::run_completed_train_prefix(const std::shared_ptr<ui::SelectedCityTrainCreateSource>& supplied){
 const auto thread=stock_thread_;thread->check();production_need(bool(supplied),"TRAIN actual source absent");
 const auto source=supplied->retain_factory_owner();production_need(source&&source.get()==supplied.get()&&!source.owner_before(supplied)&&!supplied.owner_before(source),"TRAIN supplied source control differs");
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;production_need(bound&&bound->vm,"TRAIN actual selected VM absent");bound->check();
 const auto input=source->borrow_issuer_prefix();production_need(input.host&&input.host.get()==bound->selected.get()&&!input.host.owner_before(bound->selected)&&!bound->selected.owner_before(input.host)&&input.session==&bound->selected->session(),"TRAIN prepared source different genuine Host/session");
 bound->run_operation(BnwStockLuaOperationKind::train_pop_order_prefix,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  production_need(input.revision==bound->operation_world_revision&&input.help_occurrence==bound->state->city_dirty_next_occurrence,"TRAIN genuine prefix inputs changed");
  const auto key=std::pair{input.owner,input.city};production_need(!bound->train_attempts.contains(key),"TRAIN actual stock prefix already attempted; tail reconciliation required");
  auto& state=*bound->selected->session_->state_;auto dirty=stage_production_dirty(state,bound->generation,input.owner,input.city,false,true,false);
  bound->train_attempts.emplace(key,StockBootstrap::TrainAttempt{source,input.revision,input.help_occurrence});
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  for(const auto& [key,pending]:bound->selected->session_->production_turn_pending_){
   const auto& [turn,owner,handle,start,end]=key;(void)start;(void)end;
   if(turn!=state.setup.game_turn||owner!=input.owner||handle!=input.city)continue;
   production_need(bool(pending),"TRAIN completion actual registered producer absent");pending->require_current();
   production_need(pending->private_train_mode()&&pending->progress().phase==BnwCityProductionTurnPhase::pop_order_pending,"TRAIN counts require actual generated completion frontier");pending->invalidate();
  }
  source->apply_pop_order_prefix(state);
  bound->check(true);production_need(input.revision<UINT64_MAX&&state.ui_source_revision==input.revision+1,"TRAIN actual popOrder prefix revision differs");
  bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  store_production_dirty(state,dirty);bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);
  source->accept_actual_help_dirty(*canonical);bound->check_active();lease.require_operation(serial);return true;
 });
}
void execute_selected_completed_train_prefix(std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,const std::shared_ptr<ui::SelectedCityTrainCreateSource>& source){
 production_need(bool(supplied),"TRAIN stock owner absent");const auto canonical=supplied->retain_action_factory_owner();production_need(canonical&&canonical.get()==supplied.get()&&!canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"TRAIN supplied stock control differs");canonical->run_completed_train_prefix(source);
}
void BnwSelectedStockLuaSourceOwner::require_actual_train_help(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,std::int32_t actor,std::int32_t city,std::uint64_t occurrence,std::uint64_t revision)const{
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->selected.get()==&host&&bound->state.get()==&state,"TRAIN Help different actual Host/world");bound->check_active();
 const auto found=bound->train_attempts.find({actor,city});production_need(found!=bound->train_attempts.end()&&!found->second.source.expired()&&found->second.occurrence==occurrence&&found->second.before<UINT64_MAX&&revision==found->second.before+1,"TRAIN Help no genuine source attempt");
 production_need(revision<UINT64_MAX&&occurrence<UINT64_MAX&&state.ui_source_revision==revision+1&&state.city_dirty_next_occurrence==occurrence+1&&!state.city_dirty_events.empty(),"TRAIN Help actual queue/revision absent");
 const BnwCityDirtyEvent expected{occurrence,bound->generation,actor,city,-1,-1,BnwCityDirtyKind::help,true};production_need(state.city_dirty_events.back()==expected,"TRAIN Help actual engine flag identity differs");bound->check_active();
}
void require_selected_train_help_dirty(const BnwSelectedStockLuaSourceOwner& owner,const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,std::int32_t actor,std::int32_t city,std::uint64_t occurrence,std::uint64_t revision){owner.require_actual_train_help(host,state,actor,city,occurrence,revision);}
std::shared_ptr<const BnwCityProductionTurnPending> BnwSelectedStockLuaSourceOwner::run_process_component(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city,std::shared_ptr<const BnwCityProductionTurnPending> pending){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&host&&bound->selected.get()==host.get()&&!bound->selected.owner_before(host)&&!host.owner_before(bound->selected),"Process supplied Host control differs");bound->check();
 return bound->run_operation(BnwStockLuaOperationKind::process_production,[&]{
  bound->check_active();const auto before=bound->state->ui_source_revision;auto& session=*bound->selected->session_;
  std::optional<BnwCityProductionTurnProgress> old;if(pending){pending->require_current();old=pending->progress();production_need(old->owner==actor&&old->city==city,"Process actual cursor city differs");}
  auto result=pending?session.continue_bnw_city_production_process_suppressed(bound->selected,pending):session.begin_bnw_city_production_process_suppressed(bound->selected,actor,city);
  production_need(bool(result),"Process source returned no real registered cursor");const auto now=result->progress();
  bound->check(true);production_need(now.owner==actor&&now.city==city,"Process resulting actual cursor differs");
  const auto expected=old?std::uint64_t{1}:std::uint64_t{0};production_need(before<=UINT64_MAX-expected&&bound->state->ui_source_revision==before+expected,"Process genuine prefix revision delta differs");
  bound->operation_world_revision=bound->state->ui_source_revision;bound->check_active();result->require_current();return result;
 });
}
std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_process_production(std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city,std::shared_ptr<const BnwCityProductionTurnPending> pending){
 production_need(bool(supplied),"Process actual stock owner absent");const auto canonical=supplied->retain_action_factory_owner();production_need(canonical&&canonical.get()==supplied.get()&&!canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"Process supplied stock control differs");return canonical->run_process_component(std::move(host),actor,city,std::move(pending));
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwCityProductionTurnPending> BnwSelectedStockLuaSourceOwner::run_train_component(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city,std::shared_ptr<const BnwCityProductionTurnPending> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&host&&bound->selected.get()==host.get()&&!bound->selected.owner_before(host)&&!host.owner_before(bound->selected),"Production TRAIN supplied Host control differs");bound->check();
 return bound->run_operation(BnwStockLuaOperationKind::train_production,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);auto& session=*bound->selected->session_;auto& state=*session.state_;
  const auto before=state.ui_source_revision;std::shared_ptr<BnwCityProductionTurnPending> actual;
  if(supplied){
   supplied->require_current();const auto q=supplied->progress();production_need(q.owner==actor&&q.city==city,"Production TRAIN cursor actor/city differs");
   const auto it=session.production_turn_pending_.find(supplied->source_key());production_need(it!=session.production_turn_pending_.end()&&it->second.get()==supplied.get(),"Production TRAIN supplied cursor is not genuine registered producer");actual=it->second;
   // Entry must be the real TRAIN mode, not the project/process cursor. The
   // private source key/registration alone is insufficient to change modes.
   production_need(actual->private_train_mode(),"Production TRAIN supplied cursor has another actual mode");
   switch(q.phase){
    case BnwCityProductionTurnPhase::city_screen_predicate_pending:{
     const auto input=session.city_selection_.lock();production_need(bool(input),"Production TRAIN canonical selection owner absent");input->require_current();
     const bool selected=actor==detail::bnw_game_active_player_read(state)&&input->is_selected(actor,city);input->require_current();bound->check_active();lease.require_operation(serial);
     actual->begin_ui_effect();actual->accept_city_screen_predicate(selected);break;
    }
    case BnwCityProductionTurnPhase::city_screen_dirty_pending:{
     auto flags=state.city_dirty_events;production_need(before<UINT64_MAX&&state.city_dirty_next_occurrence<UINT64_MAX,"Production TRAIN CityScreen occurrence/revision exhausted");
     const auto occurrence=state.city_dirty_next_occurrence;flags.push_back({occurrence,bound->generation,actor,city,-1,-1,BnwCityDirtyKind::city_screen,true});
     bound->check_active();lease.require_operation(serial);actual->begin_ui_effect();production_need(state.ui_source_revision==before&&state.city_dirty_next_occurrence==occurrence,"Production TRAIN CityScreen inputs changed");
     state.city_dirty_events.swap(flags);++state.city_dirty_next_occurrence;++state.ui_source_revision;bound->operation_world_revision=state.ui_source_revision;bound->check_active();actual->accept_city_screen_publication();break;
    }
    case BnwCityProductionTurnPhase::banner_event_pending:{
     auto events=state.specific_city_info_events;production_need(before<UINT64_MAX&&state.city_dirty_next_occurrence<UINT64_MAX,"Production TRAIN BANNER occurrence/revision exhausted");
     const auto occurrence=state.city_dirty_next_occurrence;events.push_back({occurrence,bound->generation,actor,city,BnwSpecificCityInfoKind::banner});
     bound->check_active();lease.require_operation(serial);actual->begin_train_banner();production_need(state.ui_source_revision==before&&state.city_dirty_next_occurrence==occurrence,"Production TRAIN BANNER inputs changed");
     state.specific_city_info_events.swap(events);++state.city_dirty_next_occurrence;++state.ui_source_revision;bound->operation_world_revision=state.ui_source_revision;bound->check_active();actual->accept_train_banner();break;
    }
    default:actual->advance_one();break;
   }
  }else{
   const auto issued=session.begin_bnw_city_production_train(host,actor,city);const auto it=session.production_turn_pending_.find(issued->source_key());production_need(it!=session.production_turn_pending_.end()&&it->second.get()==issued.get(),"Production TRAIN actual initial registration absent");actual=it->second;
  }
  bound->check(true);production_need(state.ui_source_revision>=before&&state.ui_source_revision-before<=1,"Production TRAIN native stage revision exceeds exact store");bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);actual->require_current();return std::shared_ptr<const BnwCityProductionTurnPending>{std::move(actual)};
 });
}
std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_train_production(
 std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t actor,std::int32_t city,std::shared_ptr<const BnwCityProductionTurnPending> pending){
 production_need(bool(supplied),"Production TRAIN actual stock owner absent");const auto canonical=supplied->retain_action_factory_owner();production_need(canonical&&canonical.get()==supplied.get()&&!canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"Production TRAIN supplied source control differs");return canonical->run_train_component(std::move(host),actor,city,std::move(pending));
}
}

namespace civ5::runtime {
SelectedProductionResult BnwSelectedStockLuaSourceOwner::run_completed_train_tail(
 const std::shared_ptr<const ui::SelectedCityTrainCreateSource>& supplied){
 using D=SelectedProductionDisposition;
 const auto thread=stock_thread_;thread->check();production_need(bool(supplied),"TRAIN tail source absent");
 const auto source=supplied->retain_factory_owner();production_need(source&&source.get()==supplied.get()&&!source.owner_before(supplied)&&!supplied.owner_before(source),"TRAIN tail supplied source control differs");
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;production_need(bound&&bound->vm,"TRAIN tail actual selected VM absent");bound->check();
 source->require_tail_source_owner(*canonical);
 // This proof is outside the Lua operation: an unsupported CreateUnit return
 // refuses without poisoning a VM which has not attempted a hook.
 ui::SelectedCityTrainCreateSource::BorrowedTail input;
 try{input=source->borrow_completed_create_unit_return();}
 catch(const BnwNativeUnitAllocatorUnavailable& e){return {D::blocked,true,e.what()};}
 production_need(input.host&&input.host.get()==bound->selected.get()&&!input.host.owner_before(bound->selected)&&!bound->selected.owner_before(input.host),"TRAIN tail different genuine Host");
 const auto attempt=bound->train_attempts.find({input.owner,input.city});production_need(attempt!=bound->train_attempts.end(),"TRAIN tail no actual popOrder/Help attempt");
 const auto recorded=attempt->second.source.lock();production_need(recorded&&recorded.get()==source.get()&&!recorded.owner_before(source)&&!source.owner_before(recorded),"TRAIN tail actual source lineage differs");
 production_need(input.revision==bound->state->ui_source_revision,"TRAIN tail actual source revision differs");
 try{return bound->run_operation(BnwStockLuaOperationKind::train_pop_order_tail,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  source->require_current_native();
  if(source->city_trained_hook_pending()){
   source->begin_city_trained_hook();bound->check_active();lease.require_operation(serial);
   bound->vm->call_city_trained_owned(input.owner,input.city,input.unit_id,lease);
   bound->check_active();lease.require_operation(serial);source->accept_city_trained_hook();
   return SelectedProductionResult{D::committed,false,"Actual SAME VM CityTrained completed; native overflow/reset/pop tail pending"};
  }
  auto& session=*bound->selected->session_;auto& state=*session.state_;
  const auto selection=session.city_selection_.lock();production_need(bool(selection),"TRAIN tail actual selection owner absent");selection->require_current();
  const bool selected=input.owner==detail::bnw_game_active_player_read(state)&&selection->is_selected(input.owner,input.city);selection->require_current();
  bound->check_active();lease.require_operation(serial);source->require_current_native();
  auto result=source->apply_completion_native_stage(state,selected,bound->generation);
  bound->check(true);bound->operation_world_revision=state.ui_source_revision;bound->check_active();lease.require_operation(serial);source->require_current_native();return result;
 });}catch(const std::exception& e){return {D::uncertain,true,e.what()};}
 catch(...){return {D::uncertain,true,"TRAIN completion native/Lua prefix uncertain; committed effects are retained"};}
}
SelectedProductionResult advance_selected_completed_train_tail(std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,
 const std::shared_ptr<const ui::SelectedCityTrainCreateSource>& source){
 production_need(bool(supplied),"TRAIN tail stock owner absent");const auto actual=supplied->retain_action_factory_owner();
 production_need(actual&&actual.get()==supplied.get()&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),"TRAIN tail supplied stock control differs");return actual->run_completed_train_tail(source);
}
}

namespace civ5::runtime {
ui::SelectedTrainInitializationObservation BnwSelectedStockLuaSourceOwner::run_completed_train_initialization(
 const ui::SelectedBnwReadOnlyPreview& preview,const std::shared_ptr<const ui::SelectedCityTrainCreateSource>& supplied,
 const std::shared_ptr<const BnwNativeUnitAllocatorCore>& incoming_core,std::shared_ptr<ui::BnwClientSceneFrame> scene){
 using D=SelectedProductionDisposition;
 const auto thread=stock_thread_;thread->check();production_need(supplied&&incoming_core&&scene,"TRAIN live initialization source/Core/scene absent");
 const auto source=supplied->retain_factory_owner();production_need(source&&source.get()==supplied.get()&&!source.owner_before(supplied)&&!supplied.owner_before(source),"TRAIN live initialization Source control differs");
 const auto actual=retain_action_factory_owner();source->require_tail_source_owner(*actual);
 const auto input=source->borrow_bound_initialization(incoming_core);const auto core=input.core;
 const auto bound=actual->bootstrap_;production_need(bound&&bound->vm&&input.host.get()==bound->selected.get()&&!input.host.owner_before(bound->selected)&&!bound->selected.owner_before(input.host),"TRAIN live initialization different actual Host/VM");bound->check();
 const auto attempt=bound->train_attempts.find({input.owner,input.city});production_need(attempt!=bound->train_attempts.end(),"TRAIN initialization no real popOrder/Help attempt");
 const auto held=attempt->second.source.lock();production_need(held&&held.get()==source.get()&&!held.owner_before(source)&&!source.owner_before(held),"TRAIN initialization source lineage differs");
 ui::SelectedTrainInitializationObservation observation{core,{}, {D::blocked,true,"TRAIN actual initialization not attempted"},{}};
 std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> opening_gui;
 try{auto result=bound->run_operation(BnwStockLuaOperationKind::train_initialization,[&]{
  bound->check_active();const auto serial=bound->operation_control->serial;const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto gui=bound->vm->retain_research_choice_gui_owned();gui->require_same_selected_host(*bound->selected);bound->check_active();lease.require_operation(serial);
  opening_gui=gui;
  // Graphics owns this CLOSED lender. It authenticates actual Scene/GUI/frame
  // control and copies the target BEFORE callbackful checks. It is opened
  // while the current frame still predates assignID's real world revision.
  scene->with_actual_pending_unit_visual_loan(gui,lease,[&](const graphics::BnwSelectedPendingUnitVisualLoan& actual_visual_loan){
   actual_visual_loan.require_pre_store_current();
   const auto actual_receiver=actual_visual_loan.canonical_receiver();
   actual_visual_loan.require_pre_store_current();
   const auto adopt_actual=[&]{
    const auto after=source->borrow_bound_initialization(core);bound->check(true);
    production_need(after.host.get()==bound->selected.get()&&after.revision==bound->state->ui_source_revision,"TRAIN initialization effect lacks actual Source/Core revision acknowledgment");
    bound->operation_world_revision=after.revision;bound->check_active();lease.require_operation(serial);source->require_current_native();
   };
   source->require_current_native();bound->check_active();lease.require_operation(serial);
   (void)core->reserve_constructor_reset_prefix();adopt_actual();
   (void)core->execute_constructor_storage_tail();adopt_actual();
   actual_visual_loan.require_pre_store_current();
   actual_visual_loan.bind_pre_assignment_core(*core);
   (void)core->assign_pending_unit_native_id();adopt_actual();
   (void)core->execute_bound_unit_init_reset(preview);adopt_actual();
   (void)core->execute_bound_unit_initial_facing_hover_prefix();adopt_actual();
   observation.pending=core->begin_initial_set_xy_prefix();adopt_actual();
   observation.pending=core->advance_initial_destination_collision_prefix();adopt_actual();
   // Genuine Core returns actual Pending through the loan's canonical receiver;
   // PostMessage, bools, queued intents and old-frame readiness cannot replace it.
   observation.pending=core->advance_initial_city_visibility_prefix(actual_receiver);
   production_need(bool(observation.pending),"TRAIN visibility real Pending absent");
   actual_visual_loan.require_hidden_slot_proof(*core,*observation.pending);adopt_actual();
   // Keep the SAME actual renderer lender open through the native position
   // effect. The old GUI is never revalidated/rebased after earned stores.
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_city_garrison_fortify_prefix();
  const auto after=source->borrow_bound_initialization(core);bound->check(true);
  production_need(after.revision==bound->state->ui_source_revision,"TRAIN garrison actual Source/Core ACK absent");bound->operation_world_revision=after.revision;
  bound->check_active();lease.require_operation(serial);source->require_current_native();
  // Actual native sight follows policy/garrison and fortify in SDK order.
  // Core19 refuses transition-receiver branches before map stores; Source20
  // ACK alone adopts the earned real same-world visibility/cache revision.
  observation.pending=core->advance_initial_no_receiver_sight_prefix();
  const auto sight_after=source->borrow_bound_initialization(core);bound->check(true);
  production_need(sight_after.host.get()==bound->selected.get()&&sight_after.revision==bound->state->ui_source_revision,"TRAIN sight actual Source/Core ACK absent");
  bound->operation_world_revision=sight_after.revision;
  bound->check_active();lease.require_operation(serial);source->require_current_native();
  // CvUnit::setXY: actual plot insertion/area counts follow real sight.
  observation.pending=core->advance_initial_plot_area_prefix();
  const auto plot_after=source->borrow_bound_initialization(core);bound->check(true);
  production_need(plot_after.host.get()==bound->selected.get()&&plot_after.revision==bound->state->ui_source_revision,"TRAIN plot/area actual Source/Core ACK absent");
  bound->operation_world_revision=plot_after.revision;
  bound->check_active();lease.require_operation(serial);source->require_current_native();
  // Actual native city strength refresh follows plot/area insertion.
  observation.pending=core->advance_initial_city_strength_prefix();
  const auto strength_after=source->borrow_bound_initialization(core);bound->check(true);
  production_need(strength_after.host.get()==bound->selected.get()&&strength_after.revision==bound->state->ui_source_revision,"TRAIN city strength actual Source/Core ACK absent");
  bound->operation_world_revision=strength_after.revision;
  bound->check_active();lease.require_operation(serial);source->require_current_native();
  // Continue the original setXY order on the same unpublished node. Only
  // the actual lender's canonical receiver can perform native SetPosition.
  observation.pending=core->advance_initial_post_city_checks_prefix();adopt_actual();
  observation.pending=core->advance_initial_set_position_prefix(actual_receiver);adopt_actual();
  observation.pending=core->advance_initial_unit_info_dirty_prefix();adopt_actual();
  observation.pending=core->advance_initial_enemy_city_range_prefix();adopt_actual();
  observation.pending=core->advance_initial_happiness_prefix();adopt_actual();
  // The real opaque Core-issued founder job is dispatched under this SAME
  // Bootstrap operation/VM; no Tech parent, supplied actor or zero result.
  bound->vm->dispatch_initial_happiness_founder_owned(*core,*source,lease);adopt_actual();
  observation.pending=core->advance_initial_happiness_native_suffix_prefix();adopt_actual();
  // Actual native excess/cache calculation plus SAME retained GameData flag
  // precedes the original four-integer UnitSetXY SAMEVM hook.
  observation.pending=core->advance_initial_happiness_return();adopt_actual();
  bound->vm->dispatch_initial_unit_setxy_owned(*core,*source,lease);adopt_actual();
  observation.pending=core->advance_initial_popup_release_and_garrison();adopt_actual();
  // The same initial Loan4 remains OPEN. Center uses real initialized storage,
  // pure original GUI/source origin and earned Core lineage, not oldFrame.current.
  production_need(bool(observation.pending),"TRAIN center actual predecessor Pending absent");
  scene->with_actual_initial_center_unit_renderer(*core,*observation.pending,lease,
   [&](const graphics::BnwInitialCenterUnitRendererLoan& center_loan){
    source->require_current_native();bound->check_active();lease.require_operation(serial);
    observation.pending=core->advance_initial_center_unit_visibility(center_loan,lease);
    adopt_actual();
   });
  // The actual center receiver returned; continue the native Init order in
  // the SAME still-OPEN original visual loan and canonical operation.
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_name_game_counter_prefix();adopt_actual();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_game_class_counter_prefix();adopt_actual();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_team_class_counter_prefix();adopt_actual();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_player_class_counter_prefix();adopt_actual();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_support_zero_prefix();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_resource_nuke_zero_prefix();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_military_support_prefix();adopt_actual();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_unit_info_promotion_prefix();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_unit_info_promotions_return();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_traits_promotions_return();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_player_promotions_return();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_embark_promotions_return();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_exotic_xp_return();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.pending=core->advance_initial_immobile_max_moves_prefix();
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  // Unpublished initialization stores retain world UiRev; no numeric ACK.
  // Real SetMoves/ShouldDim remains a mandatory receiver frontier.
  source->require_current_native();bound->check_active();lease.require_operation(serial);

  }); // Actual target destruction/finality still occurs inside the lender.
  source->require_current_native();bound->check_active();lease.require_operation(serial);
  observation.result={D::blocked,true,"Actual TRAIN constructor/ID/reset/facing/SetXY/visibility/garrison/sight/plot-area/city-strength/post-city/position/UnitInfo/happiness-founder/native-suffix/excess/GameData/UnitSetXY/popup-release/GARRISON/actual center-unit visibility prefix retained; actual name/GameTurnCreated/game unit-created prefix retained; actual UnitInfo, Traits, Player and embark promotion loops returned; exotic goods/native initial XP returned; immobile/maxMoves computed; actual SetMoves/ShouldDim/full Init/CreateUnit return and actual fresh-frame presentation pending"};
  return observation;
 });
 // Mint only after genuine operation/Loan COPY/invoke/DESTROY + journal tail
 // returned normally. Never inspect public result status as authority.
 thread->check();bound->check();bound->vm->require_vm_current_stamp();
 source->require_current_native();core->require_current();
 production_need(result.core.get()==core.get()&&!result.core.owner_before(core)&&!core.owner_before(result.core)&&result.pending&&opening_gui,"TRAIN normal return lost actual Core/Pending/opening GUI");
 result.pending->require_current();const auto& prefix=result.pending->prefix();
 const auto stored=source->borrow_bound_initialization(core);bound->check();
 production_need(!bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended&&
  bound->operation_control&&!bound->operation_control->invalidated.load()&&!bound->journal.empty()&&
  bound->journal.back().serial==bound->operation_control->serial&&bound->journal.back().kind==BnwStockLuaOperationKind::train_initialization&&
  bound->journal.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended&&
  stored.revision==bound->state->ui_source_revision&&prefix.initial_visibility_returned&&prefix.initial_sight_returned&&prefix.initial_plot_area_returned&&prefix.initial_city_strength_returned&&
  prefix.initial_post_city_checks_returned&&prefix.initial_set_position_returned&&prefix.initial_info_bar_dirty_stored&&
   prefix.initial_enemy_city_range_returned&&prefix.initial_happiness_prefix_stored&&prefix.initial_happiness_native_suffix_stored&&
   prefix.initial_happiness_excess_stored&&prefix.initial_happiness_returned&&prefix.initial_game_data_occurrence>0&&
   prefix.initial_unit_setxy_returned&&prefix.initial_popup_released&&prefix.popup_release_setter_occurrence>0&&
   prefix.initial_garrison_dirty_returned&&prefix.initial_center_unit_returned&&prefix.initial_name_game_counter_returned&&prefix.initial_game_class_counter_returned&&prefix.initial_team_class_counter_returned&&prefix.initial_player_class_counter_returned&&prefix.initial_support_zero_returned&&prefix.initial_resource_nuke_zero_returned&&prefix.initial_military_support_returned&&prefix.initial_unit_info_promotion_prefix_stored&&prefix.initial_unit_info_promotions_returned&&prefix.initial_traits_promotions_returned&&prefix.initial_player_promotions_returned&&prefix.initial_embark_promotions_returned&&prefix.initial_exotic_xp_returned&&prefix.initial_immobile_moves_computed&&
    prefix.boundary==core->initial_set_moves_earned_frontier_without_callbacks(*result.pending),"TRAIN real normal-return/source/phase handoff unavailable");
 auto receipt=std::shared_ptr<ui::BnwTrainRendererRefreshReceipt>(new ui::BnwTrainRendererRefreshReceipt(
  actual,source,core,result.pending,scene,opening_gui,bound->selected,bound->state,
  bound->operation_control,bound->vm.get(),bound->generation,bound->operation_control->serial,stored.revision));
 receipt->factory_self_=receipt;
 bound->pending_train_renderer=receipt;bound->pending_train_renderer_serial=bound->operation_control->serial;
 bound->pending_train_renderer_revision=stored.revision;
 actual->require_pending_train_renderer_refresh(*receipt);result.renderer_refresh=std::move(receipt);
 return result;
 }catch(const BnwNativeUnitAllocatorUnavailable& e){
  // Native predecessor may be committed. Retain actual Core+Pending storage;
  // no fake rollback or repeated prefix. The operation records uncertainty.
  bound->quarantine();observation.result={D::uncertain,true,e.what()};return observation;
 }catch(const std::exception& e){bound->quarantine();observation.result={D::uncertain,true,e.what()};return observation;}
 catch(...){bound->quarantine();observation.result={D::uncertain,true,"TRAIN initialization prefix uncertain; actual Core storage retained"};return observation;}
}
ui::SelectedTrainInitializationObservation initialize_selected_completed_train_unit(std::shared_ptr<BnwSelectedStockLuaSourceOwner> supplied,
 const ui::SelectedBnwReadOnlyPreview& preview,const std::shared_ptr<const ui::SelectedCityTrainCreateSource>& source,
 const std::shared_ptr<const BnwNativeUnitAllocatorCore>& core,std::shared_ptr<ui::BnwClientSceneFrame> scene){
 production_need(bool(supplied),"TRAIN initialization stock owner absent");const auto actual=supplied->retain_action_factory_owner();
 production_need(actual&&actual.get()==supplied.get()&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),"TRAIN initialization stock owner control differs");
 return actual->run_completed_train_initialization(preview,source,core,std::move(scene));
}
}

namespace civ5::runtime {
BnwStockLuaAttemptLease::ScopeIdentity BnwStockLuaAttemptLease::pin_active_scope_without_callbacks()const{
 const auto c=control_.lock();if(!c)throw std::runtime_error("Stock pure scope expired");
 // Immutable creator FIRST; a foreign rejection changes only an atomic.
 if(c->creator!=std::this_thread::get_id()){c->invalidated.store(true);throw std::runtime_error("Stock pure scope foreign creator");}
 const auto erased=c->source_owner.lock();if(!erased){c->invalidated.store(true);throw std::runtime_error("Stock pure scope actual source retired");}
 const auto actual=std::static_pointer_cast<const BnwSelectedStockLuaSourceOwner::StockBootstrap>(erased);
 if(!actual->thread||actual->thread->thread!=c->creator||actual->thread->retired.load()||actual->thread->poisoned.load()||
    actual->poisoned.load()||c->invalidated.load()||!actual->running||actual->operation_state!=BnwStockLuaOperationState::active||
    !actual->operation_control||actual->operation_control.get()!=c.get()||actual->operation_control.owner_before(c)||c.owner_before(actual->operation_control)||
    !actual->vm||actual->vm.get()!=c->vm||!actual->state||actual->state.get()!=c->world.get()||actual->state.owner_before(c->world)||c->world.owner_before(actual->state)||c->serial==0){
  c->invalidated.store(true);throw std::runtime_error("Stock pure scope has no actual canonical active VM/world");
 }
 return {c,erased,c->vm,c->world.get(),c->serial,c->creator};
}
void BnwStockLuaAttemptLease::require_scope_identity_without_callbacks(const ScopeIdentity& expected)const{
 const auto c=control_.lock();if(!c)throw std::runtime_error("Stock pure scope expired after native stores");
 if(c->creator!=std::this_thread::get_id()){c->invalidated.store(true);throw std::runtime_error("Stock pure scope foreign post-store creator");}
 // Compare actual canonical Control before resolving any erased source object.
 if(!expected.control||expected.control.get()!=c.get()||expected.control.owner_before(c)||c.owner_before(expected.control)||expected.creator!=c->creator){
  c->invalidated.store(true);throw std::runtime_error("Stock pure scope substituted control/creator");
 }
 const auto current=pin_active_scope_without_callbacks();
 if(!expected.source_owner||expected.source_owner.get()!=current.source_owner.get()||expected.source_owner.owner_before(current.source_owner)||current.source_owner.owner_before(expected.source_owner)||
    current.serial!=expected.serial||current.vm!=expected.vm||current.world!=expected.world){
  c->invalidated.store(true);throw std::runtime_error("Stock pure scope substituted operation/VM/world after native stores");
 }
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwMajorTeamHasTechPending> BnwSelectedStockLuaSourceOwner::run_same_team_tech(
 std::int64_t team,std::int64_t technology,std::int64_t player,bool announce){
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 if(!bound)throw std::runtime_error("Team:SetHasTech genuine selected SourceOwner/VM absent");
 bound->check();
 // Strong Traits stays ONLY in the actual executing source stack. The retained
 // VM dispatcher holds weak callbacks, never a Host/VM/world-retaining cycle.
 struct Attempt {
  std::weak_ptr<StockBootstrap> bound;
  std::shared_ptr<const BnwMajorTeamHasTechPending> receipt;
  std::shared_ptr<const BnwMajorPlayerTraitsOwner> traits;
  std::uint64_t serial=0;
 };
 const auto attempt=std::make_shared<Attempt>();attempt->bound=bound;
 try{
  return bound->run_operation(BnwStockLuaOperationKind::team_technology,[&]{
   bound->check_active();
   const BnwStockLuaAttemptLease lease(bound->operation_control);attempt->serial=lease.operation_serial();
   const std::weak_ptr<Attempt> weak=attempt;
   const auto current=[weak](){
    const auto actual=weak.lock();
    if(!actual)throw ScriptError("Team:SetHasTech actual executing source attempt expired");
    const auto owner=actual->bound.lock();
    if(!owner)throw ScriptError("Team:SetHasTech actual source VM expired");
    owner->check_active();
    if(owner->operation_control->serial!=actual->serial)throw ScriptError("Team:SetHasTech source operation replaced");
    return actual;
   };
   // Only opaque core-owned earned revision can be adopted. This is a private
   // receiver inside the executing SourceOwner, not caller rebaseline/ack.
   const auto adopt=[weak](const std::shared_ptr<const BnwMajorTeamHasTechPending>& receipt){
    const auto actual=weak.lock();const auto owner=actual?actual->bound.lock():nullptr;
    if(!actual||!owner||!receipt||actual->receipt.get()!=receipt.get()||
     actual->receipt.owner_before(receipt)||receipt.owner_before(actual->receipt))
     throw ScriptError("Team:SetHasTech substituted genuine core receipt");
    owner->thread->check();owner->check(true);
    if(!owner->running||owner->operation_state!=BnwStockLuaOperationState::active||
     !owner->operation_control||owner->operation_control->invalidated.load()||owner->operation_control->serial!=actual->serial)
     throw ScriptError("Team:SetHasTech actual source operation retired before earned adoption");
    receipt->require_stored_current(*owner->selected);
    const auto revision=receipt->after_revision();
    if(revision<owner->operation_world_revision||revision!=owner->state->ui_source_revision)
     throw ScriptError("Team:SetHasTech actual opaque earned revision differs");
    owner->operation_world_revision=revision;owner->check_active();
   };
   const auto dispatch=BnwOwnedTeamSetHasTechLuaDispatch::issue(lease,
    [current,adopt](const BnwOwnedTeamSetHasTechLuaDispatch::Arguments& args){
     const auto actual=current();const auto owner=actual->bound.lock();
     if(actual->receipt||actual->traits)throw ScriptError("Team:SetHasTech source begin repeated");
     BnwMajorTeamHasTechCommand command;
     command.team=args.team;command.technology=args.technology;command.player=args.player;
     command.first=args.first;command.announce=args.announce;
     command.expected_turn=owner->state->setup.game_turn;command.expected_generation=owner->generation;
     command.expected_ui_source_revision=owner->state->ui_source_revision;
     actual->receipt=BnwMajorTeamHasTechPending::begin_for_source_host(owner->selected,command,&actual->traits);
     adopt(actual->receipt);return actual->receipt;
    },
    [current,adopt](const auto& receipt){
     const auto actual=current();const auto owner=actual->bound.lock();
     receipt->begin_team_set_has_tech_dispatch(*owner->selected);adopt(receipt);
    },
    [current,adopt](const auto& receipt){
     const auto actual=current();const auto owner=actual->bound.lock();
     receipt->finish_team_set_has_tech_dispatch(*owner->selected);adopt(receipt);
     while(receipt->phase()==BnwMajorTeamHasTechPhase::city_strength_pending){
      (void)receipt->advance_city_strength(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::resource_reveal_pending){
      receipt->advance_resource_and_process_prefix(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::player_traits_pending){
      receipt->advance_first_player_traits(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::policy_cost_pending){
      receipt->advance_policy_and_post_player_prefix(*owner->selected);adopt(receipt);
     }
    },
    [current,adopt](const auto& receipt){
     const auto actual=current();const auto owner=actual->bound.lock();
     receipt->begin_team_tech_researched_dispatch(*owner->selected);adopt(receipt);
    },
    [current,adopt](const auto& receipt){
     const auto actual=current();const auto owner=actual->bound.lock();
     receipt->finish_team_tech_researched_dispatch(*owner->selected);adopt(receipt);
     receipt->advance_after_team_tech_researched(*owner->selected);adopt(receipt);
     if(receipt->phase()==BnwMajorTeamHasTechPhase::player_research_pending){
      receipt->advance_player_research_and_first_reward(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::technology_announcement_pending||
        receipt->phase()==BnwMajorTeamHasTechPhase::technology_share_pending){
      receipt->advance_announcements_and_sharing(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::espionage_research_pending){
      receipt->advance_espionage_prefix(*owner->selected);adopt(receipt);
     }
     while(receipt->phase()==BnwMajorTeamHasTechPhase::major_happiness_pending){
      receipt->advance_happiness_prefix(*owner->selected);adopt(receipt);
      while(receipt->phase()==BnwMajorTeamHasTechPhase::happiness_founder_pending||
            receipt->phase()==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending){
       const auto actor=receipt->begin_happiness_founder_dispatch(*owner->selected);adopt(receipt);
       const BnwStockLuaAttemptLease actual_lease(owner->operation_control);
       const auto result=owner->vm->accumulate_team_happiness_founder_owned(actor,actual_lease);
       receipt->finish_happiness_founder_dispatch(*owner->selected,result.value);adopt(receipt);
      }
      if(receipt->phase()==BnwMajorTeamHasTechPhase::happiness_suffix_pending){
       receipt->advance_happiness_suffix(*owner->selected);adopt(receipt);
      }
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::technology_ui_pending){
      receipt->advance_technology_ui_prefix(*owner->selected);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::reward_option_pending){
      owner->check_active();const auto package=owner->selected->early_package_;
      if(!package)throw ScriptError("Technology reward actual Host has no genuine early Package");
      owner->selected->require_same_early_package(*package);owner->check_active();
      const auto options=package->borrow_selected_user_options_for_source();
      owner->selected->require_same_early_package(*package);owner->check_active();
      if(owner->selected->early_package_.get()!=package.get()||
       owner->selected->early_package_.owner_before(package)||package.owner_before(owner->selected->early_package_))
       throw ScriptError("Technology reward actual Host Package changed during options issue");
      options->require_current();receipt->advance_technology_reward_ui(*owner->selected,*options);adopt(receipt);
     }
     if(receipt->phase()==BnwMajorTeamHasTechPhase::gameplay_tech_acquired_pending){
      receipt->advance_gameplay_tech_acquired(*owner->selected);adopt(receipt);
     }
    });
   // No new context, script chunk, synthetic listener, GUI activation or VM.
   // Bootstrap binds on the actual current VM and invokes its real C Team SID.
   bound->vm->invoke_team_tech_owned(dispatch,team,technology,player,announce,lease);
   // Current core deliberately cannot return past the remaining mandatory
   // suffix; an unexpected return never grants successful technology status.
   throw ScriptError("Team:SetHasTech mandatory suffix unexpectedly returned");
   return attempt->receipt;
  });
 }catch(const std::exception& error){
  throw BnwMajorTeamSourceFailure(error.what(),attempt->receipt);
 }catch(...){throw BnwMajorTeamSourceFailure("Team:SetHasTech source operation uncertain",attempt->receipt);}
}
}


namespace civ5::ui {
BnwTrainRendererRefreshReceipt::BnwTrainRendererRefreshReceipt(
 std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> owner,
 std::shared_ptr<const SelectedCityTrainCreateSource> source,
 std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> core,
 std::shared_ptr<const runtime::BnwUnitMovePending> pending,
 std::shared_ptr<BnwClientSceneFrame> scene,std::shared_ptr<const BnwOwnedCameraGuiScreen> gui,
 std::shared_ptr<const SelectedBnwPreviewHost> host,std::shared_ptr<const runtime::LiveSessionState> state,
 std::shared_ptr<const void> control,const void* vm,std::uint64_t generation,std::uint64_t serial,std::uint64_t revision)
 :source_owner_(std::move(owner)),train_source_(std::move(source)),
  origin_{std::move(host),std::move(scene),std::move(gui),std::move(core),std::move(pending),std::move(state),generation,serial,revision},
  operation_control_(std::move(control)),actual_vm_(vm){}
BnwTrainRendererRefreshReceipt::~BnwTrainRendererRefreshReceipt()=default;
std::shared_ptr<const BnwTrainRendererRefreshReceipt> BnwTrainRendererRefreshReceipt::retain_factory_owner()const{
 if(creator_!=std::this_thread::get_id()){invalidated_.store(true);throw std::runtime_error("TRAIN renderer receipt foreign creator");}
 const auto actual=factory_self_.lock();
 if(!actual||actual.get()!=this||invalidated_.load()||presented_)
  throw std::runtime_error("TRAIN renderer receipt actual factory retired/consumed");
 return actual;
}
void BnwTrainRendererRefreshReceipt::require_pending_current()const{
 const auto actual=retain_factory_owner();
 if(!source_owner_)throw std::runtime_error("TRAIN renderer receipt actual SourceOwner absent");
 source_owner_->require_pending_train_renderer_refresh(*actual);
}
BnwTrainRendererRefreshReceipt::RendererOrigin BnwTrainRendererRefreshReceipt::borrow_renderer_origin()const{
 const auto actual=retain_factory_owner();actual->require_pending_current();
 auto origin=origin_;actual->require_pending_current();return origin;
}
}
namespace civ5::runtime {
std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt>
BnwSelectedStockLuaSourceOwner::pending_train_renderer_refresh()const{
 const auto thread=stock_thread_;thread->check();const auto actual=retain_action_factory_owner();const auto bound=actual->bootstrap_;
 if(!bound)throw std::runtime_error("TRAIN renderer pending actual Bootstrap absent");
 const std::weak_ptr<const ui::BnwTrainRendererRefreshReceipt> empty;
 if(!bound->pending_train_renderer.owner_before(empty)&&!empty.owner_before(bound->pending_train_renderer))return {};
 const auto receipt=bound->pending_train_renderer.lock();
 if(!receipt){bound->quarantine();throw std::runtime_error("TRAIN pending receipt dropped; no replay/resumption");}
 actual->require_pending_train_renderer_refresh(*receipt);return receipt;
}
void BnwSelectedStockLuaSourceOwner::require_pending_train_renderer_refresh(const ui::BnwTrainRendererRefreshReceipt& supplied)const{
 const auto thread=stock_thread_;thread->check();const auto actual=retain_action_factory_owner();const auto bound=actual->bootstrap_;
 if(!bound)throw std::runtime_error("TRAIN renderer current actual Bootstrap absent");
 try{
  const auto canonical=supplied.retain_factory_owner();const auto pending=bound->pending_train_renderer.lock();
  const auto& origin=canonical->origin_;
  production_need(pending&&canonical.get()==pending.get()&&!canonical.owner_before(pending)&&!pending.owner_before(canonical)&&
   canonical->source_owner_.get()==actual.get()&&!canonical->source_owner_.owner_before(actual)&&!actual.owner_before(canonical->source_owner_),"TRAIN renderer substituted canonical receipt/source control");
  const bool refresh_slot=bound->operation_control&&origin.operation_serial<UINT64_MAX&&
   bound->operation_control->serial==origin.operation_serial+1&&!bound->journal.empty()&&
   bound->journal.back().serial==bound->operation_control->serial&&
   bound->journal.back().kind==BnwStockLuaOperationKind::train_renderer_refresh;
  const bool refresh_active=refresh_slot&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active;
  const bool normal_pending=!bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended&&
   ((bound->operation_control&&bound->operation_control->serial==origin.operation_serial)||
    (refresh_slot&&bound->journal.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended));
  production_need(bound->operation_control&&canonical->operation_control_.get()==bound->operation_control.get()&&
   !canonical->operation_control_.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(canonical->operation_control_)&&
   !bound->operation_control->invalidated.load()&&(refresh_active||normal_pending)&&
   bound->pending_train_renderer_serial==origin.operation_serial&&
   bound->pending_train_renderer_revision==origin.earned_revision&&bound->state->ui_source_revision==origin.earned_revision&&
   bound->vm.get()==canonical->actual_vm_&&bound->generation==origin.generation&&
   origin.host.get()==bound->selected.get()&&!origin.host.owner_before(bound->selected)&&!bound->selected.owner_before(origin.host)&&
   origin.state.get()==bound->state.get()&&!origin.state.owner_before(bound->state)&&!bound->state.owner_before(origin.state),"TRAIN pending real operation/VM/world/earned revision retired");
  bound->check(refresh_active);bound->vm->require_vm_current_stamp();
  canonical->train_source_->require_tail_source_owner(*actual);
  const auto stored=canonical->train_source_->borrow_bound_initialization(origin.core);
  origin.core->require_current();origin.pending->require_current();const auto& prefix=origin.pending->prefix();
  production_need(stored.core.get()==origin.core.get()&&!stored.core.owner_before(origin.core)&&!origin.core.owner_before(stored.core)&&
   stored.revision==origin.earned_revision&&prefix.initial_visibility_returned&&prefix.initial_sight_returned&&prefix.initial_plot_area_returned&&prefix.initial_city_strength_returned&&
   prefix.initial_post_city_checks_returned&&prefix.initial_set_position_returned&&prefix.initial_info_bar_dirty_stored&&
   prefix.initial_enemy_city_range_returned&&prefix.initial_happiness_prefix_stored&&prefix.initial_happiness_native_suffix_stored&&
   prefix.initial_happiness_excess_stored&&prefix.initial_happiness_returned&&prefix.initial_game_data_occurrence>0&&
   prefix.initial_unit_setxy_returned&&prefix.initial_popup_released&&prefix.popup_release_setter_occurrence>0&&
   prefix.initial_garrison_dirty_returned&&prefix.initial_center_unit_returned&&prefix.initial_name_game_counter_returned&&prefix.initial_game_class_counter_returned&&prefix.initial_team_class_counter_returned&&prefix.initial_player_class_counter_returned&&prefix.initial_support_zero_returned&&prefix.initial_resource_nuke_zero_returned&&prefix.initial_military_support_returned&&prefix.initial_unit_info_promotion_prefix_stored&&prefix.initial_unit_info_promotions_returned&&prefix.initial_traits_promotions_returned&&prefix.initial_player_promotions_returned&&prefix.initial_embark_promotions_returned&&prefix.initial_exotic_xp_returned&&prefix.initial_immobile_moves_computed&&
    prefix.boundary==origin.core->initial_set_moves_earned_frontier_without_callbacks(*origin.pending)&&origin.scene&&origin.opening_gui,
   "TRAIN pending actual source/Core/placement prefix retired");
  bound->check(refresh_active);bound->vm->require_vm_current_stamp();thread->check();
  production_need(bound->state->ui_source_revision==origin.earned_revision&&
   bound->operation_control->serial==(refresh_slot?origin.operation_serial+1:origin.operation_serial)&&
   !canonical->invalidated_.load()&&!canonical->presented_,"TRAIN pending changed during genuine currentness bookends");
 }catch(...){bound->quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::complete_train_renderer_refresh_after_present(
 std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt> incoming,
 const graphics::BnwTrainPresentedFrameProof& proof){
 const auto thread=stock_thread_;thread->check();
 const auto actual=retain_action_factory_owner();const auto bound=actual->bootstrap_;
 if(!bound||!bound->vm||!incoming)throw std::runtime_error("TRAIN presented completion actual inputs absent");
 try{
  const auto receipt=incoming->retain_factory_owner();
  production_need(receipt.get()==incoming.get()&&!receipt.owner_before(incoming)&&!incoming.owner_before(receipt),
   "TRAIN presented completion substituted receipt control");
  // Admit only original normal-return serial BEFORE any graphics callback.
  const auto pending=bound->pending_train_renderer.lock();
  production_need(pending&&pending.get()==receipt.get()&&!pending.owner_before(receipt)&&!receipt.owner_before(pending)&&
   bound->operation_control&&!bound->operation_control->invalidated.load()&&!bound->poisoned.load()&&
   !bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended&&
   bound->operation_control->serial==receipt->origin_.operation_serial&&receipt->origin_.operation_serial<UINT64_MAX,
   "TRAIN presented completion actual pending operation changed/reentered");
  actual->require_pending_train_renderer_refresh(*receipt);
  proof.require_current_for_train(*receipt);
  const auto supplied_gui=proof.canonical_gui_for_train(*receipt);
  production_need(bool(supplied_gui),"TRAIN genuine presented GUI absent");
  const auto new_gui=supplied_gui->retain_owner();
  production_need(new_gui&&new_gui.get()==supplied_gui.get()&&!new_gui.owner_before(supplied_gui)&&!supplied_gui.owner_before(new_gui),
   "TRAIN presented GUI changed canonical control");
  proof.require_current_for_train(*receipt);actual->require_pending_train_renderer_refresh(*receipt);
  bound->run_operation(BnwStockLuaOperationKind::train_renderer_refresh,[&]{
   // ACTIVE is this NEW genuine journal slot; never reuse opening ScopeIdentity
   // or call the suspended pending lookup after entering the operation.
   bound->check_active();const auto serial=bound->operation_control->serial;
   const BnwStockLuaAttemptLease lease(bound->operation_control);
   production_need(serial==receipt->origin_.operation_serial+1,"TRAIN refresh new serial differs");
   actual->require_pending_train_renderer_refresh(*receipt);
   proof.require_current_for_train(*receipt);
   const auto current_gui=proof.canonical_gui_for_train(*receipt);
   production_need(current_gui&&current_gui.get()==new_gui.get()&&!current_gui.owner_before(new_gui)&&!new_gui.owner_before(current_gui),
    "TRAIN proof substituted actual presented GUI during refresh");
   new_gui->require_same_selected_host(*bound->selected);new_gui->require_current();
   bound->check_active();lease.require_operation(serial);
   bound->vm->bind_gui_screen(new_gui);
   proof.require_current_for_train(*receipt);actual->require_pending_train_renderer_refresh(*receipt);
   bound->check_active();lease.require_operation(serial);return 0;
  });
  // Keep weak membership through all genuine operation-return bookends.
  proof.require_current_for_train(*receipt);actual->require_pending_train_renderer_refresh(*receipt);
  new_gui->require_current();bound->check();bound->vm->require_vm_current_stamp();thread->check();
  const auto final_pending=bound->pending_train_renderer.lock();
  production_need(final_pending&&final_pending.get()==receipt.get()&&!final_pending.owner_before(receipt)&&!receipt.owner_before(final_pending)&&
   !bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended&&
   !bound->poisoned.load()&&!bound->operation_control->invalidated.load()&&
   bound->operation_control->serial==receipt->origin_.operation_serial+1&&!bound->journal.empty()&&
   bound->journal.back().serial==bound->operation_control->serial&&
   bound->journal.back().kind==BnwStockLuaOperationKind::train_renderer_refresh&&
   bound->journal.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended&&
   bound->state->ui_source_revision==receipt->origin_.earned_revision&&!receipt->invalidated_.load()&&!receipt->presented_,
   "TRAIN refresh did not return with authentic stored prefix/current presentation");
  // Creator-only closed stores; no callback or allocation after final checks.
  // This acknowledges a presented partial prefix, NOT full Init/CreateUnit.
  receipt->presented_=true;
  // The genuine graphics lender finalizes ONLY after its target destructor.
  // Leave actual weak membership intact until that closed finalization.

 }catch(...){
  if(!bound->retained_error)bound->retained_error=std::current_exception();
  bound->quarantine();throw;
 }
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_train_presented_scope_without_callbacks(
 const ui::BnwTrainRendererRefreshReceipt& receipt)const{
 const auto actual=retain_action_factory_owner();const auto bound=actual->bootstrap_;
 production_need(bound&&bound->vm&&bound->operation_control,"TRAIN presented scope actual control absent");
 const auto canonical=receipt.factory_self_.lock();const auto pending=bound->pending_train_renderer.lock();const auto& origin=receipt.origin_;
 production_need(receipt.creator_==std::this_thread::get_id()&&canonical&&canonical.get()==&receipt&&
  pending&&pending.get()==canonical.get()&&!pending.owner_before(canonical)&&!canonical.owner_before(pending)&&
  receipt.source_owner_.get()==actual.get()&&!receipt.source_owner_.owner_before(actual)&&!actual.owner_before(receipt.source_owner_)&&
  receipt.presented_&&!receipt.invalidated_.load()&&!bound->poisoned.load()&&!bound->operation_control->invalidated.load()&&
  !bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended&&
  origin.operation_serial<UINT64_MAX&&bound->operation_control->serial==origin.operation_serial+1&&
  receipt.operation_control_.get()==bound->operation_control.get()&&!receipt.operation_control_.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(receipt.operation_control_)&&
  receipt.actual_vm_==bound->vm.get()&&bound->generation==origin.generation&&
  bound->pending_train_renderer_serial==origin.operation_serial&&bound->pending_train_renderer_revision==origin.earned_revision&&
  bound->state.get()==origin.state.get()&&!bound->state.owner_before(origin.state)&&!origin.state.owner_before(bound->state)&&
  bound->selected.get()==origin.host.get()&&!bound->selected.owner_before(origin.host)&&!origin.host.owner_before(bound->selected)&&
  bound->selected->session_&&bound->selected->session_->state_.get()==origin.state.get()&&
  bound->selected->session_->state_generation()==origin.generation&&bound->state->ui_source_revision==origin.earned_revision&&
  !bound->journal.empty()&&bound->journal.back().serial==bound->operation_control->serial&&
  bound->journal.back().kind==BnwStockLuaOperationKind::train_renderer_refresh&&
  bound->journal.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended,
  "TRAIN consumed scope actual canonical operation/world/journal retired");
 // No provider callback, GUI getter, DB query, allocation or new VM serial.
}
void BnwSelectedStockLuaSourceOwner::finish_train_presented_scope_after_target(
 const ui::BnwTrainRendererRefreshReceipt& receipt){
 require_train_presented_scope_without_callbacks(receipt);
 const auto bound=bootstrap_;
 bound->pending_train_renderer.reset();bound->pending_train_renderer_serial=0;bound->pending_train_renderer_revision=0;
}
void BnwSelectedStockLuaSourceOwner::quarantine_train_presented_scope_without_callbacks()noexcept{
 const auto bound=bootstrap_;if(!bound)return;
 if(stock_thread_->thread==std::this_thread::get_id())bound->quarantine();
 else {stock_thread_->poisoned.store(true);bound->poisoned.store(true);if(bound->operation_control)bound->operation_control->invalidated.store(true);}
}
}
namespace civ5::ui {
void BnwTrainRendererRefreshReceipt::require_presented_scope_without_callbacks()const{
 if(creator_!=std::this_thread::get_id()||!source_owner_)throw std::runtime_error("TRAIN presented scope creator/source absent");
 source_owner_->require_train_presented_scope_without_callbacks(*this);
}
void BnwTrainRendererRefreshReceipt::finish_presented_scope_after_target()const{
 require_presented_scope_without_callbacks();source_owner_->finish_train_presented_scope_after_target(*this);
}
void BnwTrainRendererRefreshReceipt::quarantine_presented_scope_without_callbacks()const noexcept{
 invalidated_.store(true);if(source_owner_)source_owner_->quarantine_train_presented_scope_without_callbacks();
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwMajorTechTransportConsumption> BnwSelectedStockLuaSourceOwner::dispatch_major_tech_award_actual(
 std::shared_ptr<const BnwMajorTechTransportRequest> request){
 stock_thread_->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 if(!bound||!request)throw ScriptError("TECH_AWARD actual Source/VM/request absent");
 try{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();request->require_current();
  if(request->operation_serial()!=serial)throw ScriptError("TECH_AWARD actual source operation differs");
  bound->check_active();lease.require_operation(serial);
  auto consumption=bound->vm->dispatch_major_tech_award_owned(request,lease);
  bound->check_active();lease.require_operation(serial);return consumption;
 }catch(...){bound->quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_engine_cache_revision(const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->operation_control&&!lease.control_.owner_before(bound->operation_control)&&
    !bound->operation_control.owner_before(lease.control_),"Engine UI cache adoption foreign operation");
 bound->check(true);cursor.require_engine_host(*bound->selected);
 production_need(bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&!bound->operation_control->invalidated.load(),"Engine UI cache adoption retired");
 bound->operation_world_revision=cursor.engine_actual_revision();
 production_need(bound->operation_world_revision==bound->state->ui_source_revision,"Engine UI cache earned revision differs");bound->check_active();
}

BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_engine_ui_turn(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&supplied,"Engine UI actual SourceOwner/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor&&cursor.get()==supplied.get()&&!cursor.owner_before(supplied)&&!supplied.owner_before(cursor),"Engine UI supplied cursor control differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();
  cursor->execute_engine_ui_with_actual_stock(*bound->vm,lease,*canonical);
  // Only this actual privately bound cursor can publish an earned revision.
  bound->thread->check();bound->check(true);cursor->require_engine_host(*bound->selected);
  production_need(bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&
    bound->operation_control&&!bound->operation_control->invalidated.load()&&bound->operation_control->serial==serial,
    "Engine UI actual operation retired before native cache adoption");
  bound->operation_world_revision=cursor->engine_actual_revision();
  production_need(bound->operation_world_revision==bound->state->ui_source_revision,"Engine UI earned actual revision differs");
  bound->check_active();lease.require_operation(serial);return cursor->view();
 });
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_tail_to_next_required_owner(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&supplied,"Engine UI actual SourceOwner/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor&&cursor.get()==supplied.get()&&!cursor.owner_before(supplied)&&!supplied.owner_before(cursor),"Engine UI supplied cursor control differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();
  cursor->advance_game_tail_to_next_required_owner();
  // Only this actual privately bound cursor can publish an earned revision.
  bound->thread->check();bound->check(true);cursor->require_engine_host(*bound->selected);
  production_need(bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&
    bound->operation_control&&!bound->operation_control->invalidated.load()&&bound->operation_control->serial==serial,
    "Engine UI actual operation retired before native cache adoption");
  bound->operation_world_revision=cursor->game_tail_actual_revision();
  production_need(bound->operation_world_revision==bound->state->ui_source_revision,"Engine UI earned actual revision differs");
  bound->check_active();lease.require_operation(serial);
  // Native outer Culture loop: each concrete child retains its real once/
  // predecessor contract. Up to22 actors x4 children plus UI reset/AI read.
  for(std::size_t steps=0;steps<90;++steps){
   const auto frontier=cursor->view().frontier;
   if(frontier==BnwRetainedTurnFrontier::culture_public_opinion_source_owner)
    cursor->execute_game_culture_public_opinion_with_actual_stock(*bound->vm,lease);
   else if(frontier==BnwRetainedTurnFrontier::culture_notification_source_owner)
    cursor->execute_game_culture_unmet_notifications_standalone_prefix();
   else if(frontier==BnwRetainedTurnFrontier::culture_log_source_owner)
    cursor->execute_game_culture_standalone_diagnostics_return();
   else if(frontier==BnwRetainedTurnFrontier::culture_next_player_source_owner)
    cursor->execute_game_culture_next_player_unmet_prefix();
   else if(frontier==BnwRetainedTurnFrontier::engine_ui_end_turn_reset_unavailable)
    cursor->execute_engine_ui_end_turn_reset_with_actual_stock(lease);
   else if(frontier==BnwRetainedTurnFrontier::game_ai_autoplay_source_owner)
    cursor->execute_game_ai_autoplay_zero_return();
   else break;
   bound->thread->check();bound->check(true);cursor->require_engine_host(*bound->selected);
   production_need(bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&bound->operation_control&&!bound->operation_control->invalidated.load()&&bound->operation_control->serial==serial,"Culture SAME operation retired before actual child revision adoption");
   bound->operation_world_revision=cursor->game_tail_actual_revision();
   production_need(bound->operation_world_revision==bound->state->ui_source_revision,"Culture actual child earned revision differs");
   bound->check_active();lease.require_operation(serial);
  }
  const auto final_frontier=cursor->view().frontier;
  production_need(final_frontier!=BnwRetainedTurnFrontier::culture_public_opinion_source_owner&&final_frontier!=BnwRetainedTurnFrontier::culture_notification_source_owner&&final_frontier!=BnwRetainedTurnFrontier::culture_log_source_owner&&final_frontier!=BnwRetainedTurnFrontier::culture_next_player_source_owner&&final_frontier!=BnwRetainedTurnFrontier::engine_ui_end_turn_reset_unavailable&&final_frontier!=BnwRetainedTurnFrontier::game_ai_autoplay_source_owner,"Culture native loop exhausted before actual mandatory source frontier");
  bound->check_active();lease.require_operation(serial);return cursor->view();
 });
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::run_same_tech_award_input(
 std::shared_ptr<const ui::BnwTechAwardInputDispatch> supplied){
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&supplied,"TechAward input genuine Source/token absent");
 // Canonical get/control authentication precedes token/provider callbacks.
 const auto token=supplied->retain_canonical(supplied);
 try{
  bound->run_operation(BnwStockLuaOperationKind::tech_award_input,[&]{
   bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
   const auto serial=lease.operation_serial();
   bound->vm->dispatch_tech_award_input_owned(token,lease);
   bound->check_active();lease.require_operation(serial);
   return token->phase(); // pending_frame is not a painted/completed popup.
  });
 }catch(...){token->fail_actual_dispatch();bound->quarantine();throw;}
 // Covers the genuine operation's outer retained-VM/renderer bookends too.
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_policy_adopt_hook_return(const BnwPolicyAdoptionPending& source)const{
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 if(!bound||!bound->policy_adopt_hook_return)throw std::runtime_error("PlayerAdoptPolicy genuine SourceOwner return absent");
 bound->thread->check();const auto& r=*bound->policy_adopt_hook_return;
 const auto actual=r.source.lock();const auto world=r.world.lock();
 if(!actual||actual.get()!=&source||!world||world.get()!=bound->state.get()||world.owner_before(bound->state)||bound->state.owner_before(world)||
  !bound->running||bound->operation_state!=BnwStockLuaOperationState::active||bound->poisoned.load()||!bound->operation_control||
  bound->operation_control->invalidated.load()||bound->operation_control->serial!=r.serial||bound->operation_control->vm!=r.vm||
  !bound->vm||bound->vm.get()!=r.vm||bound->generation!=r.generation||world->ui_source_revision!=r.revision||
  bound->operation_world_revision!=r.revision||!r.call_returned||!r.actual.errors.empty()||source.player()!=r.actor||source.policy()!=r.policy)
  throw std::runtime_error("PlayerAdoptPolicy SAME source/VM/world/operation/actual result differs");
 // Pure sealed result: the value is never caller input, a permission, or
 // fabricated false for no listeners. Only actual call_hook fills this seal.
}
}

namespace civ5::runtime {
BnwOwnedGameplayEventPumpLoan::SourceLineage
BnwSelectedStockLuaSourceOwner::require_gameplay_pump_source_without_callbacks()const {
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();
 // One genuine Pump weak lock plus this checker lock are excluded. The
 // synchronous actual pump operation's retained facade is separately counted.
 const auto facade_pins=canonical->gameplay_pump_facade_pins_.load();
 production_need(facade_pins<=static_cast<std::size_t>(std::numeric_limits<long>::max()-2)&&
   canonical.use_count()>static_cast<long>(facade_pins+2),
   "Gameplay pump selected source outside facade retired");
 const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->thread==thread,"Gameplay pump source allocation absent");
 bound->thread->check();
 production_need(!bound->poisoned.load()&&bound->operation_control&&
   !bound->operation_control->invalidated.load()&&bound->selected&&bound->state&&bound->vm&&
   bound->operation_state!=BnwStockLuaOperationState::retired&&
   bound->operation_state!=BnwStockLuaOperationState::uncertain,
   "Gameplay pump selected source retired/uncertain");
 const auto& control=bound->operation_control;
 production_need(control->creator==thread->thread&&control->vm==bound->vm.get()&&
   control->world.get()==bound->state.get()&&!control->world.owner_before(bound->state)&&
   !bound->state.owner_before(control->world)&&
   !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
   !control->selected_host.owner_before(bound->selected)&&!bound->selected.owner_before(control->selected_host),
   "Gameplay pump actual source Control/Host/world/VM substituted");
 const auto pins=control->transport_source_pins.load();
 production_need(pins<=static_cast<std::uint64_t>(std::numeric_limits<long>::max()-1)&&
   bound.use_count()>static_cast<long>(pins+1),
   "Gameplay pump counted source allocation outside owner retired");
 return {bound->selected,bound->state,control,bound,bound->vm.get(),bound->generation};
}
void BnwSelectedStockLuaSourceOwner::pump_gameplay_events_owned(const BnwOwnedGameplayEventPumpLoan& pump){
 pump.require_source_owner_without_callbacks(*this);
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();
 const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->operation_control&&bound->vm,"Gameplay pump actual VM absent");
 struct BorrowedIssuer final {
  const std::thread::id creator=std::this_thread::get_id();
  BnwSelectedStockLuaSourceOwner& facade;
  const std::shared_ptr<BnwStockLuaAttemptLease::Control> control;
  bool facade_counted=false,source_counted=false;
  BorrowedIssuer(BnwSelectedStockLuaSourceOwner& f,std::shared_ptr<BnwStockLuaAttemptLease::Control> c)
    :facade(f),control(std::move(c)){
   const auto f_old=facade.gameplay_pump_facade_pins_.fetch_add(1);
   if(f_old==std::numeric_limits<std::size_t>::max()){
    facade.gameplay_pump_facade_pins_.fetch_sub(1);throw std::runtime_error("Gameplay pump facade pins exhausted");}
   facade_counted=true;
   const auto s_old=control->transport_source_pins.fetch_add(1);
   if(s_old==std::numeric_limits<std::uint64_t>::max()){
    control->transport_source_pins.fetch_sub(1);facade.gameplay_pump_facade_pins_.fetch_sub(1);
    facade_counted=false;throw std::runtime_error("Gameplay pump source lender pins exhausted");}
   source_counted=true;
  }
  ~BorrowedIssuer(){
   if(creator!=std::this_thread::get_id())std::terminate();
   if(source_counted)control->transport_source_pins.fetch_sub(1);
   if(facade_counted)facade.gameplay_pump_facade_pins_.fetch_sub(1);
  }
 } borrowed(*canonical,bound->operation_control);
 try{
  pump.require_source_owner_without_callbacks(*canonical);
  bound->run_operation(BnwStockLuaOperationKind::gameplay_event_pump,[&]{
   pump.require_source_owner_without_callbacks(*canonical);
   bound->check_active();pump.require_source_owner_without_callbacks(*canonical);
   const BnwStockLuaAttemptLease lease(bound->operation_control);
   const auto source_scope=lease.pin_active_scope_without_callbacks();
   const auto serial=source_scope.serial;
   auto current=[&]{
    // Callback-free actual lender FIRST, before reusing saved facade/VM fields.
    pump.require_source_owner_without_callbacks(*canonical);
    lease.require_scope_identity_without_callbacks(source_scope);
    pump.require_for_source(lease);pump.require_source_owner_without_callbacks(*canonical);
    lease.require_operation(serial);pump.require_source_owner_without_callbacks(*canonical);
    lease.require_scope_identity_without_callbacks(source_scope);
   };
   current();bound->vm->drain_gameplay_events_owned(pump,lease);current();
   const std::weak_ptr<ui::BnwEngineUiTurnOwner> unissued_golden_engine;
   // An issued-but-expired control is mandatory failure, never an empty queue.
   if(bound->golden_age_event_engine.owner_before(unissued_golden_engine)||
      unissued_golden_engine.owner_before(bound->golden_age_event_engine)){
    ui::BnwEngineUiTurnOwner::drain_golden_age_gameplay_from_owned_weak(bound->golden_age_event_engine,pump,lease);
    current();
   }
   const std::weak_ptr<ui::BnwEngineUiTurnOwner> unissued_turn_engine;
   if(bound->game_turn_event_engine.owner_before(unissued_turn_engine)||unissued_turn_engine.owner_before(bound->game_turn_event_engine)){
    ui::BnwEngineUiTurnOwner::drain_game_turn_changed_from_owned_weak(bound->game_turn_event_engine,pump,lease);
    current();
   }
   // Snapshot only the already-issued canonical Engine control; no lock-false fallback.
   const auto notification_engine=bound->golden_age_event_engine;
   if(notification_engine.owner_before(unissued_golden_engine)||unissued_golden_engine.owner_before(notification_engine)){
    current();
    ui::BnwEngineUiTurnOwner::drain_notification_publication_from_owned_weak(notification_engine,pump,lease);
    current(); // FIRST after actual model callbacks, then full SAME-source/lease POST
    production_need(!notification_engine.owner_before(bound->golden_age_event_engine)&&
      !bound->golden_age_event_engine.owner_before(notification_engine),"Notification common-pump canonical Engine control changed");
   }
   // Read the real camera-pump Scene's strong native command owner. Never
   // fabricate a Scene/queue and never hold a transient Scene pin over Lua.
   std::weak_ptr<graphics::BnwUnitDamageCommandOwner> health_queue;
   {
    pump.pure();const auto actual_scene=pump.scene_.lock();
    production_need(static_cast<bool>(actual_scene),"Health pump actual Scene expired");
    health_queue=actual_scene->unit_damage_commands_;
    pump.pure();
   }
   const std::weak_ptr<graphics::BnwUnitDamageCommandOwner> unissued_health_queue;
   // A genuinely unbound Scene member has no commands. An issued weak control
   // must reach the real Parent pending query; expiration is mandatory failure.
   if(health_queue.owner_before(unissued_health_queue)||unissued_health_queue.owner_before(health_queue)){
    auto require_same_health_queue=[&]{
     pump.pure();const auto actual_scene=pump.scene_.lock();
     production_need(actual_scene&&
      !health_queue.owner_before(actual_scene->unit_damage_commands_)&&
      !actual_scene->unit_damage_commands_.owner_before(health_queue),
      "Health pump canonical Scene queue control changed");
     pump.pure();
    };
    for(;;){
     require_same_health_queue();current();require_same_health_queue();
     const bool consumed=bound->vm->try_dispatch_unitflag_health_head_owned(
       health_queue,pump,lease,*canonical);
     // Callback-free Scene/source FIRST, then existing full pump/Stock POST.
     require_same_health_queue();current();require_same_health_queue();
     if(!consumed)break;
    }
   }
   current();
   return 0;
  });
  pump.require_source_owner_without_callbacks(*canonical);
 }catch(...){bound->quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_clock_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"setGameTurn pure actual source/control adoption retired");
 cursor.require_game_turn_clock_store_return(*bound->selected,*bound->state,bound->generation,
  bound->turn,bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_clock_prefix(
 std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"setGameTurn actual source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor&&cursor.get()==supplied.get()&&!cursor.owner_before(supplied)&&!supplied.owner_before(cursor),
  "setGameTurn cursor canonical allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 const auto actual_timer=cursor->timer_owner();const auto timer_use=actual_timer->current_use();
 const auto timer_before=actual_timer->read(timer_use);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_paired_with_actual_stock(*canonical,lease);
  // Natural production regression probe: this SAME imported session clock
  // must preserve the actual timer allocation/use/start marks across112.
  // A stale cached turn fails require_current here, rather than being masked
  // by a compile-only check or substituted QPC value.
  production_need(cursor->timer_owner()==actual_timer,"setGameTurn changed actual timer allocation");
  actual_timer->require_current();const auto timer_after=actual_timer->read(timer_use);
  production_need(timer_after.current_start==timer_before.current_start&&timer_after.game_turn_start==timer_before.game_turn_start&&
   timer_after.frequency==timer_before.frequency&&timer_after.operation==timer_before.operation&&timer_after.reset==timer_before.reset&&
   timer_after.pause_delta==timer_before.pause_delta&&timer_after.current_stopped_elapsed==timer_before.current_stopped_elapsed&&
   timer_after.game_turn_stopped_elapsed==timer_before.game_turn_stopped_elapsed&&timer_after.current_elapsed>=timer_before.current_elapsed&&
   timer_after.game_turn_elapsed>=timer_before.game_turn_elapsed,"setGameTurn identity continuation restarted/changed actual timers");
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "setGameTurn genuine source expectations did not adopt paired stores");
  return cursor->view();
 });
}
}

namespace civ5::runtime {
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_ui_dirty_prefix(
 std::shared_ptr<BnwRetainedTurnCursor> supplied,std::shared_ptr<const ui::LiveUiHost> actual_ui){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied&&actual_ui,"setGameTurn actual UI source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor&&cursor.get()==supplied.get()&&!cursor.owner_before(supplied)&&!supplied.owner_before(cursor),
  "setGameTurn UI cursor canonical allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_ui_dirty_with_actual_stock(*actual_ui,*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "setGameTurn UI return changed actual adopted world expectations");
  return cursor->view();
 });
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_sent_auto_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"setGameTurn pure actual source/control adoption retired");
 cursor.require_game_turn_sent_auto_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}

BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_sent_auto_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"setGameTurn sentAutoMoves actual source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor&&cursor.get()==supplied.get()&&!cursor.owner_before(supplied)&&!supplied.owner_before(cursor),
  "setGameTurn sentAutoMoves canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_sent_auto_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "setGameTurn sentAutoMoves source expectation adoption differs");return cursor->view();
 });
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_gameplay_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"setGameTurn pure actual source/control adoption retired");
 cursor.require_game_turn_gameplay_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}


BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_gameplay_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied,
 std::shared_ptr<const ui::LiveUiHost> actual_ui){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied&&actual_ui,"GameplayTurnChanged actual source/cursor/UI absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,
  "GameplayTurnChanged canonical cursor allocation differs");bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  bound->game_turn_event_engine=actual_ui->engine_ui_turn_owner_; // Actual weak receiver before native effect.
  cursor->execute_game_turn_gameplay_with_actual_stock(*actual_ui,*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "GameplayTurnChanged actual source expectation adoption differs");return cursor->view();
 });
}
}

namespace civ5::runtime {
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_end_timer_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"EndTurnTimer actual stock source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,"EndTurnTimer canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_end_timer_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "EndTurnTimer runtime-only reset changed adopted world/turn");return cursor->view();
 });
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_elapsed_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"incrementElapsedGameTurns pure actual source/control adoption retired");
 cursor.require_game_turn_elapsed_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_elapsed_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"incrementElapsedGameTurns actual stock source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,"incrementElapsedGameTurns canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_elapsed_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "incrementElapsedGameTurns actual source expectations were not adopted");return cursor->view();
 });
}

}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_activation_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"Dynamic turn activation prefix pure actual source/control adoption retired");
 cursor.require_game_turn_activation_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_activation_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Dynamic turn activation prefix actual stock source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,"Dynamic turn activation prefix canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 std::uint64_t final_before=0,final_serial=0;
 try{
 const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  const auto before=bound->operation_world_revision;final_before=before;final_serial=serial;
  cursor->execute_game_turn_activation_prefix_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "Dynamic turn activation prefix actual source expectations were not adopted");
  cursor->require_game_turn_activation_store_return(*bound->selected,*bound->state,bound->generation,before,serial,*canonical,true);
  return cursor->view();
 });
 // run_operation itself performs callbackful VM/source bookends. Seal the
 // actual native operands again after that outer closure, without a new lease,
 // getter, revision callback, mode store or fabricated return.
 cursor->require_game_turn_activation_store_return(*bound->selected,*bound->state,bound->generation,final_before,final_serial,*canonical,true);
 return result;
 }catch(...){
  if(final_serial){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();}
  throw;
 }
}

}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_shuffle_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"Turn shuffle prefix pure actual source/control adoption retired");
 cursor.require_game_turn_shuffle_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_shuffle_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Turn shuffle prefix actual stock source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,"Turn shuffle prefix canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 std::uint64_t final_before=0,final_serial=0;
 try{
 const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  const auto before=bound->operation_world_revision;final_before=before;final_serial=serial;
  cursor->execute_game_turn_shuffle_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "Turn shuffle prefix actual source expectations were not adopted");
  cursor->require_game_turn_shuffle_store_return(*bound->selected,*bound->state,bound->generation,before,serial,*canonical,true);
  return cursor->view();
 });
 // run_operation itself performs callbackful VM/source bookends. Seal the
 // actual native operands again after that outer closure, without a new lease,
 // getter, revision callback, mode store or fabricated return.
 cursor->require_game_turn_shuffle_store_return(*bound->selected,*bound->state,bound->generation,final_before,final_serial,*canonical,true);
 return result;
 }catch(...){
  if(final_serial){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();}
  throw;
 }
}

}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_player_active_return(
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();
 // Pure controlled adoption: do NOT invoke a pre-store currentness provider.
 const auto canonical=action_self_.lock();const auto bound=bootstrap_;const auto control=lease.control_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical)&&
  bound&&bound->thread==thread&&control&&bound->operation_control&&
  !control.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(control)&&
  !control->source_owner.owner_before(bound)&&!bound.owner_before(control->source_owner)&&
  !bound->poisoned.load()&&!control->invalidated.load()&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&control->serial!=0&&control->vm==bound->vm.get()&&
  control->world.get()==bound->state.get(),"Player turn-active prefix pure actual source/control adoption retired");
 cursor.require_game_turn_player_active_store_return(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,control->serial,*canonical);
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_active_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Player turn-active prefix actual stock source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor&&cursor==supplied,"Player turn-active prefix canonical cursor allocation differs");
 bound->check();cursor->require_engine_host(*bound->selected);
 std::uint64_t final_before=0,final_serial=0;
 try{
 const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  const auto before=bound->operation_world_revision;final_before=before;final_serial=serial;
  cursor->execute_game_turn_player_active_with_actual_stock(*canonical,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  production_need(bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
   "Player turn-active prefix actual source expectations were not adopted");
  cursor->require_game_turn_player_active_store_return(*bound->selected,*bound->state,bound->generation,before,serial,*canonical,true);
  return cursor->view();
 });
 // run_operation itself performs callbackful VM/source bookends. Seal the
 // actual native operands again after that outer closure, without a new lease,
 // getter, revision callback, mode store or fabricated return.
 cursor->require_game_turn_player_active_store_return(*bound->selected,*bound->state,bound->generation,final_before,final_serial,*canonical,true);
 return result;
 }catch(...){
  if(final_serial){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();}
  throw;
 }
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::with_worldview_renderer_context_owned(
 const ui::BnwOwnedCameraGuiScreen& gui,const ui::BnwWorldViewGridToWorldLoan& grid,
 const std::function<void(const BnwWorldViewSourceOperationLoan&,
  const ui::BnwOwnedCameraGuiScreen::Loan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
  const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>& borrowed){
 with_worldview_native_dispatch_owned(gui,grid,&borrowed);
}
void BnwSelectedStockLuaSourceOwner::with_worldview_native_dispatch_owned(
 const ui::BnwOwnedCameraGuiScreen& gui,const ui::BnwWorldViewGridToWorldLoan& grid,
 const std::function<void(const BnwWorldViewSourceOperationLoan&,
  const ui::BnwOwnedCameraGuiScreen::Loan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
  const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>* borrowed){
 const auto thread=stock_thread_;thread->check();
#ifdef _WIN32
 if(!borrowed){
  const auto queue=worldview_input_queue_;
  production_need(queue&&queue->creator==std::this_thread::get_id()&&queue->alive&&!queue->failed&&
   !queue->active&&!queue->shutdown_returned&&!queue->rows.empty()&&queue->rows.back().native->message_==WM_CLOSE,
   "WorldView teardown lacks actual queued native close occurrence");
  queue->rows.back().native->require_completed();
 }else if(worldview_input_queue_&&!worldview_input_queue_->rows.empty())
  production_need(worldview_input_queue_->rows.back().native->message_!=WM_CLOSE,
   "Actual queued close requires teardown entry before renderer/window retirement");
#endif
 const auto canonical=retain_action_factory_owner();
 const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->operation_control&&bound->vm,"WorldView renderer actual VM absent");
 struct Borrow final {
  const std::thread::id creator=std::this_thread::get_id();
  BnwSelectedStockLuaSourceOwner& facade;
  const std::shared_ptr<BnwStockLuaAttemptLease::Control> control;
  bool facade_counted=false,source_counted=false;
  Borrow(BnwSelectedStockLuaSourceOwner& f,std::shared_ptr<BnwStockLuaAttemptLease::Control> c)
   :facade(f),control(std::move(c)){
   const auto f_old=facade.gameplay_pump_facade_pins_.fetch_add(1);
   if(f_old==std::numeric_limits<std::size_t>::max()){
    facade.gameplay_pump_facade_pins_.fetch_sub(1);throw std::runtime_error("WorldView facade pins exhausted");}
   facade_counted=true;
   const auto s_old=control->transport_source_pins.fetch_add(1);
   if(s_old==std::numeric_limits<std::uint64_t>::max()){
    control->transport_source_pins.fetch_sub(1);facade.gameplay_pump_facade_pins_.fetch_sub(1);
    facade_counted=false;throw std::runtime_error("WorldView source pins exhausted");}
   source_counted=true;
  }
  ~Borrow(){if(creator!=std::this_thread::get_id())std::terminate();
   if(source_counted)control->transport_source_pins.fetch_sub(1);
   if(facade_counted)facade.gameplay_pump_facade_pins_.fetch_sub(1);}
 } borrower(*canonical,bound->operation_control);
 const auto outside=[&]{
  thread->check();
  production_need(canonical->action_self_.lock().get()==canonical.get()&&
   canonical->bootstrap_.get()==bound.get()&&!bound->poisoned.load()&&
   !bound->operation_control->invalidated.load(),"WorldView actual facade/source retired");
  const auto fp=canonical->gameplay_pump_facade_pins_.load();
  const auto sp=bound->operation_control->transport_source_pins.load();
  production_need(fp<=static_cast<std::size_t>(std::numeric_limits<long>::max())&&
    canonical.use_count()>static_cast<long>(fp),"WorldView facade outside owner retired");
  production_need(sp<=static_cast<std::uint64_t>(std::numeric_limits<long>::max())&&
    bound.use_count()>static_cast<long>(sp),"WorldView source outside owner retired");
 };
 try{
  outside();
  // run_operation retains the actual pending Policy/TRAIN/research restrictions.
  bound->run_operation(BnwStockLuaOperationKind::worldview_render,[&]{
   bound->check_active();outside();
   bound->vm->activate_worldview_context_owned(gui,grid);outside();bound->check_active();
   bound->vm->with_worldview_context_owned(gui,grid,[&](const auto& loan,const auto& gui_loan,const auto& window_loan,const auto& trees){
    const auto serial=loan.operation_serial();outside();bound->check_active();
#ifdef _WIN32
    canonical->consume_worldview_native_input_owned(loan,window_loan,!borrowed);
    loan.require_operation(serial);outside();bound->check_active();
#endif
    if(borrowed){
     std::optional<std::function<void(const BnwWorldViewSourceOperationLoan&,
       const ui::BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
       const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>> target;
     try{
      loan.require_operation(serial);outside();bound->check_active();
      target.emplace(*borrowed);loan.require_operation(serial);outside();bound->check_active();
      production_need(static_cast<bool>(*target),"WorldView actual paint target absent");
      (*target)(loan,gui_loan,window_loan,trees);loan.require_operation(serial);outside();bound->check_active();
      target.reset();loan.require_operation(serial);outside();bound->check_active();
     }catch(...){target.reset();throw;}
    }
   });
   outside();bound->check_active();return 0;
  });outside();
 }catch(...){bound->quarantine();throw;}
}
}

namespace civ5::runtime {
BnwSelectedStockLuaSourceOwner::BnwSelectedStockLuaSourceOwner(
 std::shared_ptr<const BnwCivisOwnedStockSourceBundle> source)
 :civis_sources_(std::move(source)),game_root_(civis_sources_->recipe().canonical_payload_root),
  catalog_(civis_sources_->recipe().catalog),selected_(civis_sources_->recipe().profile),
  identity_(civis_sources_->retain_host()->identity()),stock_thread_(std::make_shared<StockThreadLease>()){
 stock_thread_->check();civis_sources_->require_current();
 if(!selected_.can_activate||!selected_.all_requirements_located||!selected_.enabled_mods.empty())
  throw std::runtime_error("Own stock selected profile cannot activate");
 const auto declaration=civis_sources_->loaded("Assets/DLC/Expansion2/UI/InGame/InGame.xml");
 if(!declaration.xml||declaration.xml->name!="Context")throw std::runtime_error("Own stock InGame declaration absent");
 visit_contexts(*declaration.xml,evidence_);
 if(evidence_.first_context_id!="CivilopediaScreen"||evidence_.declared_in_game_contexts<2)
  throw std::runtime_error("Own stock first context hierarchy differs");
 const auto parent=civis_sources_->text("Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",&evidence_.first_source_sha256);
 std::istringstream lines(parent);std::string line,first;
 while(std::getline(lines,line)){const auto at=line.find_first_not_of(" \t\r");
  if(at==std::string::npos||line.compare(at,2,"--")==0)continue;
  first=line.substr(at,line.find_last_not_of(" \t\r")-at+1);break;}
 if(first!="include( \"InstanceManager\" );"&&first!="include( 'InstanceManager' );")
  throw std::runtime_error("Own stock actual first include differs");
 const auto include=civis_sources_->text("Assets/UI/InstanceManager.lua",&evidence_.first_include_sha256);
 LuaScriptHost actual_include({8ULL*1024*1024,1'000'000});
 actual_include.execute("CivilopediaScreen",include,"@Assets/UI/InstanceManager.lua");
 civis_sources_->require_current();
 actual_include.execute("CivilopediaScreen","assert(type(InstanceManager)=='table' and type(InstanceManager.new)=='function')","=own-stock-first-include-shape");
 civis_sources_->require_current();evidence_.declaration_sha256=declaration.payload_identity.sha256;
 evidence_.selected_profile=true;evidence_.first_include_executed=true;
 evidence_.blocker="Actual staged first include executed; retained full parent/WorldView contexts remain separately activated";
}
std::shared_ptr<BnwSelectedStockLuaSourceOwner> BnwSelectedStockLuaSourceOwner::create_selected_from_civis_resources(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> supplied,
 std::shared_ptr<const ui::CivisResourceBankUiSnapshot> ui,
 std::shared_ptr<const ui::CivisResourceBankFontColorSnapshot> font){
 if(!supplied||!ui||!font)throw std::runtime_error("Own stock factory actual Host/UI/font sources absent");
 const auto host=supplied->retain_civis_resource_host_owned();
 if(host.get()!=supplied.get()||host.owner_before(supplied)||supplied.owner_before(host))
  throw std::runtime_error("Own stock factory Host canonical get/control differs");
 const auto bank=host->civis_resource_bank_->retain_owner();
 if(bank.get()!=host->civis_resource_bank_.get()||bank.owner_before(host->civis_resource_bank_)||host->civis_resource_bank_.owner_before(bank))
  throw std::runtime_error("Own stock Host bank canonical get/control differs");
 const auto actual_ui=ui->retain_owner();const auto actual_font=font->retain_owner();
 if(actual_ui.get()!=ui.get()||actual_ui.owner_before(ui)||ui.owner_before(actual_ui)||
    actual_font.get()!=font.get()||actual_font.owner_before(font)||font.owner_before(actual_font))
  throw std::runtime_error("Own stock UI/font canonical get/control differs");
 host->require_same_civis_resources(*bank);actual_ui->require_same_bank(*bank);
 actual_font->require_same_bank(*bank);actual_font->require_same_ui(*actual_ui);
 const auto source=std::shared_ptr<const BnwCivisOwnedStockSourceBundle>(
  new BnwCivisOwnedStockSourceBundle(host,bank,actual_ui,actual_font));
 auto result=std::shared_ptr<BnwSelectedStockLuaSourceOwner>(new BnwSelectedStockLuaSourceOwner(source));
 result->action_self_=result;source->require_boundary();return result;
}
BnwStockLuaBootstrapProgress BnwSelectedStockLuaSourceOwner::start_stock_bootstrap_from_civis_resources(){
 const auto thread=stock_thread_;thread->check();const auto canonical=retain_action_factory_owner();
 if(!canonical->civis_sources_)throw std::runtime_error("Own stock source factory lineage absent");
 const auto source=canonical->civis_sources_;source->require_current();
 const auto host=source->retain_host();const auto root=source->recipe().canonical_payload_root;
 return canonical->start_stock_bootstrap(host,root);
}
}

#ifdef _WIN32
namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_worldview_input_origin_without_callbacks()const {
 stock_thread_->check();const auto queue=worldview_input_queue_;
 production_need(queue&&queue->creator==std::this_thread::get_id()&&queue->alive&&!queue->failed&&
  !queue->shutdown_returned&&bootstrap_&&bootstrap_->operation_control&&!bootstrap_->poisoned.load()&&
  !bootstrap_->operation_control->invalidated.load(),"WorldView input source/queue retired");
 const std::weak_ptr<const void> source=bootstrap_;
 const std::weak_ptr<const BnwStockLuaAttemptLease::Control> control=bootstrap_->operation_control;
 production_need(bootstrap_.get()==queue->source_get&&bootstrap_->operation_control.get()==queue->control_get&&
  !source.owner_before(queue->source)&&!queue->source.owner_before(source)&&!source.expired()&&
  !control.owner_before(queue->control)&&!queue->control.owner_before(control)&&!control.expired(),
  "WorldView input canonical Bootstrap/control replaced");
}
void BnwSelectedStockLuaSourceOwner::capture_worldview_native_input_owned(
 std::shared_ptr<const ui::BnwWorldViewWindowInputLoan> native){
 stock_thread_->check();
 production_need(static_cast<bool>(native),"WorldView native input source missing");native->require_entered();
 const auto issued=native->self_.lock();
 production_need(issued&&issued.get()==native.get()&&!issued.owner_before(native)&&!native.owner_before(issued),
  "WorldView native input control alias differs");
 const auto canonical=action_self_.lock();
 production_need(canonical&&canonical.get()==this&&!canonical.owner_before(action_self_)&&!action_self_.owner_before(canonical),
  "WorldView input canonical Source facade retired");
 const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control&&bound->state&&bound->selected&&!bound->poisoned.load()&&
  !bound->operation_control->invalidated.load(),"WorldView input has no genuine retained Bootstrap/world");
 //The actual native occurrence can precede deferred queue drain. Close
 //constructor-only hover reads BEFORE any queue allocation or callbacks.
 production_need(bound->vm!=nullptr,"WorldView input capture actual Bootstrap VM absent");
 bound->vm->note_worldview_native_input_entry_owned();
 if(!worldview_input_queue_){
  worldview_input_queue_=std::make_shared<WorldViewInputQueue>();
  worldview_input_queue_->source=bound;worldview_input_queue_->control=bound->operation_control;
  worldview_input_queue_->source_get=bound.get();worldview_input_queue_->control_get=bound->operation_control.get();
 }
 const auto queue=worldview_input_queue_;require_worldview_input_origin_without_callbacks();
 if(queue->active){queue->failed=true;throw std::runtime_error("WorldView input source capture reentered Lua delivery");}
 try{
  if(!queue->rows.empty())production_need(queue->rows.back().native->state_.get()==native->state_.get()&&
    queue->rows.back().native->sequence_<native->sequence_,"WorldView native input binding/sequence substituted");
  const auto generation=bound->selected->session().state_generation();
  production_need(generation!=0,"WorldView native input world generation absent");
  queue->rows.push_back({native,bound->state,bound->state.get(),generation,bound->state->ui_source_revision});
  native->capture();require_worldview_input_origin_without_callbacks();native->require_entered();
 }catch(...){queue->failed=true;throw;}
}
void BnwSelectedStockLuaSourceOwner::consume_worldview_native_input_owned(
 const BnwWorldViewSourceOperationLoan& loan,const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,bool closing){
 const auto queue=worldview_input_queue_;if(!queue){production_need(!closing,"WorldView close queue absent");return;}
 require_worldview_input_origin_without_callbacks();
 if(queue->active){queue->failed=true;throw std::runtime_error("WorldView native input drain reentered");}
 queue->active=true;
 struct End{std::shared_ptr<WorldViewInputQueue> queue;~End(){queue->active=false;}} end{queue};
 const auto serial=loan.operation_serial();
 const auto current=[&]{require_worldview_input_origin_without_callbacks();loan.require_operation(serial);
  require_worldview_input_origin_without_callbacks();};
 try{
  current();
  bool closed=false;
  while(!queue->rows.empty()){
   auto row=std::move(queue->rows.front());queue->rows.pop_front();
   current();
   production_need(row.world_get==bootstrap_->state.get()&&!row.world.owner_before(bootstrap_->state)&&
    !bootstrap_->state.owner_before(row.world)&&!row.world.expired()&&
    row.generation==bootstrap_->selected->session().state_generation()&&row.revision==bootstrap_->state->ui_source_revision,
    "WorldView queued native input world/generation/revision changed");
   row.native->claim_for(window);current();row.native->require_claimed_for(window);
   if(row.native->message_==WM_CLOSE){
    production_need(closing&&queue->rows.empty(),"WorldView close occurrence is not final actual teardown stage");
    bootstrap_->vm->dispatch_worldview_shutdown_owned(loan,window,*row.native);
    current();row.native->require_claimed_for(window);closed=true;
   }else{
    const bool actual_consumed=bootstrap_->vm->dispatch_worldview_input_owned(loan,window,*row.native);
    (void)actual_consumed; // Actual Lua result retained locally; no synchronous native ACK is invented.
    current();row.native->require_claimed_for(window);
   }
  }
  production_need(closing==closed,"WorldView teardown has no returned actual close callback");
  current();if(closed)queue->shutdown_returned=true;
 }catch(...){queue->failed=true;throw;}
}
void BnwSelectedStockLuaSourceOwner::shutdown_worldview_renderer_context_owned(
 const ui::BnwOwnedCameraGuiScreen& gui,const ui::BnwWorldViewGridToWorldLoan& grid){
 stock_thread_->check();const auto queue=worldview_input_queue_;
 production_need(queue&&queue->alive&&!queue->failed&&!queue->active&&!queue->shutdown_returned&&
   !queue->rows.empty()&&queue->rows.back().native->message_==WM_CLOSE,"WorldView actual close occurrence absent");
 const auto close=queue->rows.back().native;
 with_worldview_native_dispatch_owned(gui,grid,nullptr);
 close->require_source();
 production_need(queue->alive&&!queue->failed&&queue->shutdown_returned&&!queue->active&&queue->rows.empty()&&
   close->claimed_&&close->dispatch_completed_&&!close->state_->active_sequence&&
   close->state_->sequence==close->claim_boundary_,"WorldView close/source shutdown never returned normally");
 close->shutdown_completed_=true;close->require_shutdown_returned();
}
}
#endif

namespace civ5::runtime {
std::optional<std::int64_t> BnwSelectedStockLuaSourceOwner::query_treasury_founder_benefits(std::int64_t actor,const LiveSessionState& expected_world){
 const auto thread=stock_thread_;thread->check();
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&actor>=0&&actor<64,"Treasury actual selected VM/actor absent");
 bound->check();
 production_need(bound->state.get()==&expected_world,"Treasury query expected world is not the canonical selected session");
 return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();
  BnwTreasurySameVmIncomeLoan income(*bound->vm,lease,bound->state,actor);
  const auto result=income.founder_benefits_read_only(actor,*bound->state);
  bound->check_active();lease.require_operation(serial);
  return result;
 });
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwLivePlayerGoldPending> BnwSelectedStockLuaSourceOwner::begin_imported_player_gold_component(std::int64_t actor,std::shared_ptr<const ui::LiveUiHost> actual_ui,std::shared_ptr<const BnwNativeUnitAllocatorCore> actual_deficit_core){
 const auto thread=stock_thread_;thread->check();
 const bool deficit_core_supplied=bool(actual_deficit_core);
 const std::weak_ptr<const BnwNativeUnitAllocatorCore> weak_deficit_core=actual_deficit_core;
 actual_deficit_core.reset(); // Incoming temporary retention cannot attest an outside Core owner.
 const auto canonical=retain_action_factory_owner();const auto bound=canonical->bootstrap_;
 production_need(bound&&bound->vm&&bound->selected&&bound->selected->session_&&actor>=0&&actor<64,
  "Live Gold component actual source/player absent");bound->check();
 auto* const session=bound->selected->session_.get();
 production_need(session->source().source.container==saves::SaveContainerKind::civ5&&
  session->bnw_game_debug_owner_current()&&session->state_.get()==bound->state.get(),
  "Live Gold component requires genuine original-import physical session");
 const auto key=std::pair{actor,bound->state->setup.game_turn};
 production_need(!bound->live_gold_components.contains(key),"Live Gold component replay for same actor/turn refused");
 std::shared_ptr<const BnwLivePlayerGoldPending> issued;
 try{
  return bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
   bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
   const auto facade=session->retain_bnw_facade_lifetime();facade->require_current();
   const auto generation=session->state_generation();
   auto [row,inserted]=bound->live_gold_components.emplace(key,std::shared_ptr<const BnwLivePlayerGoldPending>{});
   production_need(inserted,"Live Gold component registry already reserved");auto* const slot=&row->second;
   BnwLivePlayerGoldPending::Binding binding(lease);
   binding.world=session->state_;binding.session=session;binding.host=bound->selected;
   binding.source=canonical;binding.facade_storage=facade;binding.source_registry_slot=slot;
   binding.require_facade=[facade]{facade->require_current();};
   const std::weak_ptr<StockBootstrap> weak_bound=bound;
   const std::weak_ptr<const BnwSelectedStockLuaSourceOwner> weak_source=canonical;
   binding.fresh_income=[thread,facade,weak_bound,lease,serial,actor]{
    thread->check();facade->require_current();lease.require_operation(serial);const auto actual=weak_bound.lock();
    production_need(actual&&actual->vm,"Live Gold real income VM retired");actual->check_active();
    return std::unique_ptr<BnwTreasurySameVmIncomeLoan>(new BnwTreasurySameVmIncomeLoan(*actual->vm,lease,actual->state,actor));
   };
   binding.prepare_revision=[thread,facade,weak_bound,weak_source,lease,serial,session,generation,key,slot](
    std::uint64_t old_revision,std::uint64_t new_revision)->std::shared_ptr<void>{
    // PURE store preparation: no source currentness provider/Lua/GUI callback.
    // This interval sees changed owned values; mirror148 validates raw targets.
    thread->check();facade->require_current();const auto actual=weak_bound.lock();const auto source=weak_source.lock();
    const auto control=lease.control_.lock();
    production_need(actual&&source&&source->bootstrap_.get()==actual.get()&&control&&
     actual->operation_control&&control.get()==actual->operation_control.get()&&
     !control.owner_before(actual->operation_control)&&!actual->operation_control.owner_before(control)&&
     !control->source_owner.owner_before(actual)&&!actual.owner_before(control->source_owner)&&
     control->creator==std::this_thread::get_id()&&control->serial==serial&&control->vm==actual->vm.get()&&
     !control->invalidated.load()&&!actual->poisoned.load()&&actual->running&&
     actual->operation_state==BnwStockLuaOperationState::active&&control->world.get()==actual->state.get()&&
     session->state_.get()==actual->state.get()&&session->state_generation()==generation&&
     actual->generation==generation&&actual->state->setup.game_turn==key.second&&
     actual->operation_world_revision==old_revision&&actual->state->ui_source_revision==old_revision&&
     old_revision<std::numeric_limits<std::uint64_t>::max()&&new_revision==old_revision+1,
     "Live Gold pure actual source/world/revision preparation failed");
    const auto at=actual->live_gold_components.find(key);
    production_need(at!=actual->live_gold_components.end()&& &at->second==slot&&*slot&&
     (*slot)->view().actor==key.first&&(*slot)->view().turn==key.second&&
     ((*slot)->view().phase==BnwLivePlayerGoldPhase::executing||
      ((*slot)->view().phase==BnwLivePlayerGoldPhase::deficit_random_executing&&
       (*slot)->view().deficit_scan_returned&&!(*slot)->view().deficit_no_disband_returned&&!(*slot)->view().deficit_rng_stored)),
     "Live Gold actual closed Pending registry/phase differs");
    return actual; // exact temporary strong scope, only across no-callback stores
   };
   if(actual_ui){
    const std::weak_ptr<const ui::LiveUiHost> weak_ui=actual_ui;
    binding.actual_dirty=[thread,facade,weak_bound,weak_ui,lease,serial](
     const std::shared_ptr<const BnwLivePlayerGoldPending>& pending,TreasuryDirty kind,const BnwStockLuaAttemptLease& reserved){
     // Pending reserved this precise original native call before this first
     // callbackful retained receiver. No supplied bool or deferred ACK.
     thread->check();facade->require_current();lease.require_operation(serial);reserved.require_operation(serial);
     const auto source=weak_bound.lock();const auto receiving_ui=weak_ui.lock();
     production_need(source&&source->selected&&receiving_ui,"Live Gold actual UI/Source receiver retired");source->check_active();
     const auto receiver=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(receiving_ui,source->selected);
     source->check_active();lease.require_operation(serial);
     receiver->consume_live_gold_dirty(*source->selected->session_->state_,pending,kind,reserved);
     source->check_active();lease.require_operation(serial);reserved.require_operation(serial);
    };
   }
   if(deficit_core_supplied){
    binding.deficit_core=weak_deficit_core;const auto weak_core=weak_deficit_core;
    binding.actual_deficit=[thread,facade,weak_bound,weak_source,weak_core,lease,serial,actor,generation](
     const std::shared_ptr<const BnwLivePlayerGoldPending>& pending,const BnwStockLuaAttemptLease& reserved){
     thread->check();facade->require_current();lease.require_operation(serial);reserved.require_operation(serial);
     const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
     const auto& core=core_pin->current();
     const auto source=weak_bound.lock();const auto issuer=weak_source.lock();
     production_need(source&&issuer&&source->selected&&source->selected->session_&&
      issuer->bootstrap_.get()==source.get(),"Live Gold deficit actual Core/Source supplier retired");
     source->check_active();const auto& world=*source->state;
     pending->require_deficit_source(*source->selected,world,reserved);
     StockBootstrap::LiveGoldDeficitReturn r;r.pending=pending;r.core=weak_core;r.world=source->state;r.source=issuer;
     r.actor=actor;r.turn=world.setup.game_turn;r.serial=serial;r.generation=generation;r.revision=world.ui_source_revision;
     r.map_rng=world.random.map.state();r.game_rng=world.random.gameplay.state();
     // SDK firstUnit/nextUnit + IsCombatUnit from the SAME genuine actor Bank,
     // then getNumCities. Detached counts/flags never issue this record.
     core.require_live_gold_roster_source(*source->selected,world,static_cast<std::int32_t>(actor));
     r.combat_rows=StockBootstrap::capture_live_gold_combat_rows(world,actor);
     r.combat_count=core.read_native_existing_combat_unit_count();r.unit_query_returned=true;
     pending->require_deficit_source(*source->selected,world,reserved);
     (void)resolve_bnw_city_registry(world.world,world.cities,static_cast<std::int32_t>(actor),0);
     const auto& cities=world.world.all_players->players.at(static_cast<std::size_t>(actor)).cities;
     production_need(cities.cities.size()<=static_cast<std::size_t>(INT32_MAX),"Live Gold native city GetCount overflow");
     r.city_count=static_cast<std::int32_t>(cities.cities.size());r.city_query_returned=true;
     core.require_live_gold_roster_source(*source->selected,world,static_cast<std::int32_t>(actor));
     pending->require_deficit_source(*source->selected,world,reserved);
     source->check_active();lease.require_operation(serial);reserved.require_operation(serial);
     // Final pure receiver inventory follows every callbackful source provider.
     core.require_live_gold_roster_source_without_callbacks(*source->selected,world,static_cast<std::int32_t>(actor));
     (void)resolve_bnw_city_registry(world.world,world.cities,static_cast<std::int32_t>(actor),0);
     production_need(world.world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities.size()==
      static_cast<std::size_t>(r.city_count)&&world.ui_source_revision==r.revision&&world.setup.game_turn==r.turn&&
      world.random.map.state()==r.map_rng&&world.random.gameplay.state()==r.game_rng,
      "Live Gold deficit observational unit/city scan changed world/clock/RNG");
     StockBootstrap::require_live_gold_combat_rows_without_callbacks(world,actor,r.combat_rows,r.combat_count);
     pending->require_deficit_identity_without_callbacks(*source->selected,world,weak_core,serial);
     // No world mutation, provider, RNG or allocation in receipt publication.
     source->live_gold_deficit_return=std::move(r);
     pending->accept_actual_deficit_scan_return(*issuer,core);
     source->check_active();lease.require_operation(serial);reserved.require_operation(serial);
     (void)core_pin->current();
     core.require_live_gold_roster_source_without_callbacks(*source->selected,world,static_cast<std::int32_t>(actor));
     const auto& actual_return=*source->live_gold_deficit_return;
     StockBootstrap::require_live_gold_combat_rows_without_callbacks(world,actor,actual_return.combat_rows,actual_return.combat_count);
     if(pending->view().phase==BnwLivePlayerGoldPhase::deficit_random_owner_required){
      // Original getJonRandNum(100,"Disband rand") follows the returned count
      // and city comparison. Reserve BEFORE acquiring canonical Game providers.
      pending->begin_actual_deficit_random(*source->selected,world,reserved);
      const auto context=BnwLiveGameContext::retain_for_actual_host(source->selected);
      const auto game=context->get_game();game->require_actual_binding(*source->selected);
      source->check_active();lease.require_operation(serial);reserved.require_operation(serial);
      core.require_live_gold_roster_source_without_callbacks(*source->selected,world,static_cast<std::int32_t>(actor));
      StockBootstrap::require_live_gold_combat_rows_without_callbacks(world,actor,actual_return.combat_rows,actual_return.combat_count);
      game->draw_actual_deficit_random100(*source->selected->session_->state_,pending,serial,*issuer,core);
      pending->accept_actual_deficit_rng_return(*game);
      (void)game->require_actual_deficit_random_return(*pending);
      game->destroy();game->require_destroyed_for(*source->selected);
      source->check_active();lease.require_operation(serial);reserved.require_operation(serial);
      (void)core_pin->current();
      (void)game->require_actual_deficit_random_return(*pending,true);
      issuer->require_live_gold_deficit_random_operands_without_callbacks(*pending,core);
     }
    };
   }
   binding.commit_revision=+[](void* scope,std::uint64_t actual_revision)noexcept{
    // SAME prepared scope pointer, issued by the private method above.
    static_cast<StockBootstrap*>(scope)->operation_world_revision=actual_revision;
   };
   binding.treasury_rules=load_treasury_rules(bound->selected->database(),content::TechnologyProfile::bnw,bound->selected->types());
   binding.city_rules=load_city_yield_rules(bound->selected->database(),content::TechnologyProfile::bnw);
   load_city_building_replacements(bound->selected->database(),binding.city_rules);
   binding.maintenance_rules=load_maintenance_rules(bound->selected->database(),content::TechnologyProfile::bnw,bound->selected->types());
   bound->check_active();lease.require_operation(serial);
   if(deficit_core_supplied){
    // Qualify supplied actor/facade before the first native Gold write. The
    // query itself still runs later at the original DoDeficit source position.
    const auto preflight=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_deficit_core);
    const auto& core=preflight->current();
    core.require_live_gold_roster_source(*bound->selected,*bound->state,static_cast<std::int32_t>(actor));
    bound->check_active();lease.require_operation(serial);(void)preflight->current();
    core.require_live_gold_roster_source_without_callbacks(*bound->selected,*bound->state,static_cast<std::int32_t>(actor));
   }
   const auto returned=BnwLivePlayerGoldPending::execute_owned(std::move(binding),actor,issued);
   bound->check_active();lease.require_operation(serial);
   production_need(row->second==returned&&returned->view().phase==BnwLivePlayerGoldPhase::native_returned,
    "Live Gold actual native return/retained receipt differs");
   return returned;
  });
 }catch(...){
  // run_operation quarantines genuine VM; no getter/provider in this catch.
  // Both first-write and late-bookend failures retain the actual issued owner.
  if(issued)throw BnwLivePlayerGoldSourceFailure(issued,"Actual Live Gold source failed; retained receipt, no retry without checkpoint");
  throw;
 }
}
}

namespace civ5::runtime {
std::array<std::int32_t,2> BnwSelectedStockLuaSourceOwner::require_live_gold_deficit_scan_return(
 const BnwLivePlayerGoldPending& pending,const BnwNativeUnitAllocatorCore& core)const{
 const auto thread=stock_thread_;thread->check();const auto source=action_self_.lock();const auto bound=bootstrap_;
 production_need(source&&source.get()==this&&bound&&bound->selected&&bound->selected->session_&&bound->operation_control&&
  bound->live_gold_deficit_return,"Live Gold actual sealed deficit scan absent");
 const auto& r=*bound->live_gold_deficit_return;const auto actual=r.pending.lock();
 const auto issuer=r.source.lock();const auto world=r.world.lock();const auto& control=*bound->operation_control;
 const auto at=bound->live_gold_components.find(std::pair{r.actor,r.turn});
 production_need(actual&&actual.get()==&pending&&at!=bound->live_gold_components.end()&&at->second==actual&&!r.core.expired()&&!r.core.owner_before(core.factory_self_)&&
  !core.factory_self_.owner_before(r.core)&&issuer&&
  issuer.get()==this&&!issuer.owner_before(source)&&!source.owner_before(issuer)&&world&&world.get()==bound->state.get()&&
  !world.owner_before(bound->state)&&!bound->state.owner_before(world)&&!control.source_owner.owner_before(bound)&&!bound.owner_before(control.source_owner)&&
  !bound->poisoned.load()&&!control.invalidated.load()&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&
  control.creator==std::this_thread::get_id()&&control.serial==r.serial&&control.vm==bound->vm.get()&&
  control.world.get()==world.get()&&bound->generation==r.generation&&bound->selected->session_->state_.get()==world.get()&&
  bound->selected->session_->state_generation()==r.generation&&bound->operation_world_revision==r.revision&&
  world->ui_source_revision==r.revision&&world->setup.game_turn==r.turn&&world->random.map.state()==r.map_rng&&
  world->random.gameplay.state()==r.game_rng&&r.unit_query_returned&&r.city_query_returned&&r.combat_count>=0&&r.city_count>=0,
  "Live Gold SAME physical Source/Core/world original deficit scan return differs");
 core.require_live_gold_roster_source_without_callbacks(*bound->selected,*world,static_cast<std::int32_t>(r.actor));
 StockBootstrap::require_live_gold_combat_rows_without_callbacks(*world,r.actor,r.combat_rows,r.combat_count);
 pending.require_deficit_identity_without_callbacks(*bound->selected,*world,r.core,r.serial);
 return {r.combat_count,r.city_count};
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_live_gold_deficit_random_operands_without_callbacks(
 const BnwLivePlayerGoldPending& pending,const BnwNativeUnitAllocatorCore& core)const{
 const auto thread=stock_thread_;thread->check();const auto source=action_self_.lock();const auto bound=bootstrap_;
 production_need(source&&source.get()==this&&bound&&bound->selected&&bound->selected->session_&&
  bound->operation_control&&bound->live_gold_deficit_return,"Live Gold actual RNG operand issuer absent");
 const auto& r=*bound->live_gold_deficit_return;const auto actual=r.pending.lock();const auto issuer=r.source.lock();
 const auto world=r.world.lock();const auto& control=*bound->operation_control;const auto view=pending.view();
 const auto at=bound->live_gold_components.find(std::pair{r.actor,r.turn});
 production_need(actual&&actual.get()==&pending&&at!=bound->live_gold_components.end()&&at->second==actual&&issuer&&
  issuer.get()==this&&!issuer.owner_before(source)&&!source.owner_before(issuer)&&!r.core.expired()&&
  !r.core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(r.core)&&world&&
  world.get()==bound->state.get()&&!world.owner_before(bound->state)&&!bound->state.owner_before(world)&&
  !control.source_owner.owner_before(bound)&&!bound.owner_before(control.source_owner)&&!bound->poisoned.load()&&
  !control.invalidated.load()&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&
  control.creator==std::this_thread::get_id()&&control.serial==r.serial&&control.vm==bound->vm.get()&&
  control.world.get()==world.get()&&bound->generation==r.generation&&bound->selected->session_->state_.get()==world.get()&&
  bound->selected->session_->state_generation()==r.generation&&bound->operation_world_revision==world->ui_source_revision&&
  world->setup.game_turn==r.turn&&world->random.map.state()==r.map_rng&&r.unit_query_returned&&r.city_query_returned&&
  r.combat_count>r.city_count&&view.actor==r.actor&&view.turn==r.turn&&view.deficit_scan_returned&&
  view.deficit_combat_units==r.combat_count&&view.deficit_cities==r.city_count&&
  ((!view.deficit_rng_stored&&world->ui_source_revision==r.revision&&world->random.gameplay.state()==r.game_rng)||
   (view.deficit_rng_stored&&r.revision<UINT64_MAX&&world->ui_source_revision==r.revision+1&&
    view.deficit_rng_before==r.game_rng&&world->random.gameplay.state()==view.deficit_rng_after)),
  "Live Gold RNG SAME source/query history or precise one-draw lineage differs");
 core.require_live_gold_roster_source_without_callbacks(*bound->selected,*world,static_cast<std::int32_t>(r.actor));
 StockBootstrap::require_live_gold_combat_rows_without_callbacks(*world,r.actor,r.combat_rows,r.combat_count);
 (void)resolve_bnw_city_registry(world->world,world->cities,static_cast<std::int32_t>(r.actor),0);
 production_need(world->world.all_players->players.at(static_cast<std::size_t>(r.actor)).cities.cities.size()==
  static_cast<std::size_t>(r.city_count),"Live Gold RNG actual canonical city count changed after providers");
 pending.require_deficit_rng_identity_without_callbacks(*bound->selected,*world,r.serial);
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer> BnwSelectedStockLuaSourceOwner::issue_player_turn_dirty_owned(
 const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();const auto self=action_self_.lock();const auto bound=bootstrap_;const auto c=lease.control_.lock();
 production_need(self&&self.get()==this&&bound&&c&&bound->operation_control==c&&bound->running&&
  bound->operation_state==BnwStockLuaOperationState::active&&!bound->poisoned.load()&&!c->invalidated.load()&&c->serial&&cursor,
  "Player dirty Source factory lacks actual active operation");
 return BnwPlayerTurnActiveDirtyIssuer::create_owned(self,cursor,bound,c,bound->state,bound->generation,c->serial);
}
void BnwSelectedStockLuaSourceOwner::require_player_turn_dirty_source_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer& issuer,
 const BnwRetainedTurnCursor& cursor,const BnwStockLuaAttemptLease& lease,bool publisher)const{
 const auto thread=stock_thread_;thread->check();const auto self=action_self_.lock();const auto bound=bootstrap_;const auto c=lease.control_.lock();
 const auto selected=c?c->selected_host.lock():nullptr;
 production_need(self&&self.get()==this&&!self.owner_before(action_self_)&&!action_self_.owner_before(self)&&bound&&bound->thread==thread&&
  c&&c==bound->operation_control&&!c.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(c)&&
  !c->source_owner.owner_before(bound)&&!bound.owner_before(c->source_owner)&&
  !bound->poisoned.load()&&!c->invalidated.load()&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active&&
  c->serial&&c->vm==bound->vm.get()&&c->world.get()==bound->state.get()&&selected&&selected==bound->selected&&
  selected->session().read_state_lease().get()==bound->state.get()&&selected->session().state_generation()==bound->generation&&
  bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
  "Player dirty pure active Source/control/VM expectations changed");
 issuer.require_source_lineage_without_callbacks(*this,cursor,bound.get(),c.get(),*bound->state,bound->generation,c->serial);
 cursor.require_player_turn_dirty_operation_without_callbacks(issuer,*this,c->serial,publisher);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_dirty_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Player dirty actual source/cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Player dirty canonical cursor substituted");bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_game_turn_player_dirty_with_actual_stock(*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  cursor->require_player_turn_dirty_return_without_callbacks(*self,lease,false);return cursor->view();
 });cursor->require_player_turn_dirty_outer_return_without_callbacks(*self,false);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_turn_dirty_without_callbacks();throw;}
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::resume_game_turn_player_after_dirty_delivery(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Player dirty delivery actual source/cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Player dirty delivery canonical cursor substituted");bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->resume_game_turn_player_after_dirty_delivery_with_actual_stock(*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  cursor->require_player_turn_dirty_return_without_callbacks(*self,lease,true);return cursor->view();
 });cursor->require_player_turn_dirty_outer_return_without_callbacks(*self,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_turn_dirty_without_callbacks();throw;}
}
void BnwSelectedStockLuaSourceOwner::require_player_turn_dirty_idle_source_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer& issuer,
 const BnwRetainedTurnCursor& cursor)const{
 const auto thread=stock_thread_;thread->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto c=bound?bound->operation_control:nullptr;
 const auto selected=c?c->selected_host.lock():nullptr;
 production_need(self&&self.get()==this&&bound&&c&&!bound->running&&!bound->poisoned.load()&&!c->invalidated.load()&&
  bound->operation_state==BnwStockLuaOperationState::returned_suspended&&c->serial&&c->vm==bound->vm.get()&&
  c->world.get()==bound->state.get()&&selected&&selected==bound->selected&&selected->session().read_state_lease().get()==bound->state.get()&&
  selected->session().state_generation()==bound->generation&&!c->source_owner.owner_before(bound)&&!bound.owner_before(c->source_owner)&&
  bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn,
  "Player dirty outer source footer did not return current");
 issuer.require_source_lineage_without_callbacks(*this,cursor,bound.get(),c.get(),*bound->state,bound->generation,c->serial);
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_player_end_turn_source_without_callbacks(const BnwRetainedTurnCursor& cursor,
 const BnwStockLuaAttemptLease* lease,bool idle,std::uint64_t expected)const{
 const auto thread=stock_thread_;thread->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto c=lease?lease->control_.lock():(bound?bound->operation_control:nullptr);const auto selected=c?c->selected_host.lock():nullptr;
 production_need(self&&self.get()==this&&!self.owner_before(action_self_)&&!action_self_.owner_before(self)&&bound&&bound->thread==thread&&
  c&&c==bound->operation_control&&!c.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(c)&&
  !c->source_owner.owner_before(bound)&&!bound.owner_before(c->source_owner)&&!bound->poisoned.load()&&!c->invalidated.load()&&
  c->serial&&c->vm==bound->vm.get()&&c->world.get()==bound->state.get()&&selected&&selected==bound->selected&&
  selected->session().read_state_lease().get()==bound->state.get()&&selected->session().state_generation()==bound->generation&&
  bound->operation_world_revision==expected&&bound->turn==bound->state->setup.game_turn&&
  (idle?(!lease&&!bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended):
   (lease&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active)),
  "setEndTurn pure actual Source/control/VM operation changed");
 // Cursor's captured actual delivery issuer/physical predecessor is checked
 // immediately outside this source-only guard; no callback or numeric grant.
 (void)cursor;
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_game_turn_player_end_turn_return(const BnwRetainedTurnCursor& cursor,
 const BnwStockLuaAttemptLease& lease)const{
 const auto thread=stock_thread_;thread->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"setEndTurn actual adoption issuer absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 cursor.require_game_turn_player_end_turn_return_without_callbacks(*bound->selected,*bound->state,bound->generation,
  bound->operation_world_revision,bound->operation_control->serial,*this,false);
 // Exactly the authentic EndTurn receipt's zero-or-one mutation, never a broad
 // revision>= rule. No old revision callback between scalar store and adoption.
 bound->turn=bound->state->setup.game_turn;bound->operation_world_revision=bound->state->ui_source_revision;
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_end_turn_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied){
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"setEndTurn actual Source/cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"setEndTurn canonical Cursor substituted");bound->check();cursor->require_engine_host(*bound->selected);
 std::uint64_t before=0,serial=0;
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);serial=lease.operation_serial();before=bound->operation_world_revision;
  cursor->execute_game_turn_player_end_turn_with_actual_stock(*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  require_player_end_turn_source_without_callbacks(*cursor,&lease,false,bound->state->ui_source_revision);
  cursor->require_game_turn_player_end_turn_return_without_callbacks(*bound->selected,*bound->state,bound->generation,before,serial,*self,true);
  return cursor->view();
 });
 // Seal the real earned inputs again AFTER run_operation's VM/source footer.
 require_player_end_turn_source_without_callbacks(*cursor,nullptr,true,bound->state->ui_source_revision);
 cursor->require_game_turn_player_end_turn_return_without_callbacks(*bound->selected,*bound->state,bound->generation,before,serial,*self,true);
 return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_turn_dirty_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::issue_player_busy_units_return_owned(const BnwRetainedTurnCursor& cursor,
 const BnwNativeUnitAllocatorCore& core,const BnwStockLuaAttemptLease& lease,
 std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>& issued)const{
 stock_thread_->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto actor=cursor.player_busy_actor_without_callbacks(*this,lease);
 production_need(self&&self.get()==this&&bound&&bound->operation_control,"Busy query canonical actual Source absent");
 core.query_player_turn_busy_units_owned(*bound->selected,*bound->state,self,bound->operation_control,lease.operation_serial(),actor,issued);
}
void BnwSelectedStockLuaSourceOwner::seal_player_busy_units_return_without_callbacks(const BnwNativeUnitAllocatorCore& core,
 const BnwNativePlayerBusyUnitsReturn& returned,const BnwStockLuaAttemptLease* lease,bool idle)const{
 stock_thread_->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto c=lease?lease->control_.lock():(bound?bound->operation_control:nullptr);const auto selected=c?c->selected_host.lock():nullptr;
 production_need(self&&self.get()==this&&!self.owner_before(action_self_)&&!action_self_.owner_before(self)&&bound&&bound->thread==stock_thread_&&
  c&&c==bound->operation_control&&!c.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(c)&&
  !c->source_owner.owner_before(bound)&&!bound.owner_before(c->source_owner)&&!bound->poisoned.load()&&!c->invalidated.load()&&
  c->serial&&c->vm==bound->vm.get()&&c->world.get()==bound->state.get()&&selected&&selected==bound->selected&&
  selected->session().read_state_lease().get()==bound->state.get()&&selected->session().state_generation()==bound->generation&&
  bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn&&
  (idle?(!lease&&!bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended):
   (lease&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active)),
  "Busy pure actual Source/control/VM operation changed");
 returned.require_current_without_callbacks(core,*selected,*bound->state,*this,c.get(),bound->generation,c->serial);
}
void BnwSelectedStockLuaSourceOwner::require_player_busy_source_without_callbacks(const BnwRetainedTurnCursor& cursor,
 const BnwNativeUnitAllocatorCore& core,const BnwStockLuaAttemptLease* lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Busy issued operation source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_game_turn_player_busy_return_without_callbacks(core,*bound->selected,*bound->state,*this,
  bound->operation_control.get(),bound->generation,bound->operation_control->serial);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_busy_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>& weak_core){
 stock_thread_->check(); // creator BEFORE mutable weak/Core/Source reads
 // Genuine counted outside-owner borrow before callback COPY/VM providers.
 const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Busy actual Source/cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Busy canonical Cursor substituted");
 cursor->require_player_busy_entry_without_callbacks(core_pin->current(),*self);
 bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();const auto& core=core_pin->current();
  cursor->execute_game_turn_player_busy_with_actual_stock(core,*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  const auto& final_core=core_pin->current();require_player_busy_source_without_callbacks(*cursor,final_core,&lease,false);
  return cursor->view();
 });
 const auto& final_core=core_pin->current();require_player_busy_source_without_callbacks(*cursor,final_core,nullptr,true);
 return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_busy_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::issue_player_busy_cities_return_owned(const BnwRetainedTurnCursor& cursor,
 const BnwNativeUnitAllocatorCore& core,const BnwStockLuaAttemptLease& lease,
 std::shared_ptr<const BnwNativePlayerBusyUnitsReturn> units,std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>& issued)const{
 stock_thread_->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto actor=cursor.player_busy_city_actor_without_callbacks(core,*this,lease);
 production_need(self&&self.get()==this&&bound&&bound->operation_control,"Busy query canonical actual Source absent");
 core.query_player_turn_busy_cities_owned(*bound->selected,*bound->state,self,bound->operation_control,lease.operation_serial(),actor,std::move(units),issued);
}
void BnwSelectedStockLuaSourceOwner::seal_player_busy_cities_return_without_callbacks(const BnwNativeUnitAllocatorCore& core,
 const BnwNativePlayerBusyCitiesReturn& returned,const BnwStockLuaAttemptLease* lease,bool idle)const{
 stock_thread_->check();const auto self=action_self_.lock();const auto bound=bootstrap_;
 const auto c=lease?lease->control_.lock():(bound?bound->operation_control:nullptr);const auto selected=c?c->selected_host.lock():nullptr;
 production_need(self&&self.get()==this&&!self.owner_before(action_self_)&&!action_self_.owner_before(self)&&bound&&bound->thread==stock_thread_&&
  c&&c==bound->operation_control&&!c.owner_before(bound->operation_control)&&!bound->operation_control.owner_before(c)&&
  !c->source_owner.owner_before(bound)&&!bound.owner_before(c->source_owner)&&!bound->poisoned.load()&&!c->invalidated.load()&&
  c->serial&&c->vm==bound->vm.get()&&c->world.get()==bound->state.get()&&selected&&selected==bound->selected&&
  selected->session().read_state_lease().get()==bound->state.get()&&selected->session().state_generation()==bound->generation&&
  bound->operation_world_revision==bound->state->ui_source_revision&&bound->turn==bound->state->setup.game_turn&&
  (idle?(!lease&&!bound->running&&bound->operation_state==BnwStockLuaOperationState::returned_suspended):
   (lease&&bound->running&&bound->operation_state==BnwStockLuaOperationState::active)),
  "Busy pure actual Source/control/VM operation changed");
 returned.require_current_without_callbacks(core,*selected,*bound->state,*this,c.get(),bound->generation,c->serial);
}
void BnwSelectedStockLuaSourceOwner::require_player_busy_city_source_without_callbacks(const BnwRetainedTurnCursor& cursor,
 const BnwNativeUnitAllocatorCore& core,const BnwStockLuaAttemptLease* lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Busy issued operation source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_game_turn_player_busy_city_return_without_callbacks(core,*bound->selected,*bound->state,*this,
  bound->operation_control.get(),bound->generation,bound->operation_control->serial);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_busy_city_prefix(std::shared_ptr<BnwRetainedTurnCursor> supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>& weak_core){
 stock_thread_->check(); // creator BEFORE mutable weak/Core/Source reads
 // Genuine counted outside-owner borrow before callback COPY/VM providers.
 const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied,"Busy actual Source/cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Busy canonical Cursor substituted");
 cursor->require_player_busy_city_entry_without_callbacks(core_pin->current(),*self);
 bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);
  const auto serial=lease.operation_serial();const auto& core=core_pin->current();
  cursor->execute_game_turn_player_busy_city_with_actual_stock(core,*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  const auto& final_core=core_pin->current();require_player_busy_city_source_without_callbacks(*cursor,final_core,&lease,false);
  return cursor->view();
 });
 const auto& final_core=core_pin->current();require_player_busy_city_source_without_callbacks(*cursor,final_core,nullptr,true);
 return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_busy_city_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_player_busy_city_reentry_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control&&bound->running,"City busy active reentry source absent");
 const BnwStockLuaAttemptLease lease(bound->operation_control);
 require_player_busy_units_parent_for_city_without_callbacks(cursor,core,&lease,false);
}
void BnwSelectedStockLuaSourceOwner::require_player_busy_units_parent_for_city_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"City busy actual parent source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_player_busy_units_parent_for_city_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation);
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_player_end_after_busy_reentry_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control&&bound->running,"AI EndTurn canonical active reentry source absent");
 const BnwStockLuaAttemptLease lease(bound->operation_control);
 if(cursor.player_end_after_busy_progress().issued)require_player_end_after_busy_source_without_callbacks(cursor,core,&lease,false);
 else require_player_busy_city_source_without_callbacks(cursor,core,&lease,false);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_player_end_after_busy_return(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"AI EndTurn actual adoption source absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 cursor.require_player_end_after_busy_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,
  bound->operation_world_revision,lease.operation_serial(),false);
 // Actual native EndTurn zero-or-one store, paired adoption BEFORE providers.
 bound->operation_world_revision=bound->state->ui_source_revision;bound->turn=bound->state->setup.game_turn;
}
void BnwSelectedStockLuaSourceOwner::require_player_end_after_busy_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"AI EndTurn physical issued source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 const auto before=cursor.player_end_after_busy_before_without_callbacks();
 cursor.require_player_end_after_busy_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,
  before,bound->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_game_turn_player_end_after_busy_prefix(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak_core){
 stock_thread_->check();const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;production_need(bound&&bound->vm&&supplied,"AI EndTurn real Source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor==supplied,"AI EndTurn supplied foreign Cursor");
 cursor->require_player_end_after_busy_entry_without_callbacks(core_pin->current(),*self);
 bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
   bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
   const auto&core=core_pin->current();cursor->execute_game_turn_player_end_after_busy_with_actual_stock(core,*self,lease);
   bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
   const auto&final_core=core_pin->current();require_player_end_after_busy_source_without_callbacks(*cursor,final_core,&lease,false);return cursor->view();
  });const auto&final_core=core_pin->current();require_player_end_after_busy_source_without_callbacks(*cursor,final_core,nullptr,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_player_end_after_busy_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_processed_auto_reentry_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control&&bound->running,"ProcessedAutoMoves SAME active reentry Source absent");
 const BnwStockLuaAttemptLease lease(bound->operation_control);
 if(cursor.processed_auto_moves_progress().issued)require_processed_auto_source_without_callbacks(cursor,core,&lease,false);
 else require_processed_auto_parent_source_without_callbacks(cursor,core,&lease,false);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_processed_auto_return(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"ProcessedAutoMoves actual Source adoption absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 production_need(cursor.processed_auto_before_without_callbacks()==bound->operation_world_revision,"ProcessedAutoMoves physical predecessor revision differs");
 cursor.require_processed_auto_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial(),false);
 bound->operation_world_revision=bound->state->ui_source_revision;bound->turn=bound->state->setup.game_turn;
}
void BnwSelectedStockLuaSourceOwner::require_processed_auto_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"ProcessedAutoMoves actual issued Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_processed_auto_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,bound->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_processed_auto_moves_after_busy(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak_core){
 stock_thread_->check();const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;production_need(bound&&bound->vm&&supplied,"ProcessedAutoMoves actual Source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor==supplied,"ProcessedAutoMoves foreign Cursor");
 cursor->require_processed_auto_entry_without_callbacks(core_pin->current(),*self);bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_processed_auto_with_actual_stock(core_pin->current(),*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  const auto&final_core=core_pin->current();require_processed_auto_source_without_callbacks(*cursor,final_core,&lease,false);return cursor->view();
 });const auto&final_core=core_pin->current();require_processed_auto_source_without_callbacks(*cursor,final_core,nullptr,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_processed_auto_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::issue_attrition_first_owned(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease&lease,std::shared_ptr<const BnwNativeAttritionPrefix>&issued)const{
 stock_thread_->check();require_attrition_first_parent_source_without_callbacks(cursor,core,&lease,false);
 const auto bound=bootstrap_;const auto actor=cursor.player_end_after_busy_progress().player;
 production_need(bound&&actor,"Attrition actual false-AI native actor absent");
 core.prepare_player_attrition_first_owned(*bound->selected,*bound->state,action_self_.lock(),bound->operation_control,lease.operation_serial(),*actor,issued);
}
void BnwSelectedStockLuaSourceOwner::seal_attrition_first_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwNativeAttritionPrefix&prefix,
 const BnwStockLuaAttemptLease&lease,bool executed)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition actual Source/control seal absent");
 prefix.require_current_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial(),executed);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_first_parent_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition typed parent actual Source missing");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 const bool processed=cursor.processed_auto_moves_progress().issued;
 const auto issued=processed?cursor.processed_auto_serial_without_callbacks():cursor.end_after_busy_serial_without_callbacks();
 production_need(issued&&issued<UINT64_MAX&&bound->operation_control->serial==issued+(idle?0U:1U),
  "Attrition typed parent must be exact next authentic Source operation");
 if(processed)cursor.require_processed_auto_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,issued,true);
 else cursor.require_player_end_after_busy_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,
  cursor.player_end_after_busy_before_without_callbacks(),issued,true);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_first_reentry_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control&&bound->running,"Attrition SAME active Source reentry absent");
 const BnwStockLuaAttemptLease lease(bound->operation_control);
 if(cursor.attrition_first_progress().executed)require_attrition_first_source_without_callbacks(cursor,core,&lease,false);
 else require_attrition_first_parent_source_without_callbacks(cursor,core,&lease,false);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_attrition_first_return(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition actual Source adoption absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 production_need(cursor.attrition_first_before_without_callbacks()==bound->operation_world_revision,"Attrition actual native precursor revision differs");
 cursor.require_attrition_first_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial(),false);
 bound->operation_world_revision=bound->state->ui_source_revision;bound->turn=bound->state->setup.game_turn;
}
void BnwSelectedStockLuaSourceOwner::require_attrition_first_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition actual issued Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_attrition_first_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,bound->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_first_prefix(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak_core){
 stock_thread_->check();const auto core_pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;production_need(bound&&bound->vm&&supplied,"Attrition actual Source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor==supplied,"Attrition foreign Cursor");
 cursor->require_attrition_first_entry_without_callbacks(core_pin->current(),*self);bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_attrition_first_with_actual_stock(core_pin->current(),*self,lease);
  bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  const auto&final_core=core_pin->current();require_attrition_first_source_without_callbacks(*cursor,final_core,&lease,false);return cursor->view();
 });const auto&final_core=core_pin->current();require_attrition_first_source_without_callbacks(*cursor,final_core,nullptr,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_attrition_first_without_callbacks();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_processed_auto_parent_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"ProcessedAuto typed parent actual Source missing");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 const auto issued=cursor.end_after_busy_serial_without_callbacks();
 production_need(issued&&issued<UINT64_MAX&&bound->operation_control->serial==issued+(idle?0U:1U),
  "ProcessedAuto typed parent must be exact next authentic Source operation");
 cursor.require_player_end_after_busy_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,
  cursor.player_end_after_busy_before_without_callbacks(),issued,true);
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_attrition_mountain_parent_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition mountain actual Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);const auto issued=cursor.attrition_first_serial_without_callbacks();
 production_need(issued&&issued<UINT64_MAX&&bound->operation_control->serial==issued+(idle?0U:1U),"Attrition mountain requires exact next authentic Source operation");
 cursor.require_attrition_first_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,issued,true);
}
void BnwSelectedStockLuaSourceOwner::issue_attrition_mountain_owned(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease&lease,const BnwNativeAttritionPrefix&prefix)const{require_attrition_mountain_parent_source_without_callbacks(cursor,core,&lease,false);const auto bound=bootstrap_;
 prefix.prepare_mountain_owned(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial());
}
void BnwSelectedStockLuaSourceOwner::seal_attrition_mountain_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwNativeAttritionPrefix&prefix,
 const BnwStockLuaAttemptLease&lease,bool executed)const{stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition mountain Source seal absent");
 prefix.require_mountain_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial(),executed);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_attrition_mountain_return(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition mountain actual adoption absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 production_need(bound->operation_world_revision==bound->state->ui_source_revision,"Attrition mountain unexpectedly mutated world before required damage owner");
 cursor.require_attrition_mountain_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,lease.operation_serial(),false);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_mountain_source_without_callbacks(const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{stock_thread_->check();const auto bound=bootstrap_;production_need(bound&&bound->operation_control,"Attrition mountain actual current Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_attrition_mountain_return_without_callbacks(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),bound->generation,bound->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_mountain_prefix(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak_core){stock_thread_->check();const auto pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;production_need(bound&&bound->vm&&supplied,"Attrition mountain real Source/cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor==supplied,"Attrition mountain supplied foreign Cursor");
 cursor->require_attrition_mountain_entry_without_callbacks(pin->current(),*self);bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_attrition_mountain_with_actual_stock(pin->current(),*self,lease);bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
  require_attrition_mountain_source_without_callbacks(*cursor,pin->current(),&lease,false);return cursor->view();
 });require_attrition_mountain_source_without_callbacks(*cursor,pin->current(),nullptr,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();bound->quarantine();cursor->fail_attrition_mountain_without_callbacks();throw;}
}
}

#include "bnw_attrition_damage_source.inc"
#include "bnw_attrition_garrison_source.inc"
#include "bnw_attrition_city_strength_source.inc"
#include "bnw_attrition_city_info_source.inc"
#include "bnw_attrition_unit_info_source.inc"
#include "bnw_attrition_unit_selected_source.inc"
#include "bnw_attrition_selected_unit_info_source.inc"
#include "bnw_attrition_selection_plot_source.inc"
#include "bnw_attrition_plot_list_source.inc"
#include "bnw_attrition_damage_dll_wrapper_source.inc"

namespace civ5::runtime {
std::weak_ptr<BnwLocalClientScalarOwner>
BnwSelectedStockLuaSourceOwner::canonical_local_sp_client_origin_owned(
 const BnwWorldViewSourceOperationLoan& source)const{
 if(!stock_thread_)throw std::runtime_error("SP frame source creation thread owner absent");
 stock_thread_->check();
 const auto bound=bootstrap_;
 if(!bound)throw std::runtime_error("SP frame canonical Bootstrap absent");
 try{
  const auto lease=source.sp_client_source_owned();
  const auto scope=lease.pin_active_scope_without_callbacks();
  lease.require_scope_identity_without_callbacks(scope);
  if(!bound->vm||!bound->selected||!bound->state||!bound->operation_control||bound->operation_control.get()!=scope.control.get()||
     bound->operation_control.owner_before(scope.control)||scope.control.owner_before(bound->operation_control)||
     scope.source_owner.get()!=bound.get()||scope.source_owner.owner_before(bound)||bound.owner_before(scope.source_owner))
   throw std::runtime_error("SP frame source is not SAME canonical Bootstrap/control");
  source.require_sp_client_bootstrap_owned(*bound->vm,bound->selected,bound->state);
  const auto actual=bound->local_client;
  if(!actual)throw std::runtime_error("SP frame canonical local client has not been born");
  actual->require_frame_current_without_callbacks(scope);
  const std::weak_ptr<BnwLocalClientScalarOwner> result=actual;
  lease.require_scope_identity_without_callbacks(scope);
  source.require_sp_client_bootstrap_owned(*bound->vm,bound->selected,bound->state);
  return result;
 }catch(...){bound->quarantine();throw;}
}
void BnwSelectedStockLuaSourceOwner::run_sp_client_update_owned(
 const BnwWorldViewSourceOperationLoan& source,const ui::BnwScrollAnimationUpdateLoan& timing){
 const auto client=canonical_local_sp_client_origin_owned(source);
 const auto lease=source.sp_client_source_owned();
 BnwSpClientUpdateLoan loan(source,timing,lease,client);
 try{
  loan.execute_owned();
  const auto after=canonical_local_sp_client_origin_owned(source);
  const auto before_client=client.lock();const auto after_client=after.lock();
  if(!before_client||!after_client||before_client.get()!=after_client.get()||client.owner_before(after)||after.owner_before(client))
   throw std::runtime_error("SP frame canonical client changed after Update");
 }catch(...){if(bootstrap_)bootstrap_->quarantine();throw;}
}
}
