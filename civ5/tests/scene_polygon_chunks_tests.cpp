#include "civ5/terrain/scene_polygon_chunks.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
using Bytes=std::vector<std::uint8_t>;
std::size_t checks=0,compared=0,packets=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
void put(Bytes &out,std::uint32_t value){for(unsigned i=0;i<4;++i)out.push_back(std::uint8_t(value>>(i*8)));}
struct Reader {
    std::span<const std::uint8_t> data;
    std::uint32_t word(){check(data.size()>=4,"Truncated polygon chunk oracle");
        std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(data[i])<<(8*i);
        data=data.subspan(4);return value;}
    Point2 point(){return {std::bit_cast<float>(word()),std::bit_cast<float>(word())};}
    Bytes blob(){const auto n=word();check(n<=data.size(),"Truncated polygon chunk record");
        Bytes out(data.begin(),data.begin()+n);data=data.subspan(n);return out;}
};
void equal(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected){
    compared+=expected.size();check(actual.size()==expected.size(),"Polygon assembly byte count");
    auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if(mismatch.first!=actual.end())throw std::runtime_error("Polygon assembly byte " +
        std::to_string(mismatch.first-actual.begin()));
    ++checks;
}
void oracle(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open polygon chunk oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5SPC001",
        "Wrong polygon chunk oracle");
    Reader r{std::span(data).subspan(8)};const auto cases=r.word();check(cases<=1000,"Excessive polygon chunk cases");
    for(unsigned ci=0;ci<cases;++ci){
        try{
            const auto case_bytes=r.blob();Reader c{case_bytes};
            const auto precision=c.word(),steps=c.word();
            check((precision==24||precision==53)&&steps<=20,"Invalid polygon chunk case");
            ScenePolygonPointAssembly assembly;
            for(unsigned step=0;step<steps;++step){
                ScenePolygonPointChunk packet{};
                packet.block_index=c.word();packet.expected_packets=c.word();packet.point_count=c.word();
                for(unsigned i=0;i<16;++i)packet.points.push_back(c.point());
                const auto expected=c.blob();
                const bool complete=assembly.append(packet);++packets;
                Bytes actual;put(actual,assembly.expected_packets());put(actual,assembly.received_packets());
                put(actual,complete?1:0);put(actual,static_cast<std::uint32_t>(assembly.points().size()));
                for(const auto point:assembly.points()){
                    put(actual,std::bit_cast<std::uint32_t>(point.x));
                    put(actual,std::bit_cast<std::uint32_t>(point.y));
                }
                equal(actual,expected);
            }
            check(c.data.empty(),"Trailing polygon chunk case");
        }catch(const std::exception &e){
            throw std::runtime_error("case "+std::to_string(ci)+": "+e.what());
        }
    }
    check(r.data.empty(),"Trailing polygon chunk oracle");
    std::cout<<"Original cases="<<cases<<"; packets="<<packets<<"; exact bytes="<<compared<<'\n';
}
void guards(){
    ScenePolygonPointAssembly assembly;
    ScenePolygonPointChunk p{};p.block_index=0;p.expected_packets=1;p.point_count=3;
    p.points.assign(16,Point2{0,0});
    p.points[0]={1,2};p.points[1]={3,4};p.points[2]={5,6};
    check(assembly.append(p),"Valid first polygon chunk did not complete");
    for(auto [index,count,expected]:{std::array<std::uint32_t,3>{32,1,1},
                                     {0,17,1},{31,17,1},{0,1,0xffffffffu},{0,2,1},{0,0,1}}){
        auto invalid=p;invalid.block_index=index;invalid.point_count=count;invalid.expected_packets=expected;
        bool caught=false;try{assembly.append(invalid);}catch(const std::invalid_argument &){caught=true;}
        check(caught&&assembly.expected_packets()==1&&assembly.received_packets()==1&&
              assembly.points().size()==3,"Invalid packet changed polygon assembly");
    }
}
}
int main(int argc,char **argv){
    try{check(argc<=2,"Usage: civ5_scene_polygon_chunks_tests [oracle.bin]");
        guards();if(argc==2)oracle(argv[1]);
        std::cout<<"PASS scene polygon chunks checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<"FAIL scene polygon chunks: "<<e.what()<<'\n';return 1;}
}
