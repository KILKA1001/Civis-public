#include "civ5/runtime/bnw_sight_diplomacy_preflight.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
struct Player {
    std::int64_t id=-1,team=-1;
    bool alive=false,human=false,minor=false;
};
bool raw_alive(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_bAlive") {
            require(!found,"Duplicate BNW diplomacy player alive field");
            found=&field;
        }
    require(found && found->values.size()==1 &&
            (found->values[0]==0 || found->values[0]==1),
            "BNW diplomacy player alive field is unavailable");
    return found->values[0]==1;
}
std::vector<Player> players(const LiveSessionState &state) {
    const auto &world=state.world;
    require(world.pregame && world.all_players && world.teams &&
            world.pregame->layout==saves::GameStateLayout::expansion2_version1 &&
            world.all_players->layout==saves::GameStateLayout::expansion2_version1 &&
            world.all_players->players.size()==64 &&
            world.teams->dimensions.civ_teams==63 &&
            world.teams->dimensions.major==22 &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            state.setup.players.size()==64,
            "BNW meet diplomacy live player profile is incomplete");
    const auto &archive=world.pregame->archive;
    const auto mapping=archive.integer_arrays.find("team_types");
    const auto slots=archive.integer_arrays.find("slot_status");
    const auto claims=archive.integer_arrays.find("slot_claims");
    const auto minors=archive.flag_arrays.find("minor_nation_civs");
    require(mapping!=archive.integer_arrays.end() &&
            slots!=archive.integer_arrays.end() &&
            claims!=archive.integer_arrays.end() &&
            minors!=archive.flag_arrays.end() &&
            mapping->second.size()>=64 && slots->second.size()>=64 &&
            claims->second.size()>=64 && minors->second.size()>=64,
            "BNW meet diplomacy saved team/slot/minor arrays are unavailable");
    std::vector<Player> result;
    result.reserve(64);
    for(std::size_t i=0;i<64;++i) {
        const auto &raw=world.all_players->players[i];
        const auto setup=state.setup.players.find(static_cast<std::int64_t>(i));
        require(raw.index==i && raw.saved_player_id==static_cast<std::int64_t>(i) &&
                setup!=state.setup.players.end() &&
                setup->second.team==mapping->second[i] &&
                setup->second.slot_status==slots->second[i] &&
                setup->second.slot_claim==claims->second[i] &&
                setup->second.alive==raw_alive(raw) &&
                setup->second.team>=0 && setup->second.team<64,
                "BNW meet diplomacy player ID or setup view disagrees");
        result.push_back({static_cast<std::int64_t>(i),setup->second.team,
            setup->second.alive,
            setup->second.alive && state.setup.is_human(setup->second),
            minors->second[i]});
    }
    return result;
}
bool team_human(const std::vector<Player> &players,std::int64_t team) {
    for(const auto &p:players)
        if(p.alive && p.team==team && p.human) return true;
    return false;
}
bool team_minor(const std::vector<Player> &players,std::int64_t team) {
    // CvTeam::isMinorCiv scans MAX_MAJOR_CIVS..MAX_PLAYERS; its first alive
    // member of this team decides the result.
    for(std::size_t i=22;i<64;++i) {
        const auto &p=players[i];
        if(!p.alive || p.team!=team) continue;
        return p.minor;
    }
    return false;
}
void push(BnwSightDiplomacyPreflightResult &result,
          BnwMeetDiplomacyCall call,std::int64_t own,std::int64_t other,
          std::int64_t our_player=-1,std::int64_t their_player=-1,
          std::int64_t third=-1) {
    require(result.intents.size()<100000,
            "BNW meet diplomacy intent budget exceeded");
    result.intents.push_back({call,own,other,our_player,their_player,third});
}
void side(BnwSightDiplomacyPreflightResult &result,
          const std::vector<Player> &roster,std::int64_t own,
          std::int64_t other,bool always_war) {
    require(own>=0 && own<63 && other>=0 && other<63 && own!=other,
            "BNW meet diplomacy team pair is invalid");
    // CvTeam::makeHasMet invokes updateTechShare first, then this branch.
    if(always_war && team_human(roster,own)) {
        push(result,BnwMeetDiplomacyCall::declare_war,own,other);
        result.blockers.push_back(
            BnwMeetDiplomacyBlocker::always_war_declare_war);
        throw std::runtime_error(
            "BNW meet always-war requires unported declareWar before diplomacy");
    }
    const auto minor=team_minor(roster,own);
    // CvTeam.cpp:3375-3423: source player loops are bounded by
    // MAX_CIV_PLAYERS (63), while the later queue loop uses MAX_PLAYERS (64).
    for(std::size_t i=0;i<63;++i) {
        const auto &ours=roster[i];
        if(!ours.alive || ours.team!=own) continue;
        for(std::size_t j=0;j<63;++j) {
            const auto &theirs=roster[j];
            if(i==j || !theirs.alive || theirs.team!=other) continue;
            push(result,BnwMeetDiplomacyCall::proximity_our_to_their,
                 own,other,ours.id,theirs.id);
            push(result,BnwMeetDiplomacyCall::proximity_their_to_our,
                 own,other,theirs.id,ours.id);
            if(!minor)
                push(result,BnwMeetDiplomacyCall::first_contact,
                     own,other,ours.id,theirs.id);
            for(std::size_t k=0;k<63;++k)
                if(roster[k].alive && k!=i && k!=j)
                    push(result,BnwMeetDiplomacyCall::third_party_met_someone,
                         own,other,ours.id,theirs.id,roster[k].id);
        }
    }
    if(team_human(roster,other))
        for(const auto &p:roster)
            if(p.alive && p.team==own && !p.human)
                push(result,BnwMeetDiplomacyCall::clear_research_queue,
                     own,other,p.id);
}
} // namespace

BnwSightDiplomacyPreflightResult preflight_bnw_sight_diplomacy(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightDiplomacyPreflightResult result;
    try {
        auto share=apply_bnw_sight_tech_share_detached(
            source,state,request,database,types,defines,identity);
        if(!share.calculated || !share.detached)
            throw std::runtime_error(share.reason.empty()
                ? "BNW diplomacy needs no-grant detached tech share"
                : share.reason);
        const auto roster=players(*share.detached);
        // ResearchContext's lookup follows CvPreGame::GetGameOption's first
        // saved-name match, then the active GameOptions.Default row.
        const auto option=share.detached->setup.game_option_value(
            database,"GAMEOPTION_ALWAYS_WAR");
        const bool always_war=option==1; // CvGame::isOption compares to 1.
        for(const auto &contact:share.contacts) {
            if(!contact.new_contact) continue;
            side(result,roster,contact.revealing_team,contact.other_team,
                 always_war);
            side(result,roster,contact.other_team,contact.revealing_team,
                 always_war);
        }
        result.detached=std::move(share.detached);
        result.calculated=true;
        result.blockers.push_back(
            BnwMeetDiplomacyBlocker::proximity_and_first_contact_ai);
        result.blockers.push_back(
            BnwMeetDiplomacyBlocker::research_queue_and_minor_effects);
        result.blockers.push_back(
            BnwMeetDiplomacyBlocker::gameplay_ui_lua_and_persistence);
        result.reason="BNW meet diplomacy call order traced; mandatory AI, queue, minor, UI and Lua effects remain";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.detached.reset();
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
