#include "civ5/runtime/bnw_unit_jump_composite.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
const saves::MapRegionContainer &areas(const saves::MapRegionsObservation &regions) {
    const saves::MapRegionContainer *found=nullptr;
    for(const auto &container:regions.containers)
        if(name(container.name)=="areas") {
            require(!found,"Duplicate BNW jump area container");
            found=&container;
        }
    require(found,"Missing BNW jump area container");return *found;
}
saves::MapRegionContainer &areas(saves::MapRegionsObservation &regions) {
    (void)areas(static_cast<const saves::MapRegionsObservation &>(regions));
    for(auto &container:regions.containers)
        if(name(container.name)=="areas") return container;
    throw std::runtime_error("Missing BNW jump area container");
}
std::int64_t id(const saves::MapRegionRecord &record) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:record.fields)
        if(name(field.name)=="id") {
            require(!found,"Duplicate BNW jump area ID");found=&field;
        }
    require(found,"Missing BNW jump area ID");return found->value;
}
const saves::PlotArrayObservation &revealed(const saves::MapRegionRecord &record) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:record.arrays)
        if(name(array.name)=="revealed_tiles") {
            require(!found,"Duplicate BNW jump area reveals");
            found=&array;
        }
    require(found && found->columns==1 && found->values.size()==80,
            "Missing BNW jump area reveals");
    return *found;
}
saves::PlotArrayObservation &revealed(saves::MapRegionRecord &record) {
    (void)revealed(static_cast<const saves::MapRegionRecord &>(record));
    for(auto &array:record.arrays)
        if(name(array.name)=="revealed_tiles") return array;
    throw std::runtime_error("Missing BNW jump area reveals");
}
saves::SavedSessionSnapshot mask_sight_areas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source) {
    require(world.map_regions && source.map_regions,
            "BNW jump composition requires decoded areas");
    auto copy=world;
    const auto &from=areas(*source.map_regions);
    auto &to=areas(*copy.map_regions);
    require(from.records.size()==to.records.size(),
            "BNW jump composition area roster changed");
    for(std::size_t i=0;i<from.records.size();++i) {
        require(id(from.records[i])==id(to.records[i]),
                "BNW jump composition area ID changed");
        revealed(to.records[i]).values=revealed(from.records[i]).values;
    }
    return copy;
}
void check_transfer(
    const saves::SavedSessionSnapshot &source,
    BnwUnitJumpCheckpointTarget target,
    const BnwAreaUnitTransfer &transfer) {
    require(target.from_plot==transfer.from_plot &&
            target.to_plot==transfer.to_plot &&
            target.player_index<source.all_players->players.size() &&
            source.all_players->players[target.player_index].saved_player_id==
                transfer.owner,
            "BNW jump area transfer disagrees with moved unit");
    const auto planned=plan_bnw_area_unit_transfer(
        source,transfer.owner,transfer.from_plot,transfer.to_plot);
    require(planned.owner==transfer.owner &&
            planned.from_area==transfer.from_area &&
            planned.to_area==transfer.to_area &&
            planned.from_units==transfer.from_units &&
            planned.from_owner_units==transfer.from_owner_units &&
            planned.to_units==transfer.to_units &&
            planned.to_owner_units==transfer.to_owner_units,
            "BNW jump area transfer source counters changed");
}
bool same_section(const saves::CivisSaveSection &a,
                  const saves::CivisSaveSection &b) {
    return a.id==b.id && a.flags==b.flags && a.bytes==b.bytes;
}
} // namespace

BnwUnitJumpCompositeSections make_bnw_unit_jump_composite(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target,
    const BnwAreaUnitTransfer &transfer,
    std::int64_t team) {
    require(source.all_players.has_value(),
            "BNW jump composition requires player roster");
    check_transfer(source,target,transfer);
    BnwUnitJumpCompositeSections result;
    result.unit=make_bnw_unit_jump_checkpoint(world,source,identity,target);
    result.sight=make_bnw_unit_sight_checkpoint(world,source,identity,team);
    const auto masked=mask_sight_areas(world,source);
    result.area=make_bnw_area_unit_checkpoint(
        masked,source,identity,transfer);
    return result;
}

void restore_bnw_unit_jump_composite(
    const BnwUnitJumpCompositeSections &sections,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target,
    const BnwAreaUnitTransfer &transfer,
    std::int64_t team) {
    require(world.all_players.has_value(),
            "BNW jump composition requires player roster");
    check_transfer(world,target,transfer);
    const auto source=world;
    auto draft=world;
    restore_bnw_unit_jump_checkpoint(sections.unit,draft,identity);
    restore_bnw_area_unit_checkpoint(sections.area,draft,identity);
    restore_bnw_unit_sight_checkpoint(sections.sight,draft,identity);
    const auto checked=make_bnw_unit_jump_composite(
        draft,source,identity,target,transfer,team);
    require(same_section(checked.unit,sections.unit) &&
            same_section(checked.area,sections.area) &&
            same_section(checked.sight,sections.sight),
            "BNW jump composed sections disagree after restore");
    world=std::move(draft);
}
} // namespace civ5::runtime
