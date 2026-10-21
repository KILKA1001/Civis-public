#pragma once
#include "civ5/runtime/bnw_activation_connections.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
namespace civ5::runtime {
struct BnwBuilderRuntimeLogging {bool logging,ai_logging,builder_ai_logging;};
struct BnwBuilderTaskingPlayerReadState {std::int32_t num_cities=-1;bool target_is_null=true;std::uint64_t updates=0;};
class BnwActivationBuilderOwner {
 public:
    const auto &players() const noexcept{return players_;}
    const auto &city_yields()const noexcept{return city_yields_;}
 private:
    friend BnwActivationBuilderOwner bind_bnw_activation_builder_original_import(const saves::SavedSession &,const SessionCheckpointIdentity &,BnwBuilderRuntimeLogging);
    friend LiveActionResult update_bnw_activation_builder_tasking(LiveSessionState &,BnwActivationBuilderOwner &,const BnwActivationConnectionsOwner &,const BnwCityStrategyYieldRules &,std::int64_t);
    SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};bool logging_=false;
    std::map<std::int64_t,BnwBuilderTaskingPlayerReadState> players_;
    BnwCityStrategyYieldOwner city_yields_;
};
// Actual Read reconstructs numCities=-1,target=NULL once. Logging is the real
// standalone runtime config conjunction from Init, never a saved guessed flag.
BnwActivationBuilderOwner bind_bnw_activation_builder_original_import(
    const saves::SavedSession &,const SessionCheckpointIdentity &,BnwBuilderRuntimeLogging);
// Complete Update branch for fewer than two actual connectable cities: clear
// route-work vector, store actual roster count and UpdateBestYields for each
// current city in native FLTA slot order. Route/logging effects remain gates.
// State+owner publish atomically on the supplied private draft only.
LiveActionResult update_bnw_activation_builder_tasking(LiveSessionState &,
    BnwActivationBuilderOwner &,const BnwActivationConnectionsOwner &,
    const BnwCityStrategyYieldRules &,std::int64_t player);
}
