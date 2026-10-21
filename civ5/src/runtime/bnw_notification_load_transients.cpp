#include "civ5/runtime/bnw_notification_load_transients.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t field(const saves::PlayerAdministrationRecord &record,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:record.fields)
        if(name(entry.name)==wanted) {
            require(!found,"BNW notification load field duplicated");
            found=&entry;
        }
    require(found,"BNW notification load field missing");
    return found->value;
}
const saves::PlayerAdministrationRecord &ring(
    const saves::SavedPlayerObservation &player) {
    const auto &admin=player.administration;
    require(admin.layout==saves::GameStateLayout::expansion2_version1 &&
            admin.blocks.size()==3 &&
            name(admin.blocks[0].name)=="notifications",
            "BNW notification load administration shape changed");
    return admin.blocks[0];
}
void same_slot_shape(const saves::PlayerAdministrationRecord &source,
                     const saves::PlayerAdministrationRecord &world,
                     std::size_t index) {
    require(name(source.name)=="notification" &&
            name(world.name)=="notification" &&
            source.index==index && world.index==index &&
            source.start_offset==world.start_offset &&
            source.end_offset==world.end_offset &&
            source.fields.size()==11 && world.fields.size()==11 &&
            source.blobs.size()==2 && world.blobs.size()==2 &&
            source.arrays.empty() && world.arrays.empty() &&
            source.children.empty() && world.children.empty(),
            "BNW notification load slot identity or schema changed");
    for(std::size_t i=0;i<source.fields.size();++i)
        require(name(source.fields[i].name)==name(world.fields[i].name),
                "BNW notification load slot field schema changed");
    for(std::size_t i=0;i<source.blobs.size();++i)
        require(name(source.blobs[i].name)==name(world.blobs[i].name) &&
                source.blobs[i].bytes.size()==world.blobs[i].bytes.size(),
                "BNW notification load string wire shape changed");
}
} // namespace

BnwNotificationLoadTransients rebuild_bnw_notification_load_transients(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            state.world.all_players && source.snapshot.all_players &&
            state.world.all_players->players.size()==64 &&
            source.snapshot.all_players->players.size()==64,
            "BNW notification load source or player roster changed");
    BnwNotificationLoadTransients result;
    for(std::size_t i=0;i<64;++i) {
        const auto &saved=source.snapshot.all_players->players[i];
        const auto &current=state.world.all_players->players[i];
        require(saved.index==i && current.index==i &&
                saved.saved_player_id==static_cast<std::int64_t>(i) &&
                current.saved_player_id==static_cast<std::int64_t>(i) &&
                saved.administration.start_offset==current.administration.start_offset &&
                saved.administration.end_offset==current.administration.end_offset,
                "BNW notification load player identity changed");
        const auto &raw=ring(saved),&live=ring(current);
        const auto has=field(live,"has_notifications");
        require(has==field(raw,"has_notifications") && (has==0 || has==1),
                "BNW notification object presence changed");
        if(!has) {
            require(raw.children.empty() && live.children.empty() &&
                    raw.fields.size()==1 && live.fields.size()==1,
                    "BNW absent notification object has saved slots");
            continue;
        }
        // SDK CvNotifications.cpp:238-285. Init clears 100 slots; Read
        // overwrites their saved fields. Version <=1 then translates all
        // 100 type indices; that dependent effect is not implemented here.
        require(field(raw,"version")==2 && field(live,"version")==2 &&
                field(raw,"m_ePlayer")==static_cast<std::int64_t>(i) &&
                field(live,"m_ePlayer")==static_cast<std::int64_t>(i) &&
                raw.fields.size()==6 && live.fields.size()==6 &&
                raw.children.size()==100 && live.children.size()==100 &&
                raw.start_offset==live.start_offset &&
                raw.end_offset==live.end_offset,
                "BNW notification load requires version-2 fixed owner/slots");
        const auto begin=field(live,"m_iNotificationsBeginIndex");
        const auto end=field(live,"m_iNotificationsEndIndex");
        const auto next=field(live,"m_iCurrentLookupIndex");
        require(begin>=0 && begin<100 && end>=0 && end<100 && next>=0,
                "BNW notification load ring cursor invalid");
        BnwNotificationLoadPlayer player;
        for(std::size_t n=0;n<100;++n) {
            same_slot_shape(raw.children[n],live.children[n],n);
            // Notification operator>> sets these on *every* slot, including
            // inactive positions and dismissed entries.
            player.slots[n]={true,false};
        }
        std::set<std::int64_t> lookups;
        for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100) {
            const auto &slot=live.children[static_cast<std::size_t>(cursor)];
            const auto lookup=field(slot,"m_iLookupIndex");
            const auto dismissed=field(slot,"m_bDismissed");
            require(lookup>=0 && lookup<next && lookups.insert(lookup).second &&
                    field(slot,"m_ePlayerID")==static_cast<std::int64_t>(i) &&
                    dismissed>=0 && dismissed<=1,
                    "BNW notification load active ring entry invalid");
            if(!dismissed) player.active_undismissed.push_back(
                static_cast<std::size_t>(cursor));
        }
        result.external_update_pending|=!player.active_undismissed.empty();
        result.players.emplace(static_cast<std::int64_t>(i),std::move(player));
    }
    result.current=true;
    return result;
}

} // namespace civ5::runtime
