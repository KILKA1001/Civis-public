#include "civ5/ui/bnw_feature_art_tag_read_owner.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>

// Explicit allocation fault probe for the public batch admission boundary.
// The caller's large vector is real and allocated before enabling the probe;
// no fabricated invalid span or inaccessible memory is passed to the owner.
namespace allocation_probe {
std::atomic<bool> enabled{false},observed{false};
std::atomic<std::size_t> threshold{1};
}
void *operator new(std::size_t size) {
    if(allocation_probe::enabled && size>=allocation_probe::threshold) {
        allocation_probe::observed=true;
        throw std::bad_alloc();
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
using Owner=ui::BnwFeatureArtTagReadOwner;
using Status=ui::BnwFeatureArtTagReadStatus;
std::uint64_t checks=0;
void check(bool value,const char *message) {
    ++checks;
    if(!value) throw std::runtime_error(message);
}
template<class F> void rejects(F action,const char *message) {
    bool failed=false;
    try { action(); } catch(const std::exception &) { failed=true; }
    check(failed,message);
}
std::unique_ptr<content::RulesDatabase> rules(bool tag_column=true,bool collision=false) {
    auto db=std::make_unique<content::RulesDatabase>();
    if(tag_column) db->apply_sql("CREATE TABLE Features(ID,Type,ArtDefineTag,NaturalWonder);"
        "INSERT INTO Features VALUES(0,'FEATURE_FOREST','ART_DEF_FEATURE_FOREST',0),"
        "(2,'FEATURE_FUJI','ART_DEF_FEATURE_FUJI',1),(2,'FEATURE_FALLOUT','ART_DEF_FEATURE_FALLOUT',0),"
        "(6,'FEATURE_NULL_TAG',NULL,0),(7,'FEATURE_EMPTY_TAG','',0),(8,NULL,'ART_DEF_FEATURE_UNKNOWN',0);");
    else db->apply_sql("CREATE TABLE Features(ID,Type,NaturalWonder); INSERT INTO Features VALUES(0,'FEATURE_FOREST',0);");
    if(collision) db->apply_sql("CREATE TABLE Later(ID,Type); INSERT INTO Later VALUES(1,'FEATURE_FOREST');");
    return db;
}
saves::SavedSession source() {
    saves::SavedSession result;
    result.source.bytes={1,2,3};
    result.snapshot.payload_end_offset=3;
    result.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    result.snapshot.pregame.emplace();
    result.snapshot.pregame->layout=saves::GameStateLayout::expansion2_version1;
    result.snapshot.pregame->archive.integers["active_player"]=0;
    auto &header=result.snapshot.map_header;
    header.layout=saves::GameStateLayout::expansion2_version1;
    header.grid={8,2}; header.guid[0]=17;
    result.snapshot.map_projection.emplace();
    auto &map=*result.snapshot.map_projection;
    map.grid=header.grid;
    for(std::uint64_t i=0;i<16;++i) {
        saves::SavedMapCellProjection cell;
        cell.index=i; cell.x=i%8; cell.y=i/8; cell.feature_type=-1;
        map.cells.push_back(std::move(cell));
    }
    const std::array<std::int64_t,9> slots{0,1,2,3,4,6,7,8,99};
    for(std::size_t i=0;i<slots.size();++i) map.cells[i].feature_type=slots[i];
    map.cells[9].feature_type.reset(); map.cells[9].feature_type_hash=content::saved_info_type_hash("FEATURE_FOREST");
    map.cells[10].feature_type_hash=0; // contradictory numeric+hash: not known absence.
    map.cells[11].feature_type.reset(); map.cells[11].feature_type_hash=0;
    map.cells[12].feature_type=-2;
    map.cells[13].feature_type.reset(); map.cells[13].feature_type_hash=UINT64_MAX;
    map.cells[14].feature_type.reset(); map.cells[14].feature_type_hash=123;
    return result;
}
runtime::LiveSession session() {
    auto saved=source();
    runtime::LiveSessionState state{
        .world=saved.snapshot,.setup={},.technologies={},.player_research={},.cities={},.unhappiness={},
        .maintenance={},.treasury={},.science={},.research_turn={},
        .random={runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
                 runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})},
        .research_selection={},.game_clock={},.player_gold_publication={},.player_research_publication={},
        .player_early_publication={},.nearby_cities={},.city_flavor_recipients={},.city_strategy_runtime={},.player_flavor_recipients={},
        .plot_impassability={},.adjacent_land={},.plot_visibility={},.diplomacy_scratchpad={},
        .notification_load_transients={},.player_plot_lists={},.player_read_tail={},.game_plot_extras={},
        .player_culture_bomb_timers={},.military_request_counters={},
        .city_dirty_events={},.city_dirty_next_occurrence=1,.specific_city_info_events={},.ui_source_revision=1,
        .plot_gameplay_events={},.plot_gameplay_next_occurrence=1,
        .standalone_services={},.bnw_unit_mission_continuation_quarantine={}};
    state.setup.game_turn=12;
    state.setup.players[0].alive=true; state.setup.players[0].team=3;
    return runtime::LiveSession::from_restored_bnw(std::move(saved),std::move(state));
}
runtime::SessionCheckpointIdentity identity(const runtime::LiveSession &live,const content::RulesDatabase &db) {
    return {runtime::identify_bytes(live.source().source.bytes).sha256,db.gameplay_fingerprint_sha256()};
}
constexpr std::array<std::uint64_t,16> all{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
constexpr std::array<std::uint64_t,1> first{0};
void source_slots_and_read_scope() {
    auto db=rules(); auto types=db->cache_info_types(); auto live=session();
    rejects([&] { Owner owner(live,*db,types,identity(live,*db),0); },"Unsealed caller asserted immutable rules");
    db->seal_immutable_preview_fingerprints();
    const auto id=identity(live,*db);
    Owner owner(live,*db,types,id,0);
    const auto receipt=owner.capture(live,all,1);
    check(receipt && owner.matches(live,receipt,1),"Current logical read not accepted");
    check(receipt->stamp().source.rules_sha256==id.rules_sha256 && receipt->stamp().active_team==3 &&
        receipt->stamp().turn==12 && receipt->stamp().map_guid[0]==17,"Selected source stamp differs");
    const auto plots=receipt->plots();
    check(plots[0].status==Status::key && plots[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Forest source tag differs");
    check(plots[1].status==Status::null_info_slot && !plots[1].art_define_tag,"Hole fabricated a feature key");
    check(plots[2].type=="FEATURE_FUJI" && plots[2].natural_wonder==true,"Fuji logical Info differs");
    check(plots[3].info_slot==3 && plots[3].authored_id==2 && plots[3].type=="FEATURE_FALLOUT","Overlapping source ID did not append");
    check(plots[4].status==Status::null_info_slot,"Collection gap is not null Info");
    check(plots[5].status==Status::absent_art_tag && !plots[5].art_define_tag,"NULL tag invented a key");
    check(plots[6].status==Status::key && plots[6].art_define_tag=="","Authored empty tag was replaced");
    check(plots[7].status==Status::absent_type,"NULL type invented an Info Type");
    check(plots[8].status==Status::unresolved_identity,"Outside collection became NO_FEATURE");
    check(plots[9].info_slot==0 && plots[9].art_define_tag==plots[0].art_define_tag,"Hash path differs from native cache");
    check(plots[10].status==Status::unresolved_identity,"Dual wire identity silently selected one side");
    check(plots[11].status==Status::no_feature && plots[15].status==Status::no_feature,"Wire NO_FEATURE differs");
    for(const auto i:{12,13,14}) check(plots[i].status==Status::unresolved_identity,"Unknown wire identity became NO_FEATURE");
    // Existing loader is the independent source for collection classification.
    const auto existing=content::saved_terrain_features(*db,types);
    for(const auto index:{0,1,2,3,4,5,6,7,8,9}) {
        const auto resolved=existing.resolve(live.state().world.map_projection->cells[index]);
        if(plots[index].status==Status::unresolved_identity) check(!resolved,"Existing loader outside slot differs");
        else if(plots[index].status==Status::null_info_slot) check(resolved && resolved->type.empty(),"Existing null slot differs");
        else check(resolved && resolved->type==plots[index].type.value_or(""),"Existing slot/type owner differs");
    }
    check(!owner.matches(live,receipt,2),"Wrong frame accepted");
    auto other=session(); check(!owner.matches(other,receipt,1),"Foreign session accepted");
    Owner other_owner(live,*db,types,id,0);
    check(!other_owner.matches(live,receipt,1),"Foreign read owner accepted receipt");
    std::string actual_join_key;
    check(owner.visit_current_art_keys(live,receipt,1,[&](auto rows) { actual_join_key=*rows[0].art_define_tag; }),
        "Current read could not pass exact key to later registry join");
    check(actual_join_key=="ART_DEF_FEATURE_FOREST","Registry join read retargeted key");
    const auto next=owner.capture(live,first,2);
    check(next && !owner.matches(live,receipt,1),"Retired batch still current");
    const std::array<std::uint64_t,2> duplicate{0,0};
    rejects([&] { (void)owner.capture(live,duplicate,2); },"Duplicate plots accepted");
    check(!owner.matches(live,next,2),"Throwing capture left old latest current");
    const std::array<std::uint64_t,1> outside{16};
    rejects([&] { (void)owner.capture(live,outside,3); },"Outside map plot accepted");
    auto restored=owner.capture(live,first,3);
    check(restored!=nullptr,"Safe capture after input error refused");
    live.mutable_state().world.map_projection->cells[0].feature_type=2;
    check(!owner.matches(live,restored,3),"Unnotified in-place feature mutation accepted stale key");
    restored=owner.capture(live,first,3);
    check(restored && restored->plots()[0].type=="FEATURE_FUJI","Current in-place requery not reflected");
    live.mutable_state().setup.players[0].team=4;
    check(!owner.matches(live,restored,3),"In-place active team mutation accepted");
    live.mutable_state().setup.players[0].team=3;
    live.mutable_state().world.map_header.guid[0]=18;
    check(!owner.matches(live,restored,3),"Changed map owner identity accepted");
    live.mutable_state().world.map_header.guid[0]=17;
    const auto checkpoint=live.checkpoint();
    for(int i=0;i<16;++i) live.restore(checkpoint);
    check(!owner.matches(live,restored,3) && !owner.capture(live,first,3),"Identical replacement generation accepted");
    Owner rebound(live,*db,types,id,0);
    const auto fresh=rebound.capture(live,first,1);
    check(fresh && rebound.matches(live,fresh,1),"Explicit generation rebind failed at frame1");
    // Facade/type storage can go away; only immutable Impl currentness retained.
    db.reset(); types=content::InfoTypeCatalog{};
    check(rebound.matches(live,fresh,1),"Later current read dereferenced dead rules/types facade");
    check(fresh->plots()[0].art_define_tag=="ART_DEF_FEATURE_FUJI","Retained snapshot storage lost");
}
void errors_and_global_cache() {
    auto live=session();
    auto no_column=rules(false); auto types=no_column->cache_info_types(); no_column->seal_immutable_preview_fingerprints();
    Owner absent(live,*no_column,types,identity(live,*no_column),0);
    auto receipt=absent.capture(live,first,1);
    check(receipt && receipt->plots()[0].status==Status::absent_art_tag,"Absent ArtDefineTag column fabricated key");
    auto collision=rules(true,true); types=collision->cache_info_types(); collision->seal_immutable_preview_fingerprints();
    Owner source_cache(live,*collision,types,identity(live,*collision),0);
    const std::array<std::uint64_t,1> hashed{9};
    receipt=source_cache.capture(live,hashed,1);
    check(receipt && receipt->plots()[0].info_slot==1 && receipt->plots()[0].status==Status::null_info_slot,
        "Global cross-table overwrite was replaced with Features-only cache");
    auto foreign=rules(); const auto foreign_types=foreign->cache_info_types();
    rejects([&] { Owner bad(live,*collision,foreign_types,identity(live,*collision),0); },"Foreign type cache accepted");
    auto wrong=identity(live,*collision); wrong.rules_sha256="wrong";
    rejects([&] { Owner bad(live,*collision,types,wrong,0); },"Wrong source rules digest accepted");
    wrong=identity(live,*collision); wrong.payload_sha256="wrong";
    rejects([&] { Owner bad(live,*collision,types,wrong,0); },"Wrong source payload digest accepted");
    rejects([&] { Owner bad(live,*collision,types,identity(live,*collision),1); },"Foreign active player accepted");
    auto broken=std::make_unique<content::RulesDatabase>(); broken->apply_sql("CREATE TABLE Other(ID,Type);");
    types=broken->cache_info_types(); broken->seal_immutable_preview_fingerprints();
    rejects([&] { Owner bad(live,*broken,types,identity(live,*broken),0); },"Failed Features SQL read accepted");
}
void admission_without_copy_allocation() {
    auto live=session(); auto db=rules(); const auto types=db->cache_info_types();
    db->seal_immutable_preview_fingerprints(); Owner owner(live,*db,types,identity(live,*db),0);
    const auto previous=owner.capture(live,first,1);
    // This allocation belongs to the caller and precedes the rejection probe.
    const std::vector<std::uint64_t> over_budget(1024*1024+1,0);
    bool threw=false;
    std::shared_ptr<const ui::BnwFeatureArtTagReadReceipt> result;
    allocation_probe::threshold=over_budget.size()*sizeof(std::uint64_t);
    allocation_probe::observed=false; allocation_probe::enabled=true;
    try { result=owner.capture(live,over_budget,2); } catch(...) { threw=true; }
    allocation_probe::enabled=false;
    check(!threw && !result && !allocation_probe::observed,"Over-budget admission attempted caller-span copy allocation");
    check(!owner.matches(live,previous,1),"Rejected over-budget capture did not retire old receipt");
    const auto restored=owner.capture(live,first,3);
    check(restored!=nullptr,"Input admission refusal poisoned a valid source");
    allocation_probe::threshold=1;
    allocation_probe::observed=false; allocation_probe::enabled=true; threw=false;
    try { result=owner.capture(live,first,0); } catch(...) { threw=true; }
    allocation_probe::enabled=false;
    check(!threw && !result && !allocation_probe::observed,"Zero-frame admission copied caller span before refusing");
    check(!owner.matches(live,restored,3),"Zero-frame refusal left old publication current");
}
void callbacks_and_poison() {
    auto db=rules(); auto types=db->cache_info_types(); db->seal_immutable_preview_fingerprints();
    auto live=session(); const auto id=identity(live,*db);
    Owner changes(live,*db,types,id,0); auto receipt=changes.capture(live,first,1);
    check(!changes.visit_current_art_keys(live,receipt,1,[&](auto) {
        live.mutable_state().world.map_projection->cells[0].feature_type=2;
    }),"Visitor stale in-place key accepted after callback");
    live.mutable_state().world.map_projection->cells[0].feature_type=0;
    const auto checkpoint=live.checkpoint();
    receipt=changes.capture(live,first,2);
    check(!changes.visit_current_art_keys(live,receipt,2,[&](auto) { live.restore(checkpoint); }),
        "Callback generation replacement accepted");
    Owner storage(live,*db,types,id,0); receipt=storage.capture(live,first,1);
    const auto retained=receipt;
    check(storage.visit_current_art_keys(live,receipt,1,[&](auto rows) {
        receipt.reset(); check(rows[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Pinned rows lost inside visitor");
    }),"Caller receipt storage reset retargeted or destroyed pinned receipt");
    check(!receipt && storage.matches(live,retained,1),"Receipt input storage was reread after callback");
    std::optional<std::shared_ptr<const ui::BnwFeatureArtTagReadReceipt>> optional_storage{retained};
    check(storage.visit_current_art_keys(live,*optional_storage,1,[&](auto rows) {
        optional_storage.reset();
        check(rows[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Destroyed optional input retargeted pinned rows");
    }),"Destroyed optional shared_ptr argument caused false rejection/UAF");
    check(!optional_storage && storage.matches(live,retained,1),"Optional argument reread after storage destruction");
    using Rows=std::span<const ui::BnwFeatureArtTagReadFacts>;
    auto captured_payload=std::make_shared<const std::string>("retained callback payload");
    std::weak_ptr<const std::string> captured_weak=captured_payload;
    std::function<void(Rows)> mutable_visitor;
    bool retargeted_ran=false;
    mutable_visitor=[captured_payload,&mutable_visitor,&captured_weak,&retargeted_ran](Rows rows) {
        mutable_visitor=[&retargeted_ran](Rows) { retargeted_ran=true; };
        check(!captured_weak.expired() && *captured_payload=="retained callback payload" &&
            rows[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Caller callback retarget destroyed active captures");
    };
    captured_payload.reset();
    check(storage.visit_current_art_keys(live,retained,1,mutable_visitor),"Retargeted caller callback replaced active operation");
    check(!retargeted_ran && captured_weak.expired(),"Pinned visitor leaked or replacement executed within original call");
    captured_payload=std::make_shared<const std::string>("optional callback payload");
    captured_weak=captured_payload;
    std::optional<std::function<void(Rows)>> optional_visitor;
    optional_visitor=[captured_payload,&optional_visitor,&captured_weak](Rows rows) {
        optional_visitor.reset();
        check(!captured_weak.expired() && *captured_payload=="optional callback payload" &&
            rows[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Optional caller callback destruction freed active capture");
    };
    captured_payload.reset();
    check(storage.visit_current_art_keys(live,retained,1,*optional_visitor),"Destroyed optional callable storage caused UAF/refusal");
    check(!optional_visitor && captured_weak.expired(),"Destroyed caller function was reread or active capture leaked");
    auto retired=std::make_unique<Owner>(live,*db,types,id,0); receipt=retired->capture(live,first,1);
    check(!retired->visit_current_art_keys(live,receipt,1,[&](auto rows) {
        retired.reset(); check(rows[0].art_define_tag.has_value(),"Owner retirement destroyed active rows");
    }),"Owner destroyed in visitor still authorized current read");
    check(receipt->plots()[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Retired receipt not readable");
    Owner reentry(live,*db,types,id,0); receipt=reentry.capture(live,first,1);
    check(!reentry.visit_current_art_keys(live,receipt,1,[&](auto) {
        check(!reentry.matches(live,receipt,1),"Reentrant current read entered owner");
    }),"Swallowed reentry left outer current read authorized");
    check(!reentry.capture(live,first,2),"Poisoned reentry owner recovered implicitly");
    Owner throwing(live,*db,types,id,0); receipt=throwing.capture(live,first,1);
    check(!throwing.visit_current_art_keys(live,receipt,1,[](auto) { throw std::runtime_error("uncertain join"); }),
        "Throwing visitor accepted");
    check(!throwing.matches(live,receipt,1),"Uncertain visitor outcome did not poison owner");
    Owner threaded(live,*db,types,id,0); receipt=threaded.capture(live,first,1);
    bool foreign_thread_result=true;
    std::thread worker([&] { foreign_thread_result=threaded.matches(live,receipt,1); }); worker.join();
    check(!foreign_thread_result && !threaded.matches(live,receipt,1),"Foreign-thread owner access not permanently refused");
    Owner poisoned_rules(live,*db,types,id,0); receipt=poisoned_rules.capture(live,first,1);
    rejects([&] { db->apply_sql("UPDATE Features SET ArtDefineTag='retargeted'"); },"Sealed DB writer accepted");
    check(!poisoned_rules.matches(live,receipt,1),"Retained DB currentness ignored attempted mutation poison");
    check(receipt->plots()[0].art_define_tag=="ART_DEF_FEATURE_FOREST","Poison changed retained immutable facts");
}
}
int main() {
    try {
        source_slots_and_read_scope(); errors_and_global_cache(); admission_without_copy_allocation(); callbacks_and_poison();
        std::cout<<"PASS "<<checks<<" feature ArtDefineTag read-owner checks; no visibility/placement/draw readiness\n";
        return 0;
    } catch(const std::exception &error) { std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n'; return 1; }
}
