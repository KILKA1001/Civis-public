#include "civ5/materials/terrain_decal_store.hpp"
#include "civ5/materials/terrain_decal_draw.hpp"
#include "civ5/terrain/terrain_road_grid.hpp"
#include "civ5/terrain/scene_mask.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
using Bytes = std::vector<std::uint8_t>;
std::size_t checks = 0, snapshots = 0, original_bytes = 0, operations = 0;
void check(bool ok, const char *message) { ++checks; if (!ok) throw std::runtime_error(message); }
template<class F> void rejects(F f) {
    bool caught = false; try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "Invalid store input accepted");
}
void put(Bytes &b, std::uint32_t v, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) b.push_back(static_cast<std::uint8_t>(v >> (i*8)));
}
void real(Bytes &b, float v) { put(b, std::bit_cast<std::uint32_t>(v)); }
std::uint32_t word(const Bytes &b, std::size_t at, unsigned size = 4) {
    std::uint32_t v = 0; for (unsigned i = 0; i < size; ++i) v |= std::uint32_t(b.at(at+i)) << (i*8); return v;
}
void material_bytes(Bytes &b, const TerrainDecalRecord &r) {
    real(b, r.priority); put(b, r.albedo); put(b, r.heightmap); put(b, r.specular);
}
void links(Bytes &b, const TerrainDecalLinks &r) { put(b, r.next); put(b, r.previous); put(b, r.active); }
Bytes snapshot(const TerrainDecalStore &s, std::uint32_t columns = 8, std::uint32_t rows = 8) {
    Bytes b;
    for (float v : s.domain()) real(b, v);
    real(b, s.scales().x); real(b, s.scales().y); put(b, columns); put(b, rows);
    put(b, static_cast<std::uint32_t>(s.cell_heads().size()*4)); for (auto v : s.cell_heads()) put(b, v);
    put(b, s.small_free()); put(b, s.small_live()); put(b, static_cast<std::uint32_t>(s.small_records().size()));
    for (const auto &r : s.small_records()) { links(b, r); real(b, r.center.x); real(b, r.center.y); put(b, r.batch); put(b, r.record); }
    put(b, s.large_free()); put(b, s.large_live()); put(b, s.large_head()); put(b, s.large_tail());
    put(b, static_cast<std::uint32_t>(s.large_records().size()));
    for (const auto &r : s.large_records()) { links(b, r); put(b, r.batch); put(b, r.record); }
    put(b, static_cast<std::uint32_t>(s.batches().size()));
    for (const auto &batch : s.batches()) {
        put(b, batch.free); put(b, batch.head); put(b, batch.tail); put(b, batch.live);
        put(b, batch.holes); put(b, batch.upload); put(b, batch.buffer); put(b, static_cast<std::uint32_t>(batch.records.size()));
        for (const auto &r : batch.records) {
            links(b, r); put(b, r.uploaded); put(b, r.mode); put(b, r.first_vertex); put(b, r.triangles); material_bytes(b, r.material);
            for (float v : r.bounds) real(b, v);
        }
        put(b, static_cast<std::uint32_t>(batch.vertices.size()*8));
        for (auto v : batch.vertices) for (auto h : v) put(b, h, 2);
    }
    return b;
}
TerrainDecalGeometry quad(float x = 0, float y = 0, float width = 10, float height = 8) {
    return {{{x,y,0,0},{x+width,y,1,0},{x,y+height,0,1},{x+width,y+height,1,1}}, {2,1,0,2,3,1}};
}
struct Backend : TerrainDecalBufferBackend {
    const TerrainDecalStore &store;
    Bytes events;
    std::uint32_t creates = 0;
    bool fail_create = false, fail_upload = false, null_create = false;
    explicit Backend(const TerrainDecalStore &s) : store(s) {}
    std::uint32_t create() override {
        if(fail_create)throw std::runtime_error("Create failure");
        if(null_create)return 0;
        const auto id=100+(++creates);put(events,0);put(events,id);put(events,65535);return id;
    }
    void upload(std::uint32_t id,std::span<const std::array<std::uint16_t,4>> vertices) override {
        const TerrainDecalBatch *batch=nullptr;
        for(const auto &b:store.batches())if(b.buffer==id) {check(!batch,"Duplicate buffer ID");batch=&b;}
        check(batch&&batch->upload&&batch->vertices.data()==vertices.data(),"Buffer upload source/state");
        std::uint32_t uploaded=0;
        for(const auto &r:batch->records)if(r.active&&r.uploaded)++uploaded;
        put(events,1);put(events,id);put(events,uploaded);put(events,static_cast<std::uint32_t>(vertices.size()*8));
        for(auto v:vertices)for(auto h:v)put(events,h,2);
        if(fail_upload)throw std::runtime_error("Upload failure");
    }
};
Bytes draw_bytes(const TerrainDecalDrawList &list) {
    Bytes b;put(b,list.selected);put(b,list.capacity_exhausted);put(b,static_cast<std::uint32_t>(list.draws.size()));
    for(const auto &d:list.draws) {
        put(b,d.batch);put(b,d.record);for(float f:d.constants)real(b,f);
        Bytes packet(100);
        const auto field=[&](std::size_t at,std::uint32_t value,unsigned size=4) {
            for(unsigned i=0;i<size;++i)packet[at+i]=static_cast<std::uint8_t>(value>>(i*8));
        };
        field(0,0x4e);field(4,d.index_buffer);field(8,d.vertex_buffer);field(12,d.triangles);field(16,d.first_vertex);
        field(0x18,d.albedo,2);field(0x1a,d.heightmap,2);field(0x38,2,2);field(0x60,2,1);
        b.insert(b.end(),packet.begin(),packet.end());
    }
    return b;
}
void draw_guards() {
    TerrainDecalStore s({-1024,-1024,1024,1024},8,8,37);Backend backend(s);
    const std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const auto draw=[&](std::array<float,4> bounds) {return prepare_terrain_decal_draws(s,bounds,0,131072,matrix,77);};
    rejects([&]{draw({0,0,-1,1});});rejects([&]{draw({0,0,1,std::numeric_limits<float>::infinity()});});
    rejects([&]{draw({std::numeric_limits<float>::quiet_NaN(),0,1,1});});rejects([&]{draw({2000,0,2100,1});});
    s.submit({.125f,13,29,37},quad());s.upload_pending(backend);
    const auto before=snapshot(s);const auto retained=draw({0,0,10,8});
    check(retained.draws.size()==1&&retained.selected==1&&!retained.capacity_exhausted,"Resident draw admission");
    check(retained.draws[0].constants[24]==.125f&&retained.draws[0].vertex_buffer==101,"Resident draw constants/buffer");
    const auto bytes=draw_bytes(retained);s.remove(0);s.compact(0);
    check(draw({0,0,10,8}).draws.empty()&&draw_bytes(retained)==bytes,"Draw list did not retain its own values");
    check(snapshot(s)!=before,"Draw guard fixture failed to mutate");
    s.submit({std::numeric_limits<float>::quiet_NaN(),13,29,37},quad());s.upload_pending(backend);
    const auto invalid_before=snapshot(s);rejects([&]{draw({0,0,10,8});});
    check(snapshot(s)==invalid_before,"Rejected draw changed store");
}
void guards() {
    const std::array<float, 4> domain{-1024,-1024,1024,1024};
    rejects([&] { TerrainDecalStore(domain,0,1,37); });
    rejects([&] { TerrainDecalStore(domain,1048576,2,37); });
    rejects([&] { TerrainDecalStore({0,0,0,1},1,1,37); });
    rejects([&] { TerrainDecalStore({0,0,std::numeric_limits<float>::infinity(),1},1,1,37); });
    rejects([&] { TerrainDecalStore({0,0,std::numeric_limits<float>::denorm_min(),1},1,1,37); });
    const TerrainDecalRecord material{.125f,13,29,37};
    TerrainDecalStore s(domain,8,8,37); Backend backend(s); const auto empty = snapshot(s);
    check(s.submit(material,{}) == absent_terrain_decal && snapshot(s) == empty, "Empty submission effects");
    auto oversized = quad(); oversized.indices.resize(65535,0);
    check(s.submit(material,oversized) == absent_terrain_decal && snapshot(s) == empty, "Oversized submission effects");
    rejects([&] { s.remove(absent_terrain_decal); });
    rejects([&] { s.remove(0); }); rejects([&] { s.compact(0); });
    s.upload_pending(backend);check(backend.events.empty(),"Empty store created buffers");
    auto bad = quad(); bad.indices[0] = 4;
    rejects([&] { s.submit(material,bad); });
    check(snapshot(s) == empty, "Invalid operation changed store");
    const TerrainDecalRegionRefresh fail = [](auto, auto, auto) { throw std::runtime_error("refresh failure"); };
    bool caught = false;
    try { s.submit(material,quad(),0,fail); } catch (const std::runtime_error &) { caught = true; }
    check(caught && s.batches().size() == 1 && s.batches()[0].records.empty() && !s.small_live(), "Failed submit orphaned records");
    std::vector<std::uint32_t> small,large;
    for (unsigned i = 0; i < 7; ++i) { small.push_back(s.submit(material,quad())); large.push_back(s.submit(material,quad(0,0,300,300))); }
    s.remove(small[2]); s.remove(large[2]); s.upload_pending(backend);
    for (const auto handle : {small[0],small[1],small[3],small[6],large[0],large[1],large[3],large[6]}) {
        const auto before = snapshot(s); caught = false;
        try {
            s.remove(handle,[&](auto, bool stamp, bool specular) {
                check(!s.find(handle) && !stamp && !specular, "Removal callback order/flags");
                throw std::runtime_error("remove failure");
            });
        } catch (const std::runtime_error &) { caught = true; }
        check(caught && snapshot(s) == before && s.find(handle), "Failed removal did not restore all links");
    }
    const auto old_size = s.batches()[0].vertices.size();
    s.remove(small[0]);
    check(!s.batches()[0].upload && s.batches()[0].holes && s.batches()[0].vertices.size() == old_size,
          "Removal eagerly uploaded or compacted");
    const auto removed = snapshot(s); rejects([&] { s.remove(small[0]); });
    check(snapshot(s) == removed, "Double removal changed state");
    const auto reused = s.submit(material,quad()); check(reused == small[0], "Original handle reuse lost");
    s.compact(0); check(!s.batches()[0].holes && s.batches()[0].upload && s.find(reused), "Compaction changed live handle");
    const auto before_move = snapshot(s); TerrainDecalStore moved(std::move(s));
    check(snapshot(moved) == before_move && moved.find(reused), "Move lost retained resources");
    TerrainDecalStore pending(domain,8,8,37);Backend retry(pending);
    const auto h=pending.submit(material,quad());const auto before_upload=snapshot(pending);
    for(bool null_value:{false,true}) {
        retry.null_create=null_value;retry.fail_create=!null_value;caught=false;
        try{pending.upload_pending(retry);}catch(const std::runtime_error &){caught=true;}
        check(caught&&snapshot(pending)==before_upload,"Failed buffer creation changed pending state");
    }
    retry.null_create=false;retry.fail_create=false;retry.fail_upload=true;caught=false;
    try{pending.upload_pending(retry);}catch(const std::runtime_error &){caught=true;}
    check(caught&&pending.batches()[0].buffer==101&&pending.batches()[0].upload&&!pending.find(h)->uploaded,
          "Failed upload marked records resident or lost the allocated buffer");
    retry.fail_upload=false;retry.events.clear();pending.upload_pending(retry);
    check(retry.creates==1&&pending.find(h)->uploaded&&!pending.batches()[0].upload,"Upload retry did not reuse its buffer");
    retry.events.clear();pending.upload_pending(retry);check(retry.events.empty(),"Clean buffer uploaded again");
}
void refresh_integration() {
    TerrainDecalStore store({0,0,64,64},2,2,37);Backend backend(store);
    auto grid=make_terrain_road_grid(4,3,{-32,-24,32,24});
    grid.segments.push_back({{{-6,-5},{0,0},{6,5}},0,0});grid.junctions={{}};
    publish_terrain_road_segment(grid,0,0,0,0);
    SceneMask mask(128,128);TerrainRandom random(17);
    TerrainDecalPalette palette;palette.ranges[0]={0,1};palette.records.push_back({.125f,13,29,37});
    const Point2 offset{32,24};const auto precision=CoastBoundsPrecision::double_or_extended;
    const auto reciprocal=[](float v){return 1.f/v;};
    const std::array<Point2,4> polygon{{{-12,-12},{12,-12},{12,12},{-12,12}}};
    const std::array<float,4> region{-16,-16,16,16};
    unsigned submits=0,removes=0,refreshes=0;bool fail=false;
    const TerrainDecalRegionRefresh refresh=[&](auto,bool stamp,bool specular) {
        check(!stamp&&!specular,"Road store refresh flags");
        if(fail)throw std::runtime_error("Controlled cache refresh failure");
        ++refreshes;
    };
    const TerrainDecalRemove remove=[&](auto,auto handle){store.remove(handle,refresh);++removes;};
    const TerrainDecalSubmit submit=[&](auto,auto selected,const auto &material,const auto &geometry) {
        check(selected==0&&geometry.indices.size()==12,"Road-store submission input");
        ++submits;return store.submit(material,geometry,0,refresh);
    };
    const auto run=[&]{refresh_terrain_road_region(grid,region,mask.decal_view(offset),palette,random,remove,submit);};
    mask.polygon(polygon,offset,true,precision,reciprocal);run();
    const auto cell=grid.segments[0].cell;
    const auto handle=grid.cells[cell].handle;
    check(handle==0&&store.find(handle)&&store.small_live()==1&&store.batches()[0].vertices.size()==12,
          "Polygon/region did not create an owned strip");
    store.upload_pending(backend);mask.polygon(polygon,offset,false,precision,reciprocal);
    const auto before=snapshot(store,2,2);fail=true;bool caught=false;
    try{run();}catch(const std::runtime_error &){caught=true;}
    check(caught&&snapshot(store,2,2)==before&&grid.cells[cell].handle==handle&&grid.cells[cell].dirty,
          "Failed road removal lost its pending handle");
    fail=false;run();
    check(grid.cells[cell].handle==absent_terrain_decal&&!store.find(handle)&&!store.small_live()&&
          store.batches()[0].holes&&!store.batches()[0].upload,"Cleared polygon retained a live strip");
    mask.polygon(polygon,offset,true,precision,reciprocal);run();
    check(grid.cells[cell].handle==handle&&store.find(handle)&&store.batches()[0].vertices.size()==24&&
          store.batches()[0].holes,"Restored polygon did not reuse its slot without eager compaction");
    store.compact(0);
    check(store.batches()[0].vertices.size()==12&&store.find(handle)->first_vertex==0&&
          !store.batches()[0].holes&&submits==2&&removes==1&&refreshes==3&&random.calls()==0,
          "Road-store lifecycle or selector state changed");
    check(!store.find(handle)->uploaded,"Reused record inherited the old upload bit");
    store.upload_pending(backend);check(store.find(handle)->uploaded&&backend.creates==1,"Updated road did not reuse its GPU buffer ID");
    const auto original_state=snapshot(store,2,2);
    const std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const auto draws=prepare_terrain_decal_draws(store,{0,0,64,64},0,131072,matrix,77);
    check(draws.draws.size()==1&&draws.draws[0].triangles==4&&draws.draws[0].first_vertex==0&&
          draws.draws[0].albedo==13&&draws.draws[0].heightmap==29&&draws.draws[0].vertex_buffer==101,
          "Polygon/road/store/upload did not produce the original draw packet");
    check(snapshot(store,2,2)==original_state,"Preparing road draws changed store");
}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path) : in(path,std::ios::binary) {
        check(bool(in),"Cannot open store oracle"); char magic[8]{};
        check(bool(in.read(magic,8)) && std::string_view(magic,8)=="C5DCS002","Store oracle signature");
    }
    std::uint32_t integer() {
        std::uint32_t v=0;
        for (unsigned i=0;i<4;++i) { auto c=in.get(); check(c!=std::char_traits<char>::eof(),"Truncated store oracle"); v|=std::uint32_t(static_cast<unsigned char>(c))<<(i*8); }
        return v;
    }
    float real() { return std::bit_cast<float>(integer()); }
    Bytes blob() {
        const auto n=integer();check(n<=16*1024*1024,"Excessive store oracle blob");Bytes b(n);
        if(n) check(bool(in.read(reinterpret_cast<char *>(b.data()),n)),"Truncated store blob");return b;
    }
};
void oracle(const char *path) {
    Reader r(path);const auto cases=r.integer();check(cases<=100,"Excessive store cases");
    for (unsigned c=0;c<cases;++c) {
        std::array<float,4> domain{};for(auto &v:domain)v=r.real();
        const auto columns=r.integer(),rows=r.integer(),blank=r.integer(),count=r.integer();
        check(count<=10000,"Excessive store operations");TerrainDecalStore store(domain,columns,rows,blank);Backend backend(store);
        Bytes events;std::uint32_t event_count=0;
        const TerrainDecalRegionRefresh refresh=[&](auto bounds,bool stamp,bool specular) {
            for(float f:bounds)real(events,f);put(events,stamp);put(events,specular);
            put(events,static_cast<std::uint32_t>(store.batches().size()));std::uint32_t live=0;
            for(const auto &b:store.batches())live+=b.live;
            put(events,live);put(events,store.small_live());put(events,store.large_live());++event_count;
        };
        for(unsigned op=0;op<count;++op) {
            try {
                const auto kind=r.integer();events.clear();event_count=0;++operations;
                if(kind==0) {
                    const auto expected=r.blob(),actual=snapshot(store,columns,rows);
                    if(actual!=expected) {
                        std::size_t i=0;while(i<actual.size()&&i<expected.size()&&actual[i]==expected[i])++i;
                        std::cerr<<" first differing state byte="<<i<<" sizes="<<actual.size()<<","<<expected.size()<<'\n';
                    }
                    check(actual==expected,"Store state differs from original");++snapshots;original_bytes+=expected.size();
                } else if(kind==1) {
                    const auto enabled=r.integer(),mode=r.integer();TerrainDecalRecord material;
                    material.priority=r.real();material.albedo=r.integer();material.heightmap=r.integer();material.specular=r.integer();
                    const auto source=r.blob(),ids=r.blob();check(source.size()%16==0&&ids.size()%6==0,"Malformed geometry oracle");
                    TerrainDecalGeometry mesh;
                    for(std::size_t i=0;i<source.size();i+=16)mesh.vertices.push_back({std::bit_cast<float>(word(source,i)),std::bit_cast<float>(word(source,i+4)),std::bit_cast<float>(word(source,i+8)),std::bit_cast<float>(word(source,i+12))});
                    for(std::size_t i=0;i<ids.size();i+=2)mesh.indices.push_back(static_cast<std::uint16_t>(word(ids,i,2)));
                    const auto handle=store.submit(material,mesh,static_cast<std::uint8_t>(mode),enabled?refresh:TerrainDecalRegionRefresh{});
                    check(handle==r.integer(),"Submission handle differs from original");
                    check((handle==absent_terrain_decal)==(store.find(handle)==nullptr),"Returned handle lookup");
                } else if(kind==2) {
                    const auto enabled=r.integer(),handle=r.integer();store.remove(handle,enabled?refresh:TerrainDecalRegionRefresh{});
                    check(!store.find(handle),"Removed handle still live");
                } else if(kind==3)store.compact(r.integer());
                else if(kind==4) {
                    backend.events.clear();store.upload_pending(backend);const auto expected=r.blob();
                    check(backend.events==expected,"Buffer allocation/upload calls differ from original");original_bytes+=expected.size();
                }
                else if(kind==5) { const float x=r.real(),y=r.real();check(store.spatial_cell({x,y})==r.integer(),"Spatial cell differs from original"); }
                else if(kind==6) {
                    std::array<float,4> bounds{};for(auto &v:bounds)v=r.real();
                    const auto mode=r.integer(),available=r.integer(),index=r.integer();
                    std::array<float,16> matrix{};for(auto &v:matrix)v=r.real();
                    const auto expected=r.blob(),before=snapshot(store,columns,rows);
                    const auto actual=draw_bytes(prepare_terrain_decal_draws(store,bounds,mode,available,matrix,index));
                    if(actual!=expected) {
                        std::size_t i=0;while(i<actual.size()&&i<expected.size()&&actual[i]==expected[i])++i;
                        std::cerr<<" first differing draw byte="<<i<<" sizes="<<actual.size()<<","<<expected.size()<<'\n';
                    }
                    check(actual==expected,"Draw selection/order/constants/packets differ from original");
                    check(snapshot(store,columns,rows)==before,"Draw query mutated store");original_bytes+=expected.size();
                }
                else throw std::runtime_error("Unknown oracle operation");
                if(kind==1||kind==2) {
                    const auto ec=r.integer();check(ec==event_count&&ec<=1,"Refresh event count");
                    Bytes expected;for(unsigned i=0;i<ec*10;++i)put(expected,r.integer());
                    check(events==expected,"Refresh boundary state differs from original");original_bytes+=expected.size();
                }
            } catch(const std::exception &e) {throw std::runtime_error("Case "+std::to_string(c)+" operation "+std::to_string(op)+": "+e.what());}
        }
    }
    check(r.in.peek()==std::char_traits<char>::eof(),"Trailing store oracle bytes");
}
} // namespace
int main(int argc,char **argv) {
    try {
        guards();draw_guards();refresh_integration();for(int i=1;i<argc;++i)oracle(argv[i]);
        std::cout<<"PASS terrain decal store checks="<<checks<<" operations="<<operations<<" snapshots="<<snapshots<<" originalBytes="<<original_bytes<<'\n';return 0;
    } catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
