#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/vp_research_selection.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
bool vp(const ResearchSelectionContext &c){return c.setup.profile==TechnologyProfile::vp542;}
const content::TechnologyInfo *info(const ResearchSelectionContext &c,std::int64_t tech){return tech<0?nullptr:c.technologies.find(static_cast<std::uint64_t>(tech));}
const TeamTechnologies &team(const ResearchSelectionContext &c,std::int64_t player) {
    const auto id=c.setup.players.at(player).team;const TeamTechnologies *found=nullptr;
    for(const auto &t:c.teams)if(t.team()==id){if(found)throw std::runtime_error("Duplicate research selection team");found=&t;}
    if(!found||found->profile()!=c.setup.profile||found->slots()!=c.technologies.collection.slots)
        throw std::runtime_error("Research selection team/profile/span mismatch");
    return *found;
}
void validate(const ResearchSelectionContext &c,std::int64_t player) {
    if(c.state.profile!=c.setup.profile||c.turn.profile!=c.setup.profile||c.players.profile!=c.setup.profile||
       c.technologies.profile!=c.setup.profile||!c.technologies.all_succeeded()||
       (vp(c)?(!c.options||!c.options->all_succeeded()):c.options!=nullptr)||
       !c.state.players.contains(player)||!c.turn.players.contains(player)||!c.players.players.contains(player))
        throw std::runtime_error("Research selection requires matching complete player/rules state");
    (void)team(c,player);
}
void dirty(ResearchSelectionContext &c,std::int64_t player){const auto t=c.setup.players.at(player).team;if(t==c.active_team)c.services.research_queue_dirty(t);}
std::int64_t size64(std::size_t n){if(n>INT64_MAX)throw std::overflow_error("Research path exceeds int64");return static_cast<std::int64_t>(n);}
// Sparse cursor preserves list order, holes and source early termination.
struct Prerequisite {std::uint64_t index;std::int64_t tech;};
std::optional<Prerequisite> next(const content::RuleValues &a,std::int64_t limit,std::uint64_t &cursor) {
    if(limit<=0||cursor>=static_cast<std::uint64_t>(limit))return {};
    if(a.default_value!=-1)throw std::runtime_error("Research prerequisite default is not NO_TECH");
    const auto bound=std::min(a.extent,static_cast<std::uint64_t>(limit));
    while(cursor<bound){const auto it=a.values.lower_bound(cursor);
        if(it==a.values.end()||it->first>=bound){cursor=bound;break;}
        cursor=it->first+1;if(it->second!=-1)return Prerequisite{it->first,it->second};
    }
    if(static_cast<std::uint64_t>(limit)>a.extent)throw std::out_of_range("Research prerequisite limit exceeds source extent");
    return {};
}
std::int64_t bnw_path(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech) {
    const auto &t=team(c,player);const auto &queued=c.turn.players.at(player).researching;
    struct Frame {std::int64_t tech,total=1;std::optional<std::int64_t> best;std::uint64_t cursor=0;int phase=0;bool entered=false,waiting=false;
        Frame(std::int64_t id):tech(id){}};
    std::vector<Frame> stack{{tech}};std::map<std::int64_t,std::int64_t> memo;std::set<std::int64_t> active;std::int64_t value=0;
    while(!stack.empty()) {
        auto &f=stack.back();const auto *p=info(c,f.tech);
        if(!f.entered){
            if(!p||t.has_tech(f.tech)||queued.contains(f.tech)){value=0;stack.pop_back();continue;}
            if(memo.contains(f.tech)){value=memo.at(f.tech);stack.pop_back();continue;}
            if(!active.insert(f.tech).second)throw ResearchSelectionUnavailable("Cyclic research prerequisites");
            f.entered=true;
        }
        if(f.waiting){if(f.phase==0)f.total=wide::add(f.total,value);else if(!f.best||value<*f.best)f.best=value;f.waiting=false;}
        const auto child=next(f.phase==0?p->and_prerequisites:p->or_prerequisites,f.phase==0?c.limits.and_count:c.limits.or_count,f.cursor);
        if(child){f.waiting=true;stack.push_back({child->tech});continue;}
        if(f.phase==0){f.phase=1;f.cursor=0;continue;}
        value=wide::add(f.total,f.best.value_or(0));memo[f.tech]=value;active.erase(f.tech);stack.pop_back();
    }
    return value;
}
}
ResearchSelectionState restore_research_selection(const saves::SavedSessionSnapshot &s,const ResearchContext &setup,const content::RulesDatabase &db) {
    const bool is_vp=setup.profile==TechnologyProfile::vp542;
    const auto layout=is_vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!s.pregame||!s.all_players||s.pregame->layout!=layout||s.all_players->layout!=layout)
        throw std::runtime_error("Research selection import profile mismatch");
    ResearchSelectionState result;result.profile=setup.profile;
    auto query=db.query_rows({"SELECT Type FROM Civilizations",{}},[&](const auto &r){result.disabled_technologies[r.text("Type").value_or("")]={};});
    if(!query.all_succeeded())throw std::runtime_error("Research civilization catalog unavailable");
    query=db.query_rows({"SELECT CivilizationType, Technologies.ID FROM Civilization_DisableTechs INNER JOIN Technologies ON TechType=Technologies.Type",{}},[&](const auto &r){
        const auto type=r.text("CivilizationType").value_or("");const auto id=r.integer_at(1);
        if(result.disabled_technologies.contains(type)&&id>=0)result.disabled_technologies.at(type).insert(id);
    });
    if(!query.all_succeeded())throw std::runtime_error("Civilization research restrictions unavailable");
    const auto &keys=s.pregame->slots.string_arrays.at("civilization_keys");
    for(const auto &saved:s.all_players->players){const auto id=saved.saved_player_id;
        if(id<0||!setup.players.contains(id)||saved.core.layout!=layout||static_cast<std::uint64_t>(id)>=keys.size())
            throw std::runtime_error("Research selection import player mismatch");
        ResearchSelectionPlayer p;if(result.disabled_technologies.contains(keys[id]))p.civilization=keys[id];
        if(!is_vp){
            bool found=false;
            for(const auto &f:saved.core.fields)if(f.name&&std::string_view(f.name)=="m_iAdvancedStartPoints") {
                if(found||f.values.size()!=1)throw std::runtime_error("Invalid advanced start research state");
                p.advanced_start_points=f.values.front();found=true;
            }
            if(!found)throw std::runtime_error("Missing advanced start research state");
        }
        if(!result.players.emplace(id,std::move(p)).second)throw std::runtime_error("Duplicate research selection player");
    }
    if(result.players.size()!=setup.players.size())throw std::runtime_error("Research selection roster mismatch");
    return result;
}
bool can_ever_research(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech) {
    validate(c,player);if(vp(c)&&c.no_science)return false;
    const auto *p=info(c,tech);if(!p||p->booleans.at("Disable"))return false;
    const auto &civilization=c.state.players.at(player).civilization;
    if(!civilization)throw ResearchSelectionUnavailable("Research requires a resolved civilization Info");
    if(c.state.disabled_technologies.at(*civilization).contains(tech))return false;
    if(vp(c)&&c.options->enabled("BALANCE_ERA_RESTRICTION")&&c.team_eras.at(c.setup.players.at(player).team)<p->era)return false;
    return c.services.test_all("PlayerCanEverResearch",player,tech).value_or(true);
}
bool can_research(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech,bool trading) {
    validate(c,player);if(!info(c,tech))return false;
    const auto &p=c.turn.players.at(player);
    if(vp(c)?p.cities_founded<=0:(!p.founded_first_city&&c.state.players.at(player).advanced_start_points<0))return false;
    const auto &t=team(c,player);if(t.has_tech(tech))return false;
    if(!research_prerequisites_met(c.technologies,t,tech,c.limits,trading)||!can_ever_research(c,player,tech))return false;
    return c.services.test_all("PlayerCanResearch",player,tech).value_or(true);
}
bool can_research_for_free(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech) {
    if(!vp(c)&&(tech<0||static_cast<std::uint64_t>(tech)>=c.technologies.collection.slots))return false;
    return can_research(c,player,tech);
}
std::int64_t research_path_length(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech) {
    validate(c,player);return vp(c)?vp_research_path_length(c.technologies,team(c,player),c.limits,tech):bnw_path(c,player,tech);
}
std::int64_t research_queue_position(const PlayerResearchState &p,std::int64_t tech) {
    const auto it=std::find(p.queue.begin(),p.queue.end(),tech);return it==p.queue.end()?-1:wide::add(size64(static_cast<std::size_t>(it-p.queue.begin())),1);
}
void clear_research_queue(ResearchSelectionContext &c,std::int64_t player) {
    validate(c,player);c.players.players.at(player).queue.clear();c.turn.players.at(player).researching.clear();dirty(c,player);
}
void pop_research(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech) {
    validate(c,player);auto &queue=c.players.players.at(player).queue;const auto it=std::find(queue.begin(),queue.end(),tech);
    if(it!=queue.end())queue.erase(it);c.turn.players.at(player).researching.erase(tech);dirty(c,player);
}
bool push_research(ResearchSelectionContext &c,std::int64_t player,std::int64_t tech,bool clear) {
    validate(c,player);const auto &t=team(c,player);auto &flags=c.turn.players.at(player).researching;
    struct Frame {std::int64_t tech;bool clear=false,entered=false,has_or=false;int phase=0;std::uint64_t cursor=0;
        std::optional<std::int64_t> selected,length;Frame(std::int64_t id,bool reset=false):tech(id),clear(reset){}};
    std::vector<Frame> stack{{tech,clear}};std::set<std::int64_t> active;
    while(!stack.empty()) {
        auto &f=stack.back();const auto *p=info(c,f.tech);if(!p)return false;
        if(!f.entered){
            if(t.has_tech(f.tech)||flags.contains(f.tech)){stack.pop_back();continue;}
            if(!can_ever_research(c,player,f.tech))return false;
            if(f.clear)clear_research_queue(c,player);
            if(!active.insert(f.tech).second)throw ResearchSelectionUnavailable("Cyclic research prerequisites");
            f.entered=true;
        }
        if(f.phase==0){if(auto pre=next(p->and_prerequisites,c.limits.and_count,f.cursor)){stack.push_back({pre->tech});continue;}
            f.phase=1;f.cursor=0;}
        if(f.phase==1){
            while(auto pre=next(p->or_prerequisites,c.limits.or_count,f.cursor)) {
                f.has_or=true;
                if(t.has_tech(pre->tech)){f.selected=pre->tech;break;}
                if(can_ever_research(c,player,pre->tech)){
                    const auto length=research_path_length(c,player,pre->tech);
                    if(!f.length||length<*f.length){f.selected=pre->tech;f.length=length;}
                }
            }
            f.phase=2;if(f.selected){stack.push_back({*f.selected});continue;}if(f.has_or)return false;
        }
        c.players.players.at(player).queue.push_back(f.tech);flags.insert(f.tech);dirty(c,player);
        active.erase(f.tech);stack.pop_back();
    }
    return true;
}
} // namespace civ5::runtime
