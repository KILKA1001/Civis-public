#include "civ5/runtime/bnw_sight_tech_share.hpp"
#include "civ5/content/technology_rules.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {return s?std::string_view(s):std::string_view();}
std::int64_t field(const saves::TeamObservation &team,std::string_view key) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:team.fields) if(name(item.name)==key) {
        require(!found,"Duplicate BNW tech-share team field");found=&item;
    }
    require(found,"Missing BNW tech-share team field");return found->value;
}
const std::vector<std::int64_t> &array(const saves::TeamObservation &team,
                                       std::string_view key,std::size_t width) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:team.arrays) if(name(item.name)==key) {
        require(!found,"Duplicate BNW tech-share team array");found=&item;
    }
    require(found && found->columns==1 && found->values.size()==width,
            "BNW tech-share team array width is unavailable");
    return found->values;
}
struct TeamInputs {
    const saves::TeamObservation &raw;
    const TeamTechnologies &technologies;
    const std::vector<std::int64_t> &met;
    const std::vector<std::int64_t> &share;
    bool alive=false;
};
std::vector<TeamInputs> inputs(const LiveSessionState &state,
                               const content::TechnologyRules &rules) {
    require(state.world.teams &&
            state.world.teams->layout==saves::GameStateLayout::expansion2_version1 &&
            state.world.teams->dimensions.teams==64 &&
            state.world.teams->dimensions.civ_teams==63 &&
            state.world.teams->teams.size()==64 &&
            state.technologies.size()==64 && rules.all_succeeded() &&
            rules.profile==content::TechnologyProfile::bnw &&
            rules.collection.slots>0 && rules.collection.slots<=INT32_MAX,
            "BNW tech-share team/technology profile is incomplete");
    std::vector<TeamInputs> result;
    result.reserve(64);
    for(std::size_t i=0;i<64;++i) {
        const auto &team=state.world.teams->teams[i];
        const auto &tech=state.technologies[i];
        require(team.index==i && field(team,"id")==static_cast<std::int64_t>(i) &&
                tech.team()==static_cast<std::int64_t>(i) &&
                tech.profile()==content::TechnologyProfile::bnw &&
                tech.slots()==rules.collection.slots && tech.remap().all_mapped(),
                "BNW tech-share team identity, slots or hash remap disagree");
        const auto alive_count=field(team,"alive_count");
        require(alive_count>=0 && alive_count<=64,
                "BNW tech-share team alive count is invalid");
        const auto &met=array(team,"has_met",64);
        const auto &share=array(team,"tech_share",64);
        for(const auto flag:met)
            require(flag==0 || flag==1,"BNW tech-share has_met is not boolean");
        for(const auto count:share)
            require(count>=0 && count<=INT32_MAX,
                    "BNW tech-share count is outside SDK range");
        result.push_back({team,tech,met,share,alive_count>0});
    }
    return result;
}
BnwSightTechShareCheck evaluate(const std::vector<TeamInputs> &teams,
                                const content::TechnologyRules &rules,
                                std::int64_t recipient) {
    require(recipient>=0 && recipient<63,
            "BNW tech-share recipient is outside civilization teams");
    const auto &own=teams.at(static_cast<std::size_t>(recipient));
    BnwSightTechShareCheck result;
    result.recipient_team=recipient;
    // CvTeam::updateTechShare(Tech) repeats this threshold scan for each
    // unknown technology. Keeping it once is equivalent while no grant runs.
    for(std::size_t i=0;i<64;++i)
        if(own.share[i]>0) {result.best_share=static_cast<std::int64_t>(i+1);break;}
    for(std::uint64_t tech=0;tech<rules.collection.slots;++tech) {
        require(rules.find(tech),
                "BNW tech-share SDK tech slot is absent from active rules");
        ++result.technologies_examined;
        if(own.technologies.has_tech(static_cast<std::int64_t>(tech)) ||
           result.best_share<0) continue;
        std::int64_t count=0;
        for(std::size_t other=0;other<63;++other) {
            const auto &candidate=teams[other];
            if(candidate.alive &&
               candidate.technologies.has_tech(static_cast<std::int64_t>(tech)) &&
               own.met[other]==1) ++count;
        }
        if(count>=result.best_share) {
            result.first_grant_tech=static_cast<std::int64_t>(tech);
            result.first_grant_contacts=count;
            break;
        }
    }
    return result;
}
} // namespace

BnwSightTechShareResult apply_bnw_sight_tech_share_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightTechShareResult result;
    try {
        auto meet=apply_bnw_sight_meets_detached(
            source,state,request,database,types,defines,identity);
        if(!meet.calculated || !meet.detached)
            throw std::runtime_error(meet.reason.empty()
                ? "BNW tech-share needs a detached team meet state"
                : meet.reason);
        std::size_t new_contacts=0;
        const BnwSightMeetTransition *contact=nullptr;
        for(const auto &item:meet.contacts)
            if(item.new_contact) {++new_contacts;contact=&item;}
        require(new_contacts<=1,
                "BNW tech-share needs interleaved flags for multiple new contacts");
        if(contact) {
            const auto rules=content::load_technology_rules(
                database,types,content::TechnologyProfile::bnw,5);
            const auto teams=inputs(*meet.detached,rules);
            // Exactly one new pair: A's has_met[B] is the same immediately
            // after its first makeHasMet as in the detached final pair. No
            // grant means B's later evaluation sees unchanged tech flags.
            for(const auto team:{contact->revealing_team,contact->other_team}) {
                auto check=evaluate(teams,rules,team);
                result.checks.push_back(check);
                if(check.first_grant_tech)
                    throw std::runtime_error(
                        "BNW tech-share would grant technology via unported setHasTech effects");
            }
        }
        result.contacts=std::move(meet.contacts);
        result.detached=std::move(meet.detached);
        result.calculated=true;
        result.reason="BNW tech-share has no grant in this detached contact; diplomacy, Lua, UI and persistence remain";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.detached.reset();
        result.contacts.clear();
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
