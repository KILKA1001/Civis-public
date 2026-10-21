#include "civ5/runtime/bnw_game_clock.hpp"
#include <limits>
#include <stdexcept>
#include <string>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr auto bnw_layout=saves::GameStateLayout::expansion2_version1;
constexpr auto bnw_profile=content::TechnologyProfile::bnw;

saves::NamedSaveFlag *score_dirty(saves::GameStateObservation &game) {
    saves::NamedSaveFlag *found=nullptr;
    for (auto &flag:game.flags) {
        if (flag.name && std::string_view(flag.name)=="score_dirty") {
            if (found) return nullptr;
            found=&flag;
        }
    }
    return found;
}
} // namespace

bool BnwGameClockServices::complete() const noexcept {
    return net_debug_log && turn_timer_dirty && game_data_dirty &&
           gameplay_turn_changed && end_turn_timer_reset;
}

LiveActionResult run_bnw_game_clock_phase(
    LiveSessionState &state, const BnwGameClockServices &services) {
    if (!services.complete())
        return {LiveActionOutcome::rejected,
                "BNW game clock requires all engine and UI publication services"};
    if (state.game_clock.pending)
        return {LiveActionOutcome::rejected,
                "BNW game clock has an unpublished turn change"};
    if (state.world.game_state.layout!=bnw_layout ||
        state.setup.profile!=bnw_profile ||
        state.maintenance.profile!=bnw_profile ||
        !state.world.pregame || state.world.pregame->layout!=bnw_layout ||
        !std::holds_alternative<saves::Expansion2PrefixObservation>(
            state.world.game_state.prefix))
        return {LiveActionOutcome::rejected,
                "BNW game clock requires matching decoded GameCore state"};
    auto &pregame=state.world.pregame->archive.integers;
    const auto saved_turn=pregame.find("game_turn");
    auto *dirty=score_dirty(state.world.game_state);
    if (saved_turn==pregame.end() || !dirty)
        return {LiveActionOutcome::rejected,
                "BNW game clock is missing saved turn or score flag"};
    auto &prefix=std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix);
    const auto elapsed=prefix.scalars[1]; // elapsed_game_turns in Expansion2PrefixObservation
    if (state.setup.game_turn<0 || state.maintenance.elapsed_turns<0 ||
        state.setup.game_turn!=saved_turn->second ||
        state.maintenance.elapsed_turns!=elapsed)
        return {LiveActionOutcome::rejected,
                "BNW game clock owner and decoded mirrors disagree"};
    if (state.setup.game_turn==std::numeric_limits<std::int64_t>::max() ||
        elapsed==std::numeric_limits<std::int64_t>::max())
        return {LiveActionOutcome::rejected,"BNW game clock would overflow"};

    const auto next_turn=state.setup.game_turn+1;
    // setGameTurn: CvPreGame, score flag, m_sentAutoMoves; UI/engine work is
    // staged until commit. Then incrementElapsedGameTurns updates both owners.
    state.setup.game_turn=next_turn;
    saved_turn->second=next_turn;
    dirty->value=true;
    state.game_clock.sent_auto_moves=false;
    state.maintenance.elapsed_turns=elapsed+1;
    prefix.scalars[1]=elapsed+1;
    state.game_clock.pending=BnwGameClockPublication{next_turn,0,false};
    return {LiveActionOutcome::committed,{}};
}

BnwTurnHandler make_bnw_game_clock_handler(BnwGameClockServices services) {
    const auto publisher=services;
    return {BnwTurnPhase::game_clock,
            [services=std::move(services)](LiveSessionState &state,std::int64_t player) {
                if (player!=-1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW game clock is a game phase"};
                return run_bnw_game_clock_phase(state,services);
            },
            [publisher](LiveSession &session) {
                if(!session.state().game_clock.pending)
                    return LiveActionResult{LiveActionOutcome::committed,{}};
                return publish_bnw_game_clock(session,publisher);
            }};
}

LiveActionResult publish_bnw_game_clock(
    LiveSession &session, const BnwGameClockServices &services) {
    if (!services.complete())
        return {LiveActionOutcome::rejected,
                "BNW game clock publication services are incomplete"};
    const auto &pending=session.state().game_clock.pending;
    if (!pending)
        return {LiveActionOutcome::rejected,
                "BNW game clock has no committed turn to publish"};
    if (pending->uncertain)
        return {LiveActionOutcome::rejected,
                "BNW game clock publication is uncertain after callback failure"};
    if (pending->next_step>5 || pending->turn!=session.state().setup.game_turn ||
        !session.state().world.pregame ||
        session.state().world.pregame->archive.integers.find("game_turn")==
            session.state().world.pregame->archive.integers.end() ||
        session.state().world.pregame->archive.integers.at("game_turn")!=pending->turn)
        return {LiveActionOutcome::rejected,
                "BNW game clock publication disagrees with committed turn"};
    while (session.state().game_clock.pending &&
           session.state().game_clock.pending->next_step<5) {
        const auto publication=*session.state().game_clock.pending;
        session.mutable_state().game_clock.pending->uncertain=true;
        try {
            switch (publication.next_step) {
            case 0: {
                const auto message="Game Turn "+std::to_string(publication.turn)+"\n";
                services.net_debug_log(message);
                break;
            }
            case 1: services.turn_timer_dirty();break;
            case 2: services.game_data_dirty();break;
            case 3: services.gameplay_turn_changed(publication.turn);break;
            case 4: services.end_turn_timer_reset();break;
            default: throw std::logic_error("Invalid BNW game clock publication step");
            }
        } catch (const std::exception &error) {
            session.mutable_state().game_clock.pending->uncertain=true;
            return {LiveActionOutcome::committed,
                    std::string("BNW clock committed; publication uncertain: ")+error.what()};
        } catch (...) {
            session.mutable_state().game_clock.pending->uncertain=true;
            return {LiveActionOutcome::committed,
                    "BNW clock committed; publication uncertain"};
        }
        ++session.mutable_state().game_clock.pending->next_step;
        session.mutable_state().game_clock.pending->uncertain=false;
    }
    session.mutable_state().game_clock.pending.reset();
    return {LiveActionOutcome::committed,{}};
}

LiveActionResult advance_bnw_game_clock(
    LiveSession &session, const BnwGameClockServices &services) {
    auto result=apply_live_action(session,[&](LiveSessionState &state) {
        return run_bnw_game_clock_phase(state,services);
    });
    if (result.outcome==LiveActionOutcome::rejected) return result;
    return publish_bnw_game_clock(session,services);
}

LiveActionResult run_bnw_game_turn_with_clock(
    LiveSession &session, std::span<const BnwTurnHandler> other_handlers,
    const BnwGameClockServices &services) {
    std::vector<BnwTurnHandler> handlers;
    handlers.reserve(other_handlers.size()+1);
    handlers.insert(handlers.end(),other_handlers.begin(),other_handlers.end());
    handlers.push_back(make_bnw_game_clock_handler(services));
    return run_bnw_game_turn(session,handlers);
}

} // namespace civ5::runtime
