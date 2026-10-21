#include "civ5/runtime/bnw_player_flavor_recipients.hpp"
#include "civ5/runtime/live_session.hpp"
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
// CvEnums.h SlotStatus: SS_COMPUTER=1, SS_TAKEN=3.
constexpr std::int64_t computer=1,taken=3;
static_assert(std::is_nothrow_move_assignable_v<BnwPlayerFlavorRecipients>);
void require(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
std::int64_t core_id(const saves::PlayerCoreObservation &core) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(field.name && std::string_view(field.name)=="m_eID") {
            require(!found,"Duplicate BNW player flavor ID");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW player flavor ID unavailable");
    return found->values.front();
}
template<class Block> void require_block(const std::vector<Block> &blocks,
                                          std::string_view name) {
    bool found=false;
    for(const auto &block:blocks)
        if(block.name && std::string_view(block.name)==name) {
            require(!found && block.start_offset<block.end_offset,
                    "BNW player flavor target block invalid or duplicated");
            found=true;
        }
    require(found,"BNW player flavor target block unavailable");
}
BnwPlayerFlavorRecipients collect(const LiveSessionState &state) {
    const auto &world=state.world;
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw,
            "BNW player flavor registrations require matching owned profiles");
    const auto &all=*world.all_players;
    const auto player_count=all.context.player_count;
    const auto barbarian=all.context.strategy.civ_players;
    // CvDLLUtilDefines.h: MAX_CIV_PLAYERS=63, MAX_PLAYERS=64, and
    // BARBARIAN_PLAYER=MAX_CIV_PLAYERS for this Expansion2 wire profile.
    require(player_count==64 && barbarian==63 &&
            all.players.size()==player_count,
            "BNW player flavor source roster dimensions are unverified");
    const auto status_field=world.pregame->archive.integer_arrays.find("slot_status");
    require(status_field!=world.pregame->archive.integer_arrays.end() &&
            status_field->second.size()>=player_count,
            "BNW player flavor saved slot statuses unavailable");
    BnwPlayerFlavorRecipients result;
    std::set<std::int64_t> ids;
    for(const auto &saved:all.players) {
        const auto id=saved.saved_player_id;
        require(id>=0 && static_cast<std::uint64_t>(id)<player_count &&
                ids.insert(id).second && saved.core.layout==bnw &&
                saved.strategy.layout==bnw && saved.knowledge.layout==bnw &&
                core_id(saved.core)==id,
                "BNW player flavor saved player identity/profile mismatch");
        const auto setup=state.setup.players.find(id);
        require(setup!=state.setup.players.end() &&
                setup->second.slot_status==
                    status_field->second.at(static_cast<std::size_t>(id)) &&
                setup->second.slot_status>=0 && setup->second.slot_status<=4,
                "BNW player flavor live slot status differs from saved setup");
        auto &recipients=result.by_player[id];
        if(id==static_cast<std::int64_t>(barbarian) ||
           (setup->second.slot_status!=computer &&
            setup->second.slot_status!=taken)) continue;
        require_block(saved.knowledge.blocks,"flavor_manager");
        require_block(saved.knowledge.blocks,"player_techs");
        require_block(saved.strategy.blocks,"policies");
        require_block(saved.strategy.blocks,"wonder_production_ai");
        recipients={BnwPlayerFlavorRecipient::technology_ai,
                    BnwPlayerFlavorRecipient::policy_ai,
                    BnwPlayerFlavorRecipient::wonder_production_ai};
    }
    require(ids.size()==player_count,
            "BNW player flavor roster has missing players");
    result.current=true;
    return result;
}
} // namespace

void rebuild_bnw_player_flavor_recipients(LiveSessionState &state) {
    state.player_flavor_recipients.current=false;
    auto replacement=collect(state);
    state.player_flavor_recipients=std::move(replacement);
}

const std::vector<BnwPlayerFlavorRecipient> &bnw_player_flavor_recipients(
    const LiveSessionState &state,std::int64_t player) {
    const auto &cache=state.player_flavor_recipients;
    require(cache.current,"BNW player flavor registrations are not current");
    const auto source=collect(state);
    require(cache.by_player==source.by_player,
            "BNW player flavor registrations became stale");
    const auto found=cache.by_player.find(player);
    require(found!=cache.by_player.end(),
            "BNW player flavor recipient owner unavailable");
    return found->second;
}

} // namespace civ5::runtime
