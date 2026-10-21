#include "civ5/runtime/bnw_player_knowledge_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
template<class F> void rejects(F &&operation) {
    bool failed=false;
    try {operation();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW player knowledge checkpoint accepted");
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
saves::PlayerKnowledgeBlock &block(saves::SavedPlayerObservation &player,
                                   std::string_view wanted) {
    for(auto &entry:player.knowledge.blocks)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW knowledge block unavailable");
}
std::int64_t &field(saves::PlayerKnowledgeBlock &owner,
                    std::string_view wanted) {
    for(auto &entry:owner.fields)
        if(name(entry.name)==wanted) return entry.value;
    throw std::runtime_error("Selected BNW knowledge scalar unavailable");
}
std::vector<std::int64_t> &array(saves::PlayerKnowledgeBlock &owner,
                                  std::string_view wanted) {
    for(auto &entry:owner.arrays)
        if(name(entry.name)==wanted) return entry.values;
    throw std::runtime_error("Selected BNW knowledge array unavailable");
}
saves::TeamHashedArray &hashed(saves::PlayerKnowledgeBlock &owner,
                                std::string_view wanted) {
    for(auto &entry:owner.hashes)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW knowledge hash array unavailable");
}
bool same_knowledge(const saves::SavedSessionSnapshot &a,
                    const saves::SavedSessionSnapshot &b) {
    if(!a.all_players || !b.all_players ||
       a.all_players->players.size()!=b.all_players->players.size()) return false;
    for(std::size_t i=0;i<a.all_players->players.size();++i) {
        const auto &left=a.all_players->players[i].knowledge.blocks;
        const auto &right=b.all_players->players[i].knowledge.blocks;
        if(left.size()!=right.size()) return false;
        for(std::size_t j=0;j<left.size();++j) {
            if(name(left[j].name)!=name(right[j].name) ||
               left[j].fields.size()!=right[j].fields.size() ||
               left[j].arrays.size()!=right[j].arrays.size() ||
               left[j].hashes.size()!=right[j].hashes.size()) return false;
            for(std::size_t k=0;k<left[j].fields.size();++k)
                if(name(left[j].fields[k].name)!=name(right[j].fields[k].name) ||
                   left[j].fields[k].value!=right[j].fields[k].value) return false;
            for(std::size_t k=0;k<left[j].arrays.size();++k)
                if(name(left[j].arrays[k].name)!=name(right[j].arrays[k].name) ||
                   left[j].arrays[k].values!=right[j].arrays[k].values) return false;
            for(std::size_t k=0;k<left[j].hashes.size();++k) {
                const auto &x=left[j].hashes[k],&y=right[j].hashes[k];
                if(name(x.name)!=name(y.name) || x.wire_count!=y.wire_count ||
                   x.rows.size()!=y.rows.size()) return false;
                for(std::size_t row=0;row<x.rows.size();++row)
                    if(x.rows[row].type_hash!=y.rows[row].type_hash ||
                       x.rows[row].values!=y.rows[row].values) return false;
            }
        }
    }
    return true;
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,
                                       std::uint32_t id) {
    for(const auto &item:doc.sections) if(item.id==id) return item;
    throw std::runtime_error("Missing full BNW knowledge section");
}
struct Cleanup {
    std::filesystem::path first,second;
    ~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }
};
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source().snapshot;
    auto world=source;
    constexpr std::size_t player=0;
    auto &religion=block(world.all_players->players.at(player),"player_religions");
    auto &prophets=field(religion,"prophets_spawned");
    check(prophets<INT32_MAX,"Selected BNW prophet count at i32 limit");
    ++prophets;
    auto &techs=block(world.all_players->players.at(player),"player_techs");
    auto &weights=hashed(techs,"civ_tech_priority");
    std::size_t chosen=weights.rows.size();
    for(std::size_t i=0;i<weights.rows.size();++i)
        if(weights.rows[i].type_hash!=0 &&
           weights.rows[i].values.size()==1 &&
           weights.rows[i].values[0]<INT32_MAX) {
            chosen=i;break;
        }
    check(chosen<weights.rows.size(),
          "Selected BNW has no mutable technology hash value");
    ++weights.rows[chosen].values[0];
    auto &researching=hashed(techs,"researching_tech");
    std::size_t research_row=researching.rows.size();
    for(std::size_t i=0;i<researching.rows.size();++i)
        if(researching.rows[i].type_hash!=0 &&
           researching.rows[i].values.size()==1) {
            research_row=i;break;
        }
    check(research_row<researching.rows.size(),
          "Selected BNW has no technology research flag");
    researching.rows[research_row].values[0]^=1;
    auto &flavors=block(world.all_players->players.at(player),"flavor_manager");
    auto &active=array(flavors,"active_flavors");
    check(!active.empty() && active.front()<INT32_MAX,
          "Selected BNW has no mutable active flavor");
    ++active.front();
    const auto checkpoint=runtime::make_bnw_player_knowledge_checkpoint(
        world,source,host.identity());
    check(checkpoint.id==runtime::bnw_player_knowledge_checkpoint_section_id,
          "BNW full knowledge section ID changed");
    {
        auto changed=world;
        ++field(block(changed.all_players->players.at(player),
                      "player_techs"),"version");
        rejects([&] {(void)runtime::make_bnw_player_knowledge_checkpoint(
            changed,source,host.identity());});
        changed=world;
        ++hashed(block(changed.all_players->players.at(player),
                        "player_techs"),"civ_tech_priority").rows[chosen].type_hash;
        rejects([&] {(void)runtime::make_bnw_player_knowledge_checkpoint(
            changed,source,host.identity());});
        changed=world;
        array(block(changed.all_players->players.at(player),
                    "flavor_manager"),"active_flavors").push_back(1);
        rejects([&] {(void)runtime::make_bnw_player_knowledge_checkpoint(
            changed,source,host.identity());});
        changed=world;
        ++changed.all_players->players.at(player).saved_player_id;
        rejects([&] {(void)runtime::make_bnw_player_knowledge_checkpoint(
            changed,source,host.identity());});
    }
    runtime::validate_bnw_player_knowledge_composition(
        std::array{saves::CivisSaveSection{checkpoint}});
    rejects([&] {runtime::validate_bnw_player_knowledge_composition(
        std::array{checkpoint,saves::CivisSaveSection{33,0,{1}}});});
    rejects([&] {runtime::validate_bnw_player_knowledge_composition(
        std::array{checkpoint,checkpoint});});
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    Cleanup cleanup{
        std::filesystem::current_path()/("bnw-full-knowledge-"+suffix+"-1.CivisSave"),
        std::filesystem::current_path()/("bnw-full-knowledge-"+suffix+"-2.CivisSave")};
    auto doc=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    doc.sections.push_back(checkpoint);
    saves::write_civis_save(cleanup.first,doc);
    const auto loaded=saves::read_civis_save(cleanup.first);
    runtime::validate_bnw_player_knowledge_composition(loaded.sections);
    {
        const auto imported=saves::read_loaded_save(cleanup.first);
        check(imported.live_bnw_player_knowledge &&
              *imported.live_bnw_player_knowledge==checkpoint.bytes,
              "Normal Civis importer lost ID35");
        const auto reexport=saves::make_civis_import_document(imported,original.sha256);
        check(section(reexport,checkpoint.id).bytes==checkpoint.bytes,
              "Normal Civis re-export lost ID35");
        auto conflict=imported;
        conflict.live_team_research=std::vector<std::uint8_t>{1};
        conflict.live_research_turn=std::vector<std::uint8_t>{1};
        conflict.live_bnw_research_queue=std::vector<std::uint8_t>{1};
        rejects([&] {(void)saves::make_civis_import_document(
            conflict,original.sha256);});
        saves::SavedSessionDecodeContext decode;
        decode.decode_map_plots=false;
        rejects([&] {(void)saves::read_saved_session(cleanup.first,decode);});
        decode.permit_live_bnw_player_knowledge_overlay=true;
        const auto opt_in=saves::read_saved_session(cleanup.first,decode);
        check(opt_in.source.live_bnw_player_knowledge &&
              *opt_in.source.live_bnw_player_knowledge==checkpoint.bytes,
              "Explicit BNW ID35 decode opt-in lost the section");
        auto normal=ui::bootstrap_selected_bnw_preview(cleanup.first,game_root,0);
        check(!normal.preview.host && normal.preview.blocker &&
              normal.preview.blocker->stage==ui::SelectedBnwPreviewStage::live_restore,
              "Normal BNW LiveSession published unsynchronized ID35 raw state");
    }
    auto restored=runtime::restore_bnw_player_knowledge_checkpoint(
        section(loaded,checkpoint.id),source,source,host.identity());
    check(same_knowledge(restored,world),
          "BNW full knowledge round-trip omitted a scalar, array or hash value");
    {
        auto corrupt=section(loaded,checkpoint.id);
        corrupt.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_checkpoint(
            corrupt,source,source,host.identity());});
        auto foreign=host.identity();foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_checkpoint(
            section(loaded,checkpoint.id),source,source,foreign);});
        auto already_changed=world;
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_checkpoint(
            section(loaded,checkpoint.id),already_changed,source,host.identity());});
        check(same_knowledge(source,host.session().source().snapshot),
              "Rejected BNW knowledge checkpoint mutated immutable source");
    }
    const auto again=runtime::make_bnw_player_knowledge_checkpoint(
        restored,source,host.identity());
    auto second=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    second.sections.push_back(again);
    saves::write_civis_save(cleanup.second,second);
    const auto reread=saves::read_civis_save(cleanup.second);
    const auto final=runtime::restore_bnw_player_knowledge_checkpoint(
        section(reread,again.id),source,source,host.identity());
    check(same_knowledge(final,world) &&
          runtime::identify_file(path).sha256==original.sha256 &&
          !host.session().readiness().can_continue(),
          "BNW knowledge re-save changed state, source or readiness");
    std::cout << "PASS BNW full PlayerKnowledge raw checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW full PlayerKnowledge checkpoint: "
                  << error.what() << '\n';return 1;
    }
}
