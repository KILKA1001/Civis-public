#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_game_score.hpp"
#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/runtime/bnw_team_turn_checkpoint.hpp"
#include "civ5/runtime/bnw_live_map_player_city_export.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
#include "civ5/runtime/bnw_research_queue_checkpoint.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace {
using namespace civ5;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t &raw_field(saves::SavedPlayerObservation &player,
                        std::string_view wanted) {
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(auto &field:block.fields)
                if(name(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW treasury field missing");
}
std::int64_t raw_field(const saves::SavedPlayerObservation &player,
                       std::string_view wanted) {
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(const auto &field:block.fields)
                if(name(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW treasury field missing");
}
std::vector<std::int64_t> &raw_history(saves::SavedPlayerObservation &player,
                                        std::string_view wanted) {
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(auto &array:block.arrays)
                if(name(array.name)==wanted) return array.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
const std::vector<std::int64_t> &raw_history(
    const saves::SavedPlayerObservation &player,std::string_view wanted) {
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(const auto &array:block.arrays)
                if(name(array.name)==wanted) return array.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &document,
                                       std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &entry:document.sections) if(entry.id==id) {
        check(!found,"Duplicate composed BNW checkpoint section");found=&entry;
    }
    check(found,"Missing composed BNW checkpoint section");return *found;
}
struct TemporaryFile {
    std::filesystem::path path;
    ~TemporaryFile() {
        std::error_code error;std::filesystem::remove(path,error);
    }
};
std::filesystem::path output_path(std::string_view label) {
    return std::filesystem::current_path()/
        ("bnw-composed-"+std::string(label)+"-"+
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
         ".CivisSave");
}
void unchanged_source(const std::filesystem::path &path,
                      std::string_view sha256) {
    check(runtime::identify_file(path).sha256==sha256,
          "Composed BNW checkpoint modified the original Civ5Save");
}
void verify_roundtrip(const std::filesystem::path &save_path,
                      const saves::CivisSaveDocument &written,
                      std::string_view source_sha256) {
    const auto imported=saves::read_loaded_save(save_path);
    const auto reexport=saves::make_civis_import_document(imported,source_sha256);
    for(const auto &expected:written.sections)
        if(expected.id!=static_cast<std::uint32_t>(
                           saves::CivisSaveSectionId::imported_payload) &&
           expected.id!=static_cast<std::uint32_t>(
                           saves::CivisSaveSectionId::source_metadata)) {
            const auto &again=section(reexport,expected.id);
            check(again.flags==expected.flags && again.bytes==expected.bytes,
                  "Normal BNW re-export changed a composed section");
        }
    TemporaryFile again{output_path("reexport")};
    saves::write_civis_save(again.path,reexport);
    const auto reread=saves::read_civis_save(again.path);
    for(const auto &expected:written.sections)
        if(expected.id!=static_cast<std::uint32_t>(
                           saves::CivisSaveSectionId::imported_payload) &&
           expected.id!=static_cast<std::uint32_t>(
                           saves::CivisSaveSectionId::source_metadata))
            check(section(reread,expected.id).bytes==expected.bytes,
                  "Second physical BNW save changed a composed section");
}
void clock_gold_expense(ui::SelectedBnwPreviewHost &host,
                        const std::filesystem::path &game_root,
                        const std::filesystem::path &original,
                        std::string_view source_sha256) {
    const auto &source=host.session().source().snapshot;
    const auto &baseline=host.session().state();
    auto state=baseline;
    constexpr std::size_t player=0;
    auto &prefix=std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix);
    const auto source_turn=source.pregame->archive.integers.at("game_turn");
    const auto &old_gold=baseline.treasury.players.at(player);
    const auto next_turn=std::max({source_turn+1,
        static_cast<std::int64_t>(old_gold.balance_history_times100.size()+1),
        static_cast<std::int64_t>(old_gold.income_history_times100.size()+1)});
    check(source_turn>=0 && prefix.scalars[1]>=0 &&
          next_turn>source_turn && next_turn<INT32_MAX,
          "Selected Gandhi cannot stage a later BNW gold history turn");
    // Diagnostic source-shaped transition, not a claim that DoGold and all
    // mandatory Lua/deficit/UI phases can execute on this live session.
    prefix.scalars[1]+=next_turn-source_turn;
    state.world.pregame->archive.integers.at("game_turn")=next_turn;
    state.setup.game_turn=next_turn;
    state.maintenance.elapsed_turns=prefix.scalars[1];
    auto &raw=state.world.all_players->players.at(player);
    check(raw_field(raw,"m_iGold")>=0 && raw_field(raw,"m_iGold")<INT32_MAX-100 &&
          raw_field(raw,"m_iLifetimeGrossGoldIncome")>=0 &&
          raw_field(raw,"m_iLifetimeGrossGoldIncome")<INT32_MAX-2,
          "Selected Gandhi gold source exceeds BNW i32 fixture range");
    raw_field(raw,"m_iGold")+=100;
    raw_field(raw,"m_iLifetimeGrossGoldIncome")+=2;
    raw_history(raw,"m_GoldBalanceForTurnTimes100").push_back(
        raw_field(raw,"m_iGold"));
    raw_history(raw,"m_GoldChangeForTurnTimes100").push_back(100);
    ++raw_field(raw,"m_GoldBalanceForTurnTimes100_wire_count");
    ++raw_field(raw,"m_GoldChangeForTurnTimes100_wire_count");
    auto &owned=state.treasury.players.at(player);
    owned.balance_times100+=100;
    owned.lifetime_gross_gold+=2;
    owned.balance_history_times100.push_back(owned.balance_times100);
    owned.income_history_times100.push_back(100);
    const auto clock=runtime::make_bnw_game_clock_checkpoint(
        state,source,host.identity());
    const auto gold=runtime::make_bnw_treasury_balance_checkpoint_pair(
        state,source,baseline.treasury,host.identity());
    check(gold.raw.bytes.at(8)==3,
          "Composed BNW gold checkpoint must be count-aware v3");
    auto &expense=state.maintenance.players.at(player);
    check(expense.cached_unit_expense>=0 &&
          expense.cached_unit_expense<INT32_MAX-1 &&
          expense.cached_supply_expense>=0 &&
          expense.cached_supply_expense<INT32_MAX-2,
          "Selected Gandhi expense source exceeds BNW i32 fixture range");
    ++expense.cached_unit_expense;
    expense.cached_supply_expense+=2;
    const auto cache=runtime::make_bnw_maintenance_expense_checkpoint(
        state,source,baseline.maintenance,host.identity(),&gold.raw);
    auto document=saves::make_civis_import_document(
        host.session().source().source,source_sha256);
    document.sections.insert(document.sections.end(),
        {clock,gold.raw,gold.owned,cache});
    TemporaryFile saved{output_path("clock-gold-expense")};
    saves::write_civis_save(saved.path,document);
    const auto reopened=saves::read_civis_save(saved.path);
    for(const auto &part:{clock,gold.raw,gold.owned,cache})
        check(section(reopened,part.id).bytes==part.bytes,
              "Atomic Civis writer changed a composed BNW section");
    verify_roundtrip(saved.path,document,source_sha256);
    auto preview=ui::bootstrap_selected_bnw_preview(saved.path,game_root,0);
    check(!preview.preview.blocker && preview.preview.host,
          "Selected Gandhi clock/gold/expense loader rejected checkpoint");
    check(!preview.preview.host->session().readiness().can_continue(),
          "Partial BNW clock/gold/expense checkpoint enabled continuation");
    const auto &live=preview.preview.host->session().state();
    const auto &read_raw=live.world.all_players->players.at(player);
    check(live.setup.game_turn==next_turn &&
          live.maintenance.elapsed_turns==prefix.scalars[1] &&
          live.treasury.players.at(player).balance_times100==owned.balance_times100 &&
          live.treasury.players.at(player).lifetime_gross_gold==
              owned.lifetime_gross_gold &&
          live.treasury.players.at(player).balance_history_times100==
              owned.balance_history_times100 &&
          live.treasury.players.at(player).income_history_times100==
              owned.income_history_times100 &&
          live.maintenance.players.at(player).cached_unit_expense==
              expense.cached_unit_expense &&
          live.maintenance.players.at(player).cached_supply_expense==
              expense.cached_supply_expense &&
          raw_field(read_raw,"m_iExpensePerTurnUnitMaintenance")==
              expense.cached_unit_expense &&
          raw_field(read_raw,"m_iExpensePerTurnUnitSupply")==
              expense.cached_supply_expense &&
          raw_history(read_raw,"m_GoldBalanceForTurnTimes100")==
              owned.balance_history_times100,
          "Normal BNW loader lost the combined clock, gold or expense state");
    unchanged_source(original,source_sha256);
}
void clock_unchanged_world(ui::SelectedBnwPreviewHost &host,
                           const std::filesystem::path &game_root,
                           const std::filesystem::path &original,
                           std::string_view source_sha256) {
    const auto frame=host.preview_input().frame_sources;
    check(frame.database,"Selected BNW active rules unavailable");
    const auto &rules=*frame.database;
    const auto types=rules.cache_info_types();
    const auto defines=rules.cache_global_defines(
        content::GlobalDefineProfile::bnw);
    const auto features=content::saved_terrain_features(rules,types).resolve;
    auto state=host.session().state();
    auto &prefix=std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix);
    const auto next_turn=state.setup.game_turn+1;
    check(next_turn>state.setup.game_turn && prefix.scalars[1]>=0,
          "Selected BNW clock-only turn exceeds source range");
    state.setup.game_turn=next_turn;
    state.world.pregame->archive.integers.at("game_turn")=next_turn;
    ++state.maintenance.elapsed_turns;
    ++prefix.scalars[1];
    check(prefix.scalars[23]<INT32_MAX && state.world.supporting_classes &&
          !state.world.supporting_classes->barbarians.empty() &&
          !state.world.supporting_classes->goody_hut_results.empty() &&
          state.world.supporting_classes->goody_hut_results[0][0]<INT32_MAX,
          "Selected BNW supporting checkpoint fixture unavailable");
    ++prefix.scalars[23];
    state.world.supporting_classes->barbarians[0].counter=-2;
    ++state.world.supporting_classes->goody_hut_results[0][0];
    bool found_score_dirty=false;
    for(auto &flag:state.world.game_state.flags)
        if(flag.name && std::string_view(flag.name)=="score_dirty") {
            flag.value=true;
            found_score_dirty=true;
        }
    check(found_score_dirty,"Selected BNW score dirty flag unavailable");
    runtime::update_bnw_game_score(state,rules,types,defines);
    const auto scored_roster=state.world.game_state.roster->arrays;
    (void)state.random.gameplay.draw(17);
    const auto expected_rng=state.random.gameplay.state();
    const auto exported=runtime::make_bnw_live_turn_base_partial_export(
        state,host.session().source(),host.identity(),source_sha256,
        features,rules,host.technologies(),types,defines);
    const auto score_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_bnw_game_score_replay);
    const auto score_bytes=section(exported.document,score_id).bytes;
    const auto deals_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_bnw_game_deals);
    const auto deals_bytes=section(exported.document,deals_id).bytes;
    const auto team_turn_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_bnw_team_turn);
    const auto team_turn_bytes=section(exported.document,team_turn_id).bytes;
    TemporaryFile saved{output_path("clock-unchanged-world")};
    saves::write_civis_save(saved.path,exported.document);
    verify_roundtrip(saved.path,exported.document,source_sha256);
    auto preview=ui::bootstrap_selected_bnw_preview(saved.path,game_root,0);
    check(!preview.preview.blocker && preview.preview.host,
          "Normal BNW loader rejected clock with unchanged player/city trees");
    const auto &live=preview.preview.host->session().state();
    check(live.setup.game_turn==next_turn &&
          live.maintenance.elapsed_turns==state.maintenance.elapsed_turns &&
          live.random.gameplay.state()==expected_rng &&
          std::get<saves::Expansion2PrefixObservation>(
              live.world.game_state.prefix).scalars[23]==prefix.scalars[23] &&
          live.world.supporting_classes->barbarians[0].counter==-2 &&
          live.world.supporting_classes->goody_hut_results[0][0]==
              state.world.supporting_classes->goody_hut_results[0][0] &&
          live.world.game_state.roster->arrays==scored_roster &&
          runtime::make_bnw_game_score_checkpoint(
              live.world,host.session().source().snapshot,
              host.identity()).bytes==score_bytes &&
          runtime::make_bnw_game_deals_checkpoint(
              live.world,host.session().source().snapshot,
              host.identity()).bytes==deals_bytes &&
          runtime::make_bnw_team_turn_checkpoint(
              live.world,host.session().source().snapshot,
              host.identity()).bytes==team_turn_bytes &&
          !preview.preview.host->session().readiness().can_continue(),
          "Normal BNW loader lost clock/RNG/supporting save or overstated readiness");
    auto next=live;
    auto &next_prefix=std::get<saves::Expansion2PrefixObservation>(
        next.world.game_state.prefix);
    ++next.setup.game_turn;
    ++next.world.pregame->archive.integers.at("game_turn");
    ++next.maintenance.elapsed_turns;
    ++next_prefix.scalars[1];
    ++next_prefix.scalars[23];
    ++next.world.supporting_classes->barbarians[0].counter;
    auto regressed=next;
    regressed.random.gameplay=
        runtime::restore_session_random(host.session().source().snapshot.progression)
            .gameplay;
    bool rejected=false;
    try {
        (void)runtime::make_bnw_live_turn_base_partial_export(
            regressed,preview.preview.host->session().source(),
            preview.preview.host->identity(),source_sha256,
            features,rules,preview.preview.host->technologies(),types,defines);
    } catch(const std::exception &) {rejected=true;}
    check(rejected,"BNW turn export accepted RNG regression from prior save");
    const auto second_export=runtime::make_bnw_live_turn_base_partial_export(
        next,preview.preview.host->session().source(),
        preview.preview.host->identity(),source_sha256,
        features,rules,preview.preview.host->technologies(),types,defines);
    TemporaryFile second{output_path("clock-rng-no-draw")};
    saves::write_civis_save(second.path,second_export.document);
    auto again=ui::bootstrap_selected_bnw_preview(second.path,game_root,0);
    check(!again.preview.blocker && again.preview.host &&
          again.preview.host->session().state().setup.game_turn==next.setup.game_turn &&
          again.preview.host->session().state().random.gameplay.state()==expected_rng &&
          again.preview.host->session().state().world.supporting_classes->
              barbarians[0].counter==-1 &&
          std::get<saves::Expansion2PrefixObservation>(
              again.preview.host->session().state().world.game_state.prefix).
              scalars[23]==next_prefix.scalars[23],
          "Normal BNW loader lost no-draw RNG or supporting state on second save");
    unchanged_source(original,source_sha256);
}
void research_expense(ui::SelectedBnwPreviewHost &host,
                      const std::filesystem::path &game_root,
                      const std::filesystem::path &original,
                      std::string_view source_sha256) {
    const auto &source=host.session().source().snapshot;
    const auto &baseline=host.session().state();
    const auto frame=host.preview_input().frame_sources;
    check(frame.database && frame.technologies,
          "Selected BNW technology rules unavailable");
    const auto types=frame.database->cache_info_types();
    const auto &rules=*frame.technologies;
    auto state=baseline;
    constexpr std::size_t player=0;
    check(state.player_research.players.at(player).queue.empty() &&
          state.research_turn.players.at(player).researching.empty(),
          "Selected Gandhi research fixture requires empty queue");
    const auto &saved=state.world.all_players->players.at(player);
    const saves::TeamHashedArray *rows=nullptr;
    for(const auto &block:saved.knowledge.blocks)
        if(name(block.name)=="player_techs")
            for(const auto &array:block.hashes)
                if(name(array.name)=="researching_tech") rows=&array;
    check(rows,"Selected Gandhi researching hash rows unavailable");
    std::int64_t technology=-1;
    for(const auto &row:rows->rows) {
        if(row.type_hash==0 || row.values.size()!=1 || row.values[0]!=0)
            continue;
        const auto *type=types.find_hash(row.type_hash);
        if(type && type->id>=0 &&
           rules.find(static_cast<std::uint64_t>(type->id))) {
            technology=type->id;break;
        }
    }
    check(technology>=0,"Selected Gandhi technology flag unavailable");
    state.player_research.players.at(player).queue={technology};
    state.research_turn.players.at(player).researching.insert(technology);
    auto &expense=state.maintenance.players.at(player);
    check(expense.cached_unit_expense>=0 &&
          expense.cached_unit_expense<INT32_MAX,
          "Selected Gandhi expense cache exceeds i32 fixture range");
    ++expense.cached_unit_expense;
    const auto queue=runtime::make_bnw_research_queue_checkpoint_bundle(
        state,baseline,source,rules,types,host.identity());
    const auto cache=runtime::make_bnw_maintenance_expense_checkpoint(
        state,source,baseline.maintenance,host.identity());
    auto document=saves::make_civis_import_document(
        host.session().source().source,source_sha256);
    document.sections.insert(document.sections.end(),
        {queue.raw,queue.research,queue.research_turn,cache});
    TemporaryFile saved_file{output_path("research-expense")};
    saves::write_civis_save(saved_file.path,document);
    verify_roundtrip(saved_file.path,document,source_sha256);
    auto preview=ui::bootstrap_selected_bnw_preview(saved_file.path,game_root,0);
    check(!preview.preview.blocker && preview.preview.host,
          "Selected Gandhi research/expense loader rejected checkpoint");
    check(!preview.preview.host->session().readiness().can_continue(),
          "Partial BNW research/expense checkpoint enabled continuation");
    const auto &live=preview.preview.host->session().state();
    check(live.player_research.players.at(player).queue==
              std::vector<std::int64_t>{technology} &&
          live.research_turn.players.at(player).researching.contains(technology) &&
          live.maintenance.players.at(player).cached_unit_expense==
              expense.cached_unit_expense &&
          raw_field(live.world.all_players->players.at(player),
                    "m_iExpensePerTurnUnitMaintenance")==
              expense.cached_unit_expense,
          "Normal BNW loader lost research or expense composition");
    unchanged_source(original,source_sha256);
}
void batch_moves_normal_loader(ui::SelectedBnwPreviewHost &host,
                               const std::filesystem::path &game_root,
                               const std::filesystem::path &original,
                               std::string_view source_sha256) {
    const auto &source=host.session().source().snapshot;
    auto changed=source;
    auto scalar=[](saves::SavedUnitObservation &unit,std::string_view wanted)->std::int64_t & {
        saves::UnitCoreField *found=nullptr;
        for(auto &field:unit.core.fields)if(name(field.name)==wanted) {
            check(!found,"Duplicate loader unit scalar");found=&field;
        }
        check(found && found->encoding==saves::UnitCoreEncoding::i32 && found->values.size()==1,
              "Loader native unit scalar missing");return found->values[0];
    };
    std::vector<std::pair<std::size_t,std::size_t>> picked;
    // Real imported rows, with detached diagnostic target values. This does
    // not claim that FinishMoves or an incomplete move Pending has completed.
    for(std::size_t p=0;p<changed.all_players->players.size() && picked.size()<2;++p)
        for(std::size_t u=0;u<changed.all_players->players[p].units.units.size() && picked.size()<2;++u) {
            auto &moves=scalar(changed.all_players->players[p].units.units[u],"m_iMoves");
            moves=picked.empty()?(moves==INT32_MIN?INT32_MAX:INT32_MIN):(moves==-1?0:-1);
            picked.emplace_back(p,u);
        }
    check(picked.size()==2,"Normal selected loader needs two actual unit operands");
    const auto frame=host.preview_input().frame_sources;
    check(frame.database,"Normal selected loader has no rule owner");
    const auto types=frame.database->cache_info_types();
    const auto defines=frame.database->cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto features=content::saved_terrain_features(*frame.database,types);
    const auto pair=runtime::make_bnw_map_unit_moves_checkpoint_pair(
        changed,source,host.identity(),features.resolve,frame.database,&types,&defines);
    auto document=saves::make_civis_import_document(host.session().source().source,source_sha256);
    document.sections.push_back(pair.map);document.sections.push_back(pair.units);
    TemporaryFile saved{output_path("batch-moves")};saves::write_civis_save(saved.path,document);
    verify_roundtrip(saved.path,document,source_sha256);
    auto preview=ui::bootstrap_selected_bnw_preview(saved.path,game_root,0,true);
    check(!preview.preview.blocker && preview.preview.host,"Actual LiveSession loader rejected V3 batch");
    check(!preview.preview.host->session().readiness().can_continue(),"Moves-only codec invented full continuation");
    const auto &restored=preview.preview.host->session().state().world;
    for(const auto [p,u]:picked) {
        auto actual=restored.all_players->players[p].units.units[u];
        auto expected=changed.all_players->players[p].units.units[u];
        for(auto field:{"m_iID","m_eOwner","m_iX","m_iY","m_iMoves"})
            check(scalar(actual,field)==scalar(expected,field),"LiveSession restore lost native batch field");
    }
    unchanged_source(original,source_sha256);
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        const std::filesystem::path selected=argv[1],game_root=argv[2];
        const auto source=runtime::identify_file(selected);
        auto bootstrap=ui::bootstrap_selected_bnw_preview(selected,game_root,0);
        check(!bootstrap.preview.blocker && bootstrap.preview.host,
              "Selected Gandhi BNW preview unavailable");
        clock_gold_expense(*bootstrap.preview.host,game_root,selected,source.sha256);
        clock_unchanged_world(*bootstrap.preview.host,game_root,selected,source.sha256);
        research_expense(*bootstrap.preview.host,game_root,selected,source.sha256);
        batch_moves_normal_loader(*bootstrap.preview.host,game_root,selected,source.sha256);
        unchanged_source(selected,source.sha256);
        std::cout << "PASS BNW composed checkpoint normal loader\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW composed checkpoint loader: "
                  << error.what() << '\n';
        return 1;
    }
}
