#include "civ5/runtime/team_technologies.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
std::int64_t add(std::int64_t a, std::int64_t b) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
        throw std::overflow_error("Team technology counter exceeds int64");
    return a + b;
}
std::int64_t field(const saves::TeamObservation &saved, std::string_view name) {
    const saves::NamedSaveInteger *found = nullptr;
    for (const auto &item : saved.fields) {
        if (!item.name || item.name != name) continue;
        if (found) throw std::invalid_argument("Duplicate saved team technology field");
        found = &item;
    }
    if (!found) throw std::invalid_argument("Missing saved team technology field: " + std::string(name));
    return found->value;
}
const std::vector<std::int64_t> &array(const saves::TeamObservation &saved, std::string_view name,
                                       std::uint64_t count) {
    const saves::PlotArrayObservation *found = nullptr;
    for (const auto &item : saved.arrays) {
        if (!item.name || item.name != name) continue;
        if (found) throw std::invalid_argument("Duplicate saved team technology array");
        found = &item;
    }
    if (!found || found->columns != 1 || found->values.size() != count)
        throw std::invalid_argument("Invalid saved team technology array: " + std::string(name));
    return found->values;
}
} // namespace
TeamTechnologies::TeamTechnologies(content::TechnologyProfile profile, std::int64_t team, std::uint64_t slots)
    : profile_(profile), team_(team), slots_(slots) {
    if (profile != content::TechnologyProfile::bnw && profile != content::TechnologyProfile::vp542)
        throw std::invalid_argument("Unknown team technology profile");
    if (profile == content::TechnologyProfile::vp542)
        initialize_vp_team_technologies(extensions_);
}
std::uint64_t TeamTechnologies::occupied_slots() const {
    return profile_ == content::TechnologyProfile::vp542 ? vp_team_occupied_slots(*this) : values_.size();
}
std::vector<std::uint64_t> TeamTechnologies::occupied_indices() const {
    if (profile_ == content::TechnologyProfile::vp542) return vp_team_occupied_indices(*this);
    std::vector<std::uint64_t> result;
    result.reserve(values_.size());
    for (const auto &[slot, value] : values_) result.push_back(slot);
    return result;
}
std::uint64_t TeamTechnologies::index(std::int64_t tech) const {
    if (tech < 0 || static_cast<std::uint64_t>(tech) >= slots_)
        throw std::out_of_range("Team technology index");
    return static_cast<std::uint64_t>(tech);
}
TeamTechnologyValue TeamTechnologies::at(std::int64_t tech) const {
    const auto it = values_.find(index(tech));
    return it == values_.end() ? TeamTechnologyValue{} : it->second;
}
void TeamTechnologies::store(std::uint64_t slot, TeamTechnologyValue value) {
    if (value == TeamTechnologyValue{}) values_.erase(slot);
    else values_.insert_or_assign(slot, value);
}
bool TeamTechnologies::has_tech(std::int64_t tech) const { return tech == -1 || at(tech).has_tech; }
std::int64_t TeamTechnologies::progress_times100(std::int64_t tech) const {
    return tech == -1 ? 0 : at(tech).progress_times100;
}
std::int64_t TeamTechnologies::progress(std::int64_t tech) const { return progress_times100(tech) / 100; }
std::int64_t TeamTechnologies::known_count() const {
    if (profile_ == content::TechnologyProfile::vp542) return vp_team_known_count(*this);
    std::int64_t count = 0;
    for (const auto &[slot, value] : values_) if (value.has_tech) count = add(count, 1);
    return count;
}
bool TeamTechnologies::all_researched() const {
    if (profile_ == content::TechnologyProfile::vp542) return vp_team_all_researched(*this);
    std::uint64_t count = 0;
    for (const auto &[slot, value] : values_) if (value.has_tech || value.count > 0) ++count;
    return count == slots_;
}
bool TeamTechnologies::set_has_tech(std::int64_t tech, bool value, const HasTechHook &hook) {
    auto current = at(tech);
    if (current.has_tech == value) return false;
    current.has_tech = value;
    store(index(tech), current);
    if (value) last_ = tech;
    if (hook) hook(team_, tech, value);
    return true;
}
void TeamTechnologies::set_last_tech(std::int64_t tech) { (void)index(tech); last_ = tech; }
void TeamTechnologies::set_no_trade(std::int64_t tech, bool value) {
    auto current = at(tech); current.no_trade = value; store(index(tech), current);
}
void TeamTechnologies::set_progress_times100(std::int64_t tech, std::int64_t value) {
    auto current = at(tech); current.progress_times100 = value; store(index(tech), current);
}
void TeamTechnologies::increment_count(std::int64_t tech) {
    auto current = at(tech); current.count = add(current.count, 1); store(index(tech), current);
}
void TeamTechnologies::replace_live_values(std::int64_t last,
                                           std::map<std::uint64_t,TeamTechnologyValue> values) {
    if (last != -1) (void)index(last);
    for (const auto &[slot,value] : values) {
        if (slot > static_cast<std::uint64_t>(INT64_MAX) || slot >= slots_ ||
            value == TeamTechnologyValue{})
            throw std::invalid_argument("Invalid common team technology checkpoint row");
    }
    values_ = std::move(values);
    last_ = last;
}

TeamTechnologies restore_team_technologies(const saves::TeamObservation &saved,
    const content::TechnologyRules &rules, const content::InfoTypeCatalog &types, std::uint64_t row_budget) {
    if (!rules.all_succeeded() || !types.report().all_succeeded())
        throw std::invalid_argument("Team technology restore requires complete rules and global types");
    // GetID() is the saved CvTeam field, not the team's position in the stream.
    TeamTechnologies result(rules.profile, field(saved, "id"), rules.collection.slots);
    const bool vp = rules.profile == content::TechnologyProfile::vp542;
    if (field(saved, "tech_version") != 1 ||
        (vp && (field(saved, "tech_mod_version") != 151 || field(saved, "tech_sentinel") != 0xdeadbeef)))
        throw std::invalid_argument("Unverified saved team technology version");
    result.last_ = field(saved, "last_tech"); // Source does not remap this raw ID.
    result.remap_.last_tech_outside = result.last_ != -1 &&
        (result.last_ < 0 || static_cast<std::uint64_t>(result.last_) >= result.slots_);
    if (vp) vp_team_change_known_count(result, field(saved, "num_techs")); // Never reconstruct this from flags.
    const auto count = field(saved, "saved_techs");
    if (count < 0 || static_cast<std::uint64_t>(count) > row_budget)
        throw std::length_error("Saved team technologies exceed host row budget");
    result.remap_.saved = static_cast<std::uint64_t>(count);
    const auto &hashes = array(saved, "tech_hashes", count);
    const auto &has = array(saved, "has_tech", count);
    const auto &trade = array(saved, "no_trade_tech", count);
    const auto &progress = array(saved, vp ? "research_progress_times100" : "research_progress", count);
    const auto &counts = array(saved, "tech_count", count);
    const auto *eureka = vp ? &array(saved, "eureka_counter", count) : nullptr;
    std::set<std::uint64_t> visited;
    for (std::size_t i = 0; i < hashes.size(); ++i) {
        if (hashes[i] < 0 || static_cast<std::uint64_t>(hashes[i]) > UINT32_MAX ||
            (has[i] != 0 && has[i] != 1) || (trade[i] != 0 && trade[i] != 1))
            throw std::invalid_argument("Invalid saved technology wire hash or boolean");
        if (hashes[i] == 0) { ++result.remap_.sentinels; continue; }
        const auto *type = types.find_hash(static_cast<std::uint64_t>(hashes[i]));
        if (!type || type->id == -1) { ++result.remap_.unknown; continue; }
        if (type->id < 0 || static_cast<std::uint64_t>(type->id) >= result.slots_) {
            ++result.remap_.outside; continue;
        }
        const auto slot = static_cast<std::uint64_t>(type->id);
        ++result.remap_.mapped;
        if (!visited.insert(slot).second) ++result.remap_.duplicates;
        if (!rules.find(slot)) ++result.remap_.holes;
        // All arrays use the same remap. A later duplicate replaces every
        // component, including false/zero; importing does not call setters.
        result.store(slot, {has[i] != 0, trade[i] != 0, progress[i], counts[i]});
        if (eureka) vp_team_set_eureka(result, static_cast<std::int64_t>(slot), (*eureka)[i]);
    }
    return result;
}
std::vector<TeamTechnologies> restore_team_technologies(const saves::TeamsObservation &saved,
    const content::TechnologyRules &rules, const content::InfoTypeCatalog &types, std::uint64_t row_budget) {
    if (!rules.all_succeeded() || !types.report().all_succeeded())
        throw std::invalid_argument("Team technology restore requires complete rules and global types");
    if (!((saved.layout == saves::GameStateLayout::expansion2_version1 && rules.profile == content::TechnologyProfile::bnw) ||
          (saved.layout == saves::GameStateLayout::vp542_corpus && rules.profile == content::TechnologyProfile::vp542)))
        throw std::invalid_argument("Saved teams and technology rules use different profiles");
    if (saved.teams.size() > row_budget) throw std::length_error("Saved teams exceed host row budget");
    std::set<std::uint64_t> ids;
    std::vector<TeamTechnologies> result;
    for (const auto &team : saved.teams) {
        if (!ids.insert(team.index).second) throw std::invalid_argument("Duplicate saved team index");
        auto restored = restore_team_technologies(team, rules, types, row_budget);
        row_budget -= restored.remap().saved;
        result.push_back(std::move(restored));
    }
    return result;
}
} // namespace civ5::runtime
