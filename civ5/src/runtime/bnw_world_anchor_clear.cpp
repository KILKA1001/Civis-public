#include "civ5/runtime/bnw_world_anchor_clear.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::int64_t &field(saves::PlotObservation &plot,std::string_view name) {
    saves::NamedSaveInteger *found=nullptr;
    for(auto &value:plot.fields)
        if(value.name && std::string_view(value.name)==name) {
            require(!found,"Duplicate BNW world anchor plot field");
            found=&value;
        }
    require(found,"Missing BNW world anchor plot field");
    return found->value;
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &value:plot.fields)
        if(value.name && std::string_view(value.name)==name) {
            require(!found,"Duplicate BNW world anchor plot field");
            found=&value;
        }
    require(found,"Missing BNW world anchor plot field");
    return found->value;
}
} // namespace

BnwWorldAnchorClearPlan plan_bnw_world_anchor_clear(
    const LiveSessionState &state,std::uint64_t index) {
    BnwWorldAnchorClearPlan plan;
    try {
        const auto &world=state.world;
        require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
                world.map_plots && world.map_plots->layout==bnw &&
                world.map_projection &&
                world.map_plots->plots.size()==world.map_header.grid.size() &&
                world.map_projection->cells.size()==world.map_header.grid.size() &&
                index<world.map_header.grid.size(),
                "BNW world anchor requires complete owned map and plot");
        const auto &plot=world.map_plots->plots.at(static_cast<std::size_t>(index));
        const auto &cell=world.map_projection->cells.at(static_cast<std::size_t>(index));
        const auto xy=world.map_header.grid.coordinates(index);
        require(plot.index==index && cell.index==index &&
                plot.x==xy[0] && plot.y==xy[1] &&
                cell.x==xy[0] && cell.y==xy[1] &&
                xy[0]<=INT32_MAX && xy[1]<=INT32_MAX,
                "BNW world anchor plot identity or source coordinates invalid");
        const auto anchor=field(plot,"world_anchor");
        const auto old_data=field(plot,"world_anchor_data");
        require(anchor>=-1 && anchor<=127 && old_data>=-128 && old_data<=127,
                "BNW world anchor exceeds signed source byte range");
        if(anchor!=-1)
            plan.removal=BnwWorldAnchorRemovalEvent{
                index,anchor,old_data,static_cast<std::int64_t>(xy[0]),
                static_cast<std::int64_t>(xy[1]),false,-1,
                world.map_header.guid,world.map_header.grid.width,
                world.map_header.grid.height};
        plan.valid=true;
    } catch(const std::exception &error) {
        plan.reason=error.what();
    }
    return plan;
}

BnwWorldAnchorJournal::BnwWorldAnchorJournal(BnwWorldAnchorEmitter emitter)
    :emitter_(std::move(emitter)) {}

LiveActionResult stage_bnw_world_anchor_clear(
    LiveSessionState &draft,std::uint64_t index,BnwWorldAnchorJournal &journal) {
    const auto plan=plan_bnw_world_anchor_clear(draft,index);
    if(!plan.valid) return {LiveActionOutcome::rejected,plan.reason};
    if(journal.uncertain_ || journal.delivered_)
        return {LiveActionOutcome::rejected,
                "BNW world anchor journal has already begun publication"};
    for(const auto &event:journal.events_)
        if(event.plot==index)
            return {LiveActionOutcome::rejected,
                    "BNW world anchor plot already staged"};
    if(plan.removal && !journal.emitter_)
        return {LiveActionOutcome::rejected,
                "BNW GameplayWorldAnchor removal emitter unavailable"};
    try {
        auto &plot=draft.world.map_plots->plots.at(static_cast<std::size_t>(index));
        auto &anchor=field(plot,"world_anchor");
        auto &data=field(plot,"world_anchor_data");
        // Allocate the journal entry before touching the detached plot.
        if(plan.removal) journal.events_.push_back(*plan.removal);
        anchor=-1;
        data=-1;
    } catch(const std::exception &error) {
        return {LiveActionOutcome::rejected,error.what()};
    }
    return {LiveActionOutcome::committed,{}};
}

BnwWorldAnchorPublication BnwWorldAnchorJournal::publish(
    const LiveSessionState &committed) {
    if(uncertain_)
        return {BnwWorldAnchorPublicationStatus::uncertain,delivered_,
                "BNW GameplayWorldAnchor delivery is uncertain; do not retry"};
    if(!emitter_ && !events_.empty())
        return {BnwWorldAnchorPublicationStatus::rejected,delivered_,
                "BNW GameplayWorldAnchor removal emitter unavailable"};
    // Verify every staged write before the first external event. This catches
    // publication against an unrelated or uncommitted world.
    for(const auto &event:events_) {
        const auto plan=plan_bnw_world_anchor_clear(committed,event.plot);
        if(!plan.valid || plan.removal ||
           committed.world.map_header.guid!=event.map_guid ||
           committed.world.map_header.grid.width!=event.map_width ||
           committed.world.map_header.grid.height!=event.map_height ||
           field(committed.world.map_plots->plots.at(
               static_cast<std::size_t>(event.plot)),"world_anchor_data")!=-1 ||
           static_cast<std::int64_t>(committed.world.map_plots->plots.at(
               static_cast<std::size_t>(event.plot)).x)!=event.x ||
           static_cast<std::int64_t>(committed.world.map_plots->plots.at(
               static_cast<std::size_t>(event.plot)).y)!=event.y)
            return {BnwWorldAnchorPublicationStatus::rejected,delivered_,
                    "BNW GameplayWorldAnchor committed plot differs from staged clear"};
    }
    while(delivered_<events_.size()) {
        // Mark uncertain before crossing the external boundary. If the engine
        // throws after processing, retry would duplicate its observable event.
        uncertain_=true;
        try {emitter_(events_[delivered_]);}
        catch(const std::exception &error) {
            return {BnwWorldAnchorPublicationStatus::uncertain,delivered_,error.what()};
        }
        catch(...) {
            return {BnwWorldAnchorPublicationStatus::uncertain,delivered_,
                    "BNW GameplayWorldAnchor callback failed"};
        }
        ++delivered_;
        uncertain_=false;
    }
    return {BnwWorldAnchorPublicationStatus::complete,delivered_,{}};
}

} // namespace civ5::runtime
