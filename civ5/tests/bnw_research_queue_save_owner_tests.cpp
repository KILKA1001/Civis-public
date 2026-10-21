#include "civ5/runtime/bnw_research_queue_save_owner.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <set>
#include <string_view>
namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW research choice checkpoint accepted");
}
std::string_view name(const char *v) {return v?std::string_view(v):std::string_view();}
saves::PlayerAdministrationRecord &pre_city(saves::SavedPlayerObservation &player) {
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="pre_city") return block;
    throw std::runtime_error("Selected BNW pre-city block unavailable");
}
const saves::PlayerAdministrationRecord &pre_city(
    const saves::SavedPlayerObservation &player) {
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="pre_city") return block;
    throw std::runtime_error("Selected BNW pre-city block unavailable");
}
saves::PlayerKnowledgeBlock &techs(saves::SavedPlayerObservation &player) {
    for(auto &block:player.knowledge.blocks)
        if(name(block.name)=="player_techs") return block;
    throw std::runtime_error("Selected BNW player-tech block unavailable");
}
const saves::PlayerKnowledgeBlock &techs(
    const saves::SavedPlayerObservation &player) {
    for(const auto &block:player.knowledge.blocks)
        if(name(block.name)=="player_techs") return block;
    throw std::runtime_error("Selected BNW player-tech block unavailable");
}
std::vector<std::int64_t> &queue(saves::PlayerAdministrationRecord &block) {
    for(auto &array:block.arrays)
        if(name(array.name)=="m_researchQueue") return array.values;
    throw std::runtime_error("Selected BNW research queue unavailable");
}
const std::vector<std::int64_t> &queue(
    const saves::PlayerAdministrationRecord &block) {
    for(const auto &array:block.arrays)
        if(name(array.name)=="m_researchQueue") return array.values;
    throw std::runtime_error("Selected BNW research queue unavailable");
}
saves::TeamHashedArray &flags(saves::PlayerKnowledgeBlock &block) {
    for(auto &array:block.hashes)
        if(name(array.name)=="researching_tech") return array;
    throw std::runtime_error("Selected BNW researching flags unavailable");
}
const saves::TeamHashedArray &flags(const saves::PlayerKnowledgeBlock &block) {
    for(const auto &array:block.hashes)
        if(name(array.name)=="researching_tech") return array;
    throw std::runtime_error("Selected BNW researching flags unavailable");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,
                                       std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:doc.sections) if(part.id==id) {
        check(!found,"Duplicate BNW research choice section");found=&part;
    }
    check(found,"Missing BNW research choice section");return *found;
}
void selected(const std::filesystem::path& input,const std::filesystem::path& root){
 const auto pin=runtime::identify_file(input);
 const auto temp=std::filesystem::temp_directory_path()/
     ("civis-research-save-owner-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directory(temp);
 const auto first=temp/"first.CivisSave",again=temp/"again.CivisSave",third=temp/"third.CivisSave";
 std::int64_t chosen=-1;std::size_t slot=0;
 {
 auto boot=ui::bootstrap_selected_bnw_preview(input,root,0);
 check(boot.preview.host&&!boot.preview.blocker,"Genuine selected BNW fixture unavailable");
 auto& h=*boot.preview.host;const auto& baseline=h.session().state();
 auto state=baseline;const auto& source=h.session().source().snapshot;
 const auto& types=h.types();const auto& rules=h.technologies();
 auto& raw=state.world.all_players->players.at(0);auto& rows=flags(techs(raw)).rows;
 check(queue(pre_city(raw)).empty()&&baseline.player_research.players.at(0).queue.empty()&&
       baseline.research_turn.players.at(0).researching.empty(),
       "Natural research save fixture requires empty initial research queue");
 for(std::size_t i=0;i<rows.size();++i){
  if(!rows[i].type_hash||rows[i].values.size()!=1||rows[i].values[0])continue;
  const auto* type=types.find_hash(rows[i].type_hash);
  const auto* info=type&&type->id>=0?rules.find(static_cast<std::uint64_t>(type->id)):nullptr;
  if(!info||type->table!="Technologies"||type->type!=info->base.type)continue;
  chosen=type->id;slot=i;break;
 }
 check(chosen>=0,"Natural fixture lacks an actual technology/flag slot");
 // This is a decoded persistence fixture, not a player command or a tech grant.
 state.player_research.players.at(0).queue={chosen};
 state.research_turn.players.at(0).researching.insert(chosen);
 const auto bundle=runtime::make_bnw_research_queue_checkpoint_bundle(
     state,baseline,source,rules,types,h.identity());
 auto doc=saves::make_civis_import_document(h.session().source().source,pin.sha256);
 doc.sections.push_back(bundle.raw);doc.sections.push_back(bundle.research);
 doc.sections.push_back(bundle.research_turn);
 doc.sections.push_back(runtime::make_simulation_random_checkpoint(baseline.random,h.identity()));
 saves::write_civis_save(first,doc);
 }
 const auto doc=saves::read_civis_save(first);
 {
 auto loaded=ui::bootstrap_selected_bnw_preview(first,root,0);
 check(loaded.preview.host&&!loaded.preview.blocker,"Normal loader rejected research queue bundle");
 ui::SelectedBnwReadOnlyPreview preview(std::move(loaded.preview));
 const auto& loaded_state=preview.session().state();
 check(loaded_state.player_research.players.at(0).queue==std::vector<std::int64_t>{chosen}&&
       loaded_state.research_turn.players.at(0).researching.contains(chosen)&&
       queue(pre_city(loaded_state.world.all_players->players.at(0)))==std::vector<std::int64_t>{chosen}&&
       flags(techs(loaded_state.world.all_players->players.at(0))).rows.at(slot).values[0]==1,
       "Normal loader lost raw/owned queue or researching flags");
 auto owner=runtime::BnwResearchQueueSaveOwner::from_selected_preview(preview,first);
 const auto prepared=owner->prepare();
 for(const auto& part:doc.sections){
  const auto& kept=section(prepared,part.id);
  check(kept.flags==part.flags&&kept.bytes==part.bytes,
        "Loaded re-save changed a research companion or another retained section");
 }
 owner->write_new(again);
 rejects([&]{owner->write_new(temp/"wrong.Civ5Save");});
 rejects([&]{(void)runtime::BnwResearchQueueSaveOwner::from_selected_preview(preview,input);});
 // Genuine mutable facade API retires the actual import owner in place.
 preview.with_owned_host_camera_input([&](auto actual,const auto*){
  (void)const_cast<ui::SelectedBnwPreviewHost&>(*actual).session().mutable_state();
 });
 const auto stale=temp/"stale.CivisSave";
 rejects([&]{owner->write_new(stale);});check(!std::filesystem::exists(stale),
      "Stale owner wrote a partial checkpoint");
 }
 {
 auto reopened=ui::bootstrap_selected_bnw_preview(again,root,0);
 check(reopened.preview.host&&!reopened.preview.blocker,"Normal loader rejected genuine owner re-save");
 const auto& final=reopened.preview.host->session().state();
 check(final.player_research.players.at(0).queue==std::vector<std::int64_t>{chosen}&&
       final.research_turn.players.at(0).researching==std::set<std::int64_t>{chosen}&&
       queue(pre_city(final.world.all_players->players.at(0)))==std::vector<std::int64_t>{chosen}&&
       flags(techs(final.world.all_players->players.at(0))).rows.at(slot).values==std::vector<std::int64_t>{1}&&
       !reopened.preview.host->session().readiness().can_continue(),
       "Genuine research checkpoint restart differs or falsely grants full continuation");
 ui::SelectedBnwReadOnlyPreview next(std::move(reopened.preview));
 auto resaver=runtime::BnwResearchQueueSaveOwner::from_selected_preview(next,again);
 resaver->write_new(third);
 }
 const auto second_doc=saves::read_civis_save(again),third_doc=saves::read_civis_save(third);
 for(const auto& part:second_doc.sections){const auto& kept=section(third_doc,part.id);
  check(kept.flags==part.flags&&kept.bytes==part.bytes,"Second genuine re-save drifted");}
 check(runtime::identify_file(input).sha256==pin.sha256,"Original Civ5Save changed");
 std::cout<<"PASS decoded queue/flags -> normal load -> genuine resave -> restart; full continuation unavailable\n";
}
}
int main(int argc,char** argv){try{
 check(argc==3,"Supply natural BNW save and actual game root");selected(argv[1],argv[2]);return 0;
}catch(const std::exception& e){std::cerr<<"FAIL research save owner: "<<e.what()<<'\n';return 1;}}
