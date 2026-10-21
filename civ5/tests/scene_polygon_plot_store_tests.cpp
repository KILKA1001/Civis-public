#include "civ5/terrain/scene_polygon_plot_store.hpp"
#include "civ5/terrain/scene_polygon_owner.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5::terrain;
namespace {
using Bytes=std::vector<std::uint8_t>;
std::size_t checks=0,compared=0;
void check(bool ok,const char *message){++checks;if(!ok)throw std::runtime_error(message);}
void put(Bytes &out,std::uint32_t value){
    for(unsigned i=0;i<4;++i)out.push_back(std::uint8_t(value>>(8*i)));
}
void blob(Bytes &out,std::span<const std::uint8_t> bytes){
    put(out,static_cast<std::uint32_t>(bytes.size()));
    out.insert(out.end(),bytes.begin(),bytes.end());
}
struct Reader {
    std::span<const std::uint8_t> data;
    std::span<const std::uint8_t> take(std::size_t size){
        check(size<=data.size(),"Truncated original plot-slot oracle");
        const auto value=data.first(size);data=data.subspan(size);return value;
    }
    std::uint32_t word(){
        const auto bytes=take(4);std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[i])<<(8*i);
        return value;
    }
    Bytes blob(){const auto bytes=take(word());return Bytes(bytes.begin(),bytes.end());}
};
Bytes snapshot(const ScenePolygonPlotStore &store){
    Bytes result;put(result,store.free_head());
    put(result,static_cast<std::uint32_t>(store.active_count()));
    put(result,static_cast<std::uint32_t>(store.slots().size()));
    Bytes raw;for(const auto &slot:store.slots())raw.insert(raw.end(),slot.begin(),slot.end());
    blob(result,raw);return result;
}
void compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected){
    compared+=expected.size();check(actual.size()==expected.size(),"Plot-slot snapshot size");
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if(mismatch.first!=actual.end())
        throw std::runtime_error("Plot-slot state byte "+
            std::to_string(mismatch.first-actual.begin()));
    ++checks;
}
void run(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open plot-slot oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PSL001",
        "Wrong plot-slot oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=20,"Plot-slot case count");
    for(unsigned ci=0;ci<cases;++ci){
        const auto body=oracle.blob();Reader scenario{body};
        const auto precision=scenario.word(),steps=scenario.word();
        check((precision==24||precision==53)&&steps>0&&steps<=20,
            "Invalid plot-slot scenario");
        const auto initial=scenario.blob();Reader start{initial};
        check(start.word()==ScenePolygonPlotStore::absent,
            "Controlled plot slot source has a free head");
        const auto active=start.word(),length=start.word();
        const auto records=start.blob();
        check(active==length&&records.size()==length*144&&start.data.empty(),
            "Invalid initial plot-slot vector");
        ScenePolygonPlotStore store(records);
        compare(snapshot(store),initial);
        for(unsigned si=0;si<steps;++si){
            const auto operation=scenario.word(),value=scenario.word();
            const auto raw=scenario.blob();
            std::uint32_t returned=0xffffffffu;
            if(operation==1){check(raw.empty(),"Removal has source plot bytes");store.release(value);}
            else if(operation==2){
                check(raw.size()==144,"Insertion has invalid plot bytes");
                ScenePolygonPlotBytes record{};
                std::copy(raw.begin(),raw.end(),record.begin());
                returned=store.insert(record);
                check(std::equal(record.begin(),record.end(),raw.begin()),
                    "Plot insertion mutated source record");
                const auto coords=store.plot_coordinates(returned);
                check(coords[0]==static_cast<std::int16_t>(
                    std::uint16_t(raw[16])|(std::uint16_t(raw[17])<<8))&&
                    coords[1]==static_cast<std::int16_t>(
                    std::uint16_t(raw[18])|(std::uint16_t(raw[19])<<8)),
                    "Plot coordinates not retained from source event");
            }else throw std::runtime_error("Unknown plot-slot operation");
            check(returned==scenario.word(),"Original plot slot index differs");
            compare(snapshot(store),scenario.blob());
        }
        check(scenario.data.empty(),"Trailing plot-slot case");
    }
    check(oracle.data.empty(),"Trailing plot-slot oracle");
    std::cout<<"Original plot-slot cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void run_events(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open plot-event oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PEV001",
        "Wrong plot-event oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=20,"Plot-event case count");
    const auto word_at=[](std::span<const std::uint8_t> bytes,std::size_t offset){
        std::uint32_t value=0;
        for(unsigned byte=0;byte<4;++byte)value|=std::uint32_t(bytes[offset+byte])<<(8*byte);
        return value;
    };
    const auto signed_at=[](std::span<const std::uint8_t> bytes,std::size_t offset){
        return static_cast<std::int16_t>(std::uint16_t(bytes[offset])|
            (std::uint16_t(bytes[offset+1])<<8));
    };
    for(unsigned ci=0;ci<cases;++ci){
        const auto precision=oracle.word(),scene_identity=oracle.word(),
            source_gate=oracle.word(),variation_count=oracle.word(),
            random_value=oracle.word(),handle=oracle.word(),
            expected_row=oracle.word(),expected_selector=oracle.word(),
            expected_variant=oracle.word(),expected_draws=oracle.word();
        const auto tile_shift=static_cast<std::int32_t>(oracle.word());
        check(precision==24||precision==53,"Invalid plot-event precision");
        const auto raw=oracle.blob(),expected=oracle.blob();
        check(raw.size()==40&&expected.size()==144,"Invalid plot-event record size");
        ScenePolygonPlotSourceEvent event{};
        event.identity=word_at(raw,4);event.gate=word_at(raw,8);
        event.style=raw[12];event.first_flags=word_at(raw,16);
        event.second_flags=word_at(raw,20);
        event.x=signed_at(raw,24);event.y=signed_at(raw,26);
        event.selector=raw[32];event.variant=raw[36];
        check(event.gate==source_gate,"Original plot-event source gate differs");
        unsigned draws=0,selections=0;
        const auto record=make_scene_polygon_plot_record(event,scene_identity,
            variation_count,[&]{++draws;return random_value;},
            [&](std::uint32_t row,std::uint8_t selector,std::uint8_t variant){
                check(row==expected_row&&selector==expected_selector&&
                    variant==expected_variant,"Original material request differs");
                ++selections;return handle;
            });
        check(draws==expected_draws&&selections==1,"Original plot-event callback count differs");
        compare(record,expected);
        ScenePolygonPlotStore store;
        std::vector<std::uint16_t> tiles(8*256,0xffffu);
        ScenePolygonOwnerGrid grid{};grid.width=8;grid.height=8;
        grid.tile_ids=tiles;grid.plot_store=&store;
        for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
            for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
        grid.tile_matrix[12]=tile_shift;
        ScenePolygonOwner owner(grid);
        const std::array<std::int32_t,3> position{event.x,event.y,
            -std::int32_t(event.x)-event.y};
        check(owner.insert_cell(position,record)==0&&store.active_count()==1,
            "Original plot event did not bind tile zero");
        compare(store.slots()[0],expected);
        ScenePolygonEvent polygon{};polygon.position=position;polygon.polygon_id=ci+1;
        polygon.chunk={0,1,1,std::vector<Point2>(16,{1,1})};
        check(owner.accept(polygon)==ScenePolygonEventResult::completed&&
            owner.plot(0).size()==1,"Plot-event tile rejected subsequent polygon points");
    }
    check(oracle.data.empty(),"Trailing plot-event oracle");
    std::cout<<"Original plot-event cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void run_replacements(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open plot-replacement oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PRP001",
        "Wrong plot-replacement oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=20,"Plot-replacement case count");
    const auto word_at=[](std::span<const std::uint8_t> bytes,std::size_t offset){
        std::uint32_t value=0;
        for(unsigned byte=0;byte<4;++byte)value|=std::uint32_t(bytes[offset+byte])<<(8*byte);
        return value;
    };
    for(unsigned ci=0;ci<cases;++ci){
        const auto precision=oracle.word(),occupied=oracle.word(),gate_matches=oracle.word(),
            variations=oracle.word();
        const auto shift=std::bit_cast<std::int32_t>(oracle.word());
        const auto action=oracle.word(),tile=oracle.word(),expected_dirty=oracle.word(),
            expected_random=oracle.word(),scene_identity=oracle.word(),scene_gate=oracle.word(),
            material_handle=oracle.word(),random_value=oracle.word(),
            material_row=oracle.word(),material_selector=oracle.word(),
            material_variant=oracle.word();
        check((precision==24||precision==53)&&occupied<=1&&gate_matches<=1&&
            action<=3&&expected_dirty<=1&&expected_random<=1,
            "Invalid plot-replacement scenario header");
        const auto raw=oracle.blob(),initial=oracle.blob(),expected=oracle.blob();
        check(raw.size()==40,"Invalid plot-replacement event size");
        Reader start{initial};
        check(start.word()==ScenePolygonPlotStore::absent,"Initial replacement free head");
        const auto active=start.word(),length=start.word();
        const auto records=start.blob();
        check(active==length&&length==8&&records.size()==length*144&&
            start.data.empty(),"Invalid original replacement plot vector");
        ScenePolygonPlotStore store(records);
        compare(snapshot(store),initial);
        ScenePolygonPlotSourceEvent event{};
        event.identity=word_at(raw,4);event.gate=word_at(raw,8);
        event.style=raw[12];event.first_flags=word_at(raw,16);
        event.second_flags=word_at(raw,20);
        event.x=static_cast<std::int16_t>(std::uint16_t(raw[24])|
            (std::uint16_t(raw[25])<<8));
        event.y=static_cast<std::int16_t>(std::uint16_t(raw[26])|
            (std::uint16_t(raw[27])<<8));
        event.selector=raw[32];event.variant=raw[36];
        check((event.gate==scene_gate)==bool(gate_matches),
            "Original replacement gate input differs");
        std::vector<std::uint16_t> tiles(8*256,0xffffu);
        const auto column=std::int32_t(event.x)+shift+event.y/2;
        check(event.y>=0&&event.y<8&&column>=0&&column<8,
            "Original replacement tile outside controlled grid");
        if(occupied)tiles[static_cast<std::size_t>(event.y)*256+column]=1;
        ScenePolygonOwnerGrid grid{};grid.width=8;grid.height=8;
        grid.tile_ids=tiles;grid.plot_store=&store;
        for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
            for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
        grid.tile_matrix[12]=shift;
        ScenePolygonOwner owner(grid);
        unsigned dirty=0,draws=0,materials=0,notifications=0;
        const auto result=owner.apply_plot_source_event(event,scene_identity,
            scene_gate,variations,[&]{++draws;return random_value;},
            [&](std::uint32_t row,std::uint8_t selector,std::uint8_t variant){
                check(row==material_row&&selector==material_selector&&
                    variant==material_variant,"Replacement material request differs");
                ++materials;return material_handle;
            },identity_matrix(),{0.f,0.f},
            [](std::uint32_t){throw std::runtime_error("Unexpected replacement decal handle");},
            [](std::uint16_t,std::uint32_t,std::uint32_t,bool,std::span<const Point2>){
                throw std::runtime_error("Unexpected replacement mask callback");
            },[&](std::array<float,4>){++dirty;},
            [&](std::uint16_t published){
                check(published==tile,"Replacement notification tile differs");
                ++notifications;
            });
        check(static_cast<unsigned>(result.action)==action&&result.tile_id==tile&&
            dirty==expected_dirty&&draws==expected_random&&
            materials==unsigned(action>=2)&&notifications==unsigned(action>=2),
            "Original plot replacement lifecycle differs");
        compare(snapshot(store),expected);
        ScenePolygonEvent polygon{};
        polygon.position={event.x,event.y,-std::int32_t(event.x)-event.y};
        polygon.polygon_id=ci+1;
        polygon.chunk={0,1,1,std::vector<Point2>(16,{1,1})};
        check(owner.accept(polygon)==(action>=2?
            ScenePolygonEventResult::completed:ScenePolygonEventResult::rejected),
            "Replacement did not publish or clear the polygon-event tile");
    }
    check(oracle.data.empty(),"Trailing plot-replacement oracle");
    std::cout<<"Original plot-replacement cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void run_material_lookup(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open material lookup oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PML001",
        "Wrong material lookup oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=40,"Material lookup case count");
    for(unsigned ci=0;ci<cases;++ci){
        const auto precision=oracle.word(),row=oracle.word();
        const auto selector=std::bit_cast<std::int32_t>(oracle.word());
        const auto variant=oracle.word(),expected=oracle.word();
        const auto raw=oracle.blob();
        check((precision==24||precision==53)&&raw.size()==40*4&&row*5<40,
            "Invalid original material lookup table");
        std::array<std::uint32_t,40> words{};
        for(unsigned index=0;index<words.size();++index)
            for(unsigned byte=0;byte<4;++byte)
                words[index]|=std::uint32_t(raw[index*4+byte])<<(byte*8);
        // 637F30 receives but does not read the third (variant) argument.
        (void)variant;
        Bytes actual,reference;
        put(actual,resolve_scene_polygon_material_id(
            std::span<const std::uint32_t>(words).subspan(row*5),selector));
        put(reference,expected);compare(actual,reference);
    }
    check(oracle.data.empty(),"Trailing material lookup oracle");
    std::cout<<"Original material lookup cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void run_material_rows(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open material rows oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PMR001",
        "Wrong material rows oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=40,"Material rows case count");
    for(unsigned ci=0;ci<cases;++ci){
        const auto precision=oracle.word(),farm=oracle.word(),wheat=oracle.word(),count=oracle.word();
        check((precision==24||precision==53)&&count<=32,"Invalid material rows scenario");
        const auto raw=oracle.blob();check(raw.size()==80,"Invalid initial material rows");
        std::array<std::uint32_t,20> words{};
        for(unsigned index=0;index<words.size();++index)
            for(unsigned byte=0;byte<4;++byte)
                words[index]|=std::uint32_t(raw[index*4+byte])<<(byte*8);
        std::vector<ScenePolygonMaterialSource> sources;
        for(unsigned index=0;index<count;++index)
            sources.push_back({oracle.word(),oracle.word(),oracle.word(),oracle.word()});
        const auto expected=oracle.blob();check(expected.size()==80,"Invalid final material rows");
        populate_scene_polygon_material_rows(words,farm,sources);
        Bytes actual;for(auto value:words)put(actual,value);
        compare(actual,expected);
        // The second art-definition query initializes owner identity, but does
        // not influence the material-row producer's table writes.
        (void)wheat;
    }
    check(oracle.data.empty(),"Trailing material rows oracle");
    std::cout<<"Original material rows cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void run_special_improvement_materials(const char *path){
    std::ifstream in(path,std::ios::binary);
    check(bool(in),"Cannot open special improvement materials oracle");
    Bytes data((std::istreambuf_iterator<char>(in)),{});
    check(data.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(data.data()),8)=="C5PSI001",
        "Wrong special improvement materials oracle");
    Reader oracle{std::span(data).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=40,"Special improvement case count");
    for(unsigned ci=0;ci<cases;++ci){
        const auto precision=oracle.word();
        const std::array<std::uint32_t,3> ids{oracle.word(),oracle.word(),oracle.word()};
        const auto count=oracle.word();
        check((precision==24||precision==53)&&count<=32,
            "Invalid special improvement scenario");
        const auto raw=oracle.blob();check(raw.size()==84,"Invalid initial special improvement rows");
        std::array<std::uint32_t,21> words{};
        for(unsigned index=0;index<words.size();++index)
            for(unsigned byte=0;byte<4;++byte)
                words[index]|=std::uint32_t(raw[index*4+byte])<<(byte*8);
        std::vector<ScenePolygonMaterialSource> sources;
        for(unsigned index=0;index<count;++index)
            sources.push_back({oracle.word(),oracle.word(),oracle.word(),oracle.word()});
        const auto expected=oracle.blob();
        check(expected.size()==84,"Invalid final special improvement rows");
        populate_scene_special_improvement_material_rows(words,ids,sources);
        Bytes actual;for(auto value:words)put(actual,value);
        compare(actual,expected);
    }
    check(oracle.data.empty(),"Trailing special improvement oracle");
    std::cout<<"Original special improvement cases="<<cases<<" checks="<<checks
             <<" exact bytes="<<compared<<'\n';
}
void smoke(){
    ScenePolygonPlotStore store;
    ScenePolygonPlotBytes source{};source[16]=7;source[18]=9;
    check(store.insert(source)==0&&store.plot_coordinates(0)==std::array<std::int16_t,2>{7,9},
        "New plot lost source coordinates");
    store.release(0);
    check(store.free_head()==0&&store.insert(source)==0&&store.free_head()==ScenePolygonPlotStore::absent,
        "Freed plot was not reused");
}
void owner_lifecycle(){
    ScenePolygonPlotBytes first{};
    std::vector<std::uint8_t> raw(first.begin(),first.end());
    ScenePolygonPlotStore store(raw);
    std::vector<std::uint16_t> tiles(256,0xffffu);tiles[0]=0;
    ScenePolygonOwnerGrid grid{};grid.width=1;grid.height=1;
    grid.tile_ids=tiles;grid.plot_store=&store;
    for(auto *matrix:{&grid.bounds_matrix,&grid.tile_matrix})
        for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
    ScenePolygonOwner owner(grid);
    std::vector<std::array<float,4>> regions;
    const auto remove=[&]{return owner.remove_cell_using_plot({0,0,0},
        identity_matrix(),{32,24},
        [](std::uint32_t){throw std::runtime_error("Unexpected plot-slot decal handle");},
        [](std::uint16_t,std::uint32_t,std::uint32_t,bool,std::span<const Point2>){},
        [&](std::array<float,4> region){regions.push_back(region);});};
    check(remove().found&&store.free_head()==0&&store.active_count()==0,
        "Owner removal did not release the source plot slot");
    ScenePolygonPlotBytes next{};next[16]=7;next[18]=8;
    check(owner.insert_cell({0,0,0},next)==0&&
        store.plot_coordinates(0)==std::array<std::int16_t,2>{7,8},
        "Owner did not rebind the reused plot slot");
    ScenePolygonEvent event{};event.position={0,0,0};event.polygon_id=1;
    event.chunk={0,1,1,std::vector<Point2>(16,{1,1})};
    check(owner.accept(event)==ScenePolygonEventResult::completed&&owner.plot(0).size()==1,
        "New source plot rejected its polygon event");
    check(remove().found&&regions.size()==2&&regions[1][0]>regions[0][0]&&
        regions[1][1]>regions[0][1]&&store.free_head()==0,
        "Dirty rectangle did not use the reused plot's retained coordinates");
}
}
int main(int argc,char **argv){
    try{
        check(argc>=1&&argc<=7,
            "Usage: civ5_scene_polygon_plot_store_tests [slots.bin [event.bin [replace.bin [material.bin [rows.bin [special.bin]]]]]]");
        smoke();owner_lifecycle();if(argc>=2)run(argv[1]);
        if(argc>=3)run_events(argv[2]);if(argc>=4)run_replacements(argv[3]);
        if(argc>=5)run_material_lookup(argv[4]);if(argc>=6)run_material_rows(argv[5]);
        if(argc==7)run_special_improvement_materials(argv[6]);
        std::cout<<"PASS plot-slot checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){
        std::cerr<<"FAIL plot slots: "<<e.what()<<'\n';return 1;
    }
}
