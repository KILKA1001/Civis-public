#include "civ5/ui/bnw_unit_panel_preview_bridge.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
namespace {
std::int64_t scalar(const saves::UnitCoreObservation &core, std::string_view name) {
    std::optional<std::int64_t> result;
    for (const auto &field : core.fields) if (field.name && std::string_view(field.name) == name) {
        if (result || field.values.size() != 1) throw std::runtime_error("Ambiguous selected unit scalar");
        result = field.values.front();
    }
    if (!result || *result < INT32_MIN || *result > INT32_MAX)
        throw std::runtime_error("Missing/out-of-range selected unit scalar");
    return *result;
}
std::int64_t type_hash(const saves::UnitCoreObservation &core) {
    std::optional<std::int64_t> result;
    for (const auto &field : core.fields) if (field.name && std::string_view(field.name) == "idUnitType") {
        if (result || field.values.size() != 1) throw std::runtime_error("Ambiguous selected unit type hash");
        result = field.values.front();
    }
    if (!result || *result < 0 || *result > UINT32_MAX) throw std::runtime_error("Unknown selected unit type hash");
    return *result;
}
const saves::SavedUnitObservation &unit(const runtime::LiveSessionState &state, std::int64_t player, std::int64_t id) {
    (void)runtime::bnw_unit_order_plot(state, {player, id});
    const saves::SavedUnitObservation *result = nullptr;
    for (const auto &owner : state.world.all_players->players) {
        if (owner.saved_player_id != player) continue;
        for (const auto &candidate : owner.units.units) if (scalar(candidate.core, "m_iID") == id) {
            if (result) throw std::runtime_error("Ambiguous selected unit owner");
            result = &candidate;
        }
    }
    if (!result || scalar(result->core, "m_eOwner") != player || scalar(result->core, "m_bDeathDelay") != 0)
        throw std::runtime_error("Selected owned unit is absent/dying");
    return *result;
}
std::int64_t domain(const runtime::LiveSessionState &state, const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types, std::int64_t player, std::int64_t id, const saves::UnitCoreObservation &core) {
    const auto *info = types.find_hash(static_cast<std::uint32_t>(type_hash(core)));
    if (!info || info->table != "Units" || state.maintenance.players.at(player).units.at(id).type != info->id)
        throw std::runtime_error("Selected unit type/current maintenance binding differs");
    std::optional<std::int64_t> result;
    const auto query = database.query_rows({"SELECT Type,Domain FROM Units WHERE ID=?", {info->id}},
        [&](const content::RulesRowView &row) {
            if (result || row.text("Type") != info->type) throw std::runtime_error("Ambiguous selected unit Info");
            const auto name = row.text("Domain");
            const auto *entry = name ? types.find(*name) : nullptr;
            constexpr std::array<std::string_view, 5> source_domains{
                "DOMAIN_SEA", "DOMAIN_AIR", "DOMAIN_LAND", "DOMAIN_IMMOBILE", "DOMAIN_HOVER"};
            if (!entry || entry->table != "Domains" || entry->id < 0 || entry->id >= 5 ||
                entry->type != source_domains.at(entry->id)) throw std::runtime_error("Selected unit domain is unresolved");
            result = entry->id;
        }, 2);
    if (!query.all_succeeded() || !result) throw std::runtime_error("Selected unit domain Info query failed");
    return *result;
}
std::int64_t floor_ratio(std::int64_t value, std::int64_t divisor) {
    return value / divisor - (value % divisor < 0);
}
}
BnwUnitPanelPreviewBridge::BnwUnitPanelPreviewBridge(const runtime::LiveSession &session,
    const content::RulesDatabase &database, const content::InfoTypeCatalog &types,
    const runtime::SessionCheckpointIdentity &identity, std::int64_t active_player)
    : session_(&session), state_(&session.state()), state_generation_(session.state_generation()), database_(&database), types_(&types), identity_(identity), player_(active_player) {
    if (!types.report().all_succeeded() ||
        identity.payload_sha256 != runtime::identify_bytes(session.source().source.bytes).sha256 ||
        identity.rules_sha256 != database.gameplay_fingerprint_sha256())
        throw std::invalid_argument("UnitPanel selected source/rules binding differs");
}
std::optional<LiveWorldViewUnitReadStatsFeed> BnwUnitPanelPreviewBridge::capture(
    const runtime::LiveSession &session, std::int64_t selected, std::uint64_t frame) const {
    if (&session != session_ || &session.state() != state_ || session.state_generation()!=state_generation_ || !frame || selected < 0) return std::nullopt;
    const auto &state = session.state();
    const auto player = state.setup.players.find(player_);
    if (player == state.setup.players.end() || !player->second.alive ||
        state.setup.game_turn < INT32_MIN || state.setup.game_turn > INT32_MAX) return std::nullopt;
    try {
        const auto &core = unit(state, player_, selected).core;
        const auto actual_domain = domain(state, *database_, *types_, player_, selected, core);
        const auto embarked = scalar(core, "m_bEmbarked");
        if (embarked != 0 && embarked != 1) return std::nullopt;
        LiveWorldViewUnitReadStatsFeed feed;
        feed.player_id = player_; feed.unit_id = selected;
        feed.turn = static_cast<std::int32_t>(state.setup.game_turn); feed.frame_number = frame;
        feed.source_world_revision = runtime::bnw_unit_order_world_revision(state, identity_, *database_);
        const auto movement = runtime::quote_bnw_unit_movement(state, *database_, *types_, identity_, {player_, selected});
        if (actual_domain == 1) feed.movement_blocker = "Air UnitPanel needs actual range/rebase query owners";
        else if (actual_domain == 4) feed.movement_blocker = "Hover UnitPanel needs native current-plot domain movement owner";
        else if (!movement.max_moves || movement.terms.denominator <= 0)
            feed.movement_blocker = movement.blocker.empty() ? "Native movement denominator/query is unresolved" : movement.blocker;
        else {
            const auto left = std::max<std::int64_t>(0, scalar(core, "m_iMoves")); // SDK movesLeft(), not cached initial moves.
            feed.movement = std::to_string(left / movement.terms.denominator) + "/" +
                std::to_string(floor_ratio(*movement.max_moves, movement.terms.denominator)) + " [ICON_MOVES]";
        }
        if (actual_domain == 1) feed.strength_blocker = "Air UnitPanel needs actual base-ranged query owner";
        else if (embarked) feed.strength_blocker = "Embarked UnitPanel religious/tourism strength branches are unresolved";
        else if (const auto strength = scalar(core, "m_iBaseCombat"); strength > 0)
            feed.strength = std::to_string(strength) + " [ICON_STRENGTH]";
        else feed.strength_blocker = "Noncombat UnitPanel religious/tourism strength branches are unresolved";
        return feed;
    } catch (const std::exception &) { return std::nullopt; }
}
bool BnwUnitPanelPreviewBridge::matches(const runtime::LiveSession &session,
    const LiveWorldViewUnitReadStatsFeed &feed, std::int64_t selected, std::uint64_t frame) const {
    if (feed.player_id != player_ || feed.unit_id != selected || feed.frame_number != frame ||
        feed.turn != session.state().setup.game_turn) return false;
    // Requery all native operands too: the retained unit-order revision excludes
    // team extra_moves and player trait movement operands. A same-frame write to
    // either must not leave an old numeric label accepted.
    const auto current = capture(session, selected, frame);
    return current && current->source_world_revision == feed.source_world_revision &&
        current->movement == feed.movement && current->strength == feed.strength &&
        current->movement_blocker == feed.movement_blocker && current->strength_blocker == feed.strength_blocker;
}
} // namespace civ5::ui
