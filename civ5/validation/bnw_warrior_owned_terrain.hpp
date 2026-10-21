#pragma once
#include "material_world_validation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"

namespace civ5::graphics {
struct BnwWarriorOwnedTerrainGenerationReceipt {
    std::string source_save_sha256,source_payload_sha256,input_sha256,feature2_sha256,river_edges_sha256,
        expanded_grid_sha256,final_height_pool_sha256;
    std::size_t verified_masks=0,marsh_writes=0;
    bool saved_river_edges=true,marsh_before_coast=true;
    std::uint8_t marsh_prior_byte=255;
    bool original_loading_rng_recovered=false;
};
// Unforgeable through a public aggregate: only the source-checked generator
// below creates this retained terrain/provenance pair. This proves our actual
// controlled generation call, not original native loading RNG/visual parity.
class BnwWarriorOwnedTerrain {
  public:
    const std::shared_ptr<const materials::MaterialWorld> &world() const {return world_;}
    const BnwWarriorOwnedTerrainGenerationReceipt &receipt() const {return receipt_;}
  private:
    friend BnwWarriorOwnedTerrain generate_bnw_gandhi_warrior_owned_terrain(
        const std::filesystem::path &,const std::filesystem::path &,
        const std::filesystem::path &,const std::filesystem::path &,
        const saves::SavedSessionSnapshot &);
    BnwWarriorOwnedTerrain(std::shared_ptr<const materials::MaterialWorld> world,
        BnwWarriorOwnedTerrainGenerationReceipt receipt):world_(std::move(world)),receipt_(std::move(receipt)){}
    std::shared_ptr<const materials::MaterialWorld> world_;
    BnwWarriorOwnedTerrainGenerationReceipt receipt_;
};
inline void verify_bnw_warrior_owned_terrain_cells(const saves::SavedSessionSnapshot &snapshot,
    std::span<const std::uint8_t> expanded) {
    if(!snapshot.map_terrain || !snapshot.map_projection)throw std::invalid_argument("Complete selected terrain/projection missing");
    const auto &map=snapshot.map_header;const auto &t=*snapshot.map_terrain;const auto &p=*snapshot.map_projection;
    if(map.grid.width!=40 || map.grid.height!=24 || t.grid.width!=40 || t.grid.height!=24 ||
       p.grid.width!=40 || p.grid.height!=24 || t.wrap_x!=map.wrap_x || t.wrap_y!=map.wrap_y ||
       p.wrap_x!=map.wrap_x || p.wrap_y!=map.wrap_y || t.cells.size()!=960 || p.cells.size()!=960 || expanded.size()!=40*28*8)
        throw std::invalid_argument("Complete selected terrain cardinality/layout differs");
    for(std::size_t x=0;x<40;++x)for(std::size_t y=0;y<24;++y) {
        const auto &cell=t.cells[x*24+y];const auto &position=p.cells[y*40+x];
        if(!cell.native_terrain_mask_known || position.index!=y*40+x || position.x!=x || position.y!=y)
            throw std::invalid_argument("Selected terrain mask/source position unresolved");
        const auto at=(x*28+y+2)*8;
        const auto mask=std::uint32_t(expanded[at])|(std::uint32_t(expanded[at+1])<<8)|(std::uint32_t(expanded[at+2])<<16)|(std::uint32_t(expanded[at+3])<<24);
        if(mask!=cell.native_terrain_mask)throw std::invalid_argument("Selected terrain/live source mask differs");
    }
}
inline BnwWarriorOwnedTerrain generate_bnw_gandhi_warrior_owned_terrain(
    const std::filesystem::path &save,const std::filesystem::path &asset_root,
    const std::filesystem::path &input_path,const std::filesystem::path &feature2_path,
    const saves::SavedSessionSnapshot &snapshot) {
    if(!std::filesystem::equivalent(asset_root,std::filesystem::path("C:/CrazyProject/civ5-assets")))
        throw std::invalid_argument("Canonical shared terrain AssetRoot required");
    BnwWarriorOwnedTerrainGenerationReceipt receipt;
    receipt.source_save_sha256=runtime::identify_file(save).sha256;
    receipt.source_payload_sha256=runtime::identify_bytes(saves::read_loaded_save(save).bytes).sha256;
    receipt.input_sha256=runtime::identify_file(input_path).sha256;
    receipt.feature2_sha256=runtime::identify_file(feature2_path).sha256;
    if(receipt.source_save_sha256!="b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab" ||
       receipt.input_sha256!="8e20560b2a1078340c27eb9133aad084649024113124f226d8048e3a57096093" ||
       receipt.feature2_sha256!="348f821ea2cc8b61b915cd975123bc1dcf05d815f9da17d73581fc3410092b80")
        throw std::invalid_argument("Selected controlled terrain generation source identity differs");
    auto input=primitive_input(input_path);auto &source=input.worlds.at(0);
    verify_bnw_warrior_owned_terrain_cells(snapshot,source.cells);
    if(source.input.mode!=terrain::game_world_grid_mode(snapshot.map_header.wrap_x) ||
       source.input.wrap_x!=snapshot.map_header.wrap_x || source.input.map_width!=40 || source.input.map_height!=24)
        throw std::invalid_argument("Selected generator layout/mode differs");
    receipt.verified_masks=960;receipt.expanded_grid_sha256=runtime::identify_bytes(source.cells).sha256;
    std::vector<terrain::TerrainRiverPlot> rivers;
    for(const auto &cell:snapshot.map_terrain->cells) {
        if(!cell.rivers)throw std::invalid_argument("Selected generated river source missing");const auto &r=*cell.rivers;
        rivers.push_back({r.is_river,r.west,r.northwest,r.northeast,r.flow_e,r.flow_se,r.flow_sw});
    }
    const auto grids=terrain::build_world_grid_layout(40,24,source.input.mode,source.input.factory);
    input.materials.at(0).edges=terrain::build_terrain_river_edges(grids.source_grid,rivers);
    std::string river_wire="BNW_SELECTED_RIVER_EDGES_V1;";
    for(const auto &r:input.materials.at(0).edges)river_wire+=std::to_string(r.x)+","+std::to_string(r.y)+","+std::to_string(r.z)+","+std::to_string(r.direction)+";";
    receipt.river_edges_sha256=runtime::identify_bytes(assets::Bytes(river_wire.begin(),river_wire.end())).sha256;
    const auto features=input_bytes(feature2_path);
    if(features.size()%20)throw std::invalid_argument("Selected feature2 layout differs");
    const auto word=[&](std::size_t a){std::uint32_t v=0;for(unsigned b=0;b<4;++b)v|=std::uint32_t(features[a+b])<<(b*8);return std::bit_cast<std::int32_t>(v);};
    std::vector<terrain::TerrainFeature2Source> marshes;
    for(std::size_t a=0;a<features.size();a+=20)marshes.push_back({{word(a),word(a+4),word(a+8)},word(a+12),word(a+16)});
    const assets::AssetEnvironment environment(asset_root);const auto textures=archive(environment,"Resource/DX9/TerrainTextures.fpk");
    const auto stamp_handle=textures->find(assets::AssetKey("marsh_stamp.dds"));
    if(!std::holds_alternative<assets::AssetHandle>(stamp_handle))throw std::invalid_argument("Selected authored marsh stamp missing");
    terrain::TerrainMarshStamp::Storage prior{};prior[0]=receipt.marsh_prior_byte;
    const auto stamp=terrain::TerrainMarshStamp::prepare(assets::DdsData::parse(std::get<assets::AssetHandle>(stamp_handle).read_bytes(65536)),prior);
    auto world=build_world(asset_root,input,0,&stamp,marshes,
        [&](auto,std::int32_t,auto){++receipt.marsh_writes;},receipt.saved_river_edges,receipt.marsh_before_coast);
    receipt.final_height_pool_sha256=runtime::identify_bytes(world->terrain().owner().height.pool_bytes()).sha256;
    return BnwWarriorOwnedTerrain(std::move(world),std::move(receipt));
}
} // namespace civ5::graphics
