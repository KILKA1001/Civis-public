#include "civ5/runtime/bnw_sight_research_clear.hpp"
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
void validate_queue(const LiveSessionState &state,std::int64_t id,
                    const content::TechnologyRules &rules) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.player_research.profile==content::TechnologyProfile::bnw &&
            state.research_turn.profile==content::TechnologyProfile::bnw &&
            state.research_selection.profile==content::TechnologyProfile::bnw &&
            state.setup.players.contains(id) &&
            state.player_research.players.contains(id) &&
            state.research_turn.players.contains(id) &&
            state.research_selection.players.contains(id) &&
            rules.profile==content::TechnologyProfile::bnw &&
            rules.all_succeeded(),
            "BNW meet research queue owned profile or player is incomplete");
    const auto &queue=state.player_research.players.at(id).queue;
    const auto &flags=state.research_turn.players.at(id);
    require(flags.unmapped_researching==0,
            "BNW meet research flags include unresolved saved technology");
    std::set<std::int64_t> queued;
    for(const auto tech:queue) {
        require(tech>=0 && rules.find(static_cast<std::uint64_t>(tech)),
                "BNW meet research queue technology is outside active rules");
        queued.insert(tech);
    }
    for(const auto tech:flags.researching)
        require(tech>=0 && rules.find(static_cast<std::uint64_t>(tech)),
                "BNW meet researching flag is outside active rules");
    require(queued==flags.researching,
            "BNW meet research queue and researching flags disagree");
}
} // namespace

BnwSightResearchClearResult apply_bnw_sight_research_clear_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightResearchClearResult result;
    try {
        auto diplomacy=preflight_bnw_sight_diplomacy(
            source,state,request,database,types,defines,identity);
        if(!diplomacy.calculated || !diplomacy.detached)
            throw std::runtime_error(diplomacy.reason.empty()
                ? "BNW meet research clear requires detached diplomacy plan"
                : diplomacy.reason);
        auto &copy=*diplomacy.detached;
        const auto rules=content::load_technology_rules(
            database,types,content::TechnologyProfile::bnw,5);
        std::set<std::int64_t> seen;
        const auto active=copy.world.pregame->archive.integers.find("active_player");
        require(active!=copy.world.pregame->archive.integers.end() &&
                copy.setup.players.contains(active->second),
                "BNW meet active player is unavailable for research UI effects");
        const auto active_team=copy.setup.players.at(active->second).team;
        for(const auto &intent:diplomacy.intents) {
            if(intent.call!=BnwMeetDiplomacyCall::clear_research_queue) continue;
            const auto id=intent.recipient_player;
            require(seen.insert(id).second &&
                    copy.setup.players.contains(id) &&
                    copy.setup.players.at(id).alive &&
                    copy.setup.players.at(id).team==intent.recipient_team &&
                    !copy.setup.is_human(copy.setup.players.at(id)),
                    "BNW meet research clear recipient identity changed");
            validate_queue(copy,id,rules);
            // CvPlayer::clearResearchQueue: queue.clear(), then
            // setResearchingTech(false) for every active technology slot.
            copy.player_research.players.at(id).queue.clear();
            if(id==active->second)
                for(const auto tech:copy.research_turn.players.at(id).researching)
                    result.popup_dirty_intents.emplace_back(id,tech);
            copy.research_turn.players.at(id).researching.clear();
            result.cleared_players.push_back(id);
            if(intent.recipient_team==active_team)
                result.active_team_dirty_intents.push_back(intent.recipient_team);
        }
        if(!result.cleared_players.empty()) {
            // The loader compares queue and flags across companion overlays.
            // Building all three here refuses partial/uncheckpointable state.
            result.research_checkpoint=make_research_checkpoint(
                copy.technologies,copy.player_research,copy.setup,rules,identity);
            result.turn_checkpoint=make_research_turn_checkpoint(
                copy.research_turn,copy.setup,rules,identity);
            result.selection_checkpoint=make_research_selection_checkpoint(
                copy.research_selection,copy.player_research,
                copy.research_turn,copy.setup,rules,identity);
        }
        result.detached=std::move(diplomacy.detached);
        result.calculated=true;
        result.reason="BNW meet queue clear is detached; prior diplomacy AI and later minor/UI/Lua effects remain";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.detached.reset();
        result.cleared_players.clear();
        result.research_checkpoint.reset();
        result.turn_checkpoint.reset();
        result.selection_checkpoint.reset();
        result.popup_dirty_intents.clear();
        result.active_team_dirty_intents.clear();
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
