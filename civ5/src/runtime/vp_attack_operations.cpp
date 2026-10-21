#include "civ5/runtime/vp_attack_operations.hpp"
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
const saves::ArmyOperationsNode &child(const saves::ArmyOperationsNode &parent,std::string_view name) {
    const saves::ArmyOperationsNode *found=nullptr;
    for(const auto &item:parent.children)if(item.name&&std::string_view(item.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP operation field: "+std::string(name));
        found=&item;
    }
    if(!found)throw std::runtime_error("Missing VP operation field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const saves::ArmyOperationsNode &parent,std::string_view name) {
    const auto &values=child(parent,name).values;
    if(values.size()!=1)throw std::runtime_error("Invalid VP operation scalar: "+std::string(name));
    return values.front();
}
}
VpAttackOperationsProfileState &ensure_vp_attack_operations_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP attack operations require VP GameCore profile");
    if(auto *profile=world.extensions.find<VpAttackOperationsProfileState>())return *profile;
    return world.extensions.emplace<VpAttackOperationsProfileState>();
}
const VpAttackOperationsProfileState *find_vp_attack_operations_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpAttackOperationsProfileState>():nullptr;
}
void restore_vp_attack_operations_profile(const saves::SavedSessionSnapshot &snapshot,
                                          ResearchContext &world) {
    if(world.profile!=vp||!snapshot.all_players||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP attack operation import requires VP players");
    VpAttackOperationsProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!world.players.contains(id)||
           saved.army_operations.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP attack operation player is not in one VP roster");
        std::vector<VpAttackOperation> attacks;
        const auto &operations=saved.army_operations.operations;
        for(std::size_t i=0;i<operations.size();++i) {
            const auto &operation=operations[i];
            const auto type=scalar(operation,"operation_type");
            // Release-5.4.2 AIOperationTypes: land=2, naval=3, combined=4.
            if(type<2||type>4)continue;
            const auto &base=child(operation,"CvAIOperation");
            VpAttackOperation attack;
            attack.id=scalar(base,"m_iID");
            attack.type=type;
            attack.owner=scalar(base,"m_eOwner");
            attack.enemy=scalar(base,"m_eEnemy");
            attack.state=scalar(base,"m_eCurrentState");
            attack.target_x=scalar(base,"m_iTargetX");
            attack.target_y=scalar(base,"m_iTargetY");
            attack.source_index=i;
            if(attack.owner!=id)
                throw std::runtime_error("VP attack operation owner does not match saved player");
            attacks.push_back(attack);
        }
        if(!imported.players.emplace(id,std::move(attacks)).second)
            throw std::runtime_error("Duplicate VP attack operation player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP attack operation roster");
    ensure_vp_attack_operations_profile(world)=std::move(imported);
}
const VpAttackOperation *vp_first_intrigue_attack_operation(
    const ResearchContext &world,std::int64_t spy_owner,std::int64_t target_player) {
    const auto *profile=find_vp_attack_operations_profile(world);
    if(!profile||!world.players.contains(spy_owner)||!profile->players.contains(spy_owner))
        throw std::runtime_error("Unresolved VP intrigue attack operation owner");
    const auto &attacks=profile->players.at(spy_owner);
    // VP 5.4.2 CvAIOperation.h: AI_OPERATION_STATE_ABORTED is zero.
    // CvPlayer::getFirstAIOperationOfType excludes only that state.
    for(const auto type:{4,3,2})
        for(const auto &attack:attacks)
            if(attack.type==type&&attack.enemy==target_player&&attack.state!=0)
                return &attack;
    return nullptr;
}
} // namespace civ5::runtime
