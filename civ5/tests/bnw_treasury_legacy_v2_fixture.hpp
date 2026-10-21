#pragma once
// Test-only pinned previous production codec, renamed symbols only.
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','G','O','L','1'};
constexpr std::size_t players=64;
constexpr std::size_t max_history=16'384,max_section_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0' && c<='9') || (c>='a' && c<='f');
            }),"Invalid BNW treasury balance SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW treasury balance exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void string(std::string_view value) {
        require(value.size()<=UINT32_MAX,"BNW treasury schema name too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW treasury balance section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW treasury source, rules, roster or schema changed");
        at+=expected.size();
    }
};
std::vector<std::int64_t> read_history(Reader &in) {
    const auto count=in.u32();
    require(count<=max_history && in.at<=in.bytes.size() &&
            count<=(in.bytes.size()-in.at)/4,
            "Invalid BNW treasury history count");
    std::vector<std::int64_t> result;
    result.reserve(count);
    for(std::uint32_t i=0;i<count;++i) result.push_back(in.i32());
    return result;
}
void write_history(Writer &out,const std::vector<std::int64_t> &values) {
    require(values.size()<=max_history,"BNW treasury history exceeds supported width");
    out.u32(static_cast<std::uint32_t>(values.size()));
    for(const auto value:values) out.i32(value);
}
void validate_history_transition(const std::vector<std::int64_t> &old,
                                 const std::vector<std::int64_t> &live) {
    require(old.size()<=max_history && live.size()<=max_history &&
            live.size()>=old.size() && live.size()-old.size()<=1 &&
            std::equal(old.begin(),old.end(),live.begin()),
            "BNW treasury history must retain prior samples and append at most once");
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW treasury balance needs complete plain player roster");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &admin=player.administration;
        require(player.index==i &&
                player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.core.layout==bnw && admin.layout==bnw &&
                admin.blocks.size()==3 &&
                name(admin.blocks[0].name)=="notifications" &&
                name(admin.blocks[1].name)=="treasury" &&
                name(admin.blocks[2].name)=="pre_city",
                "BNW treasury player ID or administration block order changed");
    }
}
const saves::NamedSaveInteger &field(
    const saves::PlayerAdministrationRecord &record,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:record.fields)
        if(name(entry.name)==wanted) {
            require(!found,"Duplicate BNW treasury field");found=&entry;
        }
    require(found,"Missing BNW treasury field");return *found;
}
saves::NamedSaveInteger &mutable_field(
    saves::PlayerAdministrationRecord &record,std::string_view wanted) {
    (void)field(record,wanted);
    for(auto &entry:record.fields)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Missing BNW mutable treasury field");
}
saves::NamedSaveInteger &balance(saves::SavedPlayerObservation &player) {
    auto &record=player.administration.blocks[1];
    return mutable_field(record,"m_iGold");
}
std::int64_t balance(const saves::SavedPlayerObservation &player) {
    return field(player.administration.blocks[1],"m_iGold").value;
}
const saves::PlotArrayObservation &array(
    const saves::PlayerAdministrationRecord &record,std::string_view wanted) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &entry:record.arrays)
        if(name(entry.name)==wanted) {
            require(!found,"Duplicate BNW treasury history");found=&entry;
        }
    require(found && found->columns==1,
            "Missing BNW treasury history");return *found;
}
std::vector<std::int64_t> &mutable_history(
    saves::PlayerAdministrationRecord &record,std::string_view wanted) {
    (void)array(record,wanted);
    for(auto &entry:record.arrays)
        if(name(entry.name)==wanted) return entry.values;
    throw std::runtime_error("Missing BNW mutable treasury history");
}
void record(Writer &out,const saves::PlayerAdministrationRecord &value,
            bool mask_balance,bool extended) {
    out.string(name(value.name));out.u64(value.index);
    out.u64(value.start_offset);out.u64(value.end_offset);
    out.u64(value.fields.size());
    for(const auto &item:value.fields) {
        out.string(name(item.name));
        const bool mask=mask_balance &&
            (name(item.name)=="m_iGold" ||
             (extended && name(item.name)=="m_iLifetimeGrossGoldIncome"));
        out.i64(mask?0:item.value);
    }
    out.u64(value.arrays.size());
    for(const auto &item:value.arrays) {
        out.string(name(item.name));out.u64(item.columns);
        const bool mask=mask_balance && extended &&
            (name(item.name)=="m_GoldBalanceForTurnTimes100" ||
             name(item.name)=="m_GoldChangeForTurnTimes100");
        out.u64(mask?0:item.values.size());
        if(!mask) for(const auto entry:item.values) out.i64(entry);
    }
    out.u64(value.blobs.size());
    for(const auto &item:value.blobs) {
        out.string(name(item.name));out.u64(item.bytes.size());
        out.bytes.insert(out.bytes.end(),item.bytes.begin(),item.bytes.end());
    }
    out.u64(value.children.size());
    for(const auto &child:value.children) record(out,child,false,extended);
}
std::string masked_hash(const saves::SavedSessionSnapshot &world,bool extended=false) {
    shape(world);Writer out;
    const auto &all=*world.all_players;
    out.u64(all.start_offset);out.u64(all.end_offset);
    for(std::size_t i=0;i<players;++i) {
        const auto &player=all.players[i];
        const auto &admin=player.administration;
        out.u32(static_cast<std::uint32_t>(i));
        out.i32(player.saved_player_id);
        out.u64(admin.start_offset);out.u64(admin.end_offset);
        for(std::size_t j=0;j<admin.blocks.size();++j)
            record(out,admin.blocks[j],j==1,extended);
    }
    return identify_bytes(out.bytes).sha256;
}
std::int64_t saved_turn(const saves::SavedSessionSnapshot &world) {
    require(world.pregame && world.pregame->layout==bnw,
            "BNW treasury history needs saved game clock");
    const auto found=world.pregame->archive.integers.find("game_turn");
    require(found!=world.pregame->archive.integers.end() && found->second>=0,
            "BNW treasury history game turn unavailable");
    return found->second;
}
void owned_matches_raw(const TreasuryContext &live,
                       const TreasuryContext &baseline,
                       const saves::SavedSessionSnapshot &world,
                       const saves::SavedSessionSnapshot &source,
                       const ResearchContext &setup,bool extended) {
    shape(world);shape(source);
    require(live.profile==content::TechnologyProfile::bnw &&
            baseline.profile==content::TechnologyProfile::bnw &&
            setup.profile==content::TechnologyProfile::bnw &&
            live.players.size()==players && baseline.players.size()==players &&
            setup.players.size()==players &&
            live.minor_players==baseline.minor_players &&
            live.religions.empty() && baseline.religions.empty() &&
            live.trade_routes.empty() && baseline.trade_routes.empty() &&
            source.trade.routes.empty(),
            "BNW treasury checkpoint requires unchanged plain context outside its four values");
    const auto turn=extended?saved_turn(world):0;
    if(extended)
        require(turn>=saved_turn(source) && setup.game_turn==turn,
                "BNW treasury history and game clock owners disagree");
    for(std::size_t i=0;i<players;++i) {
        const auto id=static_cast<std::int64_t>(i);
        require(setup.players.contains(id) && live.players.contains(id) &&
                baseline.players.contains(id),
                "BNW treasury owned roster changed");
        const auto &player=live.players.at(id);
        const auto &old=baseline.players.at(id);
        const auto &raw=world.all_players->players[i];
        const auto &raw_old=source.all_players->players[i];
        const auto &treasury=raw.administration.blocks[1];
        const auto &old_treasury=raw_old.administration.blocks[1];
        require(player.profile_extensions.values.empty() &&
                old.profile_extensions.values.empty() &&
                player.source_core==old.source_core &&
                player.source_administration==old.source_administration &&
                player.source_core==
                    std::optional(std::pair{raw.core.start_offset,raw.core.end_offset}) &&
                player.source_administration==
                    std::optional(std::pair{raw.administration.start_offset,
                                            raw.administration.end_offset}) &&
                player.balance_times100==balance(raw) &&
                old.balance_times100==balance(raw_old) &&
                (extended || player.lifetime_gross_gold==old.lifetime_gross_gold) &&
                player.lifetime_gross_gold==
                    field(treasury,"m_iLifetimeGrossGoldIncome").value &&
                old.lifetime_gross_gold==
                    field(old_treasury,"m_iLifetimeGrossGoldIncome").value &&
                (extended || player.balance_history_times100==old.balance_history_times100) &&
                player.balance_history_times100==
                    array(treasury,"m_GoldBalanceForTurnTimes100").values &&
                old.balance_history_times100==
                    array(old_treasury,"m_GoldBalanceForTurnTimes100").values &&
                (extended || player.income_history_times100==old.income_history_times100) &&
                player.income_history_times100==
                    array(treasury,"m_GoldChangeForTurnTimes100").values &&
                old.income_history_times100==
                    array(old_treasury,"m_GoldChangeForTurnTimes100").values &&
                player.connections_times100==old.connections_times100 &&
                old.connections_times100==
                    field(old_treasury,"m_iCityConnectionGoldTimes100").value &&
                player.connection_modifier==old.connection_modifier &&
                old.connection_modifier==
                    field(old_treasury,"m_iCityConnectionTradeRouteGoldModifier").value &&
                player.connection_change==old.connection_change &&
                old.connection_change==
                    field(old_treasury,"m_iCityConnectionTradeRouteGoldChange").value &&
                player.gold_per_partner==old.gold_per_partner &&
                player.ally==old.ally && player.anarchy==old.anarchy,
                "BNW owned treasury differs from raw balance or unchanged baseline");
        if(extended) {
            validate_history_transition(old.balance_history_times100,
                                        player.balance_history_times100);
            validate_history_transition(old.income_history_times100,
                                        player.income_history_times100);
            if(player.balance_history_times100.size()>old.balance_history_times100.size())
                require(old.balance_history_times100.size()<
                        static_cast<std::uint64_t>(turn),
                        "BNW gold balance history appended before its turn");
            if(player.income_history_times100.size()>old.income_history_times100.size())
                require(old.income_history_times100.size()<
                        static_cast<std::uint64_t>(turn),
                        "BNW gold income history appended before its turn");
        }
    }
}
} // namespace

saves::CivisSaveSection legacy_v2_make_bnw_treasury_balance_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    shape(world);shape(source);
    bool extended=false;
    for(std::size_t i=0;i<players;++i) {
        const auto &old=source.all_players->players[i].administration.blocks[1];
        const auto &live=world.all_players->players[i].administration.blocks[1];
        extended|=field(old,"m_iLifetimeGrossGoldIncome").value!=
                  field(live,"m_iLifetimeGrossGoldIncome").value ||
                  array(old,"m_GoldBalanceForTurnTimes100").values!=
                  array(live,"m_GoldBalanceForTurnTimes100").values ||
                  array(old,"m_GoldChangeForTurnTimes100").values!=
                  array(live,"m_GoldChangeForTurnTimes100").values;
    }
    const auto source_hash=masked_hash(source,extended);
    require(masked_hash(world,extended)==source_hash,
            "BNW treasury administration has unsupported mutation");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(extended?2:1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.u32(players);
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto old=balance(source.all_players->players[i]);
        const auto live=balance(world.all_players->players[i]);
        require(old>=0 && live>=0 && old<=INT32_MAX && live<=INT32_MAX,
                "BNW treasury balance is outside supported wire range");
        changed|=old!=live;
        out.i32(static_cast<std::int64_t>(i));out.i32(old);out.i32(live);
        if(extended) {
            const auto &old_record=source.all_players->players[i].administration.blocks[1];
            const auto &live_record=world.all_players->players[i].administration.blocks[1];
            const auto old_lifetime=field(old_record,"m_iLifetimeGrossGoldIncome").value;
            const auto live_lifetime=field(live_record,"m_iLifetimeGrossGoldIncome").value;
            require(old_lifetime>=0 && live_lifetime>=old_lifetime,
                    "BNW treasury lifetime gross cannot decrease");
            changed|=old_lifetime!=live_lifetime;
            out.i32(old_lifetime);out.i32(live_lifetime);
            for(const auto key:{"m_GoldBalanceForTurnTimes100",
                                "m_GoldChangeForTurnTimes100"}) {
                const auto &old_history=array(old_record,key).values;
                const auto &live_history=array(live_record,key).values;
                validate_history_transition(old_history,live_history);
                changed|=old_history!=live_history;
                write_history(out,old_history);write_history(out,live_history);
            }
        }
    }
    require(changed,"BNW treasury balance checkpoint has no change");
    require(out.bytes.size()<=max_section_bytes,
            "BNW treasury balance section exceeds byte budget");
    return {bnw_treasury_balance_checkpoint_section_id,0,
            std::move(out.bytes)};
}

void legacy_v2_restore_bnw_treasury_balance_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_treasury_balance_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()<=max_section_bytes &&
            section.bytes.size()>=8+4+64*3+4+players*12 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW treasury balance checkpoint");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW treasury balance version");
    const bool extended=version==2;
    require(extended || section.bytes.size()==8+4+64*3+4+players*12,
            "Unsupported BNW treasury balance v1 length");
    const auto source_hash=masked_hash(source,extended);
    require(masked_hash(world,extended)==source_hash,
            "BNW treasury destination administration changed");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(source_hash);
    require(in.u32()==players,"BNW treasury balance roster count changed");
    struct Pending {
        std::int64_t balance=0,lifetime=0;
        std::vector<std::int64_t> balance_history,income_history;
        saves::NamedSaveInteger *balance_field=nullptr,*lifetime_field=nullptr;
        std::vector<std::int64_t> *balance_rows=nullptr,*income_rows=nullptr;
    };
    std::array<Pending,players> pending{};
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=in.i32(),old=in.i32(),live=in.i32();
        require(id==static_cast<std::int64_t>(i) && old>=0 && live>=0 &&
                balance(source.all_players->players[i])==old &&
                balance(world.all_players->players[i]).value==old,
                "BNW treasury balance source row or player ID changed");
        changed|=old!=live;pending[i].balance=live;
        pending[i].balance_field=&balance(world.all_players->players[i]);
        if(extended) {
            const auto old_lifetime=in.i32(),live_lifetime=in.i32();
            const auto &old_record=source.all_players->players[i].administration.blocks[1];
            const auto &current_record=world.all_players->players[i].administration.blocks[1];
            require(old_lifetime>=0 && live_lifetime>=old_lifetime &&
                    field(old_record,"m_iLifetimeGrossGoldIncome").value==old_lifetime &&
                    field(current_record,"m_iLifetimeGrossGoldIncome").value==old_lifetime,
                    "BNW treasury lifetime gross source changed");
            pending[i].lifetime=live_lifetime;
            auto &mutable_record=world.all_players->players[i].administration.blocks[1];
            pending[i].lifetime_field=&mutable_field(
                mutable_record,"m_iLifetimeGrossGoldIncome");
            pending[i].balance_rows=&mutable_history(
                mutable_record,"m_GoldBalanceForTurnTimes100");
            pending[i].income_rows=&mutable_history(
                mutable_record,"m_GoldChangeForTurnTimes100");
            changed|=old_lifetime!=live_lifetime;
            for(const auto key:{"m_GoldBalanceForTurnTimes100",
                                "m_GoldChangeForTurnTimes100"}) {
                const auto previous=read_history(in);
                const auto replacement=read_history(in);
                require(array(old_record,key).values==previous &&
                        array(current_record,key).values==previous,
                        "BNW treasury history source changed");
                validate_history_transition(previous,replacement);
                changed|=previous!=replacement;
                if(std::string_view(key)=="m_GoldBalanceForTurnTimes100")
                    pending[i].balance_history=replacement;
                else pending[i].income_history=replacement;
            }
        }
    }
    require(in.at==section.bytes.size() && changed,
            "BNW treasury balance section unchanged or incomplete");
    // All field/array references and allocations were obtained above. The
    // publication phase uses only scalar assignments and vector swaps.
    for(std::size_t i=0;i<players;++i) {
        pending[i].balance_field->value=pending[i].balance;
        if(extended) {
            pending[i].lifetime_field->value=pending[i].lifetime;
            pending[i].balance_rows->swap(pending[i].balance_history);
            pending[i].income_rows->swap(pending[i].income_history);
        }
    }
}

BnwTreasuryBalanceCheckpointPair legacy_v2_make_bnw_treasury_balance_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    const TreasuryContext &baseline_treasury,
    const SessionCheckpointIdentity &identity) {
    require(!state.player_early_publication && !state.player_gold_publication &&
            !state.game_clock.pending,
            "BNW treasury or game clock has unpublished effects");
    const auto raw=legacy_v2_make_bnw_treasury_balance_checkpoint(state.world,source,identity);
    const bool extended=raw.bytes[magic.size()]==2;
    owned_matches_raw(state.treasury,baseline_treasury,state.world,source,
                      state.setup,extended);
    return {raw,
            make_treasury_checkpoint(state.treasury,state.setup,identity)};
}

BnwTreasuryBalanceRestored legacy_v2_restore_bnw_treasury_balance_checkpoint_pair(
    const BnwTreasuryBalanceCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const TreasuryContext &baseline_treasury,
    const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,
    const saves::SavedSessionSnapshot *world_after_clock) {
    auto world=world_after_clock?*world_after_clock:source;
    auto treasury=baseline_treasury;
    legacy_v2_restore_bnw_treasury_balance_checkpoint(pair.raw,world,source,identity);
    restore_treasury_checkpoint(pair.owned,treasury,setup,identity);
    owned_matches_raw(treasury,baseline_treasury,world,source,setup,
                      pair.raw.bytes[magic.size()]==2);
    return {std::move(world),std::move(treasury)};
}

} // namespace civ5::runtime
