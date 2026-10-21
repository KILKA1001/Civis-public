#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include <iostream>
#include <algorithm>
#include <bit>
#include <limits>
using namespace civ5;
void require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
saves::SavedUnitObservation unit(std::uint64_t index,std::int64_t owner,std::int64_t id,std::int64_t facing) {
    saves::SavedUnitObservation u;u.index=index;u.core.layout=saves::GameStateLayout::expansion2_version1;
    auto add=[&](const char *name,std::int64_t value){saves::UnitCoreField f{};f.name=name;f.encoding=saves::UnitCoreEncoding::i32;f.values={value};u.core.fields.push_back(std::move(f));};
    add("m_eOwner",owner);add("m_iID",id);add("m_eUnitType",1);add("m_iX",32);add("m_iY",14);add("m_eFacingDirection",facing);add("m_bEmbarked",0);
    return u;
}
int main(int argc,char **argv){
    using namespace graphics;
    require(argc==2,"Need shared asset root");
    auto opened=assets::FpkSource::open(std::filesystem::path(argv[1])/"Resource/Common/UnitModels.fpk","Resource/Common/UnitModels.fpk");
    auto pack=std::get<std::shared_ptr<assets::FpkSource>>(opened);
    const auto ini=std::find_if(pack->entries().begin(),pack->entries().end(),[](const auto &e){return e.normalized_name=="unitvisconfig.ini";});
    require(ini!=pack->entries().end()&&ini->payload_offset==410240&&ini->payload_length==330,"Shared runtime FPK directory differs");
    auto found=pack->find(assets::AssetKey("unitvisconfig.ini"));
    auto cfg=read_bnw_unit_visual_configuration(std::get<assets::AssetHandle>(found));
    require(cfg.config_sha256=="81baff60f45644a49244dec9ddc997f5a58bacdc17d5a4e1616b4817345e57a6" &&
        cfg.scene_spacing==1 && cfg.member_multiplier==1 && cfg.placement_seed==-1 && cfg.threaded,"Actual shared config differs");
    saves::SavedSessionSnapshot source;source.all_players.emplace();source.all_players->layout=saves::GameStateLayout::expansion2_version1;
    source.all_players->context.player_count=2;source.all_players->players.resize(2);
    for(std::size_t i=0;i<2;++i){auto &p=source.all_players->players[i];p.index=i;p.saved_player_id=static_cast<std::int64_t>(i);}
    source.all_players->players[0].units.units={unit(0,0,16384,0),unit(1,0,16385,1)};
    source.all_players->players[1].units.units={unit(0,1,16385,2)};
    BnwImportedUnitVisualOwner roster(source,cfg,1);
    require(roster.find(0,16385).pool_index==1&&roster.find(1,16385).pool_index==2,"Fresh full roster indices differ");
    const auto expected=bnw_warrior_authored_formation(1,1),formed=roster.warrior_formation(0,16385);
    require(expected.final_graphics_rng_state==formed.final_graphics_rng_state && expected.members[0].x==formed.members[0].x,"Owner ignored pool/config inputs");
    require(roster.matches(source),"Same source roster rejected");
    auto moved=source;moved.all_players->players[0].units.units[1].core.fields[3].values={33};
    require(!roster.matches(moved),"Unit movement accepted as same revision");
    auto fields=source;fields.all_players->players[0].units.units[1].core.fields[5].values={2};
    require(!roster.matches(fields),"Facing change accepted as same revision");
    int rejected=0;auto reject=[&](auto fn){try{fn();}catch(const std::invalid_argument &){++rejected;}};
    auto incomplete=source;incomplete.all_players->players.pop_back();reject([&]{BnwImportedUnitVisualOwner bad(incomplete,cfg,1);});
    auto duplicate=source;duplicate.all_players->players[0].units.units[1].core.fields[1].values={16384};reject([&]{BnwImportedUnitVisualOwner bad(duplicate,cfg,1);});
    auto absent=source;absent.all_players->players[0].units.units[0].core.fields.pop_back();reject([&]{BnwImportedUnitVisualOwner bad(absent,cfg,1);});
    auto unknown=cfg;unknown.config_sha256.clear();reject([&]{BnwImportedUnitVisualOwner bad(source,unknown,1);});
    auto embarked=source;embarked.all_players->players[0].units.units[1].core.fields.back().values={1};BnwImportedUnitVisualOwner boat(embarked,cfg,1);reject([&]{boat.warrior_formation(0,16385);});
    reject([&]{roster.find(0,999);});
    require(rejected==6,"Bad source/config input accepted");
    // Explicit small terrain fixture, not a native placement receipt. Contrast
    // nearest lookup with interpolation and verify original signed wrap policy.
    auto height=terrain::initialize_height_owner(terrain::HeightOwnerParameters::native_call(17,33),1,1,
        std::vector<std::uint8_t>(8,0),{0,0,0},terrain::LandStyleGrid(std::vector<std::uint8_t>(terrain::LandStyleGrid::size,0)),
        nullptr,nullptr,[](std::int32_t n){return std::vector<std::uint8_t>(n,0xa7);});
    std::vector<std::uint16_t> dense(32*48);
    for(std::size_t i=0;i<dense.size();++i)dense[i]=static_cast<std::uint16_t>(i+1);
    height.height=terrain::PageStorage::initialize(32,48,2,nullptr,[](std::int32_t n){return std::vector<std::uint8_t>(n,0);});
    height.height=height.height.with_dense_values(std::span<const std::uint16_t>(dense));
    const auto &hp=height.parameters;
    const auto wx=[&](float x){return x*hp.scale_x-hp.center_x;};
    const auto wy=[&](float y){return y*hp.scale_y-hp.center_y;};
    const auto decoded=[&](std::size_t x,std::size_t y){return (static_cast<float>(dense[y*32+x])*0.003921568859368563F)*hp.scale_z;};
    require(bnw_unit_world_height(height,wx(2.75F),wy(3.75F),false)==decoded(2,3),"Height query interpolated instead of truncating");
    require(bnw_unit_world_height(height,wx(-.75F),wy(3.75F),false)==decoded(0,3),"Negative fraction not truncated toward zero");
    require(bnw_unit_world_height(height,wx(18.75F),wy(3.75F),true)==decoded(1,3),"Height wrap used aligned page width rather than owner width");
    require(bnw_unit_world_height(height,wx(-1.25F),wy(3.75F),true)==decoded(16,3),"Signed native x wrap differs");
    require(bnw_unit_world_height(height,wx(-1701),wy(3),true)==0,"Negative signed remainder was rewritten to floor modulo");
    require(bnw_unit_world_height(height,wx(-2),wy(3),false)==0 && bnw_unit_world_height(height,wx(32),wy(3),false)==0,"Outside height grid did not return zero");
    reject([&]{bnw_unit_world_height(height,std::numeric_limits<float>::infinity(),0,true);});
    reject([&]{bnw_unit_world_height(height,2147483648.0F,0,true);});
    // Selected 40x24 world fixture uses the same pinned reciprocal estimate as
    // existing terrain tests. No original unit draw matrices are inferred.
    const auto world=terrain::build_world_layout(40,24,1,{},[](float){return std::bit_cast<float>(0x45800001u);});
    const auto world_hp=terrain::HeightOwnerParameters::from_world(world);
    const auto center=bnw_unit_world_center(world,world_hp,32,14);
    // Source mapping first two outputs are {25,14}; native then recomputes z.
    const auto projection=terrain::terrain_ice_projection(world,world_hp);
    const auto &m=projection.world_matrix;
    const float expected_x=((25*m[0]+(-39)*m[8])+14*m[4])+m[12];
    const float expected_y=((25*m[1]+(-39)*m[9])+14*m[5])+m[13];
    require(std::abs(center.x-(expected_x*world_hp.scale_x-world_hp.center_x))<.0002F &&
        std::abs(center.y-(expected_y*world_hp.scale_y-world_hp.center_y))<.0002F,"Source logical plot projection differs");
    require(std::bit_cast<std::uint32_t>(bnw_unit_initial_minimum_height())==0x4121de22u,"Native geometry constructor floor differs");
    reject([&]{bnw_unit_world_center(world,world_hp,40,14);});
    auto bad_hp=world_hp;bad_hp.width--;
    reject([&]{bnw_unit_world_center(world,bad_hp,32,14);});
    require(rejected==10,"Height/world bad input accepted");
    auto small_source=source;
    for(auto &player:small_source.all_players->players)for(auto &saved:player.units.units){saved.core.fields[3].values={1};saved.core.fields[4].values={1};}
    BnwImportedUnitVisualOwner scene_owner(small_source,cfg,1);
    const auto small_world=terrain::build_world_layout(2,2,1,{},[](float){return std::bit_cast<float>(0x45800001u);});
    const auto small_hp=terrain::HeightOwnerParameters::from_world(small_world);
    auto scene_height=terrain::initialize_height_owner(small_hp,2,2,std::vector<std::uint8_t>(32,0),{1,-2,1},
        terrain::LandStyleGrid(std::vector<std::uint8_t>(terrain::LandStyleGrid::size,0)),nullptr,nullptr,
        [](std::int32_t n){return std::vector<std::uint8_t>(n,0);});
    const auto floor_transforms=bnw_warrior_owned_scene_transforms(scene_owner,small_source,0,16385,small_world,scene_height,true,bnw_unit_initial_minimum_height(),.14F);
    for(const auto &transform:floor_transforms)require(transform[14]==bnw_unit_initial_minimum_height(),"Native constructor floor not applied to owned scene");
    std::vector<std::uint16_t> high(static_cast<std::size_t>(small_hp.width)*small_hp.height,30000);
    scene_height.height=scene_height.height.with_dense_values(std::span<const std::uint16_t>(high));
    const auto terrain_transforms=bnw_warrior_owned_scene_transforms(scene_owner,small_source,0,16385,small_world,scene_height,true,bnw_unit_initial_minimum_height(),.14F);
    const float elevated=(30000.F*0.003921568859368563F)*small_hp.scale_z;
    for(const auto &transform:terrain_transforms)require(transform[14]==elevated,"Owned scene ignored actual per-member terrain heights");
    require(scene_owner.matches(small_source),"Rendering mutated source roster");
    reject([&]{bnw_warrior_owned_scene_transforms(scene_owner,source,0,16385,small_world,scene_height,true,bnw_unit_initial_minimum_height(),.14F);});
    require(rejected==11,"Stale scene roster accepted");
    std::cout<<"Owned unit visual config/roster/world: actual shared FPK config; full roster pool indices; formation seed; nearest u16 height, native width wrap, source plot projection and constructor floor; 12 owned member transforms and 11 negatives PASS\n";
}
