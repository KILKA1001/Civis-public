#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "bnw_maintenance_legacy_v1_fixture.hpp"
#include "bnw_treasury_legacy_v2_fixture.hpp"
#include <variant>
#include <algorithm>
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
template<class F> void rejects(F &&operation) {
    bool rejected=false;
    try {operation();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW maintenance checkpoint accepted");
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
    throw std::runtime_error("Selected BNW treasury field unavailable");
}
std::int64_t raw_field(const saves::SavedPlayerObservation &player,
                       std::string_view wanted) {
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(const auto &field:block.fields)
                if(name(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW treasury field unavailable");
}
std::vector<std::int64_t> &raw_history(saves::SavedPlayerObservation &player,
                                            std::string_view wanted) {
    for(auto &block:player.administration.blocks)if(name(block.name)=="treasury")
        for(auto &row:block.arrays)if(name(row.name)==wanted)return row.values;
    throw std::runtime_error("Selected BNW treasury history unavailable");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &document,
                                       std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &entry:document.sections) if(entry.id==id) {
        check(!found,"Duplicate selected BNW section");found=&entry;
    }
    check(found,"Missing selected BNW section");return *found;
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto source_file=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source().snapshot;
    const auto &baseline=host.session().state().maintenance;
    auto state=host.session().state();
    constexpr std::size_t chosen=0;
    auto &cached=state.maintenance.players.at(chosen);
    check(cached.cached_unit_expense>=0 &&
          cached.cached_unit_expense<INT32_MAX-1 &&
          cached.cached_supply_expense>=0 &&
          cached.cached_supply_expense<INT32_MAX-2,
          "Selected BNW expense fixture outside i32 range");
    ++cached.cached_unit_expense;
    cached.cached_supply_expense+=2;
    const auto expected_unit=cached.cached_unit_expense;
    const auto expected_supply=cached.cached_supply_expense;
    auto checkpoint=runtime::make_bnw_maintenance_expense_checkpoint(
        state,source,baseline,host.identity());
    check(checkpoint.bytes.at(8)==2,"Expense writer must use count-aware ID34 v2");
    check(checkpoint.id==runtime::bnw_maintenance_expense_checkpoint_section_id,
          "BNW maintenance section ID changed");
    check(raw_field(state.world.all_players->players.at(chosen),
                    "m_iExpensePerTurnUnitMaintenance")==
              raw_field(source.all_players->players.at(chosen),
                  "m_iExpensePerTurnUnitMaintenance"),
          "BNW maintenance action unexpectedly changed raw import");
    {
        auto &raw=state.world.all_players->players.at(chosen);
        ++raw_field(raw,"m_iExpensePerTurnUnitMaintenance");
        rejects([&] {(void)runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity());});
        --raw_field(raw,"m_iExpensePerTurnUnitMaintenance");
        ++raw_field(raw,"m_iGoldPerTurnFromDiplomacy");
        rejects([&] {(void)runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity());});
        --raw_field(raw,"m_iGoldPerTurnFromDiplomacy");
        ++state.maintenance.players.at(chosen).base_free;
        rejects([&] {(void)runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity());});
        --state.maintenance.players.at(chosen).base_free;
        ++raw.saved_player_id;
        rejects([&] {(void)runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity());});
        --raw.saved_player_id;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first_path=std::filesystem::current_path()/
        ("bnw-maintenance-"+suffix+"-1.CivisSave");
    const auto second_path=std::filesystem::current_path()/
        ("bnw-maintenance-"+suffix+"-2.CivisSave");
    struct Cleanup {
        std::filesystem::path a,b;
        ~Cleanup() {std::error_code error;
            std::filesystem::remove(a,error);
            std::filesystem::remove(b,error);}
    } cleanup{first_path,second_path};
    auto document=saves::make_civis_import_document(
        host.session().source().source,source_file.sha256);
    document.sections.push_back(checkpoint);
    saves::write_civis_save(first_path,document);
    const auto reopened=saves::read_civis_save(first_path);
    const auto loaded=section(reopened,checkpoint.id);
    auto live_import=ui::bootstrap_selected_bnw_preview(first_path,game_root,0);
    check(!live_import.preview.blocker && live_import.preview.host,
          "Normal BNW loader rejected ID34 maintenance expense");
    const auto &live=live_import.preview.host->session();
    check(live.state().maintenance.players.at(chosen).cached_unit_expense==
              expected_unit &&
          live.state().maintenance.players.at(chosen).cached_supply_expense==
              expected_supply &&
          raw_field(live.state().world.all_players->players.at(chosen),
                    "m_iExpensePerTurnUnitMaintenance")==expected_unit &&
          raw_field(live.state().world.all_players->players.at(chosen),
                    "m_iExpensePerTurnUnitSupply")==expected_supply &&
          !live.readiness().can_continue(),
          "Normal BNW loader lost ID34 raw or owned expense cache");
    const auto imported=saves::read_loaded_save(first_path);
    const auto reexport=saves::make_civis_import_document(imported,
                                                           source_file.sha256);
    check(section(reexport,checkpoint.id).bytes==checkpoint.bytes,
          "Normal BNW re-export lost ID34 maintenance expense");
    auto restored=runtime::restore_bnw_maintenance_expense_checkpoint(
        loaded,source,source,baseline,baseline,host.identity());
    check(restored.maintenance.players.at(chosen).cached_unit_expense==
              expected_unit &&
          restored.maintenance.players.at(chosen).cached_supply_expense==
              expected_supply &&
          raw_field(restored.world.all_players->players.at(chosen),
                    "m_iExpensePerTurnUnitMaintenance")==
              expected_unit,
          "BNW maintenance raw/owned round-trip diverged");
    {
        auto bad=loaded;bad.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_maintenance_expense_checkpoint(
            bad,source,source,baseline,baseline,host.identity());});
        auto foreign=host.identity();foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_maintenance_expense_checkpoint(
            loaded,source,source,baseline,baseline,foreign);});
    }
    {
        auto gold_world=source;
        auto &gold=raw_field(gold_world.all_players->players.at(chosen),"m_iGold");
        check(gold>=0 && gold<INT32_MAX,
              "Selected BNW gold companion fixture outside i32 range");
        ++gold;
        const auto companion=runtime::make_bnw_treasury_balance_checkpoint(
            gold_world,source,host.identity());
        ++raw_field(state.world.all_players->players.at(chosen),"m_iGold");
        rejects([&] {(void)runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity());});
        const auto together=runtime::make_bnw_maintenance_expense_checkpoint(
            state,source,baseline,host.identity(),&companion);
        auto after_gold=source;
        runtime::restore_bnw_treasury_balance_checkpoint(
            companion,after_gold,source,host.identity());
        rejects([&] {(void)runtime::restore_bnw_maintenance_expense_checkpoint(
            together,after_gold,source,baseline,baseline,host.identity());});
        const auto combined=runtime::restore_bnw_maintenance_expense_checkpoint(
            together,after_gold,source,baseline,baseline,host.identity(),&companion);
        check(raw_field(combined.world.all_players->players.at(chosen),
                        "m_iGold")==gold &&
              combined.maintenance.players.at(chosen).cached_unit_expense==
                  expected_unit,
              "BNW ID30 then ID34 composition lost a domain");
        --raw_field(state.world.all_players->players.at(chosen),"m_iGold");
    }
    {
        // Genuine historical bytes from the pinned previous production
        // codecs, using their old diagnostic count convention. No DoGold.
        auto legacy_state=host.session().state();
        legacy_state.maintenance.players.at(chosen).cached_unit_expense=expected_unit;
        legacy_state.maintenance.players.at(chosen).cached_supply_expense=expected_supply;
        const auto legacy_expense=runtime::maintenance_legacy_fixture::make_bnw_maintenance_expense_checkpoint(
            legacy_state,source,baseline,host.identity(),nullptr);
        check(legacy_expense.bytes.at(8)==1,"Pinned historical writer must produce ID34 v1");
        const auto old_restored=runtime::restore_bnw_maintenance_expense_checkpoint(
            legacy_expense,source,source,baseline,baseline,host.identity());
        check(old_restored.maintenance.players.at(chosen).cached_unit_expense==expected_unit,
              "Historical expense-only ID34 v1 lost its value");

        auto &player=legacy_state.world.all_players->players.at(chosen);
        auto &owner=legacy_state.treasury.players.at(chosen);
        const auto needed=std::max(owner.balance_history_times100.size(),owner.income_history_times100.size());
        while(static_cast<std::uint64_t>(legacy_state.setup.game_turn)<=needed) {
            check(legacy_state.setup.game_turn>=0 && legacy_state.setup.game_turn<INT32_MAX,
                  "Legacy diagnostic clock outside source i32 range");
            ++legacy_state.setup.game_turn;
            ++legacy_state.world.pregame->archive.integers.at("game_turn");
            ++legacy_state.maintenance.elapsed_turns;
            ++std::get<saves::Expansion2PrefixObservation>(legacy_state.world.game_state.prefix).scalars[1];
        }
        ++raw_field(player,"m_iGold");++owner.balance_times100;
        ++raw_field(player,"m_iLifetimeGrossGoldIncome");++owner.lifetime_gross_gold;
        raw_history(player,"m_GoldBalanceForTurnTimes100").push_back(owner.balance_times100);
        raw_history(player,"m_GoldChangeForTurnTimes100").push_back(100);
        owner.balance_history_times100.push_back(owner.balance_times100);
        owner.income_history_times100.push_back(100);
        // Old v2 diagnostics appended arrays while retaining source counts.
        const auto old_pair=runtime::legacy_v2_make_bnw_treasury_balance_checkpoint_pair(
            legacy_state,source,host.session().state().treasury,host.identity());
        const auto old_combined=runtime::maintenance_legacy_fixture::make_bnw_maintenance_expense_checkpoint(
            legacy_state,source,baseline,host.identity(),&old_pair.raw);
        auto clock_world=source;runtime::BnwGameClockState clock_owner;
        const auto clock_section=runtime::make_bnw_game_clock_checkpoint(legacy_state,source,host.identity());
        runtime::restore_bnw_game_clock_checkpoint(clock_section,clock_world,clock_owner,host.identity());
        auto gold_restored=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            old_pair,source,host.session().state().treasury,legacy_state.setup,host.identity(),&clock_world);
        auto before_expense=baseline;before_expense.elapsed_turns=legacy_state.maintenance.elapsed_turns;
        const auto compatible=runtime::restore_bnw_maintenance_expense_checkpoint(
            old_combined,gold_restored.world,source,before_expense,baseline,host.identity(),&old_pair.raw);
        check(raw_field(compatible.world.all_players->players.at(chosen),
                       "m_GoldBalanceForTurnTimes100_wire_count")==
                  static_cast<std::int64_t>(owner.balance_history_times100.size()) &&
              compatible.maintenance.players.at(chosen).cached_supply_expense==expected_supply,
              "Historical ID30 v2 then ID34 v1 lost reconstructed counts or expense");

        auto current=legacy_state;
        ++raw_field(current.world.all_players->players.at(chosen),"m_GoldBalanceForTurnTimes100_wire_count");
        ++raw_field(current.world.all_players->players.at(chosen),"m_GoldChangeForTurnTimes100_wire_count");
        const auto new_pair=runtime::make_bnw_treasury_balance_checkpoint_pair(
            current,source,host.session().state().treasury,host.identity());
        const auto new_expense=runtime::make_bnw_maintenance_expense_checkpoint(
            current,source,baseline,host.identity(),&new_pair.raw);
        auto new_gold=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            new_pair,source,host.session().state().treasury,current.setup,host.identity(),&clock_world);
        const auto combined=runtime::restore_bnw_maintenance_expense_checkpoint(
            new_expense,new_gold.world,source,before_expense,baseline,host.identity(),&new_pair.raw);
        check(combined.maintenance.players.at(chosen).cached_unit_expense==expected_unit &&
              raw_field(combined.world.all_players->players.at(chosen),"m_GoldChangeForTurnTimes100_wire_count")==
                  static_cast<std::int64_t>(owner.income_history_times100.size()),
              "ID30 v3 plus ID34 v2 history composition lost real count/cache");
        rejects([&]{(void)runtime::make_bnw_maintenance_expense_checkpoint(
            current,source,baseline,host.identity());});
        auto bad=current;
        --raw_field(bad.world.all_players->players.at(chosen),"m_GoldChangeForTurnTimes100_wire_count");
        rejects([&]{(void)runtime::make_bnw_maintenance_expense_checkpoint(
            bad,source,baseline,host.identity(),&new_pair.raw);});
        auto bad_destination=new_gold.world;
        --raw_field(bad_destination.all_players->players.at(chosen),"m_GoldBalanceForTurnTimes100_wire_count");
        rejects([&]{(void)runtime::restore_bnw_maintenance_expense_checkpoint(
            new_expense,bad_destination,source,before_expense,baseline,host.identity(),&new_pair.raw);});
        check(raw_field(bad_destination.all_players->players.at(chosen),"m_iExpensePerTurnUnitSupply")==
                  raw_field(source.all_players->players.at(chosen),"m_iExpensePerTurnUnitSupply") &&
              before_expense.players.at(chosen).cached_supply_expense==baseline.players.at(chosen).cached_supply_expense,
              "Rejected composition partly mutated destination");
        auto damaged=new_pair.raw;damaged.bytes.at(232)^=1;
        rejects([&]{(void)runtime::restore_bnw_maintenance_expense_checkpoint(
            new_expense,new_gold.world,source,before_expense,baseline,host.identity(),&damaged);});
    }
    state.world=std::move(restored.world);
    state.maintenance=std::move(restored.maintenance);
    const auto again=runtime::make_bnw_maintenance_expense_checkpoint(
        state,source,baseline,host.identity());
    auto next=saves::make_civis_import_document(
        host.session().source().source,source_file.sha256);
    next.sections.push_back(again);
    saves::write_civis_save(second_path,next);
    const auto reread=saves::read_civis_save(second_path);
    const auto final=runtime::restore_bnw_maintenance_expense_checkpoint(
        section(reread,again.id),source,source,baseline,baseline,host.identity());
    check(final.maintenance.players.at(chosen).cached_unit_expense==
              expected_unit &&
          runtime::identify_file(path).sha256==source_file.sha256,
          "BNW maintenance re-save or original Civ5Save changed");
    std::cout << "PASS BNW maintenance expense raw/owned checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW maintenance expense checkpoint: "
                  << error.what() << '\n';return 1;
    }
}
