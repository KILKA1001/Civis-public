#include "civ5/runtime/bnw_player_administration_live_bundle.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::string_view name(const char *text) {
    return text?std::string_view(text):std::string_view();
}
const saves::PlayerAdministrationRecord &block(
    const saves::SavedPlayerObservation &player,std::size_t index,
    std::string_view expected) {
    const auto &blocks=player.administration.blocks;
    require(blocks.size()==3 && index<blocks.size() &&
            name(blocks[index].name)==expected,
            "BNW ID36 administration block roster changed");
    return blocks[index];
}
std::int64_t field(const saves::PlayerAdministrationRecord &record,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:record.fields)
        if(name(entry.name)==wanted) {
            require(!found,"BNW ID36 duplicate administration field");
            found=&entry;
        }
    require(found,"BNW ID36 administration field unavailable");
    return found->value;
}
bool same(const saves::PlayerAdministrationRecord &a,
          const saves::PlayerAdministrationRecord &b) {
    if(name(a.name)!=name(b.name) || a.index!=b.index ||
       a.start_offset!=b.start_offset || a.end_offset!=b.end_offset ||
       a.fields.size()!=b.fields.size() || a.arrays.size()!=b.arrays.size() ||
       a.blobs.size()!=b.blobs.size() || a.children.size()!=b.children.size())
        return false;
    for(std::size_t i=0;i<a.fields.size();++i)
        if(name(a.fields[i].name)!=name(b.fields[i].name) ||
           a.fields[i].value!=b.fields[i].value) return false;
    for(std::size_t i=0;i<a.arrays.size();++i)
        if(name(a.arrays[i].name)!=name(b.arrays[i].name) ||
           a.arrays[i].columns!=b.arrays[i].columns ||
           a.arrays[i].values!=b.arrays[i].values) return false;
    for(std::size_t i=0;i<a.blobs.size();++i)
        if(name(a.blobs[i].name)!=name(b.blobs[i].name) ||
           a.blobs[i].bytes!=b.blobs[i].bytes) return false;
    for(std::size_t i=0;i<a.children.size();++i)
        if(!same(a.children[i],b.children[i])) return false;
    return true;
}
void require_owned_matches_raw(const LiveSessionState &state) {
    require(state.world.all_players &&
            state.world.all_players->players.size()==64 &&
            state.treasury.profile==content::TechnologyProfile::bnw &&
            state.player_research.profile==content::TechnologyProfile::bnw &&
            state.maintenance.profile==content::TechnologyProfile::bnw &&
            state.treasury.players.size()==64 &&
            state.player_research.players.size()==64 &&
            state.maintenance.players.size()==64,
            "BNW ID36 owned treasury, maintenance or queue roster unavailable");
    for(std::size_t i=0;i<64;++i) {
        const auto &saved=state.world.all_players->players[i];
        require(saved.index==i && saved.saved_player_id==static_cast<std::int64_t>(i) &&
                state.treasury.players.contains(i) &&
                state.player_research.players.contains(i) &&
                state.maintenance.players.contains(i),
                "BNW ID36 owned player identity changed");
        const auto &tr=block(saved,1,"treasury");
        const auto &owned=state.treasury.players.at(i);
        const auto &maintenance=state.maintenance.players.at(i);
        require(owned.source_administration &&
                owned.source_administration->first==saved.administration.start_offset &&
                owned.source_administration->second==saved.administration.end_offset &&
                owned.balance_times100==field(tr,"m_iGold") &&
                owned.connections_times100==field(tr,"m_iCityConnectionGoldTimes100") &&
                owned.connection_modifier==field(tr,"m_iCityConnectionTradeRouteGoldModifier") &&
                owned.connection_change==field(tr,"m_iCityConnectionTradeRouteGoldChange") &&
                owned.lifetime_gross_gold==field(tr,"m_iLifetimeGrossGoldIncome") &&
                tr.arrays.size()==2 &&
                owned.balance_history_times100==tr.arrays[0].values &&
                owned.income_history_times100==tr.arrays[1].values &&
                maintenance.base_building==field(tr,"m_iBaseBuildingGoldMaintenance") &&
                maintenance.base_improvement==field(tr,"m_iBaseImprovementGoldMaintenance") &&
                maintenance.cached_unit_expense==field(tr,"m_iExpensePerTurnUnitMaintenance") &&
                maintenance.cached_supply_expense==field(tr,"m_iExpensePerTurnUnitSupply") &&
                maintenance.diplomacy_gold==field(tr,"m_iGoldPerTurnFromDiplomacy"),
                "BNW ID36 raw treasury and owned economy contexts diverged");
        const auto &pre=block(saved,2,"pre_city");
        require(pre.arrays.size()==4 &&
                name(pre.arrays[3].name)=="m_researchQueue" &&
                state.player_research.players.at(i).queue==pre.arrays[3].values,
                "BNW ID36 raw and owned research queue diverged");
    }
}
void valid_ring(const saves::PlayerAdministrationRecord &ring,
                std::size_t player,std::int64_t game_turn) {
    // CvNotifications::Read, CvNotifications.cpp:264-286, then the same
    // active-ring traversal as Update/Dismiss. Version 0/1 require a legacy
    // notification-type translation that this detached path does not run.
    require(field(ring,"has_notifications")==1 && field(ring,"version")==2 &&
            field(ring,"m_ePlayer")==static_cast<std::int64_t>(player) &&
            ring.children.size()==100,
            "BNW ID36 changed notification ring lacks version-2 owner/slots");
    const auto begin=field(ring,"m_iNotificationsBeginIndex");
    const auto end=field(ring,"m_iNotificationsEndIndex");
    const auto lookup=field(ring,"m_iCurrentLookupIndex");
    require(begin>=0 && begin<100 && end>=0 && end<100 &&
            lookup>=0 && lookup<=INT32_MAX && game_turn>=0,
            "BNW ID36 notification ring cursor invalid");
    std::set<std::int64_t> seen;
    for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100) {
        const auto &slot=ring.children[static_cast<std::size_t>(cursor)];
        const auto slot_lookup=field(slot,"m_iLookupIndex");
        const auto slot_turn=field(slot,"m_iTurn");
        require(name(slot.name)=="notification" &&
                slot.index==static_cast<std::uint64_t>(cursor) &&
                slot_lookup>=0 && slot_lookup<lookup && seen.insert(slot_lookup).second &&
                field(slot,"m_ePlayerID")==static_cast<std::int64_t>(player) &&
                slot_turn>=0 && slot_turn<=game_turn &&
                field(slot,"m_bDismissed")>=0 &&
                field(slot,"m_bDismissed")<=1 &&
                slot.blobs.size()==2 && slot.blobs[0].bytes.size()<512 &&
                std::find(slot.blobs[0].bytes.begin(),slot.blobs[0].bytes.end(),0)==
                    slot.blobs[0].bytes.end(),
                "BNW ID36 active notification cannot be reconstructed safely");
    }
}
std::pair<std::size_t,std::size_t> validate_subset(
    const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &after) {
    const auto &before=*baseline.world.all_players;
    require(after.all_players && after.all_players->players.size()==64,
            "BNW ID36 restored player roster unavailable");
    std::size_t rings=0,flags=0;
    for(std::size_t i=0;i<64;++i) {
        const auto &a=before.players[i],&b=after.all_players->players[i];
        const auto &old_ring=block(a,0,"notifications");
        const auto &new_ring=block(b,0,"notifications");
        if(!same(old_ring,new_ring)) {
            require(baseline.setup.players.contains(i) &&
                    baseline.setup.players.at(i).alive,
                    "BNW ID36 changed notification belongs to inactive player");
            valid_ring(old_ring,i,baseline.setup.game_turn);
            valid_ring(new_ring,i,baseline.setup.game_turn);
            ++rings;
        }
        require(same(block(a,1,"treasury"),block(b,1,"treasury")),
                "BNW ID36 changed treasury without owned economy restoration");
        const auto &old_pre=block(a,2,"pre_city");
        const auto &new_pre=block(b,2,"pre_city");
        require(old_pre.fields.size()==new_pre.fields.size() &&
                old_pre.arrays.size()==new_pre.arrays.size() &&
                old_pre.blobs.size()==new_pre.blobs.size() &&
                old_pre.children.size()==new_pre.children.size(),
                "BNW ID36 pre-city structure changed");
        for(std::size_t n=0;n<old_pre.fields.size();++n) {
            const auto &x=old_pre.fields[n],&y=new_pre.fields[n];
            require(name(x.name)==name(y.name),
                    "BNW ID36 pre-city field identity changed");
            const auto key=name(x.name);
            if(key=="m_bEverPoppedGoody" || key=="m_bEverTrainedBuilder") {
                require(x.value>=0 && x.value<=1 && y.value>=x.value && y.value<=1,
                        "BNW ID36 goody/builder history flag is not monotonic");
                flags+=x.value!=y.value;
            } else require(x.value==y.value,
                           "BNW ID36 unsupported pre-city scalar changed");
        }
        for(std::size_t n=0;n<old_pre.arrays.size();++n)
            require(name(old_pre.arrays[n].name)==name(new_pre.arrays[n].name) &&
                    old_pre.arrays[n].columns==new_pre.arrays[n].columns &&
                    old_pre.arrays[n].values==new_pre.arrays[n].values,
                    "BNW ID36 changed yield, unit cycle or research queue");
        for(std::size_t n=0;n<old_pre.blobs.size();++n)
            require(name(old_pre.blobs[n].name)==name(new_pre.blobs[n].name) &&
                    old_pre.blobs[n].bytes==new_pre.blobs[n].bytes,
                    "BNW ID36 changed an unsupported pre-city blob");
        for(std::size_t n=0;n<old_pre.children.size();++n)
            require(same(old_pre.children[n],new_pre.children[n]),
                    "BNW ID36 changed city-name collection");
    }
    require(rings+flags>0,"BNW ID36 bundle contains no supported change");
    return {rings,flags};
}
} // namespace

BnwPlayerAdministrationNotificationBundle
restore_bnw_player_administration_notification_bundle(
    const saves::CivisSaveSection &section,
    const LiveSessionState &baseline,
    const saves::SavedSession &source,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity) {
    require(baseline.setup.profile==content::TechnologyProfile::bnw &&
            baseline.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            identify_bytes(database.gameplay_fingerprint_data()).sha256==
                identity.rules_sha256,
            "BNW ID36 source, rules or profile changed");
    require_owned_matches_raw(baseline);
    auto world=restore_bnw_player_administration_checkpoint(
        section,baseline.world,source.snapshot,identity);
    const auto [rings,flags]=validate_subset(baseline,world);
    BnwPlayerAdministrationNotificationBundle result{baseline,rings,flags};
    result.state.world=std::move(world);
    require_owned_matches_raw(result.state);
    return result;
}

} // namespace civ5::runtime
