#include "civ5/runtime/bnw_team_progress_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','T','P','R','1'};
constexpr std::size_t team_count=64;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void sha(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0' && c<='9') || (c>='a' && c<='f');
            }),"Invalid BNW team progress SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW team progress exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(value.size()<=UINT32_MAX,"BNW team schema name too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW team progress checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(i*8);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW team progress checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(i*8);
        return value;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW team progress source, rules or schema changed");
        at+=expected.size();
    }
};
const saves::NamedSaveInteger &field(const saves::TeamObservation &team,
                                      std::string_view wanted) {
    const saves::NamedSaveInteger *result=nullptr;
    for(const auto &item:team.fields) if(name(item.name)==wanted) {
        require(!result,"Duplicate BNW team field");result=&item;
    }
    require(result,"Missing BNW team field");return *result;
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                         std::string_view wanted) {
    const saves::PlotArrayObservation *result=nullptr;
    for(const auto &item:team.arrays) if(name(item.name)==wanted) {
        require(!result,"Duplicate BNW team array");result=&item;
    }
    require(result && result->columns==1,"Missing BNW team array");return *result;
}
saves::PlotArrayObservation &progress(saves::TeamObservation &team) {
    (void)array(team,"research_progress");
    for(auto &item:team.arrays) if(name(item.name)=="research_progress") return item;
    throw std::runtime_error("Missing BNW research progress");
}
const saves::PlotArrayObservation &progress(const saves::TeamObservation &team) {
    return array(team,"research_progress");
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.teams && world.teams->layout==bnw &&
            world.teams->dimensions.teams==team_count &&
            world.teams->dimensions.profile_extensions.values.empty() &&
            world.teams->teams.size()==team_count,
            "BNW team progress requires complete plain team roster");
    for(std::size_t i=0;i<team_count;++i) {
        const auto &team=world.teams->teams[i];
        const auto saved=field(team,"saved_techs").value;
        require(team.index==i && field(team,"id").value==static_cast<std::int64_t>(i) &&
                field(team,"tech_version").value==1 && saved>=0 &&
                static_cast<std::uint64_t>(saved)==progress(team).values.size() &&
                array(team,"tech_hashes").values.size()==progress(team).values.size() &&
                array(team,"has_tech").values.size()==progress(team).values.size() &&
                array(team,"no_trade_tech").values.size()==progress(team).values.size() &&
                array(team,"tech_count").values.size()==progress(team).values.size(),
                "BNW team progress ID or technology row count changed");
    }
}
std::string masked_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    const auto &teams=*world.teams;
    const auto &d=teams.dimensions;
    out.u64(d.teams);out.u64(d.players);out.u64(d.civ_teams);
    out.u64(d.fixed_players);out.u64(d.fixed_teams);out.u64(d.domains);
    out.u64(d.major);out.u64(d.yields);
    out.u64(teams.start_offset);out.u64(teams.end_offset);
    for(const auto &team:teams.teams) {
        out.u64(team.index);out.u64(team.start_offset);out.u64(team.end_offset);
        out.u64(team.fields.size());
        for(const auto &f:team.fields) {out.string(name(f.name));out.i64(f.value);}
        out.u64(team.arrays.size());
        for(const auto &a:team.arrays) {
            out.string(name(a.name));out.u64(a.columns);out.u64(a.values.size());
            const bool mask=name(a.name)=="research_progress";
            for(const auto v:a.values) out.i64(mask?0:v);
        }
        out.u64(team.hashes.size());
        for(const auto &h:team.hashes) {
            out.string(name(h.name));out.i64(h.wire_count);out.u64(h.rows.size());
            for(const auto &row:h.rows) {
                out.u64(row.type_hash);out.u64(row.values.size());
                for(const auto v:row.values) out.i64(v);
            }
        }
        out.u64(team.project_art.size());
        for(const auto &art:team.project_art) {
            out.i64(art.key);out.i64(art.source_count);
            out.u32(art.wire_count.has_value());
            if(art.wire_count) out.u64(*art.wire_count);
            out.u64(art.values.size());for(const auto v:art.values) out.i64(v);
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void players_unchanged(const PlayerResearchContext &live,
                       const PlayerResearchContext &baseline) {
    require(live.profile==content::TechnologyProfile::bnw &&
            baseline.profile==content::TechnologyProfile::bnw &&
            live.world_city_modifier==baseline.world_city_modifier &&
            live.barbarian_team==baseline.barbarian_team &&
            live.civ_teams==baseline.civ_teams &&
            live.major_players==baseline.major_players &&
            live.bnw_minor_scan_players==baseline.bnw_minor_scan_players &&
            live.players.size()==baseline.players.size() &&
            live.teams.size()==baseline.teams.size() &&
            live.leagues.size()==baseline.leagues.size(),
            "BNW team progress changed player research context");
    for(const auto &[id,old]:baseline.players) {
        const auto found=live.players.find(id);
        require(found!=live.players.end(),"BNW player research roster changed");
        const auto &p=found->second;
        require(p.queue==old.queue && p.overflow_times100==old.overflow_times100 &&
                p.max_effective_cities==old.max_effective_cities &&
                p.capital==old.capital && p.pregame_minor==old.pregame_minor &&
                p.cities.size()==old.cities.size() &&
                p.profile_extensions.values.empty() && old.profile_extensions.values.empty(),
                "BNW team progress changed player research state");
        for(const auto &[city,old_city]:old.cities) {
            const auto it=p.cities.find(city);
            require(it!=p.cities.end() && it->second.puppet==old_city.puppet &&
                    it->second.ignore_happiness==old_city.ignore_happiness &&
                    it->second.razing==old_city.razing &&
                    it->second.profile_extensions.values.empty() &&
                    old_city.profile_extensions.values.empty(),
                    "BNW team progress changed research city state");
        }
    }
    for(const auto &[id,old]:baseline.teams) {
        const auto it=live.teams.find(id);
        require(it!=live.teams.end(),"BNW research team roster changed");
        const auto &t=it->second;
        require(t.alive_count==old.alive_count && t.master==old.master &&
                t.liberated_by==old.liberated_by && t.members==old.members &&
                t.met==old.met && t.embassy==old.embassy && t.at_war==old.at_war &&
                t.war_peace.has_value()==old.war_peace.has_value() &&
                (!t.war_peace || (t.war_peace->locked_turns==old.war_peace->locked_turns &&
                                  t.war_peace->permanent==old.war_peace->permanent)),
                "BNW team progress changed research team context");
    }
    for(std::size_t i=0;i<live.leagues.size();++i)
        require(live.leagues[i].members==baseline.leagues[i].members &&
                live.leagues[i].discovered_tech_modifiers==
                    baseline.leagues[i].discovered_tech_modifiers,
                "BNW team progress changed league research context");
}
void setup_unchanged(const ResearchContext &live,
                     const ResearchContext &baseline) {
    require(live.profile==content::TechnologyProfile::bnw &&
            baseline.profile==content::TechnologyProfile::bnw &&
            live.extensions.values.empty() && baseline.extensions.values.empty() &&
            live.game_type==baseline.game_type && live.speed==baseline.speed &&
            live.start_era==baseline.start_era &&
            live.current_era==baseline.current_era &&
            live.game_handicap==baseline.game_handicap &&
            live.game_turn==baseline.game_turn &&
            live.map_random_seed==baseline.map_random_seed &&
            live.world_percent==baseline.world_percent &&
            live.team_members==baseline.team_members &&
            live.pitboss==baseline.pitboss &&
            live.players.size()==baseline.players.size() &&
            live.game_options.size()==baseline.game_options.size(),
            "BNW team progress changed research setup");
    for(const auto &[id,old]:baseline.players) {
        const auto it=live.players.find(id);
        require(it!=live.players.end() && it->second.team==old.team &&
                it->second.handicap==old.handicap &&
                it->second.slot_status==old.slot_status &&
                it->second.slot_claim==old.slot_claim &&
                it->second.alive==old.alive &&
                it->second.ever_alive==old.ever_alive &&
                it->second.policies==old.policies &&
                it->second.policies_resolved==old.policies_resolved,
                "BNW team progress changed research setup player");
    }
    for(std::size_t i=0;i<live.game_options.size();++i)
        require(live.game_options[i].name==baseline.game_options[i].name &&
                live.game_options[i].value==baseline.game_options[i].value,
                "BNW team progress changed research setup option");
}
void owned_matches_raw(std::span<const TeamTechnologies> live,
                       std::span<const TeamTechnologies> baseline,
                       const saves::SavedSessionSnapshot &world,
                       const saves::SavedSessionSnapshot &source,
                       const content::TechnologyRules &rules,
                       const content::InfoTypeCatalog &types) {
    require(rules.profile==content::TechnologyProfile::bnw &&
            rules.all_succeeded() && types.report().all_succeeded() &&
            live.size()==team_count && baseline.size()==team_count,
            "BNW team progress rules or owned roster changed");
    const auto decoded=restore_team_technologies(*world.teams,rules,types);
    const auto original=restore_team_technologies(*source.teams,rules,types);
    require(decoded.size()==team_count && original.size()==team_count,
            "BNW team progress decoded roster changed");
    for(std::size_t i=0;i<team_count;++i) {
        const auto &a=live[i],&b=baseline[i],&c=decoded[i],&d=original[i];
        require(a.profile()==rules.profile && b.profile()==rules.profile &&
                a.team()==static_cast<std::int64_t>(i) &&
                b.team()==a.team() && c.team()==a.team() && d.team()==a.team() &&
                a.slots()==rules.collection.slots && b.slots()==a.slots() &&
                a.remap().all_mapped() && b.remap().all_mapped() &&
                c.remap().all_mapped() && d.remap().all_mapped() &&
                a.remap().duplicates==0 && b.remap().duplicates==0 &&
                c.remap().duplicates==0 && d.remap().duplicates==0 &&
                a.remap().holes==0 && b.remap().holes==0 &&
                c.remap().holes==0 && d.remap().holes==0 &&
                a.profile_extensions().values.empty() &&
                b.profile_extensions().values.empty() &&
                a.last_tech()==b.last_tech() && b.last_tech()==d.last_tech() &&
                a.last_tech()==c.last_tech(),
                "BNW team progress identity or last technology changed");
        const auto &old_rows=progress(source.teams->teams[i]).values;
        const auto &live_rows=progress(world.teams->teams[i]).values;
        const auto &hashes=array(source.teams->teams[i],"tech_hashes").values;
        std::set<std::int64_t> changed_slots;
        for(std::size_t row=0;row<old_rows.size();++row) if(old_rows[row]!=live_rows[row]) {
            require(hashes[row]>0 &&
                    static_cast<std::uint64_t>(hashes[row])<=UINT32_MAX,
                    "BNW progress changed an unresolved saved technology row");
            const auto *type=types.find_hash(static_cast<std::uint64_t>(hashes[row]));
            require(type && type->id>=0 &&
                    static_cast<std::uint64_t>(type->id)<a.slots() &&
                    rules.find(static_cast<std::uint64_t>(type->id)) &&
                    changed_slots.insert(type->id).second,
                    "BNW progress changed an ambiguous technology row");
        }
        for(std::uint64_t slot=0;slot<a.slots();++slot) {
            const auto id=static_cast<std::int64_t>(slot);
            const auto av=a.at(id),bv=b.at(id),cv=c.at(id),dv=d.at(id);
            require(av==cv && bv==dv &&
                    av.has_tech==bv.has_tech && av.no_trade==bv.no_trade &&
                    av.count==bv.count,
                    "BNW team progress raw/owned or non-progress field changed");
        }
    }
}
} // namespace

saves::CivisSaveSection make_bnw_team_progress_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const ResearchCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    const auto source_hash=masked_hash(source);
    require(masked_hash(world)==source_hash,
            "BNW team progress has unsupported raw team mutation");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.u32(team_count);
    std::uint32_t changed=0;
    for(std::size_t team=0;team<team_count;++team) {
        const auto &old=progress(source.teams->teams[team]).values;
        const auto &now=progress(world.teams->teams[team]).values;
        for(std::size_t row=0;row<old.size();++row)
            if(old[row]!=now[row]) {
                require(team<=UINT32_MAX && row<=UINT32_MAX && changed<UINT32_MAX,
                        "BNW team progress row count exceeds wire range");
                ++changed;out.u32(static_cast<std::uint32_t>(team));
                out.u32(static_cast<std::uint32_t>(row));
                out.i32(old[row]);out.i32(now[row]);
            }
    }
    require(changed>0,"BNW team progress checkpoint has no change");
    out.u32(changed);
    return {bnw_team_progress_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_team_progress_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const ResearchCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    require(section.id==bnw_team_progress_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*3+4+4+16 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW team progress checkpoint");
    const auto source_hash=masked_hash(source);
    require(masked_hash(world)==source_hash,
            "BNW team progress destination changed");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW team progress version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(source_hash);
    require(in.u32()==team_count,"BNW team progress roster changed");
    const auto remaining=section.bytes.size()-in.at;
    require(remaining>=20 && (remaining-4)%16==0,
            "Invalid BNW team progress row encoding");
    const auto rows=(remaining-4)/16;
    std::vector<std::array<std::int64_t,4>> pending;
    pending.reserve(rows);
    std::uint64_t previous=0;
    for(std::size_t n=0;n<rows;++n) {
        const auto team=in.u32(),row=in.u32();
        const auto old=in.i32(),live=in.i32();
        require(team<team_count && row<progress(source.teams->teams[team]).values.size() &&
                old!=live && progress(source.teams->teams[team]).values[row]==old &&
                progress(world.teams->teams[team]).values[row]==old,
                "BNW team progress source row changed");
        const auto key=(std::uint64_t(team)<<32)|row;
        require(n==0 || key>previous,"BNW team progress rows unordered or duplicate");
        previous=key;pending.push_back({team,row,old,live});
    }
    require(in.u32()==rows && in.at==section.bytes.size(),
            "BNW team progress row count mismatch");
    // All checks precede the first destination write.
    for(const auto &r:pending)
        progress(world.teams->teams[static_cast<std::size_t>(r[0])]).values[
            static_cast<std::size_t>(r[1])]=r[3];
}

BnwTeamProgressCheckpointPair make_bnw_team_progress_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &baseline_setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const ResearchCheckpointIdentity &identity) {
    setup_unchanged(state.setup,baseline_setup);
    players_unchanged(state.player_research,baseline_players);
    owned_matches_raw(state.technologies,baseline_technologies,state.world,source,rules,types);
    const auto raw=make_bnw_team_progress_checkpoint(state.world,source,identity);
    const auto owned=make_research_checkpoint(state.technologies,state.player_research,
        state.setup,rules,identity);
    require(!owned.profile,"BNW team progress received profile research section");
    return {raw,owned.common};
}

BnwTeamProgressRestored restore_bnw_team_progress_checkpoint_pair(
    const BnwTeamProgressCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const ResearchCheckpointIdentity &identity) {
    BnwTeamProgressRestored result{source,
        {baseline_technologies.begin(),baseline_technologies.end()},baseline_players};
    restore_bnw_team_progress_checkpoint(pair.raw,result.world,source,identity);
    restore_research_checkpoint(pair.owned,std::nullopt,result.technologies,
        result.player_research,setup,rules,identity);
    players_unchanged(result.player_research,baseline_players);
    owned_matches_raw(result.technologies,baseline_technologies,result.world,source,rules,types);
    return result;
}

} // namespace civ5::runtime
