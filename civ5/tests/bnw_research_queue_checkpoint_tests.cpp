#include "civ5/runtime/bnw_research_queue_checkpoint.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
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
std::int64_t &count(saves::PlayerAdministrationRecord &block) {
    for(auto &field:block.fields)
        if(name(field.name)=="m_researchQueue_wire_count") return field.value;
    throw std::runtime_error("Selected BNW research queue count unavailable");
}
std::int64_t &pre_field(saves::PlayerAdministrationRecord &block,
                        std::string_view wanted) {
    for(auto &field:block.fields)
        if(name(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW pre-city field unavailable");
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
std::int64_t &treasury_balance(saves::SavedPlayerObservation &player) {
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(auto &field:block.fields)
                if(name(field.name)=="m_iGold") return field.value;
    throw std::runtime_error("Selected BNW treasury balance unavailable");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,
                                       std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:doc.sections) if(part.id==id) {
        check(!found,"Duplicate BNW research choice section");found=&part;
    }
    check(found,"Missing BNW research choice section");return *found;
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto frame=host.preview_input().frame_sources;
    check(frame.database && frame.technologies,
          "Selected BNW activated technology rules unavailable");
    const auto &rules=*frame.technologies;
    const auto types=frame.database->cache_info_types();
    const auto &source=host.session().source().snapshot;
    const auto &baseline=host.session().state();
    auto state=baseline;
    constexpr std::size_t player=0;
    auto &raw=state.world.all_players->players.at(player);
    auto &pre=pre_city(raw);
    auto &rows=flags(techs(raw)).rows;
    check(queue(pre).empty() &&
          baseline.player_research.players.at(player).queue.empty() &&
          baseline.research_turn.players.at(player).researching.empty(),
          "Selected Gandhi research choice fixture requires empty queue");
    std::int64_t chosen=-1;std::size_t chosen_row=0;
    for(std::size_t row=0;row<rows.size();++row) {
        if(rows[row].type_hash==0 || rows[row].values.size()!=1 ||
           rows[row].values[0]!=0) continue;
        const auto *type=types.find_hash(rows[row].type_hash);
        if(!type || type->id<0 ||
           !rules.find(static_cast<std::uint64_t>(type->id))) continue;
        chosen=type->id;chosen_row=row;break;
    }
    check(chosen>=0,"Selected BNW has no mapped researching flag row");
    // push_bnw_research_choice mutates the owned contexts; the raw import is
    // deliberately still at its source values until the export projection.
    state.player_research.players.at(player).queue={chosen};
    state.research_turn.players.at(player).researching.insert(chosen);
    check(queue(pre).empty() && rows[chosen_row].values[0]==0,
          "BNW research action unexpectedly mutated raw import");
    const auto bundle=runtime::make_bnw_research_queue_checkpoint_bundle(
        state,baseline,source,rules,types,host.identity());
    check(bundle.raw.id==runtime::bnw_research_queue_checkpoint_section_id &&
          bundle.research.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_team_research) &&
          bundle.research_turn.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_research_turn),
          "BNW research choice companion IDs changed");
    {
        rows[chosen_row].values[0]=2;
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        rows[chosen_row].values[0]=0;
        rows[chosen_row].values[0]=1;
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        rows[chosen_row].values[0]=0;
        ++count(pre);
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        --count(pre);
        ++state.player_research.players.at(player).overflow_times100;
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        --state.player_research.players.at(player).overflow_times100;
        state.research_turn.players.at(player).researching.clear();
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        state.research_turn.players.at(player).researching.insert(chosen);
        auto &field=pre_field(pre,"m_eEndTurnBlockingType");
        ++field;
        rejects([&] {(void)runtime::make_bnw_research_queue_checkpoint_bundle(
            state,baseline,source,rules,types,host.identity());});
        --field;
    }
    // ID30 owns treasury, so a separately validated treasury mutation must
    // not collide with ID33's pre-city/knowledge source-bound slice.
    {
        const auto old=treasury_balance(raw);
        check(old>=0 && old<INT32_MAX,"Selected BNW treasury test balance invalid");
        treasury_balance(raw)=old+1;
        const auto gold=[&] {
            auto treasury_only=source;
            ++treasury_balance(treasury_only.all_players->players.at(player));
            return runtime::make_bnw_treasury_balance_checkpoint(
                treasury_only,source,host.identity());
        }();
        auto projected=source;
        runtime::restore_bnw_research_queue_checkpoint(
            bundle.raw,projected,source,host.identity());
        ++treasury_balance(projected.all_players->players.at(player));
        const auto research=runtime::make_bnw_research_queue_checkpoint(
            projected,source,host.identity());
        check(gold.id==runtime::bnw_treasury_balance_checkpoint_section_id &&
              research.id==runtime::bnw_research_queue_checkpoint_section_id,
              "BNW treasury and research raw sections overlap");
        auto composed=source;
        runtime::restore_bnw_treasury_balance_checkpoint(
            gold,composed,source,host.identity());
        runtime::restore_bnw_research_queue_checkpoint(
            research,composed,source,host.identity());
        check(treasury_balance(composed.all_players->players.at(player))==old+1 &&
              queue(pre_city(composed.all_players->players.at(player)))==
                  std::vector<std::int64_t>{chosen},
              "BNW ID30 then ID33 detached restore lost a domain");
        treasury_balance(raw)=old;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first_path=std::filesystem::current_path()/
        ("bnw-research-choice-"+suffix+"-1.CivisSave");
    const auto second_path=std::filesystem::current_path()/
        ("bnw-research-choice-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first_path,second_path};
    auto doc=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    doc.sections.push_back(bundle.raw);doc.sections.push_back(bundle.research);
    doc.sections.push_back(bundle.research_turn);
    saves::write_civis_save(first_path,doc);
    const auto reopened=saves::read_civis_save(first_path);
    const runtime::BnwResearchQueueCheckpointBundle loaded{
        section(reopened,bundle.raw.id),section(reopened,bundle.research.id),
        section(reopened,bundle.research_turn.id)};
    auto live_import=ui::bootstrap_selected_bnw_preview(first_path,game_root,0);
    check(!live_import.preview.blocker && live_import.preview.host,
          "Normal BNW loader rejected ID33 with its ID8 and ID11 companions");
    const auto &live=live_import.preview.host->session();
    check(live.state().player_research.players.at(player).queue==
              std::vector<std::int64_t>{chosen} &&
          live.state().research_turn.players.at(player).researching.contains(chosen) &&
          queue(pre_city(live.state().world.all_players->players.at(player)))==
              std::vector<std::int64_t>{chosen} &&
          flags(techs(live.state().world.all_players->players.at(player))).rows[
              chosen_row].values[0]==1 &&
          !live.readiness().can_continue(),
          "Normal BNW loader lost raw or owned research choice state");
    const auto imported=saves::read_loaded_save(first_path);
    const auto reexport=saves::make_civis_import_document(imported,original.sha256);
    check(section(reexport,bundle.raw.id).bytes==bundle.raw.bytes &&
          section(reexport,bundle.research.id).bytes==bundle.research.bytes &&
          section(reexport,bundle.research_turn.id).bytes==bundle.research_turn.bytes,
          "Normal BNW re-export lost an ID33 companion");
    auto restored=runtime::restore_bnw_research_queue_checkpoint_bundle(
        loaded,baseline,source,rules,types,host.identity());
    check(queue(pre_city(restored.world.all_players->players.at(player)))==
              std::vector<std::int64_t>{chosen} &&
          restored.players.players.at(player).queue==
              std::vector<std::int64_t>{chosen} &&
          restored.turn.players.at(player).researching.contains(chosen) &&
          flags(techs(restored.world.all_players->players.at(player))).rows[
              chosen_row].values[0]==1,
          "BNW research choice raw/owned round-trip diverged");
    {
        auto corrupt=loaded;corrupt.raw.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_research_queue_checkpoint_bundle(
            corrupt,baseline,source,rules,types,host.identity());});
        auto untouched=source;
        rejects([&] {runtime::restore_bnw_research_queue_checkpoint(
            corrupt.raw,untouched,source,host.identity());});
        check(queue(pre_city(untouched.all_players->players.at(player))).empty() &&
              flags(techs(untouched.all_players->players.at(player))).rows[
                  chosen_row].values[0]==0,
              "Rejected BNW research choice partially published raw state");
        corrupt=loaded;corrupt.research_turn.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_research_queue_checkpoint_bundle(
            corrupt,baseline,source,rules,types,host.identity());});
        auto foreign=host.identity();foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_research_queue_checkpoint_bundle(
            loaded,baseline,source,rules,types,foreign);});
    }
    state.world=std::move(restored.world);
    state.technologies=std::move(restored.technologies);
    state.player_research=std::move(restored.players);
    state.research_turn=std::move(restored.turn);
    const auto again=runtime::make_bnw_research_queue_checkpoint_bundle(
        state,baseline,source,rules,types,host.identity());
    auto next=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    next.sections.push_back(again.raw);next.sections.push_back(again.research);
    next.sections.push_back(again.research_turn);
    saves::write_civis_save(second_path,next);
    const auto reread=saves::read_civis_save(second_path);
    const auto final=runtime::restore_bnw_research_queue_checkpoint_bundle(
        {section(reread,again.raw.id),section(reread,again.research.id),
         section(reread,again.research_turn.id)},
        baseline,source,rules,types,host.identity());
    check(final.players.players.at(player).queue==
              std::vector<std::int64_t>{chosen} &&
          final.turn.players.at(player).researching.contains(chosen) &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW research choice re-save or original Civ5Save changed");
    std::cout << "PASS BNW research choice raw/owned checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW research choice checkpoint: "
                  << error.what() << '\n';return 1;
    }
}
