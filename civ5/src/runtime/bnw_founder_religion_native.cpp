#include "civ5/runtime/bnw_founder_religion_native.hpp"
#include <limits>
#include <string_view>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int32_t no_religion=-1,pantheon=0;
void need(bool condition,const char *reason) {if(!condition)throw std::runtime_error(reason);}
void native_integer(std::int64_t value) {
    need(value>=INT32_MIN && value<=INT32_MAX,"BNW religion value exceeds source native int");
}
std::int64_t owner(const saves::PlotObservation &plot) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:plot.fields)if(field.name && std::string_view(field.name)=="owner") {
        need(!found,"BNW founder religion duplicate raw plot owner");found=&field;
    }
    need(found,"BNW founder religion raw plot owner is unavailable");return found->value;
}
}
BnwFounderReligionReadModel BnwFounderReligionReadModel::bind(const LiveSessionState &state) {
    BnwFounderReligionReadModel result;
    const auto &world=state.world;
    const auto &religions=world.religion_culture;
    need(state.setup.profile==content::TechnologyProfile::bnw &&
         world.game_state.layout==bnw && religions.layout==bnw &&
         religions.religions_version && *religions.religions_version>=2 &&
         *religions.religions_version<=4 &&
         religions.start_offset<religions.culture_offset &&
         religions.culture_offset<=religions.end_offset &&
         religions.end_offset<=world.payload_end_offset,
         "BNW founder religion serialized list boundary is unavailable");
    for(const auto &[id,player]:state.setup.players) {
        native_integer(id);need(id>=0,"BNW founder religion player identity is invalid");
        result.players_.insert(id);
    }
    need(!result.players_.empty(),"BNW founder religion player roster is unavailable");
    for(const auto &entry:religions.religions) {
        need(entry.version && *entry.version>=1 && *entry.version<=4,
             "BNW founder religion entry version is unavailable");
        native_integer(entry.religion);native_integer(entry.founder);
        native_integer(entry.holy_city_x);native_integer(entry.holy_city_y);
        need(entry.founder==-1 || result.players_.contains(entry.founder),
             "BNW founder religion founder is outside owned roster");
        need(*entry.version>=2 || !entry.pantheon,
             "BNW legacy religion pantheon disagrees with native Read default");
        // Preserve duplicates and source order: CreatedByPlayer selects LAST,
        // while GetReligion selects FIRST. No sorting or deduplication.
        result.religions_.push_back(entry);
    }
    need(world.map_header.layout==bnw && world.map_projection &&
         world.map_plots && world.map_plots->layout==bnw,
         "BNW founder religion requires its current owned map");
    const auto &map=*world.map_projection;
    result.map_={map.grid,map.wrap_x,map.wrap_y,bnw};result.map_.validate();
    need(map.grid.width==world.map_header.grid.width &&
         map.grid.height==world.map_header.grid.height &&
         map.wrap_x==world.map_header.wrap_x && map.wrap_y==world.map_header.wrap_y &&
         map.cells.size()==map.grid.size() && world.map_plots->plots.size()==map.cells.size(),
         "BNW founder religion map dimensions disagree");
    for(std::size_t i=0;i<map.cells.size();++i) {
        const auto &cell=map.cells[i];const auto &raw=world.map_plots->plots[i];
        const auto xy=map.grid.coordinates(i);
        need(cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
             raw.index==i && raw.x==xy[0] && raw.y==xy[1] &&
             cell.owner && *cell.owner==owner(raw),
             "BNW founder religion plot identity/ownership is unresolved or differs");
        need(*cell.owner==-1 || result.players_.contains(*cell.owner),
             "BNW founder religion plot owner is outside owned roster");
        result.plot_owners_.push_back(*cell.owner);
    }
    return result;
}
std::int32_t BnwFounderReligionReadModel::religion_created_by_player(std::int64_t player) const {
    need(players_.contains(player),"BNW religion creator query has no owned player");
    auto result=no_religion;
    for(const auto &entry:religions_)if(entry.founder==player && !entry.pantheon)
        result=static_cast<std::int32_t>(entry.religion);
    return result;
}
const saves::ReligionObservation *BnwFounderReligionReadModel::religion(
    std::int32_t religion,std::int64_t player) const {
    for(const auto &entry:religions_) {
        if(entry.religion==religion && entry.religion==pantheon) {
            if(entry.founder==player)return &entry;
        } else if(entry.religion==religion)return &entry;
    }
    return nullptr;
}
bool BnwFounderReligionReadModel::eligible_for_founder_benefits(
    std::int32_t religion_id,std::int64_t player) const {
    need(players_.contains(player),"BNW founder eligibility query has no owned player");
    const auto *entry=religion(religion_id,-1);
    if(!entry)return false;
    const auto plot=map_.plot(entry->holy_city_x,entry->holy_city_y);
    return entry->founder==player && !entry->pantheon && plot &&
        plot_owners_.at(static_cast<std::size_t>(*plot))==player;
}
BnwReligionHappinessQuote quote_bnw_happiness_from_religion(
    const LiveSessionState &state,const SavedContentProfile &selected,
    const SessionCheckpointIdentity &identity,BnwGameplayLuaEpochOwner &owner,
    std::int64_t player) {
    BnwReligionHappinessQuote result;
    try {
        auto lua=quote_bnw_founder_benefits_lua(state,selected,identity,owner,player);
        if(lua.status==BnwFounderBenefitsLuaStatus::blocked || !lua.epoch) {
            result.blocker=lua.blocker;return result;
        }
        need(lua.epoch->world->setup.profile==content::TechnologyProfile::bnw &&
             lua.epoch->world->setup.players.contains(player),
             "BNW founder callback changed its world/player profile");
        std::optional<BnwFounderReligionReadModel> model;
        std::int32_t religion_id=no_religion;
        if(lua.status==BnwFounderBenefitsLuaStatus::native_fallback_required) {
            need(!lua.religion,"BNW native fallback has an unexpected Lua override");
            model=BnwFounderReligionReadModel::bind(*lua.epoch->world);
            const auto created=model->religion_created_by_player(player);
            if(model->eligible_for_founder_benefits(created,player))religion_id=created;
            result.native_fallback_used=true;
        } else {
            need(lua.status==BnwFounderBenefitsLuaStatus::override_quoted && lua.religion,
                 "BNW founder callback override is unavailable");
            religion_id=*lua.religion;
        }
        result.founder_resolved=true;result.founder_religion=religion_id;
        if(religion_id==no_religion) {
            result.happiness_quoted=true;result.happiness=0;
        } else {
            if(!model)model=BnwFounderReligionReadModel::bind(*lua.epoch->world);
            if(!model->religion(religion_id,-1)) {
                result.happiness_quoted=true;result.happiness=0;
            }
        }
        result.epoch=std::move(lua.epoch);
        result.blocker=result.happiness_quoted?
            "BNW religious happiness zero branch quoted; aggregate happiness and atomic VM/world publication remain":
            "BNW founded religion requires peace state, resolved founder beliefs, following-city and peaceful-foreign-follower owners";
    } catch(const std::exception &error) {
        result={};result.blocker=error.what();
    }
    return result;
}
}
