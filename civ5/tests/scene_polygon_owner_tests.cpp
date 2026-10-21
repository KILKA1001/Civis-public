#include "civ5/terrain/scene_polygon_owner.hpp"
#include "civ5/terrain/terrain_road_grid.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
using Bytes=std::vector<std::uint8_t>;
std::size_t checks=0,compared=0,events=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
void put(Bytes &out,std::uint32_t value){for(unsigned i=0;i<4;++i)out.push_back(std::uint8_t(value>>(i*8)));}
void set(Bytes &out,std::size_t offset,std::uint32_t value){
    check(offset+4<=out.size(),"Polygon queue command extent");
    for(unsigned i=0;i<4;++i)out[offset+i]=std::uint8_t(value>>(i*8));
}
void blob(Bytes &out,const Bytes &data){put(out,static_cast<std::uint32_t>(data.size()));out.insert(out.end(),data.begin(),data.end());}
struct Reader {
    std::span<const std::uint8_t> data;
    std::uint32_t word(){check(data.size()>=4,"Truncated polygon owner oracle");
        std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(data[i])<<(i*8);
        data=data.subspan(4);return value;}
    std::span<const std::uint8_t> take(std::size_t n){check(data.size()>=n,"Truncated polygon owner record");
        auto value=data.first(n);data=data.subspan(n);return value;}
    Bytes blob(){auto raw=take(word());return Bytes(raw.begin(),raw.end());}
};
std::uint32_t at(std::span<const std::uint8_t> packet,std::size_t offset){
    check(packet.size()>=offset+4,"Truncated source polygon packet");
    std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(packet[offset+i])<<(i*8);
    return value;
}
void equal(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected){
    compared+=expected.size();check(actual.size()==expected.size(),"Polygon owner state size");
    auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if(mismatch.first!=actual.end())throw std::runtime_error("Polygon owner state byte "+
        std::to_string(mismatch.first-actual.begin()));
    ++checks;
}
void run(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open polygon owner oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5SPO002",
        "Wrong polygon owner oracle");
    Reader r{std::span(data).subspan(8)};
    const auto cases=r.word();check(cases>0&&cases<=100,"Polygon owner case count");
    for(unsigned ci=0;ci<cases;++ci){
        try{
            const auto case_bytes=r.blob();Reader c{case_bytes};
            const auto precision=c.word(),tile=c.word(),deferred=c.word();
            const auto bounds_shift=std::bit_cast<std::int32_t>(c.word());
            const auto tile_shift=std::bit_cast<std::int32_t>(c.word());
            const auto steps=c.word();
            check((precision==24||precision==53)&&tile<=0xffff&&deferred<=1&&steps<=64,
                "Invalid polygon owner scenario");
            std::vector<std::uint16_t> tiles(8*256,0xffff);
            tiles[2]=static_cast<std::uint16_t>(tile);
            ScenePolygonOwnerGrid grid{};grid.width=8;grid.height=8;grid.tile_ids=tiles;
            for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
                for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
            grid.bounds_matrix[12]=bounds_shift;
            grid.tile_matrix[12]=tile_shift;
            ScenePolygonOwner owner(grid);
            const auto snapshot=[&](std::uint32_t result,const std::vector<Bytes> &submits){
                Bytes actual;put(actual,result);
                put(actual,owner.completed_count());
                put(actual,static_cast<std::uint32_t>(owner.pending_count()));
                put(actual,static_cast<std::uint32_t>(owner.records().size()));
                const auto plot=owner.plot(tile==0xffff?0:static_cast<std::uint16_t>(tile));
                put(actual,static_cast<std::uint32_t>(plot.size()));
                put(actual,static_cast<std::uint32_t>(submits.size()));
                for(const auto &submit:submits)blob(actual,submit);
                for(const auto &record:owner.records()){
                    put(actual,record.polygon_id);put(actual,record.tile_id);
                    put(actual,static_cast<std::uint32_t>(record.assembly.points().size()));
                    for(const auto point:record.assembly.points()){
                        put(actual,std::bit_cast<std::uint32_t>(point.x));
                        put(actual,std::bit_cast<std::uint32_t>(point.y));
                    }
                }
                for(auto index:plot){actual.push_back(std::uint8_t(index));actual.push_back(std::uint8_t(index>>8));}
                put(actual,static_cast<std::uint32_t>(owner.deferred_events().size()));
                for(const auto &queued:owner.deferred_events()){
                    Bytes command(0xc0,0);
                    set(command,0,0xc0);set(command,0x14,0x5750f0);
                    set(command,0x18,std::bit_cast<std::uint32_t>(queued.position[2]));
                    for(unsigned i=0;i<3;++i)
                        set(command,0x20+i*4,std::bit_cast<std::uint32_t>(queued.position[i]));
                    set(command,0x2c,queued.polygon_id);
                    set(command,0x30,queued.opaque_word);
                    set(command,0x34,queued.chunk.block_index);
                    set(command,0x38,queued.chunk.expected_packets);
                    set(command,0x3c,queued.chunk.point_count);
                    check(queued.chunk.points.size()==16,"Deferred polygon packet extent");
                    for(unsigned i=0;i<16;++i){
                        set(command,0x40+i*8,std::bit_cast<std::uint32_t>(queued.chunk.points[i].x));
                        set(command,0x44+i*8,std::bit_cast<std::uint32_t>(queued.chunk.points[i].y));
                    }
                    actual.insert(actual.end(),command.begin(),command.end());
                }
                return actual;
            };
            for(unsigned step=0;step<steps;++step){
                auto raw=c.take(0xa0);
                ScenePolygonEvent event{};
                for(unsigned i=0;i<3;++i)event.position[i]=std::bit_cast<std::int32_t>(at(raw,i*4));
                event.polygon_id=at(raw,0xc);
                event.opaque_word=at(raw,0x10);
                event.chunk.block_index=at(raw,0x14);
                event.chunk.expected_packets=at(raw,0x18);
                event.chunk.point_count=at(raw,0x1c);
                for(unsigned i=0;i<16;++i)event.chunk.points.push_back({
                    std::bit_cast<float>(at(raw,0x20+i*8)),
                    std::bit_cast<float>(at(raw,0x24+i*8))});
                std::vector<Bytes> submits;
                const auto result=owner.accept(event,[&](std::uint16_t output_tile,
                        std::uint32_t slot,std::uint32_t index,std::span<const Point2> points){
                    Bytes row;put(row,output_tile);put(row,slot);put(row,index);
                    put(row,static_cast<std::uint32_t>(points.size()));
                    for(const auto point:points){put(row,std::bit_cast<std::uint32_t>(point.x));
                                                  put(row,std::bit_cast<std::uint32_t>(point.y));}
                    submits.push_back(std::move(row));
                },deferred!=0);
                const auto expected=c.blob();
                equal(snapshot(static_cast<std::uint32_t>(result),submits),expected);++events;
            }
            const auto has_drain=c.word();check(has_drain<=1,"Invalid polygon owner drain flag");
            if(has_drain){
                std::vector<Bytes> submits;
                const auto queued=owner.deferred_events().size();
                check(owner.drain_deferred([&](std::uint16_t output_tile,
                        std::uint32_t slot,std::uint32_t index,std::span<const Point2> points){
                    Bytes row;put(row,output_tile);put(row,slot);put(row,index);
                    put(row,static_cast<std::uint32_t>(points.size()));
                    for(const auto point:points){put(row,std::bit_cast<std::uint32_t>(point.x));
                                                 put(row,std::bit_cast<std::uint32_t>(point.y));}
                    submits.push_back(std::move(row));
                })==queued,"Polygon deferred replay count");
                equal(snapshot(4,submits),c.blob());
            }
            check(c.data.empty(),"Trailing polygon owner scenario");
        }catch(const std::exception &e){
            throw std::runtime_error("case "+std::to_string(ci)+": "+e.what());
        }
    }
    check(r.data.empty(),"Trailing polygon owner oracle");
    std::cout<<"Original cases="<<cases<<"; events="<<events<<"; exact bytes="<<compared<<'\n';
}
void neighbor_checks(){
    constexpr std::array<std::array<std::int32_t,3>,6> offsets{{
        {0,1,-1},{-1,0,1},{1,-1,0},{0,-1,1},{1,0,-1},{-1,1,0}}};
    constexpr std::array<std::int32_t,3> center{3,3,-6};
    std::vector<std::uint16_t> tiles(8*256,0xffffu);
    tiles[3*256+4]=0;
    ScenePolygonOwnerGrid grid{};grid.width=8;grid.height=8;grid.tile_ids=tiles;
    for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
        for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
    for(unsigned i=0;i<offsets.size();++i){
        const auto x=center[0]+offsets[i][0],y=center[1]+offsets[i][1];
        tiles[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x+y/2)]=
            static_cast<std::uint16_t>(i+1);
    }
    ScenePolygonOwner owner(grid);
    for(unsigned i=0;i<offsets.size()+1;++i){
        const auto &offset=offsets[i%offsets.size()];
        ScenePolygonEvent event{};
        for(unsigned axis=0;axis<3;++axis)event.position[axis]=center[axis]+offset[axis];
        event.polygon_id=100+i;
        event.chunk={0,1,1,std::vector<Point2>(16,{float(i),0})};
        check(owner.accept(event)==ScenePolygonEventResult::completed,
            "Adjacent source polygon did not complete");
    }
    const std::vector<ScenePolygonNeighborReapply> expected{
        {1,0,0},{1,6,1},{2,1,0},{3,2,0},{4,3,0},{5,4,0},{6,5,0}};
    check(owner.neighbor_reapplications(center)==expected,
        "Source neighbor walk changed tile or polygon order");
    tiles[3*256+3]=1;
    const std::vector<ScenePolygonNeighborReapply> repeated{
        {1,0,0},{1,6,1},{1,0,0},{1,6,1},{3,2,0},{4,3,0},{5,4,0},{6,5,0}};
    check(owner.neighbor_reapplications(center)==repeated,
        "Repeated neighbor tile lost source-order reapplication");
    tiles[3*256+3]=2;
    tiles[3*256+4]=0xffffu;
    check(owner.neighbor_reapplications(center).empty(),
        "Missing central plot must not reapply neighbors");
    tiles[3*256+4]=0;
    check(owner.neighbor_reapplications({-10,3,7}).empty(),
        "Out-of-bounds central plot must not reapply neighbors");
}
void removal_oracle(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open polygon removal oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5SPR001",
        "Wrong polygon removal oracle");
    Reader r{std::span(data).subspan(8)};
    const auto cases=r.word();check(cases==2,"Polygon removal case count");
    constexpr std::array<std::int32_t,3> center{3,3,-6};
    constexpr std::array<std::array<std::int32_t,3>,6> offsets{{
        {0,1,-1},{-1,0,1},{1,-1,0},{0,-1,1},{1,0,-1},{-1,1,0}}};
    for(unsigned ci=0;ci<cases;++ci){
        const auto case_bytes=r.blob();Reader c{case_bytes};
        const auto central_count=c.word(),operation_count=c.word();
        check(central_count==ci+1&&operation_count<20,"Polygon removal scenario header");
        std::vector<std::uint16_t> tiles(8*256,0xffffu);
        ScenePolygonOwnerGrid grid{};grid.width=8;grid.height=8;grid.tile_ids=tiles;
        for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
            for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
        std::array<std::array<std::int32_t,3>,7> positions{};positions[0]=center;
        for(unsigned i=0;i<offsets.size();++i)
            for(unsigned axis=0;axis<3;++axis)positions[i+1][axis]=center[axis]+offsets[i][axis];
        for(unsigned i=0;i<positions.size();++i){
            const auto x=positions[i][0],y=positions[i][1];
            tiles[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x+y/2)]=
                static_cast<std::uint16_t>(i+1);
        }
        ScenePolygonOwner owner(grid);
        const ScenePolygonDirtyProjection projection{{3,3},identity_matrix(),{0.f,0.f}};
        const auto add_event=[&](const std::array<std::int32_t,3> &position,
                                 std::uint32_t polygon_id){
            ScenePolygonEvent event{};event.position=position;event.polygon_id=polygon_id;
            event.chunk={0,1,1,std::vector<Point2>(16,{float(polygon_id),0})};
            return event;
        };
        for(unsigned i=0;i<positions.size();++i)
            check(owner.accept(add_event(positions[i],101+i))==ScenePolygonEventResult::completed,
                  "Source removal fixture polygon did not complete");
        owner.set_record_handles(0,{900,901});
        if(central_count==2){
            check(owner.accept(add_event(center,108))==ScenePolygonEventResult::completed,
                  "Second central polygon did not complete");
            owner.set_record_handles(7,{902,903});
        }
        Bytes actual_calls;
        const auto removed=owner.remove_cell(center,projection,
            [&](std::uint32_t handle){put(actual_calls,1);put(actual_calls,handle);
                put(actual_calls,0);put(actual_calls,0);},
            [&](std::uint16_t tile,std::uint32_t slot,std::uint32_t,
                bool setting,std::span<const Point2>){
                put(actual_calls,2);put(actual_calls,tile);put(actual_calls,slot);
                put(actual_calls,setting?1:0);
            },
            [&](std::array<float,4>){put(actual_calls,3);put(actual_calls,0);
                put(actual_calls,0);put(actual_calls,0);});
        check(removed.found&&removed.tile_id==1&&removed.removed==central_count&&
            removed.reapplied==6,"Polygon removal counts");
        equal(actual_calls,c.take(operation_count*16));
        const auto free_head=c.word(),active_count=c.word(),plot_head=c.word(),plot_count=c.word();
        const auto center_tile=c.word(),first_flag=c.word(),free_next=c.word();
        const auto handle0=c.word(),handle1=c.word();
        check(free_head==0&&active_count==owner.active_record_count()&&
            plot_head==1&&plot_count==6&&center_tile==0xffffu&&
            first_flag==1&&!owner.records()[0].active&&
            owner.records()[0].handles==std::array<std::uint32_t,2>{handle0,handle1},
            "Original removal state mismatch");
        for(unsigned tile=2;tile<=7;++tile)
            check(owner.plot(static_cast<std::uint16_t>(tile)).size()==1,
                "Neighbor plot changed during removal");
        check(owner.plot(1).empty()&&
            owner.accept(add_event(center,200))==ScenePolygonEventResult::rejected,
            "Removed tile remains addressable");
        const auto repeat=owner.remove_cell(center,projection,
            [](std::uint32_t){throw std::runtime_error("Repeated removal touched decal");},
            [](std::uint16_t,std::uint32_t,std::uint32_t,bool,std::span<const Point2>){
                throw std::runtime_error("Repeated removal touched mask");
            },[](std::array<float,4>){
                throw std::runtime_error("Repeated removal emitted dirty command");});
        check(!repeat.found&&repeat.removed==0&&repeat.reapplied==0,
            "Missing central tile was removed twice");
        std::vector<std::uint32_t> reused;
        for(unsigned i=0;i<central_count;++i)
            check(owner.accept(add_event(positions[1],300+i),
                [&](std::uint16_t,std::uint32_t,std::uint32_t index,std::span<const Point2>){
                    reused.push_back(index);
                })==ScenePolygonEventResult::completed,"Freed polygon record not reusable");
        check(reused.size()==central_count&&reused[0]==free_head&&
            (central_count==1||reused[1]==free_next),
            "Polygon free-list reuse order changed");
        check(c.data.empty(),"Trailing polygon removal scenario");
    }
    check(r.data.empty(),"Trailing polygon removal oracle");
}
void dirty_region_oracle(const char *path,bool command=false){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open polygon dirty region oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&std::string_view(reinterpret_cast<const char *>(data.data()),8)==
        (command?"C5SPC001":"C5SPB001"),
        "Wrong polygon dirty region oracle");
    Reader r{std::span(data).subspan(8)};
    const auto cases=r.word();check(cases==9,"Polygon dirty region case count");
    constexpr std::array<std::int32_t,3> center{3,3,-6};
    constexpr std::array<std::array<std::int32_t,3>,6> offsets{{
        {0,1,-1},{-1,0,1},{1,-1,0},{0,-1,1},{1,0,-1},{-1,1,0}}};
    for(unsigned ci=0;ci<cases;++ci){
        const auto case_bytes=r.blob();Reader c{case_bytes};
        const auto precision=c.word(),width=c.word(),height=c.word();
        const auto x=std::bit_cast<std::int32_t>(c.word());
        const auto y=std::bit_cast<std::int32_t>(c.word());
        const auto missing=c.word(),extra=c.word();
        check((precision==24||precision==53)&&width>0&&width<=8&&height>0&&height<=8&&
              x>=-32768&&x<=32767&&y>=-32768&&y<=32767&&missing<64&&extra<=1,
              "Invalid polygon dirty region scenario");
        ScenePolygonDirtyProjection projection{};
        projection.plot_coordinates={static_cast<std::int16_t>(x),static_cast<std::int16_t>(y)};
        for(float &value:projection.world_matrix)value=std::bit_cast<float>(c.word());
        for(float &value:projection.world_offset)value=std::bit_cast<float>(c.word());
        std::array<std::uint8_t,128> expected_packet{};
        std::array<std::uint32_t,4> expected{};
        if(command){
            const auto source=c.take(expected_packet.size());
            std::copy(source.begin(),source.end(),expected_packet.begin());
            for(unsigned lane=0;lane<4;++lane)
                expected[lane]=at(expected_packet,16+4*lane);
        }else for(auto &value:expected)value=c.word();
        check(c.data.empty(),"Trailing polygon dirty region case");
        std::vector<std::uint16_t> tiles(8*256,0xffffu);
        tiles[3*256+4]=1;
        for(unsigned i=0;i<offsets.size();++i){
            if(missing&(1u<<i))continue;
            const auto nx=center[0]+offsets[i][0],ny=center[1]+offsets[i][1];
            const auto column=nx+(ny>>1);
            if(column>=0&&column<static_cast<int>(width)&&ny>=0&&ny<static_cast<int>(height))
                tiles[static_cast<std::size_t>(ny)*256+static_cast<std::size_t>(column)]=
                    static_cast<std::uint16_t>(i+2);
        }
        ScenePolygonOwnerGrid grid{};grid.width=static_cast<std::int32_t>(width);
        grid.height=static_cast<std::int32_t>(height);grid.tile_ids=tiles;
        for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
            for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
        // 4A9750 uses +700, while the polygon-event gate uses +5B0.
        grid.bounds_matrix[12]=1;
        ScenePolygonOwner owner(grid);
        unsigned dirty_calls=0;
        const auto result=owner.remove_cell(center,projection,
            [](std::uint32_t){throw std::runtime_error("Unexpected dirty oracle handle");},
            [](std::uint16_t,std::uint32_t,std::uint32_t,bool,std::span<const Point2>){
                throw std::runtime_error("Unexpected dirty oracle mask");
            },[&](std::array<float,4> region){
                ++dirty_calls;
                if(command){
                    const auto packet=make_terrain_road_region_command(region);
                    equal(packet,expected_packet);
                }
                for(unsigned lane=0;lane<4;++lane)
                    if(std::bit_cast<std::uint32_t>(region[lane])!=expected[lane])
                        throw std::runtime_error("Dirty region case "+std::to_string(ci)+
                            " lane "+std::to_string(lane)+" differs from original");
            });
        check(result.found&&result.tile_id==1&&result.removed==0&&
              result.reapplied==0&&dirty_calls==1,"Polygon dirty region callback lifecycle");
    }
    check(r.data.empty(),"Trailing polygon dirty region oracle");
    std::cout<<"Original dirty "<<(command?"command":"region")<<" cases="<<cases
             <<"; exact float words="<<cases*4<<'\n';
}
}
int main(int argc,char **argv){
    try{check(argc>=2&&argc<=5,
        "Usage: civ5_scene_polygon_owner_tests owner.bin [removal.bin] [dirty-region.bin] [dirty-command.bin]");
        run(argv[1]);neighbor_checks();
        if(argc>=3)removal_oracle(argv[2]);
        if(argc>=4)dirty_region_oracle(argv[3]);
        if(argc==5)dirty_region_oracle(argv[4],true);
        std::cout<<"PASS scene polygon owner checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<"FAIL scene polygon owner: "<<e.what()<<'\n';return 1;}
}
