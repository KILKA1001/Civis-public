#pragma once
#include "civ5/assets/asset_source.hpp"
#include "civ5/graphics/bnw_warrior_authored_formation.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/terrain_auxiliary_owners.hpp"
#include <map>
#include <string>

namespace civ5::graphics {
struct BnwUnitVisualConfiguration {
    float scene_spacing=0,member_multiplier=0;
    std::int32_t placement_seed=0;
    bool threaded=false;
    std::string config_sha256;
    assets::Provenance provenance;
};
// Caller selects this handle through its actual mounted VFS. No filesystem
// default, guessed global scale, or original-process requirement is introduced.
BnwUnitVisualConfiguration read_bnw_unit_visual_configuration(const assets::AssetHandle &handle);

struct BnwImportedUnitVisualRecord {
    std::int64_t owner=0,id=0,type=0,x=0,y=0,facing=0;
    bool embarked=false;
    std::uint32_t pool_index=0;
    bool operator==(const BnwImportedUnitVisualRecord &) const=default;
};
// Owns a fresh Civis renderer roster in serialized player/unit order, matching
// SDK setupGraphical's forward unit iterator. This is our fresh renderer pool,
// not a claim that original native heap indices survived loading. No lifecycle
// replay or native allocation/destruction is inferred for an already live pool.
class BnwImportedUnitVisualOwner {
  public:
    BnwImportedUnitVisualOwner(const saves::SavedSessionSnapshot &world,
                              BnwUnitVisualConfiguration configuration,std::int64_t warrior_type);
    const std::vector<BnwImportedUnitVisualRecord> &records() const noexcept {return records_;}
    const BnwUnitVisualConfiguration &configuration() const noexcept {return configuration_;}
    const BnwImportedUnitVisualRecord &find(std::int64_t owner,std::int64_t id) const;
    bool matches(const saves::SavedSessionSnapshot &world) const;
    BnwWarriorFormation warrior_formation(std::int64_t owner,std::int64_t id) const;
  private:
    BnwUnitVisualConfiguration configuration_;
    std::int64_t warrior_type_;
    std::vector<BnwImportedUnitVisualRecord> records_;
    std::map<std::pair<std::int64_t,std::int64_t>,std::size_t> identity_;
};
// 6DAEF0 ->4F3C80->5E1430 nearest u16 height query. Use the actual same-revision
// final height owner, not a rendered A8 texture or bilinear shader sample.
float bnw_unit_world_height(const terrain::HeightOwnerState &owner,float x,float y,bool wrap_x);
// 47A850 source grid mapping ->592840 world inverse/scales/centers. The world
// and height owner must describe the same current terrain revision.
BnwWarriorFormationPoint bnw_unit_world_center(const terrain::WorldLayout &world,
    const terrain::HeightOwnerParameters &owner,std::int32_t map_x,std::int32_t map_y);
// Native geometry-owner constructor 7A1934/7A1946 initializes +5B4 from
// float constant 9F25CC. Caller retains this value in its fresh render owner.
constexpr float bnw_unit_initial_minimum_height() { return 10.116731643676758F; }
// Actual generated scene transforms. Minimum floor and member model scale must
// come from their renderer/rules owners. No BnwWarriorPlacementReceipt or
// production flags are created, and no simulation state/RNG is mutated.
std::array<std::array<float,16>,12> bnw_warrior_owned_scene_transforms(
    const BnwImportedUnitVisualOwner &units,const saves::SavedSessionSnapshot &current,
    std::int64_t player,std::int64_t unit,const terrain::WorldLayout &world,
    const terrain::HeightOwnerState &height,bool wrap_x,float minimum_height,float member_scale);
} // namespace civ5::graphics
