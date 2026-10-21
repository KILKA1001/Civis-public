#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_player_gold.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
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
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f,const char *why) {
    bool failed=false;try{f();}catch(const std::exception &){failed=true;}
    check(failed,why);
}
std::int64_t &gold(saves::SavedSessionSnapshot &world,std::string_view key) {
    for(auto &block:world.all_players->players.at(0).administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(auto &field:block.fields)
                if(field.name && std::string_view(field.name)==key) return field.value;
    throw std::runtime_error("Actual selected treasury scalar missing");
}
std::vector<std::int64_t> &history(saves::SavedSessionSnapshot &world,std::string_view key) {
    for(auto &block:world.all_players->players.at(0).administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(auto &array:block.arrays)
                if(array.name && std::string_view(array.name)==key) return array.values;
    throw std::runtime_error("Actual selected treasury history missing");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:doc.sections) if(part.id==id) {
        check(!found,"Duplicate treasury section after composition");found=&part;
    }
    check(found,"Treasury section omitted by actual turn-base caller");return *found;
}
struct TemporaryFile {
    std::filesystem::path path;
    ~TemporaryFile(){std::error_code error;std::filesystem::remove(path,error);}
};
TemporaryFile temporary(std::string_view label) {
    return {std::filesystem::current_path()/("bnw-treasury-export-"+std::string(label)+"-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave")};
}
void advance_diagnostic_clock(runtime::LiveSessionState &state) {
    // Detached source-shaped codec fixture. This is deliberately not an
    // executed CvGame::doTurn or an acknowledgement of its engine effects.
    check(state.setup.game_turn<INT32_MAX,"Fixture game turn overflow");
    ++state.setup.game_turn;
    ++state.world.pregame->archive.integers.at("game_turn");
    ++state.maintenance.elapsed_turns;
    ++std::get<saves::Expansion2PrefixObservation>(state.world.game_state.prefix).scalars[1];
}
void diagnostic_balance(runtime::LiveSessionState &state) {
    check(gold(state.world,"m_iGold")>=0 && gold(state.world,"m_iGold")<INT32_MAX-100,
          "Fixture treasury balance out of range");
    gold(state.world,"m_iGold")+=100;
    state.treasury.players.at(0).balance_times100+=100;
}
void diagnostic_expenses(runtime::LiveSessionState &state) {
    auto &cache=state.maintenance.players.at(0);
    check(cache.cached_unit_expense>=0 && cache.cached_unit_expense<INT32_MAX-1 &&
          cache.cached_supply_expense>=0 && cache.cached_supply_expense<INT32_MAX-2,
          "Diagnostic expense overflow");
    ++cache.cached_unit_expense;cache.cached_supply_expense+=2;
    gold(state.world,"m_iExpensePerTurnUnitMaintenance")=cache.cached_unit_expense;
    gold(state.world,"m_iExpensePerTurnUnitSupply")=cache.cached_supply_expense;
}
void expenses_match(runtime::LiveSessionState &state,const runtime::LiveSessionState &expected) {
    check(state.maintenance.players.at(0).cached_unit_expense==expected.maintenance.players.at(0).cached_unit_expense &&
          state.maintenance.players.at(0).cached_supply_expense==expected.maintenance.players.at(0).cached_supply_expense &&
          gold(state.world,"m_iExpensePerTurnUnitMaintenance")==expected.maintenance.players.at(0).cached_unit_expense &&
          gold(state.world,"m_iExpensePerTurnUnitSupply")==expected.maintenance.players.at(0).cached_supply_expense,
          "Actual combined loader lost raw/owned expense caches");
}
void selected(const std::filesystem::path &path,const std::filesystem::path &assets) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,assets,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,"Actual BNW source bootstrap blocked");
    auto &host=*bootstrap.preview.host;
    const auto frame=host.preview_input().frame_sources;
    check(frame.database,"Actual BNW rule owner absent");
    const auto &rules=*frame.database;
    const auto types=rules.cache_info_types();
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto features=content::saved_terrain_features(rules,types).resolve;
    const auto baseline=host.session().state();
    const auto &source=host.session().source();
    const auto identity=host.identity();
    const auto export_state=[&](const auto &state,const auto &input,const auto &id) {
        return runtime::make_bnw_live_turn_base_partial_export(state,input,id,
            original.sha256,features,rules,host.technologies(),types,defines);
    };
    // Exercise the real DoGold entry point honestly: its actual VM supplier
    // is absent. No lua_ready=true lambda, no fake founder query or dirty ACK.
    auto missing=baseline;
    const auto treasury_rules=runtime::load_treasury_rules(rules,content::TechnologyProfile::bnw,types);
    const auto city_rules=runtime::load_city_yield_rules(rules,content::TechnologyProfile::bnw);
    const auto maintenance_rules=runtime::load_maintenance_rules(rules,content::TechnologyProfile::bnw,types);
    const auto before=runtime::make_treasury_checkpoint(missing.treasury,missing.setup,identity).bytes;
    const auto rejected=runtime::run_bnw_player_gold_phase(missing,0,
        {treasury_rules,city_rules,maintenance_rules,defines,0},{});
    check(rejected.outcome==runtime::LiveActionOutcome::rejected &&
          runtime::make_treasury_checkpoint(missing.treasury,missing.setup,identity).bytes==before &&
          !missing.player_gold_publication,"Missing genuine Lua supplier mutated or accepted DoGold");

    auto state=baseline;advance_diagnostic_clock(state);diagnostic_balance(state);diagnostic_expenses(state);
    const auto exported=export_state(state,source,identity);
    constexpr auto owned_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury);
    constexpr auto raw_id=runtime::bnw_treasury_balance_checkpoint_section_id;
    check(!exported.full_persistence && !exported.uncovered_domains.empty(),
          "Treasury export claimed full persistence");
    auto gold_projection=state;
    for(const auto key:{"m_iExpensePerTurnUnitMaintenance","m_iExpensePerTurnUnitSupply"})
        gold(gold_projection.world,key)=gold(gold_projection.world,key)-
            (key==std::string_view("m_iExpensePerTurnUnitMaintenance")?1:2);
    check(section(exported.document,runtime::bnw_maintenance_expense_checkpoint_section_id).bytes.at(8)==2,
          "Actual caller omitted count-aware expenses");
    check(section(exported.document,raw_id).bytes==
          runtime::make_bnw_treasury_balance_checkpoint_pair(gold_projection,source.snapshot,
              baseline.treasury,identity).raw.bytes,"Actual turn-base raw treasury stale");
    check(section(exported.document,owned_id).bytes==
          runtime::make_treasury_checkpoint(state.treasury,state.setup,identity).bytes,
          "Actual turn-base owned treasury stale");
    {
        auto bad=state;bad.player_gold_publication.emplace();
        rejects([&]{(void)export_state(bad,source,identity);},"Pending gold effects were saved");
        bad=state;bad.game_clock.pending.emplace();
        rejects([&]{(void)export_state(bad,source,identity);},"Pending clock effects were saved");
        bad=state;++bad.treasury.players.at(0).balance_times100;
        rejects([&]{(void)export_state(bad,source,identity);},"Owned-only mutation was saved");
        bad=state;++gold(bad.world,"m_iGold");
        rejects([&]{(void)export_state(bad,source,identity);},"Raw-only mutation was saved");
        bad=state;++bad.world.all_players->players.at(0).administration.start_offset;
        rejects([&]{(void)export_state(bad,source,identity);},"World identity mismatch accepted");
        auto orphan=source;orphan.source.live_treasury=section(exported.document,owned_id).bytes;
        rejects([&]{(void)export_state(state,orphan,identity);},"Orphan ID10 was saved");
        orphan=source;orphan.source.live_bnw_treasury_balance=section(exported.document,raw_id).bytes;
        rejects([&]{(void)export_state(state,orphan,identity);},"Orphan ID30 was saved");
        bad=state;++bad.maintenance.players.at(0).cached_unit_expense;
        rejects([&]{(void)export_state(bad,source,identity);},"Owned-only expenses saved");
        bad=state;++gold(bad.world,"m_iExpensePerTurnUnitSupply");
        rejects([&]{(void)export_state(bad,source,identity);},"Raw-only expenses saved");
        auto wrong=identity;wrong.rules_sha256=std::string(64,'a');
        rejects([&]{(void)export_state(state,source,wrong);},"Rule mismatch accepted");
        wrong=identity;wrong.payload_sha256=std::string(64,'b');
        rejects([&]{(void)export_state(state,source,wrong);},"Payload mismatch accepted");
    }
    auto first=temporary("diagnostic-first");saves::write_civis_save(first.path,exported.document);
    auto resumed=ui::bootstrap_selected_bnw_preview(first.path,assets,0);
    check(!resumed.preview.blocker && resumed.preview.host,"Actual paired treasury loader blocked");
    check(!resumed.preview.host->session().readiness().can_continue(),"Treasury loader enabled full continuation");
    auto next=resumed.preview.host->session().state();
    check(next.treasury.players.at(0).balance_times100==state.treasury.players.at(0).balance_times100 &&
          gold(next.world,"m_iGold")==state.treasury.players.at(0).balance_times100,
          "Actual loader lost paired balance");
    expenses_match(next,state);
    advance_diagnostic_clock(next);diagnostic_balance(next);diagnostic_expenses(next);
    const auto &prior=resumed.preview.host->session().source();
    auto damaged=prior;damaged.source.live_bnw_treasury_balance->back()^=1;
    rejects([&]{(void)export_state(next,damaged,identity);},"Damaged previous raw pair accepted");
    auto damaged_expense=prior;damaged_expense.source.live_bnw_maintenance_expense->back()^=1;
    rejects([&]{(void)export_state(next,damaged_expense,identity);},"Damaged previous ID34 accepted");
    const auto second_export=export_state(next,prior,identity);
    auto second=temporary("diagnostic-second");saves::write_civis_save(second.path,second_export.document);
    auto again=ui::bootstrap_selected_bnw_preview(second.path,assets,0);
    check(!again.preview.blocker && again.preview.host &&
          again.preview.host->session().state().treasury.players.at(0).balance_times100==
              next.treasury.players.at(0).balance_times100 &&
          !again.preview.host->session().readiness().can_continue(),
          "Second actual save/reload retained stale treasury or enabled continuation");
    auto second_state=again.preview.host->session().state();expenses_match(second_state,next);
    // Construct one source-shaped history sample only to test malicious
    // regressions against authenticated previous v3. Never claim DoGold ran.
    auto historical=state;
    const auto needed=std::max(historical.treasury.players.at(0).balance_history_times100.size(),
                               historical.treasury.players.at(0).income_history_times100.size());
    while(static_cast<std::uint64_t>(historical.setup.game_turn)<=needed)
        advance_diagnostic_clock(historical);
    ++gold(historical.world,"m_iLifetimeGrossGoldIncome");
    ++historical.treasury.players.at(0).lifetime_gross_gold;
    history(historical.world,"m_GoldBalanceForTurnTimes100").push_back(gold(historical.world,"m_iGold"));
    history(historical.world,"m_GoldChangeForTurnTimes100").push_back(100);
    ++gold(historical.world,"m_GoldBalanceForTurnTimes100_wire_count");
    ++gold(historical.world,"m_GoldChangeForTurnTimes100_wire_count");
    historical.treasury.players.at(0).balance_history_times100.push_back(gold(historical.world,"m_iGold"));
    historical.treasury.players.at(0).income_history_times100.push_back(100);
    const auto history_export=export_state(historical,source,identity);
    auto history_file=temporary("diagnostic-history");saves::write_civis_save(history_file.path,history_export.document);
    auto history_host=ui::bootstrap_selected_bnw_preview(history_file.path,assets,0);
    check(!history_host.preview.blocker && history_host.preview.host,"v3 combined actual reload blocked");
    auto historical_loaded=history_host.preview.host->session().state();expenses_match(historical_loaded,historical);
    check(gold(historical_loaded.world,"m_GoldBalanceForTurnTimes100_wire_count")==
              static_cast<std::int64_t>(historical_loaded.treasury.players.at(0).balance_history_times100.size()),
          "Combined history/expense restart lost native count");
    const auto &history_source=history_host.preview.host->session().source();
    auto regressed=history_host.preview.host->session().state();
    history(regressed.world,"m_GoldBalanceForTurnTimes100").pop_back();
    --gold(regressed.world,"m_GoldBalanceForTurnTimes100_wire_count");
    regressed.treasury.players.at(0).balance_history_times100.pop_back();
    rejects([&]{(void)export_state(regressed,history_source,identity);},"Saved history truncation accepted");
    regressed=history_host.preview.host->session().state();
    --gold(regressed.world,"m_iLifetimeGrossGoldIncome");
    --regressed.treasury.players.at(0).lifetime_gross_gold;
    rejects([&]{(void)export_state(regressed,history_source,identity);},"Saved lifetime regression accepted");
    {
        auto expense_only=baseline;advance_diagnostic_clock(expense_only);diagnostic_expenses(expense_only);
        const auto document=export_state(expense_only,source,identity);
        check(std::none_of(document.document.sections.begin(),document.document.sections.end(),[](const auto &part){
                  return part.id==raw_id || part.id==owned_id;}),"Expense-only writer invented gold pair");
        auto file=temporary("expense-only");saves::write_civis_save(file.path,document.document);
        auto loaded=ui::bootstrap_selected_bnw_preview(file.path,assets,0);
        check(!loaded.preview.blocker && loaded.preview.host,"Expense-only composer restart blocked");
        auto restored=loaded.preview.host->session().state();expenses_match(restored,expense_only);
    }
    check(runtime::identify_file(path).sha256==original.sha256,"Original Civ5Save overwritten");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply BNW save and canonical local asset root");
        selected(argv[1],argv[2]);
        std::cout<<"PASS treasury composer diagnostic restarts and real missing-service rejection; DoGold positive unverified\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr<<"FAIL treasury composer: "<<error.what()<<'\n';return 1;
    }
}
