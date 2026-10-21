#include "civ5/runtime/bnw_player_temporary_bonus_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
constexpr std::array<std::string_view, 3> names{
    "m_iAttackBonusTurns", "m_iCultureBonusTurns", "m_iTourismBonusTurns"};
} // namespace

BnwTemporaryBonusTurnStep apply_bnw_temporary_bonus_turn(
    LiveSessionState &state, std::int64_t player_id) {
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == bnw && state.world.all_players &&
            state.world.all_players->layout == bnw,
            "BNW temporary boosts require decoded Expansion2 players");
    const auto setup = state.setup.players.find(player_id);
    require(setup != state.setup.players.end() && setup->second.alive,
            "BNW temporary boosts require an alive player");

    saves::SavedPlayerObservation *saved = nullptr;
    for (auto &candidate : state.world.all_players->players)
        if (candidate.saved_player_id == player_id) {
            require(!saved, "duplicate BNW temporary boost player ID");
            saved = &candidate;
        }
    require(saved && saved->core.layout == bnw,
            "BNW temporary boost player core unavailable");

    std::array<saves::PlayerCoreField *, 5> fields{};
    constexpr std::array<std::string_view, 5> field_names{
        "m_eID", "m_bAlive", "m_iAttackBonusTurns",
        "m_iCultureBonusTurns", "m_iTourismBonusTurns"};
    for (auto &field : saved->core.fields)
        for (std::size_t i = 0; i < field_names.size(); ++i)
            if (field.name && std::string_view(field.name) == field_names[i]) {
                require(!fields[i], "duplicate BNW temporary boost scalar");
                fields[i] = &field;
            }
    for (const auto *field : fields)
        require(field && field->values.size() == 1,
                "missing or invalid BNW temporary boost scalar");
    require(fields[0]->encoding == saves::PlayerCoreEncoding::i32 &&
            fields[0]->values[0] == player_id &&
            fields[1]->encoding == saves::PlayerCoreEncoding::boolean &&
            fields[1]->values[0] == 1,
            "BNW temporary boost player identity differs");

    BnwTemporaryBonusTurnStep step;
    step.player_id = player_id;
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto *field = fields[i + 2];
        require(field->encoding == saves::PlayerCoreEncoding::i32 &&
                field->values[0] >= 0 &&
                field->values[0] <= std::numeric_limits<std::int32_t>::max(),
                "invalid BNW temporary bonus duration");
        step.before[i] = static_cast<std::int32_t>(field->values[0]);
        // CvPlayer.cpp:4153-4164 and 12187-12236: each positive value is
        // decremented by one; Change*BonusTurns has no other side effects.
        step.after[i] = step.before[i] > 0 ? step.before[i] - 1 : 0;
    }
    // Validate all three fields before writing any of them. The enclosing
    // turn publishes this draft only after every required phase succeeds.
    for (std::size_t i = 0; i < names.size(); ++i)
        fields[i + 2]->values[0] = step.after[i];
    return step;
}
} // namespace civ5::runtime
