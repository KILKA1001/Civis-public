#include "civ5/runtime/bnw_game_deals.hpp"
#include <climits>
#include <utility>
#include <map>
#include <string_view>
#include <stdexcept>

namespace civ5::runtime {
namespace {
LiveActionResult reject(const char *reason) {
    return {LiveActionOutcome::rejected,reason};
}
} // namespace

namespace {
void resource_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::int32_t resource_i32(std::int64_t value){resource_need(value>=INT32_MIN&&value<=INT32_MAX,"BNW ResourceTotal source i32 overflow");return static_cast<std::int32_t>(value);}
const saves::PlayerCoreField& resource_field(const saves::SavedPlayerObservation& player,std::string_view name){
    const saves::PlayerCoreField* found=nullptr;
    for(const auto& field:player.core.fields)if(field.name&&std::string_view(field.name)==name){resource_need(!found,"Duplicate BNW ResourceTotal raw field");found=&field;}
    resource_need(found,"Missing BNW ResourceTotal raw field");return *found;
}
std::int32_t resource_scalar(const saves::SavedPlayerObservation& player,std::string_view name){
    const auto& field=resource_field(player,name);
    resource_need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&!field.wire_count&&
        field.hashes.empty()&&field.bytes.empty()&&field.row_counts.empty(),"BNW ResourceTotal scalar schema differs");
    return resource_i32(field.values.front());
}
}
std::int32_t compute_bnw_deal_resource_total_diagnostic(const LiveSessionState& state,
    const content::RulesDatabase& database,const content::InfoTypeCatalog& types,
    const content::GlobalDefineCache& defines,std::int64_t player,std::int64_t resource){
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    resource_need(state.setup.profile==content::TechnologyProfile::bnw&&defines.profile()==content::GlobalDefineProfile::bnw&&
        types.report().all_succeeded()&&resource>=0&&resource<=INT32_MAX,"BNW ResourceTotal source profile/resource unavailable");
    struct ResourceSlot{std::string type;std::int32_t usage;};std::map<std::int64_t,ResourceSlot> slots;
    const auto report=database.prefetch_info("Resources",[&](auto slot,auto authored,const auto& row){
        resource_need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"BNW ResourceTotal allocation slot/ID differs");
        const auto name=row.text("Type");resource_need(name&&!name->empty(),"BNW ResourceTotal Info Type unavailable");
        const auto usage=resource_i32(row.integer("ResourceUsage"));
        const auto* typed=types.find(*name);const auto hash=content::saved_info_type_hash(*name);const auto* byhash=types.find_hash(hash);
        resource_need(typed&&byhash&&typed->table=="Resources"&&typed->id==authored&&typed->type==*name&&
            byhash->table==typed->table&&byhash->id==typed->id&&byhash->type==typed->type,
            "BNW ResourceTotal selected Info name/hash collision or slot binding differs");
        resource_need(slots.emplace(authored,ResourceSlot{*name,usage}).second,"BNW ResourceTotal duplicate allocation slot");return true;
    });
    resource_need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&
        static_cast<std::uint64_t>(resource)<report.slots,"BNW ResourceTotal actual resource allocation unavailable");
    const auto target=slots.find(resource);
    if(target==slots.end())return 0; // Actual source null ResourceInfo branch, BEFORE arrays.
    resource_need(player>=0&&player<64&&state.world.all_players&&state.world.all_players->layout==bnw&&
        state.world.all_players->players.size()==64,"BNW ResourceTotal actual player roster unavailable");
    const auto& raw=state.world.all_players->players[static_cast<std::size_t>(player)];
    resource_need(raw.index==static_cast<std::uint64_t>(player)&&raw.saved_player_id==player&&raw.core.layout==bnw&&
        resource_scalar(raw,"m_eID")==player,"BNW ResourceTotal raw player identity differs");
    const auto value=[&](std::string_view name){
        const auto& field=resource_field(raw,name);
        resource_need(field.encoding==saves::PlayerCoreEncoding::hashed_i32&&field.wire_count&&
            *field.wire_count==field.hashes.size()&&
            field.bytes.empty()&&field.row_counts.empty(),"BNW ResourceTotal hashed array schema differs");
        std::size_t at=0;std::optional<std::int32_t> last;
        for(auto hash:field.hashes){
            resource_need(hash<=UINT32_MAX,"BNW ResourceTotal hash exceeds source width");if(!hash)continue;
            resource_need(at<field.values.size(),"BNW ResourceTotal hash lacks consumed payload");const auto number=resource_i32(field.values[at++]);
            const auto* info=types.find_hash(hash);if(!info)continue; // Source consumes unknown hash payload, does not SET.
            const auto mapped=slots.find(info->id);const auto* byname=types.find(info->type);
            resource_need(info->table=="Resources"&&mapped!=slots.end()&&mapped->second.type==info->type&&byname&&
                byname->table==info->table&&byname->id==info->id&&byname->type==info->type&&
                content::saved_info_type_hash(info->type)==hash,"BNW ResourceTotal known foreign/colliding hash mapping differs");
            if(info->id==resource)last=number; // Native ReadHashedDataArray SET, serialized last wins.
        }
        resource_need(at==field.values.size(),"BNW ResourceTotal array has extra payload");
        resource_need(last.has_value(),"BNW ResourceTotal reached known slot lacks explicit restored coverage; no Reset0 inferred");return *last;
    };
    auto total=value("m_paiNumResourceTotal");
    if(target->second.usage==1){ // SDK ResourceUsageTypes: bonus0, strategic1, luxury2.
        const auto modifier=resource_scalar(raw,"m_iStrategicResourceMod");
        if(modifier!=0)total=resource_i32(std::int64_t(total)*modifier)/100;
    }
    total=resource_i32(std::int64_t(total)+value("m_paiResourceImport"));
    auto minors=value("m_paiResourceFromMinors");
    if(resource_scalar(raw,"m_iMinorResourceBonusCount")>0){
        const auto* entry=defines.find("MINOR_POLICY_RESOURCE_MULTIPLIER");
        resource_need(entry&&entry->kind==content::GlobalDefineKind::integer&&entry->lookup.found&&
            entry->lookup.sqlite_code==0&&entry->lookup.error.empty()&&!entry->retained_previous,
            "BNW ResourceTotal actual minor resource define unavailable");
        const auto* multiplier=std::get_if<std::int64_t>(&entry->lookup.value);
        resource_need(multiplier,"BNW ResourceTotal minor resource define type differs");
        minors=resource_i32(std::int64_t(minors)*resource_i32(*multiplier))/100;
    }
    total=resource_i32(std::int64_t(total)+minors);
    total=resource_i32(std::int64_t(total)+value("m_paiResourcesSiphoned"));
    return resource_i32(std::int64_t(total)-value("m_paiResourceExport"));
}

BnwTurnHandler make_bnw_game_deals_handler() {
    return {BnwTurnPhase::game_deals,
            [](LiveSessionState &state,std::int64_t player) -> LiveActionResult {
                if(player!=-1)
                    return reject("BNW deals turn requires the game phase");
                constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
                if(state.setup.profile!=content::TechnologyProfile::bnw ||
                   state.world.game_state.layout!=bnw ||
                   state.world.history.layout!=bnw ||
                   state.world.history.deals.version!=1 ||
                   state.world.history.deals.next_deal_id)
                    return reject("BNW deals turn requires the decoded BNW deal layout");
                // Expansion2 CvDealClasses.cpp, CvGameDeals::DoTurn: the entire
                // method is guarded by m_CurrentDeals.size() > 0. Proposed
                // and historical deals do not change in this branch.
                const auto &current=state.world.history.deals.groups[1];
                if(current.empty())
                    return {LiveActionOutcome::committed,{}};
                if(state.setup.game_turn<0 || state.setup.game_turn>INT32_MAX)
                    return reject("BNW deals turn requires a source i32 game turn");
                const auto turn=state.setup.game_turn;
                // First source loop invokes DoEndTradedItem for *every* item
                // expiring today, before it tests resource validity. Preflight
                // the whole list before any current/historical mutation.
                for(const auto &deal:current) {
                    if(!deal.version || *deal.version<1 || *deal.version>3 ||
                       deal.id || !deal.cancelled ||
                       deal.from_player<0 || deal.from_player>=64 ||
                       deal.to_player<0 || deal.to_player>=64 ||
                       deal.from_player==deal.to_player)
                        return reject("BNW current deal layout or players unavailable");
                    for(const auto &item:deal.items) {
                        if(item.final_turn< -1 ||
                           (item.from_player!=deal.from_player &&
                            item.from_player!=deal.to_player) ||
                           !item.from_renewed || !item.to_renewed)
                            return reject("BNW current deal item layout unavailable");
                        if(item.final_turn==turn)
                            return reject("BNW expiring deal item needs DoEndTradedItem and UI effects");
                    }
                }
                // Second source loop checks resource shortage only for items
                // with final_turn>-1 and != turn, breaking at the first peace
                // treaty item. Gold-per-turn shortage is commented out in BNW.
                constexpr std::int64_t resources=3,peace_treaty=13;
                for(const auto &deal:current)
                    for(const auto &item:deal.items) {
                        if(item.final_turn<0) continue;
                        if(item.type==resources)
                            return reject("BNW deal resource validity needs live getNumResourceTotal");
                        if(item.type==peace_treaty) break;
                    }
                // With no expired item or invalid resource, bSomethingChanged
                // stays false: there is no UI dirty bit, notification, treasury,
                // team, Lua or RNG effect. DoUpdateCurrentDealsList copies the
                // current list, clears it, and appends ended deals to history.
                auto next=state.world.history.deals;
                auto &retained=next.groups[1],&history=next.groups[2];
                retained.clear();
                retained.reserve(current.size());
                for(const auto &deal:current) {
                    if(deal.final_turn<=turn)
                        history.push_back(deal);
                    else
                        retained.push_back(deal);
                }
                state.world.history.deals=std::move(next);
                return {LiveActionOutcome::committed,{}};
            }};
}

} // namespace civ5::runtime
