#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "bnw_treasury_legacy_v2_fixture.hpp"
#include <algorithm>
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
    bool failed=false;
    try {f();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW treasury balance checkpoint was accepted");
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
saves::SavedSessionDecodeContext context() {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    return result;
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
const std::vector<std::int64_t> &raw_array(
    const saves::SavedPlayerObservation &player,std::string_view wanted) {
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(const auto &array:block.arrays)
                if(name(array.name)==wanted) return array.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
std::vector<std::int64_t> &raw_array(
    saves::SavedPlayerObservation &player,std::string_view wanted) {
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="treasury")
            for(auto &array:block.arrays)
                if(name(array.name)==wanted) return array.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
runtime::TreasuryContext baseline_from_raw(
    const saves::SavedSessionSnapshot &world) {
    runtime::TreasuryContext result;
    result.profile=content::TechnologyProfile::bnw;
    for(const auto &raw:world.all_players->players) {
        runtime::TreasuryPlayer player;
        player.source_core={{raw.core.start_offset,raw.core.end_offset}};
        player.source_administration={{raw.administration.start_offset,
                                       raw.administration.end_offset}};
        player.balance_times100=raw_field(raw,"m_iGold");
        player.connections_times100=raw_field(raw,"m_iCityConnectionGoldTimes100");
        player.connection_modifier=
            raw_field(raw,"m_iCityConnectionTradeRouteGoldModifier");
        player.connection_change=
            raw_field(raw,"m_iCityConnectionTradeRouteGoldChange");
        player.lifetime_gross_gold=raw_field(raw,"m_iLifetimeGrossGoldIncome");
        player.balance_history_times100=
            raw_array(raw,"m_GoldBalanceForTurnTimes100");
        player.income_history_times100=
            raw_array(raw,"m_GoldChangeForTurnTimes100");
        check(result.players.emplace(raw.saved_player_id,std::move(player)).second,
              "Selected BNW treasury roster duplicate");
    }
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {std::move(world),{},{},{},{},{},{},{},{},{},
            std::move(random),{},{},{},{},{},{},{},{},{},{},{}};
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!found,"Duplicate BNW treasury section");found=&part;
        }
    check(found,"Missing BNW treasury section");return *found;
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    const auto baseline=baseline_from_raw(imported.snapshot);
    auto state=state_from_world(imported.snapshot);
    state.setup.profile=content::TechnologyProfile::bnw;
    for(std::int64_t p=0;p<64;++p) state.setup.players.emplace(p,
        runtime::ResearchPlayerState{});
    state.treasury=baseline;
    constexpr std::size_t chosen=0;
    auto &raw=state.world.all_players->players[chosen];
    check(raw_field(raw,"m_iGold")>=0 &&
          raw_field(raw,"m_iGold")<=INT32_MAX-200,
          "Selected BNW balance is outside supported positive range");
    raw_field(raw,"m_iGold")+=100;
    state.treasury.players.at(chosen).balance_times100+=100;
    const auto first=raw_field(raw,"m_iGold");
    auto pair=runtime::make_bnw_treasury_balance_checkpoint_pair(
        state,imported.snapshot,baseline,identity);
    check(pair.raw.id==runtime::bnw_treasury_balance_checkpoint_section_id &&
          pair.owned.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_treasury),
          "BNW raw/owned treasury section IDs changed");
    {
        ++raw_field(raw,"m_iGoldPerTurnFromDiplomacy");
        rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
            state,imported.snapshot,baseline,identity);});
        --raw_field(raw,"m_iGoldPerTurnFromDiplomacy");
        ++state.treasury.players.at(chosen).lifetime_gross_gold;
        rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
            state,imported.snapshot,baseline,identity);});
        --state.treasury.players.at(chosen).lifetime_gross_gold;
        ++state.treasury.players.at(chosen).balance_times100;
        rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
            state,imported.snapshot,baseline,identity);});
        --state.treasury.players.at(chosen).balance_times100;
        ++raw.saved_player_id;
        rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
            state,imported.snapshot,baseline,identity);});
        --raw.saved_player_id;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first_path=std::filesystem::current_path()/
        ("bnw-treasury-balance-"+suffix+"-1.CivisSave");
    const auto second_path=std::filesystem::current_path()/
        ("bnw-treasury-balance-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup(){
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first_path,second_path};
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
    saves::write_civis_save(first_path,document);
    {
        const auto loaded=saves::read_loaded_save(first_path);
        check(loaded.live_bnw_treasury_balance.has_value() &&
              loaded.live_treasury.has_value() &&
              *loaded.live_bnw_treasury_balance==pair.raw.bytes &&
              *loaded.live_treasury==pair.owned.bytes,
              "Common Civis loader lost the paired treasury balance sections");
        const auto copied=saves::make_civis_import_document(
            loaded,original.sha256);
        check(section(copied,runtime::bnw_treasury_balance_checkpoint_section_id).bytes==
                  pair.raw.bytes,
              "Common Civis re-save lost raw treasury balance section");
    }
    document.sections.clear();state.world={};
    {
        const auto reopened=saves::read_civis_save(first_path);
        pair.raw=section(reopened,runtime::bnw_treasury_balance_checkpoint_section_id);
        pair.owned=section(reopened,static_cast<std::uint32_t>(
            saves::CivisSaveSectionId::live_treasury));
        auto restored=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            pair,imported.snapshot,baseline,state.setup,identity);
        check(raw_field(restored.world.all_players->players[chosen],"m_iGold")==first &&
              restored.treasury.players.at(chosen).balance_times100==first &&
              raw_field(imported.snapshot.all_players->players[chosen],"m_iGold")==
                  first-100,
              "BNW treasury raw/owned balance did not survive write/read");
        state.world=std::move(restored.world);
        state.treasury=std::move(restored.treasury);
        raw_field(state.world.all_players->players[chosen],"m_iGold")+=100;
        state.treasury.players.at(chosen).balance_times100+=100;
        auto second=runtime::make_bnw_treasury_balance_checkpoint_pair(
            state,imported.snapshot,baseline,identity);
        rejects([&] {
            (void)runtime::restore_bnw_treasury_balance_checkpoint_pair(
                {pair.raw,second.owned},imported.snapshot,baseline,
                state.setup,identity);
        });
        auto next=saves::make_civis_import_document(
            imported.source,original.sha256);
        next.sections.push_back(second.raw);next.sections.push_back(second.owned);
        saves::write_civis_save(second_path,next);
    }
    {
        const auto reopened=saves::read_civis_save(second_path);
        runtime::BnwTreasuryBalanceCheckpointPair second{
            section(reopened,runtime::bnw_treasury_balance_checkpoint_section_id),
            section(reopened,static_cast<std::uint32_t>(
                saves::CivisSaveSectionId::live_treasury))};
        auto restored=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            second,imported.snapshot,baseline,state.setup,identity);
        check(raw_field(restored.world.all_players->players[chosen],"m_iGold")==
                  first+100 &&
              restored.treasury.players.at(chosen).balance_times100==first+100,
              "BNW treasury second save did not restore raw/owned balance");
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW treasury checkpoint altered original Civ5Save");
    // Diagnostic v2 projection of a later turn. The clock section is a
    // required companion when SDK history sampling appends a turn row.
    {
        auto advanced=state_from_world(imported.snapshot);
        advanced.setup.profile=content::TechnologyProfile::bnw;
        for(std::int64_t p=0;p<64;++p) advanced.setup.players.emplace(p,
            runtime::ResearchPlayerState{});
        advanced.treasury=baseline;
        auto &prefix=std::get<saves::Expansion2PrefixObservation>(
            advanced.world.game_state.prefix);
        const auto source_turn=imported.snapshot.pregame->archive.integers.at("game_turn");
        const auto next_turn=std::max({source_turn+1,
            static_cast<std::int64_t>(baseline.players.at(chosen).balance_history_times100.size()+1),
            static_cast<std::int64_t>(baseline.players.at(chosen).income_history_times100.size()+1)});
        check(source_turn>=0 && prefix.scalars[1]>=0,
              "Selected Gandhi v2 fixture has invalid source clock");
        prefix.scalars[1]+=next_turn-source_turn;
        advanced.world.pregame->archive.integers.at("game_turn")=next_turn;
        advanced.setup.game_turn=next_turn;
        advanced.maintenance.elapsed_turns=prefix.scalars[1];
        auto &current=advanced.world.all_players->players[chosen];
        raw_field(current,"m_iGold")+=100;
        raw_field(current,"m_iLifetimeGrossGoldIncome")+=2;
        raw_array(current,"m_GoldBalanceForTurnTimes100").push_back(
            raw_field(current,"m_iGold"));
        raw_array(current,"m_GoldChangeForTurnTimes100").push_back(100);
        ++raw_field(current,"m_GoldBalanceForTurnTimes100_wire_count");
        ++raw_field(current,"m_GoldChangeForTurnTimes100_wire_count");
        auto &owned=advanced.treasury.players.at(chosen);
        owned.balance_times100+=100;
        owned.lifetime_gross_gold+=2;
        owned.balance_history_times100.push_back(owned.balance_times100);
        owned.income_history_times100.push_back(100);
        auto clock=runtime::make_bnw_game_clock_checkpoint(
            advanced,imported.snapshot,identity);
        auto v2=runtime::make_bnw_treasury_balance_checkpoint_pair(
            advanced,imported.snapshot,baseline,identity);
        check(v2.raw.bytes.at(8)==3,
              "BNW four-field treasury checkpoint did not use count-aware v3");
        {
            auto legacy=advanced;
            // Exercise the pinned PREVIOUS codec, which really produced v2.
            // This historical diagnostic shape is never sent to DoGold.
            for(const auto key:{"m_GoldBalanceForTurnTimes100_wire_count",
                                "m_GoldChangeForTurnTimes100_wire_count"})
                raw_field(legacy.world.all_players->players[chosen],key)=
                    raw_field(imported.snapshot.all_players->players[chosen],key);
            const auto old_pair=runtime::legacy_v2_make_bnw_treasury_balance_checkpoint_pair(
                legacy,imported.snapshot,baseline,identity);
            check(old_pair.raw.bytes.at(8)==2,"Pinned prior codec did not produce v2");
            auto legacy_destination=imported.snapshot;
            runtime::BnwGameClockState legacy_clock;
            runtime::restore_bnw_game_clock_checkpoint(clock,legacy_destination,legacy_clock,identity);
            const auto decoded=runtime::restore_bnw_treasury_balance_checkpoint_pair(
                old_pair,imported.snapshot,baseline,advanced.setup,identity,&legacy_destination);
            const auto &player=decoded.world.all_players->players[chosen];
            check(raw_field(player,"m_GoldBalanceForTurnTimes100_wire_count")==
                      static_cast<std::int64_t>(raw_array(player,"m_GoldBalanceForTurnTimes100").size()) &&
                  raw_field(player,"m_GoldChangeForTurnTimes100_wire_count")==
                      static_cast<std::int64_t>(raw_array(player,"m_GoldChangeForTurnTimes100").size()),
                  "Legacy v2 import failed to reconstruct native count/vector pair");
        }
        {
            auto bad=advanced;
            --raw_field(bad.world.all_players->players[chosen],"m_GoldBalanceForTurnTimes100_wire_count");
            rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
                bad,imported.snapshot,baseline,identity);});
            bad=advanced;
            ++raw_field(bad.world.all_players->players[chosen],"m_GoldChangeForTurnTimes100_wire_count");
            rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
                bad,imported.snapshot,baseline,identity);});
            auto bad_source=imported.snapshot;
            raw_field(bad_source.all_players->players[chosen],"m_GoldBalanceForTurnTimes100_wire_count")=-1;
            rejects([&]{(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
                advanced,bad_source,baseline,identity);});
            auto bad_wire=v2.raw;
            // Fixed header(208), ID/balances(12), lifetime pair(8), then
            // first history old/new native counts: new count begins at232.
            check(bad_wire.bytes.size()>232,"v3 history count wire offset absent");
            bad_wire.bytes[232]^=1;
            auto destination=imported.snapshot;
            rejects([&]{runtime::restore_bnw_treasury_balance_checkpoint(
                bad_wire,destination,imported.snapshot,identity);});
            check(raw_field(destination.all_players->players[chosen],"m_iGold")==
                      raw_field(imported.snapshot.all_players->players[chosen],"m_iGold") &&
                  raw_array(destination.all_players->players[chosen],"m_GoldBalanceForTurnTimes100")==
                      baseline.players.at(chosen).balance_history_times100 &&
                  raw_field(destination.all_players->players[chosen],"m_GoldBalanceForTurnTimes100_wire_count")==
                      raw_field(imported.snapshot.all_players->players[chosen],"m_GoldBalanceForTurnTimes100_wire_count"),
                  "Corrupt v3 count partially changed raw destination");
        }
        {
            auto bad=advanced;
            raw_array(bad.world.all_players->players[chosen],
                      "m_GoldBalanceForTurnTimes100").push_back(1);
            bad.treasury.players.at(chosen).balance_history_times100.push_back(1);
            rejects([&] {(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
                bad,imported.snapshot,baseline,identity);});
            ++raw_field(bad.world.all_players->players[chosen],
                        "m_iGoldPerTurnFromDiplomacy");
            rejects([&] {(void)runtime::make_bnw_treasury_balance_checkpoint_pair(
                bad,imported.snapshot,baseline,identity);});
        }
        const auto v2_path=std::filesystem::current_path()/
            ("bnw-treasury-v2-"+suffix+"-1.CivisSave");
        const auto v2_again_path=std::filesystem::current_path()/
            ("bnw-treasury-v2-"+suffix+"-2.CivisSave");
        Cleanup extended_cleanup{v2_path,v2_again_path};
        auto saved=saves::make_civis_import_document(imported.source,original.sha256);
        saved.sections.push_back(clock);saved.sections.push_back(v2.raw);
        saved.sections.push_back(v2.owned);
        saves::write_civis_save(v2_path,saved);
        const auto reopened=saves::read_civis_save(v2_path);
        auto clock_world=imported.snapshot;
        runtime::BnwGameClockState restored_clock;
        runtime::restore_bnw_game_clock_checkpoint(
            section(reopened,clock.id),clock_world,restored_clock,identity);
        const runtime::BnwTreasuryBalanceCheckpointPair reloaded{
            section(reopened,v2.raw.id),section(reopened,v2.owned.id)};
        rejects([&] {(void)runtime::restore_bnw_treasury_balance_checkpoint_pair(
            reloaded,imported.snapshot,baseline,advanced.setup,identity);});
        auto restored=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            reloaded,imported.snapshot,baseline,advanced.setup,identity,&clock_world);
        check(raw_field(restored.world.all_players->players[chosen],"m_iGold")==
                  owned.balance_times100 &&
              raw_field(restored.world.all_players->players[chosen],
                        "m_iLifetimeGrossGoldIncome")==owned.lifetime_gross_gold &&
              raw_array(restored.world.all_players->players[chosen],
                        "m_GoldBalanceForTurnTimes100")==owned.balance_history_times100 &&
              raw_array(restored.world.all_players->players[chosen],
                        "m_GoldChangeForTurnTimes100")==owned.income_history_times100,
              "BNW v3 four-field treasury state did not round-trip");
        check(raw_field(restored.world.all_players->players[chosen],"m_GoldBalanceForTurnTimes100_wire_count")==
                  static_cast<std::int64_t>(owned.balance_history_times100.size()) &&
              raw_field(restored.world.all_players->players[chosen],"m_GoldChangeForTurnTimes100_wire_count")==
                  static_cast<std::int64_t>(owned.income_history_times100.size()),
              "BNW v3 raw wire counts did not round-trip");
        advanced.world=std::move(restored.world);
        advanced.treasury=std::move(restored.treasury);
        ++raw_field(advanced.world.all_players->players[chosen],"m_iGold");
        ++raw_field(advanced.world.all_players->players[chosen],
                    "m_iLifetimeGrossGoldIncome");
        ++advanced.treasury.players.at(chosen).balance_times100;
        ++advanced.treasury.players.at(chosen).lifetime_gross_gold;
        auto second=runtime::make_bnw_treasury_balance_checkpoint_pair(
            advanced,imported.snapshot,baseline,identity);
        auto again=saves::make_civis_import_document(imported.source,original.sha256);
        again.sections.push_back(clock);again.sections.push_back(second.raw);
        again.sections.push_back(second.owned);
        saves::write_civis_save(v2_again_path,again);
        const auto second_reopened=saves::read_civis_save(v2_again_path);
        auto final=runtime::restore_bnw_treasury_balance_checkpoint_pair(
            {section(second_reopened,second.raw.id),
             section(second_reopened,second.owned.id)},imported.snapshot,
            baseline,advanced.setup,identity,&clock_world);
        check(final.treasury.players.at(chosen).balance_times100==
                  advanced.treasury.players.at(chosen).balance_times100 &&
              final.treasury.players.at(chosen).lifetime_gross_gold==
                  advanced.treasury.players.at(chosen).lifetime_gross_gold &&
              runtime::identify_file(path).sha256==original.sha256,
              "BNW v2 re-save or original Civ5Save changed");
    }
    std::cout << "PASS BNW raw/owned treasury balance checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW treasury balance checkpoint: "
                  << error.what() << '\n';
        return 1;
    }
}
