#include "civ5/terrain/scene_mask.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes=civ5::assets::Bytes;
namespace {
std::size_t checks=0,compared=0,snapshots=0,writes=0;
void check(bool ok,const char *message) { ++checks;if(!ok)throw std::runtime_error(message); }
void word(Bytes &b,std::uint32_t value,unsigned n=4) { for(unsigned i=0;i<n;++i)b.push_back(std::uint8_t(value>>(8*i))); }
void blob(Bytes &b,std::span<const std::uint8_t> value) { word(b,static_cast<std::uint32_t>(value.size()));b.insert(b.end(),value.begin(),value.end()); }
Bytes coordinates(std::span<const Point2> points) {
    Bytes b;for(auto p:points)for(auto f:{p.x,p.y})word(b,std::bit_cast<std::uint32_t>(f));return b;
}
Bytes indices(std::span<const std::uint16_t> values) {Bytes b;for(auto i:values)word(b,i,2);return b;}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path):in(path,std::ios::binary) {
        check(bool(in),"Cannot open polygon reference");char magic[8]{};
        check(bool(in.read(magic,8)) && std::string_view(magic,8)=="C5PMS002","Invalid polygon reference");
    }
    std::uint32_t word() {
        std::uint32_t value=0;for(unsigned i=0;i<4;++i){const auto c=in.get();check(c!=EOF,"Truncated reference");value|=std::uint32_t(c)<<(8*i);}return value;
    }
    float real(){return std::bit_cast<float>(word());}
    Bytes blob(){const auto n=word();check(n<=4*1024*1024,"Excessive reference blob");Bytes b(n);if(n)check(bool(in.read(reinterpret_cast<char *>(b.data()),n)),"Truncated blob");return b;}
};
void equal(const Bytes &actual,const Bytes &expected,const char *message) {
    compared+=expected.size();++snapshots;
    if(actual!=expected) {
        std::size_t at=0;while(at<std::min(actual.size(),expected.size()) && actual[at]==expected[at])++at;
        throw std::runtime_error(std::string(message)+" at byte "+std::to_string(at)+" expected size "+
            std::to_string(expected.size())+" actual size "+std::to_string(actual.size()));
    }
    check(true,message);
}
void oracle(const char *path) {
    Reader r(path);const auto count=r.word();check(count<=10000,"Excessive polygon cases");
    for(unsigned ci=0;ci<count;++ci) {
        unsigned operation=0;
        try {
            const auto width=r.word(),height=r.word(),pc=r.word(),chunk=r.word();
            check((pc==24 || pc==53) && chunk>=1 && chunk<=30,"Invalid polygon precision/chunk size");
            const auto precision=pc==24?CoastBoundsPrecision::single:CoastBoundsPrecision::double_or_extended;
            const auto x=r.real(),y=r.real();const Point2 offset{x,y};
            const auto n=r.word();check(n<=512,"Excessive polygon points");
            std::vector<Point2> points;for(unsigned i=0;i<n;++i){const auto px=r.real(),py=r.real();points.emplace_back(px,py);}
            SceneMask mask(width,height);Bytes commands;word(commands,0x1d);commands.resize(24);
            const std::array<Point2,4> background{{{0,0},{128,0},{128,128},{0,128}}};
            const std::array<std::uint16_t,6> background_indices{{0,1,2,0,2,3}};
            const auto initial=mask.indexed(background,background_indices,2,true,precision);
            word(commands,0x20);for(auto f:initial.bounds)word(commands,std::bit_cast<std::uint32_t>(f));word(commands,initial.bits);
            const auto operations=r.word();check(operations<=1000,"Excessive polygon operations");
            for(;operation<operations;++operation) {
                const bool setting=r.word()!=0;const auto estimate=r.word();unsigned estimate_calls=0;
                const auto reciprocal=[&](float value){check(value==.866f,"Wrong RCPSS input");++estimate_calls;return std::bit_cast<float>(estimate);};
                const auto mesh=build_scene_mask_polygon(points,offset,precision,reciprocal);
                equal(mesh?coordinates(std::span(&mesh->centroid,1)):Bytes{},r.blob(),"Original polygon centroid");
                const auto packet_count=r.word();check(packet_count==(mesh?2:0),"Original polygon success differs");
                check(estimate_calls==(mesh?2*n:0),"Unexpected estimate callback count");
                if(mesh) {
                    const auto ids=indices(mesh->indices);
                    for(unsigned i=0;i<2;++i) {
                        Bytes actual;word(actual,i?16:1);word(actual,i?setting:1);
                        blob(actual,coordinates(i?mesh->expanded_raster_points:mesh->raster_points));blob(actual,ids);
                        equal(actual,r.blob(),"Original polygon packet");
                    }
                }
                Bytes events;
                const auto updates=mask.polygon(points,offset,setting,precision,reciprocal,[&](auto at,auto value){word(events,at);word(events,value);++writes;});
                check(updates.size()==packet_count,"Wrong polygon invalidation count");
                for(auto update:updates){word(commands,0x20);for(auto f:update.bounds)word(commands,std::bit_cast<std::uint32_t>(f));word(commands,update.bits);}
                Bytes actual;word(actual,mask.width());word(actual,mask.height());blob(actual,mask.flags());
                word(actual,static_cast<std::uint32_t>(events.size()/8));actual.insert(actual.end(),events.begin(),events.end());blob(actual,commands);
                equal(actual,r.blob(),"Original polygon mask/writes/commands");
            }
        }catch(const std::exception &e){throw std::runtime_error("Case "+std::to_string(ci)+" operation "+std::to_string(operation)+": "+e.what());}
    }
    check(r.in.peek()==EOF,"Trailing polygon reference");std::cout<<count<<" original cases, ";
}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument &){caught=true;}check(caught,"Invalid polygon input accepted");}
void guards_and_consumer() {
    const auto precision=CoastBoundsPrecision::double_or_extended;
    // A deliberately supplied reciprocal, independent of any host SSE unit.
    const auto reciprocal=[](float){return 1.f/.866f;};
    const std::array<Point2,4> points{{{4,4},{36,4},{36,36},{4,36}}};
    SceneMask mask(128,128);
    const TerrainDecalCellGeometry lines{{{12,12},{20,16},{28,20}}};
    check(build_terrain_decal_geometry(lines,mask.decal_view({0,0})).vertices.empty(),"Unmarked polygon admitted decal");
    const auto set=mask.polygon(points,{0,0},true,precision,reciprocal);
    check(set.size()==2 && set[0].bits==1 && set[1].bits==16,"Polygon command order");
    check(!build_terrain_decal_geometry(lines,mask.decal_view({0,0})).vertices.empty(),"Marked polygon rejected decal");
    mask.polygon(points,{0,0},false,precision,reciprocal);
    check(build_terrain_decal_geometry(lines,mask.decal_view({0,0})).vertices.empty(),"Cleared polygon admitted decal");
    check(std::any_of(mask.flags().begin(),mask.flags().end(),[](auto v){return v==1;}),"Clear removed persistent bit 0");
    const Bytes before(mask.flags().begin(),mask.flags().end());
    const std::array<Point2,3> collinear{{{0,0},{4,4},{8,8}}};
    check(mask.polygon(collinear,{0,0},true,precision,{}).empty(),"Failed polygon emitted commands");
    rejects([&]{mask.polygon(points,{0,0},true,precision,{});});
    rejects([&]{mask.polygon(points,{0,0},true,precision,[](float){return std::numeric_limits<float>::infinity();});});
    auto invalid=points;invalid[2].x=std::numeric_limits<float>::quiet_NaN();
    rejects([&]{mask.polygon(invalid,{0,0},true,precision,reciprocal);});
    // Original points are in range, but the expanded packet crosses the guard.
    auto boundary=points;for(auto &p:boundary)p.x+=65498;
    rejects([&]{mask.polygon(boundary,{0,0},true,precision,reciprocal);});
    check(std::equal(before.begin(),before.end(),mask.flags().begin()),"Rejected polygon partially mutated mask");
}
}
int main(int argc,char **argv) {
    try{check(argc<=2,"Usage: polygon_mask_tests [oracle.bin]");guards_and_consumer();if(argc==2)oracle(argv[1]);
        std::cout<<"PASS polygon mask: "<<checks<<" checks, "<<snapshots<<" snapshots, "<<compared<<" original bytes, "<<writes<<" writes\n";
    }catch(const std::exception &e){std::cerr<<"FAIL polygon mask: "<<e.what()<<'\n';return 1;}
}
