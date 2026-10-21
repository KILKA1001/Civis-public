#include "civ5/ui/bnw_resource_art_tag_read_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <numeric>
#include <stdexcept>
#include <thread>

namespace allocation_probe {
std::atomic<bool> enabled{false},observed{false};
std::atomic<std::size_t> threshold{1};
}
void *operator new(std::size_t size) {
    if(allocation_probe::enabled && size>=allocation_probe::threshold) {
        allocation_probe::observed=true; throw std::bad_alloc();
    }
    if(auto *p=std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p,std::size_t) noexcept { std::free(p); }
void operator delete[](void *p,std::size_t) noexcept { std::free(p); }

namespace {
using namespace civ5;
using Owner=ui::BnwResourceArtTagReadOwner;
using Status=ui::BnwResourceArtInfoStatus;
using Variant=ui::BnwResourceArtVariant;
using Rows=std::span<const ui::BnwResourceArtTagReadFacts>;
std::uint64_t checks=0;
void check(bool value,const char *message) { ++checks; if(!value) throw std::runtime_error(message); }
template<class F> void rejects(F action,const char *message) {
    bool failed=false; try { action(); } catch(const std::exception &) { failed=true; } check(failed,message);
}
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint64_t,1> first{0};
constexpr std::array<std::uint64_t,16> all{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
std::unique_ptr<content::RulesDatabase> rules(bool tags=true,bool collision=false,bool threshold=true) {
    auto db=std::make_unique<content::RulesDatabase>();
    db->apply_sql("CREATE TABLE Defines(Name,Value); CREATE TABLE Features(ID,Type); INSERT INTO Features VALUES(1,'FEATURE_MARSH');");
    if(threshold) db->apply_sql("INSERT INTO Defines VALUES('HEAVY_RESOURCE_THRESHOLD',4);");
    if(tags) db->apply_sql("CREATE TABLE Resources(ID,Type,ArtDefineTag,ArtDefineTagHeavy,ArtDefineTagAlt,ArtDefineTagAltHeavy);"
        "INSERT INTO Resources VALUES(0,'RESOURCE_FUR','normal-fur','heavy-fur','alt-fur','alt-heavy-fur'),"
        "(1,'RESOURCE_OIL','normal-oil','heavy-oil','alt-oil','alt-heavy-oil'),"
        "(2,'RESOURCE_ALUMINUM','normal-aluminum','heavy-aluminum','alt-aluminum','alt-heavy-aluminum'),"
        "(3,'RESOURCE_COAL','n3','h3','a3','ah3'),(4,'RESOURCE_GEMS','n4','h4','a4','ah4'),"
        "(5,'RESOURCE_GOLD','n5','h5','a5','ah5'),(6,'RESOURCE_IRON','n6','h6','a6','ah6'),"
        "(7,'RESOURCE_MARBLE','n7','h7','a7','ah7'),(8,'RESOURCE_URANIUM','n8','h8','a8','ah8'),"
        "(10,'RESOURCE_UNKNOWN','n10','h10','a10','ah10'),(10,'RESOURCE_OVERLAP','n11','h11','a11','ah11'),"
        "(12,NULL,'unknown-type',NULL,NULL,NULL),(13,'RESOURCE_NULL_TAG',NULL,'',NULL,'alt-heavy-null');");
    else db->apply_sql("CREATE TABLE Resources(ID,Type); INSERT INTO Resources VALUES(0,'RESOURCE_FUR');");
    db->apply_sql("CREATE TABLE Other(ID,Type); INSERT INTO Other VALUES(9,'RESOURCE_HOLE'),(11,'APPENDED_RESOURCE_SLOT'),(12,'NULL_RESOURCE_ID');");
    if(collision) db->apply_sql("CREATE TABLE Later(ID,Type); INSERT INTO Later VALUES(1,'RESOURCE_FUR');");
    return db;
}
saves::SavedSession source() {
    saves::SavedSession result; result.source.bytes={1,2,3}; result.snapshot.payload_end_offset=3;
    auto &world=result.snapshot; world.game_state.layout=bnw;
    world.pregame.emplace(); world.pregame->layout=bnw;
    world.pregame->archive.integers["active_player"]=0;
    world.pregame->archive.integer_arrays["team_types"]={3};
    world.map_header.layout=bnw; world.map_header.grid={16,1}; world.map_header.guid[0]=17;
    world.map_projection.emplace(); world.map_projection->grid=world.map_header.grid;
    world.map_plots.emplace(); world.map_plots->layout=bnw; world.map_plots->dimensions.teams=80;
    world.teams.emplace(); world.teams->layout=bnw; world.teams->teams.resize(64);
    constexpr std::array<const char *,16> names{"RESOURCE_FUR","RESOURCE_OIL","RESOURCE_ALUMINUM","RESOURCE_COAL",
        "RESOURCE_GEMS","RESOURCE_GOLD","RESOURCE_IRON","RESOURCE_MARBLE","RESOURCE_URANIUM","RESOURCE_HOLE",
        "RESOURCE_UNKNOWN","APPENDED_RESOURCE_SLOT",nullptr,"RESOURCE_NULL_TAG",nullptr,"UNRECOGNIZED_RESOURCE"};
    for(std::uint64_t i=0;i<16;++i) {
        saves::SavedMapCellProjection cell;
        cell.index=i; cell.x=i; cell.y=0; cell.resource_type_hash=names[i] ? content::saved_info_type_hash(names[i]) : 0;
        if(i==12) { cell.resource_type_hash.reset(); cell.resource_type=12; }
        cell.feature_type_hash=content::saved_info_type_hash("FEATURE_MARSH");
        cell.plot_type=i==0 ? 1 : i==1 ? 3 : 0;
        cell.plot_city=saves::SavedPlotCityReference{-1,-1}; cell.revealed_teams=std::set<std::int64_t>{3};
        saves::PlotObservation raw; raw.index=i; raw.x=i; raw.version=7;
        raw.fields={{"resource_type_hash",static_cast<std::int64_t>(i==12 ? content::saved_info_type_hash("NULL_RESOURCE_ID") : *cell.resource_type_hash)},
            {"feature_type_hash",static_cast<std::int64_t>(*cell.feature_type_hash)}, {"resource_count",i==0 ? 4 : 5},
            {"plot_type",*cell.plot_type},{"plot_city_owner",-1},{"plot_city_id",-1}};
        raw.arrays.push_back({"revealed_words",1,{8,0,0,0}});
        raw.arrays.push_back({"visibility",1,std::vector<std::int64_t>(80,0)});
        raw.arrays.back().values[3]=2;
        world.map_projection->cells.push_back(std::move(cell)); world.map_plots->plots.push_back(std::move(raw));
    }
    return result;
}
runtime::LiveSession session() {
    auto saved=source();
    runtime::LiveSessionState state{
        .world=saved.snapshot,.setup={},.technologies={},.player_research={},.cities={},.unhappiness={},
        .maintenance={},.treasury={},.science={},.research_turn={},
        .random={runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})},
        .research_selection={},.game_clock={},.player_gold_publication={},.player_research_publication={},
        .player_early_publication={},.nearby_cities={},.city_flavor_recipients={},.city_strategy_runtime={},.player_flavor_recipients={},
        .plot_impassability={},.adjacent_land={},.plot_visibility={},.diplomacy_scratchpad={},
        .notification_load_transients={},.player_plot_lists={},.player_read_tail={},.game_plot_extras={},
        .player_culture_bomb_timers={},.military_request_counters={},
        .city_dirty_events={},.city_dirty_next_occurrence=1,.specific_city_info_events={},.ui_source_revision=1,
        .plot_gameplay_events={},.plot_gameplay_next_occurrence=1,
        .standalone_services={},.bnw_unit_mission_continuation_quarantine={}};
    state.setup.game_turn=12; state.setup.players[0].alive=true; state.setup.players[0].team=3;
    runtime::rebuild_bnw_live_plot_visibility(state);
    return runtime::LiveSession::from_restored_bnw(std::move(saved),std::move(state));
}
runtime::SessionCheckpointIdentity identity(const runtime::LiveSession &live,const content::RulesDatabase &db) {
    return {runtime::identify_bytes(live.source().source.bytes).sha256,db.gameplay_fingerprint_sha256()};
}
// Explicit fault probe: bypass notify/cache retirement to test same-generation
// source requery; production code must use the actual mutator/event owners.
runtime::LiveSessionState &fault(runtime::LiveSession &s) { return const_cast<runtime::LiveSessionState &>(s.state()); }
std::int64_t &field(runtime::LiveSession &s,std::uint64_t plot,std::string_view name) {
    for(auto &entry:fault(s).world.map_plots->plots.at(plot).fields) if(entry.name==name) return entry.value;
    throw std::runtime_error("Fixture field absent");
}
void scalar_contract() {
    ui::BnwResourceArtScalarInput in; in.resource=0; in.cached_resource_types={0,1,2,3,4,5,6,7,8}; in.cached_marsh=1;
    in.threshold=4; in.quantity=4;
    check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::normal,"Signed equality became heavy");
    in.quantity=5; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::heavy,"Signed strict heavy branch differs");
    in.hills=true; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::alt_heavy,"FUR hills branch differs");
    in.quantity=-128; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::alt,"Signed byte was interpreted unsigned");
    in.resource=1; in.water=true; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::alt,"OIL water branch differs");
    in.resource=2; in.feature=1; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::alt,"Marsh branch differs");
    in.feature=-1; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::normal,"Non-marsh became alt");
    in.resource=99; check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::normal,"Uncached resource guessed alt");
    in.cached_resource_types[0]=2; in.resource=2; in.hills=false; in.feature=1;
    check(ui::select_bnw_resource_art_scalar_variant(in)==Variant::normal,"First source cache match was replaced by later match");
    in.resource=-1; check(!ui::select_bnw_resource_art_scalar_variant(in),"NO_RESOURCE produced scalar variant");
}
void catalog_and_current_facts() {
    auto live=session(); auto db=rules(); auto types=db->cache_info_types();
    rejects([&] { Owner bad(live,*db,types,identity(live,*db),0); },"Unsealed scope accepted");
    db->seal_immutable_preview_fingerprints(); const auto id=identity(live,*db);
    Owner owner(live,*db,types,id,0); auto receipt=owner.capture(live,all,1);
    check(receipt && owner.matches(live,receipt,1),"Current resource facts refused");
    check(receipt->stamp().active_team==3 && receipt->stamp().turn==12 && receipt->stamp().width==16 && receipt->stamp().map_guid[0]==17,
        "Current source stamp differs");
    const auto plots=receipt->plots();
    check(plots[0].art_define_tags==std::array<std::optional<std::string>,4>{"normal-fur","heavy-fur","alt-fur","alt-heavy-fur"},"Four tags lost order");
    check(plots[0].heavy_threshold==4 && !plots[0].quantity_exceeds_heavy_threshold && plots[1].quantity_exceeds_heavy_threshold,"Define cache replaced with assumed3");
    check(plots[0].hills && !plots[0].water && plots[1].water && !plots[1].hills,"Actual plot_type predicates differ");
    check(plots[0].cached_resource_predicate==true && plots[1].cached_resource_predicate==true && plots[2].cached_resource_predicate==true,"Source cache predicates differ");
    check(plots[9].info_status==Status::null_info_slot && plots[9].info_slot==9,"Source hole became Info");
    check(plots[11].info_slot==11 && plots[11].authored_id==10 && plots[11].type=="RESOURCE_OVERLAP","Overlapping source ID did not append");
    check(types.find("RESOURCE_OVERLAP")->id==10,"Global type-cache authored ID was replaced with appended Info slot");
    check(plots[12].info_status==Status::absent_type && plots[12].logical_resource==12 && !plots[12].type,"NULL ResourceInfo Type invented identity");
    check(plots[13].art_define_tags[0]==std::nullopt && plots[13].art_define_tags[1]=="" && plots[13].art_define_tags[3]=="alt-heavy-null","NULL/empty tags replaced");
    check(plots[14].info_status==Status::no_resource && plots[14].logical_resource==-1,"Wire NO_RESOURCE differs");
    check(plots[15].info_status==Status::unresolved_identity,"Unknown hash fabricated a resource");
    check(plots[0].revealed && plots[0].visibility_count==2 && plots[0].plot_city==saves::SavedPlotCityReference{-1,-1},"Current scalar reveal/visibility/handle differs");
    check(receipt->stamp().debug_cache.status==ui::BnwResourceDebugCacheReadStatus::unavailable &&
        !receipt->stamp().debug_cache.cached_mode,"Diagnostic import fabricated checked debug false");
    for(const auto &plot:plots) check(plot.layout_blockers[0]==ui::BnwResourceLayoutReadBlocker::missing_current_debug_owner &&
        plot.layout_blockers[1]==ui::BnwResourceLayoutReadBlocker::missing_current_globals_cache_owner,"Unknown native Game/GC owners omitted");
    check(!owner.matches(live,receipt,2),"Foreign frame accepted"); auto other=session(); check(!owner.matches(other,receipt,1),"Foreign session accepted");
    Owner foreign(live,*db,types,id,0); check(!foreign.matches(live,receipt,1),"Foreign receipt owner accepted");
    const auto unknown_feature=content::saved_info_type_hash("UNKNOWN_CURRENT_FEATURE");
    field(live,1,"feature_type_hash")=unknown_feature; fault(live).world.map_projection->cells[1].feature_type_hash=unknown_feature;
    field(live,2,"feature_type_hash")=unknown_feature; fault(live).world.map_projection->cells[2].feature_type_hash=unknown_feature;
    check(!owner.matches(live,receipt,1),"Consumed current feature mutation accepted");
    const std::array<std::uint64_t,2> partial_indices{1,2}; const auto partial=owner.capture(live,partial_indices,2);
    check(partial && !partial->plots()[0].feature && partial->plots()[0].info_status==Status::info &&
        partial->plots()[0].cached_resource_predicate==true,"Unknown FeatureType incorrectly retired known OIL catalog/water facts");
    check(partial && !partial->plots()[1].feature && partial->plots()[1].info_status==Status::info &&
        !partial->plots()[1].cached_resource_predicate,"Unknown marsh branch fabricated a predicate or lost owned ResourceInfo tags");
    receipt=owner.capture(live,first,1);
    field(live,0,"resource_count")=5; check(!owner.matches(live,receipt,1),"Same-generation signed quantity mutation accepted");
    receipt=owner.capture(live,first,2); check(receipt && receipt->plots()[0].quantity==5,"Requery did not publish actual new quantity");
    fault(live).world.map_plots->plots[0].arrays[0].values[0]=0; fault(live).world.map_projection->cells[0].revealed_teams->clear();
    check(!owner.matches(live,receipt,2),"Same-generation reveal mutation accepted");
    receipt=owner.capture(live,first,3); check(receipt && !receipt->plots()[0].revealed && receipt->plots()[0].info_status==Status::info,
        "Unrevealed raw resource replaced with invented renderer candidate");
    fault(live).world.map_plots->plots[0].arrays[1].values[3]=-1;
    check(!owner.matches(live,receipt,3),"Changed raw visibility passed stale current visibility owner");
    runtime::rebuild_bnw_live_plot_visibility(fault(live)); receipt=owner.capture(live,first,3);
    check(receipt && receipt->plots()[0].visibility_count==0,"Read owner used raw negative visibility instead of native clamped current cache");
    field(live,0,"plot_type")=3; fault(live).world.map_projection->cells[0].plot_type=3;
    check(!owner.matches(live,receipt,3),"In-place current plot predicate mutation accepted");
    field(live,0,"resource_count")=128; rejects([&] { owner.capture(live,first,4); },"Unsigned quantity above signed native byte accepted");
    check(!owner.matches(live,receipt,3),"Rejected capture retained old publication"); field(live,0,"resource_count")=-128;
    receipt=owner.capture(live,first,5); check(receipt && receipt->plots()[0].quantity==-128,"Native signed byte lower bound differs");
    fault(live).setup.players[0].team=4; check(!owner.matches(live,receipt,5),"Active team source mismatch accepted"); fault(live).setup.players[0].team=3;
    const auto checkpoint=live.checkpoint();
    for(int i=0;i<16;++i) { live.restore(checkpoint); check(!owner.matches(live,receipt,5),"Repeated identical restore/address reuse accepted"); }
    Owner rebound(live,*db,types,id,0); receipt=rebound.capture(live,first,1); check(receipt && rebound.matches(live,receipt,1),"Explicit current generation rebind refused frame1");
    db.reset(); types=content::InfoTypeCatalog{}; check(rebound.matches(live,receipt,1),"Retained SAME Impl/snapshot dereferenced retired DB/types facade");
    check(receipt->plots()[0].art_define_tags[0]=="normal-fur","Retained catalog storage lost");
}
void query_errors_and_admission() {
    auto live=session(); auto db=rules(false); auto types=db->cache_info_types(); db->seal_immutable_preview_fingerprints();
    Owner absent(live,*db,types,identity(live,*db),0); auto receipt=absent.capture(live,first,1);
    check(receipt && std::all_of(receipt->plots()[0].art_define_tags.begin(),receipt->plots()[0].art_define_tags.end(),[](auto &x) {return !x;}),"Absent tag columns fabricated keys");
    db=rules(true,true); types=db->cache_info_types(); db->seal_immutable_preview_fingerprints();
    Owner collision(live,*db,types,identity(live,*db),0); receipt=collision.capture(live,first,1);
    check(receipt && receipt->plots()[0].logical_resource==1 && receipt->plots()[0].type=="RESOURCE_OIL" && receipt->plots()[0].first_cached_resource_match==0,
        "Cross-table type overwrite/first match was replaced with Resources-only cache");
    auto missing=rules(true,false,false); auto mt=missing->cache_info_types(); missing->seal_immutable_preview_fingerprints();
    Owner default_define(live,*missing,mt,identity(live,*missing),0); receipt=default_define.capture(live,first,1);
    check(receipt && receipt->plots()[0].heavy_threshold==0,"Missing BNW define cache invented constructor3 fallback");
    auto wrong=identity(live,*db); wrong.payload_sha256="wrong";
    rejects([&] { Owner bad(live,*db,types,wrong,0); },"Foreign payload identity accepted");
    wrong=identity(live,*db); wrong.rules_sha256="wrong"; rejects([&] { Owner bad(live,*db,types,wrong,0); },"Foreign Rules identity accepted");
    rejects([&] { Owner bad(live,*db,mt,identity(live,*db),0); },"Foreign global type cache accepted");
    rejects([&] { Owner bad(live,*db,types,identity(live,*db),1); },"Foreign active player accepted");
    auto broken=std::make_unique<content::RulesDatabase>(); broken->apply_sql("CREATE TABLE Defines(Name,Value); CREATE TABLE Other(ID,Type);");
    const auto bt=broken->cache_info_types(); broken->seal_immutable_preview_fingerprints();
    rejects([&] { Owner bad(live,*broken,bt,identity(live,*broken),0); },"Missing Resources SQL read accepted");
    Owner owner(live,*db,types,identity(live,*db),0); const auto previous=owner.capture(live,first,1);
    const std::vector<std::uint64_t> over_budget(1024*1024+1,0);
    allocation_probe::threshold=over_budget.size()*sizeof(std::uint64_t); allocation_probe::observed=false; allocation_probe::enabled=true;
    bool threw=false; try { receipt=owner.capture(live,over_budget,2); } catch(...) { threw=true; } allocation_probe::enabled=false;
    check(!threw && !receipt && !allocation_probe::observed,"Over-budget input was allocated before admission");
    check(!owner.matches(live,previous,1),"Refused batch did not retire publication");
    receipt=owner.capture(live,first,3); check(receipt!=nullptr,"Admission refusal poisoned valid source");
    const std::array<std::uint64_t,2> duplicate{0,0}; rejects([&] { owner.capture(live,duplicate,4); },"Duplicate requested plot accepted");
    check(!owner.matches(live,receipt,3),"Throwing requested batch retained publication");
    const std::array<std::uint64_t,1> outside{16}; rejects([&] { owner.capture(live,outside,5); },"Outside plot accepted");
    check(!owner.capture(live,first,0) && !owner.capture(live,{},1),"Invalid frame/empty admission accepted");
}
void callbacks_and_poison() {
    auto live=session(); auto db=rules(); auto types=db->cache_info_types(); db->seal_immutable_preview_fingerprints(); const auto id=identity(live,*db);
    Owner storage(live,*db,types,id,0); auto receipt=storage.capture(live,first,1); const auto retained=receipt;
    std::optional<std::shared_ptr<const ui::BnwResourceArtTagReadReceipt>> optional_storage{receipt};
    check(storage.visit_current_resource_facts(live,*optional_storage,1,[&](Rows rows) {
        optional_storage.reset(); receipt.reset(); check(rows[0].art_define_tags[0]=="normal-fur","Pinned input retired inside callback");
    }),"Destroyed caller receipt storage caused UAF/retarget");
    auto payload=std::make_shared<const std::string>("owned callback"); std::weak_ptr<const std::string> weak=payload;
    std::optional<std::function<void(Rows)>> visitor;
    visitor=[payload,&visitor,&weak](Rows rows) { visitor.reset(); check(!weak.expired() && *payload=="owned callback" && rows[0].type=="RESOURCE_FUR","Pinned callback captures destroyed"); };
    payload.reset(); check(storage.visit_current_resource_facts(live,retained,1,*visitor),"Destroyed caller function storage caused UAF/refusal");
    check(!visitor && weak.expired(),"Pinned callable leaked or caller storage reread");
    check(!storage.visit_current_resource_facts(live,retained,1,[&](Rows) { field(live,0,"resource_count")=5; }),"Visitor in-place mutation authorized after callback");
    const auto checkpoint=live.checkpoint();
    Owner replacement(live,*db,types,id,0); auto replaced=replacement.capture(live,first,1);
    check(!replacement.visit_current_resource_facts(live,replaced,1,[&](Rows) { live.restore(checkpoint); }),"Callback state replacement authorized after callback");
    check(replaced->plots()[0].quantity==5,"Retained receipt payload altered by state replacement");
    auto retired=std::make_unique<Owner>(live,*db,types,id,0); receipt=retired->capture(live,first,1);
    check(!retired->visit_current_resource_facts(live,receipt,1,[&](Rows rows) { retired.reset(); check(rows[0].type=="RESOURCE_FUR","Retired owner destroyed active facts"); }),"Retired owner still authorized");
    check(receipt->plots()[0].type=="RESOURCE_FUR","Retired receipt lost owned facts");
    Owner reentry(live,*db,types,id,0); receipt=reentry.capture(live,first,1);
    check(!reentry.visit_current_resource_facts(live,receipt,1,[&](Rows) { check(!reentry.matches(live,receipt,1),"Reentrant read entered owner"); }),"Reentry left outer read authorized");
    check(!reentry.capture(live,first,2),"Permanent reentry poison recovered");
    Owner throwing(live,*db,types,id,0); receipt=throwing.capture(live,first,1);
    check(!throwing.visit_current_resource_facts(live,receipt,1,[](Rows) { throw std::runtime_error("uncertain callback"); }) && !throwing.matches(live,receipt,1),"Throwing visitor not poisoned");
    Owner threaded(live,*db,types,id,0); receipt=threaded.capture(live,first,1); bool foreign=true;
    std::thread worker([&] { foreign=threaded.matches(live,receipt,1); }); worker.join();
    check(!foreign && !threaded.matches(live,receipt,1),"Wrong-thread owner not permanently refused");
    Owner poisoned(live,*db,types,id,0); receipt=poisoned.capture(live,first,1);
    rejects([&] { db->apply_sql("UPDATE Resources SET ArtDefineTag='retargeted'"); },"Sealed writer accepted");
    check(!poisoned.matches(live,receipt,1) && receipt->plots()[0].art_define_tags[0]=="normal-fur","Rules poison ignored or immutable facts retargeted");
}
// Explicit diagnostic fixture over the existing current raw/owned city
// registry. It does not attest original Game reset, native debug/GC caches,
// city creation execution or drawing/placement readiness.
void seed_current_city_fixture(runtime::LiveSession &live) {
    auto &s=fault(live); s.world.all_players.emplace(); s.world.all_players->layout=bnw;
    s.world.all_players->players.resize(64); s.cities.players.clear();
    for(std::int32_t owner=0;owner<64;++owner) {
        auto &p=s.world.all_players->players[owner]; p.index=owner; p.saved_player_id=owner; p.core.layout=bnw;
        auto &c=p.cities; c.layout=bnw; c.header.layout=bnw; c.header.source_slots=8;
        c.header.fields={{"num_slots",8},{"last_index",-1},{"free_list_head",-1},{"free_list_count",0},{"current_id",16384}};
        c.header.next_free_indices=std::vector<std::int64_t>(8,-1); c.header.wire_city_count=0;
        s.cities.players[owner];
    }
    auto &c=s.world.all_players->players[0].cities;
    c.header.fields[1].value=1; c.header.fields[2].value=0; c.header.fields[3].value=1; c.header.wire_city_count=1;
    saves::SavedCityObservation raw{}; raw.core.layout=bnw; raw.core.start_offset=10; raw.core.end_offset=20;
    for(const auto &[name,value]:std::array<std::pair<const char *,std::int64_t>,2>{{{"m_iID",8193},{"m_eOwner",0}}}) {
        saves::CityCoreField field{}; field.name=name; field.encoding=saves::CityCoreEncoding::i32; field.values={value}; raw.core.fields.push_back(std::move(field));
    }
    c.cities.push_back(std::move(raw));
    auto &owned=s.cities.players[0].cities[8193]; owned.id=8193; owned.owner=0; owned.source_core=std::pair<std::uint64_t,std::uint64_t>{10,20};
}
void plot_city_handle(runtime::LiveSession &live,std::uint64_t plot,std::int64_t owner,std::int64_t id) {
    field(live,plot,"plot_city_owner")=owner; field(live,plot,"plot_city_id")=id;
    fault(live).world.map_projection->cells[plot].plot_city=saves::SavedPlotCityReference{owner,id};
}
void current_city_lookup_facts() {
    auto live=session(); seed_current_city_fixture(live);
    const auto *index_only=runtime::resolve_bnw_current_city(live.state(),0,1);
    const auto *full_id=runtime::resolve_bnw_current_city(live.state(),0,8193);
    check(index_only && index_only==full_id,"Existing FLTA index-only handle did not resolve current full-ID city");
    check(!runtime::resolve_bnw_current_city(live.state(),0,16385),"Stale full-ID generation resolved another city at same index");
    check(!runtime::resolve_bnw_current_city(live.state(),0,0),"Empty native FLTA slot became a city");
    check(!runtime::resolve_bnw_current_city(live.state(),1,1),"City lookup crossed owning player registry");
    check(!runtime::resolve_bnw_current_city(live.state(),64,1) && !runtime::resolve_bnw_current_city(live.state(),-2,1) &&
        !runtime::resolve_bnw_current_city(live.state(),0,-1),"Native invalid owner/NO_ID branch queried unrelated registry");
    plot_city_handle(live,0,0,1); plot_city_handle(live,1,0,8193); plot_city_handle(live,2,0,16385);
    plot_city_handle(live,3,0,0); plot_city_handle(live,4,1,1); plot_city_handle(live,5,64,1);
    fault(live).world.map_projection->cells[0].is_city=false; fault(live).world.map_projection->cells[3].is_city=true;
    auto db=rules(); auto types=db->cache_info_types(); db->seal_immutable_preview_fingerprints(); const auto id=identity(live,*db);
    Owner owner(live,*db,types,id,0); const std::array<std::uint64_t,6> requested{0,1,2,3,4,5};
    auto receipt=owner.capture(live,requested,1); check(receipt && owner.matches(live,receipt,1),"Current plot-city read receipt refused");
    check(receipt->plots()[0].plot_is_city==true && receipt->plots()[1].plot_is_city==true &&
        receipt->plots()[0].resolved_city_id==8193 && receipt->plots()[1].resolved_city_id==8193,
        "Copied index-only/full-ID lookup facts differ or serialized is_city flag overrode source lookup");
    check(receipt->plots()[0].resolved_city_source_core==std::optional{std::pair<std::uint64_t,std::uint64_t>{10,20}},"Copied current city source binding lost");
    for(const auto i:{2,3,4,5}) check(receipt->plots()[i].plot_is_city==false && !receipt->plots()[i].resolved_city_id,
        "Absent/stale/wrong-owner native city handle fabricated a current city");
    const auto generation=live.state_generation();
    fault(live).cities.players[0].cities.at(8193).source_core=std::pair<std::uint64_t,std::uint64_t>{10,21};
    check(live.state_generation()==generation && !owner.matches(live,receipt,1),"Unnotified current city lease mismatch accepted");
    rejects([&] { runtime::resolve_bnw_current_city(live.state(),0,1); },"Wrong current city source binding accepted by thin lookup");
    fault(live).cities.players[0].cities.at(8193).source_core=std::pair<std::uint64_t,std::uint64_t>{10,20};
    check(owner.matches(live,receipt,1),"Restored exact current city input remains stale");
    fault(live).world.all_players->players[0].saved_player_id=1;
    check(!owner.matches(live,receipt,1),"Wrong raw owner registry accepted"); fault(live).world.all_players->players[0].saved_player_id=0;
    auto &raw_city=fault(live).world.all_players->players[0].cities.cities[0]; raw_city.core.fields[1].values[0]=1;
    check(!owner.matches(live,receipt,1),"Unnotified raw city core owner mismatch accepted"); raw_city.core.fields[1].values[0]=0;
    raw_city.core.fields[0].encoding=saves::CityCoreEncoding::hash32;
    check(!owner.matches(live,receipt,1),"Wrong current city ID encoding accepted in copied facts"); raw_city.core.fields[0].encoding=saves::CityCoreEncoding::i32;
    auto duplicate=raw_city; duplicate.core.fields[0].values[0]=16385;
    auto &roster=fault(live).world.all_players->players[0].cities;
    roster.cities.push_back(std::move(duplicate)); roster.header.wire_city_count=2; roster.header.fields[1].value=2;
    auto &duplicate_owned=fault(live).cities.players[0].cities[16385]; duplicate_owned.id=16385; duplicate_owned.owner=0; duplicate_owned.source_core=std::pair<std::uint64_t,std::uint64_t>{10,20};
    check(!owner.matches(live,receipt,1),"Overlapping current native FLTA slot accepted");
    roster.cities.pop_back(); roster.header.wire_city_count=1; roster.header.fields[1].value=1; fault(live).cities.players[0].cities.erase(16385);
    check(owner.matches(live,receipt,1),"Exact native registry recovery remains stale");
    check(!owner.visit_current_resource_facts(live,receipt,1,[&](Rows) { plot_city_handle(live,0,0,16385); }),"Callback plot-city mutation accepted after visitor");
    plot_city_handle(live,0,0,1); receipt=owner.capture(live,first,2); const auto retained=receipt;
    roster.cities.clear(); roster.header.wire_city_count=0; roster.header.fields[1].value=-1; roster.header.fields[2].value=-1; roster.header.fields[3].value=0;
    fault(live).cities.players[0].cities.clear();
    check(!owner.matches(live,receipt,2),"Same-generation removal of actual current city kept stale positive fact");
    receipt=owner.capture(live,first,3); check(receipt && receipt->plots()[0].plot_is_city==false && retained->plots()[0].plot_is_city==true,
        "Current native hole/retained immutable city facts differ");
    const auto checkpoint=live.checkpoint(); live.restore(checkpoint);
    check(!owner.matches(live,receipt,3),"Identical state-generation replacement authorized old city read");
    Owner rebound(live,*db,types,id,0); receipt=rebound.capture(live,first,1); check(receipt && rebound.matches(live,receipt,1),"Explicit city lookup generation rebind refused");
    auto absent=session(); rejects([&] { runtime::resolve_bnw_current_city(absent.state(),0,1); },"Missing current registry crashed or returned invented absence");
    field(live,0,"plot_city_owner")=INT64_MAX; fault(live).world.map_projection->cells[0].plot_city->owner=INT64_MAX;
    check(!rebound.matches(live,receipt,1),"Wide out-of-native handle was truncated into a city owner");
}
void actual_selected(const std::filesystem::path &save,const std::filesystem::path &game) {
    auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);
    if(!result.preview) throw std::runtime_error(result.preview.blocker ? result.preview.blocker->reason : "Actual bootstrap failed");
    auto &host=*result.preview.host; auto &live=host.session();
    Owner owner(live,host.database(),host.types(),host.identity(),host.active_player_id());
    std::vector<std::uint64_t> plots(live.state().world.map_projection->cells.size()); std::iota(plots.begin(),plots.end(),0);
    const auto receipt=owner.capture(live,plots,1); check(receipt && owner.matches(live,receipt,1),"Actual Gandhi resource facts not current");
    std::map<std::uint64_t,std::array<std::optional<std::string>,4>> catalog;
    const auto info_report=host.database().prefetch_info("Resources",[&](auto slot,auto,const auto &row) {
        catalog[slot]={row.text("ArtDefineTag"),row.text("ArtDefineTagHeavy"),row.text("ArtDefineTagAlt"),row.text("ArtDefineTagAltHeavy")}; return true;
    });
    check(info_report.all_succeeded(),"Actual selected ResourceInfo source read failed");
    std::uint64_t resources=0,revealed=0,cities=0;
    for(const auto &row:receipt->plots()) {
        check(row.heavy_threshold==std::get<std::int64_t>(host.defines().find("HEAVY_RESOURCE_THRESHOLD")->lookup.value),"Actual selected Heavy define differs");
        if(row.logical_resource && *row.logical_resource>=0) ++resources;
        if(row.revealed) ++revealed;
        if(row.info_status==Status::info) check(row.info_slot && catalog.at(*row.info_slot)==row.art_define_tags,"Actual ResourceInfo four-tag source join differs");
        const auto *city=runtime::resolve_bnw_current_city(live.state(),static_cast<std::int32_t>(row.plot_city->owner),
            static_cast<std::int32_t>(row.plot_city->id));
        check(row.plot_is_city==std::optional<bool>{city!=nullptr},"Actual plot-city fact differs from existing current GetAt owner");
        if(city) { ++cities; check(row.resolved_city_id && row.resolved_city_source_core==
            std::optional{std::pair{city->core.start_offset,city->core.end_offset}},"Actual copied city binding/source scope differs"); }
        else check(!row.resolved_city_id && !row.resolved_city_source_core,"Absent actual city fabricated resolved identity");
    }
    check(resources>0 && revealed>0,"Actual source has no exercised resource/reveal facts");
    check(receipt->stamp().debug_cache.status==ui::BnwResourceDebugCacheReadStatus::checked_cache &&
        receipt->stamp().debug_cache.cached_mode==std::optional<bool>{false},"Actual reset/Read cached debug false absent");
    check(std::all_of(receipt->plots().begin(),receipt->plots().end(),[](const auto &row) {
        return row.layout_blockers.size()==1 && row.layout_blockers[0]==ui::BnwResourceLayoutReadBlocker::missing_current_globals_cache_owner;
    }),"Checked debug cache read kept missing-debug blocker or invented GC readiness");
    // Explicit actual-owner fault probe: no Game toggle/reset/new-city proof.
    const auto clean_debug_checkpoint=live.checkpoint();
    const auto debug_generation=live.state_generation();
    auto &actual_live=live;
    check(!owner.visit_current_resource_facts(live,receipt,1,[&](Rows) { (void)actual_live.mutable_state(); }),
        "Same-generation callback dirtying retained checked Game debug authority");
    check(live.state_generation()==debug_generation,"Public dirtying unexpectedly replaced state");
    rejects([&] { (void)live.bnw_game_debug_mode(); },"Dirty same-generation actual Game cache still authorized");
    check(!owner.matches(live,receipt,1) && receipt->stamp().debug_cache.cached_mode==std::optional<bool>{false},
        "Retired actual cache authority remained current or changed immutable retained facts");
    actual_live.restore(clean_debug_checkpoint);
    check(!owner.matches(live,receipt,1),"Clean debug checkpoint replacement authorized old Resource receipt");
    Owner rebound(live,host.database(),host.types(),host.identity(),host.active_player_id());
    const auto rebound_receipt=rebound.capture(live,std::array<std::uint64_t,1>{0},1);
    check(rebound_receipt && rebound.matches(live,rebound_receipt,1) &&
        rebound_receipt->stamp().debug_cache.cached_mode==std::optional<bool>{false},"Explicit actual debug Resource rebind refused");
    std::cout<<"ACTUAL selected plots="<<plots.size()<<" resources="<<resources<<" revealed="<<revealed
        <<" cities="<<cities<<" active_team="<<receipt->stamp().active_team<<" threshold="<<receipt->plots()[0].heavy_threshold
        <<" rules="<<receipt->stamp().source.rules_sha256<<"; renderer key unavailable\n";
}
}
int main(int argc,char **argv) {
    try {
        scalar_contract(); catalog_and_current_facts(); query_errors_and_admission(); callbacks_and_poison(); current_city_lookup_facts();
        std::cout<<"PASS "<<checks<<" fixture/scalar/resource-currentness checks; no native layout/drawing key\n";
        if(argc==3) actual_selected(argv[1],argv[2]); else if(argc!=1) throw std::runtime_error("Expected [save gameRoot]");
        std::cout<<"PASS total "<<checks<<" ResourceInfo read-owner checks\n"; return 0;
    } catch(const std::exception &error) { std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n'; return 1; }
}
