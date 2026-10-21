#include "civ5/runtime/bnw_fresh_player_entities_uninit.hpp"
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects result;
    result.player_id = 0;
    result.selected_rules_sha256 = "selected-sha";
    result.constructors_complete = true;
    return result;
}
} // namespace

int main() {
    const auto objects = first_player();
    BnwFreshPlayerTailContainersDraft tail;
    tail.selected_rules_sha256 = "selected-sha";
    tail.archive.selected_rules_sha256 = "selected-sha";
    tail.archive.all_registered_member_defaults = true;
    tail.detached_container_effects_complete = true;

    const auto ctor = make_bnw_fresh_unallocated_free_list();
    auto first_reset = project_bnw_fresh_player_entities_uninit(
        objects,tail,"selected-sha",ctor,ctor);
    check(first_reset.detached_effects_complete &&
          first_reset.cities == ctor && first_reset.units == ctor &&
          first_reset.source_order ==
              std::vector<BnwFreshPlayerEntitiesEvent>{
                  {"m_cities.Uninit",false},{"m_units.Uninit",false}} &&
          !first_reset.archive_traversal_verified &&
          !first_reset.initialized_by_gamecore,
          "constructor reset must call both Uninit branches without allocation");

    const auto initialized = make_bnw_fresh_initialized_free_list();
    auto fresh_reinit = project_bnw_fresh_player_entities_uninit(
        objects,tail,"selected-sha",initialized,initialized);
    check(fresh_reinit.detached_effects_complete &&
          !fresh_reinit.cities.array_allocated &&
          !fresh_reinit.units.array_allocated &&
          fresh_reinit.cities.nodes.empty() && fresh_reinit.units.nodes.empty() &&
          fresh_reinit.cities.num_slots == 8 &&
          fresh_reinit.units.num_slots == 8 &&
          fresh_reinit.cities.free_list_head == -1 &&
          fresh_reinit.units.free_list_head == -1 &&
          fresh_reinit.cities.current_id == 8192 &&
          fresh_reinit.units.current_id == 8192 &&
          fresh_reinit.source_order ==
              std::vector<BnwFreshPlayerEntitiesEvent>{
                  {"m_cities.Uninit",true},{"m_units.Uninit",true}} &&
          initialized.array_allocated && initialized.nodes.size() == 8,
          "initialized empty arrays must RemoveAll and delete only detached slots");

    auto city_with_object = initialized;
    city_with_object.nodes[2].occupied = true;
    const auto blocked_city = project_bnw_fresh_player_entities_uninit(
        objects,tail,"selected-sha",
        city_with_object,initialized);
    check(!blocked_city.detached_effects_complete &&
          blocked_city.blocker ==
              BnwFreshPlayerEntitiesBlocker::city_destructor_effects_pending &&
          blocked_city.blocked_at == "m_cities.Uninit" &&
          blocked_city.source_order.empty() &&
          city_with_object.nodes[2].occupied,
          "city destructor must block before any container mutation");

    auto unit_with_object = initialized;
    unit_with_object.nodes[3].occupied = true;
    const auto blocked_unit = project_bnw_fresh_player_entities_uninit(
        objects,tail,"selected-sha",
        initialized,unit_with_object);
    check(!blocked_unit.detached_effects_complete &&
          blocked_unit.blocker ==
              BnwFreshPlayerEntitiesBlocker::unit_destructor_effects_pending &&
          blocked_unit.blocked_at == "m_units.Uninit" &&
          blocked_unit.source_order.empty() &&
          initialized.array_allocated && unit_with_object.nodes[3].occupied,
          "unit destructor must prevent publication of earlier city effects");

    auto malformed = initialized;
    malformed.current_id = 16384;
    check(project_bnw_fresh_player_entities_uninit(
              objects,tail,"selected-sha",
              malformed,initialized).blocker ==
              BnwFreshPlayerEntitiesBlocker::city_container_state_unverified,
          "unproved freelist identity must fail closed");
    check(project_bnw_fresh_player_entities_uninit(
              objects,tail,"wrong-sha",
              ctor,ctor).blocker ==
              BnwFreshPlayerEntitiesBlocker::predecessor_or_rules_invalid,
          "selected rules provenance must match");
    fresh_reinit.rollback();
    check(!fresh_reinit.detached_effects_complete &&
          fresh_reinit.source_order.empty() &&
          fresh_reinit.selected_rules_sha256.empty() &&
          !fresh_reinit.initialized_by_gamecore,
          "rollback must discard both detached containers");
}
