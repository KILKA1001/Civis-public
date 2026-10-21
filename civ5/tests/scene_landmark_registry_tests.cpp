#include "civ5/content/landmark_art_registry.hpp"
#include "civ5/assets/asset_source.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

using namespace civ5;
using namespace civ5::content;
namespace {
using Bytes=std::vector<std::uint8_t>;
void check(bool okay,const char *message){if(!okay)throw std::runtime_error(message);}
struct Reader {
    std::span<const std::uint8_t> data;
    std::uint32_t word(){
        check(data.size()>=4,"Short landmark oracle word");
        std::uint32_t value=0;
        for(unsigned byte=0;byte<4;++byte)value|=std::uint32_t(data[byte])<<(8*byte);
        data=data.subspan(4);return value;
    }
    std::span<const std::uint8_t> blob(){
        const auto length=word();check(length<=data.size(),"Short landmark oracle blob");
        const auto result=data.first(length);data=data.subspan(length);return result;
    }
};
std::array<std::uint32_t,20> words(std::span<const std::uint8_t> bytes){
    check(bytes.size()==80,"Wrong landmark material table extent");
    Reader reader{bytes};std::array<std::uint32_t,20> result{};
    for(auto &value:result)value=reader.word();
    return result;
}
std::array<std::uint32_t,21> special_words(std::span<const std::uint8_t> bytes){
    check(bytes.size()==84,"Wrong special landmark material table extent");
    Reader reader{bytes};std::array<std::uint32_t,21> result{};
    for(auto &value:result)value=reader.word();
    return result;
}
void smoke(){
    constexpr std::string_view xml=R"(<LandmarkArtInfos>
      <LandmarkArtInfo><ImprovementType>ART_DEF_IMPROVEMENT_FARM</ImprovementType>
      <ResourceType>ART_DEF_RESOURCE_NONE</ResourceType><Era>Ancient</Era><State>Any</State></LandmarkArtInfo>
      <LandmarkArtInfo><ImprovementType>ART_DEF_IMPROVEMENT_NONE</ImprovementType>
      <ResourceType>ART_DEF_RESOURCE_WHEAT</ResourceType><Era>Any</Era><State>Constructed</State></LandmarkArtInfo>
      </LandmarkArtInfos>)";
    const auto bytes=std::span(reinterpret_cast<const std::uint8_t *>(xml.data()),xml.size());
    const auto registry=build_landmark_art_registry(read_landmark_art_rows(bytes));
    check(registry.art_types.size()==2 && registry.materials.size()==2,
          "Synthetic landmark import size");
    check(registry.id("ART_DEF_IMPROVEMENT_FARM")==0 &&
          registry.id("ART_DEF_RESOURCE_WHEAT")==1 &&
          registry.id("MISSING")==0xffffffffu,"Synthetic landmark IDs");
    check(registry.materials[0].improvement==0 &&
          registry.materials[0].resource==0xffffffffu &&
          registry.materials[0].era==1 && registry.materials[0].state==1,
          "Synthetic farm record");
    constexpr std::string_view types_xml=R"(<LandmarkTypes><ImprovementTypes>
      <Type friendly="Farm">ART_DEF_IMPROVEMENT_FARM</Type>
      </ImprovementTypes></LandmarkTypes>)";
    const auto type_bytes=std::span(reinterpret_cast<const std::uint8_t *>(types_xml.data()),types_xml.size());
    check(read_landmark_type_rows(type_bytes)==std::vector<LandmarkTypeRow>{{
          "ART_DEF_IMPROVEMENT_FARM","Improvement","Farm"}},
          "Synthetic landmark type record");
}
void real_base(const char *archive,const char *oracle_path,
               int manifest_count,char **manifests){
    auto opened=assets::FpkSource::open(archive,"Resource/Common/MiscData.fpk");
    check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(opened),"Cannot open base FPK");
    const auto fpk=std::get<std::shared_ptr<assets::FpkSource>>(std::move(opened));
    auto lookup=fpk->find(assets::AssetKey("civ5artdefines_landmarks.xml"));
    check(std::holds_alternative<assets::AssetHandle>(lookup),"Missing base landmark XML");
    const auto base=std::get<assets::AssetHandle>(std::move(lookup));
    auto types_lookup=fpk->find(assets::AssetKey("civ5artdefines_landmarktypes.xml"));
    check(std::holds_alternative<assets::AssetHandle>(types_lookup),"Missing base landmark types XML");
    const auto base_types=std::get<assets::AssetHandle>(std::move(types_lookup));
    Catalog catalog;
    assets::ResourceMountTable resources(assets::resource_lookup(fpk),
        assets::resource_lookup_all(fpk));
    std::vector<std::size_t> selected;
    for(int i=0;i<manifest_count;++i){
        const auto manifest=std::filesystem::canonical(manifests[i]);
        catalog.packages.push_back(read_manifest(manifest));
        auto loose=std::make_shared<assets::LooseResourceIndex>(manifest.parent_path());
        loose->scan({manifest.parent_path(),{},true});
        resources.mount({static_cast<std::uint64_t>(i+1),0,0,
                         assets::resource_lookup(loose),true,true,
                         assets::resource_lookup_all(loose),assets::resource_keys(loose)});
        selected.push_back(static_cast<std::size_t>(i));
    }
    if(manifest_count){
        const auto inactive=load_landmark_art_registry(base,base_types,{},catalog,resources,
            {true,true,true});
        check(inactive.registry.materials.size()==320 && inactive.registry.art_types.size()==77,
              "Installed but inactive DLC changed base art IDs");
        check(inactive.type_rows.size()==90,
              "Installed but inactive DLC changed base landmark types");
    }
    const auto loaded=load_landmark_art_registry(base,base_types,selected,catalog,resources,
        {true,true,true});
    const auto &registry=loaded.registry;
    check(!loaded.documents.empty() &&
          loaded.documents[0].first_row==0 && loaded.documents[0].row_count==320,
          "Wrong active landmark document sequence");
    std::size_t next_row=320;
    for(std::size_t i=1;i<loaded.documents.size();++i){
        const auto &document=loaded.documents[i];
        check(manifest_count && document.first_row==next_row &&
              document.package && document.mount_id &&
              *document.mount_id==*document.package+1,
              "Wrong active DLC landmark source provenance");
        next_row+=document.row_count;
    }
    check(next_row==registry.materials.size() &&
          (manifest_count || loaded.type_documents==0),
          "Active landmark document totals changed");
    std::ifstream input(oracle_path,std::ios::binary);
    check(bool(input),"Cannot open base landmark oracle");
    Bytes data((std::istreambuf_iterator<char>(input)),{});
    check(data.size()>=8 &&
          std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5LAR001",
          "Wrong base landmark oracle");
    Reader expected{std::span(data).subspan(8)};
    const auto type_count=expected.word(),row_count=expected.word();
    check(registry.art_types.size()==type_count && registry.materials.size()==row_count,
          "Real base landmark cardinality");
    for(std::size_t i=0;i<type_count;++i){
        const auto name=expected.blob();
        check(registry.art_types[i]==std::string_view(
                  reinterpret_cast<const char *>(name.data()),name.size()),
              "Real base art type ordering");
    }
    for(const auto &source:registry.materials){
        check(source.improvement==expected.word() && source.resource==expected.word() &&
              source.era==expected.word() && source.state==expected.word(),
              "Real base landmark source record");
    }
    const auto farm=expected.word(),wheat=expected.word();
    check(registry.id("ART_DEF_IMPROVEMENT_FARM")==farm &&
          registry.id("ART_DEF_RESOURCE_WHEAT")==wheat,"Real base art IDs");
    auto table=words(expected.blob());
    const auto final=words(expected.blob());
    terrain::populate_scene_polygon_material_rows(table,farm,registry.materials);
    check(table==final,"Real base farm materials differ from retail 628920");
    const std::array<std::uint32_t,3> special_ids{
        expected.word(),expected.word(),expected.word()};
    auto special_table=special_words(expected.blob());
    const auto special_final=special_words(expected.blob());
    check(expected.data.empty(),"Trailing landmark oracle");
    check(registry.id("ART_DEF_IMPROVEMENT_MOAI_STATUES")==special_ids[0] &&
          registry.id("ART_DEF_IMPROVEMENT_PONTOON_BRIDGE")==special_ids[1] &&
          registry.id("ART_DEF_IMPROVEMENT_FEITORIA")==special_ids[2],
          "Real special-improvement art IDs");
    terrain::populate_scene_special_improvement_material_rows(
        special_table,special_ids,registry.materials);
    check(special_table==special_final,
          "Real special materials differ from retail 70D500");
    std::cout<<"Original landmark rows="<<row_count<<" types="<<type_count
             <<" farm="<<farm<<" wheat="<<wheat<<" moai="<<special_ids[0]
             <<" exact words="<<row_count*4+82<<'\n';
}
}
int main(int argc,char **argv){
    try{
        check(argc==1 || argc>=3,
              "Usage: civ5_scene_landmark_registry_tests [MiscData.fpk oracle.bin [DLC.Civ5Pkg...]]");
        smoke();if(argc>=3)real_base(argv[1],argv[2],argc-3,argv+3);
        std::cout<<"PASS landmark art registry\n";return 0;
    }catch(const std::exception &error){
        std::cerr<<"FAIL landmark art registry: "<<error.what()<<'\n';return 1;
    }
}
