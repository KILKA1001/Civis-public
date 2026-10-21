#include "civ5/terrain/scene_mask.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes=civ5::assets::Bytes;
namespace {
std::size_t checks=0,compared=0,snapshots=0,writes=0;
void check(bool ok,const char *message) { ++checks; if (!ok) throw std::runtime_error(message); }
void word(Bytes &b,std::uint32_t value,unsigned n=4) { for(unsigned i=0;i<n;++i)b.push_back(std::uint8_t(value>>(8*i))); }
void blob(Bytes &b,std::span<const std::uint8_t> v) { word(b,static_cast<std::uint32_t>(v.size()));b.insert(b.end(),v.begin(),v.end()); }
struct Reader {
    Bytes bytes;std::size_t at=0;
    std::uint32_t word(unsigned n=4) { check(n<=bytes.size()-at,"Truncated mask reference");std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(bytes[at++])<<(8*i);return v; }
    float real() {return std::bit_cast<float>(word());}
    Bytes blob() {const auto n=word();check(n<=bytes.size()-at,"Truncated mask blob");Bytes b(bytes.begin()+at,bytes.begin()+at+n);at+=n;return b;}
};
void compare(Reader &r,const SceneMask &mask,const Bytes &events,const Bytes &commands) {
    Bytes actual;word(actual,mask.width());word(actual,mask.height());blob(actual,mask.flags());
    word(actual,static_cast<std::uint32_t>(events.size()/8));actual.insert(actual.end(),events.begin(),events.end());blob(actual,commands);
    const auto expected=r.blob();compared+=expected.size();++snapshots;
    if(actual!=expected) {
        std::size_t at=0;while(at<std::min(actual.size(),expected.size()) && actual[at]==expected[at])++at;
        throw std::runtime_error("Mask mismatch byte "+std::to_string(at)+" expected size "+std::to_string(expected.size())+" actual "+std::to_string(actual.size()));
    }
    check(true,"Original mask snapshot");
}
void oracle(const char *path) {
    Reader r;std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open mask reference");r.bytes.assign(std::istreambuf_iterator<char>(in),{});
    check(r.bytes.size()>=12 && std::string_view(reinterpret_cast<const char *>(r.bytes.data()),8)=="C5MSK001","Invalid mask reference");r.at=8;
    const auto count=r.word();check(count<=10000,"Excessive mask cases");
    for(unsigned ci=0;ci<count;++ci) {
        unsigned operation=0;
        try {
            const auto width=r.word(),height=r.word(),pc=r.word();check(pc==24 || pc==53,"Invalid mask precision");
            const auto precision=pc==24?CoastBoundsPrecision::single:CoastBoundsPrecision::double_or_extended;
            SceneMask mask(width,height);Bytes commands;word(commands,0x1d);commands.resize(24);
            compare(r,mask,{},commands);const auto n=r.word();check(n<=10000,"Excessive operations");
            for(;operation<n;++operation) {
                const auto indexed=r.word(),np=r.word();check(np<=1000000,"Excessive mask vertices");
                std::vector<Point2> points;for(unsigned i=0;i<np;++i){const auto x=r.real(),y=r.real();points.push_back({x,y});}
                const auto ni=r.word();check(ni<=1000000,"Excessive mask indices");std::vector<std::uint16_t> indices;
                for(unsigned i=0;i<ni;++i)indices.push_back(static_cast<std::uint16_t>(r.word(2)));
                const auto bits=r.word(),setting=r.word();Bytes events;
                const auto observer=[&](std::uint32_t at,std::uint8_t value){word(events,at);word(events,value);++writes;};
                const auto update=indexed?mask.indexed(points,indices,static_cast<std::uint8_t>(bits),setting!=0,precision,observer):
                    mask.triangles(points,static_cast<std::uint8_t>(bits),setting!=0,precision,observer);
                word(commands,0x20);for(auto f:update.bounds)word(commands,std::bit_cast<std::uint32_t>(f));word(commands,update.bits);
                compare(r,mask,events,commands);
            }
        } catch(const std::exception &e){throw std::runtime_error("Case "+std::to_string(ci)+" operation "+std::to_string(operation)+": "+e.what());}
    }
    check(r.at==r.bytes.size(),"Trailing mask reference");std::cout<<count<<" original cases, ";
}
void guards() {
    SceneMask mask(33,65);check(mask.width()==16 && mask.height()==32,"Halving/rounding dimensions");
    const std::array<Point2,3> points{{{2,2},{20,2},{2,20}}};const std::array<std::uint16_t,3> bad{{0,1,3}};bool caught=false;
    try{mask.indexed(points,bad,16,true,CoastBoundsPrecision::double_or_extended);}catch(const std::invalid_argument &){caught=true;}
    check(caught && std::all_of(mask.flags().begin(),mask.flags().end(),[](auto v){return v==0;}),"Malformed indices mutated mask");
}
void coast_oracle(const char *path) {
    Reader r;std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open coast mask reference");r.bytes.assign(std::istreambuf_iterator<char>(in),{});
    check(r.bytes.size()>=12 && std::string_view(reinterpret_cast<const char *>(r.bytes.data()),8)=="C5CMS001","Invalid coast mask reference");r.at=8;
    const auto cases=r.word();check(cases<=10000,"Excessive coast cases");
    for(unsigned ci=0;ci<cases;++ci) {
        try {
            const auto width=r.word(),height=r.word(),pc=r.word(),sets=r.word();check((pc==24 || pc==53) && sets<=10000,"Invalid coast mask input");
            const auto precision=pc==24?CoastBoundsPrecision::single:CoastBoundsPrecision::double_or_extended;
            std::vector<CoastCurveSet> curves;
            for(unsigned si=0;si<sets;++si) {
                const auto flag=r.word(),count=r.word();Reader source{r.blob()};check(count<=1000000 && source.bytes.size()==std::size_t(count)*5*14,"Coast mask curve extent");
                CoastCurves group;
                for(auto &line:group)for(unsigned i=0;i<count;++i) {
                    const auto x=source.real(),y=source.real(),t=source.real();const auto a=source.word(1),b=source.word(1);
                    line.push_back({x,y,t,static_cast<std::uint8_t>(a),static_cast<std::uint8_t>(b)});
                }
                curves.emplace_back(std::move(group),flag);
                if(flag)continue;
                const auto mesh=build_coast_mask_mesh(curves.back());Bytes actual,points,indices;
                for(auto p:mesh.points){word(points,std::bit_cast<std::uint32_t>(p.x));word(points,std::bit_cast<std::uint32_t>(p.y));}
                for(auto i:mesh.indices)word(indices,i,2);
                blob(actual,points);blob(actual,indices);word(actual,static_cast<std::uint32_t>(mesh.chunks.size()));
                for(auto chunk:mesh.chunks)for(auto v:{chunk.first_vertex,chunk.vertex_count,chunk.first_triangle,chunk.triangle_count,std::uint32_t(chunk.bits)})word(actual,v);
                const auto expected=r.blob();compared+=expected.size();++snapshots;check(actual==expected,"Original coast mask mesh differs");
            }
            const auto mask=build_coast_scene_mask(curves,width,height,precision);Bytes actual;word(actual,mask.width());word(actual,mask.height());blob(actual,mask.flags());
            const auto expected=r.blob();compared+=expected.size();++snapshots;check(actual==expected,"Original coast mask field differs");
        } catch(const std::exception &e){throw std::runtime_error("Coast mask case "+std::to_string(ci)+": "+e.what());}
    }
    check(r.at==r.bytes.size(),"Trailing coast mask reference");std::cout<<cases<<" coast cases, ";
}
}
int main(int argc,char **argv) {
    try{check(argc<=3,"Usage: scene_mask_tests [oracle.bin [coast-oracle.bin]]");guards();if(argc>=2)oracle(argv[1]);if(argc==3)coast_oracle(argv[2]);
        std::cout<<"PASS scene mask: "<<checks<<" checks, "<<snapshots<<" snapshots, "<<compared<<" original bytes, "<<writes<<" writes\n";
    }catch(const std::exception &e){std::cerr<<"FAIL scene mask: "<<e.what()<<'\n';return 1;}
}
