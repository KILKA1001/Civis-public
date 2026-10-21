#include "civ5/runtime/bnw_personality_flavors.hpp"
#include <climits>
#include <set>
#include <stdexcept>
namespace civ5::runtime {
namespace {
const saves::PlayerKnowledgeBlock &flavor_block(
    const saves::PlayerKnowledgeObservation &observation) {
    const saves::PlayerKnowledgeBlock *found=nullptr;
    for(const auto &block:observation.blocks)
        if(block.name && std::string_view(block.name)=="flavor_manager") {
            if(found) throw std::runtime_error("BNW duplicate FlavorManager block");
            found=&block;
        }
    if(!found || observation.layout!=saves::GameStateLayout::expansion2_version1)
        throw std::runtime_error("BNW FlavorManager block unavailable");
    return *found;
}
}
const std::vector<std::int64_t> &checked_bnw_personality_flavors(
 const saves::SavedPlayerObservation &saved,const saves::SavedPlayerObservation &owned,
 const content::RulesDatabase &database) {
    const auto &a=flavor_block(saved.knowledge),&b=flavor_block(owned.knowledge);
    auto count=[](const saves::PlayerKnowledgeBlock &block) {
        std::optional<std::int64_t> value;
        for(const auto &entry:block.fields)
            if(entry.name && std::string_view(entry.name)=="flavors_wire_count") {
                if(value) throw std::runtime_error("BNW duplicate FlavorManager count");
                value=entry.value;
            }
        if(!value || *value<=0 || *value>1024)
            throw std::runtime_error("BNW FlavorManager count unavailable");
        return *value;
    };
    const auto n=count(a);
    if(count(b)!=n)
        throw std::runtime_error("BNW FlavorManager source/owned/rules dimensions differ");
    std::set<std::int64_t> ids;
    const auto report=database.query_rows({"SELECT ID FROM Flavors",{}},
        [&](const auto &row) {ids.insert(row.integer("ID"));},1024);
    if(!report.all_succeeded() || report.rows!=static_cast<std::uint64_t>(n) ||
       ids.size()!=static_cast<std::size_t>(n) ||
       *ids.begin()!=0 || *ids.rbegin()!=n-1)
        throw std::runtime_error("BNW active Flavors IDs differ from saved array dimensions");
    auto array=[&](const saves::PlayerKnowledgeBlock &block,
                   std::string_view name)->const std::vector<std::int64_t>& {
        const saves::PlotArrayObservation *found=nullptr;
        for(const auto &entry:block.arrays)
            if(entry.name && name==entry.name) {
                if(found) throw std::runtime_error("BNW duplicate FlavorManager array");
                found=&entry;
            }
        if(!found || found->columns!=1 ||
           found->values.size()!=static_cast<std::size_t>(n))
            throw std::runtime_error("BNW FlavorManager array dimensions unavailable");
        return found->values;
    };
    const auto &personality=array(a,"personality_flavors");
    if(array(b,"personality_flavors")!=personality ||
       array(b,"active_flavors")!=array(a,"active_flavors"))
        throw std::runtime_error("BNW source/owned FlavorManager arrays differ");
    for(const auto &value:personality)if(value<INT32_MIN||value>INT32_MAX)
        throw std::runtime_error("BNW native personality flavor exceeds int width");
    for(const auto &value:array(a,"active_flavors"))if(value<INT32_MIN||value>INT32_MAX)
        throw std::runtime_error("BNW native active flavor exceeds int width");
    return personality;
}
}
