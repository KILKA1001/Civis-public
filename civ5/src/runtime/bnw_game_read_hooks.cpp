#include "civ5/runtime/bnw_game_read_hooks.hpp"
#include "civ5/runtime/research_context.hpp"
#include <stdexcept>
#include <string_view>
#include <limits>

namespace civ5::runtime {
void apply_bnw_game_read_scalar_hooks(saves::GameStateObservation &game) {
    if(game.layout!=saves::GameStateLayout::expansion2_version1 ||
       !std::holds_alternative<saves::Expansion2PrefixObservation>(game.prefix))
        throw std::invalid_argument("BNW CvGame::Read hooks require an Expansion2 game state");
    saves::NamedSaveInteger *state=nullptr;
    for(auto &control:game.controls) {
        if(control.name && std::string_view(control.name)=="game_state") {
            if(state) throw std::invalid_argument("Duplicate BNW game state control");
            state=&control;
        }
    }
    if(!state) throw std::invalid_argument("Missing BNW game state control");

    // SDK CvGame.cpp, CvGame::Read: positive autoplay counts become one.
    auto &autoplay=std::get<saves::Expansion2PrefixObservation>(game.prefix).scalars[21];
    if(autoplay>0) autoplay=1;
    // SDK CvEnums.h: GAMESTATE_ON=0, OVER=1, EXTENDED=2.
    if(state->value==1) state->value=2;
}

void apply_bnw_game_read_session_hook(saves::SavedSessionSnapshot &world) {
    if(world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
       world.history.layout!=saves::GameStateLayout::expansion2_version1 ||
       !world.pregame ||
       world.pregame->layout!=saves::GameStateLayout::expansion2_version1)
        throw std::invalid_argument("BNW CvGame::Read sessions require one Expansion2 snapshot");
    const auto type=world.pregame->archive.integers.find("game_type");
    if(type==world.pregame->archive.integers.end() || type->second<0 || type->second>3)
        throw std::invalid_argument("BNW CvGame::Read game type is unavailable");
    // SDK CvEnums.h GAME_NETWORK_MULTIPLAYER=1. Hotseat and email games also
    // increment, because CvGame::Read checks isNetworkMultiPlayer here.
    if(type->second!=1) {
        if(world.history.sessions>=std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("BNW session count exceeds source i32 width");
        ++world.history.sessions;
    }
}

void apply_bnw_game_read_active_player_hook(saves::SavedSessionSnapshot &world,
                                            const ResearchContext &setup) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    if(world.game_state.layout!=bnw || !world.pregame ||
       world.pregame->layout!=bnw || !world.all_players ||
       world.all_players->layout!=bnw ||
       setup.profile!=content::TechnologyProfile::bnw)
        throw std::invalid_argument("BNW active-player hook requires one restored player roster");
    const auto game_type=world.pregame->archive.integers.find("game_type");
    const auto active=world.pregame->archive.integers.find("active_player");
    if(game_type==world.pregame->archive.integers.end() ||
       active==world.pregame->archive.integers.end() ||
       game_type->second<0 || game_type->second>3 ||
       setup.game_type!=game_type->second)
        throw std::invalid_argument("BNW active-player setup is inconsistent");
    // SDK CvDLLUtilDefines.h: 63 civilization players, then barbarian slot 63.
    if(world.all_players->context.strategy.civ_players!=63 ||
       world.all_players->players.size()!=64 || setup.players.size()!=64)
        throw std::invalid_argument("BNW active-player roster has unexpected dimensions");
    if(game_type->second!=0) return; // isGameMultiPlayer(): network, hotseat, PBEM.
    for(std::int64_t id=0;id<63;++id) {
        const auto &saved=world.all_players->players.at(static_cast<std::size_t>(id));
        const auto player=setup.players.find(id);
        if(saved.saved_player_id!=id || player==setup.players.end())
            throw std::invalid_argument("BNW active-player roster ID differs from saved player");
        if(setup.is_human(player->second)) {
            world.pregame->archive.integers["active_player"]=id;
            return;
        }
    }
}
}
