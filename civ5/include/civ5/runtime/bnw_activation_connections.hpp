#pragma once
#include "civ5/runtime/bnw_game_activation_preflight.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {
// CvCityConnections::Init clears the nonserialized plot route vector;
// Read restores only the route matrix and connectable city plot IDs.
struct BnwActivationConnectionsPlayer {
    bool plot_routes_known=false;
    std::vector<std::pair<std::int64_t,std::uint8_t>> plot_routes;
    std::optional<std::int64_t> updated_turn;
};
struct BnwActivationConnectionsOwner {
    SessionCheckpointIdentity identity;
    std::array<std::uint8_t,16> map_guid{};
    std::map<std::int64_t,BnwActivationConnectionsPlayer> players;
};
// A new native transient owner is initialized from an ORIGINAL Civ5 import.
// Civis continuation needs its own persisted/reconstructed transient proof.
BnwActivationConnectionsOwner make_bnw_activation_connections_import_owner(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity);
// Complete confirmed no-route/no-water human-major source branch. Existing
// true route flags, plot-route broadcasts and usable routes need fuller owners.
// Each function validates first and publishes atomically on its supplied draft.
LiveActionResult update_bnw_activation_city_connections(LiveSessionState &state,
    BnwActivationConnectionsOwner &owner,const content::RulesDatabase &rules,
    std::int64_t player);
LiveActionResult update_bnw_activation_city_connection_gold(LiveSessionState &state,
    const BnwActivationConnectionsOwner &owner,std::int64_t player);
} // namespace civ5::runtime
