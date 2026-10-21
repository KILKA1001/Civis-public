#include "civ5/ui/selected_city_production_services.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <atomic>
#include <climits>
#include <set>
#include "civ5/runtime/bnw_city_current_support_strategy.hpp"
#include <algorithm>
#include <utility>
#include <thread>
#include <type_traits>
namespace civ5::ui {
namespace {
void need(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
std::int32_t native(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Selected production native int32 overflow");return static_cast<std::int32_t>(v);}
auto& named(auto& rows,std::string_view name){
    using T=std::remove_reference_t<decltype(rows.front())>; T* found=nullptr;
    for(auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Selected production duplicate raw field");found=&r;}
    need(found,"Selected production raw field absent");return *found;
}
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr const char* dirty_barrier="Selected production requires typed SelectionButtons/CityScreen/PlotListButtons, CityInfo and SAME stock SetSpecificCityInfoDirty(PRODUCTION) publication";
}
struct SelectedCityProductionServices::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> invalidated{false};bool running=false;
    std::shared_ptr<const SelectedBnwPreviewHost> parent;
    runtime::LiveSession* session=nullptr;
    std::shared_ptr<const runtime::LiveSessionState> initial;
    std::uint64_t generation=0;
    std::function<bool()> rules_current;
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> stock;
    // Weak facade only; issued prefix history stays sticky even after caller
    // drop. Supplier additionally owns the genuine SAME-session registry.
    std::map<std::pair<std::int32_t,std::int32_t>,std::weak_ptr<const SelectedCityTrainCreateSource>> train_prefixes;
    void check(){
        if(thread!=std::this_thread::get_id()){invalidated.store(true);throw std::runtime_error("Selected production foreign creator thread");}
        need(!invalidated.load()&&parent&&session&&&parent->session()==session,"Selected production parent retired");
        need(rules_current&&rules_current(),"Selected production SAME immutable rules retired");
        need(session->state_generation()==generation&&session->read_state_lease().get()==initial.get(),"Selected production world allocation/generation changed");
        (void)session->bnw_game_debug_mode(); // mutable_state retires the actual import read owner.
    }
    void entry(){
        if(thread!=std::this_thread::get_id()){invalidated.store(true);throw std::runtime_error("Selected production foreign creator thread");}
        if(stock){
            const auto canonical=stock->retain_production_entry_owner_without_callbacks();
            need(canonical&&canonical.get()==stock.get()&&!canonical.owner_before(stock)&&!stock.owner_before(canonical),
                "Production entry requires actual canonical stock control");
        }
        check(); // only AFTER pure pending denial may DB/session guards run
    }
    struct Operation {
        Impl& p;
        explicit Operation(Impl& a):p(a){p.entry();if(p.running){p.invalidated.store(true);throw std::runtime_error("Selected production reentry");}p.running=true;}
        ~Operation(){p.running=false;}
    };
    SelectedMaintainPreflight inspect(const runtime::LiveSessionState& state,std::int64_t owner,std::int64_t handle,std::int64_t process){
        check();SelectedMaintainPreflight out;out.owner=native(owner);out.city=native(handle);out.process=native(process);
        need(state.setup.profile==content::TechnologyProfile::bnw&&state.cities.profile==content::TechnologyProfile::bnw&&state.world.game_state.layout==bnw&&state.world.pregame&&state.world.teams,"Selected production requires genuine BNW world");
        need(owner==parent->active_player_id()&&state.setup.players.contains(owner)&&state.setup.players.at(owner).alive,"Selected production active living owner absent");
        const auto* raw=runtime::resolve_bnw_current_city(state,out.owner,out.city);need(raw,"Selected production actual city absent");
        const auto& city=state.cities.players.at(owner).cities.at(handle);
        need(city.id==handle&&city.owner==owner&&city.production_orders&&city.production_progress&&city.production_progress->source_profile==content::TechnologyProfile::bnw&&city.production_progress->fully_resolved&&raw->production.layout==bnw&&raw->production.context.bnw_city_version,"Selected production native queue unresolved");
        // First bounded branch: no popOrder, making counters or operation slot.
        // Existing orders need all ordered pop notifications and commitments.
        const auto& queue=named(raw->production.blocks,"queue");
        need(queue.children.size()<=INT32_MAX&&named(queue.fields,"wire_count").value==static_cast<std::int64_t>(queue.children.size())&&queue.children.size()==city.production_orders->size(),"Selected production raw/owned queue count differs");
        if(!queue.children.empty()){out.blocker="Selected production existing queue requires actual popOrder commitment/operation/UI owners";return out;}
        const auto& db=parent->database();const auto& types=parent->types();
        const content::InfoTypeEntry* info=nullptr;
        for(const auto& [type,e]:types.entries())if(e.table=="Processes"&&e.id==process){(void)type;need(!info,"Selected production ambiguous ProcessInfo");info=&e;}
        if(!info){out.blocker="Selected production native ProcessInfo is absent";return out;}
        need(parent->production_rules().processes.contains(process)&&types.find_hash(info->hash)&&types.find_hash(info->hash)->type==info->type&&types.find_hash(info->hash)->table=="Processes"&&types.find_hash(info->hash)->id==process,"Selected production ProcessInfo hash/slot differs");
        std::size_t rows=0;
        auto report=db.query_rows({"SELECT ID, Type, TechPrereq FROM Processes WHERE ID=?",{process}},[&](const auto& row){
            ++rows;need(row.integer("ID")==process&&row.text("Type")==std::optional{info->type},"Selected production ProcessInfo source differs");
            const auto text=row.text("TechPrereq");const auto* prerequisite=text?types.find(*text):nullptr;
            out.prerequisite=prerequisite?native(prerequisite->id):-1;
            need(!prerequisite||prerequisite->table=="Technologies","Selected production TechPrereq wrong actual table");
        },2);need(report.all_succeeded()&&rows==1,"Selected production ProcessInfo query unavailable");
        const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
        need(owner>=0&&static_cast<std::uint64_t>(owner)<teams.size(),"Selected production raw owner team absent");
        const auto team=teams[static_cast<std::size_t>(owner)];need(team==state.setup.players.at(owner).team,"Selected production raw/owned owner team differs");
        const saves::TeamObservation* raw_team=nullptr;
        for(const auto& t:state.world.teams->teams)if(named(t.fields,"id").value==team){need(!raw_team,"Selected production duplicate raw team");raw_team=&t;}
        const runtime::TeamTechnologies* owned=nullptr;
        for(const auto& t:state.technologies)if(t.team()==team){need(!owned,"Selected production duplicate owned team");owned=&t;}
        need(raw_team&&owned,"Selected production owner TeamTechs absent");
        const auto restored=runtime::restore_team_technologies(*raw_team,parent->technologies(),types);
        need(owned->profile()==content::TechnologyProfile::bnw&&owned->slots()==restored.slots()&&owned->last_tech()==restored.last_tech()&&owned->nondefault_values()==restored.nondefault_values()&&owned->profile_extensions().values.empty()&&restored.remap().all_mapped(),"Selected production raw/owned TeamTechs differ");
        // Source HasTech(NO_TECH) is true; no fabricated default research flags.
        if(!owned->has_tech(out.prerequisite)){out.blocker="Selected production prerequisite technology is not acquired";return out;}
        bool league=false;
        const auto league_report=db.prefetch_info("LeagueProjects",[&](auto,auto,const auto& row){
            const auto text=row.text("Process");const auto* e=text?types.find(*text):nullptr;
            need(!e||e->table=="Processes","Selected production LeagueProject Process type differs");
            if(e&&e->id==process)league=true;return true;
        });need(league_report.all_succeeded()&&!league_report.overlapping_ids,"Selected production LeagueProjectInfo source unavailable");
        if(league){out.blocker="Selected production requires actual CanContributeToLeagueProject";return out;}
        // CvProcessInfo::CacheResults -> SetYields -> PopulateArrayByValue:
        // six native yields, default 0; same joined rows overwrite by ID.
        std::set<std::int64_t> seen;
        report=db.query_rows({"SELECT Yields.ID AS YieldID, Yield AS Amount FROM Process_ProductionYields INNER JOIN Yields ON YieldType=Yields.Type WHERE ProcessType=?",{info->type}},[&](const auto& row){
            const auto y=row.integer("YieldID");need(y>=0&&y<6&&seen.insert(y).second,"Selected production ambiguous/outside native process yield row");
            out.process_yields[static_cast<std::size_t>(y)]=native(row.integer("Amount"));
        });need(report.all_succeeded(),"Selected production native process yields unavailable");
        const auto& modifiers=named(raw->core.fields,"m_aiProductionToYieldModifier");
        need(state.cities.yield_slots==6&&modifiers.encoding==saves::CityCoreEncoding::vector_i32&&modifiers.wire_count==6&&modifiers.values.size()==6&&modifiers.hashes.empty()&&modifiers.bytes.empty()&&modifiers.row_counts.empty()&&modifiers.row_keys.empty(),"Selected production raw six-yield modifier unavailable");
        for(std::size_t y=0;y<6;++y){
            const auto it=city.yields.find(static_cast<std::int64_t>(y));const auto live=it==city.yields.end()?0:it->second.process_modifier;
            out.before[y]=native(modifiers.values[y]);need(live==out.before[y],"Selected production raw/owned process modifier differs");
            // processProcess(+1) and changeProductionToYieldModifier: native int
            // addition, no division, clamping, production award or rate refresh.
            out.after[y]=native(static_cast<std::int64_t>(out.before[y])+out.process_yields[y]);
        }
        out.native_eligible=true;check();return out;
    }
    std::string lua_blocker(){
        check();if(!stock)return "Selected production actual stock Lua owner is missing; PlayerCanMaintain and CityCanMaintain cannot run";
        auto result=stock->fork_at(*initial);check();
        // This actual owner currently always refuses. Even a future epoch is
        // not production hook escrow + atomic publication in this component.
        if(result.epoch)return "Selected production requires SAME stock PlayerCanMaintain/CityCanMaintain escrow and atomic publication adapter";
        return "Selected production actual stock Lua epoch unavailable: "+(result.blocker.empty()?std::string("all selected contexts/listeners are not published"):result.blocker);
    }
};
SelectedCityProductionServices::SelectedCityProductionServices(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
SelectedCityProductionServices::~SelectedCityProductionServices()=default;
std::shared_ptr<SelectedCityProductionServices> SelectedCityProductionServices::from_selected_preview(const SelectedBnwReadOnlyPreview& preview,std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> stock){
    if(stock){const auto canonical=stock->retain_production_entry_owner_without_callbacks();
        need(canonical.get()==stock.get()&&!canonical.owner_before(stock)&&!stock.owner_before(canonical),"Production factory stock control differs");}
    auto p=std::make_shared<Impl>();p->stock=std::move(stock);
    preview.with_owned_host_camera_input([p](auto parent,const LiveBnwPreviewInput* input){
        need(parent&&input&&&parent->session()==&input->session,"Selected production requires SAME genuine retained parent/session");
        p->parent=std::move(parent);p->session=&input->session;p->initial=p->session->read_state_lease();p->generation=p->session->state_generation();
        p->rules_current=p->parent->database().immutable_preview_fingerprint_currentness();
        p->entry();need(p->parent->database().gameplay_fingerprint_sha256()==p->parent->identity().rules_sha256,"Selected production actual rules identity differs");p->entry();
    });
    return std::shared_ptr<SelectedCityProductionServices>(new SelectedCityProductionServices(std::move(p)));
}
std::shared_ptr<SelectedCityProductionServices> SelectedCityProductionServices::from_actual_source_owner(
 std::shared_ptr<const SelectedBnwPreviewHost> parent,runtime::LiveSession& session,
 std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> stock){
 need(parent&&stock,"Production private SourceOwner factory inputs absent");
 const auto canonical=stock->retain_production_entry_owner_without_callbacks();
 need(canonical.get()==stock.get()&&!canonical.owner_before(stock)&&!stock.owner_before(canonical),"Production factory canonical stock control differs");
 need(&parent->session()==&session,"Production private SourceOwner factory SAME Host/session absent");
 auto p=std::make_shared<Impl>();p->parent=std::move(parent);p->stock=std::move(stock);p->session=&session;
 p->initial=session.read_state_lease();p->generation=session.state_generation();
 p->rules_current=p->parent->database().immutable_preview_fingerprint_currentness();p->entry();
 need(p->parent->database().gameplay_fingerprint_sha256()==p->parent->identity().rules_sha256,"Production private SourceOwner actual rules identity differs");p->entry();
 return std::shared_ptr<SelectedCityProductionServices>(new SelectedCityProductionServices(std::move(p)));
}
SelectedMaintainPreflight SelectedCityProductionServices::native_maintain_preflight(std::int64_t owner,std::int64_t city,std::int64_t process)const{
    const auto p=impl_;Impl::Operation operation(*p);auto result=p->inspect(*p->initial,owner,city,process);p->entry();return result;
}
std::string SelectedCityProductionServices::unavailable_reason()const{
    const auto p=impl_;Impl::Operation operation(*p);auto reason=p->lua_blocker();p->entry();return reason;
}
runtime::BnwProductionOrderServices SelectedCityProductionServices::production_services()const{
    const auto p=impl_;p->entry();runtime::BnwProductionOrderServices s;
    // Throw preserves the precise owner frontier through existing UI availability.
    s.lua_ready=[p]()->bool{Impl::Operation op(*p);throw std::runtime_error(p->lua_blocker());};
    s.core_can_order_read_only=[p](const auto& state,auto owner,auto city,const auto& choice){Impl::Operation op(*p);need(choice.kind==runtime::BnwProductionKind::maintain,"Selected production only ordinary MAINTAIN native preflight is implemented");const auto native_result=p->inspect(state,owner,city,choice.item);if(!native_result.native_eligible)return false;throw std::runtime_error(p->lua_blocker());};
    s.lua_test_all_read_only=[p](std::string_view,auto,auto,auto,const auto&)->std::optional<bool>{Impl::Operation op(*p);throw std::runtime_error(p->lua_blocker());};
    // Native effect calculations above are observational. No detached effects
    // may be admitted before both hook escrow and typed UI lifecycle exist.
    s.apply_owned_effect=[p](auto&,auto,auto,auto,const auto&){Impl::Operation op(*p);throw std::runtime_error(dirty_barrier);};
    s.dirty_after_commit=[p](auto,auto){Impl::Operation op(*p);throw std::runtime_error(dirty_barrier);};
    return s;
}
BnwProductionPreview SelectedCityProductionServices::production_preview()const{
    const auto p=impl_;p->entry();return [p](const auto& state,auto owner,auto city,const auto& choice){
        Impl::Operation op(*p);if(choice.kind!=runtime::BnwProductionKind::maintain)return UiCommandResult{UiCommandStatus::unavailable,"Selected production only ordinary MAINTAIN native preflight is implemented"};
        const auto result=p->inspect(state,owner,city,choice.item);
        if(!result.native_eligible)return UiCommandResult{UiCommandStatus::unavailable,result.blocker};
        return UiCommandResult{UiCommandStatus::unavailable,p->lua_blocker()};
    };
}

namespace {
bool same_yields(const std::map<std::int64_t,runtime::CityYieldComponents>& a,
                 const std::map<std::int64_t,runtime::CityYieldComponents>& b)noexcept {
    if(a.size()!=b.size())return false;
    auto x=a.begin(),y=b.begin();
    for(;x!=a.end();++x,++y){
        const auto& p=x->second;const auto& q=y->second;
        if(x->first!=y->first||p.terrain!=q.terrain||p.buildings!=q.buildings||
           p.specialists!=q.specialists||p.misc!=q.misc||p.religion!=q.religion||
           p.per_population!=q.per_population||p.per_religion!=q.per_religion||
           p.rate_modifier!=q.rate_modifier||p.resource_modifier!=q.resource_modifier||
           p.process_modifier!=q.process_modifier)return false;
    }return true;
}
bool same_quote(const SelectedMaintainPreflight& a,const SelectedMaintainPreflight& b)noexcept {
    return a.native_eligible&&b.native_eligible&&a.owner==b.owner&&a.city==b.city&&
        a.process==b.process&&a.prerequisite==b.prerequisite&&a.before==b.before&&
        a.after==b.after&&a.process_yields==b.process_yields;
}
}
struct SelectedMaintainPreparedAction::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::function<void()> current;
    std::function<SelectedMaintainPreflight()> quote;
    std::shared_ptr<const runtime::LiveSessionState> state;
    SelectedMaintainPreflight before;
    const saves::SavedCityObservation* raw=nullptr;
    std::uint64_t ui_revision=0,version=0;
    bool committed=false;
    std::map<std::int64_t,runtime::CityYieldComponents> before_yields,after_yields;
    std::vector<saves::CityProductionRecord> after_raw_queue;
    std::vector<runtime::CityYieldState::ProductionOrder> after_orders;
    std::vector<std::int64_t> after_modifiers;
    void match(const runtime::LiveSessionState& actual)const {
        need(thread==std::this_thread::get_id(),"Selected production action foreign creator thread");
        need(!committed&&&actual==state.get(),"Selected production action consumed or wrong world");
        need(actual.ui_source_revision==ui_revision&&ui_revision<UINT64_MAX,"Selected production source revision changed/overflow");
        const auto* c=runtime::resolve_bnw_current_city(actual,before.owner,before.city);
        need(c==raw&&c&&c->production.context.bnw_city_version==version&&c->production.layout==bnw,"Selected production current city/version changed");
        const auto& q=named(c->production.blocks,"queue");
        need(q.children.empty()&&q.arrays.empty()&&q.blobs.empty()&&q.fields.size()==1&&named(q.fields,"wire_count").value==0,"Selected production prepared empty queue changed");
        const auto& owned=actual.cities.players.at(before.owner).cities.at(before.city);
        need(owned.production_orders&&owned.production_orders->empty()&&same_yields(owned.yields,before_yields),"Selected production prepared owned queue/yields changed");
        const auto& m=named(c->core.fields,"m_aiProductionToYieldModifier");
        need(m.encoding==saves::CityCoreEncoding::vector_i32&&m.wire_count==6&&m.values.size()==6&&m.hashes.empty()&&m.bytes.empty()&&m.row_counts.empty()&&m.row_keys.empty(),"Selected production prepared raw modifier shape changed");
        for(std::size_t y=0;y<6;++y)need(m.values[y]==before.before[y],"Selected production prepared raw modifier changed");
    }
};
SelectedMaintainPreparedAction::SelectedMaintainPreparedAction(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
SelectedMaintainPreparedAction::~SelectedMaintainPreparedAction()=default;
std::int32_t SelectedMaintainPreparedAction::owner()const noexcept{return impl_->before.owner;}
std::int32_t SelectedMaintainPreparedAction::city()const noexcept{return impl_->before.city;}
std::int32_t SelectedMaintainPreparedAction::process()const noexcept{return impl_->before.process;}
bool SelectedMaintainPreparedAction::native_committed()const noexcept{return impl_->committed;}
void SelectedMaintainPreparedAction::require_current_native()const{
    auto& a=*impl_;need(a.thread==std::this_thread::get_id(),"Selected production action foreign creator thread");
    a.current();need(!a.committed,"Selected production action was already consumed");
    const auto now=a.quote();need(same_quote(a.before,now),"Selected production native inputs changed during stock hooks");
    a.current();a.match(*a.state);
}
void SelectedMaintainPreparedAction::apply_native_in_place(runtime::LiveSessionState& state){
    auto& a=*impl_;require_current_native();
    // Obtain mutable targets ONLY from the supplier's actual SAME in-place
    // session handoff. No const_cast, detached state or generic mutable_state.
    need(&state==a.state.get(),"Selected production native writer received a different world");
    saves::SavedCityObservation* raw=nullptr;
    for(auto& p:state.world.all_players->players)for(auto& c:p.cities.cities)
        if(&c==a.raw)raw=&c;
    need(raw,"Selected production native writer lost current registry city");
    auto& queue=named(raw->production.blocks,"queue");
    auto& count=named(queue.fields,"wire_count");
    auto& modifiers=named(raw->core.fields,"m_aiProductionToYieldModifier");
    auto& city=state.cities.players.at(a.before.owner).cities.at(a.before.city);
    auto& orders=*city.production_orders;
    a.match(state); // last semantic check; no callbacks/allocation after this.
    static_assert(noexcept(queue.children.swap(a.after_raw_queue)));
    static_assert(noexcept(orders.swap(a.after_orders)));
    static_assert(noexcept(city.yields.swap(a.after_yields)));
    static_assert(noexcept(modifiers.values.swap(a.after_modifiers)));
    // CvCity::pushOrder(maintain, bPop=true,bAppend=false), genuine EMPTY
    // queue: clear/stop are no-ops, insertBeginning then processProcess(+1).
    queue.children.swap(a.after_raw_queue);count.value=1;
    orders.swap(a.after_orders);
    modifiers.values.swap(a.after_modifiers);city.yields.swap(a.after_yields);
    ++state.ui_source_revision;a.committed=true;
}
std::unique_ptr<SelectedMaintainPreparedAction> SelectedCityProductionServices::prepare_maintain(
    std::int64_t owner,std::int64_t city,std::int64_t process)const {
    const auto p=impl_;p->entry();
    auto a=std::make_unique<SelectedMaintainPreparedAction::Impl>();
    a->state=p->initial;a->before=p->inspect(*a->state,owner,city,process);
    if(!a->before.native_eligible)throw std::runtime_error(a->before.blocker);
    a->current=[p]{p->entry();};
    a->quote=[p,owner,city,process]{return p->inspect(*p->initial,owner,city,process);};
    a->raw=runtime::resolve_bnw_current_city(*a->state,a->before.owner,a->before.city);
    a->version=*a->raw->production.context.bnw_city_version;
    a->ui_revision=a->state->ui_source_revision;
    a->before_yields=a->state->cities.players.at(owner).cities.at(city).yields;
    a->after_yields=a->before_yields;
    for(std::size_t y=0;y<6;++y){
        // Native zero deltas perform no setter. Do not synthesize absent rows.
        if(a->before.process_yields[y])a->after_yields[static_cast<std::int64_t>(y)].process_modifier=a->before.after[y];
        a->after_modifiers.push_back(a->before.after[y]);
    }
    const content::InfoTypeEntry* info=nullptr;
    for(const auto& [type,e]:p->parent->types().entries())if(e.table=="Processes"&&e.id==process){(void)type;need(!info,"Selected production ambiguous prepared Info");info=&e;}
    need(info,"Selected production prepared Info missing");
    const bool hashed=a->version>=5;
    // These live-generated logical observations have NO imported wire span.
    // Zero start/end identifies new records; not a claim of saved byte parity.
    saves::CityProductionRecord order{"order",0,0,0,{}, {}, {}, {}};
    order.fields={{"eOrderType",4},{hashed?"iData1_hash":"iData1",hashed?static_cast<std::int64_t>(info->hash):process},{"iData2",-1},{"bSave",0},{"bRush",0}};
    a->after_raw_queue.push_back(std::move(order));
    a->after_orders.push_back({4,process,-1,false,false,hashed?std::optional<std::uint32_t>{info->hash}:std::nullopt});
    a->current();a->match(*a->state);
    return std::unique_ptr<SelectedMaintainPreparedAction>(new SelectedMaintainPreparedAction(std::move(a)));
}
runtime::SelectedProductionResult SelectedCityProductionServices::run_maintain(
    std::int64_t owner,std::int64_t city,std::int64_t process,bool execute)const {
    const auto p=impl_;Impl::Operation operation(*p);
    using D=runtime::SelectedProductionDisposition;
    if(!p->stock)return {D::blocked,false,"Actual SAME stock production owner is missing"};
    std::unique_ptr<SelectedMaintainPreparedAction> action;
    std::unique_ptr<runtime::BnwSelectedProductionActionLoan> loan;
    // All native allocations/eligibility and genuine issuer admission precede
    // any hook invocation. A refused borrow cannot fabricate readiness.
    try{
        action=prepare_maintain(owner,city,process);
        loan=runtime::borrow_selected_city_production(p->stock,p->parent,action->owner(),action->city());
        need(static_cast<bool>(loan),"Actual SAME stock production loan unavailable");
        loan->require_same(*p->parent,*p->session,action->owner(),action->city());
        action->require_current_native();
    }catch(const std::exception& e){return {D::blocked,false,e.what()};}
    try{
        auto result=execute?loan->execute_maintain(*action):loan->preview_maintain(*action);
        const bool committed=action->native_committed();
        // Producer-owned cleanup is inside the operation/bookend interval.
        loan.reset();
        if(result.native_committed!=committed||(!execute&&committed)||
           (result.disposition==D::committed&&!committed)||
           (execute&&result.disposition==D::eligible)){
            p->invalidated.store(true);
            return {D::uncertain,committed,"Actual stock production result disagrees with once native stores; operation quarantined"};
        }
        if(result.disposition==D::uncertain)p->invalidated.store(true);
        else {p->entry();if(!committed)action->require_current_native();}
        return result;
    }catch(const std::exception& e){
        p->invalidated.store(true);
        return {D::uncertain,action->native_committed(),std::string("Stock production callback/publication failed; cannot retry: ")+e.what()};
    }catch(...){p->invalidated.store(true);return {D::uncertain,action->native_committed(),"Stock production callback/publication failed; cannot retry"};}
}
runtime::SelectedProductionResult SelectedCityProductionServices::preview_maintain_command(std::int64_t owner,std::int64_t city,std::int64_t process)const{
    return run_maintain(owner,city,process,false);
}
runtime::SelectedProductionResult SelectedCityProductionServices::execute_maintain_command(std::int64_t owner,std::int64_t city,std::int64_t process)const{
    return run_maintain(owner,city,process,true);
}
namespace {
std::int32_t source_define(const content::RulesDatabase& db,std::string_view name){
    const auto r=db.global_define(name,content::GlobalDefineKind::integer);
    need(r.found&&r.sqlite_code==0&&r.error.empty()&&std::holds_alternative<std::int64_t>(r.value),"Construct actual native integer define absent");
    return native(std::get<std::int64_t>(r.value));
}
std::int32_t raw_player_scalar(const saves::SavedPlayerObservation& p,std::string_view name){
    const auto& f=named(p.core.fields,name);need(f.values.size()==1&&!f.wire_count&&f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),"Construct actual player scalar shape unavailable");
    return native(f.values.front());
}
const saves::PlayerCoreField& making(const saves::SavedPlayerObservation& p,std::uint64_t slots){
    const auto& f=named(p.core.fields,"m_paiBuildingClassMaking");
    need(f.encoding==saves::PlayerCoreEncoding::vector_i32&&f.wire_count==slots&&f.values.size()==slots&&f.row_counts.empty()&&f.hashes.empty()&&f.bytes.empty(),"Construct actual BuildingClassMaking vector unavailable");
    for(const auto v:f.values)need(native(v)>=0,"Construct native making negative");return f;
}
SelectedConstructPreflight inspect_construct(const SelectedBnwPreviewHost& host,const runtime::LiveSessionState& s,
    std::int64_t owner,std::int64_t city,std::int64_t building,const std::function<void()>& current){
    current();SelectedConstructPreflight out;out.owner=native(owner);out.city=native(city);out.building=native(building);
    need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.pregame&&s.world.teams&&s.world.all_players&&s.world.game_state.layout==bnw,"Construct requires actual SAME BNW state");
    need(owner==host.active_player_id()&&s.setup.players.at(owner).alive,"Construct actual active living owner unavailable");
    const auto* raw=runtime::resolve_bnw_current_city(s,out.owner,out.city);need(raw,"Construct actual city unavailable");
    const auto& live=s.cities.players.at(owner).cities.at(city);
    need(live.production_orders&&live.production_orders->empty()&&live.production_progress&&live.production_progress->fully_resolved&&live.production_progress->source_profile==content::TechnologyProfile::bnw&&raw->production.layout==bnw&&raw->production.context.bnw_city_version,"Construct existing/unresolved queue requires genuine pop commitment/operation owners");
    const auto& q=named(raw->production.blocks,"queue");need(q.children.empty()&&q.fields.size()==1&&q.arrays.empty()&&q.blobs.empty()&&named(q.fields,"wire_count").value==0,"Construct actual raw empty queue differs");
    const auto& db=host.database();const auto& types=host.types();
    const content::InfoTypeEntry* info=nullptr;
    for(const auto& [key,e]:types.entries())if(e.table=="Buildings"&&e.id==building){(void)key;need(!info,"Construct ambiguous actual BuildingInfo");info=&e;}
    const auto* hash=info?types.find_hash(info->hash):nullptr;
    need(info&&host.production_rules().buildings.contains(building)&&hash&&hash->table==info->table&&hash->type==info->type&&hash->id==info->id,"Construct actual BuildingInfo hash/slot unavailable");
    const auto type=[&](const std::optional<std::string>& text,std::string_view table){
        if(!text||text->empty())return std::int32_t{-1};const auto* t=types.find(*text);
        need(t&&t->table==table&&t->id>=0,"Construct rule reference unresolved");return native(t->id);
    };
    std::size_t rows=0;std::int32_t cost=0,prerequisite=-1,cities_needed=0,level_needed=0;bool espionage=false;
    auto report=db.query_rows({"SELECT * FROM Buildings WHERE ID=?",{building}},[&](const auto& r){
        ++rows;need(r.integer("ID")==building&&r.text("Type")==std::optional{info->type},"Construct actual native BuildingInfo source differs");
        out.building_class=type(r.text("BuildingClass"),"BuildingClasses");cost=native(r.integer("Cost"));prerequisite=type(r.text("PrereqTech"),"Technologies");
        espionage=r.boolean("Espionage");cities_needed=native(r.integer("CitiesPrereq"));level_needed=native(r.integer("LevelPrereq"));
        // Finite ordinary branch: these reached positive requirements need
        // separate native owners, never inferred false or waived.
        for(const auto name:{"PolicyBranchType","ReplacementBuildingClass","VictoryPrereq","MaxStartEra","NearbyTerrainRequired","ProhibitedCityTerrain"})
            need(!r.text(name)||r.text(name)->empty(),"Construct reached policy/replacement/victory/era/location requirement unsupported");
        for(const auto name:{"Water","River","FreshWater","Mountain","NearbyMountainRequired","Hill","Flat","HolyCity","NoOccupiedUnhappiness"})
            need(!r.boolean(name),"Construct reached special location/religion/occupation requirement unsupported");
        need(r.integer("MutuallyExclusiveGroup")==-1,"Construct reached mutually exclusive group unsupported");
    },2);need(report.all_succeeded()&&rows==1,"Construct BuildingInfo query unavailable");
    const auto saved=runtime::detail::bnw_culture_saved_no_espionage_option(s.world.pregame->game_options);
    bool no_espionage=false;if(saved)no_espionage=*saved==1;
    else{std::optional<std::int32_t> fallback;report=db.query_rows({"Select \"Default\" from GameOptions where Type=? LIMIT 1",{std::string("GAMEOPTION_NO_ESPIONAGE")}},[&](const auto& r){fallback=native(r.integer_at(0));},1);need(report.all_succeeded(),"Construct actual GameOption default query unavailable");no_espionage=fallback&&*fallback==1;}
    if(no_espionage&&espionage){out.blocker="Construct non-espionage game excludes this building";return out;}
    std::uint64_t class_slots=0;bool class_found=false,no_limit=false;
    const auto cr=db.prefetch_info("BuildingClasses",[&](auto slot,auto id,const auto& r){
        need(slot==static_cast<std::uint64_t>(id)&&id>=0,"Construct native class slot differs");
        if(id==out.building_class){class_found=true;no_limit=r.boolean("NoLimit");
            need(r.integer("MaxGlobalInstances")==-1&&r.integer("MaxTeamInstances")==-1&&r.integer("MaxPlayerInstances")==-1,"Construct limited building class unsupported");}
        return true;
    });need(cr.all_succeeded()&&!cr.overlapping_ids&&cr.rows+cr.holes==cr.slots&&class_found&&cr.slots<=INT32_MAX,"Construct actual BuildingClassInfo collection unavailable");class_slots=cr.slots;
    runtime::CityYieldRules replacements;runtime::load_city_building_replacements(db,replacements);
    const auto& ids=s.world.pregame->slots.integer_arrays.at("civilizations");const auto& keys=s.world.pregame->slots.string_arrays.at("civilization_keys");
    need(ids.size()==64&&keys.size()==64&&owner>=0&&owner<64&&ids.at(static_cast<std::size_t>(owner))==s.cities.players.at(owner).civilization,"Construct actual raw/owned civilization differs");
    const auto key=keys.at(static_cast<std::size_t>(owner)).substr(0,keys.at(static_cast<std::size_t>(owner)).find('\0'));
    need(type(std::optional{key},"Civilizations")==ids.at(static_cast<std::size_t>(owner)),"Construct actual civilization native Type differs");
    const auto& mapping=replacements.civilization_buildings.at(ids.at(static_cast<std::size_t>(owner)));
    const auto mapped=mapping.find(out.building_class);
    if(mapped==mapping.end()||mapped->second!=building){out.blocker="Construct civilization does not use this building";return out;}
    if(cost==-1){out.blocker="Construct native production cost is -1";return out;}
    const auto& team_ids=s.world.pregame->archive.integer_arrays.at("team_types");need(team_ids.size()==64,"Construct actual teams unavailable");
    const auto team=native(team_ids.at(static_cast<std::size_t>(owner)));need(team==s.setup.players.at(owner).team&&team>=0&&team<64&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Construct actual owner team differs");
    const auto& t=s.world.teams->teams.at(static_cast<std::size_t>(team));need(t.index==static_cast<std::uint64_t>(team)&&named(t.fields,"id").value==team,"Construct raw team registry differs");
    const auto restored=runtime::restore_team_technologies(t,host.technologies(),types);const runtime::TeamTechnologies* owned=nullptr;
    for(const auto& e:s.technologies)if(e.team()==team){need(!owned,"Construct duplicate owned tech team");owned=&e;}
    need(owned&&owned->profile()==content::TechnologyProfile::bnw&&owned->slots()==restored.slots()&&owned->last_tech()==restored.last_tech()&&owned->nondefault_values()==restored.nondefault_values()&&owned->profile_extensions().values.empty()&&restored.remap().all_mapped(),"Construct actual raw/owned TeamTechs differs");
    out.technologies.push_back(prerequisite);
    if(!owned->has_tech(prerequisite)){out.blocker="Construct prerequisite technology is not acquired";return out;}
    const auto and_slots=source_define(db,"NUM_BUILDING_AND_TECH_PREREQS");need(and_slots>=0&&static_cast<std::uint64_t>(and_slots)<=owned->slots(),"Construct native AND-tech array loop bound unsupported");
    report=db.query_rows({"SELECT Technologies.ID FROM Building_TechAndPrereqs INNER JOIN Technologies ON TechType=Technologies.Type WHERE BuildingType=?",{info->type}},[&](const auto& r){out.technologies.push_back(native(r.integer_at(0)));});
    need(report.all_succeeded()&&out.technologies.size()-1<=static_cast<std::uint64_t>(and_slots),"Construct native AND-tech array unavailable");
    for(const auto tech:out.technologies)if(!owned->has_tech(tech)){out.blocker="Construct additional prerequisite technology is not acquired";return out;}
    const auto building_slots=host.production_rules().building_slots;need(building_slots<=INT32_MAX&&building>=0&&static_cast<std::uint64_t>(building)<building_slots,"Construct native Building array span unavailable");
    const auto& obsolete=named(t.hashes,"obsolete_building_count");need(obsolete.wire_count>=0&&static_cast<std::uint64_t>(obsolete.wire_count)==obsolete.rows.size(),"Construct actual obsolete hash count differs");
    std::vector<std::int32_t> ob(static_cast<std::size_t>(building_slots),0);std::set<std::int32_t> seen;
    for(const auto& row:obsolete.rows){need(row.type_hash<=UINT32_MAX,"Construct obsolete hash width differs");if(!row.type_hash){need(row.values.empty(),"Construct null obsolete hash has payload");continue;}
        const auto* e=types.find_hash(row.type_hash);need(e&&e->table=="Buildings"&&e->id>=0&&static_cast<std::uint64_t>(e->id)<building_slots&&row.values.size()==1&&seen.insert(native(e->id)).second,"Construct obsolete Building Reset/Read remap unavailable");ob.at(static_cast<std::size_t>(e->id))=native(row.values[0]);}
    if(ob.at(static_cast<std::size_t>(building))>0){out.blocker="Construct native Team obsolete cache excludes this building";return out;}
    // Complete PopulateArrayByValue/Existence inner joins. Missing type refs
    // have the same no-row meaning as native SQL; reached real rows refuse.
    const auto absent=[&](const char* sql){bool any=false;const auto rr=db.query_rows({sql,{info->type}},[&](const auto&){any=true;});need(rr.all_succeeded()&&!any,"Construct reached resource/class prerequisite or locked-building owner unsupported");};
    absent("SELECT 1 FROM Building_ResourceQuantityRequirements INNER JOIN Resources ON ResourceType=Resources.Type WHERE BuildingType=? AND Cost>0");
    absent("SELECT 1 FROM Building_PrereqBuildingClasses INNER JOIN BuildingClasses ON BuildingClassType=BuildingClasses.Type WHERE BuildingType=? AND NumBuildingNeeded<>0");
    absent("SELECT 1 FROM Building_ClassesNeededInCity INNER JOIN BuildingClasses ON BuildingClassType=BuildingClasses.Type WHERE BuildingType=?");
    absent("SELECT 1 FROM Building_LockedBuildingClasses INNER JOIN BuildingClasses ON BuildingClassType=BuildingClasses.Type WHERE BuildingType=?");
    absent("SELECT 1 FROM Building_LocalResourceAnds INNER JOIN Resources ON ResourceType=Resources.Type WHERE BuildingType=?");
    absent("SELECT 1 FROM Building_LocalResourceOrs INNER JOIN Resources ON ResourceType=Resources.Type WHERE BuildingType=?");
    for(const auto& [cls,id]:mapping){need(cls>=0&&static_cast<std::uint64_t>(cls)<class_slots,"Construct civilization class mapping outside span");if(id==-1)continue;
        need(id>=0&&static_cast<std::uint64_t>(id)<building_slots,"Construct civilization Building mapping outside span");
        // Player obsolete-prereq branch would read class counts even when its
        // cached required count is 0; this finite slice stops at that branch.
        need(ob.at(static_cast<std::size_t>(id))<=0,"Construct reached obsolete empire prerequisite branch unsupported");}
    need(s.world.all_players->players.size()==64,"Construct actual Player collection unavailable");
    const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(owner));need(player.saved_player_id==owner,"Construct actual player ID differs");
    out.making_values=making(player,class_slots).values;need(out.building_class>=0&&static_cast<std::uint64_t>(out.building_class)<class_slots,"Construct native class index outside span");
    out.making_before=native(out.making_values.at(static_cast<std::size_t>(out.building_class)));out.making_after=native(static_cast<std::int64_t>(out.making_before)+1);
    out.team_making=0;
    for(std::size_t i=0;i<64;++i){const auto& p=s.world.all_players->players.at(i);need(p.saved_player_id==static_cast<std::int64_t>(i),"Construct actual ascending player slots differ");
        const auto alive=raw_player_scalar(p,"m_bAlive");need((alive==0||alive==1)&&s.setup.players.at(static_cast<std::int64_t>(i)).alive==(alive!=0),"Construct raw/owned living player differs");
        if(!alive)continue;const auto current_team=native(team_ids[i]);need(current_team==s.setup.players.at(static_cast<std::int64_t>(i)).team,"Construct raw/owned team differs");
        if(current_team==team)out.team_making=native(static_cast<std::int64_t>(out.team_making)+making(p,class_slots).values.at(static_cast<std::size_t>(out.building_class)));
    }
    const auto num_cities=native(static_cast<std::int64_t>(player.cities.cities.size()));
    if(num_cities<cities_needed||raw_player_scalar(player,"m_iHighestUnitLevel")<level_needed){out.blocker="Construct city-count/unit-level prerequisite is not met";return out;}
    const auto city_max=source_define(db,"CITY_MAX_NUM_BUILDINGS");
    // Native empire prerequisite count loop is evaluated even for required0.
    // Validate genuine counts for every actual city, not a fabricated zero.
    for(const auto& c:player.cities.cities){
        const auto id=native(named(c.core.fields,"m_iID").values.at(0));need(runtime::resolve_bnw_current_city(s,out.owner,id)==&c,"Construct actual empire city registry differs");
        const auto& own=s.cities.players.at(owner).cities.at(id);const auto& buildings=named(c.production.blocks,"buildings");
        need(own.building_counts_resolved&&c.production.layout==bnw&&c.production.context.bnw_city_version==c.core.version&&named(buildings.fields,"version").value>=0&&named(buildings.fields,"version").value<=1,"Construct actual building counts unresolved");
        const auto real=runtime::detail::bnw_native_building_counts_reset_read_math(named(buildings.children,"m_paiNumRealBuilding"),building_slots,types);
        const auto free=runtime::detail::bnw_native_building_counts_reset_read_math(named(buildings.children,"m_paiNumFreeBuilding"),building_slots,types);
        for(const auto* values:{&own.real_buildings,&own.free_buildings})for(const auto& [key,n]:*values)need(key>=0&&static_cast<std::uint64_t>(key)<building_slots&&native(n)>=0,"Construct owned building count outside actual span");
        for(std::size_t b=0;b<real.size();++b){const auto key=static_cast<std::int64_t>(b);const auto r=own.real_buildings.find(key),f=own.free_buildings.find(key);
            need(real[b]>=0&&free[b]>=0&&(r==own.real_buildings.end()?0:r->second)==real[b]&&(f==own.free_buildings.end()?0:f->second)==free[b],"Construct actual raw/owned Building Reset/Read counts differ");
            const auto count=city_max<=1?std::max(real[b],free[b]):native(static_cast<std::int64_t>(real[b])+free[b]);
            if(&c==raw&&key==building)out.city_count=count;
        }
        const auto& puppet=named(c.core.fields,"m_bPuppet");need(puppet.encoding==saves::CityCoreEncoding::boolean&&puppet.values.size()==1&&(puppet.values[0]==0||puppet.values[0]==1)&&own.puppet==(puppet.values[0]!=0),"Construct empire puppet flag differs");
    }
    out.player_native_eligible=true;
    // City-specific conditions are OBSERVED here, but Loan must execute actual
    // PlayerCanConstruct BEFORE consuming this phase (including a city veto).
    if(out.city_count>=city_max){out.blocker="Construct native city already has maximum count";current();return out;}
    if(!no_limit)need(source_define(db,"MAX_BUILDINGS_PER_CITY")==-1,"Construct reached finite city-total building limit owner unsupported");
    out.city_native_eligible=true;current();return out;
}
bool same_construct(const SelectedConstructPreflight& a,const SelectedConstructPreflight& b){
    return a.owner==b.owner&&a.city==b.city&&a.building==b.building&&a.building_class==b.building_class&&a.technologies==b.technologies&&a.making_values==b.making_values&&a.making_before==b.making_before&&a.making_after==b.making_after&&a.team_making==b.team_making&&a.city_count==b.city_count&&a.player_native_eligible==b.player_native_eligible&&a.city_native_eligible==b.city_native_eligible;
}
}
struct SelectedConstructPreparedAction::Impl {
    std::function<void()> current;
    std::function<SelectedConstructPreflight()> quote;
    std::shared_ptr<const runtime::LiveSessionState> state;
    SelectedConstructPreflight before;
    const saves::SavedCityObservation* raw=nullptr;
    const std::thread::id thread=std::this_thread::get_id();
    std::uint64_t revision=0,version=0;
    bool committed=false;
    std::vector<std::int64_t> after_making;
    std::vector<saves::CityProductionRecord> after_queue;
    std::vector<runtime::CityYieldState::ProductionOrder> after_orders;
    void check()const{need(thread==std::this_thread::get_id(),"Construct action foreign creator thread");current();need(!committed,"Construct action already consumed");}
};
SelectedConstructPreparedAction::SelectedConstructPreparedAction(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
SelectedConstructPreparedAction::~SelectedConstructPreparedAction()=default;
std::int32_t SelectedConstructPreparedAction::owner()const noexcept{return impl_->before.owner;}
std::int32_t SelectedConstructPreparedAction::city()const noexcept{return impl_->before.city;}
std::int32_t SelectedConstructPreparedAction::building()const noexcept{return impl_->before.building;}
bool SelectedConstructPreparedAction::native_committed()const noexcept{return impl_->committed;}
void SelectedConstructPreparedAction::require_player_native()const{
    auto& a=*impl_;a.check();const auto q=a.quote();need(q.player_native_eligible&&same_construct(a.before,q),"Construct actual native inputs changed before Player hook");a.check();
}
bool SelectedConstructPreparedAction::city_native_allowed()const{
    auto& a=*impl_;a.check();const auto q=a.quote();need(q.player_native_eligible&&same_construct(a.before,q),"Construct actual native inputs changed during Player hook");a.check();return q.city_native_eligible;
}
void SelectedConstructPreparedAction::require_current_native()const{
    require_player_native();need(city_native_allowed(),"Construct native city veto");
}
void SelectedConstructPreparedAction::apply_native_in_place(runtime::LiveSessionState& state){
    auto& a=*impl_;require_current_native();
    need(&state==a.state.get()&&state.ui_source_revision==a.revision&&a.revision<UINT64_MAX,"Construct SAME world/revision changed");
    need(runtime::resolve_bnw_current_city(state,a.before.owner,a.before.city)==a.raw,"Construct native city pointer changed");
    saves::SavedCityObservation* raw=nullptr;
    for(auto& c:state.world.all_players->players.at(static_cast<std::size_t>(a.before.owner)).cities.cities)if(&c==a.raw)raw=&c;
    need(raw&&raw->production.context.bnw_city_version==a.version,"Construct native writer lost actual city/version");
    auto& q=named(raw->production.blocks,"queue");auto& count=named(q.fields,"wire_count");
    auto& orders=*state.cities.players.at(a.before.owner).cities.at(a.before.city).production_orders;
    auto& player=state.world.all_players->players.at(static_cast<std::size_t>(a.before.owner));auto& values=named(player.core.fields,"m_paiBuildingClassMaking").values;
    need(values==a.before.making_values&&orders.empty()&&q.children.empty()&&count.value==0,"Construct final queue/making changed");
    static_assert(noexcept(values.swap(a.after_making)));static_assert(noexcept(q.children.swap(a.after_queue)));static_assert(noexcept(orders.swap(a.after_orders)));
    // Exact reached native store: making+1 (all active resource requirements
    // proved nonpositive), then insertBeginning. StartHeadOrder(CONSTRUCT) has
    // no process effect. Supplier publishes genuine Help dirty FIRST, then
    // ordered city dirty notifications; dirty failure is uncertain/once.
    values.swap(a.after_making);q.children.swap(a.after_queue);count.value=1;orders.swap(a.after_orders);
    ++state.ui_source_revision;a.committed=true;
}
SelectedConstructPreflight SelectedCityProductionServices::native_construct_preflight(std::int64_t owner,std::int64_t city,std::int64_t building)const{
    const auto p=impl_;Impl::Operation operation(*p);return inspect_construct(*p->parent,*p->initial,owner,city,building,[p]{p->entry();});
}
std::unique_ptr<SelectedConstructPreparedAction> SelectedCityProductionServices::prepare_construct(std::int64_t owner,std::int64_t city,std::int64_t building)const{
    const auto p=impl_;p->entry();auto a=std::make_unique<SelectedConstructPreparedAction::Impl>();a->state=p->initial;
    a->before=inspect_construct(*p->parent,*a->state,owner,city,building,[p]{p->entry();});
    need(a->before.player_native_eligible,"Construct native Player predicate rejected");
    a->current=[p]{p->entry();};a->quote=[p,owner,city,building]{return inspect_construct(*p->parent,*p->initial,owner,city,building,[p]{p->entry();});};
    a->raw=runtime::resolve_bnw_current_city(*a->state,a->before.owner,a->before.city);a->revision=a->state->ui_source_revision;a->version=*a->raw->production.context.bnw_city_version;
    a->after_making=a->before.making_values;a->after_making.at(static_cast<std::size_t>(a->before.building_class))=a->before.making_after;
    const content::InfoTypeEntry* info=nullptr;for(const auto& [type,e]:p->parent->types().entries())if(e.table=="Buildings"&&e.id==building){(void)type;need(!info,"Construct prepared BuildingInfo ambiguous");info=&e;}need(info,"Construct prepared BuildingInfo absent");
    const bool hashed=a->version>=5;saves::CityProductionRecord order{"order",0,0,0,{},{},{},{}};
    order.fields={{"eOrderType",1},{hashed?"iData1_hash":"iData1",hashed?static_cast<std::int64_t>(info->hash):building},{"iData2",-1},{"bSave",0},{"bRush",0}};
    a->after_queue.push_back(std::move(order));a->after_orders.push_back({1,building,-1,false,false,hashed?std::optional<std::uint32_t>{info->hash}:std::nullopt});
    a->check();return std::unique_ptr<SelectedConstructPreparedAction>(new SelectedConstructPreparedAction(std::move(a)));
}
runtime::SelectedProductionResult SelectedCityProductionServices::run_construct(std::int64_t owner,std::int64_t city,std::int64_t building,bool execute)const{
    const auto p=impl_;Impl::Operation operation(*p);using D=runtime::SelectedProductionDisposition;
    if(!p->stock)return {D::blocked,false,"Actual SAME construct stock owner missing"};
    std::unique_ptr<SelectedConstructPreparedAction> action;std::unique_ptr<runtime::BnwSelectedProductionActionLoan> loan;
    try{action=prepare_construct(owner,city,building);loan=runtime::borrow_selected_city_production(p->stock,p->parent,action->owner(),action->city());
        need(static_cast<bool>(loan),"Actual SAME construct loan missing");loan->require_same(*p->parent,*p->session,action->owner(),action->city());action->require_player_native();
    }catch(const std::exception& e){return {D::blocked,false,e.what()};}
    try{auto result=execute?loan->execute_construct(*action):loan->preview_construct(*action);const bool committed=action->native_committed();loan.reset();
        if(result.native_committed!=committed||(!execute&&committed)||(result.disposition==D::committed&&!committed)||(execute&&result.disposition==D::eligible)){
            p->invalidated.store(true);return {D::uncertain,committed,"Actual construct result disagrees with once native stores; quarantined"};}
        if(result.disposition==D::uncertain)p->invalidated.store(true);else{p->entry();if(!committed)action->require_player_native();}
        return result;
    }catch(const std::exception& e){p->invalidated.store(true);return {D::uncertain,action->native_committed(),std::string("Construct stock callback/publication failed; cannot retry: ")+e.what()};}
    catch(...){p->invalidated.store(true);return {D::uncertain,action->native_committed(),"Construct stock callback/publication failed; cannot retry"};}
}
runtime::SelectedProductionResult SelectedCityProductionServices::preview_construct_command(std::int64_t owner,std::int64_t city,std::int64_t building)const{return run_construct(owner,city,building,false);}
runtime::SelectedProductionResult SelectedCityProductionServices::execute_construct_command(std::int64_t owner,std::int64_t city,std::int64_t building)const{return run_construct(owner,city,building,true);}
namespace {
class SelectedProductionBackend final:public BnwCityProductionUiBackend {
 public:
    using Dispatch=std::function<UiCommandResult(const UiGameplayCommand&,const UiFrameSnapshot&,bool)>;
    SelectedProductionBackend(runtime::LiveSession& session,const runtime::ProductionProgressRules& rules,
        const content::RulesDatabase& db,content::InfoLocalizer localize,Dispatch dispatch)
        :BnwCityProductionUiBackend(session,rules,{}, {},&db,std::move(localize)),dispatch_(std::move(dispatch)){}
    UiCommandResult availability(const UiGameplayCommand& c,const UiFrameSnapshot& f)const override{return dispatch_(c,f,false);}
    UiCommandResult execute(const UiGameplayCommand& c,const UiFrameSnapshot& f)override{return dispatch_(c,f,true);}
 private:Dispatch dispatch_;
};
}
UiCommandResult SelectedCityProductionServices::dispatch_city_choice(
    UiScreenController& screens,const UiGameplayCommand& command,const UiFrameSnapshot& frame)const{
    const auto p=impl_;p->entry();
    if(command.source_id()!="ProductionPopup"||!command.as<CityChooseProduction>())
        return {UiCommandStatus::invalid,"Selected production dispatcher accepts only its typed city choice"};
    auto backend=make_backend(); // Keeps its genuine selected parent and stock source.
    // Controller applies actual registry visibility and modal focus first;
    // this backend routes directly to the SAME in-place production loan.
    return screens.dispatch(UiCommand{command},frame,backend.get());
}
std::shared_ptr<const runtime::BnwCityProductionTurnPending>
SelectedCityProductionServices::advance_process_component(std::int64_t owner,std::int64_t city,
    std::shared_ptr<const runtime::BnwCityProductionTurnPending> pending)const{
    const auto p=impl_;Impl::Operation operation(*p);
    need(static_cast<bool>(p->stock),"Process production needs the actual SAME stock owner before any native turn store");
    const auto actor=native(owner),handle=native(city);
    need(actor==p->parent->active_player_id(),"Process production requires the actual active selected actor");
    if(pending){const auto before=pending->progress();
        need(before.owner==actor&&before.city==handle,"Process production pending belongs to another city");}
    // No caller callbacks, detached copy, scalar-ready flag, or revision reset.
    auto actual=runtime::advance_selected_city_process_production(p->stock,p->parent,actor,handle,std::move(pending));
    p->entry();need(static_cast<bool>(actual),"Source process producer returned no actual registered cursor");
    actual->require_current();const auto result=actual->progress();
    need(result.owner==actor&&result.city==handle,"Source process producer returned a different actual city");
    return actual;
}
std::unique_ptr<BnwCityProductionUiBackend> SelectedCityProductionServices::make_backend(content::InfoLocalizer localize)const{
    const auto p=impl_;p->entry();
    auto services=std::shared_ptr<SelectedCityProductionServices>(new SelectedCityProductionServices(p));
    auto dispatch=[p,services](const UiGameplayCommand& command,const UiFrameSnapshot& frame,bool execute)->UiCommandResult{
        try{
            p->entry();const auto* request=command.as<CityChooseProduction>();
            if(!request||command.source_id()!="ProductionPopup"||request->city_id<0||request->item_id<0)
                return {UiCommandStatus::invalid,"Invalid selected production command"};
            if(request->order_type!=4&&request->order_type!=1&&request->order_type!=0)return {UiCommandStatus::unavailable,"Selected production supports ordinary TRAIN/MAINTAIN/CONSTRUCT only"};
            const auto* stamp=frame.find<BnwClientFrameStamp>();
            if(!stamp||stamp->identity.ui_source_revision!=p->initial->ui_source_revision||
               stamp->identity.player!=p->parent->active_player_id()||
               stamp->identity.turn!=p->initial->setup.game_turn||
               stamp->identity.map_guid!=p->initial->world.map_header.guid)
                return {UiCommandStatus::unavailable,"Selected production needs the current source-revision client publication"};
            const auto* city=selected_city(frame);
            if(!city||!frame.active_player_id()||!frame.turn()||city->id!=request->city_id||city->owner!=*frame.active_player_id()||
               *frame.active_player_id()!=p->parent->active_player_id()||*frame.turn()!=p->session->state().setup.game_turn||!city->queue_resolved||!city->queue.empty())
                return {UiCommandStatus::unavailable,"Actual selected production frame/empty queue is unavailable or stale"};
            const auto r=request->order_type==0?
                (execute?services->execute_train_command(city->owner,city->id,request->item_id):services->preview_train_command(city->owner,city->id,request->item_id)):
                request->order_type==1?
                (execute?services->execute_construct_command(city->owner,city->id,request->item_id):services->preview_construct_command(city->owner,city->id,request->item_id)):
                (execute?services->execute_maintain_command(city->owner,city->id,request->item_id):services->preview_maintain_command(city->owner,city->id,request->item_id));
            using D=runtime::SelectedProductionDisposition;
            if(r.disposition==D::uncertain)return {UiCommandStatus::uncertain,r.reason};
            if(r.native_committed)return {UiCommandStatus::performed,r.reason};
            if(!execute&&r.disposition==D::eligible)return {UiCommandStatus::ready,r.reason};
            return {UiCommandStatus::unavailable,r.reason};
        }catch(const std::exception& e){return {UiCommandStatus::unavailable,e.what()};}
    };
    return std::make_unique<SelectedProductionBackend>(*p->session,p->parent->production_rules(),p->parent->database(),std::move(localize),std::move(dispatch));
}
}

namespace civ5::ui {
namespace {
// Exact finite BNW-v5+ hashed unit-production read. Native duplicate mapped
// rows overwrite; unresolved/nonunit hashes refuse instead of defaulting.
std::map<std::int64_t,std::int64_t> train_progress(const saves::SavedCityObservation& c,
    const SelectedBnwPreviewHost& host){
    need(c.production.context.bnw_city_version&&*c.production.context.bnw_city_version>=5,
        "TRAIN requires actual BNW v5+ hashed production schema");
    std::map<std::int64_t,std::int64_t> values;
    const auto& block=named(named(c.production.blocks,"before_queue").children,"m_paiUnitProduction");
    for(const auto& r:block.children){const auto hash=named(r.fields,"hash").value;
        need(hash>=0&&hash<=UINT32_MAX,"TRAIN production hash outside native uint32");
        if(!hash)continue;
        const auto* info=host.types().find_hash(static_cast<std::uint32_t>(hash));
        const auto& amount=named(r.arrays,"values").values;
        need(info&&info->table=="Units"&&host.production_rules().units.contains(info->id)&&amount.size()==1,
            "TRAIN raw production hash/value unresolved");
        values[info->id]=native(amount.front());
    }return values;
}
std::int32_t train_city_scalar(const auto& c,std::string_view key){
    const auto& f=named(c.core.fields,key);
    need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.hashes.empty()&&f.row_counts.empty()&&f.row_keys.empty()&&f.bytes.empty(),
        "TRAIN city scalar native shape unavailable");return native(f.values.front());
}
bool same_train_record(const saves::CityProductionRecord& a,const saves::CityProductionRecord& b){
    if(std::string_view(a.name?a.name:"")!=std::string_view(b.name?b.name:"")||a.index!=b.index||a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||a.fields.size()!=b.fields.size()||a.arrays.size()!=b.arrays.size()||a.blobs.size()!=b.blobs.size()||a.children.size()!=b.children.size())return false;
    for(std::size_t i=0;i<a.fields.size();++i){const auto& x=a.fields[i];const auto& y=b.fields[i];if(std::string_view(x.name?x.name:"")!=std::string_view(y.name?y.name:"")||x.value!=y.value)return false;}
    // Only queue + unit-production rows are sealed; opaque blobs are refused.
    if(!a.blobs.empty())return false;
    for(std::size_t i=0;i<a.arrays.size();++i){const auto& x=a.arrays[i];const auto& y=b.arrays[i];if(std::string_view(x.name?x.name:"")!=std::string_view(y.name?y.name:"")||x.columns!=y.columns||x.values!=y.values)return false;}
    for(std::size_t i=0;i<a.children.size();++i)if(!same_train_record(a.children[i],b.children[i]))return false;
    return true;
}
}
struct SelectedCityTrainCreateSource::Impl {
    enum class Phase {prepared,help_pending,create_pending,id_assigned,city_trained_pending,city_trained_inflight,overflow_pending,reset_pending,gold_pending,queue_pop_pending,choose_notification_pending};
    std::shared_ptr<SelectedCityProductionServices::Impl> services;
    Operands args;Phase phase=Phase::prepared;
    std::uint64_t revision=0,occurrence=0;
    std::int32_t things=0,making_before=0,making_after=0,unit_class=-1,created_id=-1,lost_production=0,overflow=0,feature=0;
    std::vector<std::int64_t> making_values;
    std::vector<runtime::CityYieldState::ProductionOrder> orders;
    std::map<std::int64_t,std::int64_t> progress;
    saves::CityProductionRecord queue{},unit_progress{};
    std::optional<runtime::BnwCityFoodProductionOrder> food;
    std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> allocator;
    bool bound=false;
    void direct()const{
        const auto& s=*services->initial;
        need(services->thread==std::this_thread::get_id()&&!services->invalidated.load()&&s.ui_source_revision==revision,
            "TRAIN source creator/revision retired");
        const auto* raw=runtime::resolve_bnw_current_city(s,args.owner,args.city);need(raw,"TRAIN current source city absent");
        const auto& city=s.cities.players.at(args.owner).cities.at(args.city);
        need(city.id==args.city&&city.owner==args.owner&&city.x==args.x&&city.y==args.y&&train_city_scalar(*raw,"m_iX")==args.x&&train_city_scalar(*raw,"m_iY")==args.y,
            "TRAIN actual city coordinates/owner changed");
        need(city.production_orders&&*city.production_orders==orders&&city.production_progress&&city.production_progress->fully_resolved&&city.production_progress->source_profile==content::TechnologyProfile::bnw&&city.production_progress->units==progress,
            "TRAIN owned head/progress changed");
        need(same_train_record(named(raw->production.blocks,"queue"),queue)&&same_train_record(named(named(raw->production.blocks,"before_queue").children,"m_paiUnitProduction"),unit_progress),
            "TRAIN raw head/progress changed");
        need(train_city_scalar(*raw,"m_iThingsProduced")==things,"TRAIN actual thingsProduced prefix changed");
        need(city.production_progress->overflow_times100==overflow&&city.production_progress->feature_times100==feature&&
            train_city_scalar(*raw,"m_iOverflowProduction")==overflow&&train_city_scalar(*raw,"m_iFeatureProduction")==feature,"TRAIN raw/owned overflow/feature changed");
        const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(args.owner));
        const auto& making=named(player.core.fields,"m_paiUnitClassMaking");
        need(making.encoding==saves::PlayerCoreEncoding::vector_i32&&making.wire_count==making_values.size()&&making.hashes.empty()&&making.row_counts.empty()&&making.bytes.empty()&&making.values==making_values,"TRAIN actual making prefix changed");
    }
    void current()const{services->check();
        if(phase!=Phase::choose_notification_pending){need(food.has_value(),"TRAIN source head absent");food->require_same_inputs(*services->initial,services->parent->identity().rules_sha256);}
        direct();}
};
SelectedCityTrainCreateSource::SelectedCityTrainCreateSource(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
SelectedCityTrainCreateSource::~SelectedCityTrainCreateSource()=default;
std::shared_ptr<const SelectedCityTrainCreateSource> SelectedCityTrainCreateSource::retain_factory_owner()const{
    const auto p=impl_;need(p&&p->services->thread==std::this_thread::get_id(),"TRAIN canonical source foreign creator");
    auto canonical=factory_self_.lock();need(canonical&&canonical.get()==this,"TRAIN canonical source retired");return canonical;
}
SelectedCityTrainCreateSource::BorrowedPrefix SelectedCityTrainCreateSource::borrow_issuer_prefix()const{
    const auto self=retain_factory_owner();const auto& p=*self->impl_;p.current();
    need(p.phase==Impl::Phase::prepared&&!p.bound,"TRAIN source borrowed after prefix began");
    return {p.services->parent,p.services->session,p.args.owner,p.args.city,p.revision,p.occurrence};
}
void SelectedCityTrainCreateSource::require_current_native()const{const auto self=retain_factory_owner();self->impl_->current();}
void SelectedCityTrainCreateSource::require_for_allocator(const SelectedBnwPreviewHost& host,const runtime::LiveSession& session,std::int32_t actor)const{
    const auto self=retain_factory_owner();const auto& p=*self->impl_;p.current();
    need((p.phase==Impl::Phase::create_pending||p.phase==Impl::Phase::id_assigned)&&p.services->parent.get()==&host&&p.services->session==&session&&p.args.owner==actor,
        "TRAIN allocator lacks genuine SAME CreateUnit source phase");
}
SelectedCityTrainCreateSource::Operands SelectedCityTrainCreateSource::allocator_operands()const{
    const auto self=retain_factory_owner();self->impl_->current();need(self->impl_->phase==Impl::Phase::create_pending||self->impl_->phase==Impl::Phase::id_assigned,"TRAIN operands before actual CreateUnit");return self->impl_->args;
}
void SelectedCityTrainCreateSource::bind_allocator_once(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();need(!p.bound&&p.phase==Impl::Phase::create_pending,"TRAIN CreateUnit source already consumed/bound");
    const auto actual=core.retain_factory_owner();need(actual&&actual.get()==&core,"TRAIN supplied allocator canonical identity differs");
    p.allocator=actual;p.bound=true; // Weak: Core Impl holds the strong Source.
}
void SelectedCityTrainCreateSource::accept_actual_unit_id_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    // Core invokes inside its existing Guard AFTER actual ID/serial stores.
    // No rules/provider callbacks, allocation, or second Core Guard here.
    const auto p=impl_;need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::create_pending&&p->revision<UINT64_MAX,
        "TRAIN ID prefix source phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN ID prefix allocator identity differs");
    core.require_train_id_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual ID prefix revision differs");
    ++p->revision;p->phase=Impl::Phase::id_assigned;
}
void SelectedCityTrainCreateSource::accept_actual_initial_garrison_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    // Pure actual Core effect acknowledgment inside its existing guard. The
    // private Core proof owns the city/policy/currentness/revision evidence.
    const auto p=impl_;need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN garrison prefix source phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN garrison prefix allocator identity differs");
    core.require_initial_garrison_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual garrison prefix revision differs");
    ++p->revision;
}
void SelectedCityTrainCreateSource::accept_actual_initial_sight_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    // Pure Core-owned native sight closure proof. No caller map/count/ready or
    // callback, allocation or new guard is admitted after actual map stores.
    const auto p=impl_;need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN sight prefix source phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN sight prefix allocator identity differs");
    core.require_initial_sight_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual sight prefix revision differs");
    ++p->revision;
}
void SelectedCityTrainCreateSource::accept_actual_initial_plot_area_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    // Pure Core-owned native plot/area closure proof. No caller map/count/ready or
    // callback, allocation or new guard is admitted after actual plot/area stores.
    const auto p=impl_;need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN plot/area prefix source phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN plot/area prefix allocator identity differs");
    core.require_initial_plot_area_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual plot/area prefix revision differs");
    ++p->revision;
}
void SelectedCityTrainCreateSource::accept_actual_initial_city_strength_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN city strength prefix creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN city strength prefix allocator differs");
    core.require_initial_city_strength_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual city strength prefix revision differs");
    ++p->revision; // real Core cache/CityInfo/event/revision stores already done
}
void SelectedCityTrainCreateSource::accept_actual_initial_happiness_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN happiness prefix creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN happiness prefix allocator differs");
    core.require_initial_happiness_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual happiness prefix revision differs");
    ++p->revision; // genuine paired native happiness stores/revision already done
}
void SelectedCityTrainCreateSource::accept_actual_initial_happiness_suffix_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN happiness suffix creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN happiness suffix allocator differs");
    core.require_initial_happiness_suffix_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual happiness suffix revision differs");
    ++p->revision; // Authentic native five-stage stores already committed; not full happiness.
}
void SelectedCityTrainCreateSource::accept_actual_initial_happiness_return(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN happiness return creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN happiness return allocator differs");
    core.require_initial_happiness_return(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual happiness return revision differs");
    ++p->revision; // Actual excess/native caches and GameData11 flag already committed.
}
void SelectedCityTrainCreateSource::require_tail_source_owner(const runtime::BnwSelectedStockLuaSourceOwner& owner)const{
    const auto self=retain_factory_owner();self->impl_->current();
    need(self->impl_->services->stock.get()==&owner,"TRAIN tail issuer different actual stock source");
}
void SelectedCityTrainCreateSource::apply_pop_order_prefix(runtime::LiveSessionState& state)const{
    const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();
    need(&state==p.services->initial.get()&&p.phase==Impl::Phase::prepared&&!p.bound&&p.revision<UINT64_MAX,
        "TRAIN popOrder prefix wrong actual world/replay");
    const auto* observed=runtime::resolve_bnw_current_city(state,p.args.owner,p.args.city);saves::SavedCityObservation* raw=nullptr;
    for(auto& player:state.world.all_players->players)for(auto& city:player.cities.cities)if(&city==observed)raw=&city;
    need(raw,"TRAIN writable current city absent");
    auto& things=named(raw->core.fields,"m_iThingsProduced").values.front();
    auto& making=named(state.world.all_players->players.at(static_cast<std::size_t>(p.args.owner)).core.fields,"m_paiUnitClassMaking").values;
    const auto after=native(static_cast<std::int64_t>(p.things)+1);const auto slot=static_cast<std::size_t>(p.unit_class);
    p.services->check();p.direct(); // Last callback bookend then noalloc checks.
    need(state.city_dirty_next_occurrence==p.occurrence,"TRAIN Help queue occurrence changed before prefix");
    // CvCity::popOrder: thingsProduced++, then changeUnitClassMaking(-1).
    // No queue removal/production reset until CreateUnit and its real tail.
    things=after;making[slot]=p.making_after;p.making_values[slot]=p.making_after;
    p.things=after;state.ui_source_revision=++p.revision;p.phase=Impl::Phase::help_pending;
}
void SelectedCityTrainCreateSource::accept_actual_help_dirty(const runtime::BnwSelectedStockLuaSourceOwner& owner)const{
    const auto self=retain_factory_owner();auto& p=*self->impl_;
    need(p.phase==Impl::Phase::help_pending&&p.services->stock.get()==&owner,"TRAIN Help issuer/phase differs");
    // Supplier verifies actual Help flag queue + occurrence + exact revision
    // delta, not rendering/delivery. No successful implementation exists here.
    runtime::require_selected_train_help_dirty(owner,*p.services->parent,*p.services->initial,p.args.owner,p.args.city,p.occurrence,p.revision);
    p.services->check();need(p.revision<UINT64_MAX&&p.services->initial->ui_source_revision==p.revision+1,"TRAIN actual Help mutation revision differs");
    ++p.revision;p.current();p.phase=Impl::Phase::create_pending;
}
std::shared_ptr<const SelectedCityTrainCreateSource> SelectedCityProductionServices::advance_completed_train_component(std::int64_t owner,std::int64_t handle)const{
    const auto services=impl_;Impl::Operation operation(*services);const auto& s=*services->initial;
    const auto actor=native(owner),city_id=native(handle);
    const auto before_revision=s.ui_source_revision;
    need(!services->train_prefixes.contains({actor,city_id}),"TRAIN popOrder prefix already attempted; genuine tail reconciliation required");
    need(owner==services->parent->active_player_id()&&s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.pregame&&s.world.all_players&&s.setup.players.at(owner).alive,
        "TRAIN actual active living BNW owner absent");
    const auto* raw=runtime::resolve_bnw_current_city(s,actor,city_id);need(raw,"TRAIN actual city absent");
    const auto& city=s.cities.players.at(owner).cities.at(handle);
    need(city.production_orders&&!city.production_orders->empty()&&city.production_orders->front().type==0&&!city.production_orders->front().save,
        "TRAIN requires actual ordinary nonrepeat completed queue head");
    need(runtime::detail::bnw_player_human_read(s,actor,services->parent->database()),"TRAIN AI completion cost requires actual handicap/teammate owner");
    need(runtime::detail::bnw_game_active_player_read(s)==actor,"TRAIN Help dirty actual active player differs");
    const auto& minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");
    need(actor>=0&&actor<63&&minor.size()==64&&!minor.at(static_cast<std::size_t>(actor))&&!s.cities.players.at(owner).minor,
        "TRAIN first completion slice requires genuine major Human");
    const auto host=services->parent;const auto& db=host->database();const auto& types=host->types();
    auto p=std::make_shared<SelectedCityTrainCreateSource::Impl>();p->services=services;
    p->args.owner=actor;p->args.city=city_id;p->args.unit=native(city.production_orders->front().item);p->args.x=native(city.x);p->args.y=native(city.y);
    need(p->args.x>=0&&p->args.y>=0,"TRAIN actual city coordinates invalid");
    p->food=runtime::inspect_bnw_city_food_production_order(s,runtime::BnwCityFoodProductionRules::load(db,host->identity().rules_sha256),host->identity().rules_sha256,owner,handle);
    (void)p->food->food_production(); // Genuine bWasFoodProduction, used later.
    std::string unit_type,class_type,default_ai,domain;std::int32_t cost=0,work=0;std::size_t count=0;
    auto report=db.query_rows({"SELECT Type, Class, Cost, DefaultUnitAI, Domain, WorkRate FROM Units WHERE ID=?",{static_cast<std::int64_t>(p->args.unit)}},[&](const auto& r){++count;unit_type=r.text("Type").value_or("");class_type=r.text("Class").value_or("");default_ai=r.text("DefaultUnitAI").value_or("");domain=r.text("Domain").value_or("");cost=native(r.integer("Cost"));work=native(r.integer("WorkRate"));},2);
    need(report.all_succeeded()&&count==1&&types.find(unit_type)&&types.find(unit_type)->table=="Units"&&types.find(unit_type)->id==p->args.unit&&host->production_rules().units.contains(p->args.unit),"TRAIN genuine UnitInfo absent");
    const auto* cls=types.find(class_type);need(cls&&cls->table=="UnitClasses"&&cls->id>=0,"TRAIN genuine UnitClassInfo absent");p->unit_class=native(cls->id);
    const auto* ai=types.find(default_ai);p->args.unit_ai=native(city.production_orders->front().secondary);
    if(p->args.unit_ai==-1){need(ai&&ai->table=="UnitAIInfos"&&ai->id>=0,"TRAIN actual DefaultUnitAI absent");p->args.unit_ai=native(ai->id);}
    bool valid_ai=false;for(const auto& [key,e]:types.entries()){(void)key;if(e.table=="UnitAIInfos"&&e.id==p->args.unit_ai)valid_ai=true;}
    need(valid_ai,"TRAIN actual queue secondary AI absent");
    const auto* native_domain=types.find(domain);
    need(native_domain&&native_domain->table=="Domains"&&
        ((domain=="DOMAIN_SEA"&&native_domain->id==0)||(domain=="DOMAIN_AIR"&&native_domain->id==1)||(domain=="DOMAIN_LAND"&&native_domain->id==2)||(domain=="DOMAIN_IMMOBILE"&&native_domain->id==3)),"TRAIN native DomainTypes mapping differs");
    std::uint64_t slots=0;std::string default_unit;bool class_seen=false;
    const auto class_report=db.prefetch_info("UnitClasses",[&](auto slot,auto id,const auto& r){need(id>=0&&slot==static_cast<std::uint64_t>(id),"TRAIN native UnitClass slot differs");if(id==p->unit_class){class_seen=true;need(r.integer("InstanceCostModifier")==0,"TRAIN nonzero instance cost modifier requires full class-count cost slice");default_unit=r.text("DefaultUnit").value_or("");}return true;});
    need(class_report.all_succeeded()&&!class_report.overlapping_ids&&class_report.rows+class_report.holes==class_report.slots&&class_report.slots<=INT32_MAX&&class_seen,"TRAIN actual UnitClass span unavailable");slots=class_report.slots;
    const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(actor));need(player.saved_player_id==actor&&player.core.layout==bnw,"TRAIN actual player codec differs");
    for(const auto key:{"m_paiUnitClassCount","m_paiUnitClassMaking"}){const auto& f=named(player.core.fields,key);need(f.encoding==saves::PlayerCoreEncoding::vector_i32&&f.wire_count==slots&&f.values.size()==slots&&f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),"TRAIN native class vectors differ");for(auto v:f.values)need(native(v)>=0,"TRAIN class count/making negative");}
    p->making_values=named(player.core.fields,"m_paiUnitClassMaking").values;p->making_before=native(p->making_values.at(static_cast<std::size_t>(p->unit_class)));p->making_after=native(static_cast<std::int64_t>(p->making_before)-1);need(p->making_after>=0,"TRAIN actual making decrement would violate SDK assert");
    const auto& extra=named(player.tail.fields,"m_aUnitExtraCosts");need(extra.encoding&&std::string_view(extra.encoding)=="stl_pairs"&&extra.wire_count==0&&extra.values.empty()&&extra.bytes.empty()&&extra.children.empty(),"TRAIN nonempty extra-cost list needs ordered native first-pair cost slice");
    // Native checked i32 multiplication BEFORE each integer division.
    cost=native(static_cast<std::int64_t>(cost)*100)/100;
    cost=native(static_cast<std::int64_t>(cost)*source_define(db,"UNIT_PRODUCTION_PERCENT"))/100;
    const auto speed=s.world.pregame->archive.integers.at("game_speed"),era=s.world.pregame->archive.integers.at("era");
    need(speed==s.setup.speed&&era==s.setup.start_era,"TRAIN raw/owned speed/start era differs");
    for(const auto& item:{std::pair{"GameSpeeds",speed},std::pair{"Eras",era}}){std::size_t rows=0;std::int32_t percent=0;
        auto result=db.query_rows({std::string("SELECT TrainPercent FROM ")+item.first+" WHERE ID=?",{item.second}},[&](const auto& r){++rows;percent=native(r.integer("TrainPercent"));},2);
        need(result.all_succeeded()&&rows==1,"TRAIN actual speed/era TrainPercent unavailable");cost=native(static_cast<std::int64_t>(cost)*percent)/100;}
    cost=std::max(std::int32_t{1},cost);
    p->progress=train_progress(*raw,*host);need(city.production_progress&&city.production_progress->fully_resolved&&city.production_progress->units==p->progress,"TRAIN raw/owned production accumulation differs");
    const auto production=runtime::current_production_progress_times100(city,host->production_rules());
    need(native(production)/100>=cost,"TRAIN actual accumulated production has not completed native cost");
    // Exact no-builder/no-resource making branch for actual civilization Unit.
    const auto& civs=s.world.pregame->slots.integer_arrays.at("civilizations");const auto& keys=s.world.pregame->slots.string_arrays.at("civilization_keys");
    need(civs.size()==64&&keys.size()==64&&civs.at(static_cast<std::size_t>(actor))==s.cities.players.at(owner).civilization,"TRAIN raw/owned civilization differs");
    const auto civkey=keys.at(static_cast<std::size_t>(actor)).substr(0,keys.at(static_cast<std::size_t>(actor)).find('\0'));
    const auto* civ=types.find(civkey);need(civ&&civ->table=="Civilizations"&&civ->id==civs.at(static_cast<std::size_t>(actor)),"TRAIN actual civilization Type differs");
    std::size_t overrides=0;report=db.query_rows({"SELECT UnitType FROM Civilization_UnitClassOverrides WHERE CivilizationType=? AND UnitClassType=?",{civkey,class_type}},[&](const auto&){++overrides;});
    need(report.all_succeeded()&&overrides==0&&default_unit==unit_type,"TRAIN mapped civilization override making effects require actual mapping slice");
    need(!(work>0&&domain=="DOMAIN_LAND"),"TRAIN making decrement requires actual builder-count effect");
    bool resources=false;report=db.query_rows({"SELECT ResourceType, Cost FROM Unit_ResourceQuantityRequirements WHERE UnitType=?",{unit_type}},[&](const auto& r){const auto key=r.text("ResourceType");const auto* e=key?types.find(*key):nullptr;need(e&&e->table=="Resources","TRAIN resource requirement native mapping unresolved");if(native(r.integer("Cost"))>0)resources=true;});
    need(report.all_succeeded()&&!resources,"TRAIN making decrement requires actual resource-used effect");
    p->orders=*city.production_orders;p->queue=named(raw->production.blocks,"queue");p->unit_progress=named(named(raw->production.blocks,"before_queue").children,"m_paiUnitProduction");p->things=train_city_scalar(*raw,"m_iThingsProduced");p->overflow=train_city_scalar(*raw,"m_iOverflowProduction");p->feature=train_city_scalar(*raw,"m_iFeatureProduction");
    (void)native(static_cast<std::int64_t>(p->things)+1);p->revision=before_revision;p->occurrence=s.city_dirty_next_occurrence;
    need(p->revision<UINT64_MAX-1&&p->occurrence<UINT64_MAX,"TRAIN prefix/Help revision occurrence exhausted");services->check();p->current();
    need(static_cast<bool>(services->stock),"TRAIN genuine popOrder requires SAME stock/Help source owner");
    auto source=std::shared_ptr<SelectedCityTrainCreateSource>(new SelectedCityTrainCreateSource(std::move(p)));source->factory_self_=source;
    need(services->train_prefixes.emplace(std::pair{actor,city_id},source).second,"TRAIN prefix duplicate source history");
    // Supplier executes its actual registered VM operation, private SAME-state
    // stores, actual Help queue and revision adoption. No absent success body.
    try{runtime::execute_selected_completed_train_prefix(services->stock,source);}
    catch(...){services->invalidated.store(true);throw;} // Actual prefix may be committed; never retry/rollback it.
    source->require_for_allocator(*host,*services->session,actor);return source;
}
}

namespace civ5::ui {
void SelectedMaintainPreparedAction::require_for_source_host(const SelectedBnwPreviewHost& host,const runtime::LiveSession& session)const{
    require_current_native();const auto& a=*impl_;
    need(&host.session()==&session&&session.read_state_lease().get()==a.state.get()&&host.active_player_id()==a.before.owner&&runtime::resolve_bnw_current_city(session.state(),a.before.owner,a.before.city)==a.raw,
        "Maintain native action belongs to a different actual Host/world/city");
}
void SelectedConstructPreparedAction::require_for_source_host(const SelectedBnwPreviewHost& host,const runtime::LiveSession& session)const{
    require_player_native();const auto& a=*impl_;
    need(&host.session()==&session&&session.read_state_lease().get()==a.state.get()&&host.active_player_id()==a.before.owner&&runtime::resolve_bnw_current_city(session.state(),a.before.owner,a.before.city)==a.raw,
        "Construct native action belongs to a different actual Host/world/city");
}
}

namespace civ5::ui {
namespace {
SelectedTrainPreflight inspect_train(const SelectedBnwPreviewHost& host,const runtime::LiveSessionState& s,
    std::int64_t owner,std::int64_t handle,std::int64_t unit,const std::function<void()>& current){
    current();SelectedTrainPreflight out;out.owner=native(owner);out.city=native(handle);out.unit=native(unit);
    need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.pregame&&s.world.teams&&s.world.all_players,
        "Train requires genuine SAME BNW world");
    need(owner>=0&&owner<63&&owner==host.active_player_id()&&runtime::detail::bnw_game_active_player_read(s)==owner&&s.setup.players.at(owner).alive,
        "Train requires actual active living major owner");
    const auto* raw=runtime::resolve_bnw_current_city(s,out.owner,out.city);need(raw,"Train actual city absent");
    const auto& city=s.cities.players.at(owner).cities.at(handle);
    need(city.production_orders&&city.production_orders->empty()&&city.production_progress&&city.production_progress->fully_resolved&&raw->production.layout==bnw&&raw->production.context.bnw_city_version,
        "Train existing or unresolved queue requires real popOrder effects");
    const auto& queue=named(raw->production.blocks,"queue");
    need(queue.fields.size()==1&&named(queue.fields,"wire_count").value==0&&queue.children.empty()&&queue.arrays.empty()&&queue.blobs.empty(),"Train raw/owned empty queue differs");
    const auto& db=host.database();const auto& types=host.types();const content::InfoTypeEntry* info=nullptr;
    for(const auto& [key,e]:types.entries())if(e.table=="Units"&&e.id==unit){(void)key;need(!info,"Train ambiguous UnitInfo");info=&e;}
    const auto* hash=info?types.find_hash(info->hash):nullptr;
    need(info&&hash&&hash->table=="Units"&&hash->type==info->type&&hash->id==unit&&host.production_rules().units.contains(unit),"Train actual UnitInfo slot/hash unavailable");
    const auto ref=[&](const std::optional<std::string>& key,std::string_view table){
        if(!key||key->empty())return std::int32_t{-1};const auto* e=types.find(*key);
        need(e&&e->table==table&&e->id>=0,"Train native referenced Info unavailable");return native(e->id);
    };
    std::size_t rows=0;std::string class_type;std::int32_t cost=0,prerequisite=-1,obsolete=-1,min_area=0;
    bool purchase_only=false;
    auto report=db.query_rows({"SELECT * FROM Units WHERE ID=?",{unit}},[&](const auto& r){
        ++rows;need(r.integer("ID")==unit&&r.text("Type")==std::optional{info->type},"Train actual UnitInfo source differs");
        class_type=r.text("Class").value_or("");out.unit_class=ref(r.text("Class"),"UnitClasses");cost=native(r.integer("Cost"));
        prerequisite=ref(r.text("PrereqTech"),"Technologies");obsolete=ref(r.text("ObsoleteTech"),"Technologies");out.unit_ai=ref(r.text("DefaultUnitAI"),"UnitAIInfos");
        need(r.text("Domain")==std::optional<std::string>{"DOMAIN_LAND"}&&types.find("DOMAIN_LAND")&&types.find("DOMAIN_LAND")->table=="Domains"&&types.find("DOMAIN_LAND")->id==2,
            "Train finite native slice requires actual LAND domain");
        // Reached complex owners are strict barriers. This is property-based,
        // not a UNIT_SCOUT name/ID grant; Scout is resolved by the scenario.
        for(const auto key:{"Found","FoundAbroad","FoundReligion","SpreadReligion","RemoveHeresy","Trade","PrereqResources"})
            need(!r.boolean(key),"Train reached founding/religion/trade/area-resource owner unavailable");
        need(r.integer("WorkRate")<=0,"Train builder making/cache effect unavailable");
        for(const auto key:{"PolicyType","SpaceshipProject","ProjectPrereq","Special"})
            need(!r.text(key)||r.text(key)->empty(),"Train reached policy/project/special-unit owner unavailable");
        need(r.integer("NukeDamageLevel")==-1,"Train reached nuclear league owner unavailable");
        purchase_only=r.boolean("PurchaseOnly");min_area=native(r.integer("MinAreaSize"));
    },2);need(report.all_succeeded()&&rows==1&&out.unit_class>=0&&out.unit_ai>=0,"Train native UnitInfo/Class/DefaultAI unavailable");
    std::uint64_t class_slots=0;bool class_seen=false;std::string default_unit;
    const auto classes=db.prefetch_info("UnitClasses",[&](auto slot,auto authored,const auto& r){
        need(authored>=0&&slot==static_cast<std::uint64_t>(authored),"Train native UnitClass slot differs");
        if(authored==out.unit_class){class_seen=true;need(r.text("Type")==std::optional{class_type},"Train actual class Type differs");default_unit=r.text("DefaultUnit").value_or("");
            for(const auto key:{"MaxGlobalInstances","MaxTeamInstances","MaxPlayerInstances"})need(r.integer(key)==-1,"Train finite class limit/count owner unavailable");}
        return true;
    });need(classes.all_succeeded()&&!classes.overlapping_ids&&classes.rows+classes.holes==classes.slots&&classes.slots<=INT32_MAX&&class_seen,
        "Train genuine sparse UnitClass span unavailable");class_slots=classes.slots;
    need(s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,"Train actual player registry unavailable");
    const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(owner));need(player.saved_player_id==owner&&player.core.layout==bnw,"Train actual player codec differs");
    // Source CvPlayerTraits::NoTrain is the current serialized cache, not a
    // SQL trait guess or a fabricated false cache. Retain every native value.
    const auto& no_train=named(player.traits.fields,"m_abNoTrain");
    need(player.traits.layout==bnw&&player.traits.version==19&&no_train.encoding==saves::PlayerTraitsEncoding::counted_bool&&no_train.wire_count&&*no_train.wire_count>=0&&
        static_cast<std::uint64_t>(*no_train.wire_count)==class_slots&&no_train.values.size()==class_slots&&no_train.rows.empty()&&no_train.hashes.empty(),"Train actual trait NoTrain cache shape unavailable");
    for(const auto v:no_train.values)need(v==0||v==1,"Train malformed trait NoTrain bool");out.no_train_values=no_train.values;
    if(no_train.values.at(static_cast<std::size_t>(out.unit_class))!=0){out.blocker="Train actual trait NoTrain veto";return out;}
    const auto& civs=s.world.pregame->slots.integer_arrays.at("civilizations");const auto& keys=s.world.pregame->slots.string_arrays.at("civilization_keys");
    need(civs.size()==64&&keys.size()==64&&civs.at(static_cast<std::size_t>(owner))==s.cities.players.at(owner).civilization,"Train raw/owned civilization differs");
    const auto civkey=keys.at(static_cast<std::size_t>(owner)).substr(0,keys.at(static_cast<std::size_t>(owner)).find('\0'));
    const auto* civ=types.find(civkey);need(civ&&civ->table=="Civilizations"&&civ->id==civs.at(static_cast<std::size_t>(owner)),"Train actual civilization Type unavailable");
    std::optional<std::string> mapped=default_unit;std::size_t overrides=0;
    report=db.query_rows({"SELECT UnitType FROM Civilization_UnitClassOverrides WHERE CivilizationType=? AND UnitClassType=?",{civkey,class_type}},[&](const auto& r){++overrides;mapped=r.text("UnitType");});
    need(report.all_succeeded()&&overrides<=1,"Train ambiguous civilization unit override");
    if(!mapped||*mapped!=info->type){out.blocker="Train civilization does not map to this actual UnitInfo";return out;}
    if(cost==-1){out.blocker="Train actual native production Cost -1";return out;}
    const auto& teams=s.world.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"Train actual team roster unavailable");
    const auto team=native(teams.at(static_cast<std::size_t>(owner)));need(team>=0&&team<64&&team==s.setup.players.at(owner).team&&s.world.teams->teams.size()==64,"Train raw/owned team differs");
    const auto& raw_team=s.world.teams->teams.at(static_cast<std::size_t>(team));need(raw_team.index==static_cast<std::uint64_t>(team)&&named(raw_team.fields,"id").value==team,"Train actual Team registry differs");
    const auto restored=runtime::restore_team_technologies(raw_team,host.technologies(),types);const runtime::TeamTechnologies* owned=nullptr;
    for(const auto& t:s.technologies)if(t.team()==team){need(!owned,"Train duplicate owned team");owned=&t;}
    need(owned&&owned->profile()==content::TechnologyProfile::bnw&&owned->slots()==restored.slots()&&owned->last_tech()==restored.last_tech()&&owned->nondefault_values()==restored.nondefault_values()&&owned->profile_extensions().values.empty()&&restored.remap().all_mapped(),
        "Train actual raw/owned TeamTechs differs");
    out.technologies.push_back(prerequisite);
    if(!owned->has_tech(prerequisite)){out.blocker="Train prerequisite technology is not acquired";return out;}
    const auto and_slots=source_define(db,"NUM_UNIT_AND_TECH_PREREQS");need(and_slots>=0,"Train native AND-tech loop bound invalid");
    report=db.query_rows({"SELECT Technologies.ID FROM Unit_TechTypes INNER JOIN Technologies ON TechType=Technologies.Type WHERE UnitType=?",{info->type}},[&](const auto& r){out.technologies.push_back(native(r.integer_at(0)));});
    need(report.all_succeeded()&&out.technologies.size()-1<=static_cast<std::uint64_t>(and_slots),"Train actual AND-tech cache exceeds native capacity");
    for(const auto tech:out.technologies)if(!owned->has_tech(tech)){out.blocker="Train additional prerequisite technology is not acquired";return out;}
    out.technologies.push_back(obsolete);
    if(obsolete!=-1&&owned->has_tech(obsolete)){out.blocker="Train obsolete technology veto";return out;}
    // Every active joined resource slot is proved nonpositive, so canTrain
    // resource checks and changeUnitClassMaking resources have no store.
    bool positive_resource=false;
    report=db.query_rows({"SELECT Resources.ID, Cost FROM Unit_ResourceQuantityRequirements INNER JOIN Resources ON ResourceType=Resources.Type WHERE UnitType=?",{info->type}},[&](const auto& r){
        need(r.integer_at(0)>=0,"Train resource slot invalid");if(native(r.integer("Cost"))>0)positive_resource=true;
    });need(report.all_succeeded()&&!positive_resource,"Train reached actual resource availability/making owner unavailable");
    const auto& m=named(player.core.fields,"m_paiUnitClassMaking");
    need(m.encoding==saves::PlayerCoreEncoding::vector_i32&&m.wire_count==class_slots&&m.values.size()==class_slots&&m.hashes.empty()&&m.bytes.empty()&&m.row_counts.empty(),"Train actual making vector unavailable");
    out.making_values=m.values;out.making_before=native(m.values.at(static_cast<std::size_t>(out.unit_class)));need(out.making_before>=0,"Train negative making count");out.making_after=native(static_cast<std::int64_t>(out.making_before)+1);
    // Native source evaluates getUnitClassMaking argument even for an unlimited
    // class. Preserve ascending alive-player TEAM sum and checked i32 addition.
    for(std::size_t i=0;i<64;++i){const auto& p=s.world.all_players->players.at(i);need(p.saved_player_id==static_cast<std::int64_t>(i),"Train actual ascending player registry differs");
        const auto alive=raw_player_scalar(p,"m_bAlive");need((alive==0||alive==1)&&s.setup.players.at(static_cast<std::int64_t>(i)).alive==(alive!=0),"Train raw/owned alive differs");
        out.player_seal.push_back(alive);if(!alive)continue;
        const auto t=native(teams[i]);need(t==s.setup.players.at(static_cast<std::int64_t>(i)).team,"Train raw/owned live team differs");out.player_seal.push_back(t);
        if(t!=team)continue;const auto& v=named(p.core.fields,"m_paiUnitClassMaking");need(v.encoding==saves::PlayerCoreEncoding::vector_i32&&v.wire_count==class_slots&&v.values.size()==class_slots,"Train teammate making vector unavailable");
        const auto n=native(v.values.at(static_cast<std::size_t>(out.unit_class)));need(n>=0,"Train negative teammate making count");out.team_making=native(static_cast<std::int64_t>(out.team_making)+n);out.player_seal.push_back(n);
    }
    out.player_native_eligible=true;
    if(purchase_only){out.blocker="Train purchase-only city veto";current();return out;}
    bool requires_building=false;
    report=db.query_rows({"SELECT BuildingClasses.ID FROM Unit_BuildingClassRequireds INNER JOIN BuildingClasses ON BuildingClassType=BuildingClasses.Type WHERE UnitType=?",{info->type}},[&](const auto&){requires_building=true;});
    need(report.all_succeeded()&&!requires_building,"Train reached city building-class requirement unavailable");
    // Native LAND plot.canTrain: real city reference, actual area getter,
    // current saved tiles count. Never infer a dry/coastal/large enough plot.
    need(s.world.map_plots&&s.world.map_projection&&s.world.map_regions&&s.world.map_regions->layout==bnw,"Train actual map/area owners unavailable");
    const auto x=native(named(raw->core.fields,"m_iX").values.at(0)),y=native(named(raw->core.fields,"m_iY").values.at(0));
    need(x>=0&&y>=0&&city.x==x&&city.y==y,"Train raw/owned city coordinates differ");
    const auto index=s.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
    const auto& plot=s.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& cell=s.world.map_projection->cells.at(static_cast<std::size_t>(index));
    need(plot.index==index&&cell.index==index&&cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==handle&&named(plot.fields,"plot_city_owner").value==owner&&named(plot.fields,"plot_city_id").value==handle,
        "Train real plot city reference differs");
    const auto area=native(named(plot.fields,"area").value);need(area==city.area,"Train raw/owned city area differs");
    const saves::MapRegionRecord* region=nullptr;
    for(const auto& container:s.world.map_regions->containers)if(container.name&&std::string_view(container.name)=="areas")for(const auto& r:container.records)
        if(named(r.fields,"id").value==area){need(!region,"Train duplicate native area ID");region=&r;}
    need(region,"Train actual city area missing");const auto tiles=native(named(region->fields,"tiles").value);need(tiles>=0,"Train negative native area size");
    out.city_seal={x,y,area,tiles,min_area};
    if(tiles<min_area){out.blocker="Train real city area below native MinAreaSize";current();return out;}
    out.city_native_eligible=true;current();return out;
}
bool same_train(const SelectedTrainPreflight& a,const SelectedTrainPreflight& b){
    return a.owner==b.owner&&a.city==b.city&&a.unit==b.unit&&a.unit_class==b.unit_class&&a.unit_ai==b.unit_ai&&a.making_before==b.making_before&&a.making_after==b.making_after&&a.team_making==b.team_making&&
        a.making_values==b.making_values&&a.no_train_values==b.no_train_values&&a.technologies==b.technologies&&a.player_seal==b.player_seal&&a.city_seal==b.city_seal&&a.player_native_eligible==b.player_native_eligible&&a.city_native_eligible==b.city_native_eligible;
}
}
struct SelectedTrainPreparedAction::Impl {
    const std::thread::id thread=std::this_thread::get_id();std::function<void()> current;std::function<SelectedTrainPreflight()> quote;
    std::shared_ptr<const runtime::LiveSessionState> state;std::shared_ptr<const SelectedBnwPreviewHost> host;
    SelectedTrainPreflight before;const saves::SavedCityObservation* raw=nullptr;std::uint64_t revision=0,version=0;bool committed=false;
    std::vector<std::int64_t> after_making;std::vector<saves::CityProductionRecord> after_queue;std::vector<runtime::CityYieldState::ProductionOrder> after_orders;
    void check()const{need(thread==std::this_thread::get_id(),"Train action foreign creator thread");current();need(!committed,"Train action already consumed");}
};
SelectedTrainPreparedAction::SelectedTrainPreparedAction(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
SelectedTrainPreparedAction::~SelectedTrainPreparedAction()=default;
std::int32_t SelectedTrainPreparedAction::owner()const noexcept{return impl_->before.owner;}
std::int32_t SelectedTrainPreparedAction::city()const noexcept{return impl_->before.city;}
std::int32_t SelectedTrainPreparedAction::unit()const noexcept{return impl_->before.unit;}
bool SelectedTrainPreparedAction::native_committed()const noexcept{return impl_->committed;}
void SelectedTrainPreparedAction::require_for_source_host(const SelectedBnwPreviewHost& host,const runtime::LiveSession& session)const{
    auto& a=*impl_;a.check();need(a.host.get()==&host&&&host.session()==&session&&session.read_state_lease().get()==a.state.get(),"Train loan different genuine Host/session");require_player_native();
}
void SelectedTrainPreparedAction::require_player_native()const{auto& a=*impl_;a.check();const auto now=a.quote();need(now.player_native_eligible&&same_train(a.before,now),"Train actual inputs changed before Player hook");a.check();}
bool SelectedTrainPreparedAction::city_native_allowed()const{auto& a=*impl_;a.check();const auto now=a.quote();need(now.player_native_eligible&&same_train(a.before,now),"Train actual inputs changed during Player hook");a.check();return now.city_native_eligible;}
void SelectedTrainPreparedAction::require_current_native()const{require_player_native();need(city_native_allowed(),"Train native City predicate veto");}
void SelectedTrainPreparedAction::apply_native_in_place(runtime::LiveSessionState& state){
    auto& a=*impl_;require_current_native();need(&state==a.state.get()&&state.ui_source_revision==a.revision&&a.revision<UINT64_MAX,"Train SAME world/revision changed");
    need(runtime::resolve_bnw_current_city(state,a.before.owner,a.before.city)==a.raw,"Train actual city pointer changed");
    saves::SavedCityObservation* raw=nullptr;for(auto& c:state.world.all_players->players.at(static_cast<std::size_t>(a.before.owner)).cities.cities)if(&c==a.raw)raw=&c;
    need(raw&&raw->production.context.bnw_city_version==a.version,"Train native writer lost actual city/version");
    auto& queue=named(raw->production.blocks,"queue");auto& count=named(queue.fields,"wire_count");
    auto& orders=*state.cities.players.at(a.before.owner).cities.at(a.before.city).production_orders;
    auto& player=state.world.all_players->players.at(static_cast<std::size_t>(a.before.owner));auto& making=named(player.core.fields,"m_paiUnitClassMaking").values;
    need(making==a.before.making_values&&named(player.traits.fields,"m_abNoTrain").values==a.before.no_train_values&&orders.empty()&&queue.children.empty()&&count.value==0,"Train final queue/traits/making changed");
    static_assert(noexcept(making.swap(a.after_making)));static_assert(noexcept(queue.children.swap(a.after_queue)));static_assert(noexcept(orders.swap(a.after_orders)));
    // Actual pushOrder: changeUnitClassMaking(+1), insertBeginning, then
    // startHeadOrder(TRAIN) has no process effect. No accumulated-production
    // change or unit creation here. Supplier owns ordered Help/city UI flags.
    making.swap(a.after_making);queue.children.swap(a.after_queue);count.value=1;orders.swap(a.after_orders);++state.ui_source_revision;a.committed=true;
}
SelectedTrainPreflight SelectedCityProductionServices::native_train_preflight(std::int64_t owner,std::int64_t city,std::int64_t unit)const{
    const auto p=impl_;Impl::Operation operation(*p);return inspect_train(*p->parent,*p->initial,owner,city,unit,[p]{p->entry();});
}
std::unique_ptr<SelectedTrainPreparedAction> SelectedCityProductionServices::prepare_train(std::int64_t owner,std::int64_t city,std::int64_t unit)const{
    const auto p=impl_;p->entry();auto a=std::make_unique<SelectedTrainPreparedAction::Impl>();a->state=p->initial;a->host=p->parent;
    a->before=inspect_train(*p->parent,*a->state,owner,city,unit,[p]{p->entry();});need(a->before.player_native_eligible,"Train native Player predicate rejected");
    a->current=[p]{p->entry();};a->quote=[p,owner,city,unit]{return inspect_train(*p->parent,*p->initial,owner,city,unit,[p]{p->entry();});};
    a->raw=runtime::resolve_bnw_current_city(*a->state,a->before.owner,a->before.city);a->revision=a->state->ui_source_revision;a->version=*a->raw->production.context.bnw_city_version;
    a->after_making=a->before.making_values;a->after_making.at(static_cast<std::size_t>(a->before.unit_class))=a->before.making_after;
    const content::InfoTypeEntry* info=nullptr;for(const auto& [key,e]:p->parent->types().entries())if(e.table=="Units"&&e.id==unit){(void)key;info=&e;}need(info,"Train prepared UnitInfo missing");
    const bool hashed=a->version>=5;saves::CityProductionRecord order{"order",0,0,0,{}, {}, {}, {}};
    order.fields={{"eOrderType",0},{hashed?"iData1_hash":"iData1",hashed?static_cast<std::int64_t>(info->hash):unit},{"iData2",a->before.unit_ai},{"bSave",0},{"bRush",0}};
    a->after_queue.push_back(std::move(order));a->after_orders.push_back({0,unit,a->before.unit_ai,false,false,hashed?std::optional<std::uint32_t>{info->hash}:std::nullopt});
    a->current();a->check();return std::unique_ptr<SelectedTrainPreparedAction>(new SelectedTrainPreparedAction(std::move(a)));
}
runtime::SelectedProductionResult SelectedCityProductionServices::run_train(std::int64_t owner,std::int64_t city,std::int64_t unit,bool execute)const{
    const auto p=impl_;Impl::Operation operation(*p);using D=runtime::SelectedProductionDisposition;
    if(!p->stock)return {D::blocked,false,"Actual SAME stock production owner is missing"};
    std::unique_ptr<SelectedTrainPreparedAction> action;std::unique_ptr<runtime::BnwSelectedProductionActionLoan> loan;
    try{action=prepare_train(owner,city,unit);loan=runtime::borrow_selected_city_production(p->stock,p->parent,action->owner(),action->city());need(bool(loan),"Actual Train stock loan unavailable");loan->require_same(*p->parent,*p->session,action->owner(),action->city());action->require_player_native();}
    catch(const std::exception& e){return {D::blocked,false,e.what()};}
    try{auto result=execute?loan->execute_train(*action):loan->preview_train(*action);const bool committed=action->native_committed();loan.reset();
        if(result.native_committed!=committed||(!execute&&committed)||(result.disposition==D::committed&&!committed)||(execute&&result.disposition==D::eligible)){p->invalidated.store(true);return {D::uncertain,committed,"Train actual stock result disagrees with once stores"};}
        if(result.disposition==D::uncertain)p->invalidated.store(true);else{p->entry();if(!committed)action->require_player_native();}return result;
    }catch(const std::exception& e){p->invalidated.store(true);return {D::uncertain,action->native_committed(),std::string("Train actual stock operation failed; cannot retry: ")+e.what()};}
    catch(...){p->invalidated.store(true);return {D::uncertain,action->native_committed(),"Train actual stock operation failed; cannot retry"};}
}
runtime::SelectedProductionResult SelectedCityProductionServices::preview_train_command(std::int64_t owner,std::int64_t city,std::int64_t unit)const{return run_train(owner,city,unit,false);}
runtime::SelectedProductionResult SelectedCityProductionServices::execute_train_command(std::int64_t owner,std::int64_t city,std::int64_t unit)const{return run_train(owner,city,unit,true);}
}

namespace civ5::ui {
std::shared_ptr<const runtime::BnwCityProductionTurnPending> SelectedCityProductionServices::advance_train_component(std::int64_t owner,std::int64_t city,std::shared_ptr<const runtime::BnwCityProductionTurnPending> pending)const{
 const auto p=impl_;Impl::Operation operation(*p);need(bool(p->stock),"Production TRAIN actual SAME stock owner absent");const auto actor=native(owner),handle=native(city);
 need(actor==p->parent->active_player_id(),"Production TRAIN actual selected actor differs");
 if(pending){pending->require_current();const auto q=pending->progress();need(q.owner==actor&&q.city==handle,"Production TRAIN genuine cursor different city");}
 auto actual=runtime::advance_selected_city_train_production(p->stock,p->parent,actor,handle,std::move(pending));p->entry();need(bool(actual),"Production TRAIN source returned no actual cursor");actual->require_current();return actual;
}
}

namespace civ5::ui {
UiCommandResult SelectedCityProductionServices::open_production_popup(
 UiScreenController& screens,const UiFrameSnapshot& frame,std::int64_t actual_city)const{
 const auto p=impl_;
 try{
  p->entry();const auto* stamp=frame.find<BnwClientFrameStamp>();const auto* city=selected_city(frame);
  if(!stamp||!frame.active_player_id()||!frame.turn()||
     stamp->identity.ui_source_revision!=p->initial->ui_source_revision||stamp->identity.player!=p->parent->active_player_id()||
     stamp->identity.turn!=p->initial->setup.game_turn||stamp->identity.map_guid!=p->initial->world.map_header.guid||
     !stamp->selection.city||*stamp->selection.city!=actual_city||!city||city->id!=actual_city||city->owner!=stamp->identity.player||
     *frame.active_player_id()!=stamp->identity.player||*frame.turn()!=stamp->identity.turn)
   return {UiCommandStatus::unavailable,"ProductionPopup needs the current issued selected-city frame"};
  const auto& state=*p->initial;const auto* raw=runtime::resolve_bnw_current_city(state,city->owner,city->id);
  if(!raw||!state.cities.players.contains(city->owner)||!state.cities.players.at(city->owner).cities.contains(city->id))
   return {UiCommandStatus::unavailable,"ProductionPopup actual owned city is unavailable"};
  const auto& actual=state.cities.players.at(city->owner).cities.at(city->id);
  if(!city->queue_resolved||!actual.production_orders||actual.production_orders->size()!=city->queue.size())
   return {UiCommandStatus::unavailable,"ProductionPopup raw/owned queue is unavailable or stale"};
  for(std::size_t i=0;i<city->queue.size();++i)if(city->queue[i].type!=actual.production_orders->at(i).type||city->queue[i].item!=actual.production_orders->at(i).item)
   return {UiCommandStatus::unavailable,"ProductionPopup current queue differs from issued frame"};
  const auto focus=screens.focused_screen();if(focus&&*focus!="CityView"&&*focus!="ProductionPopup")
   return {UiCommandStatus::unavailable,"Another UI context owns production navigation"};
  if(!screens.registry().find("ProductionPopup"))return {UiCommandStatus::unavailable,"Actual ProductionPopup context is not registered"};
  p->entry();return screens.dispatch(city_production_popup_command(),frame);
 }catch(const std::exception& e){return {UiCommandStatus::unavailable,e.what()};}
}
}

namespace civ5::ui {
SelectedCityTrainCreateSource::BorrowedTail SelectedCityTrainCreateSource::borrow_completed_create_unit_return()const{
 const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();
 need(p.bound&&p.phase!=Impl::Phase::prepared&&p.phase!=Impl::Phase::help_pending&&p.phase!=Impl::Phase::create_pending,"TRAIN tail before genuine CreateUnit source");
 if(p.phase==Impl::Phase::id_assigned){
  const auto core=p.allocator.lock();need(core!=nullptr,"TRAIN actual allocator facade retired");const auto actual=core->retain_factory_owner();
  need(actual.get()==core.get()&&!actual.owner_before(core)&&!core.owner_before(actual),"TRAIN allocator canonical control changed");
  // Full Init alone is insufficient. The Core private producer must additionally
  // prove actual validMove/XP/promotions/rally/operation CreateUnit return.
  // Current genuine Core17 refuses here; no numeric ID/boolean can open it.
  const auto id=actual->require_completed_train_create_unit_return(*self);p.current();need(id>=0,"TRAIN actual CreateUnit returned invalid index");
  p.created_id=id;p.phase=Impl::Phase::city_trained_pending;
 }
 need(p.phase!=Impl::Phase::city_trained_inflight&&p.created_id>=0,"TRAIN CityTrained inflight/unknown return cannot replay");
 return {p.services->parent,p.args.owner,p.args.city,p.created_id,p.revision};
}
bool SelectedCityTrainCreateSource::city_trained_hook_pending()const{const auto self=retain_factory_owner();self->impl_->current();return self->impl_->phase==Impl::Phase::city_trained_pending;}
void SelectedCityTrainCreateSource::begin_city_trained_hook()const{const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();need(p.phase==Impl::Phase::city_trained_pending,"TRAIN CityTrained cannot replay");p.phase=Impl::Phase::city_trained_inflight;}
void SelectedCityTrainCreateSource::accept_city_trained_hook()const{const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();need(p.phase==Impl::Phase::city_trained_inflight,"TRAIN actual CityTrained attempt absent");p.phase=Impl::Phase::overflow_pending;}
runtime::SelectedProductionResult SelectedCityTrainCreateSource::apply_completion_native_stage(
 runtime::LiveSessionState& state,bool selected,std::uint64_t generation)const{
 const auto self=retain_factory_owner();auto& p=*self->impl_;p.current();using D=runtime::SelectedProductionDisposition;
 need(&state==p.services->initial.get()&&generation==p.services->generation,"TRAIN tail different actual world/generation");
 const auto* observed=runtime::resolve_bnw_current_city(state,p.args.owner,p.args.city);saves::SavedCityObservation* raw=nullptr;
 for(auto& c:state.world.all_players->players.at(static_cast<std::size_t>(p.args.owner)).cities.cities)if(&c==observed)raw=&c;
 need(raw!=nullptr,"TRAIN tail actual mutable city absent");auto& live=*state.cities.players.at(p.args.owner).cities.at(p.args.city).production_progress;
 if(p.phase==Impl::Phase::overflow_pending){
  const auto values=runtime::BnwCityProductionTurnPending::read_train_completion_without_overflow(p.services->parent,p.args.owner,p.args.city);
  const auto overflow=native(std::int64_t{values.stored_times100}-values.needed_times100);const auto maximum=std::max(values.needed_times100,values.difference_without_overflow);
  const auto lost=std::max(std::int32_t{0},native(std::int64_t{overflow}-maximum));const auto kept=std::min(maximum,overflow);
  need(overflow>=0&&kept>=0,"TRAIN completion native overflow assertion");
  const auto after=native(std::int64_t{p.overflow}+kept);if(kept>0)need(kept<25000&&after>=0&&p.revision<UINT64_MAX,"TRAIN overflow native setter assertion/exhaustion");
  auto& field=named(raw->core.fields,"m_iOverflowProduction").values.front();p.current();p.direct();
  if(kept>0){field=after;live.overflow_times100=after;p.overflow=after;state.ui_source_revision=++p.revision;}
  p.lost_production=lost;p.phase=Impl::Phase::reset_pending;
  return {D::committed,kept>0,"Actual TRAIN overflow prefix; completion tail remains pending"};
 }
 if(p.phase==Impl::Phase::reset_pending){
  auto progress=p.progress;progress[p.args.unit]=0;auto expected_progress=progress;
  auto block=p.unit_progress;const auto* info=static_cast<const content::InfoTypeEntry*>(nullptr);
  for(const auto& [key,e]:p.services->parent->types().entries())if(e.table=="Units"&&e.id==p.args.unit){(void)key;need(!info,"TRAIN reset ambiguous actual UnitInfo");info=&e;}
  need(info!=nullptr,"TRAIN reset actual UnitInfo absent");saves::CityProductionRecord* last=nullptr;
  for(auto& row:block.children)if(named(row.fields,"hash").value==static_cast<std::int64_t>(info->hash))last=&row;
  need(last!=nullptr,"TRAIN completed unit production raw row absent");named(last->arrays,"values").values.front()=0;auto expected_block=block;
  auto flags=state.city_dirty_events;auto events=state.specific_city_info_events;const auto flags_before=flags;const auto events_before=events;
  const auto occurrence=state.city_dirty_next_occurrence;const auto count=selected?std::uint64_t{2}:std::uint64_t{1};
  need(p.revision<=UINT64_MAX-count-1&&occurrence<=UINT64_MAX-count,"TRAIN reset actual dirty/revision exhausted");auto next=occurrence;
  if(selected)flags.push_back({next++,generation,p.args.owner,p.args.city,-1,-1,runtime::BnwCityDirtyKind::city_screen,true});
  events.push_back({next++,generation,p.args.owner,p.args.city,runtime::BnwSpecificCityInfoKind::banner});
  auto& actual=named(named(raw->production.blocks,"before_queue").children,"m_paiUnitProduction");
  p.current();p.direct();need(state.city_dirty_events==flags_before&&state.specific_city_info_events==events_before&&state.city_dirty_next_occurrence==occurrence,"TRAIN reset late UI queue changed");
  static_assert(std::is_nothrow_swappable_v<saves::CityProductionRecord>);std::swap(actual,block);live.units.swap(progress);p.progress.swap(expected_progress);std::swap(p.unit_progress,expected_block);
  state.ui_source_revision=++p.revision;
  // Native changed setter queues conditional CityScreen, then unconditional BANNER.
  state.city_dirty_events.swap(flags);state.specific_city_info_events.swap(events);state.city_dirty_next_occurrence=next;
  p.revision+=count;state.ui_source_revision=p.revision;p.phase=Impl::Phase::gold_pending;
  return {D::committed,true,"Actual TRAIN production reset/BANNER prefix; gold/pop tail pending"};
 }
 if(p.phase==Impl::Phase::gold_pending){
  const auto percent=source_define(p.services->parent->database(),"MAXED_UNIT_GOLD_PERCENT");const auto gold=native(std::int64_t{p.lost_production}*percent)/100;p.current();
  if(gold>0)return {D::blocked,true,"TRAIN reset committed; mandatory actual Treasury gold and Misc/Selection/GameData flags required"};
  p.phase=Impl::Phase::queue_pop_pending;return {D::committed,false,"Actual zero lost-production gold branch; queue removal pending"};
 }
 if(p.phase==Impl::Phase::queue_pop_pending){
  need(p.orders.size()==1&&p.orders.front().type==0&&!p.orders.front().save,"TRAIN pop tail finite sole nonrepeat head required");
  auto queue=p.queue;need(queue.children.size()==1,"TRAIN pop raw sole head differs");queue.children.clear();named(queue.fields,"wire_count").value=0;auto expected_queue=queue;
  std::vector<runtime::CityYieldState::ProductionOrder> orders,expected_orders;auto flags=state.city_dirty_events;const auto flags_before=flags;
  const auto occurrence=state.city_dirty_next_occurrence;const auto count=selected?std::uint64_t{2}:std::uint64_t{0};
  need(p.revision<=UINT64_MAX-count-1&&occurrence<=UINT64_MAX-count,"TRAIN queue pop actual revision/flags exhausted");auto next=occurrence;
  if(selected){flags.push_back({next++,generation,p.args.owner,p.args.city,-1,-1,runtime::BnwCityDirtyKind::selection_buttons,true});flags.push_back({next++,generation,p.args.owner,p.args.city,-1,-1,runtime::BnwCityDirtyKind::city_screen,true});}
  auto& actual=named(raw->production.blocks,"queue");auto& actual_orders=*state.cities.players.at(p.args.owner).cities.at(p.args.city).production_orders;
  p.current();p.direct();need(state.city_dirty_events==flags_before&&state.city_dirty_next_occurrence==occurrence,"TRAIN pop late dirty inputs changed");
  // stopHeadOrder(TRAIN) has no process side effect. Sole deletion leaves
  // CleanUpQueue empty; startHeadOrder sees nullptr. No speculative orders.
  std::swap(actual,queue);std::swap(p.queue,expected_queue);actual_orders.swap(orders);p.orders.swap(expected_orders);state.ui_source_revision=++p.revision;
  state.city_dirty_events.swap(flags);state.city_dirty_next_occurrence=next;p.revision+=count;state.ui_source_revision=p.revision;
  p.food.reset();p.phase=Impl::Phase::choose_notification_pending;
  return {D::committed,true,"Actual TRAIN sole-head pop/empty cleanup; native chooseProduction notification pending"};
 }
 need(p.phase==Impl::Phase::choose_notification_pending,"TRAIN native completion stage cannot replay hook/Init");
 return {D::blocked,true,"TRAIN queue removed; mandatory genuine NOTIFICATION_PRODUCTION Add, then final Selection/City/Plot/CityInfo flags pending"};
}
runtime::SelectedProductionResult SelectedCityProductionServices::advance_completed_train_tail(const SelectedCityTrainCreateSource& source)const{
 const auto p=impl_;Impl::Operation operation(*p);const auto canonical=source.retain_factory_owner();
 need(canonical&&canonical->impl_->services.get()==p.get()&&p->stock,"TRAIN completion source different actual services/owner");
 return runtime::advance_selected_completed_train_tail(p->stock,canonical);
}
}

namespace civ5::ui {
SelectedCityTrainCreateSource::BorrowedInitialization SelectedCityTrainCreateSource::borrow_bound_initialization(
 const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>& supplied)const{
 const auto self=retain_factory_owner();const auto& p=*self->impl_;p.current();
 need(p.bound&&(p.phase==Impl::Phase::create_pending||p.phase==Impl::Phase::id_assigned)&&supplied,"TRAIN initialization no genuine bound source");
 const auto held=p.allocator.lock();need(held&&held.get()==supplied.get()&&!held.owner_before(supplied)&&!supplied.owner_before(held),"TRAIN initialization actual bound allocator control differs");
 const auto canonical=held->retain_factory_owner();need(canonical&&canonical.get()==held.get()&&!canonical.owner_before(held)&&!held.owner_before(canonical),"TRAIN initialization allocator canonical source retired");
 return {p.services->parent,canonical,p.args.owner,p.args.city,p.revision};
}
SelectedTrainInitializationObservation SelectedCityProductionServices::begin_completed_train_unit_initialization(
 const SelectedBnwReadOnlyPreview& preview,const SelectedCityTrainCreateSource& supplied,std::shared_ptr<BnwClientSceneFrame> scene)const{
 const auto p=impl_;Impl::Operation operation(*p);const auto source=supplied.retain_factory_owner();
 need(source&&source->impl_->services.get()==p.get()&&p->stock&&scene,"TRAIN live initialization actual Source/services/scene absent");
 source->require_current_native();const auto args=source->allocator_operands();
 preview.with_owned_host_camera_input([&](const auto& host,const LiveBnwPreviewInput* input){
  need(host&&input&&host.get()==p->parent.get()&&!host.owner_before(p->parent)&&!p->parent.owner_before(host)&&&input->session==p->session,"TRAIN initialization Preview belongs to another SAME Host/session");p->entry();
 });
 // The actual factory+opaque bind are invoked here, not delegated to a caller
 // result/ID. Core holds Source strongly, Source holds Core weakly.
 const auto core=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,args.owner);
 core->bind_completed_train_source(*source);source->borrow_bound_initialization(core);
 return runtime::initialize_selected_completed_train_unit(p->stock,preview,source,core,std::move(scene));
}
}

namespace civ5::ui {
void SelectedCityTrainCreateSource::accept_actual_initial_name_game_counter_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN name/game-count creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN name/game-count allocator differs");
    core.require_initial_name_game_counter_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual name/game-count revision differs");
    ++p->revision; // Actual canonical Core stores already committed; no caller counters.
}
}

namespace civ5::ui {
void SelectedCityTrainCreateSource::accept_actual_initial_game_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN game-class creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN game-class allocator differs");
    core.require_initial_game_class_counter_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual game-class revision differs");
    ++p->revision; // Actual native Game UnitClass store, never supplied counts.
}
}

namespace civ5::ui {
void SelectedCityTrainCreateSource::accept_actual_initial_team_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN team-class creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN team-class allocator differs");
    core.require_initial_team_class_counter_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual team-class revision differs");
    ++p->revision; // Authenticated Team native class vector store only.
}
}

namespace civ5::ui {
void SelectedCityTrainCreateSource::accept_actual_initial_player_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN player-class creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN player-class allocator differs");
    core.require_initial_player_class_counter_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual player-class revision differs");
    ++p->revision; // Authenticated Player native class vector store only.
}
}

namespace civ5::ui {
void SelectedCityTrainCreateSource::accept_actual_initial_military_support_prefix(const runtime::BnwNativeUnitAllocatorCore& core)const{
    const auto p=impl_;
    need(p&&p->services->thread==std::this_thread::get_id()&&p->bound&&p->phase==Impl::Phase::id_assigned&&p->revision<UINT64_MAX,
        "TRAIN military creator/phase retired");
    const auto held=p->allocator.lock();need(held&&held.get()==&core,"TRAIN military allocator differs");
    // Core issues this callback only after actual MilitarySupport=true raw
    // scalar write. Its private proof authenticates current native receipt.
    core.require_initial_military_support_prefix(*this);
    need(p->services->initial->ui_source_revision==p->revision+1,"TRAIN actual military revision differs");
    ++p->revision;
}
}
