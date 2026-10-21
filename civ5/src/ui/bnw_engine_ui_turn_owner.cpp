#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_attrition_city_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_selected_read.hpp"
#include "civ5/runtime/bnw_attrition_selected_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_selection_plot_read.hpp"
#include "civ5/runtime/bnw_attrition_plot_list_dirty_loan.hpp"
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include <type_traits>
#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/civis_locale_reference_text.hpp"
#include "civ5/ui/detail/bnw_policy_free_tech_text.hpp"
#include "civ5/ui/detail/bnw_policy_branch_notification_text.hpp"
#include "civ5/content/saved_rules.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/bnw_city_current_science_rate.hpp"
#include "civ5/runtime/bnw_city_current_production_rate.hpp"
#include "civ5/runtime/bnw_game_score.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <set>
#include <variant>
#include "civ5/runtime/bnw_city_registry.hpp"
#include <algorithm>
#include <climits>
#include <string_view>
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include <cmath>
#include "civ5/runtime/bnw_player_turn_active_dirty_issuer.hpp"
#include "civ5/ui/bnw_owned_notification_publication_owner.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include <atomic>
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <functional>
#include <stdexcept>
#include <thread>
namespace civ5::ui {
namespace {void need(bool v,const char*why){if(!v)throw std::runtime_error(why);}}
struct BnwEngineUiTurnOwner::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::weak_ptr<const BnwEngineUiTurnOwner> end_turn_self;
    std::atomic<bool> end_turn_alive{true},end_turn_failed{false};
    mutable bool end_turn_checking=false;
    std::size_t end_turn_pins=0,end_turn_ui_pins=0,end_turn_world_pins=0;
    bool end_turn_producing=false;
    std::shared_ptr<BnwEngineUiEndTurnQueue> end_turn_queue;
    std::weak_ptr<const LiveUiHost> ui;
    std::weak_ptr<const SelectedBnwPreviewHost> host;
    std::weak_ptr<BnwClientSelectionControl> selection;
    std::weak_ptr<const runtime::LiveSessionState> world;
    const runtime::LiveSession* session=nullptr;
    std::uint64_t generation=0;
    std::function<bool()> rules_current;
    BnwEngineUiRepetitionSnapshot counters;
    // This owner is created only by the actual new canonical SourceUI bind.
    // Pinned original732210->41C930 ctor writes SAME owner+1BE/+1BF zero.
    // This is a fresh lifecycle producer, never an imported old UI flag.
    enum class SourceUiTurnFlagsOrigin { fresh_constructor };
    const SourceUiTurnFlagsOrigin turn_flags_origin=SourceUiTurnFlagsOrigin::fresh_constructor;
    bool can_end_turn=false,has_moved_unit=false;
    std::uint64_t end_turn_reset_occurrence=0,can_end_turn_setters=0,has_moved_unit_setters=0;
    struct EndTurnResetReturn {
      std::uint64_t occurrence=0,can_before=0,moved_before=0;
      bool can_compare_returned=false,moved_store_returned=false;
    };
    std::optional<EndTurnResetReturn> end_turn_reset_return;
    std::vector<BnwEngineUiCitySummaryOperands> active_city_operands;
    std::vector<BnwEngineUiNetGoldSnapshot> net_queries;
    std::vector<BnwEngineUiDeficitPending> deficits;
 struct TextSource {assets::AssetHandle asset;std::string sha;};
 struct TextOrigin {std::string text;std::size_t source=0;};
 struct TextManifest {std::filesystem::path path;std::string sha;};
 std::shared_ptr<BnwWorldViewFontSourceScope> locale_scope;
 std::unique_ptr<content::RulesDatabase> locale_database;
 std::string language;
 std::vector<TextSource> text_sources;
 std::vector<TextManifest> text_manifests;
 std::map<std::string,TextOrigin> text_origins;
 std::set<std::string> ambiguous_text;
 bool locale_attempted=false,locale_bound=false;

    struct FormattedDeficit {
        std::int32_t actor=-1,turn=-1,threshold=0;std::uint32_t type=0xbe9725c6;
        std::uint64_t generation=0,revision=0;
        std::string message,summary;
    };
    // Only the SAME Host selected-text/captured-define formatter fills this.
    std::optional<FormattedDeficit> formatted_deficit;
    std::vector<BnwEngineUiNotificationEffect> notification_effects;
    struct PolicyNotificationReturn {
      std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
      std::weak_ptr<const runtime::LiveSessionState> world;
      std::int32_t actor=-1;std::uint64_t generation=0,before=0,after=0,serial=0;
      std::int64_t turn=-1;saves::PlayerAdministrationRecord ring;
      runtime::BnwNotificationLoadTransients flags;
      std::vector<BnwEngineUiNotificationEffect> effects;
    };
    std::optional<PolicyNotificationReturn> policy_notification_return;
    struct FreeTechTextReturn {
     std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
     std::weak_ptr<const runtime::LiveSessionState> world;
     std::int32_t actor=-1,old_count=0;std::int64_t effect_policy=-1,turn=-1;
     std::uint64_t generation=0,before=0,serial=0;
     std::string description_key,message;
    };
    std::optional<FreeTechTextReturn> free_tech_text_return;
    std::optional<PolicyNotificationReturn> free_tech_notification_return;
    struct ReformationNotificationReturn {PolicyNotificationReturn native;std::int64_t effect_policy=-1;bool inserted=false;};
    std::optional<ReformationNotificationReturn> reformation_notification_return;
    struct OuterBranchNotificationReturn {PolicyNotificationReturn native;runtime::BnwPolicyBranchNotificationOperands operands;bool inserted=false;std::uint64_t effects_emitted=0;};
    std::optional<OuterBranchNotificationReturn> outer_branch_notification_return;
    bool outer_branch_notification_busy=false;
    bool reformation_notification_busy=false;
    bool free_tech_text_busy=false,free_tech_notification_busy=false;
    struct GoldenAgeReplayReturn {
      std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
      std::weak_ptr<const runtime::LiveSessionState> world;
      std::int32_t actor=-1;std::int64_t turn=-1,num_golden_ages=0;
      std::uint64_t generation=0,before=0,after=0,serial=0;
      std::vector<saves::ReplayMessageObservation> prior,actual;
      saves::ReplayMessageObservation appended;
      bool stored=false;
    };
    std::optional<GoldenAgeReplayReturn> golden_age_replay_return;
    bool golden_age_replay_busy=false;
    struct GameTurnChangedReturn {
     std::weak_ptr<const runtime::BnwRetainedTurnCursor> issuer;std::weak_ptr<const runtime::LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0;
     std::vector<BnwGameTurnChangedEvent> prior,actual;
    };
    std::vector<BnwGameTurnChangedEvent> game_turn_changed_events;
    std::size_t game_turn_changed_delivered=0;
    bool game_turn_changed_producing=false,game_turn_changed_pumping=false;
    std::optional<GameTurnChangedReturn> game_turn_changed_return;
    std::vector<BnwEngineUiGoldenAgeGameplayEvent> golden_age_gameplay_events;
    std::size_t golden_age_gameplay_delivered=0,golden_age_notification_audio_delivered=0;
    struct GoldenAgeGameplayReturn {
      std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
      std::weak_ptr<const runtime::LiveSessionState> world;
      std::uint64_t generation=0,before=0,after=0,serial=0;
      std::vector<BnwEngineUiGoldenAgeGameplayEvent> prior,actual;
      BnwEngineUiGoldenAgeGameplayEvent appended;
    };
    std::optional<GoldenAgeGameplayReturn> golden_age_gameplay_return;
    std::optional<PolicyNotificationReturn> golden_age_notification_return;
    bool golden_age_gameplay_busy=false,golden_age_notification_busy=false,golden_age_pump_busy=false;

    BnwEngineUiSummary last_summary;
    std::vector<BnwEngineUiSummaryEvent> summaries;
    std::uint64_t summary_occurrence=0;
    void identity()const {
        need(creator==std::this_thread::get_id(),"Engine UI turn foreign creator");
        const auto actual_ui=ui.lock();const auto actual_host=host.lock();
        const auto actual_selection=selection.lock();const auto actual_world=world.lock();
        need(actual_ui&&actual_host&&actual_selection&&actual_world&&
            actual_ui->plot_yield_ui_thread_==creator&&
            !actual_ui->selected_owner_lifetime_.owner_before(ui)&&!ui.owner_before(actual_ui->selected_owner_lifetime_)&&
            actual_ui->city_selection_control_.get()==actual_selection.get()&&
            !actual_ui->city_selection_control_.owner_before(actual_selection)&&!actual_selection.owner_before(actual_ui->city_selection_control_),
            "Engine UI turn genuine UI/selection controls retired");
        const auto lease=actual_host->session().read_state_lease();
        need(&actual_host->session()==session&&lease.get()==actual_world.get()&&
            !lease.owner_before(actual_world)&&!actual_world.owner_before(lease)&&
            actual_host->session().state_generation()==generation,
            "Engine UI turn SAME selected session allocation changed");
    }
};
BnwEngineUiTurnOwner::BnwEngineUiTurnOwner(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwEngineUiTurnOwner::~BnwEngineUiTurnOwner(){impl_->end_turn_alive=false;impl_->end_turn_failed=true;}
std::shared_ptr<BnwEngineUiTurnOwner>BnwEngineUiTurnOwner::bind(const LiveUiHost&ui,
    std::shared_ptr<const SelectedBnwPreviewHost>host){
    need(ui.plot_yield_ui_thread_==std::this_thread::get_id(),"Engine UI binding foreign creator");
    const auto actual=ui.selected_owner_lifetime_.lock();
    need(actual&&actual.get()==&ui&&host&&ui.city_selection_control_,"Engine UI binding lacks genuine selected UI");
    auto p=std::make_shared<Impl>();p->ui=actual;p->host=host;p->selection=ui.city_selection_control_;
    p->session=&host->session();p->world=host->session().read_state_lease();p->generation=host->session().state_generation();
    p->rules_current=host->database().immutable_preview_fingerprint_currentness();
    p->identity();need(p->rules_current(),"Engine UI turn actual rules changed");p->identity();
    auto result=std::shared_ptr<BnwEngineUiTurnOwner>(new BnwEngineUiTurnOwner(p));
    p->end_turn_self=result;return result;
}
void BnwEngineUiTurnOwner::require_current()const{
    const auto p=impl_;p->identity();need(p->rules_current(),"Engine UI turn actual rules changed");p->identity();
}
std::uint64_t BnwEngineUiTurnOwner::reset_fresh_end_turn_flags(){return reset_end_turn_flags_impl(nullptr);}
std::uint64_t BnwEngineUiTurnOwner::reset_end_turn_flags_owned(const runtime::BnwStockLuaAttemptLease& source){
 return reset_end_turn_flags_impl(&source);
}
std::uint64_t BnwEngineUiTurnOwner::reset_end_turn_flags_impl(const runtime::BnwStockLuaAttemptLease* source){
 const auto p=impl_;
 need(p->creator==std::this_thread::get_id(),"End-turn producer foreign creator");
 if(p->end_turn_producing||p->end_turn_checking){p->end_turn_failed=true;throw std::runtime_error("End-turn producer reentry");}
 p->end_turn_producing=true;
 try{
 require_current();
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn initial provider retired facade before borrowed entry");
 p->identity();ensure_end_turn_queue_owned();
 need(p->turn_flags_origin==Impl::SourceUiTurnFlagsOrigin::fresh_constructor,
  "End-turn flags require actual fresh SourceUI constructor lineage");
 // The actual SAME owner field chooses the branch; no caller boolean can
 // certify a changed edge. A source-less legacy fresh call still refuses it.
 const bool changed=p->can_end_turn;
 need(!changed||source,"Changed CanEnd requires actual SAME-source operation");
 const auto serial=source?source->operation_serial():0;
 if(source)source->require_operation(serial);
 need(p->end_turn_reset_occurrence<UINT64_MAX&&p->can_end_turn_setters<UINT64_MAX&&p->has_moved_unit_setters<UINT64_MAX,
  "Actual end-turn UI setter occurrence exhausted");
 const auto before_can=p->can_end_turn_setters,before_moved=p->has_moved_unit_setters,occurrence=p->end_turn_reset_occurrence;
 require_current();
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn provider retired facade before setter entry");
 p->identity();
 need(p->can_end_turn==changed&&p->can_end_turn_setters==before_can&&p->has_moved_unit_setters==before_moved&&p->end_turn_reset_occurrence==occurrence,
  "Fresh end-turn UI inputs changed before concrete setters");
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn provider retired source BEFORE real stores");
 Impl::EndTurnResetReturn receipt;receipt.occurrence=occurrence+1;receipt.can_before=before_can;receipt.moved_before=before_moved;
 // Original51E0F0 always stores the argument in owner+1BE. Comparison
 // gates only event69: false==false performs this store without queueing.
 ++p->can_end_turn_setters;p->can_end_turn=false;
 const auto queued=changed?p->end_turn_queue->enqueue_changed_false(*this,*source):0;
 if(source)source->require_operation(serial);
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn source retired after actual store/queue");
 receipt.can_compare_returned=true;
 // Original472690 always writes bool arg to SAME owner+1BF; no calls/queue.
 ++p->has_moved_unit_setters;p->has_moved_unit=false;receipt.moved_store_returned=true;
 p->end_turn_reset_occurrence=receipt.occurrence;p->end_turn_reset_return=receipt;
 require_fresh_end_turn_flags_return(receipt.occurrence);
 if(changed)p->end_turn_queue->mark_setter_return(*this,queued,*source);
 if(source)source->require_operation(serial);
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn producer retired before return");
 p->end_turn_producing=false;return receipt.occurrence;
 }catch(...){p->end_turn_failed=true;p->end_turn_producing=false;throw;}
}
void BnwEngineUiTurnOwner::require_fresh_end_turn_flags_return(std::uint64_t occurrence)const{
 const auto p=impl_;p->identity();need(p->end_turn_reset_return.has_value(),"Actual fresh end-turn UI return absent");
 const auto& r=*p->end_turn_reset_return;
 need(p->turn_flags_origin==Impl::SourceUiTurnFlagsOrigin::fresh_constructor&&r.occurrence==occurrence&&occurrence!=0&&
  p->end_turn_reset_occurrence==occurrence&&r.can_before<UINT64_MAX&&r.moved_before<UINT64_MAX&&
  p->can_end_turn_setters==r.can_before+1&&p->has_moved_unit_setters==r.moved_before+1&&
  r.can_compare_returned&&r.moved_store_returned&&!p->can_end_turn&&!p->has_moved_unit,
  "Actual fresh end-turn UI ordered CanEnd/HasMoved return differs");
}

void BnwEngineUiTurnOwner::consume_live_gold_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwLivePlayerGoldPending>& gold,runtime::TreasuryDirty kind,
 const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Live Gold dirty foreign Engine creator");
 p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
 need(selection&&world&&world.get()==&state,"Live Gold dirty actual Engine Selection/world absent");
 selection->consume_live_gold_dirty(gold,state,kind,reserved);
 p->identity();(void)selection->require_live_gold_dirty_return(*gold,kind);
}
void BnwEngineUiTurnOwner::consume_outer_policy_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Outer policy dirty foreign Engine creator");
 p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
 need(selection&&world&&world.get()==&state,"Outer policy dirty actual Engine Selection/world absent");
 selection->consume_outer_policy_dirty(policy,state,reserved);
 p->identity();selection->require_outer_policy_dirty_return(*policy);
}
void BnwEngineUiTurnOwner::consume_golden_age_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"GoldenAge dirty foreign Engine creator");
 p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
 need(selection&&world&&world.get()==&state,"GoldenAge dirty actual Engine Selection/world absent");
 selection->consume_golden_age_dirty(policy,state,reserved);
 p->identity();selection->require_golden_age_dirty_return(*policy);
}
void BnwEngineUiTurnOwner::reset_repetition_counters(){
    const auto p=impl_;require_current();p->identity();
    // Exact 6A86D0/40A310 false setter bodies in original doTurn order.
    // No invented turn counter, callback, readiness or gameplay revision.
    p->counters.start_diplo_repeat_count=0;
    p->counters.offer_trade_repeat_count=0;
}
BnwEngineUiRepetitionSnapshot BnwEngineUiTurnOwner::repetition_snapshot()const{
    require_current();const auto result=impl_->counters;require_current();return result;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::capture_active_city_operands(std::int32_t actor){
 const auto p=impl_;require_current();const auto world=p->world.lock();need(bool(world),"Engine UI actual summary world expired");
 const auto& state=*world;(void)runtime::resolve_bnw_city_registry(state.world,state.cities,actor,0);
 std::vector<const saves::SavedCityObservation*> order;
 for(const auto& city:state.world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities)order.push_back(&city);
 const auto scalar=[](const auto& city,std::string_view name){const saves::CityCoreField* found=nullptr;for(const auto& field:city.core.fields)if(field.name&&name==field.name){need(!found,"Engine UI duplicate city summary scalar");found=&field;}need(found&&found->encoding==saves::CityCoreEncoding::i32&&found->values.size()==1&&found->values[0]>=INT32_MIN&&found->values[0]<=INT32_MAX,"Engine UI city summary scalar unknown");return static_cast<std::int32_t>(found->values[0]);};
 std::sort(order.begin(),order.end(),[&](const auto*a,const auto*b){return (scalar(*a,"m_iID")&8191)<(scalar(*b,"m_iID")&8191);});
 std::vector<BnwEngineUiCitySummaryOperands> rows;
 for(const auto* raw:order){
  const auto city=scalar(*raw,"m_iID"),population=scalar(*raw,"m_iPopulation"),strength=scalar(*raw,"m_iStrengthValue");
  need(state.cities.players.at(actor).cities.at(city).population==population,"Engine UI summary population raw/owned differs");
  const saves::CityProductionBlob* name=nullptr;
  for(const auto& block:raw->production.blocks)for(const auto& blob:block.blobs)if(blob.name&&std::string_view(blob.name)=="m_strName"){need(!name,"Engine UI duplicate city name");name=&blob;}
  need(name,"Engine UI native city name unavailable");
  rows.push_back({actor,city,population,strength,std::string(name->bytes.begin(),name->bytes.end()),p->generation,state.ui_source_revision});
  if(rows.size()==7)break; // exact upper bound; later native sorting still required
 }
 require_current();need(p->world.lock().get()==world.get()&&world->ui_source_revision==state.ui_source_revision,"Engine UI summary source changed");
 p->active_city_operands.swap(rows);
}
std::vector<BnwEngineUiNetGoldSnapshot> BnwEngineUiTurnOwner::prepare_net_gold_observation(std::int32_t actor,std::int32_t net,std::uint64_t revision)const{
 require_current();const auto p=impl_;need(p->net_queries.size()<UINT64_MAX,"Engine UI query occurrence exhausted");auto rows=p->net_queries;
 rows.push_back({actor,net,net/100,p->generation,revision,static_cast<std::uint64_t>(rows.size())+1});require_current();return rows;
}
void BnwEngineUiTurnOwner::commit_net_gold_observation(std::vector<BnwEngineUiNetGoldSnapshot>& rows)noexcept{impl_->net_queries.swap(rows);}
void BnwEngineUiTurnOwner::record_pending_deficit(std::int32_t actor,std::int32_t net,std::int32_t turn,std::uint64_t revision){
 require_current();const auto p=impl_;need(p->deficits.size()<UINT64_MAX,"Engine UI deficit occurrence exhausted");auto rows=p->deficits;
 BnwEngineUiDeficitPending row;row.actor=actor;row.turn=turn;row.net_times100=net;row.generation=p->generation;row.world_revision=revision;row.occurrence=static_cast<std::uint64_t>(rows.size())+1;rows.push_back(std::move(row));require_current();p->deficits.swap(rows);
}
std::vector<BnwEngineUiCitySummaryOperands> BnwEngineUiTurnOwner::active_city_operands()const{require_current();auto rows=impl_->active_city_operands;require_current();return rows;}
std::vector<BnwEngineUiNetGoldSnapshot> BnwEngineUiTurnOwner::net_gold_queries()const{require_current();auto rows=impl_->net_queries;require_current();return rows;}
std::vector<BnwEngineUiDeficitPending> BnwEngineUiTurnOwner::pending_deficits()const{require_current();auto rows=impl_->deficits;require_current();return rows;}
}

namespace civ5::ui {
namespace {
template<class R>const auto& summary_named(const R& rows,std::string_view name){
 const typename R::value_type* found=nullptr;
 for(const auto& v:rows)if(v.name&&name==v.name){need(!found,"Engine summary duplicate native operand");found=&v;}
 need(found,"Engine summary native operand missing");return *found;
}
std::int32_t summary_i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Engine summary native i32 overflow");return static_cast<std::int32_t>(n);}
std::int32_t summary_add(std::int32_t a,std::int32_t b){return summary_i32(std::int64_t(a)+b);}
std::int32_t summary_mul(std::int32_t a,std::int32_t b){return summary_i32(std::int64_t(a)*b);}
std::uint16_t summary_word(std::int32_t n){return static_cast<std::uint16_t>(static_cast<std::uint32_t>(n));}
auto summary_city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=summary_named(c.core.fields,name);need(f.encoding==saves::CityCoreEncoding::i32&&!f.wire_count&&f.values.size()==1,"Engine summary city scalar shape");return summary_i32(f.values.front());
}
auto summary_player_scalar(const saves::SavedPlayerObservation& p,std::string_view name){
 const auto& f=summary_named(p.core.fields,name);need(f.values.size()==1,"Engine summary player scalar shape");return summary_i32(f.values.front());
}
bool summary_player_bool(const runtime::LiveSessionState& s,std::int32_t p,std::string_view name){
 const auto& raw=s.world.all_players->players.at(static_cast<std::size_t>(p));
 need(raw.index==static_cast<std::uint64_t>(p)&&raw.saved_player_id==p&&summary_player_scalar(raw,"m_eID")==p,"Engine summary player registry differs");
 const auto& f=summary_named(raw.core.fields,name);const auto n=summary_player_scalar(raw,name);
 need(f.encoding==saves::PlayerCoreEncoding::boolean&&(n==0||n==1),"Engine summary player boolean shape");
 if(name=="m_bAlive")need(s.setup.players.at(p).alive==(n!=0),"Engine summary alive raw/owned differs");
 if(name=="m_bEverAlive")need(s.setup.players.at(p).ever_alive==(n!=0),"Engine summary everalive raw/owned differs");return n!=0;
}
auto summary_team_id(const runtime::LiveSessionState& s,std::int32_t p){
 need(s.world.pregame.has_value(),"Engine summary actual Pregame absent");
 const auto& values=s.world.pregame->archive.integer_arrays.at("team_types");need(values.size()==64,"Engine summary native team slots absent");
 const auto t=summary_i32(values.at(static_cast<std::size_t>(p)));need(t>=0&&t<64&&s.setup.players.at(p).team==t,"Engine summary team raw/owned differs");return t;
}
bool summary_minor(const runtime::LiveSessionState& s,std::int32_t p){
 const auto& values=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(values.size()==64,"Engine summary native minor slots absent");
 const bool m=values.at(static_cast<std::size_t>(p));need(s.cities.players.at(p).minor==m,"Engine summary minor raw/owned differs");return m;
}
const auto& summary_team(const runtime::LiveSessionState& s,std::int32_t t){
 need(s.world.teams&&s.world.teams->layout==saves::GameStateLayout::expansion2_version1&&s.world.teams->teams.size()==64&&t>=0&&t<64,"Engine summary native team registry absent");
 const auto& row=s.world.teams->teams.at(static_cast<std::size_t>(t));need(row.index==static_cast<std::uint64_t>(t)&&summary_i32(summary_named(row.fields,"id").value)==t,"Engine summary Team.GetID differs");return row;
}
bool summary_team_alive(const runtime::LiveSessionState& s,std::int32_t t){return summary_i32(summary_named(summary_team(s,t).fields,"alive_count").value)>0;}
bool summary_team_minor(const runtime::LiveSessionState& s,std::int32_t t){
 for(std::int32_t p=22;p<64;++p)if(summary_player_bool(s,p,"m_bAlive")&&summary_team_id(s,p)==t)return summary_minor(s,p);return false;
}
bool summary_team_un(const runtime::LiveSessionState& s,std::int32_t t){const auto n=summary_i32(summary_named(summary_team(s,t).fields,"united_nations").value);need(n==0||n==1,"Engine summary HomeUN native bool differs");return n!=0;}
bool summary_team_relation(const runtime::LiveSessionState& s,std::int32_t t,std::string_view name,std::int32_t other){
 const auto& a=summary_named(summary_team(s,t).arrays,name);need(a.columns==1&&a.values.size()==64,"Engine summary native relation slots absent");const auto n=summary_i32(a.values.at(static_cast<std::size_t>(other)));need(n==0||n==1,"Engine summary native relation boolean differs");return n!=0;
}
std::int32_t summary_define(const content::GlobalDefineCache& cache,std::string_view key){
 const auto* v=cache.find(key);need(v&&v->kind==content::GlobalDefineKind::integer&&v->lookup.found&&v->lookup.error.empty()&&std::holds_alternative<std::int64_t>(v->lookup.value),"Engine summary actual integer define absent");return summary_i32(std::get<std::int64_t>(v->lookup.value));
}
// Native Reset zeroes all Info slots; Read remaps each hash, duplicates last-win.
std::vector<std::int32_t> summary_hashed(const saves::TeamHashedArray& rows,std::string_view table,
 const content::InfoTypeCatalog& types,std::uint64_t slots,bool boolean){
 need(slots<=INT32_MAX&&rows.wire_count>=0&&static_cast<std::uint64_t>(rows.wire_count)==rows.rows.size(),"Engine summary native hashed width absent");
 std::vector<std::int32_t> dense(static_cast<std::size_t>(slots),0);
 for(const auto& r:rows.rows){need(r.type_hash<=UINT32_MAX&&(r.type_hash?r.values.size()==1:r.values.empty()),"Engine summary native hashed row malformed");if(!r.type_hash)continue;
  const auto n=summary_i32(r.values.front());need(!boolean||n==0||n==1,"Engine summary hashed boolean differs");const auto* t=types.find_hash(r.type_hash);if(!t)continue;
  need(t->table==table&&t->id>=0&&static_cast<std::uint64_t>(t->id)<slots&&t->hash==content::saved_info_type_hash(t->type),"Engine summary actual hashed Info identity differs");dense.at(static_cast<std::size_t>(t->id))=n;
 }return dense;
}
std::int32_t summary_secured_votes(const runtime::LiveSessionState& s,std::int32_t active_team,const content::GlobalDefineCache& defines){
 if(!summary_team_alive(s,active_team))return 0;
 const auto& previous=summary_named(s.world.progression.arrays,"previous_votes_cast");need(previous.columns==1&&previous.rows.size()==63,"Engine summary native previous votes absent");
 std::int32_t count=0;
 for(std::int32_t t=0;t<63;++t)if(summary_team_alive(s,t)&&!summary_team_minor(s,t)){
  const auto& r=previous.rows.at(static_cast<std::size_t>(t));need(!r.type_hash&&r.values.size()==1,"Engine summary previous-vote row malformed");if(summary_i32(r.values.front())==active_team)count=summary_add(count,1);
 }
 const auto liberated=summary_i32(summary_named(summary_team(s,active_team).fields,"liberated_by").value);
 for(int pass=0;pass<2;++pass)for(std::int32_t t=0;t<63;++t)if(summary_team_alive(s,t)&&summary_team_minor(s,t)){
  auto voting=-1;const auto by=summary_i32(summary_named(summary_team(s,t).fields,"liberated_by").value);need(by>=-1&&by<64,"Engine summary liberated team invalid");
  if(by!=-1&&summary_team_alive(s,by))voting=by;
  else {std::int32_t leader=-1;for(std::int32_t p=0;p<64;++p)if(summary_player_bool(s,p,"m_bAlive")&&summary_team_id(s,p)==t){leader=p;break;}
   if(leader==-1)for(std::int32_t p=0;p<64;++p)if(summary_team_id(s,p)==t){leader=p;break;}
   need(leader!=-1,"Engine summary minor team leader absent");const auto& ai=summary_named(s.world.all_players->players.at(static_cast<std::size_t>(leader)).planning.blocks,"minor_civ_ai");const auto ally=summary_i32(summary_named(ai.fields,"m_eAlly").value);need(ally>=-1&&ally<64,"Engine summary native ally invalid");if(ally!=-1&&summary_player_bool(s,ally,"m_bAlive"))voting=summary_team_id(s,ally);
  }
  if(voting==-1)voting=t;
  // SDK tests THIS team's liberated_by, in two separately evaluated loops.
  if(voting==active_team&&((pass==0&&liberated!=active_team)||(pass==1&&liberated==active_team)))count=summary_add(count,1);
 }
 if(summary_team_un(s,active_team))count=summary_add(count,summary_define(defines,"OWN_UNITED_NATIONS_VOTE_BONUS"));return count;
}
std::vector<const saves::SavedCityObservation*> summary_city_order(const runtime::LiveSessionState& s,std::int32_t actor){
 (void)runtime::resolve_bnw_city_registry(s.world,s.cities,actor,0);std::vector<const saves::SavedCityObservation*> rows;
 for(const auto& c:s.world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities)rows.push_back(&c);
 std::sort(rows.begin(),rows.end(),[](const auto*a,const auto*b){return (summary_city_scalar(*a,"m_iID")&8191)<(summary_city_scalar(*b,"m_iID")&8191);});
 if(rows.size()>32){std::set<std::int32_t> unique;for(const auto* c:rows)need(unique.insert(summary_city_scalar(*c,"m_iPopulation")).second,"Engine summary large-roster equal-population native partition path required");}
 // Exact <=32 native insertion path. For unique populations, every correct
 // native partition/heap path has this same unique final descending order.
 for(std::size_t i=1;i<rows.size();++i){const auto* value=rows[i];auto j=i;while(j&&summary_city_scalar(*value,"m_iPopulation")>summary_city_scalar(*rows[j-1],"m_iPopulation")){rows[j]=rows[j-1];--j;}rows[j]=value;}
 return rows;
}
std::array<char,128> summary_name(std::string_view name){std::array<char,128> result{};const auto end=name.find('\0');name=name.substr(0,end);if(name.size()<result.size())std::copy(name.begin(),name.end(),result.begin());return result;}
std::int32_t summary_culture(const runtime::LiveSessionState& s,const ui::SelectedBnwPreviewHost& host,
 std::int32_t actor,const saves::SavedCityObservation& city){
 const auto id=summary_city_scalar(city,"m_iID");const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(actor));const auto& live=s.cities.players.at(actor).cities.at(id);
 std::int32_t total=0;for(const auto key:{"m_iJONSCulturePerTurnFromBuildings","m_iJONSCulturePerTurnFromPolicies","m_iJONSCulturePerTurnFromSpecialists"})total=summary_add(total,summary_city_scalar(city,key));
 const auto& buildings=summary_named(city.production.blocks,"buildings");const auto& works=summary_named(buildings.arrays,"m_aBuildingGreatWork");need(works.columns==3&&works.values.size()%3==0,"Engine summary great-work roster shape");
 const auto& changes=summary_named(player.core.fields,"m_aiGreatWorkYieldChange");need(changes.values.size()==6,"Engine summary GreatWorkYieldChange native slots absent");const auto per_work=summary_add(summary_define(host.defines(),"BASE_CULTURE_PER_GREAT_WORK"),summary_i32(changes.values.at(4)));
 auto great_work_culture=summary_mul(per_work,summary_i32(static_cast<std::int64_t>(works.values.size()/3)));
 // Reached occupied/native theming evaluation remains strict until actual
 // selected CultureHelpers operands exist; no guessed bonus or fake work.
 need(works.values.empty(),"Engine summary occupied GreatWork/theming native evaluation required");
 auto rules=runtime::load_city_yield_rules(host.database(),content::TechnologyProfile::bnw);runtime::load_city_building_replacements(host.database(),rules);
 const auto& mapping=rules.civilization_buildings.at(s.cities.players.at(actor).civilization);
 const auto real=runtime::detail::bnw_native_building_counts_reset_read_math(summary_named(buildings.children,"m_paiNumRealBuilding"),rules.building_slots,host.types());
 const auto free=runtime::detail::bnw_native_building_counts_reset_read_math(summary_named(buildings.children,"m_paiNumFreeBuilding"),rules.building_slots,host.types());
 for(std::size_t i=0;i<real.size();++i){const auto read=[&](const auto& m){const auto it=m.find(static_cast<std::int64_t>(i));return it==m.end()?0:summary_i32(it->second);};need(read(live.real_buildings)==real[i]&&read(live.free_buildings)==free[i],"Engine summary building counts raw/owned stale");}
 for(std::uint64_t cls=0;cls<rules.building_class_slots;++cls){const auto it=mapping.find(static_cast<std::int64_t>(cls));const auto building=it==mapping.end()?-1:summary_i32(it->second);if(building==-1)continue;need(building>=0&&static_cast<std::uint64_t>(building)<rules.building_slots,"Engine summary civilization building index invalid");if(summary_add(real.at(static_cast<std::size_t>(building)),free.at(static_cast<std::size_t>(building)))>0){
  std::int32_t theming=0,slots=0;std::size_t count=0;const auto q=host.database().query({"SELECT GreatWorkCount,(SELECT COUNT(*) FROM Building_ThemingBonuses WHERE BuildingType=Buildings.Type) FROM Buildings WHERE ID=?",{std::int64_t(building)}},[&](const auto row){need(row.size()==2&&std::get_if<std::int64_t>(&row[0])&&std::get_if<std::int64_t>(&row[1]),"Engine summary actual Building theming metadata malformed");slots=summary_i32(std::get<std::int64_t>(row[0]));theming=summary_i32(std::get<std::int64_t>(row[1]));++count;});need(q==count&&count==1&&slots>=0&&theming>=0,"Engine summary actual theming metadata absent");
  // Empty real works + positive native slots proves GetThemingBonusIndex=-1.
  need(!theming||slots>0,"Engine summary zero-slot theming native evaluator required");
 }}
 total=summary_add(total,great_work_culture);
 const auto& terrain=summary_named(city.core.fields,"m_aiBaseYieldRateFromTerrain");need(terrain.values.size()==6,"Engine summary culture terrain native slots absent");const auto f=live.yields.find(4);need(summary_i32(terrain.values.at(4))==(f==live.yields.end()?0:f->second.terrain),"Engine summary culture terrain raw/owned differs");total=summary_add(total,summary_i32(terrain.values.at(4)));
 const auto& trait=summary_named(player.traits.fields,"m_iCityCultureBonus");need(trait.encoding==saves::PlayerTraitsEncoding::i32&&trait.values.size()==1,"Engine summary native trait CityCulture cache absent");total=summary_add(total,summary_i32(trait.values.front()));total=summary_add(total,summary_city_scalar(city,"m_iJONSCulturePerTurnFromReligion"));
 auto setup=runtime::restore_research_context(s.world,host.database());const auto restored=runtime::restore_city_yields(s.world,setup,host.database(),rules,host.types());need(restored.leagues.size()==s.cities.leagues.size(),"Engine summary league raw/owned roster differs");
 std::int32_t modifier=0;for(std::size_t n=0;n<s.cities.leagues.size();++n){const auto& league=s.cities.leagues[n];const auto& raw=restored.leagues[n];need(league.members==raw.members&&league.culture_per_wonder==raw.culture_per_wonder&&league.members.size()==league.culture_per_wonder.size(),"Engine summary league culture raw/owned differs");for(std::size_t i=0;i<league.members.size();++i)if(league.members[i]==actor)modifier=summary_add(modifier,summary_i32(league.culture_per_wonder[i]));}
 total=summary_add(total,summary_mul(summary_city_scalar(city,"m_iNumWorldWonders"),modifier));return total;
}
}
BnwEngineUiSummary BnwEngineUiTurnOwner::build_active_summary(std::int32_t actor)const{
 const auto p=impl_;require_current();const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world,"Engine summary actual selected owners retired");const auto& s=*world;
 need(actor==runtime::detail::bnw_game_active_player_read(s)&&host->active_player_id()==actor,"Engine summary wrong actual active player");
 const auto active_team=summary_team_id(s,actor);BnwEngineUiSummary out;
 const auto order=summary_city_order(s,actor);
 const auto db_owner=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 const auto session_owner=std::shared_ptr<const runtime::LiveSession>(host,&host->session());
 const auto food_rules=std::make_shared<const runtime::BnwCityFoodRateRules>(runtime::BnwCityFoodRateRules::load(db_owner,host->identity().rules_sha256));
 for(std::size_t i=0;i<std::min(order.size(),out.cities.size());++i){
  const auto& raw=*order[i];const auto id=summary_city_scalar(raw,"m_iID");const auto population=summary_city_scalar(raw,"m_iPopulation");need(s.cities.players.at(actor).cities.at(id).population==population,"Engine summary city population raw/owned differs");
  const saves::CityProductionBlob* blob=nullptr;for(const auto& b:raw.production.blocks)for(const auto& v:b.blobs)if(v.name&&std::string_view(v.name)=="m_strName"){need(!blob,"Engine summary duplicate city name");blob=&v;}need(blob,"Engine summary native city name absent");std::string name(blob->bytes.begin(),blob->bytes.end());
  // A literal native name has no localization lookup. Selected TXT_KEY names
  // require the genuine selected locale owner; echoing a key is not GetName.
  name=localized_city_name(name);
  auto& row=out.cities[i];row.name=summary_name(name);row.fields[0]=summary_word(population);row.fields[1]=summary_word(summary_city_scalar(raw,"m_iStrengthValue")/100);
  const auto food=runtime::inspect_bnw_city_current_food_difference(session_owner,food_rules,actor,id,true).value_times100();row.fields[2]=summary_word(food/100);
  const auto threshold=runtime::inspect_bnw_city_current_growth_threshold(host,actor,id).value();const auto left=summary_i32(std::int64_t(summary_mul(threshold,100))-summary_city_scalar(raw,"m_iFood"));
  const auto turn_food=runtime::inspect_bnw_city_current_food_difference(session_owner,food_rules,actor,id,true).value_times100();
  std::int32_t turns=left;if(turn_food>0){turns=left/turn_food;if(summary_mul(turns,turn_food)<left)turns=summary_add(turns,1);turns=std::max<std::int32_t>(1,turns);}row.fields[3]=summary_word(turns);
  row.fields[4]=summary_word(runtime::inspect_bnw_city_current_science_rate(host,actor,id).value_times100()/100);
  auto gold=runtime::BnwCityCurrentScienceRate::bind_current_source(host,actor,id,runtime::BnwCityCurrentScienceRate::SourceKind::gold);row.fields[5]=summary_word(gold.value_times100()/100);
  row.fields[6]=summary_word(summary_culture(s,*host,actor,raw));
  row.fields[7]=summary_word(runtime::inspect_bnw_city_current_production_rate(host,actor,id).value_times100()/100);
 }
 out.secured_votes=summary_word(summary_secured_votes(s,active_team,host->defines()));
 need(std::holds_alternative<saves::Expansion2PrefixObservation>(s.world.game_state.prefix),"Engine summary native game prefix absent");const auto& prefix=std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix);out.votes_needed=summary_word(summary_i32(prefix.scalars.at(27)));
 out.ever_alive=0;out.alive_capitals=0;out.apollo_teams=0;out.max_apollo=0;out.home_un=0;out.max_finished_branches=0;
 const auto* apollo=host->types().find("PROJECT_APOLLO_PROGRAM");need(apollo&&apollo->table=="Projects"&&apollo->id>=0,"Engine summary native GetProjectCount missing Project index");const auto projects=host->database().prefetch_info("Projects");const auto branches=host->database().prefetch_info("PolicyBranchTypes");need(projects.all_succeeded()&&!projects.overlapping_ids&&branches.all_succeeded()&&!branches.overlapping_ids,"Engine summary native Info slot collections absent");
 std::size_t row=0;
 for(std::int32_t other=0;other<22&&row<out.opponents.size();++other){
  if(other==actor||summary_minor(s,other)||!summary_player_bool(s,other,"m_bEverAlive"))continue;
  out.ever_alive=static_cast<std::uint16_t>(out.ever_alive+1);const auto t=summary_team_id(s,other);
  const auto project_values=summary_hashed(summary_named(summary_team(s,t).hashes,"project_count"),"Projects",host->types(),projects.slots,false);
  need(static_cast<std::uint64_t>(apollo->id)<projects.slots,"Engine summary Apollo native slot out of collection");const auto project=static_cast<std::uint32_t>(project_values.at(static_cast<std::size_t>(apollo->id)));
  if(project>0)out.apollo_teams=static_cast<std::uint16_t>(out.apollo_teams+1);if(project>out.max_apollo)out.max_apollo=static_cast<std::uint8_t>(project);
  if(!out.home_un)out.home_un=static_cast<std::uint8_t>(summary_team_un(s,t));
  const auto& policies=summary_named(s.world.all_players->players.at(static_cast<std::size_t>(other)).strategy.blocks,"policies");const auto finished=summary_hashed(summary_named(policies.hashes,"branch_finished"),"PolicyBranchTypes",host->types(),branches.slots,true);std::uint32_t num_finished=0;for(auto n:finished)if(n)++num_finished;if(num_finished>out.max_finished_branches)out.max_finished_branches=static_cast<std::uint8_t>(num_finished);
  if(!summary_player_bool(s,other,"m_bAlive"))continue;
  if(!summary_player_bool(s,other,"m_bLostCapital"))out.alive_capitals=static_cast<std::uint16_t>(out.alive_capitals+1);
  if(!summary_team_relation(s,active_team,"has_met",t))continue;
  const auto& civs=s.world.pregame->slots.integer_arrays.at("civilizations");need(civs.size()==64,"Engine summary native civilization slots absent");const auto civilization=summary_i32(civs.at(static_cast<std::size_t>(other)));need(civilization==s.cities.players.at(other).civilization,"Engine summary civilization raw/owned differs");std::size_t found=0;std::string short_name;
  const auto q=host->database().query({"SELECT ShortDescription FROM Civilizations WHERE ID=?",{std::int64_t(civilization)}},[&](const auto values){need(values.size()==1&&std::get_if<std::string>(&values[0]),"Engine summary actual Civilization ShortDescription malformed");short_name=std::get<std::string>(values[0]);++found;});need(q==found&&found==1,"Engine summary actual CivilizationInfo absent");
  auto& value=out.opponents[row++];value.name=summary_name(short_name);value.status=summary_team_relation(s,active_team,"at_war",t)?0:6;
  // Fresh getter, even after land-score updates. Never cached GameRoster score.
  value.score=summary_word(runtime::BnwGameScorePreparedWrites::read_player_score(s,host->database(),host->types(),host->defines(),other));
 }
 out.ever_alive=static_cast<std::uint16_t>(out.ever_alive+1);if(!summary_player_bool(s,actor,"m_bLostCapital"))out.alive_capitals=static_cast<std::uint16_t>(out.alive_capitals+1);
 out.voting_teams=0;for(std::int32_t t=0;t<64;++t)if(summary_team_alive(s,t)&&t!=63){out.voting_teams=static_cast<std::uint16_t>(out.voting_teams+1);if(summary_team_un(s,t))out.voting_teams=static_cast<std::uint16_t>(out.voting_teams+1);}
 need(s.world.pregame.has_value(),"Engine summary genuine max-turns source absent");out.turns_left=static_cast<std::uint32_t>(summary_i32(s.world.pregame->archive.integers.at("max_turns")))-static_cast<std::uint32_t>(summary_i32(prefix.scalars.at(1)));
 require_current();return out;
}
void BnwEngineUiTurnOwner::publish_active_summary(std::int32_t actor){
 const auto p=impl_;require_current();const auto world=p->world.lock();need(bool(world),"Engine summary world retired");const auto revision=world->ui_source_revision;
 const auto value=build_active_summary(actor);need(world->ui_source_revision==revision,"Engine summary source changed during reads");
 const bool changed=!(value==p->last_summary);auto queued=p->summaries;
 if(changed){need(p->summary_occurrence<UINT64_MAX,"Engine summary occurrence exhausted");queued.push_back({0xa8,actor,p->generation,revision,p->summary_occurrence+1,value});}
 require_current();const auto verified=build_active_summary(actor);need(value==verified&&world->ui_source_revision==revision,"Engine summary semantic inputs changed before publication");p->identity();need(p->world.lock().get()==world.get()&&world->ui_source_revision==revision,"Engine summary final same-world bookend failed");
 // Pure semantic assignment + no-throw vector swap. No provider after final
 // operand comparison. Actual message queue lives in the canonical UiHost.
 if(changed){p->last_summary=value;p->summaries.swap(queued);++p->summary_occurrence;}
}
std::vector<BnwEngineUiSummaryEvent> BnwEngineUiTurnOwner::queued_summaries()const{require_current();auto rows=impl_->summaries;require_current();return rows;}
std::vector<BnwEngineUiSummaryEvent> LiveUiHost::engine_ui_summaries()const{
 need(plot_yield_ui_thread_==std::this_thread::get_id(),"Engine UI summary read foreign creator");const auto owner=engine_ui_turn_owner_;return owner?owner->queued_summaries():std::vector<BnwEngineUiSummaryEvent>{};
}
}

namespace civ5::ui {
namespace {
std::optional<std::string> engine_text_field(const content::XmlNode& row,const char* name){
 std::optional<std::string> attribute;
 for(const auto& [key,value]:row.attributes)if(key==name){need(!attribute,"Engine Locale duplicate selected XML attribute");attribute=value;}
 if(attribute)return attribute;
 if(const auto* n=row.child(name)){need(n->children.empty(),"Engine summary selected Locale grammar node unsupported");return n->text;}return std::nullopt;
}
}
bool BnwEngineUiTurnOwner::has_selected_city_name_keys(std::int32_t actor)const{
 require_current();const auto world=impl_->world.lock();need(bool(world),"Engine Locale source world retired");const auto order=summary_city_order(*world,actor);
 for(std::size_t i=0;i<std::min<std::size_t>(7,order.size());++i)for(const auto& block:order[i]->production.blocks)for(const auto& blob:block.blobs)if(blob.name&&std::string_view(blob.name)=="m_strName"){
  const auto name=std::string(blob.bytes.begin(),blob.bytes.end());if(name.starts_with("TXT_KEY_"))return true;
 }
 return false;
}
void BnwEngineUiTurnOwner::bind_selected_localization(const std::filesystem::path& root,
 const runtime::SavedContentProfile& input_profile,const content::Catalog& input_catalog,
 const std::shared_ptr<const runtime::LiveSessionState>& source_world,const runtime::SessionCheckpointIdentity& source_identity){
 const auto p=impl_;require_current();const auto actual=p->world.lock();const auto host=p->host.lock();need(actual&&host&&source_world&&actual.get()==source_world.get()&&!actual.owner_before(source_world)&&!source_world.owner_before(actual)&&host->identity().payload_sha256==source_identity.payload_sha256&&host->identity().rules_sha256==source_identity.rules_sha256,"Engine Locale wrong actual Bootstrap source world/rules");
 if(p->locale_bound){require_selected_localization_current();return;}
 need(!p->locale_attempted,"Engine Locale source reinitialization refused");p->locale_attempted=true;
 const auto ui=p->ui.lock();need(ui&&ui->issued_plot_yield_snapshot_,"Engine Locale actual issued UI source snapshot absent");
 // Read-only camera-selected source quote. This does not issue Context/Lua,
 // Label/font, stale Scene readiness or a popup capability.
 const auto scope=BnwWorldViewFontSourceScope::from_actual_tech_context_snapshot(*ui->issued_plot_yield_snapshot_);
 const auto language=scope->locale().language;need(language=="en_US"||language=="ru_RU","Engine Locale actual selected language unsupported");
 const auto profile=input_profile;const auto catalog=input_catalog;const auto selected_root=root;
 need(profile.can_activate&&profile.all_requirements_located&&profile.files_complete&&profile.enabled_mods.empty(),"Engine Locale actual stock TextData recipe incomplete");
 auto database=host->database().clone_gameplay();std::vector<Impl::TextManifest> manifests;std::set<std::size_t> seen;
 for(const auto index:profile.enabled_dlc){need(index<catalog.packages.size()&&seen.insert(index).second,"Engine Locale duplicate selected DLC");const auto& package=catalog.packages[index];
  const auto before=runtime::identify_file(package.path);const auto actual_package=content::read_manifest(package.path);const auto after=runtime::identify_file(package.path);
  need(before.sha256==after.sha256&&before.bytes==after.bytes,"Engine Locale TextData manifest changed across parse");
  content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};const auto expected=content::package_resources(package,query),found=content::package_resources(actual_package,query);
  need(actual_package.kind==package.kind&&content::canonical_guid(actual_package.id)==content::canonical_guid(package.id)&&actual_package.version==package.version&&expected.size()==found.size(),"Engine Locale activated TextData manifest differs");
  for(std::size_t n=0;n<found.size();++n)need(expected[n].value==found[n].value&&expected[n].category==found[n].category&&expected[n].category_occurrence==found[n].category_occurrence&&expected[n].file_list==found[n].file_list,"Engine Locale TextData declarations changed");manifests.push_back({package.path,before.sha256});
 }
 const auto records=content::selected_localization_sources(selected_root,profile.enabled_dlc,catalog,scope->resources());const auto table_name=language=="en_US"?"Language_en_US":"Language_RU_RU";
 std::vector<Impl::TextSource> sources;std::map<std::string,Impl::TextOrigin> origins;std::set<std::string> ambiguous;std::map<std::string,std::optional<Impl::TextOrigin>> layer;std::optional<std::size_t> group;
 const auto flush=[&]{for(const auto& [tag,value]:layer){if(value){origins.insert_or_assign(tag,*value);ambiguous.erase(tag);}else{origins.erase(tag);ambiguous.insert(tag);}}layer.clear();};
 for(const auto& record:records){if(record.package!=group){flush();group=record.package;}const auto bytes=record.asset.read_bytes(64ULL*1024*1024);const auto doc=content::read_game_data_xml_text(std::string(bytes.begin(),bytes.end()));need(doc.name=="GameData","Engine Locale TextData root differs");
  const content::XmlNode* table=nullptr;for(const auto& child:doc.children)if(child.name==table_name){need(!table,"Engine Locale duplicate actual language table");table=&child;}if(!table)continue;
  const auto n=sources.size();sources.push_back({record.asset,runtime::identify_bytes(bytes).sha256});for(const auto& row:table->children){const auto tag=engine_text_field(row,"Tag"),text=engine_text_field(row,"Text");need(row.name=="Row"&&tag&&!tag->empty()&&text,"Engine Locale TextData row operation unsupported");if(!layer.emplace(*tag,Impl::TextOrigin{*text,n}).second)layer.at(*tag).reset();}
 }
 flush();need(!origins.empty(),"Engine Locale selected source has no rows");std::size_t count=0;const auto q=database->query({"SELECT Type FROM Localization.Languages WHERE Type=?",{language}},[&](const auto row){need(row.size()==1&&std::get_if<std::string>(&row[0]),"Engine Locale actual language DB metadata malformed");++count;});need(q==count&&count==1,"Engine Locale actual activated language absent or duplicated");
 scope->require_current();require_current();p->identity();need(p->world.lock().get()==source_world.get(),"Engine Locale source world changed during preparation");
 p->locale_scope=scope;p->language=language;p->locale_database=std::move(database);p->text_sources=std::move(sources);p->text_manifests=std::move(manifests);p->text_origins=std::move(origins);p->ambiguous_text=std::move(ambiguous);p->locale_bound=true;
 require_selected_localization_current();
}
void BnwEngineUiTurnOwner::require_selected_localization_current()const{
 const auto p=impl_;require_current();need(p->locale_bound&&p->locale_scope&&p->locale_database,"Engine Locale genuine selected source unbound");p->locale_scope->require_current();need(p->locale_scope->locale().language==p->language,"Engine Locale selected language changed");for(const auto& manifest:p->text_manifests)need(runtime::identify_file(manifest.path).sha256==manifest.sha,"Engine Locale source manifest changed");require_current();
}
std::string BnwEngineUiTurnOwner::selected_localization_text(std::string_view name)const{
 require_selected_localization_current();const auto p=impl_;const auto raw=[&](std::string_view key){need(!key.empty()&&key.find('\0')==key.npos&&!p->ambiguous_text.contains(std::string(key)),"Engine Locale city name key invalid or ambiguous");const auto found=p->text_origins.find(std::string(key));need(found!=p->text_origins.end(),"Engine Locale city key has no selected source");
  std::string text;std::size_t count=0;const auto q=p->locale_database->query({"SELECT Text FROM Localization.LocalizedText WHERE Language=? AND Tag=?",{p->language,std::string(key)}},[&](const auto row){need(row.size()==1&&std::get_if<std::string>(&row[0]),"Engine Locale actual city text row malformed");text=std::get<std::string>(row[0]);++count;});need(q==count&&count==1&&text==found->second.text,"Engine Locale activated DB/selected TextData differs");const auto& source=p->text_sources.at(found->second.source);need(runtime::identify_bytes(source.asset.read_bytes(64ULL*1024*1024)).sha256==source.sha,"Engine Locale selected city text source changed");return text;
 };
 std::string key(name.substr(0,name.find('\0')));auto text=expand_civis_locale_references(key,raw);require_selected_localization_current();return text;
}
std::string BnwEngineUiTurnOwner::localized_city_name(std::string_view name)const{
 if(!name.starts_with("TXT_KEY_"))return std::string(name);
 auto text=selected_localization_text(name);need(text.find('{')==text.npos&&text.find('}')==text.npos,"Engine Locale city name needs native argument/grammar formatter");return text;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::format_pending_deficit(){
 const auto p=impl_;require_current();require_selected_localization_current();
 const auto world=p->world.lock();const auto host=p->host.lock();need(world&&host&&!p->deficits.empty()&&!p->formatted_deficit,"Engine deficit actual pending formatter predecessor absent/used");
 const auto request=p->deficits.back();
 need(request.actor>=0&&request.actor<22&&request.net_times100/100<=0&&request.turn==world->setup.game_turn&&request.generation==p->generation&&request.world_revision==world->ui_source_revision,
      "Engine deficit pending actual source/turn/revision changed");
 // Retail602798 constructs long localized string FIRST. 6027A9 obtains
 // actual GameContext, +12C calls GetDEFICIT_UNIT_DISBANDING_THRESHOLD.
 // Civis uses the SAME Host's actually captured define + live DB equality;
 // neither ctor -5 nor the earlier player's calculated net gold is a value.
 auto message_source=selected_localization_text(request.message_key);
 const auto threshold=summary_define(host->defines(),"DEFICIT_UNIT_DISBANDING_THRESHOLD");
 const auto define=host->database().global_define("DEFICIT_UNIT_DISBANDING_THRESHOLD",content::GlobalDefineKind::integer);
 need(define.found&&define.error.empty()&&std::holds_alternative<std::int64_t>(define.value)&&summary_i32(std::get<std::int64_t>(define.value))==threshold,
      "Engine deficit actual retained GameContext define capture/current DB differs");
 // The selected EN/RU source has exactly {1_Num}; preserve icon markup.
 // Native grammar modifiers are not silently erased by the plain formatter.
 for(std::size_t at=0;(at=message_source.find('{',at))!=message_source.npos;){
  const auto end=message_source.find('}',at+1);need(end!=message_source.npos&&std::string_view(message_source).substr(at+1,end-at-1)=="1_Num",
   "Engine deficit selected message requires unsupported native grammar");at=end+1;
 }
 TopPanelLocalizationCatalog catalog;catalog.rows.emplace(request.message_key,std::move(message_source));
 auto message=catalog.format(request.message_key,{std::to_string(threshold)});
 // Retail6027D9 then creates the independent short summary (no argument).
 auto summary=selected_localization_text(request.summary_key);
 need(!message.empty()&&!summary.empty()&&message.find('{')==message.npos&&message.find('}')==message.npos&&summary.find('{')==summary.npos&&summary.find('}')==summary.npos,
      "Engine deficit selected formatting left unresolved source placeholders");
 Impl::FormattedDeficit formatted;formatted.actor=request.actor;formatted.turn=request.turn;formatted.type=request.native_type;
 formatted.generation=request.generation;formatted.revision=request.world_revision;formatted.threshold=threshold;
 formatted.message=std::move(message);formatted.summary=std::move(summary);
 require_selected_localization_current();require_current();
 const auto final_define=host->database().global_define("DEFICIT_UNIT_DISBANDING_THRESHOLD",content::GlobalDefineKind::integer);
 need(final_define.found&&final_define.error.empty()&&std::holds_alternative<std::int64_t>(final_define.value)&&std::get<std::int64_t>(final_define.value)==threshold&&
      summary_define(host->defines(),"DEFICIT_UNIT_DISBANDING_THRESHOLD")==threshold&&world->ui_source_revision==request.world_revision&&world->setup.game_turn==request.turn&&
      p->deficits.back().actor==request.actor&&p->deficits.back().occurrence==request.occurrence,"Engine deficit formatter source changed");
 p->identity();p->formatted_deficit=std::move(formatted);
}

namespace {
std::string_view engine_notification_name(const char* name){return name?std::string_view(name):std::string_view();}
auto& engine_notification_field(auto& row,std::string_view wanted){
 using Field=std::remove_reference_t<decltype(row.fields.front())>;
 Field* found=nullptr;
 for(auto& field:row.fields)if(engine_notification_name(field.name)==wanted){need(!found,"Engine Add duplicate native field");found=&field;}
 need(found,"Engine Add native field absent");return found->value;
}
auto& engine_notification_blob(auto& row,std::string_view wanted){
 using Blob=std::remove_reference_t<decltype(row.blobs.front())>;
 Blob* found=nullptr;
 for(auto& blob:row.blobs)if(engine_notification_name(blob.name)==wanted){need(!found,"Engine Add duplicate native string");found=&blob;}
 need(found,"Engine Add native string absent");return found->bytes;
}
bool engine_notification_same(const saves::PlayerAdministrationRecord& a,const saves::PlayerAdministrationRecord& b)noexcept{
 if(engine_notification_name(a.name)!=engine_notification_name(b.name)||a.index!=b.index||a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||a.fields.size()!=b.fields.size()||a.arrays.size()!=b.arrays.size()||a.blobs.size()!=b.blobs.size()||a.children.size()!=b.children.size())return false;
 for(std::size_t n=0;n<a.fields.size();++n)if(engine_notification_name(a.fields[n].name)!=engine_notification_name(b.fields[n].name)||a.fields[n].value!=b.fields[n].value)return false;
 for(std::size_t n=0;n<a.arrays.size();++n)if(engine_notification_name(a.arrays[n].name)!=engine_notification_name(b.arrays[n].name)||a.arrays[n].columns!=b.arrays[n].columns||a.arrays[n].values!=b.arrays[n].values)return false;
 for(std::size_t n=0;n<a.blobs.size();++n)if(engine_notification_name(a.blobs[n].name)!=engine_notification_name(b.blobs[n].name)||a.blobs[n].bytes!=b.blobs[n].bytes)return false;
 for(std::size_t n=0;n<a.children.size();++n)if(!engine_notification_same(a.children[n],b.children[n]))return false;
 return true;
}
bool engine_notification_flags_same(const runtime::BnwNotificationLoadTransients& a,const runtime::BnwNotificationLoadTransients& b)noexcept{
 return a.current==b.current&&a.external_update_pending==b.external_update_pending&&a.players==b.players;
}
bool engine_notification_options_same(const std::vector<saves::PregameOption>& a,const std::vector<saves::PregameOption>& b)noexcept{
 if(a.size()!=b.size())return false;
 for(std::size_t n=0;n<a.size();++n)if(a[n].name!=b[n].name||a[n].value!=b[n].value)return false;
 return true;
}
auto& engine_notification_ring(auto& state,std::int32_t actor){
 need(actor>=0&&actor<22&&state.world.all_players&&state.world.all_players->players.size()==64,"Engine Add actual player roster absent");
 auto& player=state.world.all_players->players.at(static_cast<std::size_t>(actor));
 need(player.saved_player_id==actor&&player.index==static_cast<std::uint64_t>(actor)&&player.administration.layout==saves::GameStateLayout::expansion2_version1&&player.administration.blocks.size()==3,"Engine Add actual player/administration differs");
 auto& ring=player.administration.blocks.front();need(engine_notification_name(ring.name)=="notifications","Engine Add actual notification owner absent");return ring;
}
void engine_notification_clear(saves::PlayerAdministrationRecord& row,runtime::BnwNotificationLoadSlot& flags){
 // CvNotifications::Notification::Clear deliberately DOES NOT reset
 // m_ePlayerID or m_iExtraGameData. Preserve those real existing fields.
 engine_notification_field(row,"m_eNotificationType_bits")=UINT32_MAX;
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iTurn","m_iLookupIndex"})engine_notification_field(row,name)=-1;
 engine_notification_blob(row,"m_strMessage").clear();engine_notification_blob(row,"m_strSummary").clear();
 engine_notification_field(row,"m_strMessage_wire_bytes")=0;engine_notification_field(row,"m_strSummary_wire_bytes")=0;
 engine_notification_field(row,"m_bDismissed")=0;flags={false,false};
}
}

std::vector<BnwEngineUiNotificationEffect> BnwEngineUiTurnOwner::queued_notifications()const{
 require_current();auto result=impl_->notification_effects;require_current();return result;
}
bool BnwEngineUiTurnOwner::add_formatted_deficit(runtime::LiveSessionState& state){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&p->formatted_deficit,"Engine Add requires genuine internal selected formatter return");
 // This private record is filled only by the actual formatter producer. A
 // diagnostic pending-deficit DTO or caller strings cannot authorize Add.
 const auto formatted=*p->formatted_deficit;
 const auto revision=state.ui_source_revision;
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&formatted.type==0xbe9725c6,
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){p->identity();p->formatted_deficit.reset();return false;}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){p->identity();p->formatted_deficit.reset();return false;}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0&&lookup<INT32_MAX&&revision<UINT64_MAX,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 const auto auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 const auto active=runtime::detail::bnw_game_active_player_read(state);
 const auto game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
 need(game_type==state.setup.game_type&&game_type>=0&&game_type<=3,"Engine Add source game type differs");
 const bool broadcast=initialized&&actor==active&&(game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=p->notification_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,turn_active!=0&&auto_moves!=0}; // GENERIC expiry=true
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 require_selected_localization_current();require_current();
 p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&state.ui_source_revision==revision&&p->formatted_deficit&&p->formatted_deficit->actor==actor&&p->formatted_deficit->revision==revision&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&summary_player_bool(state,actor,"m_bAutoMoves")==auto_moves&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&runtime::detail::bnw_game_active_player_read(state)==active&&
      state.world.pregame->archive.integers.at("game_type")==game_type&&state.setup.game_type==game_type&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;p->formatted_deficit.reset();return true;
}
bool BnwEngineUiTurnOwner::add_free_policy_native(runtime::LiveSessionState& state,std::string_view message_input,std::string_view summary_input,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&policy,"Free policy native Add SAME world/source absent");
 policy->require_free_policy_notification_source(*host,state);
 Impl::FormattedDeficit formatted;formatted.actor=summary_i32(policy->player());formatted.turn=summary_i32(state.setup.game_turn);formatted.type=0x1e4417c5;
 formatted.generation=p->generation;formatted.revision=state.ui_source_revision;formatted.message=std::string(message_input);formatted.summary=std::string(summary_input);
 const auto revision=state.ui_source_revision;
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&formatted.type==0x1e4417c5,
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){p->identity();return false;}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){p->identity();return false;}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0&&lookup<INT32_MAX&&revision<UINT64_MAX,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 // CvNotifications::IsNotificationRedundant FREE_POLICY scans ONLY the
 // actual active native ring, returning on undismissed same native type.
 for(auto index:actual_active)if(engine_notification_field(before_ring.children.at(index),"m_eNotificationType_bits")==0x1e4417c5){
  policy->require_free_policy_notification_source(*host,state);p->identity();
  need(state.ui_source_revision==revision&&engine_notification_same(engine_notification_ring(state,actor),before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags),"Free policy redundant native branch changed");return false;
 }

 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 const auto auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 const auto active=runtime::detail::bnw_game_active_player_read(state);
 const auto game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
 need(game_type==state.setup.game_type&&game_type>=0&&game_type<=3,"Engine Add source game type differs");
 const bool broadcast=initialized&&actor==active&&(game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=p->notification_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,false}; // FREE_POLICY never end-of-turn expires; wait-extra-turn is false
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 require_selected_localization_current();require_current();
 policy->require_free_policy_notification_source(*host,state);p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&state.ui_source_revision==revision&&policy->player()==actor&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&summary_player_bool(state,actor,"m_bAutoMoves")==auto_moves&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&runtime::detail::bnw_game_active_player_read(state)==active&&
      state.world.pregame->archive.integers.at("game_type")==game_type&&state.setup.game_type==game_type&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;return true;
}
std::shared_ptr<BnwEngineUiTurnOwner> BnwEngineUiTurnOwner::retain_for_actual_policy_ui(
 const std::shared_ptr<const LiveUiHost>& supplied,std::shared_ptr<const SelectedBnwPreviewHost> host){
 need(supplied&&host,"Policy notification actual UiHost/selected Host absent");
 need(supplied->plot_yield_ui_thread_==std::this_thread::get_id(),"Policy notification foreign UI creator");
 const auto ui=supplied->selected_owner_lifetime_.lock();
 need(ui&&ui.get()==supplied.get()&&!ui.owner_before(supplied)&&!supplied.owner_before(ui),"Policy notification forged UiHost control");
 auto owner=ui->engine_ui_turn_owner_;
 if(!owner){owner=bind(*ui,host);ui->engine_ui_turn_owner_=owner;}
 const auto actual=owner->impl_->host.lock();
 need(actual&&actual.get()==host.get()&&!actual.owner_before(host)&&!host.owner_before(actual),"Policy notification different Engine Host");
 owner->require_current();return owner;
}
void BnwEngineUiTurnOwner::consume_policy_process_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Policy dirty foreign Engine creator");
 // Pure canonical controls only: SourceOwner already reserved the once operation
 // before retaining this callbackful Engine owner. No wrapper return is a grant.
 p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
 need(selection&&world&&world.get()==&state,"Policy dirty actual Engine Selection/world absent");
 selection->consume_policy_process_dirty(policy,state,reserved);
 p->identity();selection->require_policy_process_dirty_return(*policy);
}
void BnwEngineUiTurnOwner::consume_free_policy_notification(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,
 runtime::BnwSelectedStockLuaBootstrap& bootstrap){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Policy notification foreign Engine creator");
 const auto host=p->host.lock();const auto world=p->world.lock();
 need(host&&world&&world.get()==&state,"Policy notification Engine SAME world absent");
 // Reserve the genuine original operation once before allocating text/rows
 // or invoking any selected-source currentness provider.
 const auto lease=policy->begin_free_policy_notification_dispatch(*host,state);
 const auto serial=lease.operation_serial(),revision=state.ui_source_revision;
 try{
  require_current();policy->require_free_policy_notification_source(*host,state);lease.require_operation(serial);
  const auto actor=summary_i32(policy->player());
  auto& native=engine_notification_ring(state,actor);
  const auto present=engine_notification_field(native,"has_notifications");
  need(present==0||present==1,"Policy notification nullable native owner invalid");
  bool inserted=false;
  if(present){
   // CvPlayer::ChangeNumFreePolicies: GetNotifications BEFORE the two
   // genuine selected TextData lookups; no localized text on null branch.
   bootstrap.bind_engine_ui_localization_owned(*this,lease);
   Impl::FormattedDeficit formatted;formatted.actor=actor;formatted.turn=summary_i32(state.setup.game_turn);
   formatted.type=0x1e4417c5; // CvEnums.h NOTIFICATION_FREE_POLICY
   formatted.generation=p->generation;formatted.revision=revision;
   formatted.message=selected_localization_text("TXT_KEY_NOTIFICATION_FREE_POLICY");
   formatted.summary=selected_localization_text("TXT_KEY_NOTIFICATION_FREE_POLICY_SUMMARY");
   inserted=add_free_policy_native(state,formatted.message,formatted.summary,policy);
  }
  // Capture actual native return, not a caller bool or a reconstructed
  // success record. A nullable/redundant/nonhuman/debug return is unchanged.
  if(!inserted)policy->require_free_policy_notification_source(*host,state);
  // A stored Add earns +1 before SourceOwnerAccept. Do not invoke the old
  // lease/current-revision provider in that interval. Native pre-store source
  // closure and this private actual return seal authenticate the successor.
  p->identity();
  need(state.ui_source_revision==revision+(inserted?1u:0u),"Policy notification native revision differs");
  Impl::PolicyNotificationReturn receipt;
  receipt.source=policy;receipt.world=world;receipt.actor=actor;receipt.generation=p->generation;
  receipt.before=revision;receipt.after=state.ui_source_revision;receipt.serial=serial;
  receipt.ring=engine_notification_ring(state,actor);receipt.flags=state.notification_load_transients;
  receipt.effects=p->notification_effects;receipt.turn=state.setup.game_turn;
  p->policy_notification_return=std::move(receipt);
  policy->accept_actual_free_policy_notification_return(*this);
  lease.require_operation(serial);require_free_policy_notification_return(*policy);
 }catch(...){policy->fail_free_policy_notification_dispatch();p->policy_notification_return.reset();throw;}
}
void BnwEngineUiTurnOwner::require_free_policy_notification_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->policy_notification_return.has_value(),"Policy notification genuine native return absent/foreign");
 const auto& receipt=*p->policy_notification_return;const auto actual=receipt.source.lock();const auto world=receipt.world.lock();
 need(actual&&actual.get()==&source&&world&&p->world.lock().get()==world.get()&&receipt.generation==p->generation&&
  receipt.after>=receipt.before&&receipt.after-receipt.before<=1&&world->ui_source_revision==receipt.after&&world->setup.game_turn==receipt.turn,
  "Policy notification native return source/world/revision differs");
 p->identity();
 need(engine_notification_same(engine_notification_ring(*world,receipt.actor),receipt.ring)&&
  engine_notification_flags_same(world->notification_load_transients,receipt.flags)&&p->notification_effects==receipt.effects,
  "Policy notification native return ring/DLL effects changed");
}

}

namespace civ5::ui {
std::vector<BnwEngineUiNotificationEffect> LiveUiHost::engine_ui_notifications()const{
 if(!engine_ui_turn_owner_)return {};return engine_ui_turn_owner_->queued_notifications();
}
}

namespace civ5::ui {
namespace {
bool golden_replay_message_same(const saves::ReplayMessageObservation& a,const saves::ReplayMessageObservation& b){
 if(a.turn!=b.turn||a.type!=b.type||a.player!=b.player||a.text!=b.text||a.plots.size()!=b.plots.size())return false;
 for(std::size_t n=0;n<a.plots.size();++n)if(a.plots[n].x!=b.plots[n].x||a.plots[n].y!=b.plots[n].y)return false;return true;
}
bool golden_replay_sequence_same(const std::vector<saves::ReplayMessageObservation>& a,const std::vector<saves::ReplayMessageObservation>& b){
 if(a.size()!=b.size())return false;for(std::size_t n=0;n<a.size();++n)if(!golden_replay_message_same(a[n],b[n]))return false;return true;
}
std::string golden_native_adjective_key(const runtime::LiveSessionState& state,const SelectedBnwPreviewHost& host,std::int32_t actor){
 need(actor>=0&&actor<63&&state.world.pregame&&state.world.all_players,"GoldenAge native player/Pregame owner absent");
 const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(actor));
 need(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&summary_player_scalar(raw,"m_eID")==actor,"GoldenAge native actor registry differs");
 // SDK getCivilizationAdjectiveKey tests minor before reading custom key.
 // Its MinorCivAI native type owner is not manufactured from Pregame strings.
 need(!summary_minor(state,actor),"GoldenAge minor adjective requires actual MinorCivAI GetMinorCivType owner");
 const auto& custom=state.world.pregame->archive.string_arrays.at("civ_adjectives");
 need(custom.size()==64,"GoldenAge actual Pregame civilization adjective slots absent");
 const auto& actual=custom.at(static_cast<std::size_t>(actor));if(!actual.empty())return actual;
 const auto& ids=state.world.pregame->slots.integer_arrays.at("civilizations");need(ids.size()==64,"GoldenAge actual Civilization slots absent");
 const auto id=summary_i32(ids.at(static_cast<std::size_t>(actor)));
 need(id>=0&&state.cities.players.at(actor).civilization==id,"GoldenAge native Civilization raw/owned pair differs");
 std::size_t found=0;std::string key;
 const auto q=host.database().query({"SELECT Adjective FROM Civilizations WHERE ID=?",{std::int64_t(id)}},[&](const auto values){
  need(values.size()==1&&std::get_if<std::string>(&values[0]),"GoldenAge genuine CivilizationInfo adjective malformed");key=std::get<std::string>(values[0]);++found;
 });need(q==found&&found==1&&!key.empty(),"GoldenAge genuine CivilizationInfo adjective absent/ambiguous");return key;
}
std::string golden_replay_format(std::string source,std::string_view language,std::string_view adjective){
 need(!source.empty()&&!adjective.empty()&&adjective.find('\0')==adjective.npos&&adjective.find('{')==adjective.npos&&adjective.find('}')==adjective.npos,
  "GoldenAge actual adjective requires unsupported native grammar");
 // Exact installed original EN textkey and RU key-reference argument forms.
 // Other grammar requires its real formatter and is not stripped or guessed.
 const std::string_view token=language=="en_US"?"{1_LongCivName:textkey}":language=="ru_RU"?"{@1_LongCivName}":"";
 need(!token.empty(),"GoldenAge selected native language formatter unsupported");
 const auto at=source.find(token);need(at!=source.npos&&source.find(token,at+token.size())==source.npos,"GoldenAge native template argument absent/ambiguous");
 source.replace(at,token.size(),adjective);
 need(source.size()<=1024*1024&&source.find('{')==source.npos&&source.find('}')==source.npos,"GoldenAge native replay template requires unresolved grammar");return source;
}
}
void BnwEngineUiTurnOwner::consume_golden_age_replay(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,
 runtime::BnwSelectedStockLuaBootstrap& bootstrap,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&!p->golden_age_replay_busy,"GoldenAge replay Engine foreign creator/reentry");
 p->golden_age_replay_busy=true;struct End{Impl& p;~End(){p.golden_age_replay_busy=false;}}end{*p};
 try{
  const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"GoldenAge replay Engine SAME canonical world absent");
  const auto serial=reserved.operation_serial(),revision=state.ui_source_revision;const auto actor=summary_i32(policy->player());const auto turn=state.setup.game_turn;
  require_current();policy->require_golden_age_replay_source(*host,state);reserved.require_operation(serial);
  need(revision<UINT64_MAX&&turn>=0&&turn<=INT32_MAX&&state.world.history.layout==saves::GameStateLayout::expansion2_version1&&state.world.history.replay.size()<INT32_MAX,
   "GoldenAge actual native replay width/layout unavailable");
  bootstrap.bind_engine_ui_localization_owned(*this,reserved);
  const auto adjective_key=golden_native_adjective_key(state,*host,actor);
  need(adjective_key.find('\0')==adjective_key.npos,"GoldenAge actual adjective contains an unsupported NUL");
  const auto adjective=adjective_key.starts_with("TXT_KEY_")?selected_localization_text(adjective_key):adjective_key;
  const auto message=golden_replay_format(selected_localization_text("TXT_KEY_NOTIFICATION_GOLDEN_AGE_BEGUN"),p->language,adjective);
  Impl::GoldenAgeReplayReturn receipt;receipt.source=policy;receipt.world=world;receipt.actor=actor;receipt.turn=turn;
  receipt.num_golden_ages=summary_player_scalar(state.world.all_players->players.at(static_cast<std::size_t>(actor)),"m_iNumGoldenAges");
  need(receipt.num_golden_ages>0,"GoldenAge actual returned history count absent");
  receipt.generation=p->generation;receipt.before=revision;receipt.after=revision+1;receipt.serial=serial;receipt.prior=state.world.history.replay;
  receipt.appended.turn=turn;receipt.appended.type=0;receipt.appended.player=actor;receipt.appended.plots.push_back({-1,-1});
  receipt.appended.text.assign(message.begin(),message.end());
  receipt.actual=receipt.prior;receipt.actual.push_back(receipt.appended);auto actual_store=receipt.actual;
  // Complete actual selected source reads and pending VM proof BEFORE store.
  require_selected_localization_current();require_current();policy->require_golden_age_replay_source(*host,state);reserved.require_operation(serial);
  need(world.get()==&state&&state.ui_source_revision==revision&&state.setup.game_turn==turn&&golden_native_adjective_key(state,*host,actor)==adjective_key&&
   summary_player_scalar(state.world.all_players->players.at(static_cast<std::size_t>(actor)),"m_iNumGoldenAges")==receipt.num_golden_ages&&
   golden_replay_sequence_same(state.world.history.replay,receipt.prior),"GoldenAge native replay/adjective/history changed before store");
  policy->require_golden_age_replay_source(*host,state);reserved.require_operation(serial);p->identity();
  need(state.ui_source_revision==revision&&state.setup.game_turn==turn&&golden_replay_sequence_same(state.world.history.replay,receipt.prior),"GoldenAge final native replay source bookend failed");
  // CvGame::addReplayMessage major_event0 at -1,-1 always appends; no dedup.
  // Allocation was prepared before stores. No provider between store and seal.
  state.world.history.replay.swap(actual_store);state.ui_source_revision=receipt.after;receipt.stored=true;
  p->golden_age_replay_return=std::move(receipt);
  require_golden_age_replay_return(*policy);policy->accept_actual_golden_age_replay_return(*this);
  reserved.require_operation(serial);require_golden_age_replay_return(*policy);
 }catch(...){policy->fail_golden_age_replay_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_golden_age_replay_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->golden_age_replay_return,"GoldenAge actual native replay return absent/foreign");
 const auto& r=*p->golden_age_replay_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 need(actual&&actual.get()==&source&&world&&r.stored&&r.generation==p->generation&&r.before<UINT64_MAX&&r.after==r.before+1&&
  world->ui_source_revision==r.after&&world->setup.game_turn==r.turn&&source.player()==r.actor&&r.serial!=0&&r.actor>=0&&r.actor<63,
  "GoldenAge actual sealed replay source/world/turn/revision differs");
 p->identity();need(p->world.lock().get()==world.get()&&world->world.history.layout==saves::GameStateLayout::expansion2_version1&&
  r.actual.size()==r.prior.size()+1&&r.appended.type==0&&r.appended.turn==r.turn&&r.appended.player==r.actor&&r.appended.plots.size()==1&&
  r.appended.plots[0].x==-1&&r.appended.plots[0].y==-1&&!r.appended.text.empty()&&
  golden_replay_message_same(r.actual.back(),r.appended)&&golden_replay_sequence_same(world->world.history.replay,r.actual)&&
  summary_player_scalar(world->world.all_players->players.at(static_cast<std::size_t>(r.actor)),"m_iNumGoldenAges")==r.num_golden_ages,
  "GoldenAge actual replay prior prefix/append/history differs");
 for(std::size_t n=0;n<r.prior.size();++n)need(golden_replay_message_same(r.actual[n],r.prior[n]),"GoldenAge replay altered prior native history");
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::require_end_turn_queue_identity(const std::weak_ptr<const LiveUiHost>& ui,
 const std::weak_ptr<const SelectedBnwPreviewHost>& host,const std::weak_ptr<const runtime::LiveSessionState>& world)const{
 const auto p=impl_;need(p->creator==std::this_thread::get_id(),"End-turn identity foreign creator");
 if(p->end_turn_checking){p->end_turn_failed=true;throw std::runtime_error("End-turn identity provider reentry");}
 p->end_turn_checking=true;
 try{
  const auto self=p->end_turn_self.lock();const auto u=p->ui.lock();const auto h=p->host.lock();const auto w=p->world.lock();
  const auto pure=[&]{
   need(p->end_turn_alive&&!p->end_turn_failed&&self&&u&&h&&w&&self.get()==this&&
    !ui.owner_before(p->ui)&&!p->ui.owner_before(ui)&&!host.owner_before(p->host)&&!p->host.owner_before(host)&&
    !world.owner_before(p->world)&&!p->world.owner_before(world)&&
    u->engine_ui_turn_owner_.get()==this&&!u->engine_ui_turn_owner_.owner_before(self)&&!self.owner_before(u->engine_ui_turn_owner_)&&
    u->selected_owner_lifetime_.lock().get()==u.get(),"End-turn actual UI/Engine/Host/world controls retired/substituted");
   // Exactly one lock in Queue::identity and one lock in this hook. No stored
   // strong descriptor or callback argument is counted as an outside facade.
   need(p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
    p->end_turn_world_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
    self.use_count()>2+static_cast<long>(p->end_turn_pins)&&u.use_count()>2+static_cast<long>(p->end_turn_ui_pins)&&
    h.use_count()>2&&w.use_count()>2+static_cast<long>(p->end_turn_world_pins),
    "End-turn canonical outside owner lost during provider bookend");
  };
  pure();p->identity();pure();
  std::function<bool()> current=p->rules_current;pure();p->identity();pure();
  const bool valid=current();pure();p->identity();pure();
  current={};pure();p->identity();pure();need(valid,"End-turn actual rules provider retired");
  p->end_turn_checking=false;
 }catch(...){p->end_turn_failed=true;p->end_turn_checking=false;throw;}
}
void BnwEngineUiTurnOwner::ensure_end_turn_queue_owned(){
 const auto p=impl_;need(p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"End-turn owner retired");
 auto self=p->end_turn_self.lock();auto ui=p->ui.lock();auto host=p->host.lock();
 need(self&&self.get()==this&&ui&&host&&ui->engine_ui_turn_owner_.get()==this,
  "End-turn queue must bind AFTER canonical UI Engine member publication");
 if(!p->end_turn_queue)p->end_turn_queue=BnwEngineUiEndTurnQueue::bind_fresh(std::move(self),std::move(ui),std::move(host));
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn owner retired while binding queue");
}
void BnwEngineUiTurnOwner::attach_end_turn_consumer_owned(std::shared_ptr<const BnwEngineUiEndTurnQueue::Consumer> consumer,
 const runtime::BnwStockLuaAttemptLease& source){
 ensure_end_turn_queue_owned();const auto p=impl_;p->end_turn_queue->attach_consumer(*this,std::move(consumer),source);
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn consumer attach retired Engine");
}
void BnwEngineUiTurnOwner::drain_end_turn_queue_owned(const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& source){
 const auto p=impl_;pump.require_for_source(source);
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn actual canonical Engine retired before drain");
 // A fresh canonical Engine can have no queue yet, or actual empty/unattached
 // storage. This returns no callback receipt and certifies no activation.
 ensure_end_turn_queue_owned(); // Binds only after actual canonical UI member publication.
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn idle binding retired facade");
 if(!p->end_turn_queue->has_pending_owned()){
  pump.require_for_source(source);
  need(p->end_turn_alive&&!p->end_turn_failed,"End-turn idle queue read retired Engine");return;}
 BnwEngineUiEndTurnQueue::PumpLoan specialized(*p->end_turn_queue,source,pump);
 p->end_turn_queue->drain(specialized,source);pump.require_for_source(source);
 need(p->end_turn_alive&&!p->end_turn_failed,"End-turn Engine retired during pump");
}
}

namespace civ5::ui {
std::weak_ptr<BnwEngineUiTurnOwner> BnwEngineUiTurnOwner::end_turn_engine_for_bootstrap_owned()const{
 const auto p=impl_;need(p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,
  "End-turn Bootstrap Engine publisher retired");
 const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
 need(self&&self.get()==this&&ui&&ui->engine_ui_turn_owner_.get()==this&&
  !ui->engine_ui_turn_owner_.owner_before(self)&&!self.owner_before(ui->engine_ui_turn_owner_),
  "End-turn Bootstrap publisher is not the actual canonical UI member");
 return std::const_pointer_cast<BnwEngineUiTurnOwner>(self);
}
}

namespace civ5::ui {
struct BnwEngineUiTurnOwner::EndTurnPin final {
 std::shared_ptr<BnwEngineUiTurnOwner> engine;std::shared_ptr<Impl> storage;
 explicit EndTurnPin(const std::weak_ptr<BnwEngineUiTurnOwner>& weak):engine(weak.lock()){
  need(bool(engine),"End-turn actual weak Engine expired");storage=engine->impl_;
  need(storage->creator==std::this_thread::get_id(),"End-turn counted borrower foreign creator");
  need(storage->end_turn_alive&&!storage->end_turn_failed&&storage->end_turn_pins<static_cast<std::size_t>(LONG_MAX-2),
   "End-turn counted borrower retired/exhausted");++storage->end_turn_pins;
 }
 ~EndTurnPin(){engine.reset();--storage->end_turn_pins;}
 EndTurnPin(const EndTurnPin&)=delete;EndTurnPin&operator=(const EndTurnPin&)=delete;
};
#include "detail/bnw_system_font_language_data_core.inc"
void BnwEngineUiTurnOwner::drain_end_turn_queue_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>& weak,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& source){
 pump.require_for_source(source);EndTurnPin pin(weak);
 pin.engine->drain_end_turn_queue_owned(pump,source);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"End-turn counted borrower retired during drain");
 pump.require_for_source(source);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"End-turn counted borrower retired before return");
}
}

namespace civ5::ui {
BnwEngineUiTurnOwner::EndTurnCursorPin::EndTurnCursorPin(const BnwEngineUiTurnOwner& engine,
 const LiveUiHost& ui,const runtime::LiveSessionState& world):storage_(engine.impl_){
 const auto& p=storage_;need(p&&p->creator==std::this_thread::get_id(),"End-turn Cursor borrower foreign creator");
 const auto actual_engine=p->end_turn_self.lock();const auto actual_ui=p->ui.lock();const auto actual_world=p->world.lock();
 need(p->end_turn_alive&&!p->end_turn_failed&&actual_engine&&actual_engine.get()==&engine&&actual_ui&&actual_ui.get()==&ui&&
  actual_world&&actual_world.get()==&world&&ui.engine_ui_turn_owner_.get()==&engine&&
  !ui.engine_ui_turn_owner_.owner_before(actual_engine)&&!actual_engine.owner_before(ui.engine_ui_turn_owner_)&&
  !ui.selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui.selected_owner_lifetime_),
  "End-turn Cursor borrower differs from actual canonical Engine/UI/world");
 need(p->end_turn_pins<static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<static_cast<std::size_t>(LONG_MAX-2)&&
  p->end_turn_world_pins<static_cast<std::size_t>(LONG_MAX-2),"End-turn Cursor borrower count exhausted");
 ++p->end_turn_pins;++p->end_turn_ui_pins;++p->end_turn_world_pins;
}
BnwEngineUiTurnOwner::EndTurnCursorPin::~EndTurnCursorPin(){
 if(storage_->creator!=std::this_thread::get_id())std::terminate();
 --storage_->end_turn_pins;--storage_->end_turn_ui_pins;--storage_->end_turn_world_pins;
}
}

namespace civ5::ui {
bool BnwEngineUiTurnOwner::add_golden_age_native(runtime::LiveSessionState& state,std::string_view message_input,std::string_view summary_input,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&policy,"Free policy native Add SAME world/source absent");
 policy->require_golden_age_notification_source(*host,state);
 Impl::FormattedDeficit formatted;formatted.actor=summary_i32(policy->player());formatted.turn=summary_i32(state.setup.game_turn);formatted.type=policy->golden_age_operands()[7]>0?0x3b74383cU:0x5e2fd815U;
 formatted.generation=p->generation;formatted.revision=state.ui_source_revision;formatted.message=std::string(message_input);formatted.summary=std::string(summary_input);
 const auto revision=state.ui_source_revision;
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&(formatted.type==0x3b74383cU||formatted.type==0x5e2fd815U),
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){p->identity();return false;}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){p->identity();return false;}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0&&lookup<INT32_MAX&&revision<UINT64_MAX,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 // Original GoldenAge begun/ended types have no IsNotificationRedundant
 // switch case: the actual default returns false, even same prior messages.
 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 const auto auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 const auto active=runtime::detail::bnw_game_active_player_read(state);
 const auto game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
 need(game_type==state.setup.game_type&&game_type>=0&&game_type<=3,"Engine Add source game type differs");
 const bool broadcast=initialized&&actor==active&&(game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=p->notification_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,turn_active&&auto_moves}; // GoldenAge types expire at end turn: actual auto-moves branch waits extra turn
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 require_selected_localization_current();require_current();
 policy->require_golden_age_notification_source(*host,state);p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&state.ui_source_revision==revision&&policy->player()==actor&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&summary_player_bool(state,actor,"m_bAutoMoves")==auto_moves&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&runtime::detail::bnw_game_active_player_read(state)==active&&
      state.world.pregame->archive.integers.at("game_type")==game_type&&state.setup.game_type==game_type&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;return true;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_golden_age_gameplay(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->golden_age_gameplay_busy&&!p->golden_age_pump_busy,"GoldenAge gameplay foreign Engine/reentry");
 p->golden_age_gameplay_busy=true;struct End{Impl& p;~End(){p.golden_age_gameplay_busy=false;}}end{*p};
 try{
  const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"GoldenAge gameplay canonical Host/world absent");
  const auto serial=reserved.operation_serial(),revision=state.ui_source_revision;
  require_current();policy->require_golden_age_gameplay_source(*host,state);reserved.require_operation(serial);
  need(revision<UINT64_MAX&&p->golden_age_gameplay_events.size()<UINT64_MAX,"GoldenAge actual gameplay queue/revision exhausted");
  const auto operands=policy->golden_age_operands();const auto actor=summary_i32(policy->player());
  need((operands[6]>0)!=(operands[7]>0)&&state.cities.players.at(actor).golden_age==(operands[7]>0),"GoldenAge gameplay actual changed state differs");
  Impl::GoldenAgeGameplayReturn r;r.source=policy;r.world=world;r.generation=p->generation;r.before=revision;r.after=revision+1;r.serial=serial;r.prior=p->golden_age_gameplay_events;
  r.appended={operands[7]>0?BnwGoldenAgeGameplayKind::started:BnwGoldenAgeGameplayKind::ended,actor,summary_i32(state.setup.game_turn),p->generation,r.after,static_cast<std::uint64_t>(r.prior.size())+1};
  r.actual=r.prior;r.actual.push_back(r.appended);auto actual_store=r.actual;
  require_current();policy->require_golden_age_gameplay_source(*host,state);reserved.require_operation(serial);p->identity();
  need(state.ui_source_revision==revision&&world.get()==&state&&policy->golden_age_operands()==operands&&p->golden_age_gameplay_events==r.prior&&state.setup.game_turn==r.appended.turn,
   "GoldenAge actual standalone gameplay source changed before queue store");
  // Concrete standalone facade producer: one retained typed FIFO payload.
  // Never invokes Lua/audio or original DLL callbacks during native mutation.
  p->golden_age_gameplay_events.swap(actual_store);state.ui_source_revision=r.after;p->golden_age_gameplay_return=std::move(r);
  require_golden_age_gameplay_return(*policy);policy->accept_actual_golden_age_gameplay_return(*this);
  reserved.require_operation(serial);require_golden_age_gameplay_return(*policy);
 }catch(...){policy->fail_golden_age_gameplay_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_golden_age_gameplay_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->golden_age_gameplay_return,"GoldenAge actual gameplay return absent/foreign");
 const auto& r=*p->golden_age_gameplay_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 need(actual&&actual.get()==&source&&world&&r.generation==p->generation&&r.before<UINT64_MAX&&r.after==r.before+1&&r.serial!=0&&world->ui_source_revision==r.after&&
  r.actual.size()==r.prior.size()+1&&r.appended.actor==source.player()&&r.appended.generation==r.generation&&r.appended.world_revision==r.after&&
  r.appended.turn==world->setup.game_turn&&r.appended.occurrence==r.actual.size()&&r.actual.back()==r.appended&&p->golden_age_gameplay_events==r.actual,
  "GoldenAge actual typed FIFO return source/world/payload changed");
 p->identity();need(p->world.lock().get()==world.get(),"GoldenAge gameplay canonical world retired");
 for(std::size_t n=0;n<r.prior.size();++n)need(r.actual[n]==r.prior[n],"GoldenAge gameplay changed earlier FIFO history");
 const auto operands=source.golden_age_operands();need((operands[6]>0)!=(operands[7]>0)&&
  r.appended.kind==(operands[7]>0?BnwGoldenAgeGameplayKind::started:BnwGoldenAgeGameplayKind::ended),"GoldenAge actual facade kind differs from native transition");
}
void BnwEngineUiTurnOwner::consume_golden_age_notification(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::BnwSelectedStockLuaBootstrap& bootstrap,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->golden_age_notification_busy&&!p->golden_age_pump_busy,"GoldenAge notification foreign Engine/reentry");
 p->golden_age_notification_busy=true;struct End{Impl& p;~End(){p.golden_age_notification_busy=false;}}end{*p};
 try{
  const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"GoldenAge notification canonical world absent");
  const auto serial=reserved.operation_serial(),revision=state.ui_source_revision;const auto actor=summary_i32(policy->player());
  require_current();policy->require_golden_age_notification_source(*host,state);reserved.require_operation(serial);
  const auto present=engine_notification_field(engine_notification_ring(state,actor),"has_notifications");
  need(present==0||present==1,"GoldenAge actual nullable notification owner invalid");bool inserted=false;
  if(present){
   bootstrap.bind_engine_ui_localization_owned(*this,reserved);const bool begun=policy->golden_age_operands()[7]>0;
   const auto message=selected_localization_text(begun?"TXT_KEY_NOTIFICATION_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER":"TXT_KEY_NOTIFICATION_GOLDEN_AGE_ENDED_ACTIVE_PLAYER");
   const auto summary=selected_localization_text(begun?"TXT_KEY_NOTIFICATION_SUMMARY_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER":"TXT_KEY_NOTIFICATION_SUMMARY_GOLDEN_AGE_ENDED_ACTIVE_PLAYER");
   need(message.find('{')==message.npos&&message.find('}')==message.npos&&summary.find('{')==summary.npos&&summary.find('}')==summary.npos,"GoldenAge actual notification text requires unsupported grammar");
   inserted=add_golden_age_native(state,message,summary,policy);
  }
  if(!inserted)policy->require_golden_age_notification_source(*host,state);
  p->identity();need(state.ui_source_revision==revision+(inserted?1u:0u),"GoldenAge actual Add mutation revision differs");
  Impl::PolicyNotificationReturn r;r.source=policy;r.world=world;r.actor=actor;r.generation=p->generation;r.before=revision;r.after=state.ui_source_revision;r.serial=serial;r.turn=state.setup.game_turn;
  r.ring=engine_notification_ring(state,actor);r.flags=state.notification_load_transients;r.effects=p->notification_effects;p->golden_age_notification_return=std::move(r);
  require_golden_age_notification_return(*policy);policy->accept_actual_golden_age_notification_return(*this);
  reserved.require_operation(serial);require_golden_age_notification_return(*policy);
 }catch(...){policy->fail_golden_age_notification_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_golden_age_notification_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->golden_age_notification_return,"GoldenAge actual notification return absent/foreign");
 const auto& r=*p->golden_age_notification_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 need(actual&&actual.get()==&source&&world&&r.generation==p->generation&&r.after>=r.before&&r.after-r.before<=1&&world->ui_source_revision==r.after&&world->setup.game_turn==r.turn&&r.actor==source.player()&&r.serial!=0,
  "GoldenAge actual native Add return source/world/revision differs");
 p->identity();need(p->world.lock().get()==world.get()&&engine_notification_same(engine_notification_ring(*world,r.actor),r.ring)&&
  engine_notification_flags_same(world->notification_load_transients,r.flags)&&p->notification_effects==r.effects,"GoldenAge actual notification ring/transients/publication queue changed");
}
std::vector<BnwEngineUiGoldenAgeGameplayEvent> BnwEngineUiTurnOwner::queued_golden_age_gameplay_events()const{
 require_current();const auto events=impl_->golden_age_gameplay_events;require_current();return events;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::drain_golden_age_gameplay_owned(
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& lease){
 const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->golden_age_pump_busy&&
  !p->golden_age_gameplay_busy&&!p->golden_age_notification_busy&&!p->reformation_notification_busy,
  "GoldenAge actual client pump creator/reentry failed");
 p->golden_age_pump_busy=true;
 struct End{Impl& p;~End(){p.golden_age_pump_busy=false;}}end{*p};
 // This real camera lender is issued only after the native input batch and
 // borrowed mutation scopes unwind. An ordinary source operation is insufficient.
 pump.require_for_source(lease);const auto serial=lease.operation_serial();
 p->identity();const auto actual_ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();
 const auto loan_ui=pump.ui_.lock();const auto loan_host=pump.lineage_.host.lock();const auto loan_world=pump.lineage_.world.lock();
 need(actual_ui&&host&&world&&loan_ui&&loan_host&&loan_world&&
  actual_ui.get()==loan_ui.get()&&!actual_ui.owner_before(loan_ui)&&!loan_ui.owner_before(actual_ui)&&
  host.get()==loan_host.get()&&!host.owner_before(loan_host)&&!loan_host.owner_before(host)&&
  world.get()==loan_world.get()&&!world.owner_before(loan_world)&&!loan_world.owner_before(world)&&
  pump.lineage_.generation==p->generation,"GoldenAge pump SAME actual Ui/Host/world/generation differs");
 need(p->golden_age_gameplay_delivered<=p->golden_age_gameplay_events.size()&&
  actual_ui->applied_golden_age_events_.size()==p->golden_age_gameplay_delivered&&
  std::equal(actual_ui->applied_golden_age_events_.begin(),actual_ui->applied_golden_age_events_.end(),p->golden_age_gameplay_events.begin()),
  "GoldenAge actual client FIFO prior prefix changed");
 for(std::size_t n=p->golden_age_gameplay_delivered;n<p->golden_age_gameplay_events.size();++n){
  const auto& event=p->golden_age_gameplay_events[n];
  need(event.generation==p->generation&&event.occurrence==static_cast<std::uint64_t>(n)+1&&
   event.actor>=0&&event.actor<63&&event.turn>=0&&event.world_revision<=world->ui_source_revision,
   "GoldenAge actual queued event lineage/order invalid");
 }
 need(p->golden_age_notification_audio_delivered<=p->notification_effects.size(),
  "GoldenAge notification publication cursor regressed");
 for(std::size_t n=p->golden_age_notification_audio_delivered;n<p->notification_effects.size();++n){
  const auto& effect=p->notification_effects[n];
  need(effect.generation==p->generation&&effect.occurrence==static_cast<std::uint64_t>(n)+1&&
   effect.world_revision<=world->ui_source_revision,"GoldenAge notification actual publication lineage invalid");
 }
 // Allocation and every callbackful source bookend precede actual client store.
 auto applied=p->golden_age_gameplay_events;
 const auto staged_effects=p->notification_effects;const auto audio_before=p->golden_age_notification_audio_delivered;
 const auto staged_extent=applied.size();
 pump.require_for_source(lease);lease.require_operation(serial);p->identity();
 need(p->golden_age_gameplay_events==applied&&p->notification_effects==staged_effects&&
  p->golden_age_notification_audio_delivered==audio_before&&
  actual_ui->applied_golden_age_events_.size()==p->golden_age_gameplay_delivered&&
  std::equal(actual_ui->applied_golden_age_events_.begin(),actual_ui->applied_golden_age_events_.end(),applied.begin()),
  "GoldenAge client staged FIFO/publication/prefix changed during source bookends");
 actual_ui->applied_golden_age_events_.swap(applied);
 p->golden_age_gameplay_delivered=staged_extent;
 // Only an actual Add publication emits a cue. All notification/popup paths
 // join the SAME UI-owned monotonic audio issuer; ring/FIFO indices never do.
 // Reserve the consumed publication before callbacks; failures retain history
 // and cannot replay the sound on a later pump. Physical playback is separate.
 while(p->golden_age_notification_audio_delivered<staged_effects.size()){
  const auto effect=staged_effects[p->golden_age_notification_audio_delivered];
  ++p->golden_age_notification_audio_delivered;
  if(effect.kind!=BnwEngineUiNotificationEffectKind::add_notification)continue;
  if(effect.type==0x3b74383cU)
   (void)actual_ui->publish_civis_audio_cue(UiAudioKind::notification,"NOTIFICATION_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER");
  else if(effect.type==0x5e2fd815U)
   (void)actual_ui->publish_civis_audio_cue(UiAudioKind::notification,"NOTIFICATION_GOLDEN_AGE_ENDED_ACTIVE_PLAYER");
  else if(effect.type==0x489b80b6U)
   (void)actual_ui->publish_civis_audio_cue(UiAudioKind::notification,"NOTIFICATION_FREE_TECH");
  else if(effect.type==0x050b6287U)
   (void)actual_ui->publish_civis_audio_cue(UiAudioKind::notification,"NOTIFICATION_ADD_REFORMATION_BELIEF");
 }
 p->identity();pump.require_for_source(lease);lease.require_operation(serial);
}
std::vector<BnwEngineUiGoldenAgeGameplayEvent> LiveUiHost::engine_ui_applied_golden_age_events()const{
 need(plot_yield_ui_thread_==std::this_thread::get_id(),"GoldenAge client event getter foreign creator");
 const auto owner=engine_ui_turn_owner_;
 if(!owner){need(applied_golden_age_events_.empty(),"GoldenAge client events outlived their actual owner");return {};}
 (void)owner->queued_golden_age_gameplay_events();auto result=applied_golden_age_events_;
 (void)owner->queued_golden_age_gameplay_events();return result;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::attach_end_turn_consumer_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,
 std::shared_ptr<const BnwEngineUiEndTurnQueue::Consumer> consumer,
 const runtime::BnwStockLuaAttemptLease& source){
 const auto serial=source.operation_serial();source.require_operation(serial);
 EndTurnPin pin(weak);
 // EndTurnPin explicitly counts this temporary strong borrow during every
 // actual queue/consumer/source provider guard. It never owns a stored lease.
 pin.engine->attach_end_turn_consumer_owned(std::move(consumer),source);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,
  "End-turn counted weak attach retired before source bookend");
 source.require_operation(serial);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,
  "End-turn counted weak attach retired during source bookend");
}
void BnwEngineUiTurnOwner::drain_golden_age_gameplay_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& source){
 pump.require_for_source(source);const auto serial=source.operation_serial();
 EndTurnPin pin(weak);
 pin.engine->drain_golden_age_gameplay_owned(pump,source);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,
  "GoldenAge counted weak pump retired after actual client consume");
 pump.require_for_source(source);source.require_operation(serial);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,
  "GoldenAge counted weak pump retired during source bookend");
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::require_counted_localization_owner(const EndTurnPin& pin){
 const auto p=pin.storage;
 need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,
  "Engine selected locale counted owner retired/foreign");
 const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
 need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
  !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
  !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
  p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
  p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
  self.use_count()>1+static_cast<long>(p->end_turn_pins)&&
  ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
  "Engine selected locale genuine outside owner lost during callback");
 p->identity();
}
std::string BnwEngineUiTurnOwner::selected_localization_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,std::string_view key){
 EndTurnPin pin(weak);require_counted_localization_owner(pin);
 need(!key.empty()&&key.find('\0')==key.npos,"SelectedText invalid key");
 // Existing source validates selected raw DB/TextData and expands actual
 // text-key references. This zero-argument supplier never invents grammar.
 auto result=pin.engine->selected_localization_text(key);
 require_counted_localization_owner(pin);
 need(result.find('{')==result.npos&&result.find('}')==result.npos,
  "SelectedText requires an actual argument/grammar formatter");
 pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
 return result;
}
#include "detail/bnw_gameinfo_action_lookup_empty_core.inc"
#include "detail/bnw_selected_language_extended_font_core.inc"
runtime::ScriptLanguage BnwEngineUiTurnOwner::selected_localization_language_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak){
 EndTurnPin pin(weak);require_counted_localization_owner(pin);
 pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
 const auto p=pin.storage;const auto language=p->locale_scope->locale().language;
 need(language==p->language&&(language=="en_US"||language=="ru_RU"),"SelectedLanguage actual bound scope differs");
 const auto key="Assets/Gameplay/XML/NewText/"+std::string(language=="ru_RU"?"Russian.xml":"English.xml");
 const auto& resources=p->locale_scope->resources();auto resolved=resources.resolve(assets::AssetKey(key));
 if(std::holds_alternative<std::monostate>(resolved.asset))resolved=resources.resolve(assets::AssetKey(assets::resource_basename(key)));
 const auto* metadata=std::get_if<assets::AssetHandle>(&resolved.asset);
 need(metadata,"SelectedLanguage actual bound language metadata missing/ambiguous");
 const auto bytes=metadata->read_bytes(1024*1024);const auto metadata_sha=runtime::identify_bytes(bytes).sha256;
 const auto document=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));const auto* languages=document.child("Languages");
 need(document.name=="GameData"&&languages&&languages->children.size()==1&&languages->children.front().name=="Row",
  "SelectedLanguage source metadata row shape differs");
 const auto& row=languages->children.front();const auto* type=row.child("Type");const auto* name=row.child("Name");
 need(type&&name&&type->text==language&&!name->text.empty()&&name->text.find('\0')==name->text.npos,
  "SelectedLanguage actual Type/Name metadata invalid");
 runtime::ScriptLanguage result;std::size_t count=0;
 const auto rows=p->locale_database->query({"SELECT Type,Name FROM Localization.Languages WHERE Type=?",{language}},[&](const auto values){
  need(values.size()==2&&std::get_if<std::string>(&values[0])&&std::get_if<std::string>(&values[1]),
   "SelectedLanguage actual database Type/Name malformed");
  result={std::get<std::string>(values[0]),std::get<std::string>(values[1])};++count;
 });
 need(rows==count&&count==1&&result.type==language&&result.display_name==name->text,
  "SelectedLanguage database display_name differs from actual selected metadata");
 require_counted_localization_owner(pin);pin.engine->require_selected_localization_current();
 need(runtime::identify_bytes(metadata->read_bytes(1024*1024)).sha256==metadata_sha,
  "SelectedLanguage source changed across actual metadata query");
 require_counted_localization_owner(pin);return result;
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::publish_game_turn_changed_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,runtime::BnwRetainedTurnCursor& cursor,runtime::LiveSessionState& state,
 const runtime::BnwSelectedStockLuaSourceOwner& owner,const runtime::BnwStockLuaAttemptLease& lease){
 EndTurnPin pin(weak);pin.engine->publish_game_turn_changed_owned(cursor,state,owner,lease);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"GameplayTurnChanged producer retired actual Engine");
}
void BnwEngineUiTurnOwner::publish_game_turn_changed_owned(runtime::BnwRetainedTurnCursor& cursor,
 runtime::LiveSessionState& state,const runtime::BnwSelectedStockLuaSourceOwner& source,const runtime::BnwStockLuaAttemptLease& lease){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&!p->game_turn_changed_producing&&!p->game_turn_changed_pumping,
  "GameplayTurnChanged actual producer foreign/reentered");
 p->game_turn_changed_producing=true;struct End{Impl& p;~End(){p.game_turn_changed_producing=false;}}end{*p};
 const auto serial=lease.operation_serial();const auto before=state.ui_source_revision;require_current();lease.require_operation(serial);
 const auto actual_ui=p->ui.lock();const auto world=p->world.lock();const auto canonical=cursor.retain_engine_factory_owner();
 need(actual_ui&&world&&world.get()==&state&&before<UINT64_MAX&&p->game_turn_changed_events.size()<UINT64_MAX,
  "GameplayTurnChanged actual canonical receiver/world/capacity absent");
 cursor.require_game_turn_gameplay_source(*actual_ui,*this,state,lease);
 Impl::GameTurnChangedReturn r;r.issuer=canonical;r.world=world;r.generation=p->generation;r.before=before;r.after=before+1;r.serial=serial;
 r.prior=p->game_turn_changed_events;r.actual=r.prior;
 r.actual.push_back({summary_i32(state.setup.game_turn),p->generation,r.after,static_cast<std::uint64_t>(r.prior.size())+1});auto staged=r.actual;
 require_current();lease.require_operation(serial);cursor.require_game_turn_gameplay_source(*actual_ui,*this,state,lease);
 need(state.ui_source_revision==before&&p->game_turn_changed_events==r.prior,"GameplayTurnChanged source/FIFO changed before store");
 // Actual standalone façade publication. No Lua/audio/renderer callbacks in
 // this native mutation interval. Original DLL byte parity remains separate.
 p->game_turn_changed_events.swap(staged);state.ui_source_revision=r.after;p->game_turn_changed_return=std::move(r);
 cursor.accept_actual_game_turn_gameplay_return(*this,source,lease); // Pure owner adoption first.
 lease.require_operation(serial);require_current();require_game_turn_changed_return(cursor,state,before,serial,p->generation);
}
void BnwEngineUiTurnOwner::require_game_turn_changed_return(const runtime::BnwRetainedTurnCursor& source,
 const runtime::LiveSessionState& state,std::uint64_t before,std::uint64_t serial,std::uint64_t generation)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->game_turn_changed_return,"GameplayTurnChanged actual return absent/foreign");
 const auto& r=*p->game_turn_changed_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&world&&world.get()==&state&&p->world.lock()==world&&r.generation==generation&&generation==p->generation&&
  r.before==before&&before<UINT64_MAX&&r.after==before+1&&state.ui_source_revision==r.after&&r.serial==serial&&serial!=0&&
  r.actual.size()==r.prior.size()+1&&r.actual==p->game_turn_changed_events&&r.actual.back().turn==state.setup.game_turn&&
  r.actual.back().generation==generation&&r.actual.back().world_revision==r.after&&r.actual.back().occurrence==r.actual.size()&&
  std::equal(r.prior.begin(),r.prior.end(),r.actual.begin()),"GameplayTurnChanged concrete SAME source/world/FIFO return differs");
}
void BnwEngineUiTurnOwner::drain_game_turn_changed_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>& weak,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& lease){
 pump.require_for_source(lease);EndTurnPin pin(weak);pin.engine->drain_game_turn_changed_owned(pump,lease);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"GameplayTurnChanged client pump retired Engine");pump.require_for_source(lease);
}
void BnwEngineUiTurnOwner::drain_game_turn_changed_owned(const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& lease){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&!p->game_turn_changed_pumping&&!p->game_turn_changed_producing,
  "GameplayTurnChanged actual post-native pump foreign/reentered");
 p->game_turn_changed_pumping=true;struct End{Impl& p;~End(){p.game_turn_changed_pumping=false;}}end{*p};
 pump.require_for_source(lease);const auto serial=lease.operation_serial();p->identity();
 const auto ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();const auto loan_ui=pump.ui_.lock();
 const auto loan_host=pump.lineage_.host.lock();const auto loan_world=pump.lineage_.world.lock();
 need(ui&&host&&world&&loan_ui&&loan_host&&loan_world&&ui==loan_ui&&host==loan_host&&world==loan_world&&
  !ui.owner_before(loan_ui)&&!loan_ui.owner_before(ui)&&!world.owner_before(loan_world)&&!loan_world.owner_before(world)&&
  pump.lineage_.generation==p->generation,"GameplayTurnChanged actual common pump Ui/Host/world/generation differs");
 need(p->game_turn_changed_delivered<=p->game_turn_changed_events.size()&&ui->applied_game_turn_changed_events_.size()==p->game_turn_changed_delivered&&
  std::equal(ui->applied_game_turn_changed_events_.begin(),ui->applied_game_turn_changed_events_.end(),p->game_turn_changed_events.begin()),
  "GameplayTurnChanged actual client prior prefix changed");
 const auto delivered=p->game_turn_changed_delivered;const auto old_turn=ui->applied_game_turn_;auto staged=p->game_turn_changed_events;
 const auto extent=staged.size();
 for(std::size_t n=delivered;n<extent;++n)need(staged[n].turn>=0&&staged[n].generation==p->generation&&
  staged[n].world_revision<=world->ui_source_revision&&staged[n].occurrence==static_cast<std::uint64_t>(n)+1,
  "GameplayTurnChanged queued payload lineage invalid");
 pump.require_for_source(lease);lease.require_operation(serial);p->identity();
 need(p->game_turn_changed_events==staged&&p->game_turn_changed_delivered==delivered&&ui->applied_game_turn_==old_turn&&
  ui->applied_game_turn_changed_events_.size()==delivered&&
  std::equal(ui->applied_game_turn_changed_events_.begin(),ui->applied_game_turn_changed_events_.end(),staged.begin()),
  "GameplayTurnChanged client staged FIFO changed during source bookends");
 // Actual client listener state, after the real native batch unwound. A
 // pending original DLL/Lua relay is never manufactured by this application.
 if(extent!=0)ui->applied_game_turn_=staged.back().turn;
 ui->applied_game_turn_changed_events_.swap(staged);p->game_turn_changed_delivered=extent;
 pump.require_for_source(lease);lease.require_operation(serial);p->identity();
}
std::vector<BnwGameTurnChangedEvent> BnwEngineUiTurnOwner::queued_game_turn_changed_events()const{
 require_current();auto result=impl_->game_turn_changed_events;require_current();return result;
}
std::vector<BnwGameTurnChangedEvent> LiveUiHost::engine_ui_applied_game_turn_changed_events()const{
 need(plot_yield_ui_thread_==std::this_thread::get_id(),"GameplayTurnChanged client getter foreign creator");const auto owner=engine_ui_turn_owner_;
 if(!owner){need(applied_game_turn_changed_events_.empty(),"GameplayTurnChanged client outlived owner");return {};}
 (void)owner->queued_game_turn_changed_events();auto result=applied_game_turn_changed_events_;(void)owner->queued_game_turn_changed_events();return result;
}
std::optional<std::int32_t> LiveUiHost::engine_ui_applied_game_turn()const{
 (void)engine_ui_applied_game_turn_changed_events();const auto result=applied_game_turn_;(void)engine_ui_applied_game_turn_changed_events();return result;
}
}

namespace civ5::ui {
bool BnwEngineUiTurnOwner::add_free_tech_native(runtime::LiveSessionState& state,std::string_view message_input,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&policy,"Free tech native Add SAME world/source absent");
 policy->require_free_tech_notification_source(*host,state);
 Impl::FormattedDeficit formatted;formatted.actor=summary_i32(policy->player());formatted.turn=summary_i32(state.setup.game_turn);formatted.type=0x489b80b6;
 formatted.generation=p->generation;formatted.revision=state.ui_source_revision;formatted.message=std::string(message_input);formatted.summary=std::string(message_input);
 const auto revision=state.ui_source_revision;
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&formatted.type==0x489b80b6,
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){p->identity();return false;}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){p->identity();return false;}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0&&lookup<INT32_MAX&&revision<UINT64_MAX,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 // CvNotifications::IsNotificationRedundant FREE_TECH scans ONLY the
 // actual active native ring, returning on undismissed same native type.
 for(auto index:actual_active)if(engine_notification_field(before_ring.children.at(index),"m_eNotificationType_bits")==0x489b80b6){
  policy->require_free_tech_notification_source(*host,state);p->identity();
  need(state.ui_source_revision==revision&&engine_notification_same(engine_notification_ring(state,actor),before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags),"Free tech redundant native branch changed");return false;
 }

 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 const auto auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 const auto active=runtime::detail::bnw_game_active_player_read(state);
 const auto game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
 need(game_type==state.setup.game_type&&game_type>=0&&game_type<=3,"Engine Add source game type differs");
 const bool broadcast=initialized&&actor==active&&(game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=p->notification_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 engine_notification_field(inserted,"m_iGameDataIndex")=policy->free_tech_choice_operands()[0]; // actual iDiscover; default iTechJustDiscovered is NO_TECH
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,false}; // FREE_TECH never end-of-turn expires; wait-extra-turn is false
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 require_selected_localization_current();require_current();
 policy->require_free_tech_notification_source(*host,state);p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&state.ui_source_revision==revision&&policy->player()==actor&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&summary_player_bool(state,actor,"m_bAutoMoves")==auto_moves&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&runtime::detail::bnw_game_active_player_read(state)==active&&
      state.world.pregame->archive.integers.at("game_type")==game_type&&state.setup.game_type==game_type&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;return true;
}
}

namespace civ5::ui {
namespace {
std::int32_t free_tech_native_count(const runtime::LiveSessionState& state,std::int32_t actor){
 need(actor>=0&&actor<22&&state.world.all_players&&state.world.all_players->players.size()==64,"Free tech actual major player owner absent");
 const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(actor));
 need(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&summary_player_scalar(raw,"m_eID")==actor,"Free tech actual native player identity differs");
 const auto& field=summary_named(raw.core.fields,"m_iFreeTechCount");
 need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1,"Free tech actual native count schema differs");
 return summary_i32(field.values[0]);
}
}
void BnwEngineUiTurnOwner::consume_free_tech_text(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::BnwSelectedStockLuaBootstrap& bootstrap,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->free_tech_text_busy&&!p->golden_age_pump_busy,"Free tech text foreign Engine/reentry");
 p->free_tech_text_busy=true;struct End{Impl& p;~End(){p.free_tech_text_busy=false;}}end{*p};
 try{
  const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"Free tech text canonical Host/world absent");
  const auto serial=reserved.operation_serial(),revision=state.ui_source_revision;const auto actor=summary_i32(policy->player());
  require_current();policy->require_free_tech_text_source(*host,state);reserved.require_operation(serial);
  // The actual active child PolicyInfo is distinct from the immutable public
  // root command. Locale formatting precedes chooseTech's NO_SCIENCE query.
  const auto effect=policy->effect_policy();std::string key;std::size_t rows=0;
  const auto count=host->database().query({"SELECT Description FROM Policies WHERE ID=?",{effect}},[&](const auto values){
   need(values.size()==1&&std::get_if<std::string>(&values[0]),"Free tech actual PolicyInfo text key malformed");key=std::get<std::string>(values[0]);++rows;
  });need(count==rows&&rows==1&&key.starts_with("TXT_KEY_")&&key.find('\0')==key.npos,"Free tech actual PolicyInfo text key absent");
  bootstrap.bind_engine_ui_localization_owned(*this,reserved);
  const auto argument=selected_localization_text(key);
  const auto message=detail::format_actual_policy_free_tech_text(selected_localization_text("TXT_KEY_MISC_COMPLETED_WONDER_CHOOSE_TECH"),p->language,argument);
  Impl::FreeTechTextReturn r;r.source=policy;r.world=world;r.actor=actor;r.old_count=free_tech_native_count(state,actor);r.effect_policy=effect;
  r.turn=state.setup.game_turn;r.generation=p->generation;r.before=revision;r.serial=serial;r.description_key=std::move(key);r.message=message;
  require_selected_localization_current();require_current();policy->require_free_tech_text_source(*host,state);reserved.require_operation(serial);p->identity();
  need(state.ui_source_revision==revision&&policy->effect_policy()==effect&&state.setup.game_turn==r.turn&&free_tech_native_count(state,actor)==r.old_count,
   "Free tech actual text operands changed before private return");
  p->free_tech_text_return=std::move(r);
  require_free_tech_text_return(*policy);policy->accept_actual_free_tech_text_and_count(*this);
  // The receiver performs the real paired count store. This text receipt
  // remains valid only across that exact earned predecessor/successor pair.
  reserved.require_operation(serial);require_free_tech_text_return(*policy);
 }catch(...){policy->fail_free_tech_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_free_tech_text_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->free_tech_text_return,"Free tech actual text return absent/foreign");
 const auto& r=*p->free_tech_text_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 need(actual&&actual.get()==&source&&world&&p->world.lock().get()==world.get()&&r.generation==p->generation&&r.serial!=0&&r.actor==source.player()&&
  r.effect_policy==source.effect_policy()&&world->setup.game_turn==r.turn&&!r.description_key.empty()&&!r.message.empty(),"Free tech actual text issuer/source differs");
 const auto operands=source.free_tech_choice_operands();const auto count=free_tech_native_count(*world,r.actor);
 if(source.free_tech_count_stored()){
  need(r.before<UINT64_MAX&&world->ui_source_revision==r.before+1&&source.previous_revision()==r.before&&source.after_revision()==r.before+1&&
   operands[0]>0&&operands[1]==r.old_count&&operands[3]!=1&&static_cast<std::int64_t>(r.old_count)+operands[0]==operands[2]&&count==operands[2],
   "Free tech historical text lacks exact earned count/revision lineage");
 }else{
  need(world->ui_source_revision==r.before&&source.after_revision()==r.before&&count==r.old_count,"Free tech text unearned native count/revision change");
  if(source.free_tech_choice_returned())need(operands[0]>0&&operands[1]==r.old_count&&operands[2]==r.old_count&&operands[3]==1,
   "Free tech text no-store return lacks actual NO_SCIENCE branch");
 }
 p->identity();
}
void BnwEngineUiTurnOwner::consume_free_tech_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Free tech dirty foreign Engine creator");
 p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
 need(selection&&world&&world.get()==&state,"Free tech dirty actual canonical Selection/world absent");
 selection->consume_free_tech_dirty(policy,state,reserved);p->identity();selection->require_free_tech_dirty_return(*policy);
}
void BnwEngineUiTurnOwner::consume_free_tech_notification(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->free_tech_notification_busy&&!p->golden_age_pump_busy,"Free tech notification foreign Engine/reentry");
 p->free_tech_notification_busy=true;struct End{Impl& p;~End(){p.free_tech_notification_busy=false;}}end{*p};
 try{
  const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"Free tech notification canonical world absent");
  const auto serial=reserved.operation_serial(),revision=state.ui_source_revision;const auto actor=summary_i32(policy->player());
  require_current();policy->require_free_tech_notification_source(*host,state);reserved.require_operation(serial);require_free_tech_text_return(*policy);
  const auto present=engine_notification_field(engine_notification_ring(state,actor),"has_notifications");
  need(present==0||present==1,"Free tech actual nullable notification owner invalid");bool inserted=false;
  if(present){const auto actual_message=p->free_tech_text_return->message;inserted=add_free_tech_native(state,actual_message,policy);}
  if(!inserted)policy->require_free_tech_notification_source(*host,state);
  p->identity();need(state.ui_source_revision==revision+(inserted?1u:0u),"Free tech actual native Add revision differs");
  Impl::PolicyNotificationReturn r;r.source=policy;r.world=world;r.actor=actor;r.generation=p->generation;r.before=revision;r.after=state.ui_source_revision;r.serial=serial;r.turn=state.setup.game_turn;
  r.ring=engine_notification_ring(state,actor);r.flags=state.notification_load_transients;r.effects=p->notification_effects;p->free_tech_notification_return=std::move(r);
  require_free_tech_notification_return(*policy);policy->accept_actual_free_tech_notification_return(*this);
  reserved.require_operation(serial);require_free_tech_notification_return(*policy);
 }catch(...){policy->fail_free_tech_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_free_tech_notification_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->free_tech_notification_return,"Free tech actual native notification return absent/foreign");
 const auto& r=*p->free_tech_notification_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 need(actual&&actual.get()==&source&&world&&r.generation==p->generation&&r.serial!=0&&r.after>=r.before&&r.after-r.before<=1&&world->ui_source_revision==r.after&&
  world->setup.game_turn==r.turn&&r.actor==source.player()&&source.free_tech_count_stored(),"Free tech actual Add return issuer/world/revision differs");
 p->identity();need(p->world.lock().get()==world.get()&&engine_notification_same(engine_notification_ring(*world,r.actor),r.ring)&&
  engine_notification_flags_same(world->notification_load_transients,r.flags)&&p->notification_effects==r.effects,"Free tech actual Add ring/transients/effects changed");
}
}

namespace civ5::ui {
bool BnwEngineUiTurnOwner::add_reformation_native(runtime::LiveSessionState& state,std::string_view message_input,std::string_view summary_input,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&policy,"Reformation native Add SAME world/source absent");
 policy->require_reformation_notification_source(*host,state);
 Impl::FormattedDeficit formatted;formatted.actor=summary_i32(policy->player());formatted.turn=summary_i32(state.setup.game_turn);formatted.type=0x050b6287;
 formatted.generation=p->generation;formatted.revision=state.ui_source_revision;formatted.message=std::string(message_input);formatted.summary=std::string(summary_input);
 const auto revision=state.ui_source_revision,serial=reserved.operation_serial();
 need(serial==policy->private_reformation_operation_serial(),"Reformation reserved operation differs from genuine issued lease");
 const auto before_effects=p->notification_effects;
 const auto seal=[&](const saves::PlayerAdministrationRecord& ring,const runtime::BnwNotificationLoadTransients& flags,
  const std::vector<BnwEngineUiNotificationEffect>& effects,bool inserted){
  Impl::ReformationNotificationReturn r;auto& n=r.native;n.source=policy;n.world=world;n.actor=formatted.actor;
  n.generation=p->generation;n.before=revision;n.after=revision+(inserted?1u:0u);n.serial=serial;n.turn=formatted.turn;
  n.ring=ring;n.flags=flags;n.effects=effects;r.effect_policy=policy->effect_policy();r.inserted=inserted;return r;
 };
 const auto unchanged=[&]{
  // Allocate the complete historical seal before final callbackful bookends.
  auto staged=seal(engine_notification_ring(state,formatted.actor),state.notification_load_transients,before_effects,false);
  require_selected_localization_current();require_current();policy->require_reformation_notification_source(*host,state);reserved.require_operation(serial);p->identity();
  need(!p->golden_age_pump_busy&&state.ui_source_revision==revision&&p->notification_effects==before_effects&&
   engine_notification_same(engine_notification_ring(state,formatted.actor),staged.native.ring)&&
   engine_notification_flags_same(state.notification_load_transients,staged.native.flags),"Reformation unchanged native Add operands changed");
  static_assert(std::is_nothrow_move_constructible_v<Impl::ReformationNotificationReturn>);
  p->reformation_notification_return.emplace(std::move(staged));return false;
 };
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&formatted.type==0x050b6287,
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){return unchanged();}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){return unchanged();}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 std::optional<std::int64_t> auto_moves;if(turn_active)auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 // Native Reformation expiry predicate is false, so wait-extra-turn stays
 // false even for actual turn-active auto-moves. Preserve getter short circuit.
 // CvNotifications::IsNotificationRedundant REFORMATION scans ONLY the
 // actual active native ring, returning on undismissed same native type.
 for(auto index:actual_active)if(engine_notification_field(before_ring.children.at(index),"m_eNotificationType_bits")==0x050b6287){
  policy->require_reformation_notification_source(*host,state);p->identity();
  need(state.ui_source_revision==revision&&engine_notification_same(engine_notification_ring(state,actor),before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags),"Reformation redundant native branch changed");return unchanged();
 }

 // Capacity belongs only to the actual insertion branch. A genuine duplicate
 // has no increment even when the next lookup/world revision is exhausted.
 need(lookup<INT32_MAX&&revision<UINT64_MAX,"Reformation inserted native Add cursor/revision overflow");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 need(initialized==0||initialized==1,"Reformation actual finalInitialized outside bool");
 std::optional<std::int64_t> active;std::optional<std::int32_t> game_type;
 if(initialized){active=runtime::detail::bnw_game_active_player_read(state);if(actor==*active){
  game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
  need(*game_type==state.setup.game_type&&*game_type>=0&&*game_type<=3,"Reformation source game type differs");
 }}
 const bool broadcast=initialized&&active&&actor==*active&&game_type&&(*game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=before_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,false}; // REFORMATION never end-of-turn expires; wait-extra-turn is false
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 auto staged_return=seal(after_ring,after_flags,effects,true);
 require_selected_localization_current();require_current();
 policy->require_reformation_notification_source(*host,state);reserved.require_operation(serial);p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&!p->golden_age_pump_busy&&p->notification_effects==before_effects&&state.ui_source_revision==revision&&policy->player()==actor&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&(!auto_moves||summary_player_bool(state,actor,"m_bAutoMoves")==*auto_moves)&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&(!active||runtime::detail::bnw_game_active_player_read(state)==*active)&&
      (!game_type||(state.world.pregame->archive.integers.at("game_type")==*game_type&&state.setup.game_type==*game_type))&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;
 // No provider or allocation follows the native stores before the complete
 // earned historical seal. A later throw retains both physical ring and seal.
 static_assert(std::is_nothrow_move_constructible_v<Impl::ReformationNotificationReturn>);
 p->reformation_notification_return.emplace(std::move(staged_return));return true;
}
void BnwEngineUiTurnOwner::consume_reformation_notification(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::BnwSelectedStockLuaBootstrap& bootstrap,
 const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->reformation_notification_busy&&!p->golden_age_pump_busy,
  "Reformation notification foreign Engine/reentered producer");
 p->reformation_notification_busy=true;struct End{Impl& p;~End(){p.reformation_notification_busy=false;}}end{*p};
 try{const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"Reformation canonical Host/world absent");
  const auto serial=reserved.operation_serial();need(serial==policy->private_reformation_operation_serial(),"Reformation foreign reserved operation");
  require_current();policy->require_reformation_notification_source(*host,state);reserved.require_operation(serial);
  // The positive native GetNotifications was already captured. Two actual
  // selected locale literals are distinct; no base-language or dummy fallback.
  bootstrap.bind_engine_ui_localization_owned(*this,reserved);
  const auto message=selected_localization_text("TXT_KEY_NOTIFICATION_ADD_REFORMATION_BELIEF");
  const auto summary=selected_localization_text("TXT_KEY_NOTIFICATION_SUMMARY_ADD_REFORMATION_BELIEF");
  need(message.find('{')==message.npos&&message.find('}')==message.npos&&summary.find('{')==summary.npos&&summary.find('}')==summary.npos,
   "Reformation installed literal text requires unsupported formatter");
  (void)add_reformation_native(state,message,summary,policy,reserved);
  // PURE concrete return first. The receiver adopts the true native revision
  // before any old-revision VM/Host/source bookend can be invoked.
  require_reformation_notification_return(*policy);policy->accept_actual_reformation_notification_return(*this);
  reserved.require_operation(serial);require_reformation_notification_return(*policy);
 }catch(...){policy->fail_reformation_notification_dispatch();throw;}
}
void BnwEngineUiTurnOwner::require_reformation_notification_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->reformation_notification_return,"Reformation actual native return absent/foreign");
 const auto& r=*p->reformation_notification_return;const auto& n=r.native;const auto actual=n.source.lock();const auto world=n.world.lock();
 need(actual&&actual.get()==&source&&world&&p->world.lock()==world&&n.generation==p->generation&&
  n.serial==source.private_reformation_operation_serial()&&n.serial!=0&&n.actor==source.player()&&r.effect_policy==source.effect_policy()&&
  n.after>=n.before&&n.after-n.before==(r.inserted?1u:0u)&&world->ui_source_revision==n.after&&world->setup.game_turn==n.turn,
  "Reformation concrete native issuer/effect/world/revision differs");
 p->identity();need(engine_notification_same(engine_notification_ring(*world,n.actor),n.ring)&&
  engine_notification_flags_same(world->notification_load_transients,n.flags)&&p->notification_effects==n.effects,
  "Reformation concrete native ring/transients/FIFO differs");
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_resource_policy_layout(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Policy resource layout foreign Engine creator");
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);
 try{
  p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Policy resource layout canonical Engine Selection/world absent");
  selection->consume_resource_policy_layout(policy,state,reserved);
  // PRE-store require_source is intentionally never used after this actual
  // physical return; its exact flags/revision were consumed by native stores.
  selection->require_resource_policy_layout_return(*policy);
  p->identity();reserved.require_operation(reserved.operation_serial());
  selection->require_resource_policy_layout_return(*policy);
 }catch(...){policy->fail_resource_policy_layout_dispatch();throw;}
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_happiness_dirty(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Policy happiness GameData foreign Engine creator");
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);
 try{
  p->identity();const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Policy happiness canonical Engine Selection/world absent");
  selection->consume_happiness_dirty(policy,state,reserved);
  selection->require_happiness_dirty_return(*policy);
  p->identity();reserved.require_operation(reserved.operation_serial());
  selection->require_happiness_dirty_return(*policy);
 }catch(...){policy->fail_happiness_dirty_dispatch();throw;}
}
}

namespace civ5::ui {
bool BnwEngineUiTurnOwner::add_outer_branch_native(runtime::LiveSessionState& state,std::string_view message_input,std::string_view summary_input,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& policy,const runtime::BnwStockLuaAttemptLease& reserved){
 const auto p=impl_;require_current();const auto world=p->world.lock();const auto host=p->host.lock();
 need(world&&host&&world.get()==&state&&policy,"Outer branch native Add SAME world/source absent");
 policy->require_outer_branch_notification_source(*host,state);
 const auto operands=policy->outer_branch_notification_operands_without_callbacks();need(!operands.skip&&operands.present,"Outer branch Add not actual non-null call");
 Impl::FormattedDeficit formatted;formatted.actor=operands.recipient;formatted.turn=summary_i32(state.setup.game_turn);formatted.type=operands.free_policy?0x1e4417c5U:0x9074bdeeU;
 formatted.generation=p->generation;formatted.revision=state.ui_source_revision;formatted.message=std::string(message_input);formatted.summary=std::string(summary_input);
 const auto revision=state.ui_source_revision,serial=reserved.operation_serial();
 need(serial==policy->private_outer_branch_notification_serial(),"Outer branch reserved operation differs from genuine issued lease");
 const auto before_effects=p->notification_effects;
 const auto seal=[&](const saves::PlayerAdministrationRecord& ring,const runtime::BnwNotificationLoadTransients& flags,
  const std::vector<BnwEngineUiNotificationEffect>& effects,bool inserted){
  Impl::OuterBranchNotificationReturn r;auto& n=r.native;n.source=policy;n.world=world;n.actor=formatted.actor;
  n.generation=p->generation;n.before=revision;n.after=revision+(inserted?1u:0u);n.serial=serial;n.turn=formatted.turn;
  n.ring=ring;n.flags=flags;n.effects=effects;r.operands=operands;r.inserted=inserted;r.effects_emitted=static_cast<std::uint64_t>(effects.size()-before_effects.size());return r;
 };
 const auto unchanged=[&]{
  // Allocate the complete historical seal before final callbackful bookends.
  auto staged=seal(engine_notification_ring(state,formatted.actor),state.notification_load_transients,before_effects,false);
  require_selected_localization_current();require_current();policy->require_outer_branch_notification_source(*host,state);reserved.require_operation(serial);p->identity();
  need(!p->golden_age_pump_busy&&state.ui_source_revision==revision&&policy->outer_branch_notification_operands_without_callbacks()==operands&&p->notification_effects==before_effects&&
   engine_notification_same(engine_notification_ring(state,formatted.actor),staged.native.ring)&&
   engine_notification_flags_same(state.notification_load_transients,staged.native.flags),"Outer branch unchanged native Add operands changed");
  static_assert(std::is_nothrow_move_constructible_v<Impl::OuterBranchNotificationReturn>);
  p->outer_branch_notification_return.emplace(std::move(staged));return false;
 };
 need(formatted.actor>=0&&formatted.actor<22&&formatted.generation==p->generation&&formatted.revision==revision&&formatted.turn==state.setup.game_turn&&formatted.type==(operands.free_policy?0x1e4417c5U:0x9074bdeeU),
      "Engine Add genuine formatter source/turn/revision differs");
 need(!formatted.message.empty()&&!formatted.summary.empty()&&formatted.message.find('\0')==std::string::npos&&formatted.summary.find('\0')==std::string::npos&&formatted.message.size()<=UINT32_MAX&&formatted.summary.size()<=UINT32_MAX,
      "Engine Add actual localized CvString unsupported");
 const auto actor=formatted.actor;
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add genuine Pregame/player operands absent");
 const auto before_status=state.world.pregame->archive.integer_arrays.at("slot_status");
 const auto before_claims=state.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto before_options=state.world.pregame->game_options;
 const auto owned_status=state.setup.players.at(actor).slot_status;
 const auto owned_claim=state.setup.players.at(actor).slot_claim;
 // Formatting in the retail UI caller precedes these CvNotifications::Add
 // early returns. Never use them to skip the missing formatter.
 if(!runtime::detail::bnw_player_human_read(state,actor,host->database())){return unchanged();}
 // Read the SAME LiveSession's genuine current reset/Read debug owner.
 // Neither saved flags nor a local initialized-false substitute authorizes it.
 if(host->session().bnw_game_debug_mode()){return unchanged();}
 auto& live_ring=engine_notification_ring(state,actor);
 need(engine_notification_field(live_ring,"has_notifications")==1&&engine_notification_field(live_ring,"version")==2&&engine_notification_field(live_ring,"m_ePlayer")==actor&&live_ring.fields.size()==6&&live_ring.children.size()==100,
      "Engine Add actual native owner version/100-slot shape differs");
 need(state.notification_load_transients.current&&state.notification_load_transients.players.contains(actor),"Engine Add genuine Read/transient lifecycle missing");
 const auto before_ring=live_ring;auto after_ring=before_ring;
 const auto before_flags=state.notification_load_transients;auto after_flags=before_flags;
 auto& owner_flags=after_flags.players.at(actor);
 const auto begin=summary_i32(engine_notification_field(before_ring,"m_iNotificationsBeginIndex"));
 const auto end=summary_i32(engine_notification_field(before_ring,"m_iNotificationsEndIndex"));
 const auto lookup=summary_i32(engine_notification_field(before_ring,"m_iCurrentLookupIndex"));
 need(begin>=0&&begin<100&&end>=0&&end<100&&lookup>=0,
      "Engine Add cursor/lookup/UI revision overflow");
 for(std::size_t n=0;n<100;++n){
  const auto& row=before_ring.children[n];need(engine_notification_name(row.name)=="notification"&&row.index==n&&row.fields.size()==11&&row.blobs.size()==2&&row.arrays.empty()&&row.children.empty(),"Engine Add native slot schema differs");
  const auto dismissed=engine_notification_field(row,"m_bDismissed");need(dismissed==0||dismissed==1,"Engine Add actual dismissed boolean invalid");
  const auto type=engine_notification_field(row,"m_eNotificationType_bits");need(type>=0&&type<=UINT32_MAX,"Engine Add notification hash outside native uint32");
  for(auto name:{"m_strMessage","m_strSummary"}){
   const auto& bytes=engine_notification_blob(row,name);
   need(bytes.size()==static_cast<std::uint64_t>(engine_notification_field(row,name==std::string_view("m_strMessage")?"m_strMessage_wire_bytes":"m_strSummary_wire_bytes")),"Engine Add string wire count differs");
  }
 }
 std::vector<std::size_t> actual_active;
 std::set<std::int32_t> actual_lookups;
 for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100){
  const auto index=static_cast<std::size_t>(cursor);const auto& row=before_ring.children.at(index);
  const auto old_lookup=summary_i32(engine_notification_field(row,"m_iLookupIndex"));
  need(old_lookup>=0&&old_lookup<lookup&&actual_lookups.insert(old_lookup).second&&engine_notification_field(row,"m_ePlayerID")==actor,"Engine Add active lookup/player mismatch");
  if(!engine_notification_field(row,"m_bDismissed"))actual_active.push_back(index);
 }
 need(actual_active==owner_flags.active_undismissed,"Engine Add actual raw ring/owned active flags differ");
 const auto turn_active=summary_player_bool(state,actor,"m_bTurnActive");
 std::optional<std::int64_t> auto_moves;if(turn_active)auto_moves=summary_player_bool(state,actor,"m_bAutoMoves");
 // FREE_POLICY does not expire and suppresses an undismissed same type.
 // IDEOLOGY_CHOSEN uses default expiry=true and default redundancy=false.
 // Preserve the native turnActive/autoMoves getter short circuit.
 for(auto index:actual_active)if(operands.free_policy&&engine_notification_field(before_ring.children.at(index),"m_eNotificationType_bits")==0x1e4417c5){
  policy->require_outer_branch_notification_source(*host,state);p->identity();
  need(state.ui_source_revision==revision&&engine_notification_same(engine_notification_ring(state,actor),before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags),"Outer branch redundant native branch changed");return unchanged();
 }

 // Capacity belongs only to the actual insertion branch. A genuine duplicate
 // has no increment even when the next lookup/world revision is exhausted.
 need(lookup<INT32_MAX&&revision<UINT64_MAX,"Outer branch inserted native Add cursor/revision overflow");
 const auto initialized=summary_named(state.world.game_state.flags,"final_initialized").value;
 need(initialized==0||initialized==1,"Outer branch actual finalInitialized outside bool");
 std::optional<std::int64_t> active;std::optional<std::int32_t> game_type;
 if(initialized){active=runtime::detail::bnw_game_active_player_read(state);if(actor==*active){
  game_type=summary_i32(state.world.pregame->archive.integers.at("game_type"));
  need(*game_type==state.setup.game_type&&*game_type>=0&&*game_type<=3,"Outer branch source game type differs");
 }}
 const bool broadcast=initialized&&active&&actor==*active&&game_type&&(*game_type!=2||turn_active!=0);
 // CvMap.h221-229: -1 is NOT INVALID_PLOT_COORD (-(INT_MAX)). Both
 // axes are passed through coordRange before isPlot. Authenticate current
 // raw/projected native geometry rather than guessing FX(NULL).
 std::uint64_t map_width=0,map_height=0;bool map_wrap_x=false,map_wrap_y=false;
 if(broadcast){
  const auto& header=state.world.map_header;const auto& projection=state.world.map_projection;
  need(header.layout==saves::GameStateLayout::expansion2_version1&&projection&&
       header.grid.width>0&&header.grid.width<=INT32_MAX&&header.grid.height>0&&header.grid.height<=INT32_MAX&&
       projection->grid.width==header.grid.width&&projection->grid.height==header.grid.height&&
       projection->wrap_x==header.wrap_x&&projection->wrap_y==header.wrap_y,
       "Engine deficit FX actual native map geometry raw/owned differs");
  map_width=header.grid.width;map_height=header.grid.height;map_wrap_x=header.wrap_x;map_wrap_y=header.wrap_y;
  // At least one unwrapped -1 axis is outside isPlot. A reached doubly
  // wrapped query is left strict until a genuine non-null plot FX owner.
  need(!map_wrap_x||!map_wrap_y,"Engine deficit FX reached both-wrap native plot receiver unavailable");
 }

 auto effects=before_effects;
 need(effects.size()<=UINT64_MAX-4,"Engine Add client effect occurrence overflow");effects.reserve(effects.size()+4);
 const auto append=[&](BnwEngineUiNotificationEffectKind kind,std::int32_t at_lookup,const saves::PlayerAdministrationRecord* row){
  BnwEngineUiNotificationEffect e;e.kind=kind;e.actor=actor;e.lookup=at_lookup;e.turn=formatted.turn;e.generation=p->generation;e.world_revision=revision+1;e.occurrence=static_cast<std::uint64_t>(effects.size())+1;
  if(row){e.type=static_cast<std::uint32_t>(engine_notification_field(*row,"m_eNotificationType_bits"));const auto& m=engine_notification_blob(*row,"m_strMessage");const auto& s=engine_notification_blob(*row,"m_strSummary");e.message.assign(m.begin(),m.end());e.summary.assign(s.begin(),s.end());e.x=summary_i32(engine_notification_field(*row,"m_iX"));e.y=summary_i32(engine_notification_field(*row,"m_iY"));e.game_data_index=summary_i32(engine_notification_field(*row,"m_iGameDataIndex"));e.extra_game_data=summary_i32(engine_notification_field(*row,"m_iExtraGameData"));}
  effects.push_back(std::move(e));
 };
 const auto next_end=(end+1)%100;
 std::optional<std::size_t> cleared;
 if(next_end==begin){
  const auto first=static_cast<std::size_t>(begin);auto& oldest=after_ring.children.at(first);
  if(!engine_notification_field(oldest,"m_bDismissed")){
   // RemoveOldest uses Dismiss(...,false): policy-seen branch is unreachable.
   engine_notification_field(oldest,"m_bDismissed")=1;
   append(BnwEngineUiNotificationEffectKind::remove_notification,summary_i32(engine_notification_field(oldest,"m_iLookupIndex")),&oldest);
  }
  engine_notification_clear(oldest,owner_flags.slots.at(first));
  engine_notification_field(after_ring,"m_iNotificationsBeginIndex")=(begin+1)%100;cleared=first;
 }
 const auto index=static_cast<std::size_t>(end);auto& inserted=after_ring.children.at(index);
 engine_notification_clear(inserted,owner_flags.slots.at(index));
 engine_notification_field(inserted,"m_ePlayerID")=actor;
 engine_notification_field(inserted,"m_eNotificationType_bits")=formatted.type;
 auto& message=engine_notification_blob(inserted,"m_strMessage");message.assign(formatted.message.begin(),formatted.message.end());
 auto& summary=engine_notification_blob(inserted,"m_strSummary");summary.assign(formatted.summary.begin(),formatted.summary.end());
 engine_notification_field(inserted,"m_strMessage_wire_bytes")=static_cast<std::int64_t>(message.size());
 engine_notification_field(inserted,"m_strSummary_wire_bytes")=static_cast<std::int64_t>(summary.size());
 for(auto name:{"m_iX","m_iY","m_iGameDataIndex","m_iExtraGameData"})engine_notification_field(inserted,name)=-1;
 if(!operands.free_policy)engine_notification_field(inserted,"m_iGameDataIndex")=operands.actor;
 engine_notification_field(inserted,"m_iTurn")=formatted.turn;
 engine_notification_field(inserted,"m_iLookupIndex")=lookup;
 owner_flags.slots.at(index)={true,!operands.free_policy&&turn_active&&auto_moves.value_or(0)!=0}; // IDEOLOGY_CHOSEN default expires; FREE_POLICY does not
 if(broadcast){
  append(BnwEngineUiNotificationEffectKind::add_notification,lookup,&inserted);
  append(BnwEngineUiNotificationEffectKind::gameplay_fx_null,lookup,nullptr);
  owner_flags.slots.at(index).needs_broadcast=false;
 }
 if(initialized)append(BnwEngineUiNotificationEffectKind::gameplay_minimap_notification,lookup+1,nullptr);
 engine_notification_field(after_ring,"m_iNotificationsEndIndex")=next_end;
 engine_notification_field(after_ring,"m_iCurrentLookupIndex")=lookup+1;
 owner_flags.active_undismissed.clear();
 const auto after_begin=summary_i32(engine_notification_field(after_ring,"m_iNotificationsBeginIndex"));
 for(auto cursor=after_begin;cursor!=next_end;cursor=(cursor+1)%100)if(!engine_notification_field(after_ring.children.at(static_cast<std::size_t>(cursor)),"m_bDismissed"))owner_flags.active_undismissed.push_back(static_cast<std::size_t>(cursor));
 after_flags.external_update_pending=false;
 for(const auto& [player,flags]:after_flags.players){(void)player;for(auto slot:flags.active_undismissed)after_flags.external_update_pending|=flags.slots.at(slot).needs_broadcast;}
 // All strings, row/transient/event copies and currentness callbacks finish
 // before the final nonallocating semantic match. No cloned world is swapped.
 auto staged_return=seal(after_ring,after_flags,effects,true);
 require_selected_localization_current();require_current();
 policy->require_outer_branch_notification_source(*host,state);reserved.require_operation(serial);p->identity();
 need(state.world.pregame.has_value()&&state.setup.players.contains(actor),"Engine Add source Pregame/player retired before stores");
 // Re-resolve after callbackful currentness: no earlier raw row reference
 // is dereferenced after a source callback could have replaced its storage.
 auto& final_ring=engine_notification_ring(state,actor);
 need(world.get()==&state&&!p->golden_age_pump_busy&&p->notification_effects==before_effects&&state.ui_source_revision==revision&&policy->outer_branch_notification_operands_without_callbacks()==operands&&
      engine_notification_same(final_ring,before_ring)&&engine_notification_flags_same(state.notification_load_transients,before_flags)&&
      (!broadcast||(state.world.map_header.layout==saves::GameStateLayout::expansion2_version1&&
       state.world.map_header.grid.width==map_width&&state.world.map_header.grid.height==map_height&&
       state.world.map_header.wrap_x==map_wrap_x&&state.world.map_header.wrap_y==map_wrap_y&&state.world.map_projection&&
       state.world.map_projection->grid.width==map_width&&state.world.map_projection->grid.height==map_height&&
       state.world.map_projection->wrap_x==map_wrap_x&&state.world.map_projection->wrap_y==map_wrap_y))&&
      summary_player_bool(state,actor,"m_bTurnActive")==turn_active&&(!auto_moves||summary_player_bool(state,actor,"m_bAutoMoves")==*auto_moves)&&
      summary_named(state.world.game_state.flags,"final_initialized").value==initialized&&(!active||runtime::detail::bnw_game_active_player_read(state)==*active)&&
      (!game_type||(state.world.pregame->archive.integers.at("game_type")==*game_type&&state.setup.game_type==*game_type))&&state.setup.game_turn==formatted.turn&&
      state.world.pregame->archive.integer_arrays.at("slot_status")==before_status&&state.world.pregame->archive.integer_arrays.at("slot_claims")==before_claims&&
      engine_notification_options_same(state.world.pregame->game_options,before_options)&&state.setup.players.at(actor).slot_status==owned_status&&state.setup.players.at(actor).slot_claim==owned_claim&&!host->session().bnw_game_debug_mode(),
      "Engine Add operands changed before native stores");
 auto& final_begin=engine_notification_field(final_ring,"m_iNotificationsBeginIndex");
 auto& final_end=engine_notification_field(final_ring,"m_iNotificationsEndIndex");
 auto& final_lookup=engine_notification_field(final_ring,"m_iCurrentLookupIndex");
 static_assert(std::is_nothrow_swappable_v<saves::PlayerAdministrationRecord>);
 // Native removal/clear, slot, effects, end, lookup source order. Typed effect
 // queue submission is genuine Civis storage; physical FX/present is separate.
 if(cleared)std::swap(final_ring.children[*cleared],after_ring.children[*cleared]);
 std::swap(final_ring.children[index],after_ring.children[index]);
 final_begin=after_begin;
 state.notification_load_transients.players.swap(after_flags.players);
 state.notification_load_transients.external_update_pending=after_flags.external_update_pending;
 p->notification_effects.swap(effects);
 final_end=next_end;
 final_lookup=lookup+1;
 ++state.ui_source_revision;
 // No provider or allocation follows the native stores before the complete
 // earned historical seal. A later throw retains both physical ring and seal.
 static_assert(std::is_nothrow_move_constructible_v<Impl::OuterBranchNotificationReturn>);
 p->outer_branch_notification_return.emplace(std::move(staged_return));return true;
}

void BnwEngineUiTurnOwner::consume_outer_branch_notifications(runtime::LiveSessionState& state,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::BnwSelectedStockLuaBootstrap& bootstrap,
 const runtime::BnwStockLuaAttemptLease& reserved){
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&!p->outer_branch_notification_busy&&!p->golden_age_pump_busy,"Outer branch notification foreign Engine/reentry");
 p->outer_branch_notification_busy=true;struct End{Impl& p;~End(){p.outer_branch_notification_busy=false;}}end{*p};
 try{const auto host=p->host.lock();const auto world=p->world.lock();need(host&&world&&world.get()==&state,"Outer branch notification SAME Host/world absent");
  const auto serial=reserved.operation_serial();need(serial==policy->private_outer_branch_notification_serial(),"Outer branch foreign lease");
  while(policy->phase()==runtime::BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing){
   require_current();policy->require_outer_branch_notification_source(*host,state);reserved.require_operation(serial);
   const auto q=policy->outer_branch_notification_operands_without_callbacks();
   if(q.skip||!q.present){
    // Genuine target/no-credit/NULL short circuits do not localize or Add.
    Impl::OuterBranchNotificationReturn r;auto& n=r.native;n.source=policy;n.world=world;n.actor=q.recipient;n.generation=p->generation;
    n.before=q.revision;n.after=q.revision;n.serial=serial;n.turn=state.setup.game_turn;r.operands=q;n.flags=state.notification_load_transients;n.effects=p->notification_effects;
    if(!q.skip)n.ring=engine_notification_ring(state,q.recipient);
    require_current();policy->require_outer_branch_notification_source(*host,state);reserved.require_operation(serial);p->identity();
    need(!p->golden_age_pump_busy&&policy->outer_branch_notification_operands_without_callbacks()==q&&p->notification_effects==n.effects&&
     engine_notification_flags_same(state.notification_load_transients,n.flags)&&(!q.skip||!q.present)&&
     (q.skip||engine_notification_same(engine_notification_ring(state,q.recipient),n.ring)),"Outer branch nullable short circuit changed");
    p->outer_branch_notification_return.emplace(std::move(r));
   }else{
    bootstrap.bind_engine_ui_localization_owned(*this,reserved);
    std::string message,summary;
    if(q.free_policy){message=selected_localization_text("TXT_KEY_NOTIFICATION_FREE_POLICY");summary=selected_localization_text("TXT_KEY_NOTIFICATION_FREE_POLICY_SUMMARY");}
    else{
     std::string branch_key;std::size_t found=0;
     const auto report=host->database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
      if(slot==static_cast<std::uint64_t>(q.branch)){branch_key=row.text("Description").value_or("");++found;}return true;});
     need(report.all_succeeded()&&found==1&&!branch_key.empty(),"Ideology notification actual branch Description unavailable");
     const auto branch_text=selected_localization_text(branch_key);std::string civilization;
     if(q.has_met){
      need(!summary_minor(state,q.actor),"Ideology minor shortDescription requires genuine MinorCivAI owner");
      const auto& custom=state.world.pregame->archive.string_arrays.at("civ_short_descriptions");need(custom.size()==64,"Ideology actual custom civilization slots absent");
      auto key=custom.at(static_cast<std::size_t>(q.actor));
      if(key.empty()){
       const auto& civs=state.world.pregame->slots.integer_arrays.at("civilizations");need(civs.size()==64,"Ideology actual civilization ID slots absent");
       const auto id=civs.at(static_cast<std::size_t>(q.actor));need(id==state.cities.players.at(q.actor).civilization,"Ideology civilization raw/owned differs");
       std::size_t rows=0;const auto count=host->database().query({"SELECT ShortDescription FROM Civilizations WHERE ID=?",{id}},[&](const auto values){need(values.size()==1&&std::get_if<std::string>(&values[0]),"Ideology native ShortDescription malformed");key=std::get<std::string>(values[0]);++rows;});need(count==rows&&rows==1&&!key.empty(),"Ideology CivilizationInfo unavailable");
      }
      civilization=key.starts_with("TXT_KEY_")?selected_localization_text(key):key;
     }
     message=detail::format_actual_branch_ideology_text(selected_localization_text(q.has_met?"TXT_KEY_NOTIFICATION_IDEOLOGY_CHOSEN":"TXT_KEY_NOTIFICATION_IDEOLOGY_CHOSEN_UNMET"),p->language,q.has_met,civilization,branch_text);
     summary=selected_localization_text("TXT_KEY_NOTIFICATION_SUMMARY_IDEOLOGY_CHOSEN");
    }
    need(message.find('{')==message.npos&&message.find('}')==message.npos&&summary.find('{')==summary.npos&&summary.find('}')==summary.npos,"Branch notification unresolved locale grammar");
    (void)add_outer_branch_native(state,message,summary,policy,reserved);
   }
   (void)require_outer_branch_notification_return(*policy);policy->accept_actual_outer_branch_notification_return(*this);
   reserved.require_operation(serial);(void)require_outer_branch_notification_return(*policy);
  }
 }catch(...){policy->fail_outer_branch_notification_dispatch();throw;}
}
runtime::BnwPolicyBranchNotificationHistory BnwEngineUiTurnOwner::require_outer_branch_notification_return(const runtime::BnwPolicyAdoptionPending& source)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->outer_branch_notification_return,"Outer branch actual native return absent/foreign");
 const auto& r=*p->outer_branch_notification_return;const auto& n=r.native;const auto actual=n.source.lock();const auto world=n.world.lock();
 need(actual&&actual.get()==&source&&world&&p->world.lock()==world&&n.generation==p->generation&&n.serial!=0&&
  n.serial==source.private_outer_branch_notification_serial()&&r.operands.actor==source.player()&&n.actor==r.operands.recipient&&
  n.before==r.operands.revision&&n.after>=n.before&&n.after-n.before==(r.inserted?1u:0u)&&world->ui_source_revision==n.after&&world->setup.game_turn==n.turn,
  "Outer branch physical return identity/revision differs");p->identity();
 need((r.operands.skip||engine_notification_same(engine_notification_ring(*world,n.actor),n.ring))&&
  engine_notification_flags_same(world->notification_load_transients,n.flags)&&p->notification_effects==n.effects,"Outer branch concrete ring/transient/FIFO changed");
 return {r.operands,r.inserted,n.after,r.effects_emitted};
}

}

namespace civ5::ui {
void BnwEngineUiTurnOwner::drain_notification_publication_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& lease){
 EndTurnPin pin(weak);pin.engine->drain_notification_publication_owned(pump,lease);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"Notification counted pump retired");
}
void BnwEngineUiTurnOwner::drain_notification_publication_owned(
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& lease){
 const auto p=impl_;
 need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,
  "Notification receiver foreign creator/retired");
 // Admission is callback-free and proves the supplied lease, not merely the
 // possession of a genuine pump with a different operation descriptor.
 // Foreign attempts cannot poison a legitimate already-entered receiver.
 {
  auto identity=lease.pin_active_scope_without_callbacks();pump.pure();
  const auto canonical_ui=p->ui.lock();const auto canonical_host=p->host.lock();const auto canonical_world=p->world.lock();
  const auto loan_ui=pump.ui_.lock();const auto loan_host=pump.lineage_.host.lock();const auto loan_world=pump.lineage_.world.lock();
  const auto loan_control=pump.lineage_.control.lock();const auto loan_source=pump.lineage_.canonical_source.lock();
  const auto registered=p->end_turn_self.lock();
  need(registered&&registered.get()==this&&canonical_ui&&canonical_ui->engine_ui_turn_owner_.get()==this&&
   !registered.owner_before(canonical_ui->engine_ui_turn_owner_)&&!canonical_ui->engine_ui_turn_owner_.owner_before(registered)&&
   identity.creator==p->creator&&identity.serial&&identity.vm==pump.lineage_.vm&&identity.control&&loan_control&&
   identity.control.get()==loan_control.get()&&!identity.control.owner_before(loan_control)&&!loan_control.owner_before(identity.control)&&
   identity.source_owner&&loan_source&&identity.source_owner.get()==loan_source.get()&&
   !identity.source_owner.owner_before(loan_source)&&!loan_source.owner_before(identity.source_owner)&&
   canonical_ui&&canonical_host&&canonical_world&&loan_ui&&loan_host&&loan_world&&
   canonical_ui.get()==loan_ui.get()&&!canonical_ui.owner_before(loan_ui)&&!loan_ui.owner_before(canonical_ui)&&
   canonical_host.get()==loan_host.get()&&!canonical_host.owner_before(loan_host)&&!loan_host.owner_before(canonical_host)&&
   canonical_world.get()==loan_world.get()&&!canonical_world.owner_before(loan_world)&&!loan_world.owner_before(canonical_world)&&
   identity.world==canonical_world.get()&&pump.lineage_.generation==p->generation,
   "Notification foreign supplied lease/pump/Engine rejected before busy receiver poisoning");
  const auto stored=pump.source_control_.lock();
  if(stored)need(stored.get()==identity.control.get()&&!stored.owner_before(identity.control)&&!identity.control.owner_before(stored)&&
   pump.vm_==identity.vm&&pump.operation_==identity.serial,"Notification pump has different already-issued operation");
  lease.require_scope_identity_without_callbacks(identity);pump.pure();
 } // All pure temporary strong source pins drop before callbackful providers.
 if(const auto ui=p->ui.lock();ui&&ui->notification_publication_owner_&&ui->notification_publication_owner_->busy_){
  ui->notification_publication_owner_->progress_.failed=true;throw std::runtime_error("Notification nested pump quarantined actual receiver");
 }
 need(!p->golden_age_pump_busy&&!p->outer_branch_notification_busy&&!p->reformation_notification_busy&&
  !p->golden_age_notification_busy&&!p->free_tech_notification_busy,"Notification receiver creator/reentry failed");
 // Reserve before any source callbacks or mutable canonical borrows.
 p->golden_age_pump_busy=true;struct End{Impl& p;~End(){p.golden_age_pump_busy=false;}}end{*p};
 const auto actual_ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();
 need(actual_ui&&host&&world,"Notification canonical Ui/Host/world retired");
 auto owner=actual_ui->notification_publication_owner_;
 if(!owner){
  owner=std::shared_ptr<BnwOwnedNotificationPublicationOwner>(new BnwOwnedNotificationPublicationOwner(p->ui,p->host,p->world,p->end_turn_self.lock(),p->generation));
  owner->self_=owner;actual_ui->notification_publication_owner_=owner;
 }
 if(owner->creator_==p->creator&&owner->busy_){owner->progress_.failed=true;throw std::runtime_error("Notification receiver reentered");}
 need(owner->creator_==p->creator&&!owner->progress_.failed,"Notification receiver failed/foreign");
 owner->busy_=true;struct ReceiverEnd{BnwOwnedNotificationPublicationOwner& o;~ReceiverEnd(){o.busy_=false;}}receiver_end{*owner};
 try{
  const auto serial=lease.operation_serial();
  auto current=[&]{
   need(!owner->progress_.failed,"Notification sticky nested failure before source callbacks/store");
   pump.require_for_source(lease);lease.require_operation(serial);require_current();
   const auto loan_ui=pump.ui_.lock();const auto loan_host=pump.lineage_.host.lock();const auto loan_world=pump.lineage_.world.lock();
   need(loan_ui&&loan_host&&loan_world&&actual_ui.get()==loan_ui.get()&&!actual_ui.owner_before(loan_ui)&&!loan_ui.owner_before(actual_ui)&&
    host.get()==loan_host.get()&&!host.owner_before(loan_host)&&!loan_host.owner_before(host)&&
    world.get()==loan_world.get()&&!world.owner_before(loan_world)&&!loan_world.owner_before(world)&&pump.lineage_.generation==p->generation,
    "Notification pump is not SAME actual source/UI/world");
   const auto self=owner->self_.lock();const auto engine=owner->engine_.lock();
   need(self&&self.get()==owner.get()&&engine&&engine.get()==this&&actual_ui->engine_ui_turn_owner_.get()==this&&
    !engine.owner_before(actual_ui->engine_ui_turn_owner_)&&!actual_ui->engine_ui_turn_owner_.owner_before(engine)&&owner->ui_.lock().get()==actual_ui.get()&&
    owner->host_.lock().get()==host.get()&&owner->world_.lock().get()==world.get()&&owner->generation_==p->generation&&
    actual_ui->notification_publication_owner_.get()==owner.get()&&!owner->progress_.failed&&p->end_turn_alive&&!p->end_turn_failed,
    "Notification owned receiver canonical lineage changed");
  };
  current();const auto staged=p->notification_effects;
  need(owner->progress_.received_prefix<=staged.size(),"Notification actual prefix regressed");
  while(owner->progress_.received_prefix<staged.size()){
   const auto index=static_cast<std::size_t>(owner->progress_.received_prefix);const auto& e=staged[index];
   need(e.occurrence==owner->progress_.received_prefix+1&&e.generation==p->generation&&e.world_revision<=world->ui_source_revision&&e.actor>=0&&e.actor<63&&e.lookup>=0,
    "Notification authoritative FIFO occurrence/lineage invalid");
   if(e.kind==BnwEngineUiNotificationEffectKind::gameplay_fx_null||e.kind==BnwEngineUiNotificationEffectKind::gameplay_minimap_notification){
    // No physical receiver exists for these operations in this staging owner.
    // Retain the exact blocked head; never advance a policy/presentation ACK.
    current();need(p->notification_effects==staged,"Notification unsupported head changed");
    owner->progress_.unsupported=e.kind;owner->progress_.unsupported_occurrence=e.occurrence;break;
   }
   need(e.kind==BnwEngineUiNotificationEffectKind::add_notification||e.kind==BnwEngineUiNotificationEffectKind::remove_notification,
    "Notification unknown physical operation");
   const auto before=owner->progress_;auto entries=owner->entries_;auto removed=owner->removed_;
   const BnwOwnedNotificationPublicationOwner::Key key{e.actor,e.lookup};
   if(e.kind==BnwEngineUiNotificationEffectKind::add_notification){
    need(e.actor==host->active_player_id()&&!e.message.empty()&&e.message.find('\0')==std::string::npos&&e.message.find("TXT_KEY_")==std::string::npos,
     "Notification actual Add target/text differs");
    const auto existing=entries.find(key);
    need(existing==entries.end(),"Notification same lookup was added twice without actual Remove");
    entries.emplace(key,e);removed.erase(key);
    need(std::count_if(entries.begin(),entries.end(),[&](const auto& v){return v.first.first==e.actor;})<=100,
     "Notification UI model exceeds actual native100 ring");
   }else{entries.erase(key);removed.insert(key);}
   need(before.model_revision<UINT64_MAX&&before.received_prefix<UINT64_MAX&&
    (e.kind==BnwEngineUiNotificationEffectKind::add_notification?before.add_calls:before.remove_calls)<UINT64_MAX,
    "Notification receive counters exhausted");
   current();need(p->notification_effects==staged&&owner->progress_.received_prefix==before.received_prefix&&owner->progress_.model_revision==before.model_revision,
    "Notification actual receiver changed before UI store");
   owner->entries_.swap(entries);owner->removed_.swap(removed);
   ++owner->progress_.received_prefix;++owner->progress_.model_revision;
   if(e.kind==BnwEngineUiNotificationEffectKind::add_notification)++owner->progress_.add_calls;else ++owner->progress_.remove_calls;
   owner->progress_.unsupported.reset();owner->progress_.unsupported_occurrence=0;
   // Earned physical UI model receive/history is stored BEFORE callbacks.
   current();need(p->notification_effects==staged,"Notification FIFO changed after earned receive");
  }
  current();need(p->notification_effects==staged,"Notification FIFO changed at receiver return");
 }catch(...){owner->progress_.failed=true;throw;}
}
void BnwEngineUiTurnOwner::merge_notification_frame_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const LiveUiHost& ui,LiveUiFrame& frame){
 EndTurnPin pin(weak);pin.engine->merge_notification_frame_owned(ui,frame);
 need(pin.storage->end_turn_alive&&!pin.storage->end_turn_failed,"Notification frame issuer retired");
}
void BnwEngineUiTurnOwner::merge_notification_frame_owned(const LiveUiHost& ui,LiveUiFrame& frame){
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Notification frame merge foreign creator");
 const auto actual_ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();
 {
  const auto registered=p->end_turn_self.lock();
  need(registered&&registered.get()==this&&actual_ui&&host&&world&&actual_ui.get()==&ui&&ui.engine_ui_turn_owner_.get()==this&&
   !registered.owner_before(ui.engine_ui_turn_owner_)&&!ui.engine_ui_turn_owner_.owner_before(registered)&&
   frame.owner==&ui&&frame.sequence==ui.sequence_&&frame.snapshot.active_player_id()==host->active_player_id()&&
   frame.snapshot.turn()==world->setup.game_turn,"Notification frame has different actual owner/actor/turn");
 } // Pure temporary registration pin retires BEFORE callbackful providers.
 const auto owner=ui.notification_publication_owner_;
 if(owner&&owner->creator_==p->creator&&owner->busy_){owner->progress_.failed=true;throw std::runtime_error("Notification frame receiver reentered");}
 need(!p->golden_age_pump_busy&&owner&&owner->creator_==p->creator&&!owner->progress_.failed&&owner->engine_.lock().get()==this&&owner->generation_==p->generation&&
  owner->ui_.lock().get()==&ui&&owner->host_.lock().get()==host.get()&&owner->world_.lock().get()==world.get()&&owner->self_.lock().get()==owner.get(),
  "Notification frame receiving model absent/failed/reentered/foreign");
 const auto world_revision=world->ui_source_revision,model_revision=owner->progress_.model_revision,received=owner->progress_.received_prefix;
 owner->busy_=true;struct End{BnwOwnedNotificationPublicationOwner& o;~End(){o.busy_=false;}}end{*owner};
 try{
  require_current();need(ui.engine_ui_turn_owner_.get()==this&&owner->engine_.lock().get()==this&&!owner->progress_.failed&&world->ui_source_revision==world_revision&&owner->progress_.model_revision==model_revision&&owner->progress_.received_prefix==received&&
   owner->progress_.received_prefix<=p->notification_effects.size(),"Notification frame received lineage changed/failed");
  owner->merge_frame(frame,ui);require_current();
  need(ui.engine_ui_turn_owner_.get()==this&&owner->engine_.lock().get()==this&&frame.owner==&ui&&frame.sequence==ui.sequence_&&ui.notification_publication_owner_.get()==owner.get()&&p->end_turn_alive&&!p->end_turn_failed,
   "Notification captured frame/model retired before return");
  need(!owner->progress_.failed&&world->ui_source_revision==world_revision&&owner->progress_.model_revision==model_revision&&owner->progress_.received_prefix==received,
   "Notification captured frame changed during source callbacks");
 }catch(...){owner->progress_.failed=true;throw;}
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_player_turn_dirty(
 const std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer>& issuer,const runtime::BnwStockLuaAttemptLease& lease){
 const auto p=impl_;need(p->creator==std::this_thread::get_id(),"Player dirty Engine foreign creator");
 need(issuer!=nullptr&&p->end_turn_alive&&!p->end_turn_failed,"Player dirty Engine/issuer retired");
 const auto serial=lease.operation_serial();lease.require_operation(serial);p->identity();ensure_end_turn_queue_owned();
 p->end_turn_queue->publish_player_turn_dirty(*this,issuer,lease);
 need(issuer->queued_without_callbacks()&&issuer->publisher_returned_without_callbacks(),"Player dirty original publisher did not return");
 p->identity();lease.require_operation(serial);
 need(p->end_turn_alive&&!p->end_turn_failed,"Player dirty Engine retired after actual publisher");
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::require_notification_model_origin_without_callbacks(const BnwOwnedNotificationPublicationOwner& owner,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& lease)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Notification model origin foreign Engine creator");
 // Existing private Stock scope and actual camera pump. No callback/current
 // provider is copied or invoked by this model origin admission.
 auto identity=lease.pin_active_scope_without_callbacks();pump.pure();
 const auto ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();const auto self=p->end_turn_self.lock();
 const auto actual=owner.self_.lock();const auto loan_ui=pump.ui_.lock();const auto loan_host=pump.lineage_.host.lock();
 const auto loan_world=pump.lineage_.world.lock();const auto control=pump.lineage_.control.lock();const auto source=pump.lineage_.canonical_source.lock();
 const auto pure=[&]{
  need(p->end_turn_alive&&!p->end_turn_failed&&self&&self.get()==this&&ui&&host&&world&&actual&&actual.get()==&owner&&
   owner.creator_==p->creator&&!owner.progress_.failed&&!owner.busy_&&owner.generation_==p->generation&&
   ui->notification_publication_owner_.get()==&owner&&!ui->notification_publication_owner_.owner_before(actual)&&
   !actual.owner_before(ui->notification_publication_owner_)&&ui->engine_ui_turn_owner_.get()==this&&
   !ui->engine_ui_turn_owner_.owner_before(self)&&!self.owner_before(ui->engine_ui_turn_owner_)&&
   !owner.ui_.owner_before(p->ui)&&!p->ui.owner_before(owner.ui_)&&!owner.host_.owner_before(p->host)&&!p->host.owner_before(owner.host_)&&
   !owner.world_.owner_before(p->world)&&!p->world.owner_before(owner.world_)&&!owner.engine_.owner_before(p->end_turn_self)&&!p->end_turn_self.owner_before(owner.engine_)&&
   ui->plot_yield_ui_thread_==p->creator&&!ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   owner.issued_frame_feed_&&owner.issued_ui_sequence_==ui->sequence_&&owner.issued_turn_==world->setup.game_turn&&
   owner.issued_frame_feed_->actor==host->active_player_id()&&owner.issued_frame_feed_->model_revision==owner.progress_.model_revision&&
   owner.issued_frame_feed_->received_prefix==owner.progress_.received_prefix&&owner.progress_.received_prefix<=p->notification_effects.size()&&
   host->session().read_state_lease().get()==world.get()&&host->session().state_generation()==p->generation,
   "Notification model origin is not genuine current canonical owner/feed");
  need(owner.model_loan_pins_<static_cast<std::size_t>(LONG_MAX-1)&&actual.use_count()>1+static_cast<long>(owner.model_loan_pins_)&&
   p->end_turn_pins<static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<static_cast<std::size_t>(LONG_MAX-2)&&
   p->end_turn_world_pins<static_cast<std::size_t>(LONG_MAX-2)&&self.use_count()>2+static_cast<long>(p->end_turn_pins)&&
   ui.use_count()>2+static_cast<long>(p->end_turn_ui_pins)&&host.use_count()>2&&world.use_count()>2+static_cast<long>(p->end_turn_world_pins),
   "Notification model outside canonical owner lost (counted loans excluded)");
  need(identity.creator==p->creator&&identity.serial&&identity.vm==pump.lineage_.vm&&identity.control&&control&&identity.control.get()==control.get()&&
   !identity.control.owner_before(control)&&!control.owner_before(identity.control)&&identity.source_owner&&source&&identity.source_owner.get()==source.get()&&
   !identity.source_owner.owner_before(source)&&!source.owner_before(identity.source_owner)&&loan_ui&&loan_host&&loan_world&&
   ui.get()==loan_ui.get()&&!ui.owner_before(loan_ui)&&!loan_ui.owner_before(ui)&&host.get()==loan_host.get()&&!host.owner_before(loan_host)&&!loan_host.owner_before(host)&&
   world.get()==loan_world.get()&&!world.owner_before(loan_world)&&!loan_world.owner_before(world)&&identity.world==world.get()&&pump.lineage_.generation==p->generation,
   "Notification model supplied source/control/VM/pump differs");
 };
 pure();const auto stored=pump.source_control_.lock();
 if(stored)need(stored.get()==identity.control.get()&&!stored.owner_before(identity.control)&&!identity.control.owner_before(stored)&&
  pump.vm_==identity.vm&&pump.operation_==identity.serial,"Notification model pump already has another operation");
 lease.require_scope_identity_without_callbacks(identity);pump.pure();pure();
}
BnwEngineUiNotificationEffect BnwEngineUiTurnOwner::notification_model_head_without_callbacks(const BnwOwnedNotificationPublicationOwner& owner)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Notification model head foreign Engine creator");
 // Default value storage is allocated before any borrowed actual FIFO row.
 // Real unsupported FX/minimap payloads have no message/summary strings.
 // Unexpected nonempty payloads refuse rather than invent a text producer.
 const auto ui=p->ui.lock();const auto host=p->host.lock();const auto world=p->world.lock();const auto self=p->end_turn_self.lock();const auto actual=owner.self_.lock();
 const auto pure=[&]{
  need(p->end_turn_alive&&!p->end_turn_failed&&self&&self.get()==this&&ui&&host&&world&&actual&&actual.get()==&owner&&
   owner.creator_==p->creator&&!owner.busy_&&!owner.progress_.failed&&ui->notification_publication_owner_.get()==&owner&&
   !ui->notification_publication_owner_.owner_before(actual)&&!actual.owner_before(ui->notification_publication_owner_)&&
   ui->engine_ui_turn_owner_.get()==this&&!ui->engine_ui_turn_owner_.owner_before(self)&&!self.owner_before(ui->engine_ui_turn_owner_)&&
   !owner.ui_.owner_before(p->ui)&&!p->ui.owner_before(owner.ui_)&&!owner.host_.owner_before(p->host)&&!p->host.owner_before(owner.host_)&&
   !owner.world_.owner_before(p->world)&&!p->world.owner_before(owner.world_)&&!owner.engine_.owner_before(p->end_turn_self)&&!p->end_turn_self.owner_before(owner.engine_)&&
   owner.generation_==p->generation&&owner.issued_frame_feed_&&owner.issued_ui_sequence_==ui->sequence_&&owner.issued_turn_==world->setup.game_turn&&
   owner.issued_frame_feed_->actor==host->active_player_id()&&owner.issued_frame_feed_->model_revision==owner.progress_.model_revision&&
   owner.issued_frame_feed_->received_prefix==owner.progress_.received_prefix&&host->session().read_state_lease().get()==world.get()&&host->session().state_generation()==p->generation&&
   owner.model_loan_pins_<static_cast<std::size_t>(LONG_MAX-1)&&actual.use_count()>1+static_cast<long>(owner.model_loan_pins_)&&
   p->end_turn_pins<static_cast<std::size_t>(LONG_MAX-2)&&self.use_count()>2+static_cast<long>(p->end_turn_pins)&&
   owner.progress_.unsupported&&owner.progress_.received_prefix<p->notification_effects.size(),
   "Notification model head has no genuine current unsupported FIFO row");
 };
 pure();BnwEngineUiNotificationEffect result;pure();
 const auto index=static_cast<std::size_t>(owner.progress_.received_prefix);const auto& head=p->notification_effects[index];
 need(head.kind==*owner.progress_.unsupported&&head.occurrence==owner.progress_.unsupported_occurrence&&
  (head.kind==BnwEngineUiNotificationEffectKind::gameplay_fx_null||head.kind==BnwEngineUiNotificationEffectKind::gameplay_minimap_notification)&&
  head.message.empty()&&head.summary.empty(),"Notification model head native kind/occurrence/payload differs");
 const auto prefix=owner.progress_.received_prefix,revision=owner.progress_.model_revision;const auto data=p->notification_effects.data();const auto size=p->notification_effects.size();
 result.kind=head.kind;result.actor=head.actor;result.lookup=head.lookup;result.turn=head.turn;result.x=head.x;result.y=head.y;
 result.game_data_index=head.game_data_index;result.extra_game_data=head.extra_game_data;result.type=head.type;
 result.generation=head.generation;result.world_revision=head.world_revision;result.occurrence=head.occurrence;
 pure();need(owner.progress_.received_prefix==prefix&&owner.progress_.model_revision==revision&&p->notification_effects.data()==data&&
  p->notification_effects.size()==size&&p->notification_effects[index]==result,"Notification actual head changed across readonly value copy");
 return result; // Data only; no FX/minimap/Scene/AFTER_SWAP receipt or accept.
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_attrition_city_info_dirty_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionCityInfoDirtyLoan& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"CityInfo counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "CityInfo actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition CityInfo counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition CityInfo canonical Engine Selection/world absent");
  selection->consume_attrition_city_info_dirty(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_city_info_dirty_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition CityInfo Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_city_info_dirty_return(loan);
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_attrition_unit_info_dirty_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionUnitInfoDirtyLoan& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"UnitInfo counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "UnitInfo actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition UnitInfo canonical Engine Selection/world absent");
  selection->consume_attrition_unit_info_dirty(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_unit_info_dirty_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_unit_info_dirty_return(loan);
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}

namespace civ5::ui {
bool BnwEngineUiTurnOwner::consume_attrition_unit_selected_read_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionUnitSelectedRead& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"IsSelected counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "IsSelected actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition IsSelected counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition IsSelected canonical Engine Selection/world absent");
  const bool result=selection->consume_attrition_unit_selected_read(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_unit_selected_read_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition IsSelected Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_unit_selected_read_return(loan);return result;
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_attrition_selected_unit_info_dirty_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionSelectedUnitInfoDirtyLoan& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"UnitInfo counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "UnitInfo actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition UnitInfo canonical Engine Selection/world absent");
  selection->consume_attrition_selected_unit_info_dirty(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_selected_unit_info_dirty_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_selected_unit_info_dirty_return(loan);
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}

namespace civ5::ui {
std::optional<std::int32_t> BnwEngineUiTurnOwner::consume_attrition_selection_plot_read_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionSelectionPlotRead& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"IsSelected counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "IsSelected actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition IsSelected counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition IsSelected canonical Engine Selection/world absent");
  const auto result=selection->consume_attrition_selection_plot_read(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_selection_plot_read_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition IsSelected Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_selection_plot_read_return(loan);return result;
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}

namespace civ5::ui {
void BnwEngineUiTurnOwner::consume_attrition_plot_list_dirty_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,const runtime::BnwAttritionPlotListDirtyLoan& loan,
 runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 EndTurnPin pin(weak);
 loan.require_alive_without_callbacks();const auto p=pin.storage;
 const auto pure_counted_post=[&]{
  loan.require_alive_without_callbacks();
  need(p&&p->creator==std::this_thread::get_id()&&p->end_turn_alive&&!p->end_turn_failed,"UnitInfo counted Engine retired/foreign before raw post");
  const auto self=p->end_turn_self.lock();const auto ui=p->ui.lock();
  need(self&&ui&&self.get()==pin.engine.get()&&ui->engine_ui_turn_owner_.get()==self.get()&&
   !self.owner_before(ui->engine_ui_turn_owner_)&&!ui->engine_ui_turn_owner_.owner_before(self)&&
   !ui->selected_owner_lifetime_.owner_before(p->ui)&&!p->ui.owner_before(ui->selected_owner_lifetime_)&&
   p->end_turn_pins<=static_cast<std::size_t>(LONG_MAX-2)&&p->end_turn_ui_pins<=static_cast<std::size_t>(LONG_MAX-2)&&
   self.use_count()>1+static_cast<long>(p->end_turn_pins)&&ui.use_count()>1+static_cast<long>(p->end_turn_ui_pins),
   "UnitInfo actual outside Engine/UI owner lost during callback");
 };
 try{
  loan.enter_engine_once_without_callbacks(*pin.engine,lease);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo counted Engine retired");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  const auto selection=p->selection.lock();const auto world=p->world.lock();
  need(selection&&world&&world.get()==&state,"Attrition UnitInfo canonical Engine Selection/world absent");
  selection->consume_attrition_plot_list_dirty(loan,state,lease);
  // The genuine child has adopted its actual return BEFORE callbackful POST.
  loan.require_alive_without_callbacks();
  selection->require_attrition_plot_list_dirty_return(loan);
  require_counted_localization_owner(pin);pure_counted_post();
  need(p->end_turn_alive&&!p->end_turn_failed,"Attrition UnitInfo Engine retired during POST");
  loan.require_engine_without_callbacks(*pin.engine,lease);
  selection->require_attrition_plot_list_dirty_return(loan);
 }catch(...){loan.fail_without_callbacks();p->end_turn_failed=true;throw;}
}
}
