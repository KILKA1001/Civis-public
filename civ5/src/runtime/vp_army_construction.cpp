#include "civ5/runtime/vp_army_construction.hpp"
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
const saves::ArmyOperationsNode &child(const saves::ArmyOperationsNode &parent,std::string_view name) {
    const saves::ArmyOperationsNode *found=nullptr;
    for(const auto &item:parent.children)if(item.name&&std::string_view(item.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP army field: "+std::string(name));
        found=&item;
    }
    if(!found)throw std::runtime_error("Missing VP army field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const saves::ArmyOperationsNode &parent,std::string_view name) {
    const auto &values=child(parent,name).values;
    if(values.size()!=1)throw std::runtime_error("Invalid VP army scalar: "+std::string(name));
    return values.front();
}
}
VpArmyConstructionProfileState &ensure_vp_army_construction_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP army construction requires VP GameCore profile");
    if(auto *profile=world.extensions.find<VpArmyConstructionProfileState>())return *profile;
    return world.extensions.emplace<VpArmyConstructionProfileState>();
}
const VpArmyConstructionProfileState *find_vp_army_construction_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpArmyConstructionProfileState>():nullptr;
}
void restore_vp_army_construction_profile(const saves::SavedSessionSnapshot &snapshot,
                                          ResearchContext &world) {
    if(world.profile!=vp||!snapshot.all_players||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP army construction import requires VP players");
    VpArmyConstructionProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto owner=saved.saved_player_id;
        if(!world.players.contains(owner)||
           saved.army_operations.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP army construction player is not in one VP roster");
        std::vector<VpArmyConstruction> armies;
        const auto &source=saved.army_operations.armies;
        for(std::size_t i=0;i<source.size();++i) {
            const auto &record=source[i];
            VpArmyConstruction army;
            army.id=scalar(record,"m_iID");
            army.owner=scalar(record,"m_eOwner");
            army.type=scalar(record,"m_eType");
            army.source_index=i;
            if(army.owner!=owner)
                throw std::runtime_error("VP army owner does not match saved player");
            const auto &entries=child(record,"m_FormationEntries");
            for(const auto &slot_record:entries.children) {
                if(!slot_record.name||std::string_view(slot_record.name)!="slot")
                    throw std::runtime_error("Unrecognized VP army formation entry");
                const auto required=scalar(slot_record,"m_bIsRequired");
                if(required!=0&&required!=1)
                    throw std::runtime_error("Invalid VP army required-slot flag");
                army.slots.push_back({scalar(slot_record,"m_iUnitID"),required!=0});
            }
            armies.push_back(std::move(army));
        }
        if(!imported.players.emplace(owner,std::move(armies)).second)
            throw std::runtime_error("Duplicate VP army construction player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP army construction roster");
    ensure_vp_army_construction_profile(world)=std::move(imported);
}
bool vp_is_building_army(const ResearchContext &world,std::int64_t owner,VpArmyType type) {
    const auto *profile=find_vp_army_construction_profile(world);
    if(!profile||!world.players.contains(owner)||!profile->players.contains(owner))
        throw std::runtime_error("Unresolved VP army construction player");
    for(const auto &army:profile->players.at(owner))
        if(army.type==static_cast<std::int64_t>(type))
            for(const auto &slot:army.slots)
                if(slot.unit_id==-1&&slot.required)return true;
    return false;
}
} // namespace civ5::runtime
