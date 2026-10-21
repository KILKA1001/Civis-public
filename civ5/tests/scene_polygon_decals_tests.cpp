#include "civ5/terrain/scene_polygon_decals.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes=civ5::assets::Bytes;
namespace {
std::size_t checks=0,compared=0,draws=0,mask_writes=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
void put(Bytes &out,std::uint32_t value,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)out.push_back(std::uint8_t(value>>(i*8)));}
void real(Bytes &out,float value){put(out,std::bit_cast<std::uint32_t>(value));}
void blob(Bytes &out,std::span<const std::uint8_t> value){put(out,static_cast<std::uint32_t>(value.size()));out.insert(out.end(),value.begin(),value.end());}
struct Reader {
    std::span<const std::uint8_t> data;
    std::uint32_t word(){check(data.size()>=4,"Truncated oracle word");std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(data[i])<<(8*i);data=data.subspan(4);return v;}
    float real(){return std::bit_cast<float>(word());}
    Point2 point(){const auto x=real(),y=real();return {x,y};}
    Bytes blob(){const auto n=word();check(n<=data.size(),"Truncated oracle blob");Bytes b(data.begin(),data.begin()+n);data=data.subspan(n);return b;}
};
void equal_bytes(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,const char *message){
    compared+=expected.size();
    if(!std::equal(actual.begin(),actual.end(),expected.begin(),expected.end())){
        std::size_t i=0;while(i<std::min(actual.size(),expected.size())&&actual[i]==expected[i])++i;
        throw std::runtime_error(std::string(message)+" byte="+std::to_string(i));
    }
    ++checks;
}
void command(Bytes &commands,const SceneMaskUpdate &update){put(commands,0x20);for(auto v:update.bounds)real(commands,v);put(commands,update.bits);}
void oracle(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open polygon decal oracle");
    Bytes raw((std::istreambuf_iterator<char>(in)),{});
    check(raw.size()>=8&&std::string_view(reinterpret_cast<const char *>(raw.data()),8)=="C5SPD001","Wrong polygon decal oracle");
    Reader r{std::span(raw).subspan(8)};const auto count=r.word();check(count<=10000,"Excessive polygon cases");
    for(unsigned ci=0;ci<count;++ci){
        try{
            const auto record=r.blob();Reader c{record};
            const auto pc=c.word(),path_id=c.word(),seed=c.word(),flags=c.word(),kind=c.word(),enabled=c.word(),chunk=c.word();
            check((pc==24||pc==53)&&path_id<=1&&kind<=255&&enabled<=255&&chunk>=1&&chunk<=30,"Invalid polygon source");
            const auto precision=pc==24?CoastBoundsPrecision::single:CoastBoundsPrecision::double_or_extended;
            const auto transform=path_id?Vec4Path::sse:Vec4Path::legacy_x87;
            const auto offset=c.point();const auto angle=c.real();const auto field_offset=c.point();
            const auto n=c.word();check(n<=512,"Excessive polygon points");
            std::vector<Point2> points;for(unsigned i=0;i<n;++i)points.push_back(c.point());
            ScenePolygonDecalPalette palette;
            for(auto &range:palette.ranges){range.first=c.word();range.end=c.word();}
            const auto palette_bytes=c.blob();check(palette_bytes.size()%16==0,"Invalid palette bytes");Reader rows{palette_bytes};
            while(!rows.data.empty()){const auto priority=rows.real();const auto a=rows.word(),h=rows.word(),s=rows.word();palette.records.push_back({priority,a,h,s});}
            const auto estimate=c.word(),final_random=c.word(),calls=c.word();
            const std::array<std::uint32_t,2> expected_handles{c.word(),c.word()};
            const auto submitted=c.word();check(submitted<=2,"Excessive polygon submissions");
            std::vector<Bytes> expected;for(unsigned i=0;i<submitted;++i)expected.push_back(c.blob());
            const auto expected_snapshot=c.blob();check(c.data.empty(),"Trailing polygon case data");
            Reader expected_mask{expected_snapshot};check(expected_mask.word()==64&&expected_mask.word()==64,"Unexpected oracle mask size");
            const auto expected_flags=expected_mask.blob();
            SceneMask mask(128,128);Bytes commands;put(commands,0x1d);commands.resize(24);
            const std::array<Point2,4> background{{{0,0},{128,0},{128,128},{0,128}}};
            const std::array<std::uint16_t,6> indices{0,1,2,0,2,3};
            command(commands,mask.indexed(background,indices,2,true,precision));
            TerrainRandom random(seed);ScenePolygonDecalState state{flags,std::uint8_t(kind),std::uint8_t(enabled),{0xeeeeeeee,0xdddddddd}};
            unsigned actual_submissions=0,reciprocals=0;Bytes writes;
            const auto result=update_scene_polygon_decals(points,state,mask,offset,{angle,field_offset},palette,random,precision,transform,
                [&](float v){check(v==.866f,"Wrong polygon reciprocal input");++reciprocals;return std::bit_cast<float>(estimate);},
                [&](unsigned output,std::uint32_t selected,const TerrainDecalRecord &material,const TerrainDecalGeometry &geometry){
                    check(output==actual_submissions&&output<expected.size(),"Unexpected polygon submission order");
                    // Clipped polygons can submit without covering a mask sample.
                    // Compare the complete original field at this boundary.
                    check(reciprocals==2*n&&std::equal(mask.flags().begin(),mask.flags().end(),expected_flags.begin(),expected_flags.end()),
                        "Submission preceded the original admission-mask state");
                    const auto handle=0x51000000+output;Bytes packet;put(packet,selected);put(packet,random.state());put(packet,static_cast<std::uint32_t>(random.calls()));put(packet,handle);
                    real(packet,material.priority);put(packet,material.albedo);put(packet,material.heightmap);put(packet,material.specular);
                    Bytes vertices,ids;for(auto v:geometry.vertices)for(auto f:{v.x,v.y,v.u,v.v})real(vertices,f);
                    for(auto i:geometry.indices)put(ids,i,2);blob(packet,vertices);blob(packet,ids);
                    equal_bytes(packet,expected[output],"Original polygon packet");++actual_submissions;++draws;return handle;
                },[&](auto index,auto value){put(writes,index);put(writes,value);++mask_writes;});
            check(result.submitted==submitted&&actual_submissions==submitted,"Original submission count");
            check(random.state()==final_random&&random.calls()==calls,"Original polygon RNG cursor");
            check(state.handles==expected_handles,"Original polygon handle publication");
            check(reciprocals==(result.mask_updates.empty()?0:2*n),"Extra or missing reciprocal calls");
            for(const auto &update:result.mask_updates)command(commands,update);
            Bytes snapshot;put(snapshot,mask.width());put(snapshot,mask.height());blob(snapshot,mask.flags());
            put(snapshot,static_cast<std::uint32_t>(writes.size()/8));snapshot.insert(snapshot.end(),writes.begin(),writes.end());blob(snapshot,commands);
            equal_bytes(snapshot,expected_snapshot,"Original admission mask/write/command order");
        }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(ci)+": "+e.what());}
    }
    check(r.data.empty(),"Trailing oracle cases");std::cout<<"Original cases="<<count<<"; draws="<<draws<<"; mask writes="<<mask_writes<<"; exact bytes="<<compared<<'\n';
}
void failure_boundary(){
    const std::array<Point2,4> points{{{4,4},{36,4},{36,36},{4,36}}};
    const auto precision=CoastBoundsPrecision::double_or_extended;
    const auto reciprocal=[](float){return std::bit_cast<float>(0x3f93ce56u);};
    ScenePolygonDecalPalette palette;palette.ranges.fill({0,1});palette.records.push_back({1,0,0,0});
    for(unsigned failing: {0u,1u}){
        SceneMask mask(128,128);TerrainRandom random(1);ScenePolygonDecalState state{0x10,8,1,{11,12}};
        bool caught=false;unsigned submits=0;
        try{update_scene_polygon_decals(points,state,mask,{0,0},{0,{1024,2048}},palette,random,precision,Vec4Path::sse,reciprocal,
            [&](unsigned output,auto,const auto &,const auto &){++submits;if(output==failing)throw std::runtime_error("submission failure");return 21u;});}
        catch(const std::runtime_error &){caught=true;}
        check(caught&&submits==failing+1,"Missing submission failure");
        check(state.handles==std::array<std::uint32_t,2>{failing?21u:11u,12},"Failed publication changed handles");
        check(random.calls()==failing+2,"Failure rolled back or skipped native RNG draws");
        check(std::any_of(mask.flags().begin(),mask.flags().end(),[](auto f){return (f&16)!=0;}),"Failure lost earlier mask update");
    }
}
}
int main(int argc,char **argv){
    try{check(argc<=2,"Usage: civ5_scene_polygon_decals_tests [oracle.bin]");failure_boundary();if(argc==2)oracle(argv[1]);
        std::cout<<"PASS scene polygon decals checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<"FAIL scene polygon decals: "<<e.what()<<'\n';return 1;}
}
