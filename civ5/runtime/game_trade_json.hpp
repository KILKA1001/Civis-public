#pragma once
// Included after quote inside inspect_main's anonymous namespace.
namespace trade_json {
void value(bool v) { std::cout << (v ? "true" : "false"); }
void value(std::int64_t v) { std::cout << v; }
void value(std::uint64_t v) { std::cout << v; }
template <class T> void value(const std::vector<T> &v) {
    std::cout << '[';
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i)
            std::cout << ',';
        value(v[i]);
    }
    std::cout << ']';
}
template <class T> void value(const std::optional<T> &v) {
    if (v)
        value(*v);
    else
        std::cout << "null";
}
template <class T> void field(const char *name, const T &v) {
    std::cout << ',' << quote(name) << ':';
    value(v);
}
void route(const civ5::saves::TradeRouteObservation &r) {
    const auto *vp=civ5::saves::find_vp_trade_route(r);
    std::cout << "{\"id\":";
    value(r.id);
    field("origin_id", vp?std::optional{vp->origin_id}:std::optional<std::int64_t>{});
    field("origin_x", r.origin_x);
    field("origin_y", r.origin_y);
    field("destination_id", vp?std::optional{vp->destination_id}:std::optional<std::int64_t>{});
    field("destination_x", r.destination_x);
    field("destination_y", r.destination_y);
    field("origin_owner", r.origin_owner);
    field("destination_owner", r.destination_owner);
    field("domain", r.domain);
    field("connection_type", r.connection_type);
    field("unit_location_index", r.unit_location_index);
    field("moving_forward", r.moving_forward);
    field("unit_id", r.unit_id);
    field("speed_factor", vp?std::optional{vp->speed_factor}:std::optional<std::int64_t>{});
    field("circuits_completed", r.circuits_completed);
    field("circuits_to_complete", r.circuits_to_complete);
    field("turn_route_complete", r.turn_route_complete);
    field("recalled", vp?std::optional{vp->recalled}:std::optional<bool>{});
    std::cout << ",\"plots\":[";
    for (std::size_t i = 0; i < r.plots.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << "{\"x\":" << r.plots[i].x << ",\"y\":" << r.plots[i].y << '}';
    }
    std::cout << ']';
    field("origin_yields", r.origin_yields);
    field("destination_yields", r.destination_yields);
    std::cout << '}';
}
} // namespace trade_json
void game_trade_json(const civ5::saves::GameTradeObservation &s) {
    using namespace trade_json;
    const bool vp = s.layout == civ5::saves::GameStateLayout::vp542_corpus;
    const auto *vp_state=civ5::saves::find_vp_game_trade(s);
    std::cout << "{\"layout\":" << quote(vp ? "vp542_corpus_comparison" : "expansion2_version1")
              << ",\"next_field\":" << quote(vp ? "game_corporations" : "save_game_database")
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false";
    field("version", s.version);
    field("start_offset", s.start_offset);
    field("routes_end_offset", s.routes_end_offset);
    field("matrices_offset", s.matrices_offset);
    field("end_offset", s.end_offset);
    std::cout << ",\"dimensions\":{\"major_slots\":" << s.dimensions.major_slots
              << ",\"yield_slots\":" << s.dimensions.yield_slots << "},\"routes\":[";
    for (std::size_t i = 0; i < s.routes.size(); ++i) {
        if (i)
            std::cout << ',';
        route(s.routes[i]);
    }
    std::cout << ']';
    field("routes_per_player", vp_state?std::optional{vp_state->routes_per_player}:
        std::optional<std::vector<std::vector<std::int64_t>>>{});
    field("technology_difference", s.technology_difference);
    field("policy_difference", vp_state?std::optional{vp_state->policy_difference}:
        std::optional<std::vector<std::int64_t>>{});
    field("next_id", s.next_id);
    std::cout << '}';
}
