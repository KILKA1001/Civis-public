#include "civ5/terrain/coast_road_network.hpp"
#include "../validation/coast_road_network_reference.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks=0, compared_bytes=0, snapshots=0;
void check(bool condition,const char *message) { ++checks; if (!condition) throw std::runtime_error(message); }
struct Reader {
    Bytes bytes; std::size_t at=0;
    explicit Reader(const char *path) {
        std::ifstream in(path,std::ios::binary); check(bool(in),"Cannot open coast road oracle");
        bytes.assign(std::istreambuf_iterator<char>(in),{});
        check(bytes.size()>=12 && std::string_view(reinterpret_cast<const char *>(bytes.data()),8)=="C5RDN001",
              "Invalid coast road oracle"); at=8;
    }
    std::uint32_t word() {
        check(at+4<=bytes.size(),"Truncated coast road word"); std::uint32_t v=0;
        for (unsigned i=0;i<4;++i) v|=std::uint32_t(bytes[at++])<<(8*i);
        return v;
    }
    float real() { return std::bit_cast<float>(word()); }
    Bytes blob() {
        const auto size=word(); check(size<=bytes.size()-at,"Truncated coast road blob");
        Bytes result(bytes.begin()+at,bytes.begin()+at+size); at+=size; return result;
    }
};
using coast_road_reference::snapshot;
void oracle(const char *path) {
    Reader r(path); const auto cases=r.word(); check(cases<=1000,"Excessive road cases");
    for (unsigned ci=0;ci<cases;++ci) {
        unsigned path_index=0;
        try {
            const auto width=r.word(),height=r.word(),pc=r.word();
            check(pc==24 || pc==53,"Unsupported reference precision");
            const auto precision=pc==24?X87Precision::single:X87Precision::double_precision;
            std::array<float,4> bounds; for (auto &f:bounds) f=r.real();
            auto grid=make_terrain_road_grid(static_cast<std::int32_t>(width),static_cast<std::int32_t>(height),bounds);
            const auto count=r.word(); check(count<=10000,"Excessive paths");
            std::vector<TerrainDecalPolyline> paths;
            std::vector<bool> closed;
            for (unsigned i=0;i<count;++i) {
                closed.push_back(r.word()!=0); const auto size=r.word(); check(size<=1000000,"Excessive path points");
                auto &points=paths.emplace_back();
                for (unsigned j=0;j<size;++j) { const auto x=r.real(),y=r.real(); points.push_back({x,y}); }
            }
            for (;path_index<count;++path_index) {
                insert_coast_road_path(grid,paths[path_index],closed[path_index],path_index+1,precision);
                const auto expected=r.blob(),actual=snapshot(grid); compared_bytes+=expected.size(); ++snapshots;
                if (expected!=actual) {
                    std::size_t at=0; while (at<std::min(expected.size(),actual.size()) && expected[at]==actual[at]) ++at;
                    throw std::runtime_error("Network snapshot mismatch byte "+std::to_string(at)+
                        "; expected bytes "+std::to_string(expected.size())+" actual "+std::to_string(actual.size())+
                        "; segments "+std::to_string(grid.segments.size())+" nodes "+std::to_string(grid.junctions.size()));
                }
                check(true,"Original network snapshot");
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Case "+std::to_string(ci)+" path "+std::to_string(path_index)+": "+e.what());
        }
    }
    check(r.at==r.bytes.size(),"Trailing coast road reference bytes");
    std::cout<<cases<<" original cases, ";
}
void guards() {
    auto grid=make_terrain_road_grid(4,3,{-32,-24,32,24});
    const std::vector<Point2> short_path{{0,0},{1,1},{2,2}};
    bool caught=false;
    try { insert_coast_road_path(grid,short_path,false,1,X87Precision::double_precision); }
    catch (const std::invalid_argument &) { caught=true; }
    check(caught && grid.segments.empty() && grid.junctions.empty(),"Short native open path was accepted/mutated state");
}
} // namespace
int main(int argc,char **argv) {
    try {
        if (argc>2) throw std::runtime_error("Usage: civ5_coast_road_network_tests [oracle.bin]");
        guards(); if (argc==2) oracle(argv[1]);
        std::cout<<"PASS coast road network: "<<checks<<" checks, "<<snapshots<<" snapshots, "<<compared_bytes<<" original bytes\n";
    } catch (const std::exception &e) { std::cerr<<"FAIL coast road network: "<<e.what()<<'\n'; return 1; }
}
