#include "civ5/runtime/bnw_meet_proximity_stage.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t proximity(const saves::SavedSessionSnapshot &world,
                       std::int64_t player,std::int64_t other) {
    require(world.all_players && player>=0 && player<63 &&
            other>=0 && other<63 && player!=other &&
            world.all_players->players.size()==64,
            "BNW meet proximity player roster is unavailable");
    const auto &saved=world.all_players->players.at(
        static_cast<std::size_t>(player));
    require(saved.saved_player_id==player,
            "BNW meet proximity player identity changed");
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:saved.core.fields)
        if(field.name && std::string_view(field.name)==
           "m_aiProximityToPlayer") {
            require(!found,"Duplicate BNW meet proximity vector");
            found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::vector_i32 &&
            found->values.size()==64,
            "BNW meet proximity saved vector is unavailable");
    return found->values.at(static_cast<std::size_t>(other));
}
std::size_t first_ai_index(const BnwSightMeetEffectPlan &plan,
                           const BnwSightFirstContactResult &first_contact) {
    require(plan.calculated && plan.contacts.size()==1 &&
            plan.contacts[0].new_contact && first_contact.calculated &&
            first_contact.detached &&
            first_contact.blocker==
                BnwSightFirstContactBlocker::first_contact_ai_unimplemented &&
            first_contact.proximity_changes.size()==2,
            "BNW meet first-contact/proximity detached plan is unavailable");
    const auto &contact=plan.contacts[0];
    std::size_t ai=plan.events.size();
    for(std::size_t i=0;i<plan.events.size();++i)
        if(plan.events[i].kind==BnwMeetEffectKind::first_contact_ai) {
            ai=i;break;
        }
    require(ai>=2 && ai<plan.events.size() &&
            plan.events[ai].team==contact.revealing_team &&
            plan.events[ai].other_team==contact.other_team,
            "BNW meet first-contact AI event is unavailable or out of order");
    for(std::size_t i=0;i<2;++i) {
        const auto &event=plan.events[ai-2+i];
        const auto &change=first_contact.proximity_changes[i];
        require(event.kind==BnwMeetEffectKind::proximity && event.external &&
                event.team==contact.revealing_team &&
                event.other_team==contact.other_team &&
                event.player==change.player &&
                event.other_player==change.other &&
                change.player>=0 && change.player<63 &&
                change.other>=0 && change.other<63 &&
                change.player!=change.other,
                "BNW meet proximity receipts disagree with SDK call order");
    }
    require(first_contact.proximity_changes[0].player==
                first_contact.proximity_changes[1].other &&
            first_contact.proximity_changes[0].other==
                first_contact.proximity_changes[1].player,
            "BNW meet proximity pair is not reciprocal");
    return ai;
}
void check_restored(const saves::CivisSaveSection &section,
                    const saves::SavedSessionSnapshot &target,
                    const saves::SavedSessionSnapshot &source,
                    const SessionCheckpointIdentity &identity,
                    const std::vector<BnwSightProximityChange> &changes) {
    auto restored=source;
    restore_bnw_player_proximity_checkpoint(
        section,restored,source,identity);
    for(const auto &change:changes) {
        require(proximity(source,change.player,change.other)==change.old_value &&
                proximity(target,change.player,change.other)==change.new_value &&
                proximity(restored,change.player,change.other)==change.new_value,
                "BNW meet proximity ID20 private restore differs from event");
    }
    const auto roundtrip=make_bnw_player_proximity_checkpoint(
        restored,source,identity,changes);
    require(roundtrip.id==section.id && roundtrip.flags==section.flags &&
            roundtrip.bytes==section.bytes,
            "BNW meet proximity checkpoint re-encoding changed");
}
} // namespace

BnwMeetProximityStage stage_bnw_meet_proximity(
    const BnwSightMeetEffectPlan &plan,
    const BnwSightFirstContactResult &first_contact,
    const saves::SavedSessionSnapshot &source_world,
    const SessionCheckpointIdentity &identity) {
    BnwMeetProximityStage stage;
    try {
        const auto ai=first_ai_index(plan,first_contact);
        const auto section=make_bnw_player_proximity_checkpoint(
            first_contact.detached->world,source_world,identity,
            first_contact.proximity_changes);
        require(section.id==bnw_player_proximity_checkpoint_section_id &&
                section.flags==0,
                "BNW meet proximity checkpoint owner changed");
        check_restored(section,first_contact.detached->world,
                       source_world,identity,first_contact.proximity_changes);
        const auto hash=identify_bytes(section.bytes).sha256;
        for(std::size_t i=0;i<2;++i) {
            const auto &change=first_contact.proximity_changes[i];
            stage.receipts.push_back({ai-2+i,change.player,change.other,
                change.old_value,change.new_value,hash});
        }
        stage.checkpoint=section;
        stage.staged=true;
        stage.reason="BNW reciprocal proximity calls have ID20 receipts; first-contact AI remains unexecuted";
    } catch(const std::exception &error) {
        stage.staged=false;
        stage.checkpoint.reset();stage.receipts.clear();
        stage.reason=error.what();
    }
    return stage;
}

bool verify_bnw_meet_proximity_receipts(
    const BnwMeetProximityStage &stage,
    const BnwSightMeetEffectPlan &plan,
    const BnwSightFirstContactResult &first_contact,
    const saves::SavedSessionSnapshot &source_world,
    const SessionCheckpointIdentity &identity) {
    try {
        require(stage.staged && stage.checkpoint && stage.receipts.size()==2,
                "BNW meet proximity receipts are unavailable");
        const auto ai=first_ai_index(plan,first_contact);
        const auto hash=identify_bytes(stage.checkpoint->bytes).sha256;
        for(std::size_t i=0;i<2;++i) {
            const auto &receipt=stage.receipts[i];
            const auto &change=first_contact.proximity_changes[i];
            require(receipt.event_index==ai-2+i &&
                    receipt.player==change.player &&
                    receipt.other==change.other &&
                    receipt.old_value==change.old_value &&
                    receipt.new_value==change.new_value &&
                    receipt.checkpoint_sha256==hash,
                    "BNW meet proximity receipt event or digest changed");
        }
        check_restored(*stage.checkpoint,first_contact.detached->world,
                       source_world,identity,first_contact.proximity_changes);
        return true;
    } catch(const std::exception &) {return false;}
}
} // namespace civ5::runtime
