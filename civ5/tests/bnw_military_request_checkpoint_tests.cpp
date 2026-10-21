#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/bnw_player_trade_strategy_prefix.hpp"
#include "civ5/runtime/bnw_live_player_state_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
namespace {
using namespace civ5;
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
template<class F>void rejects(F&&f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected,"Invalid military checkpoint accepted");}
std::int64_t army(const runtime::LiveSessionState&s,std::int64_t id){
 for(const auto&b:s.world.all_players->players.at(static_cast<std::size_t>(id)).strategy.blocks)
  if(b.name&&std::string_view(b.name)=="military_ai")for(const auto&f:b.fields)
   if(f.name&&std::string_view(f.name)=="army_type_building")return f.value;
 throw std::runtime_error("Actual military army field missing");
}
const saves::CivisSaveSection&part(const saves::CivisSaveDocument&d,std::uint32_t id){const saves::CivisSaveSection*r=nullptr;for(const auto&s:d.sections)if(s.id==id){check(!r,"Duplicate test checkpoint section");r=&s;}check(r,"Missing test checkpoint section");return *r;}
void natural(const std::filesystem::path&save,const std::filesystem::path&assets,const std::filesystem::path&output){
 check(!std::filesystem::exists(output),"Military fixture output must be fresh");std::filesystem::create_directory(output);
 const auto source_hash=runtime::identify_file(save).sha256;
 auto original=ui::bootstrap_selected_bnw_preview(save,assets,0);
 check(original.preview.host&&!original.preview.blocker,"Actual original BNW military bootstrap unavailable");
 const auto host=original.preview.host;auto state=host->session().state();
 check(state.military_request_counters.empty(),"Original import manufactured military zeros");
 std::vector<std::int64_t> living;for(const auto&[id,p]:state.setup.players)if(id>=0&&id<22&&p.alive)living.push_back(id);
 check(living.size()>=2,"Selected military fixture needs two real living major players");
 const auto first_actor=living.at(0),second_actor=living.at(1);
 const auto before_army=army(state,first_actor);
 const auto reset=runtime::apply_bnw_player_military_reset_counters(state,first_actor);
 check(reset.player_id==first_actor&&reset.army_type_before==before_army&&army(state,first_actor)==-1&&state.military_request_counters.size()==1,"Actual military Reset source effect absent");
 check(!state.military_request_counters.contains(second_actor),"Reset invented unrelated counters");
 const auto first=runtime::make_bnw_live_player_state_partial_export(state,host->session().source(),host->identity(),source_hash);
 check(!first.full_persistence&&!first.uncovered_domains.empty(),"Military partial export claims full persistence");
 const auto&section=part(first.document,44);
 // Identity, object/ID23 binding and transaction rejection use the real source.
 auto blank=state;blank.military_request_counters.clear();
 const auto refuses_unchanged=[&](const saves::CivisSaveSection&s,const runtime::SessionCheckpointIdentity&id){auto untouched=blank;rejects([&]{runtime::restore_bnw_military_request_checkpoint(s,untouched,host->session().source().snapshot,id);});check(untouched.military_request_counters.empty(),"Rejected military state partially published");};
 auto bad=section;bad.bytes.pop_back();refuses_unchanged(bad,host->identity());
 bad=section;bad.flags=1;refuses_unchanged(bad,host->identity());
 bad=section;bad.bytes.at(8)=2;refuses_unchanged(bad,host->identity());
 bad=section;bad.bytes.at(272)=63;refuses_unchanged(bad,host->identity());
 bad=section;bad.bytes.at(276)=1;refuses_unchanged(bad,host->identity());
 auto foreign=host->identity();foreign.rules_sha256=std::string(64,'b');refuses_unchanged(section,foreign);
 foreign=host->identity();foreign.payload_sha256=std::string(64,'c');refuses_unchanged(section,foreign);
 auto changed=blank;changed.world.all_players->players.at(0).saved_player_id=63;rejects([&]{runtime::restore_bnw_military_request_checkpoint(section,changed,host->session().source().snapshot,host->identity());});check(changed.military_request_counters.empty(),"Foreign military roster partially published");
 auto unpaired=blank;for(auto&b:unpaired.world.all_players->players.at(static_cast<std::size_t>(first_actor)).strategy.blocks)if(b.name&&std::string_view(b.name)=="military_ai")for(auto&f:b.fields)if(f.name&&std::string_view(f.name)=="army_type_building")f.value=0;rejects([&]{runtime::restore_bnw_military_request_checkpoint(section,unpaired,host->session().source().snapshot,host->identity());});check(unpaired.military_request_counters.empty(),"Conflicting army companion partially published");
 auto nonzero=state;nonzero.military_request_counters.at(first_actor).land=1;rejects([&]{(void)runtime::make_bnw_military_request_checkpoint(nonzero,host->session().source().snapshot,host->identity());});
 auto pending=state;pending.player_early_publication=runtime::BnwPlayerEarlyPublication{};rejects([&]{(void)runtime::make_bnw_military_request_checkpoint(pending,host->session().source().snapshot,host->identity());});
 auto orphan=first.document;std::erase_if(orphan.sections,[](const auto&s){return s.id==23;});const auto orphan_file=output/"orphan.CivisSave";
 rejects([&]{saves::write_civis_save(orphan_file,orphan);});check(!std::filesystem::exists(orphan_file),"Orphan military section was published");
 // Deliberately corrupt generic flags for independent common-reader coverage.
 orphan.flags=0;saves::write_civis_save(orphan_file,orphan);
 {std::fstream wire(orphan_file,std::ios::binary|std::ios::in|std::ios::out);wire.seekp(12);const char flags[4]={1,0,0,0};wire.write(flags,4);wire.flush();check(static_cast<bool>(wire),"Cannot corrupt orphan fixture flags");}
 rejects([&]{(void)saves::read_loaded_save(orphan_file);});
 // Shape-valid foreign payload/rules hashes remain codec/owner admission,
 // not structural parser grants. Ordinary bootstrap must refuse publication.
 for(const std::size_t offset:{12u,76u}) {
  auto mismatched=first.document;
  for(auto &p:mismatched.sections)if(p.id==44)p.bytes.at(offset)=static_cast<std::uint8_t>(p.bytes.at(offset)=='0'?'1':'0');
  const auto file=output/("foreign-identity-"+std::to_string(offset)+".CivisSave");saves::write_civis_save(file,mismatched);
  auto refused=ui::bootstrap_selected_bnw_preview(file,assets,0);
  check(!refused.preview.host&&refused.preview.blocker&&refused.preview.blocker->reason.find("Military checkpoint source/rules mismatch")!=std::string::npos,"Foreign military identity missed the genuine codec admission boundary");
 }
 for(const auto kind:{0,1,2}) {
  auto malformed=first.document;
  for(auto &p:malformed.sections)if(p.id==44){if(kind==0)p.flags=1;else if(kind==1)p.bytes.pop_back();else p.bytes.at(8)=2;}
  const auto file=output/("malformed-shape-"+std::to_string(kind)+".CivisSave");
  rejects([&]{saves::write_civis_save(file,malformed);});check(!std::filesystem::exists(file),"Malformed military shape was published");
 }
 auto duplicate=first.document;duplicate.sections.push_back(section);rejects([&]{saves::write_civis_save(output/"duplicate.CivisSave",duplicate);});
 const auto first_file=output/"first.CivisSave";saves::write_civis_save(first_file,first.document);
 auto restarted=ui::bootstrap_selected_bnw_preview(first_file,assets,0);
 check(restarted.preview.host&&!restarted.preview.blocker,"Ordinary first military checkpoint bootstrap failed");
 const auto restored_host=restarted.preview.host;auto restored=restored_host->session().state();
 check(restored.military_request_counters==state.military_request_counters&&army(restored,first_actor)==-1&&!restored.military_request_counters.contains(second_actor),"Actual first bootstrap lost sparse military state");
 check(restored_host->identity().payload_sha256==host->identity().payload_sha256&&restored_host->identity().rules_sha256==host->identity().rules_sha256,"Restart military source/rules identity changed");
 const auto second_reset=runtime::apply_bnw_player_military_reset_counters(restored,second_actor);
 check(second_reset.player_id==second_actor&&restored.military_request_counters.size()==2,"Second actual actor Reset did not extend sparse registry");
 auto lost=restored;lost.military_request_counters.erase(first_actor);rejects([&]{(void)runtime::make_bnw_live_player_state_partial_export(lost,restored_host->session().source(),restored_host->identity(),source_hash);});
 const auto second=runtime::make_bnw_live_player_state_partial_export(restored,restored_host->session().source(),restored_host->identity(),source_hash);
 check(!second.full_persistence&&part(second.document,44).bytes.size()==272+2*12,"Second sparse military checkpoint shape wrong");
 const auto second_file=output/"second.CivisSave";saves::write_civis_save(second_file,second.document);
 auto last=ui::bootstrap_selected_bnw_preview(second_file,assets,0);check(last.preview.host&&!last.preview.blocker,"Ordinary second military checkpoint bootstrap failed");
 check(!last.preview.host->session().readiness().can_continue(),"Military partial checkpoint invented game readiness");
 check(last.preview.host->session().state().military_request_counters==restored.military_request_counters&&army(last.preview.host->session().state(),second_actor)==-1,"Actual second bootstrap lost earned military counters");
 auto carry=saves::make_civis_import_document(last.preview.host->session().source().source,runtime::identify_file(second_file).sha256);check(part(carry,44).bytes==part(second.document,44).bytes,"Import document lost/replaced exact military bytes");
 check(runtime::identify_file(save).sha256==source_hash,"Original Civ5Save overwritten");
 rejects([&]{saves::write_civis_save(save,second.document);});
 check(runtime::identify_file(save).sha256==source_hash,"Civ5Save overwrite protection failed");
 std::cout<<"PASS actual sparse military Reset two ordinary save/load cycles; partial persistence only\n";
}
}
int main(int argc,char**argv){try{check(argc==4,"Supply real BNW save, canonical installation, FRESH output directory");natural(argv[1],argv[2],argv[3]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL military checkpoint: "<<e.what()<<'\n';return 1;}}
