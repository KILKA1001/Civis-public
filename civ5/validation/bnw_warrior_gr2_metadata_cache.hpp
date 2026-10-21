#pragma once
#include "bnw_warrior_geometry_cache.hpp"
#include "civ5/graphics/bnw_warrior_gr2_native_basis.hpp"
#include <cctype>
namespace civ5::graphics {
inline constexpr const char *bnw_warrior_arttool_metadata_sha256=
    "78ec5c7873c3cf59ec1d140dfca5990c3e0011076da434aeedb4a6bea56f49ac";
struct BnwWarriorGr2MetadataCache {
    std::array<BnwWarriorGr2ArtToolMetadata,3> members;
    std::string metadata_sha256;
    float native_model_scale(float compiled_scale) const {
        const auto first=compile_bnw_warrior_gr2_native_basis(members[0],compiled_scale).uniform_scale;
        for(std::size_t i=1;i<3;++i)
            if(compile_bnw_warrior_gr2_native_basis(members[i],compiled_scale).uniform_scale!=first)
                throw std::runtime_error("Warrior model basis scales disagree");
        return first;
    }
};
namespace warrior_metadata_detail {
inline std::size_t value(std::string_view text,std::string_view key) {
    const auto tag='"'+std::string(key)+'"';const auto begin=text.find(tag);
    if(begin==std::string_view::npos || text.find(tag,begin+tag.size())!=std::string_view::npos)
        throw std::runtime_error("Warrior metadata field missing/duplicate");
    auto at=begin+tag.size();while(at<text.size()&&std::isspace(static_cast<unsigned char>(text[at])))++at;
    if(at==text.size()||text[at++]!=':')throw std::runtime_error("Warrior metadata separator invalid");
    while(at<text.size()&&std::isspace(static_cast<unsigned char>(text[at])))++at;
    if(at==text.size())throw std::runtime_error("Warrior metadata value missing");return at;
}
inline std::string string(std::string_view text,std::string_view key) {
    const auto at=value(text,key);if(text[at]!='"')throw std::runtime_error("Warrior metadata string missing");
    const auto end=text.find('"',at+1);if(end==std::string_view::npos||end-at>512)throw std::runtime_error("Warrior metadata string extent invalid");
    return std::string(text.substr(at+1,end-at-1));
}
inline std::array<float,3> vector(std::string_view text,std::string_view key) {
    const auto at=value(text,key);if(text[at]!='[')throw std::runtime_error("Warrior metadata vector missing");
    const auto end=text.find(']',at+1);if(end==std::string_view::npos||end-at>128)throw std::runtime_error("Warrior metadata vector invalid");
    const auto v=warrior_json_numbers<float>(text.substr(at+1,end-at-1),3);
    return {v[0],v[1],v[2]};
}
inline float scalar(std::string_view text,std::string_view key) {
    const auto at=value(text,key);float v=0;const auto parse=std::from_chars(text.data()+at,text.data()+text.size(),v);
    if(parse.ec!=std::errc{}||parse.ptr==text.data()+at||!std::isfinite(v))throw std::runtime_error("Warrior metadata scalar invalid");return v;
}
} // namespace warrior_metadata_detail
inline BnwWarriorGr2MetadataCache load_bnw_warrior_gr2_metadata_cache(
    const std::filesystem::path &cache,const std::filesystem::path &metadata_path={}) {
    const auto path=metadata_path.empty()?cache/"warrior-arttool-metadata.json":metadata_path;
    const auto bytes=warrior_read_bounded(path,20'000);
    const auto sha=runtime::identify_bytes(assets::Bytes(bytes.begin(),bytes.end())).sha256;
    if(sha!=bnw_warrior_arttool_metadata_sha256)throw std::runtime_error("Warrior actual GR2 metadata identity differs");
    using namespace warrior_metadata_detail;
    // This metadata receipt is pinned as a whole. Fields are still validated
    // against each actual source/cache; no legacy normalization marker is read.
    const auto models=bytes.find("\"models\"");if(models==std::string::npos)throw std::runtime_error("Warrior metadata models missing");
    const auto prefix=std::string_view(bytes).substr(0,models);
    if(string(prefix,"schema")!="CivisWarriorArtToolMetadataV1" || string(prefix,"sdkMethod")!="LoadGrannyFile")
        throw std::runtime_error("Warrior metadata producer differs");
    const auto expected_root=std::filesystem::path("C:/CrazyProject/civ5-assets");
    // SDK JSON escapes Windows backslashes; normalize only the pinned field.
    auto root=string(prefix,"assetRoot");for(std::size_t p=0;(p=root.find("\\\\",p))!=std::string::npos;)root.replace(p,2,"/");
    if(!std::filesystem::equivalent(root,expected_root))throw std::runtime_error("Warrior metadata AssetRoot differs");
    BnwWarriorGr2MetadataCache result;result.metadata_sha256=sha;
    std::size_t begin=bytes.find("\"source\"",models);
    for(std::size_t i=0;i<3;++i) {
        if(begin==std::string::npos)throw std::runtime_error("Warrior metadata source roster incomplete");
        const auto end=bytes.find("\"source\"",begin+8);
        if(i==2&&end!=std::string::npos)throw std::runtime_error("Warrior metadata source roster duplicated");
        const auto row=std::string_view(bytes).substr(begin,end==std::string::npos?end:end-begin);
        const auto &spec=bnw_gandhi_warrior_art[i];const auto stem=std::filesystem::path(spec.gr2_name).stem().string();
        if(string(row,"source")!=stem||string(row,"tool")!="3D Studio MAX"||string(row,"sourceGr2Sha256")!=spec.gr2_sha256)
            throw std::runtime_error("Warrior metadata member/source identity differs");
        require_bnw_warrior_asset_sha(runtime::identify_file(cache/spec.gr2_name).sha256,spec.gr2_sha256);
        const auto cached=warrior_read_bounded(cache/(stem+".mesh.json"),1'000'000);
        if(string(row,"rawVertexBytesSha256")!=warrior_json_string(cached,"rawVertexBytesSha256")||
           warrior_json_string(cached,"sourceGr2Sha256")!=spec.gr2_sha256)
            throw std::runtime_error("Warrior metadata/direct SDK cache vertex identity differs");
        result.members[i]={scalar(row,"unitsPerMeter"),vector(row,"origin"),vector(row,"right"),vector(row,"up"),vector(row,"back")};
        (void)compile_bnw_warrior_gr2_native_basis(result.members[i],1);
        begin=end;
    }
    return result;
}
} // namespace civ5::graphics
