#include "civ5/runtime/bnw_research_queue_save_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','R','Q','F','1'};
constexpr std::size_t players=64,max_queue=4096,max_flags=4096,max_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void sha(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0' && c<='9') || (c>='a' && c<='f');
            }),"Invalid BNW research queue SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t v) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(v>>(8*i)));
    }
    void u64(std::uint64_t v) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(v>>(8*i)));
    }
    void i32(std::int64_t v) {
        require(v>=INT32_MIN && v<=INT32_MAX,"BNW research queue exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(v)));
    }
    void i64(std::int64_t v) {u64(static_cast<std::uint64_t>(v));}
    void raw(std::string_view v) {bytes.insert(bytes.end(),v.begin(),v.end());}
    void string(std::string_view v) {
        require(!v.empty() && v.size()<=UINT32_MAX,"BNW research schema name unavailable");
        u32(static_cast<std::uint32_t>(v.size()));raw(v);
    }
    void values(const std::vector<std::int64_t> &v) {
        require(v.size()<=max_queue,"BNW research queue exceeds limit");
        u32(static_cast<std::uint32_t>(v.size()));
        for(const auto item:v) {require(item>=0,"BNW research queue has negative technology");i32(item);}
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW research queue section");
        std::uint32_t v=0;
        for(unsigned i=0;i<4;++i) v|=std::uint32_t(bytes[at++])<<(8*i);
        return v;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW research source, rules or schema changed");
        at+=expected.size();
    }
    std::vector<std::int64_t> values() {
        const auto count=u32();
        require(count<=max_queue && at<=bytes.size() &&
                count<=(bytes.size()-at)/4,"Invalid BNW research queue count");
        std::vector<std::int64_t> result;
        result.reserve(count);
        for(std::uint32_t i=0;i<count;++i) {
            const auto v=i32();require(v>=0,"Negative BNW research queue technology");
            result.push_back(v);
        }
        return result;
    }
};
const saves::PlayerAdministrationRecord &pre_city(
    const saves::SavedPlayerObservation &player) {
    const saves::PlayerAdministrationRecord *found=nullptr;
    for(const auto &block:player.administration.blocks)
        if(name(block.name)=="pre_city") {
            require(!found,"Duplicate BNW pre-city block");found=&block;
        }
    require(found,"Missing BNW pre-city block");return *found;
}
saves::PlayerAdministrationRecord &pre_city(saves::SavedPlayerObservation &player) {
    (void)pre_city(std::as_const(player));
    for(auto &block:player.administration.blocks)
        if(name(block.name)=="pre_city") return block;
    throw std::runtime_error("Missing BNW mutable pre-city block");
}
const saves::NamedSaveInteger &queue_count(
    const saves::PlayerAdministrationRecord &block) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:block.fields)
        if(name(field.name)=="m_researchQueue_wire_count") {
            require(!found,"Duplicate BNW research queue count");found=&field;
        }
    require(found,"Missing BNW research queue count");return *found;
}
saves::NamedSaveInteger &queue_count(saves::PlayerAdministrationRecord &block) {
    (void)queue_count(std::as_const(block));
    for(auto &field:block.fields)
        if(name(field.name)=="m_researchQueue_wire_count") return field;
    throw std::runtime_error("Missing BNW mutable research queue count");
}
const saves::PlotArrayObservation &queue(const saves::PlayerAdministrationRecord &block) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:block.arrays)
        if(name(array.name)=="m_researchQueue") {
            require(!found,"Duplicate BNW research queue array");found=&array;
        }
    require(found && found->columns==1,"Missing BNW research queue array");
    return *found;
}
saves::PlotArrayObservation &queue(saves::PlayerAdministrationRecord &block) {
    (void)queue(std::as_const(block));
    for(auto &array:block.arrays)
        if(name(array.name)=="m_researchQueue") return array;
    throw std::runtime_error("Missing BNW mutable research queue array");
}
const saves::PlayerKnowledgeBlock &techs(const saves::SavedPlayerObservation &player) {
    const saves::PlayerKnowledgeBlock *found=nullptr;
    for(const auto &block:player.knowledge.blocks)
        if(name(block.name)=="player_techs") {
            require(!found,"Duplicate BNW player-tech block");found=&block;
        }
    require(found,"Missing BNW player-tech block");return *found;
}
saves::PlayerKnowledgeBlock &techs(saves::SavedPlayerObservation &player) {
    (void)techs(std::as_const(player));
    for(auto &block:player.knowledge.blocks)
        if(name(block.name)=="player_techs") return block;
    throw std::runtime_error("Missing BNW mutable player-tech block");
}
const saves::TeamHashedArray &flags(const saves::PlayerKnowledgeBlock &block) {
    const saves::TeamHashedArray *found=nullptr;
    for(const auto &array:block.hashes)
        if(name(array.name)=="researching_tech") {
            require(!found,"Duplicate BNW researching hash array");found=&array;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::uint64_t>(found->wire_count)==found->rows.size() &&
            found->rows.size()<=max_flags,
            "Missing BNW researching hash array");
    return *found;
}
saves::TeamHashedArray &flags(saves::PlayerKnowledgeBlock &block) {
    (void)flags(std::as_const(block));
    for(auto &array:block.hashes)
        if(name(array.name)=="researching_tech") return array;
    throw std::runtime_error("Missing BNW mutable researching flags");
}
void valid_queue(const saves::PlayerAdministrationRecord &block) {
    const auto count=queue_count(block).value;
    const auto &values=queue(block).values;
    require(count>=INT32_MIN && count<=INT32_MAX && values.size()<=max_queue &&
            ((count<=0 && values.empty()) ||
             (count>0 && static_cast<std::uint64_t>(count)==values.size())),
            "BNW research queue count disagrees with values");
    for(const auto value:values)
        require(value>=0 && value<=INT32_MAX,
                "Invalid BNW research queue saved technology ID");
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW research choice needs complete plain player roster");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.administration.layout==bnw && player.knowledge.layout==bnw &&
                player.knowledge.profile_extensions.values.empty(),
                "BNW research choice player ID or profile changed");
        valid_queue(pre_city(player));
        for(const auto &row:flags(techs(player)).rows)
            require((row.type_hash==0 && row.values.empty()) ||
                    (row.type_hash!=0 && row.values.size()==1 &&
                     (row.values[0]==0 || row.values[0]==1)),
                    "Invalid BNW researching flag row");
    }
}
void admin_record(Writer &out,const saves::PlayerAdministrationRecord &block,
                  bool mask_queue) {
    out.string(name(block.name));out.u64(block.index);
    out.u64(block.start_offset);out.u64(block.end_offset);
    out.u64(block.fields.size());
    for(const auto &field:block.fields) {
        out.string(name(field.name));
        out.i64(mask_queue && name(field.name)=="m_researchQueue_wire_count"?
                0:field.value);
    }
    out.u64(block.arrays.size());
    for(const auto &array:block.arrays) {
        out.string(name(array.name));out.u64(array.columns);
        const bool mask=mask_queue && name(array.name)=="m_researchQueue";
        out.u64(mask?0:array.values.size());
        if(!mask) for(const auto value:array.values) out.i64(value);
    }
    out.u64(block.blobs.size());
    for(const auto &blob:block.blobs) {
        out.string(name(blob.name));out.u64(blob.bytes.size());
        out.bytes.insert(out.bytes.end(),blob.bytes.begin(),blob.bytes.end());
    }
    out.u64(block.children.size());
    for(const auto &child:block.children) admin_record(out,child,false);
}
void knowledge_block(Writer &out,const saves::PlayerKnowledgeBlock &block) {
    out.string(name(block.name));out.u64(block.start_offset);out.u64(block.end_offset);
    out.u64(block.fields.size());
    for(const auto &field:block.fields) {
        out.string(name(field.name));out.i64(field.value);
    }
    out.u64(block.arrays.size());
    for(const auto &array:block.arrays) {
        out.string(name(array.name));out.u64(array.columns);
        out.u64(array.values.size());
        for(const auto value:array.values) out.i64(value);
    }
    out.u64(block.hashes.size());
    for(const auto &array:block.hashes) {
        out.string(name(array.name));out.i64(array.wire_count);
        out.u64(array.rows.size());
        const bool mask=name(block.name)=="player_techs" &&
                        name(array.name)=="researching_tech";
        for(const auto &row:array.rows) {
            out.u64(row.type_hash);out.u64(row.values.size());
            for(const auto value:row.values) out.i64(mask?0:value);
        }
    }
}
std::string masked_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    out.u64(world.all_players->start_offset);
    out.u64(world.all_players->end_offset);
    for(const auto &player:world.all_players->players) {
        out.u64(player.index);out.i32(player.saved_player_id);
        out.u64(player.administration.start_offset);
        out.u64(player.administration.end_offset);
        admin_record(out,pre_city(player),true);
        out.u64(player.knowledge.start_offset);
        out.u64(player.knowledge.end_offset);
        out.u64(player.knowledge.blocks.size());
        for(const auto &block:player.knowledge.blocks) knowledge_block(out,block);
    }
    return identify_bytes(out.bytes).sha256;
}
} // namespace

saves::CivisSaveSection make_bnw_research_queue_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    const auto source_hash=masked_hash(source);
    require(masked_hash(world)==source_hash,
            "BNW research choice has unsupported pre-city or knowledge mutation");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.u32(players);
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto &old_pre=pre_city(source.all_players->players[i]);
        const auto &live_pre=pre_city(world.all_players->players[i]);
        const auto &old_queue=queue(old_pre).values;
        const auto &live_queue=queue(live_pre).values;
        const auto &old_flags=flags(techs(source.all_players->players[i])).rows;
        const auto &live_flags=flags(techs(world.all_players->players[i])).rows;
        out.i32(static_cast<std::int64_t>(i));
        out.i32(queue_count(old_pre).value);out.i32(queue_count(live_pre).value);
        out.values(old_queue);out.values(live_queue);
        require(old_flags.size()==live_flags.size(),
                "BNW researching hash roster changed");
        Writer row_changes;
        std::uint32_t count=0;
        for(std::size_t row=0;row<old_flags.size();++row) {
            if(old_flags[row].values==live_flags[row].values) continue;
            require(old_flags[row].type_hash!=0 &&
                    old_flags[row].values.size()==1 &&
                    live_flags[row].values.size()==1 &&
                    count<max_flags,
                    "BNW researching flag changed an unsupported hash row");
            ++count;row_changes.u32(static_cast<std::uint32_t>(row));
            row_changes.i32(old_flags[row].values[0]);
            row_changes.i32(live_flags[row].values[0]);
        }
        out.u32(count);
        out.bytes.insert(out.bytes.end(),row_changes.bytes.begin(),row_changes.bytes.end());
        changed|=old_queue!=live_queue || count!=0;
        require(out.bytes.size()<=max_bytes,
                "BNW research choice section exceeds byte budget");
    }
    require(changed,"BNW research choice section has no queue or flag change");
    return {bnw_research_queue_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_research_queue_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    require(section.id==bnw_research_queue_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()<=max_bytes &&
            section.bytes.size()>=8+4+64*3+4+players*24 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW research choice section");
    const auto source_hash=masked_hash(source);
    require(masked_hash(world)==source_hash,
            "BNW research choice destination pre-city or knowledge changed");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW research choice version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(source_hash);
    require(in.u32()==players,"BNW research choice player roster changed");
    struct FlagWrite {std::int64_t *target;std::int64_t value;};
    struct Pending {
        saves::NamedSaveInteger *count=nullptr;
        std::vector<std::int64_t> *queue=nullptr;
        std::int64_t next_count=0;
        std::vector<std::int64_t> next_queue;
        std::vector<FlagWrite> flags;
    };
    std::array<Pending,players> pending{};
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=in.i32(),old_count=in.i32(),next_count=in.i32();
        const auto previous=in.values(),next=in.values();
        const auto &old_pre=pre_city(source.all_players->players[i]);
        auto &target_pre=pre_city(world.all_players->players[i]);
        require(id==static_cast<std::int64_t>(i) &&
                old_count==queue_count(old_pre).value &&
                old_count==queue_count(target_pre).value &&
                previous==queue(old_pre).values &&
                previous==queue(target_pre).values &&
                ((next_count<=0 && next.empty()) ||
                 (next_count>0 &&
                  static_cast<std::uint64_t>(next_count)==next.size())),
                "BNW research queue source row or count changed");
        pending[i].count=&queue_count(target_pre);
        pending[i].queue=&queue(target_pre).values;
        pending[i].next_count=next_count;
        pending[i].next_queue=next;
        changed|=previous!=next;
        const auto n=in.u32();
        const auto &source_rows=flags(techs(source.all_players->players[i])).rows;
        auto &target_rows=flags(techs(world.all_players->players[i])).rows;
        require(n<=max_flags && n<=source_rows.size() &&
                in.at<=in.bytes.size() && n<=(in.bytes.size()-in.at)/12,
                "BNW researching flag change count invalid");
        std::uint32_t previous_row=0;
        for(std::uint32_t j=0;j<n;++j) {
            const auto row=in.u32();
            const auto old=in.i32(),live=in.i32();
            require(row<source_rows.size() && (j==0 || row>previous_row) &&
                    source_rows[row].type_hash!=0 &&
                    source_rows[row].values.size()==1 &&
                    target_rows[row].values.size()==1 &&
                    source_rows[row].values[0]==old &&
                    target_rows[row].values[0]==old &&
                    (old==0 || old==1) && (live==0 || live==1) && old!=live,
                    "BNW researching flag row changed or duplicated");
            previous_row=row;
            pending[i].flags.push_back({&target_rows[row].values[0],live});
        }
        changed|=n!=0;
    }
    require(in.at==section.bytes.size() && changed,
            "BNW research choice section incomplete or unchanged");
    // No allocation or throwing lookup remains after this point.
    for(auto &entry:pending) {
        entry.count->value=entry.next_count;
        entry.queue->swap(entry.next_queue);
        for(const auto &flag:entry.flags) *flag.target=flag.value;
    }
}

namespace {
void setup_unchanged(const ResearchContext &live,const ResearchContext &old) {
    require(live.profile==content::TechnologyProfile::bnw &&
            old.profile==content::TechnologyProfile::bnw &&
            live.extensions.values.empty() && old.extensions.values.empty() &&
            live.game_type==old.game_type && live.speed==old.speed &&
            live.start_era==old.start_era && live.current_era==old.current_era &&
            live.game_handicap==old.game_handicap && live.game_turn==old.game_turn &&
            live.map_random_seed==old.map_random_seed &&
            live.world_percent==old.world_percent &&
            live.team_members==old.team_members && live.pitboss==old.pitboss &&
            live.players.size()==old.players.size() &&
            live.game_options.size()==old.game_options.size(),
            "BNW research choice changed setup context");
    for(const auto &[id,p]:old.players) {
        const auto it=live.players.find(id);
        require(it!=live.players.end() && it->second.team==p.team &&
                it->second.handicap==p.handicap &&
                it->second.slot_status==p.slot_status &&
                it->second.slot_claim==p.slot_claim &&
                it->second.alive==p.alive &&
                it->second.ever_alive==p.ever_alive &&
                it->second.policies==p.policies &&
                it->second.policies_resolved==p.policies_resolved,
                "BNW research choice changed setup player");
    }
    for(std::size_t i=0;i<live.game_options.size();++i)
        require(live.game_options[i].name==old.game_options[i].name &&
                live.game_options[i].value==old.game_options[i].value,
                "BNW research choice changed game option");
}
void selection_unchanged(const ResearchSelectionState &live,
                         const ResearchSelectionState &old) {
    require(live.profile==content::TechnologyProfile::bnw &&
            old.profile==content::TechnologyProfile::bnw &&
            live.disabled_technologies==old.disabled_technologies &&
            live.players.size()==old.players.size(),
            "BNW research choice changed selection rules");
    for(const auto &[id,p]:old.players) {
        const auto it=live.players.find(id);
        require(it!=live.players.end() &&
                it->second.advanced_start_points==p.advanced_start_points &&
                it->second.civilization==p.civilization,
                "BNW research choice changed selection player");
    }
}
std::set<std::int64_t> raw_flags(const saves::SavedPlayerObservation &player,
                                 const content::TechnologyRules &rules,
                                 const content::InfoTypeCatalog &types) {
    std::set<std::int64_t> result,seen;
    for(const auto &row:flags(techs(player)).rows) {
        if(row.type_hash==0) continue;
        const auto *type=types.find_hash(row.type_hash);
        const auto *info=type && type->id>=0 ?
            rules.find(static_cast<std::uint64_t>(type->id)) : nullptr;
        require(type && type->table=="Technologies" && info &&
                static_cast<std::uint64_t>(type->id)<rules.collection.slots &&
                type->type==info->base.type && type->hash==row.type_hash &&
                content::saved_info_type_hash(info->base.type)==row.type_hash &&
                seen.insert(type->id).second,
                "BNW researching flag hash is unresolved or duplicate");
        if(row.values[0]) result.insert(type->id);
    }
    return result;
}
void owned_matches_raw(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &setup,
    const std::vector<TeamTechnologies> &teams,
    const PlayerResearchContext &players_live,
    const ResearchTurnState &turn,
    const LiveSessionState &baseline,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,bool projected=false) {
    require(rules.profile==content::TechnologyProfile::bnw &&
            rules.all_succeeded() && types.report().all_succeeded() &&
            masked_hash(world)==masked_hash(source) &&
            masked_hash(baseline.world)==masked_hash(source) &&
            teams.size()==baseline.technologies.size() &&
            players_live.profile==content::TechnologyProfile::bnw &&
            turn.profile==content::TechnologyProfile::bnw &&
            players_live.world_city_modifier==
                baseline.player_research.world_city_modifier &&
            players_live.barbarian_team==baseline.player_research.barbarian_team &&
            players_live.civ_teams==baseline.player_research.civ_teams &&
            players_live.major_players==baseline.player_research.major_players &&
            players_live.bnw_minor_scan_players==
                baseline.player_research.bnw_minor_scan_players &&
            players_live.teams.size()==baseline.player_research.teams.size() &&
            players_live.leagues.size()==baseline.player_research.leagues.size() &&
            players_live.players.size()==players && turn.players.size()==players &&
            baseline.player_research.players.size()==players &&
            baseline.research_turn.players.size()==players,
            "BNW research choice rules, source or owned roster changed");
    setup_unchanged(setup,baseline.setup);
    for(const auto &[id,old]:baseline.player_research.teams) {
        const auto it=players_live.teams.find(id);
        require(it!=players_live.teams.end() &&
                it->second.alive_count==old.alive_count &&
                it->second.master==old.master &&
                it->second.liberated_by==old.liberated_by &&
                it->second.members==old.members && it->second.met==old.met &&
                it->second.embassy==old.embassy &&
                it->second.at_war==old.at_war &&
                it->second.war_peace.has_value()==old.war_peace.has_value() &&
                (!old.war_peace ||
                 (it->second.war_peace->locked_turns==old.war_peace->locked_turns &&
                  it->second.war_peace->permanent==old.war_peace->permanent)),
                "BNW research choice changed research team context");
    }
    for(std::size_t i=0;i<players_live.leagues.size();++i)
        require(players_live.leagues[i].members==
                    baseline.player_research.leagues[i].members &&
                players_live.leagues[i].discovered_tech_modifiers==
                    baseline.player_research.leagues[i].discovered_tech_modifiers,
                "BNW research choice changed league research context");
    for(std::size_t i=0;i<teams.size();++i) {
        const auto &current=teams[i],&old=baseline.technologies[i];
        require(current.profile()==content::TechnologyProfile::bnw &&
                old.profile()==current.profile() && current.team()==old.team() &&
                current.slots()==old.slots() &&
                current.last_tech()==old.last_tech() &&
                current.nondefault_values()==old.nondefault_values() &&
                current.profile_extensions().values.empty() &&
                old.profile_extensions().values.empty(),
                "BNW research choice changed team technology/progress state");
    }
    for(std::size_t i=0;i<players;++i) {
        const auto id=static_cast<std::int64_t>(i);
        require(players_live.players.contains(id) &&
                baseline.player_research.players.contains(id) &&
                turn.players.contains(id) && baseline.research_turn.players.contains(id),
                "BNW research choice player identity missing");
        const auto &p=players_live.players.at(id);
        const auto &old=baseline.player_research.players.at(id);
        const auto &t=turn.players.at(id);
        const auto &old_t=baseline.research_turn.players.at(id);
        const auto &raw=world.all_players->players[i];
        const auto &raw_old=baseline.world.all_players->players[i];
        require((projected || p.queue==queue(pre_city(raw)).values) &&
                old.queue==queue(pre_city(raw_old)).values &&
                p.overflow_times100==old.overflow_times100 &&
                p.max_effective_cities==old.max_effective_cities &&
                p.capital==old.capital && p.pregame_minor==old.pregame_minor &&
                p.cities.size()==old.cities.size() &&
                p.profile_extensions.values.empty() &&
                old.profile_extensions.values.empty(),
                "BNW research queue differs from raw or changed other player state");
        for(const auto &[city,c]:old.cities) {
            const auto it=p.cities.find(city);
            require(it!=p.cities.end() && it->second.puppet==c.puppet &&
                    it->second.ignore_happiness==c.ignore_happiness &&
                    it->second.razing==c.razing &&
                    it->second.profile_extensions.values.empty() &&
                    c.profile_extensions.values.empty(),
                    "BNW research choice changed city research state");
        }
        require((projected || t.researching==raw_flags(raw,rules,types)) &&
                old_t.researching==raw_flags(raw_old,rules,types) &&
                t.founded_first_city==old_t.founded_first_city &&
                t.cities_founded==old_t.cities_founded &&
                t.agreement_counters==old_t.agreement_counters &&
                t.leader_type==old_t.leader_type &&
                t.unmapped_researching==0 && old_t.unmapped_researching==0,
                "BNW researching flags differ from raw or changed other turn state");
        std::set<std::int64_t> unique_queue(p.queue.begin(),p.queue.end());
        std::set<std::int64_t> old_queue(old.queue.begin(),old.queue.end());
        require(unique_queue.size()==p.queue.size() &&
                old_queue.size()==old.queue.size() &&
                unique_queue==t.researching && old_queue==old_t.researching,
                "BNW research queue and researching flags disagree");
        for(const auto tech:p.queue)
            require(rules.find(static_cast<std::uint64_t>(tech))!=nullptr,
                    "BNW research queue technology is not in active rules");
    }
}
// Direct v1 emission retains the immutable source as the wire's old side.
// The baseline may itself have come from a genuine loaded ID33 checkpoint.
std::optional<saves::CivisSaveSection> direct_raw(
 const LiveSessionState& state,const LiveSessionState& baseline,
 const saves::SavedSessionSnapshot& source,const content::TechnologyRules& rules,
 const content::InfoTypeCatalog& types,const SessionCheckpointIdentity& identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    const auto hash=masked_hash(source);
    require(masked_hash(state.world)==hash && masked_hash(baseline.world)==hash,
        "BNW research choice raw pre-city or knowledge changed outside slice");
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);out.raw(hash);out.u32(players);
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=static_cast<std::int64_t>(i);
        const auto& desired=state.player_research.players.at(id).queue;
        const auto& desired_flags=state.research_turn.players.at(id).researching;
        const auto& current=state.world.all_players->players[i];
        const auto& original=source.all_players->players[i];
        const auto& previous=baseline.world.all_players->players[i];
        const auto& cp=pre_city(current);const auto& op=pre_city(original);
        const auto& bp=pre_city(previous);const auto& rows=flags(techs(current)).rows;
        const auto& old_rows=flags(techs(original)).rows;
        const auto& baseline_rows=flags(techs(previous)).rows;
        require(desired.size()<=max_queue && rows.size()==old_rows.size() &&
            baseline_rows.size()==old_rows.size(),"BNW research choice queue or flag roster changed");
        const auto same_rows=[](const auto& a,const auto& b) {
            return std::equal(a.begin(),a.end(),b.begin(),[](const auto& x,const auto& y){
                return x.type_hash==y.type_hash && x.values==y.values;});
        };
        const bool original_form=queue(cp).values==queue(op).values &&
            queue_count(cp).value==queue_count(op).value && same_rows(rows,old_rows);
        const bool baseline_form=queue(cp).values==queue(bp).values &&
            queue_count(cp).value==queue_count(bp).value && same_rows(rows,baseline_rows);
        const auto mapped_current=raw_flags(current,rules,types);
        const bool target_form=queue(cp).values==desired &&
            queue_count(cp).value==static_cast<std::int64_t>(desired.size()) &&
            mapped_current==desired_flags;
        require(original_form || baseline_form || target_form,
            "BNW research choice raw and owned state are partially synchronized");
        // Validate every original slot, including false flags and no-effect players.
        (void)raw_flags(original,rules,types);(void)raw_flags(previous,rules,types);
        out.i32(id);out.i32(queue_count(op).value);
        out.i32(static_cast<std::int64_t>(desired.size()));out.values(queue(op).values);
        out.values(desired);Writer changes;std::uint32_t n=0;std::set<std::int64_t> mapped;
        for(std::size_t row=0;row<old_rows.size();++row) {
            if(old_rows[row].type_hash==0) continue;
            const auto* type=types.find_hash(old_rows[row].type_hash);
            require(type && mapped.insert(type->id).second,
                "BNW research choice mapped flag row duplicate");
            const std::int64_t next=desired_flags.contains(type->id)?1:0;
            if(old_rows[row].values[0]==next) continue;
            require(n<max_flags,"BNW research choice flag change count exceeds limit");
            ++n;changes.u32(static_cast<std::uint32_t>(row));
            changes.i32(old_rows[row].values[0]);changes.i32(next);
        }
        require(std::all_of(desired_flags.begin(),desired_flags.end(),
            [&](auto tech){return mapped.contains(tech);}),
            "BNW research choice has no raw hash row for selected technology");
        out.u32(n);out.bytes.insert(out.bytes.end(),changes.bytes.begin(),changes.bytes.end());
        changed|=queue(op).values!=desired || n!=0;
        require(out.bytes.size()<=max_bytes,"BNW research choice section exceeds byte budget");
    }
    if(!changed) return std::nullopt;
    return saves::CivisSaveSection{bnw_research_queue_checkpoint_section_id,0,std::move(out.bytes)};
}
} // namespace

std::optional<BnwResearchQueueCheckpointBundle> detail::prepare_bnw_research_queue_bundle(
    const LiveSessionState &state,const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &source,const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,const SessionCheckpointIdentity &identity) {
    require(!state.player_early_publication && !state.player_research_publication &&
            !state.game_clock.pending,"BNW research choice has unpublished effects");
    selection_unchanged(state.research_selection,baseline.research_selection);
    owned_matches_raw(state.world,source,state.setup,state.technologies,
        state.player_research,state.research_turn,baseline,rules,types,true);
    const auto research=make_research_checkpoint(state.technologies,
        state.player_research,state.setup,rules,identity);
    require(!research.profile,"BNW research choice received profile checkpoint");
    const auto turn=make_research_turn_checkpoint(state.research_turn,state.setup,rules,identity);
    auto raw=direct_raw(state,baseline,source,rules,types,identity);
    if(!raw) return std::nullopt;
    return BnwResearchQueueCheckpointBundle{std::move(*raw),research.common,turn};
}

BnwResearchQueueCheckpointBundle make_bnw_research_queue_checkpoint_bundle(
    const LiveSessionState &state,const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &source,const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,const SessionCheckpointIdentity &identity) {
    auto result=detail::prepare_bnw_research_queue_bundle(state,baseline,source,rules,types,identity);
    require(result.has_value(),"BNW research choice section has no queue or flag change");
    return std::move(*result);
}

BnwResearchQueueRestored restore_bnw_research_queue_checkpoint_bundle(
    const BnwResearchQueueCheckpointBundle &bundle,
    const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &source,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity) {
    BnwResearchQueueRestored result{source,baseline.technologies,
        baseline.player_research,baseline.research_turn};
    restore_bnw_research_queue_checkpoint(bundle.raw,result.world,source,identity);
    restore_research_checkpoint(bundle.research,std::nullopt,
        result.technologies,result.players,baseline.setup,rules,identity);
    restore_research_turn_checkpoint(bundle.research_turn,result.turn,
        baseline.setup,rules,identity);
    owned_matches_raw(result.world,source,baseline.setup,result.technologies,
        result.players,result.turn,baseline,rules,types);
    return result;
}

} // namespace civ5::runtime
