#pragma once
#include "bnw_warrior_skin_diagnostic.hpp"
#include "civ5/graphics/bnw_warrior_native_vertex_consumer.hpp"
namespace civ5::graphics {
inline std::vector<std::uint8_t> warrior_decode_source_base64(std::string_view text) {
    if(text.empty()||text.size()%4)throw std::invalid_argument("Warrior raw source base64 extent");
    std::vector<std::uint8_t> out;out.reserve(text.size()/4*3);
    const auto digit=[](char c)->unsigned{if(c>='A'&&c<='Z')return unsigned(c-'A');if(c>='a'&&c<='z')return unsigned(c-'a'+26);if(c>='0'&&c<='9')return unsigned(c-'0'+52);if(c=='+')return 62;if(c=='/')return 63;throw std::invalid_argument("Warrior raw source base64 character");};
    for(std::size_t i=0;i<text.size();i+=4){const bool pad2=text[i+2]=='=',pad3=text[i+3]=='=';
        if((pad2&&!pad3)||((pad2||pad3)&&i+4!=text.size()))throw std::invalid_argument("Warrior raw source base64 padding");
        const unsigned a=digit(text[i]),b=digit(text[i+1]),c=pad2?0:digit(text[i+2]),d=pad3?0:digit(text[i+3]);
        if((pad2&&(b&15))||(!pad2&&pad3&&(c&3)))throw std::invalid_argument("Warrior raw source base64 residual bits");
        out.push_back(std::uint8_t((a<<2)|(b>>4)));if(!pad2)out.push_back(std::uint8_t((b<<4)|(c>>2)));if(!pad3)out.push_back(std::uint8_t((c<<6)|d));
    }return out;
}
inline BnwWarriorNativeFormat4Vertices load_bnw_warrior_native_vertex_cache(
    const std::filesystem::path& cache,const BnwWarriorArtMember& spec,
    const BnwWarriorSourceMesh& mesh,const BnwWarriorSkinDiagnostic& skin) {
    constexpr std::array<std::string_view,3> names{"warrior_v2.gr2","warrior.gr2","warrior_v4.gr2"};
    constexpr std::array<std::string_view,3> raw_shas{
        "d78b64da4eaadc285f2c8f8f27fbdb6b6f01e790d0262ab2bfc7e5f6a714c21d","56104434706e2921b8fe142de65b07e2aad67aa64008b95be80d532892eac9c0","e03a7afa5f2ff04e2103381aa7cb4475c92dcb87629b659cb3069f3ba513688d"};
    constexpr std::array<std::string_view,3> packed_shas{
        "46789d68a552482ce2a1cf29127a06f6ae2d1d57d2304ad79eadea7116dbb8d2","27d604401a5c34fe16664a7783932ffc531b568f7a8a453a854a03a3c6fbe6b6","580c2a88d6444ca2a8ec9da9bced92cc22396b5768dad4d32069ed325700e963"};
    const auto it=std::find(names.begin(),names.end(),spec.gr2_name);if(it==names.end())throw std::invalid_argument("Unknown native Warrior vertex source");
    const auto index=std::size_t(it-names.begin());const auto count=mesh.vertex_pnuv.size()/8;
    if(mesh.vertex_pnuv.size()!=count*8||skin.weights.size()!=count*4||skin.indices.size()!=count*4||skin.palette.size()!=20||!std::equal(skin.palette.begin(),skin.palette.end(),bnw_warrior_native_palette.begin()))
        throw std::invalid_argument("Warrior current mesh/native palette mismatch");
    require_bnw_warrior_asset_sha(mesh.gr2_sha256,spec.gr2_sha256);
    require_bnw_warrior_asset_sha(skin.flat_sha256,warrior_expected_skin_sha(spec.gr2_name));
    const auto json=warrior_read_bounded(cache/(std::filesystem::path(spec.gr2_name).stem().string()+".mesh.json"),2'000'000);
    require_bnw_warrior_asset_sha(warrior_json_string(json,"sourceGr2Sha256"),spec.gr2_sha256);
    require_bnw_warrior_asset_sha(warrior_json_string(json,"rawVertexBytesSha256"),raw_shas[index]);
    constexpr std::string_view tag="\"rawVertexBytesBase64\": \"";const auto start=json.find(tag);
    if(start==std::string::npos||json.find(tag,start+tag.size())!=std::string::npos)throw std::invalid_argument("Missing/duplicate Warrior source bytes");
    const auto begin=start+tag.size(),end=json.find('"',begin);if(end==std::string::npos)throw std::invalid_argument("Truncated Warrior source bytes");
    const auto raw=warrior_decode_source_base64(std::string_view(json).substr(begin,end-begin));
    require_bnw_warrior_asset_sha(runtime::identify_bytes(raw).sha256,raw_shas[index]);
    if(raw.size()!=count*68)throw std::invalid_argument("Warrior source raw cardinality");
    for(std::size_t v=0;v<count;++v)for(unsigned j=0;j<4;++j)
        if(raw[v*68+12+j]!=skin.weights[v*4+j]||raw[v*68+16+j]!=skin.indices[v*4+j])throw std::invalid_argument("Warrior raw stream/retained skin differs");
    auto result=convert_bnw_warrior_native_format4_vertices(raw,count,68,bnw_warrior_sdk_vertex_declaration);
    if(result.weights.zero_divisors)throw std::invalid_argument("Native Warrior zero-divisor cannot publish three-input resource");
    validate_bnw_warrior_native_format4(result.bytes,count);
    require_bnw_warrior_asset_sha(runtime::identify_bytes(result.bytes).sha256,packed_shas[index]);return result;
}
inline BnwWarriorNativePaletteRows bnw_warrior_sdk_world_palette_rows(
    const BnwWarriorSkinDiagnostic& skin,int sample,const std::array<float,16>& world) {
    if(skin.palette.size()!=20||!std::equal(skin.palette.begin(),skin.palette.end(),bnw_warrior_native_palette.begin()))throw std::invalid_argument("Warrior source palette mismatch");
    const auto pose=skin.matrices(sample);BnwWarriorNativePaletteRows rows{};
    for(unsigned i=0;i<20;++i)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){
        float value=0;for(unsigned k=0;k<4;++k)value+=world[k*4+r]*float(pose[std::size_t(skin.palette[i])][k*4+c]);
        if(!std::isfinite(value))throw std::invalid_argument("Warrior SDK world palette nonfinite");rows[i*12+r*4+c]=value;
    }
    return rows;
}
} // namespace civ5::graphics
