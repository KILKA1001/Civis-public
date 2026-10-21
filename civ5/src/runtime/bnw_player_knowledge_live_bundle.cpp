#include "civ5/runtime/bnw_player_knowledge_live_bundle.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
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
void require_research_turn_matches(const LiveSessionState &baseline,
                                   const content::TechnologyRules &technologies,
                                   const content::InfoTypeCatalog &types,
                                   const content::RulesDatabase &database) {
    const auto raw=restore_research_turn(baseline.world,baseline.setup,
                                          technologies,types,database);
    const auto &owned=baseline.research_turn;
    require(raw.profile==owned.profile &&
            raw.players.size()==owned.players.size(),
            "BNW ID35 research-turn owner differs from raw player knowledge");
    for(const auto &[id,source]:raw.players) {
        const auto found=owned.players.find(id);
        require(found!=owned.players.end(),
                "BNW ID35 research-turn player roster changed");
        const auto &current=found->second;
        require(source.founded_first_city==current.founded_first_city &&
                source.cities_founded==current.cities_founded &&
                source.agreement_counters==current.agreement_counters &&
                source.researching==current.researching &&
                source.leader_type==current.leader_type &&
                source.unmapped_researching==current.unmapped_researching,
                "BNW ID35 research-turn state and raw knowledge diverged");
    }
}
std::size_t validate_prophets_only(const saves::SavedSessionSnapshot &before,
                                   const saves::SavedSessionSnapshot &after) {
    require(before.all_players && after.all_players &&
            before.all_players->players.size()==64 &&
            after.all_players->players.size()==64,
            "BNW ID35 requires complete player knowledge roster");
    std::size_t changed_players=0;
    for(std::size_t i=0;i<64;++i) {
        const auto &left=before.all_players->players[i].knowledge.blocks;
        const auto &right=after.all_players->players[i].knowledge.blocks;
        require(left.size()==4 && right.size()==4,
                "BNW ID35 player knowledge block roster changed");
        for(std::size_t block=0;block<4;++block) {
            const auto &a=left[block],&b=right[block];
            require(name(a.name)==name(b.name) &&
                    a.fields.size()==b.fields.size() &&
                    a.arrays.size()==b.arrays.size() &&
                    a.hashes.size()==b.hashes.size(),
                    "BNW ID35 player knowledge block structure changed");
            for(std::size_t field=0;field<a.fields.size();++field) {
                const auto &old=a.fields[field],&now=b.fields[field];
                require(name(old.name)==name(now.name),
                        "BNW ID35 player knowledge field identity changed");
                if(block==0 && name(old.name)=="prophets_spawned") {
                    require(old.value>=0 && now.value>=old.value &&
                            now.value<INT32_MAX,
                            "BNW ID35 prophet counter is outside safe monotonic i32 range");
                    changed_players+=old.value!=now.value;
                } else require(old.value==now.value,
                               "BNW ID35 changed an unsupported religion, research or AI scalar");
            }
            for(std::size_t array=0;array<a.arrays.size();++array)
                require(name(a.arrays[array].name)==name(b.arrays[array].name) &&
                        a.arrays[array].values==b.arrays[array].values,
                        "BNW ID35 changed research or flavor array values");
            for(std::size_t hash=0;hash<a.hashes.size();++hash) {
                const auto &x=a.hashes[hash],&y=b.hashes[hash];
                require(name(x.name)==name(y.name) &&
                        x.wire_count==y.wire_count && x.rows.size()==y.rows.size(),
                        "BNW ID35 changed a hashed research collection");
                for(std::size_t row=0;row<x.rows.size();++row)
                    require(x.rows[row].type_hash==y.rows[row].type_hash &&
                            x.rows[row].values==y.rows[row].values,
                            "BNW ID35 changed a research flag or TechAI value");
            }
        }
    }
    require(changed_players>0,
            "BNW ID35 prophet bundle requires an actual supported change");
    return changed_players;
}
} // namespace

BnwPlayerKnowledgeProphetBundle restore_bnw_player_knowledge_prophet_bundle(
    const saves::CivisSaveSection &section,
    const LiveSessionState &baseline,
    const saves::SavedSession &source,
    const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity) {
    require(baseline.setup.profile==content::TechnologyProfile::bnw &&
            baseline.research_turn.profile==content::TechnologyProfile::bnw &&
            technologies.profile==content::TechnologyProfile::bnw &&
            technologies.all_succeeded() && types.report().all_succeeded() &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            identify_bytes(database.gameplay_fingerprint_data()).sha256==
                identity.rules_sha256,
            "BNW ID35 source, active rules or owned profile changed");
    require_research_turn_matches(baseline,technologies,types,database);
    auto world=restore_bnw_player_knowledge_checkpoint(
        section,baseline.world,source.snapshot,identity);
    const auto changed=validate_prophets_only(baseline.world,world);
    BnwPlayerKnowledgeProphetBundle result{baseline,changed};
    result.state.world=std::move(world);
    return result;
}

} // namespace civ5::runtime
