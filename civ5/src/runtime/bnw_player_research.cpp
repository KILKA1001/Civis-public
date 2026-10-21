#include "civ5/runtime/bnw_player_research.hpp"
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace civ5::runtime {
namespace {
constexpr auto bnw=content::TechnologyProfile::bnw;
using Dirty=std::pair<std::int64_t,std::uint8_t>;

class GuardedWorld final : public ResearchProgressServices {
  public:
    std::vector<Dirty> dirty;
    std::int64_t science_times100(std::int64_t) override {
        throw ResearchTurnUnavailable("BNW research injected science bypassed owned economy");
    }
    void game_data_and_score_dirty(std::int64_t team) override {
        dirty.emplace_back(team,static_cast<std::uint8_t>(
            BnwResearchDirty::game_data_and_score));
    }
    void acquire_technology(std::int64_t,std::int64_t,std::int64_t) override {
        // CvTeam::setHasTech must update the whole world, invoke Lua and reset
        // repeat techs. TeamTechnologies::set_has_tech alone is insufficient.
        throw ResearchTurnUnavailable(
            "BNW technology grant requires full SetHasTech world/Lua effects");
    }
    void city_specializations_dirty(std::int64_t) override {
        throw ResearchTurnUnavailable("BNW technology city effects unavailable");
    }
    std::int64_t culture_per_tech(std::int64_t) override {
        throw ResearchTurnUnavailable("BNW technology culture effects unavailable");
    }
    void add_culture(std::int64_t,std::int64_t) override {
        throw ResearchTurnUnavailable("BNW technology culture grant unavailable");
    }
};
class GuardedTurn final : public ResearchTurnServices {
  public:
    GuardedTurn(LiveSessionState &state,const BnwPlayerResearchServices &services,
                std::vector<Dirty> &dirty)
        :state_(state),services_(services),dirty_(dirty){}
    void choose_research_ai(std::int64_t player) override {
        if (!services_.choose_ai_on_detached)
            throw ResearchTurnUnavailable("BNW AI research choice is unavailable");
        services_.choose_ai_on_detached(player,state_);
    }
    void choose_research_ui(std::int64_t) override {
        throw ResearchTurnUnavailable("BNW chooseTech UI requires post-commit owner");
    }
    void research_buttons_dirty(std::int64_t team) override {
        dirty_.emplace_back(team,static_cast<std::uint8_t>(
            BnwResearchDirty::research_buttons));
    }
    void unlock_research_achievement(std::int64_t,ResearchAchievement) override {
        // SDK CheckForTechAchievement invokes only gDLL->UnlockAchievement.
        // Steam is outside the standalone milestone.
    }
  private:
    LiveSessionState &state_;
    const BnwPlayerResearchServices &services_;
    std::vector<Dirty> &dirty_;
};
} // namespace

LiveActionResult run_bnw_player_research_phase(
    LiveSessionState &state,std::int64_t player,
    const BnwPlayerResearchRules &rules,
    const BnwPlayerResearchServices &services) {
    if (!services.lua_ready || !services.founder_benefits_read_only ||
        !services.choose_ai_on_detached || !services.dirty_after_commit ||
        !services.lua_ready())
        return {LiveActionOutcome::rejected,
                "BNW research requires Lua, detached AI and post-commit dirty services"};
    if (state.player_research_publication)
        return {LiveActionOutcome::rejected,
                "BNW research has unpublished UI effects"};
    if (state.setup.profile!=bnw || state.player_research.profile!=bnw ||
        state.research_turn.profile!=bnw || state.science.profile!=bnw ||
        state.treasury.profile!=bnw || state.maintenance.profile!=bnw ||
        state.cities.profile!=bnw || rules.research.profile!=bnw ||
        rules.technologies.profile!=bnw || rules.treasury.profile!=bnw ||
        rules.cities.profile!=bnw || rules.maintenance.profile!=bnw ||
        rules.defines.profile()!=content::GlobalDefineProfile::bnw ||
        !rules.research.all_succeeded() || !rules.technologies.all_succeeded() ||
        !state.setup.players.contains(player) ||
        !state.player_research.players.contains(player) ||
        !state.research_turn.players.contains(player) ||
        !state.science.players.contains(player) ||
        !state.treasury.players.contains(player) ||
        !state.maintenance.players.contains(player) ||
        !state.cities.players.contains(player))
        return {LiveActionOutcome::rejected,
                "BNW research requires matching owned player and rules state"};
    const auto team_id=state.setup.players.at(player).team;
    std::size_t team_matches=0;
    for (const auto &team:state.technologies)
        if (team.team()==team_id && team.profile()==bnw &&
            team.slots()==rules.technologies.collection.slots)
            ++team_matches;
    if (team_matches!=1 ||
        state.research_turn.players.at(player).unmapped_researching!=0)
        return {LiveActionOutcome::rejected,
                "BNW research team or saved researching flags are unresolved"};
    try {
        // City/culture phases precede doResearch; refresh the empire happiness
        // dependency on this detached world before querying science/gold.
        if (!state.cities.no_happiness)
            (void)bind_bnw_unhappiness(state.unhappiness,state.setup,state.cities,
                                       rules.unhappiness,rules.defines);
        GuardedWorld world_services;
        MaintenanceWorld economy{state.setup,state.player_research,state.cities,
                                 state.technologies};
        ResearchProgressContext progress{state.player_research,state.setup,
            rules.research,rules.technologies,rules.defines,nullptr,
            state.technologies,world_services,rules.active_team,state.science.no_science};
        GuardedTurn turn_services(state,services,world_services.dirty);
        TreasuryServices income;
        income.founder_benefits_religion=[&](std::int64_t id) {
            if (!services.lua_ready())
                throw ResearchTurnUnavailable("BNW research Lua host became unavailable");
            return services.founder_benefits_read_only(id,state);
        };
        ResearchTurnContext context{state.research_turn,progress,state.science,
            economy,state.treasury,state.maintenance,rules.treasury,rules.cities,
            rules.maintenance,turn_services,rules.active_player,std::move(income)};
        (void)do_research(context,player);
        if (!world_services.dirty.empty())
            state.player_research_publication=BnwPlayerResearchPublication{
                player,std::move(world_services.dirty),0,false};
        return {LiveActionOutcome::committed,{}};
    } catch (const std::exception &error) {
        return {LiveActionOutcome::rejected,
                std::string("BNW research blocked: ")+error.what()};
    }
}

BnwTurnHandler make_bnw_player_research_handler(
    BnwPlayerResearchRules rules,BnwPlayerResearchServices services) {
    const auto publisher=services;
    return {BnwTurnPhase::player_research,
            [rules,services=std::move(services)](LiveSessionState &state,
                                                 std::int64_t player) {
                return run_bnw_player_research_phase(state,player,rules,services);
            },
            [publisher](LiveSession &session) {
                if(!session.state().player_research_publication)
                    return LiveActionResult{LiveActionOutcome::committed,{}};
                return publish_bnw_player_research(session,publisher);
            }};
}

LiveActionResult publish_bnw_player_research(
    LiveSession &session,const BnwPlayerResearchServices &services) {
    const auto &pending=session.state().player_research_publication;
    if (!pending)
        return {LiveActionOutcome::rejected,"BNW research has no UI publication"};
    if (!services.dirty_after_commit || pending->uncertain ||
        pending->next_step>pending->dirty.size())
        return {LiveActionOutcome::rejected,
                "BNW research UI publication unavailable or uncertain"};
    while (session.state().player_research_publication &&
           session.state().player_research_publication->next_step<
           session.state().player_research_publication->dirty.size()) {
        const auto step=*session.state().player_research_publication;
        const auto [team,kind]=step.dirty[step.next_step];
        if (kind>static_cast<std::uint8_t>(BnwResearchDirty::research_buttons))
            return {LiveActionOutcome::rejected,
                    "BNW research UI dirty kind invalid"};
        session.mutable_state().player_research_publication->uncertain=true;
        try {
            services.dirty_after_commit(team,static_cast<BnwResearchDirty>(kind));
        } catch (const std::exception &error) {
            session.mutable_state().player_research_publication->uncertain=true;
            return {LiveActionOutcome::committed,
                    std::string("BNW research committed; UI publication uncertain: ")+
                    error.what()};
        } catch (...) {
            session.mutable_state().player_research_publication->uncertain=true;
            return {LiveActionOutcome::committed,
                    "BNW research committed; UI publication uncertain"};
        }
        ++session.mutable_state().player_research_publication->next_step;
        session.mutable_state().player_research_publication->uncertain=false;
    }
    session.mutable_state().player_research_publication.reset();
    return {LiveActionOutcome::committed,{}};
}

LiveActionResult apply_bnw_player_research(
    LiveSession &session,std::int64_t player,
    const BnwPlayerResearchRules &rules,
    const BnwPlayerResearchServices &services) {
    auto result=apply_live_action(session,[&](LiveSessionState &state) {
        return run_bnw_player_research_phase(state,player,rules,services);
    });
    if (result.outcome==LiveActionOutcome::rejected ||
        !session.state().player_research_publication) return result;
    return publish_bnw_player_research(session,services);
}

LiveActionResult run_bnw_player_turn_with_research(
    LiveSession &session,std::int64_t player,
    std::span<const BnwTurnHandler> other_handlers,
    const BnwPlayerResearchRules &rules,
    const BnwPlayerResearchServices &services) {
    std::vector<BnwTurnHandler> handlers(other_handlers.begin(),other_handlers.end());
    handlers.push_back(make_bnw_player_research_handler(rules,services));
    return run_bnw_player_turn(session,player,handlers);
}

LiveActionResult run_bnw_player_turn_with_gold_and_research(
    LiveSession &session,std::int64_t player,
    std::span<const BnwTurnHandler> other_handlers,
    const BnwPlayerGoldRules &gold_rules,const BnwPlayerGoldServices &gold_services,
    const BnwPlayerResearchRules &research_rules,
    const BnwPlayerResearchServices &research_services) {
    std::vector<BnwTurnHandler> handlers(other_handlers.begin(),other_handlers.end());
    handlers.reserve(handlers.size()+2);
    handlers.push_back(make_bnw_player_gold_handler(gold_rules,gold_services));
    handlers.push_back(make_bnw_player_research_handler(research_rules,research_services));
    // Each factory contributes an after-commit hook. The scheduler publishes
    // every staged UI effect in source phase order after one world commit.
    return run_bnw_player_turn(session,player,handlers);
}

} // namespace civ5::runtime
