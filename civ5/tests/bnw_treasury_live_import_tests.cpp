#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <algorithm>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
std::int64_t &balance(saves::SavedPlayerObservation &player) {
    for (auto &block : player.administration.blocks)
        if (block.name && std::string_view(block.name) == "treasury")
            for (auto &field : block.fields)
                if (field.name && std::string_view(field.name) == "m_iGold")
                    return field.value;
    throw std::runtime_error("Selected BNW raw treasury balance missing");
}
std::int64_t balance(const saves::SavedPlayerObservation &player) {
    for (const auto &block : player.administration.blocks)
        if (block.name && std::string_view(block.name) == "treasury")
            for (const auto &field : block.fields)
                if (field.name && std::string_view(field.name) == "m_iGold")
                    return field.value;
    throw std::runtime_error("Selected BNW raw treasury balance missing");
}
std::int64_t &treasury_integer(saves::SavedPlayerObservation &player,
                               std::string_view name) {
    for(auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(auto &entry:block.fields)
                if(entry.name && std::string_view(entry.name)==name)
                    return entry.value;
    throw std::runtime_error("Selected BNW treasury integer missing");
}
std::int64_t treasury_integer(const saves::SavedPlayerObservation &player,
                              std::string_view name) {
    for(const auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(const auto &entry:block.fields)
                if(entry.name && std::string_view(entry.name)==name)
                    return entry.value;
    throw std::runtime_error("Selected BNW treasury integer missing");
}
std::vector<std::int64_t> &treasury_history(
    saves::SavedPlayerObservation &player,std::string_view name) {
    for(auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(auto &entry:block.arrays)
                if(entry.name && std::string_view(entry.name)==name)
                    return entry.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
const std::vector<std::int64_t> &treasury_history(
    const saves::SavedPlayerObservation &player,std::string_view name) {
    for(const auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(const auto &entry:block.arrays)
                if(entry.name && std::string_view(entry.name)==name)
                    return entry.values;
    throw std::runtime_error("Selected BNW treasury history missing");
}
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 3, "Supply selected Gandhi save and installed BNW root");
        const std::filesystem::path selected(argv[1]), game_root(argv[2]);
        auto original = ui::bootstrap_selected_bnw_preview(selected, game_root, 0);
        if (original.preview.blocker)
            throw std::runtime_error(original.preview.blocker->reason);
        check(original.preview.host != nullptr, "Selected BNW live host missing");
        auto &host = *original.preview.host;
        auto draft = host.session().state();
        const auto baseline = draft.treasury;
        const auto &source = host.session().source();
        std::size_t chosen = 0;
        bool found = false;
        for (std::size_t i = 0; i < draft.world.all_players->players.size(); ++i) {
            auto &raw = draft.world.all_players->players[i];
            const auto owned = draft.treasury.players.find(raw.saved_player_id);
            if (owned == draft.treasury.players.end()) continue;
            const auto value = balance(raw);
            if (value >= 0 && value < INT32_MAX - 100) {
                chosen = i; found = true; break;
            }
        }
        check(found, "Selected BNW has no supported treasury balance");
        auto &raw = draft.world.all_players->players[chosen];
        const auto player_id = raw.saved_player_id;
        const auto expected = balance(raw) + 100;
        balance(raw) = expected;
        draft.treasury.players.at(player_id).balance_times100 = expected;
        const auto pair = runtime::make_bnw_treasury_balance_checkpoint_pair(
            draft, source.snapshot, baseline, host.identity());
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        Cleanup cleanup{std::filesystem::current_path() /
                        ("bnw-treasury-live-import-" + suffix + ".CivisSave")};
        auto document = saves::make_civis_import_document(
            source.source, runtime::identify_file(selected).sha256);
        document.sections.push_back(pair.raw);
        document.sections.push_back(pair.owned);
        saves::write_civis_save(cleanup.path, document);
        auto resumed = ui::bootstrap_selected_bnw_preview(cleanup.path, game_root, 0);
        if (resumed.preview.blocker)
            throw std::runtime_error(resumed.preview.blocker->reason);
        check(resumed.preview.host != nullptr, "BNW paired treasury import host missing");
        const auto &live = resumed.preview.host->session().state();
        check(balance(live.world.all_players->players[chosen]) == expected &&
              live.treasury.players.at(player_id).balance_times100 == expected &&
              balance(original.preview.host->session().state().world.all_players->players[chosen])
                  == expected - 100 &&
              !resumed.preview.host->session().readiness().can_continue(),
              "BNW live treasury import lost balance or overclaimed readiness");
        auto advanced=host.session().state();
        auto &prefix=std::get<saves::Expansion2PrefixObservation>(
            advanced.world.game_state.prefix);
        const auto source_turn=source.snapshot.pregame->archive.integers.at("game_turn");
        const auto old_balance_rows=advanced.treasury.players.at(player_id).
            balance_history_times100.size();
        const auto old_income_rows=advanced.treasury.players.at(player_id).
            income_history_times100.size();
        const auto next_turn=std::max({source_turn+1,
            static_cast<std::int64_t>(old_balance_rows+1),
            static_cast<std::int64_t>(old_income_rows+1)});
        check(source_turn>=0 && prefix.scalars[1]>=0,
              "Selected BNW clock baseline invalid");
        prefix.scalars[1]+=next_turn-source_turn;
        advanced.world.pregame->archive.integers.at("game_turn")=next_turn;
        advanced.setup.game_turn=next_turn;
        advanced.maintenance.elapsed_turns=prefix.scalars[1];
        auto &advanced_raw=advanced.world.all_players->players[chosen];
        auto &advanced_owned=advanced.treasury.players.at(player_id);
        balance(advanced_raw)=expected;
        advanced_owned.balance_times100=expected;
        treasury_integer(advanced_raw,"m_iLifetimeGrossGoldIncome")+=2;
        advanced_owned.lifetime_gross_gold+=2;
        treasury_history(advanced_raw,"m_GoldBalanceForTurnTimes100").push_back(expected);
        advanced_owned.balance_history_times100.push_back(expected);
        treasury_history(advanced_raw,"m_GoldChangeForTurnTimes100").push_back(100);
        advanced_owned.income_history_times100.push_back(100);
        const auto clock=runtime::make_bnw_game_clock_checkpoint(
            advanced,source.snapshot,host.identity());
        const auto v2=runtime::make_bnw_treasury_balance_checkpoint_pair(
            advanced,source.snapshot,baseline,host.identity());
        check(v2.raw.bytes.at(8)==2,"BNW treasury history did not select v2");
        Cleanup extended{std::filesystem::current_path() /
                         ("bnw-treasury-v2-live-import-"+suffix+".CivisSave")};
        auto v2_document=saves::make_civis_import_document(
            source.source,runtime::identify_file(selected).sha256);
        v2_document.sections.push_back(clock);
        v2_document.sections.push_back(v2.raw);
        v2_document.sections.push_back(v2.owned);
        saves::write_civis_save(extended.path,v2_document);
        auto v2_resumed=ui::bootstrap_selected_bnw_preview(
            extended.path,game_root,0);
        if(v2_resumed.preview.blocker)
            throw std::runtime_error(v2_resumed.preview.blocker->reason);
        check(v2_resumed.preview.host!=nullptr,
              "BNW v2 paired treasury import host missing");
        const auto &v2_live=v2_resumed.preview.host->session().state();
        const auto &v2_player=v2_live.world.all_players->players[chosen];
        const auto &v2_treasury=v2_live.treasury.players.at(player_id);
        check(v2_live.setup.game_turn==next_turn &&
              balance(v2_player)==expected &&
              treasury_integer(v2_player,"m_iLifetimeGrossGoldIncome")==
                  advanced_owned.lifetime_gross_gold &&
              treasury_history(v2_player,"m_GoldBalanceForTurnTimes100")==
                  advanced_owned.balance_history_times100 &&
              treasury_history(v2_player,"m_GoldChangeForTurnTimes100")==
                  advanced_owned.income_history_times100 &&
              v2_treasury.balance_times100==expected &&
              v2_treasury.lifetime_gross_gold==advanced_owned.lifetime_gross_gold &&
              !v2_resumed.preview.host->session().readiness().can_continue(),
              "BNW v2 live treasury import lost clock, histories or readiness guard");
        std::cout << "BNW treasury live import pair PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
