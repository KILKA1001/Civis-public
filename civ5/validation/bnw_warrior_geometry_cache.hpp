#pragma once

#include "bnw_warrior_art_probe.hpp"
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace civ5::graphics {

// Local research cache exported by the installed Civ V SDK bridge. Its source
// GR2 and output hashes must both match before any GL buffer is populated.
// This is geometry/UV only: packed skin fields, bind pose and animation are
// not inferred from the SDK's anonymous vertex declaration.
struct BnwWarriorSourceMesh {
    std::vector<float> vertex_pnuv;
    std::vector<std::uint32_t> indices;
    std::string gr2_sha256, geometry_sha256;
};

inline const char *bnw_warrior_expected_geometry_sha(std::string_view gr2_name) {
    if(gr2_name=="warrior_v2.gr2")
        return "f9fc04fdd11680a32122578c1a111b60c65f25e3119b539ba1524f30ddee5fc1";
    if(gr2_name=="warrior.gr2")
        return "d2e263f457b3a7a64f212679d2c3d5139d3d8b9aa931fca2b6426ff025cf020b";
    if(gr2_name=="warrior_v4.gr2")
        return "fc6599697c4310f65751da546f98c1a6d193f7d8dfb1d8f68afba034e268f110";
    throw std::runtime_error("Unknown Warrior SDK geometry cache identity");
}

inline std::string warrior_read_bounded(const std::filesystem::path &path,
                                       std::uintmax_t maximum) {
    const auto size=std::filesystem::file_size(path);
    if(size==0 || size>maximum) throw std::runtime_error("Warrior geometry cache size differs");
    std::ifstream stream(path,std::ios::binary);
    if(!stream) throw std::runtime_error("Warrior geometry cache unavailable");
    std::string bytes(static_cast<std::size_t>(size),'\0');
    if(!stream.read(bytes.data(),static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Warrior geometry cache truncated");
    return bytes;
}

inline std::string warrior_json_string(std::string_view text,std::string_view key) {
    const std::string tag="\""+std::string(key)+"\": \"";
    const auto start=text.find(tag);
    if(start==std::string_view::npos)
        throw std::runtime_error("Warrior SDK sidecar identity missing");
    const auto value=start+tag.size(), end=text.find('"',value);
    if(end==std::string_view::npos || end-value>128)
        throw std::runtime_error("Warrior SDK sidecar identity invalid");
    return std::string(text.substr(value,end-value));
}

inline std::string_view warrior_json_array(std::string_view text,
                                          std::string_view name) {
    const std::string tag="\""+std::string(name)+"\": [";
    const auto first=text.find(tag);
    if(first==std::string_view::npos)
        throw std::runtime_error("Warrior SDK geometry array missing");
    const auto begin=first+tag.size(), end=text.find(']',begin);
    if(end==std::string_view::npos || end-begin>1'000'000)
        throw std::runtime_error("Warrior SDK geometry array extent invalid");
    return text.substr(begin,end-begin);
}

template<class T> inline std::vector<T> warrior_json_numbers(std::string_view text,
                                                              std::size_t expected) {
    std::vector<T> result;
    result.reserve(expected);
    const char *at=text.data(), *end=at+text.size();
    while(at<end) {
        while(at<end && (*at==' ' || *at=='\r' || *at=='\n')) ++at;
        if(at==end) break;
        T value{};
        const auto parsed=std::from_chars(at,end,value);
        if(parsed.ec!=std::errc{} || parsed.ptr==at || result.size()>=expected)
            throw std::runtime_error("Warrior SDK geometry number invalid");
        if constexpr(std::is_floating_point_v<T>)
            if(!std::isfinite(value))
                throw std::runtime_error("Warrior SDK geometry is nonfinite");
        result.push_back(value);
        at=parsed.ptr;
        while(at<end && (*at==' ' || *at=='\r' || *at=='\n')) ++at;
        if(at<end && *at++!=',')
            throw std::runtime_error("Warrior SDK geometry delimiter invalid");
    }
    if(result.size()!=expected)
        throw std::runtime_error("Warrior SDK geometry count differs");
    return result;
}

inline BnwWarriorSourceMesh load_bnw_warrior_source_mesh(
    const std::filesystem::path &cache_directory,
    const BnwWarriorArtMember &spec, std::size_t expected_vertices,
    std::size_t expected_triangles) {
    const std::string stem=std::filesystem::path(spec.gr2_name).stem().string();
    const auto gr2=cache_directory / spec.gr2_name;
    const auto geometry=cache_directory / (stem+".g3dj");
    const auto sidecar=cache_directory / (stem+".mesh.json");
    const auto actual_gr2=runtime::identify_file(gr2).sha256;
    require_bnw_warrior_asset_sha(actual_gr2,spec.gr2_sha256);
    const auto sidecar_text=warrior_read_bounded(sidecar,2'000'000);
    const auto sidecar_gr2=warrior_json_string(sidecar_text,"sourceGr2Sha256");
    const auto sidecar_geometry=warrior_json_string(sidecar_text,"geometryCacheSha256");
    require_bnw_warrior_asset_sha(sidecar_gr2,actual_gr2);
    const auto actual_geometry=runtime::identify_file(geometry).sha256;
    require_bnw_warrior_asset_sha(sidecar_geometry,actual_geometry);
    require_bnw_warrior_asset_sha(actual_geometry,
                                  bnw_warrior_expected_geometry_sha(spec.gr2_name));
    const auto json=warrior_read_bounded(geometry,2'000'000);
    if(json.find("\"attributes\": [\"POSITION\", \"NORMAL\", \"TEXCOORD0\"]")==std::string::npos ||
       json.find("\"type\": \"TRIANGLES\"")==std::string::npos)
        throw std::runtime_error("Warrior SDK geometry declaration differs");
    BnwWarriorSourceMesh mesh;
    mesh.gr2_sha256=actual_gr2;
    mesh.geometry_sha256=actual_geometry;
    mesh.vertex_pnuv=warrior_json_numbers<float>(warrior_json_array(json,"vertices"),
                                                  expected_vertices*8);
    mesh.indices=warrior_json_numbers<std::uint32_t>(warrior_json_array(json,"indices"),
                                                    expected_triangles*3);
    for(const auto index:mesh.indices)
        if(index>=expected_vertices)
            throw std::runtime_error("Warrior SDK geometry index outside vertex array");
    return mesh;
}

} // namespace civ5::graphics
