#include "civ5/content/landmark_art_registry.hpp"
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::content {
namespace {
constexpr std::string_view none_improvement = "ART_DEF_IMPROVEMENT_NONE";
constexpr std::string_view none_resource = "ART_DEF_RESOURCE_NONE";
constexpr std::string_view all_resource = "ART_DEF_RESOURCE_ALL";
struct SqliteBinaryLess {
    bool operator()(std::string_view a, std::string_view b) const {
        return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end(),
            [](unsigned char x,unsigned char y){return x<y;});
    }
};
std::string field(const XmlNode &row,const char *name){
    const auto *node=row.child(name);
    if(!node || node->text.empty())
        throw std::invalid_argument(std::string("Missing landmark art field: ")+name);
    return node->text;
}
std::string optional_field(const XmlNode &row,const char *name){
    // Retail's SQL INSERT keeps the first value when a source row repeats a
    // column. Base row 216 repeats bTerrainContour with different values.
    for(const auto &node:row.children)
        if(node.name==name)return node.text;
    return {};
}
std::uint32_t era_code(std::string_view era){
    if(era=="Ancient")return 1;
    if(era=="Medieval")return 2;
    if(era=="Industrial")return 3;
    if(era=="Modern")return 4;
    return 0;
}
std::uint32_t state_code(std::string_view state){
    if(state=="UnderConstruction")return 2;
    if(state=="Constructed")return 4;
    if(state=="Pillaged")return 8;
    return 1;
}
std::vector<LandmarkArtRow> rows_from_document(const XmlNode &document){
    if(document.name!="LandmarkArtInfos")
        throw std::invalid_argument("Expected LandmarkArtInfos root");
    std::vector<LandmarkArtRow> rows;
    rows.reserve(document.children.size());
    for(const auto &child:document.children){
        if(child.name!="LandmarkArtInfo")
            throw std::invalid_argument("Unexpected LandmarkArtInfos child");
        rows.push_back({field(child,"ImprovementType"),field(child,"ResourceType"),
                        field(child,"Era"),field(child,"State"),
                        optional_field(child,"fScale"),optional_field(child,"FXSXML"),
                        optional_field(child,"LayoutHandler"),
                        optional_field(child,"bTerrainContour"),optional_field(child,"Tech")});
    }
    return rows;
}
std::vector<LandmarkTypeRow> type_rows_from_document(const XmlNode &document){
    if(document.name!="LandmarkTypes")
        throw std::invalid_argument("Expected LandmarkTypes root");
    std::vector<LandmarkTypeRow> rows;
    for(const auto &group:document.children){
        if(!group.name.ends_with("Types") || group.name.size()<=5)
            throw std::invalid_argument("Unexpected LandmarkTypes group");
        const auto landmark_type=group.name.substr(0,group.name.size()-5);
        for(const auto &child:group.children){
            if(child.name!="Type" || child.text.empty() || child.attribute("friendly").empty())
                throw std::invalid_argument("Incomplete LandmarkTypes row");
            rows.push_back({child.text,landmark_type,child.attribute("friendly")});
        }
    }
    return rows;
}
XmlNode read_document(std::span<const std::uint8_t> xml){
    return read_xml_metadata({reinterpret_cast<const char *>(xml.data()),xml.size()});
}
}

std::vector<LandmarkArtRow> read_landmark_art_rows(std::span<const std::uint8_t> xml){
    return rows_from_document(read_document(xml));
}
std::vector<LandmarkTypeRow> read_landmark_type_rows(std::span<const std::uint8_t> xml){
    return type_rows_from_document(read_document(xml));
}

std::uint32_t LandmarkArtRegistry::id(std::string_view name) const {
    const auto at=std::lower_bound(art_types.begin(),art_types.end(),name,SqliteBinaryLess{});
    if(at==art_types.end() || *at!=name)return 0xffffffffu;
    return static_cast<std::uint32_t>(at-art_types.begin());
}

LandmarkArtRegistry build_landmark_art_registry(std::span<const LandmarkArtRow> rows){
    if(rows.size()>std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Landmark art registry exceeds U32 indices");
    LandmarkArtRegistry result;
    for(const auto &row:rows){
        if(row.improvement!=none_improvement)result.art_types.push_back(row.improvement);
        else if(row.resource!=none_resource)result.art_types.push_back(row.resource);
    }
    std::sort(result.art_types.begin(),result.art_types.end(),SqliteBinaryLess{});
    result.art_types.erase(std::unique(result.art_types.begin(),result.art_types.end()),
                           result.art_types.end());
    result.materials.reserve(rows.size());
    for(const auto &row:rows){
        terrain::ScenePolygonMaterialSource source{};
        if(row.improvement!=none_improvement){
            source.improvement=result.id(row.improvement);
            if(row.resource==none_resource)source.resource=0xffffffffu;
            else if(row.resource==all_resource)source.resource=0xfffffffeu;
            else source.resource=result.id(row.resource);
        }
        source.era=era_code(row.era);
        source.state=state_code(row.state);
        result.materials.push_back(source);
    }
    return result;
}

LandmarkArtLoadResult load_landmark_art_registry(
    const assets::AssetHandle &base,const assets::AssetHandle &base_types,
    std::span<const std::size_t> selected_dlc,
    const Catalog &catalog,const assets::ResourceMountTable &resources,
    std::array<bool,3> enabled_categories,std::uint64_t maximum_source_bytes){
    LandmarkArtLoadResult result;
    std::vector<LandmarkArtRow> rows;
    result.type_rows=read_landmark_type_rows(base_types.read_bytes(maximum_source_bytes));
    const auto append=[&](const assets::AssetHandle &handle,const std::string &request,
                          std::optional<std::size_t> package,std::optional<std::uint64_t> mount_id){
        const auto bytes=handle.read_bytes(maximum_source_bytes);
        const auto document=read_document(bytes);
        if(document.name=="LandmarkTypes"){
            if(!package)
                throw std::invalid_argument("Base landmark art source must contain LandmarkArtInfos");
            const auto values=type_rows_from_document(document);
            result.type_rows.insert(result.type_rows.end(),values.begin(),values.end());
            ++result.type_documents;
            return;
        }
        const auto values=rows_from_document(document);
        result.documents.push_back({request,package,mount_id,rows.size(),values.size()});
        rows.insert(rows.end(),values.begin(),values.end());
    };
    append(base,base.requested_key().original_name(),{},{});
    for(const auto index:selected_dlc){
        const auto &package=catalog.packages.at(index);
        PackageResourceQuery query;
        query.selector="LandmarkDefines";
        query.enabled_categories=enabled_categories;
        for(const auto &declaration:package_resources(package,query)){
            if(declaration.value.empty())continue; // Authored empty declaration.
            auto mounted=resources.resolve(assets::AssetKey(declaration.value));
            const auto *handle=std::get_if<assets::AssetHandle>(&mounted.asset);
            if(!handle)
                throw std::runtime_error("Missing or ambiguous active LandmarkDefines: "+
                                         declaration.value);
            append(*handle,declaration.value,index,mounted.mount_id);
        }
    }
    result.registry=build_landmark_art_registry(rows);
    result.rows=std::move(rows);
    return result;
}
} // namespace civ5::content
