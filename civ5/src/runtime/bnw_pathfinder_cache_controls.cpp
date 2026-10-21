#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<BnwActivationFinder,6> source_setup_order{
    BnwActivationFinder::common,BnwActivationFinder::ignore_units,
    BnwActivationFinder::route,BnwActivationFinder::water_route,
    BnwActivationFinder::influence,BnwActivationFinder::tactical};
void need(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
void validate(const LiveSessionState &state,const BnwPathfinderCacheControlOwner &owner,std::int64_t player) {
    need(owner.initialized && owner.identity.payload_sha256.size()==64 && owner.identity.rules_sha256.size()==64,
         "Pathfinder cache controls are unknown or uninitialized");
    need(owner.initialization_order.size()==source_setup_order.size() &&
         std::equal(owner.initialization_order.begin(),owner.initialization_order.end(),source_setup_order.begin()),
         "Pathfinder source control Initialize/setup proof differs");
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.pregame &&
         state.world.pregame->layout==bnw && state.world.map_header.layout==bnw &&
         state.world.map_header.guid==owner.map_guid &&
         state.world.map_header.grid.width==owner.grid.width &&
         state.world.map_header.grid.height==owner.grid.height &&
         state.world.map_header.wrap_x==owner.wrap_x && state.world.map_header.wrap_y==owner.wrap_y,
         "Pathfinder cache control map/profile binding differs");
    need(state.setup.game_type==0 && state.world.pregame->archive.integers.at("game_type")==0,
         "Pathfinder activation cache scope supports only sequential SP");
    need(state.setup.game_turn>=0 && state.setup.game_turn==state.world.pregame->archive.integers.at("game_turn") &&
         player>=0 && player<64 && state.setup.players.at(player).alive,
         "Pathfinder cache scope player or clocks differ");
}
}
bool BnwAStarCacheControl::set_mp_cache_safe(bool state) noexcept {
    const auto previous=mp_cache_safe_;
    if(state!=mp_cache_safe_) {force_reset_=true;mp_cache_safe_=state;}
    return previous;
}
void BnwAStarCacheControl::set_data(std::optional<std::uint64_t> stable_identity) noexcept {
    if(data_invalidates_ && data_!=stable_identity)force_reset_=true;
    data_=stable_identity;
}
bool BnwAStarCacheControl::permit_requested_reuse(bool requested,bool network_multiplayer,
    std::int64_t start_x,std::int64_t start_y,std::int64_t info) const noexcept {
    return requested && !force_reset_ && start_x_==start_x && start_y_==start_y && info_==info &&
           !(network_multiplayer && !mp_cache_safe_);
}
void BnwPathfinderCacheControlOwner::initialize_source_controls() {
    need(!initialized && scopes.empty(),"Pathfinder controls cannot reset a retained owner");
    need(grid.width>0 && grid.height>0 && grid.width<=INT32_MAX && grid.height<=INT32_MAX,
         "Pathfinder control Initialize needs valid source dimensions");
    auto draft=*this;
    for(const auto finder:source_setup_order) {
        auto &control=draft.at(finder);
        control={}; // new source ctor + Initialize controls, not existing cache restoration
        control.data_invalidates_=finder==BnwActivationFinder::common ||
            finder==BnwActivationFinder::ignore_units || finder==BnwActivationFinder::tactical;
        draft.initialization_order.push_back(finder);
    }
    draft.initialized=true;*this=std::move(draft);
}
BnwPathfinderCacheControlOwner make_bnw_pathfinder_cache_control_import_owner(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    need(source.source.container==saves::SaveContainerKind::civ5 &&
         identify_bytes(source.source.bytes).sha256==identity.payload_sha256 && identity.rules_sha256.size()==64 &&
         source.snapshot.map_header.layout==bnw && source.snapshot.pregame && source.snapshot.pregame->layout==bnw &&
         source.snapshot.map_plots && source.snapshot.map_plots->layout==bnw,
         "Pathfinder control initialization needs the original complete BNW import");
    BnwPathfinderCacheControlOwner owner;
    owner.identity=identity;owner.map_guid=source.snapshot.map_header.guid;owner.grid=source.snapshot.map_header.grid;
    owner.wrap_x=source.snapshot.map_header.wrap_x;owner.wrap_y=source.snapshot.map_header.wrap_y;
    owner.initialize_source_controls();return owner;
}
void enter_bnw_pathfinder_cache_scope(const LiveSessionState &state,
    BnwPathfinderCacheControlOwner &owner,std::int64_t player) {
    validate(state,owner,player);
    need(owner.next_scope>0 && owner.next_scope<std::numeric_limits<std::uint64_t>::max(),
         "Pathfinder scope identity overflow");
    auto draft=owner;
    BnwPathfinderCacheScope scope{draft.next_scope++,player,state.setup.game_turn,{}};
    for(std::size_t i=0;i<draft.controls.size();++i)
        scope.previous[i]=draft.controls[i].set_mp_cache_safe(true);
    draft.scopes.push_back(scope);owner=std::move(draft);
}
void restore_bnw_pathfinder_cache_scope(const LiveSessionState &state,
    BnwPathfinderCacheControlOwner &owner,std::int64_t player) {
    validate(state,owner,player);
    need(!owner.scopes.empty(),"Pathfinder cache restore has no corresponding enter");
    const auto scope=owner.scopes.back();
    need(scope.player==player && scope.turn==state.setup.game_turn,
         "Pathfinder cache restore belongs to another player/turn or nested scope");
    auto draft=owner;
    for(std::size_t i=0;i<draft.controls.size();++i)
        draft.controls[i].set_mp_cache_safe(scope.previous[i]);
    draft.scopes.pop_back();owner=std::move(draft);
}
void force_reset_bnw_update_moves_common_cache(const LiveSessionState &state,
    BnwPathfinderCacheControlOwner &owner,std::int64_t player) {
    validate(state,owner,player);
    need(owner.scopes.empty(),"updateMoves cannot enter an unfinished activation cache scope");
    owner.at(BnwActivationFinder::common).force_reset_cache();
}
} // namespace civ5::runtime
