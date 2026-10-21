#include "civ5/terrain/terrain_road_grid.hpp"
#include "civ5/terrain/scene_command_queue.hpp"
#include "civ5/terrain/scene_polygon_owner.hpp"
#include "civ5/terrain/scene_mask.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks = 0, compared_bytes = 0, snapshots = 0, submissions = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "Invalid road input accepted");
}
void put(Bytes &b, std::uint32_t v, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) b.push_back(static_cast<std::uint8_t>(v >> (i*8)));
}
void real(Bytes &b, float v) { put(b, std::bit_cast<std::uint32_t>(v)); }
void blob(Bytes &b, const Bytes &raw) {
    put(b, static_cast<std::uint32_t>(raw.size())); b.insert(b.end(), raw.begin(), raw.end());
}
struct Reader {
    std::ifstream in;
    bool regions=false;
    explicit Reader(const char *path) : in(path, std::ios::binary) {
        check(bool(in), "Cannot open road oracle");
        char magic[8]{}; check(bool(in.read(magic, 8)), "Missing road signature");
        regions=std::string_view(magic,8)=="C5RDR001";
        check(regions || std::string_view(magic, 8) == "C5RDG001", "Wrong road oracle");
    }
    std::uint32_t word() {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i) {
            const auto c = in.get(); check(c != std::char_traits<char>::eof(), "Truncated road oracle");
            v |= std::uint32_t(static_cast<unsigned char>(c)) << (i*8);
        }
        return v;
    }
    float real() { return std::bit_cast<float>(word()); }
    Bytes blob() {
        const auto n = word(); check(n <= 4*1024*1024, "Excessive road blob");
        Bytes b(n);
        if (n) check(bool(in.read(reinterpret_cast<char *>(b.data()), n)), "Truncated road blob");
        return b;
    }
};
TerrainDecalPalette palette(std::uint32_t first, std::uint32_t end) {
    TerrainDecalPalette p; p.ranges[0] = {first, end};
    for (std::uint32_t i = 0; i < std::max(first+1, end); ++i)
        p.records.push_back({float(i)*.1f, i+100, i+200, i+300});
    return p;
}
struct Observation {
    std::vector<std::uint32_t> removed;
    Bytes packets;
    std::uint32_t count = 0;
    std::uint32_t submit(std::size_t cell, std::uint32_t selected,
        const TerrainDecalGeometry &mesh, const TerrainRandom &random) {
        const auto handle = 0x50000000u+static_cast<std::uint32_t>(cell)*32+selected;
        for (auto v : {static_cast<std::uint32_t>(cell), selected, random.state(),
                       static_cast<std::uint32_t>(random.calls()), handle}) put(packets, v);
        Bytes vertices, indices;
        for (auto v : mesh.vertices) for (float f : {v.x, v.y, v.u, v.v}) real(vertices, f);
        for (auto i : mesh.indices) put(indices, i, 2);
        blob(packets, vertices); blob(packets, indices);
        ++count; ++submissions;
        return handle;
    }
};
Bytes snapshot(const TerrainRoadGrid &g, const TerrainRandom &random, const Observation &o) {
    Bytes b, cells;
    real(b, g.span_x); real(b, g.span_y);
    for (auto f : g.bounds) real(b, f);
    for (const auto &cell : g.cells) {
        for (auto w : cell.geometry) put(cells, w);
        put(cells, cell.handle); put(cells, cell.dirty, 1);
        for (auto r : cell.reserved) put(cells, r, 1);
    }
    blob(b, cells);
    for (const auto &members : g.members) {
        put(b, static_cast<std::uint32_t>(members.size()));
        for (auto i : members) put(b, i);
    }
    for (const auto &s : g.segments) {
        for (auto f : s.bounds) real(b, f);
        put(b, s.group); put(b, s.cell, 2); put(b, s.type, 1); put(b, s.flags, 1);
        for (auto p : s.points) { real(b, p.x); real(b, p.y); }
    }
    put(b, random.state()); put(b, static_cast<std::uint32_t>(random.calls()));
    put(b, static_cast<std::uint32_t>(o.removed.size()));
    for (auto handle : o.removed) put(b, handle);
    put(b, o.count); b.insert(b.end(), o.packets.begin(), o.packets.end());
    return b;
}
void guards() {
    rejects([] { make_terrain_road_grid(0, 1, {0, 0, 1, 1}); });
    rejects([] { make_terrain_road_grid(256, 256, {0, 0, 1, 1}); });
    rejects([] { make_terrain_road_grid(1, 1, {0, 0, 0, 1}); });
    rejects([] { make_terrain_road_grid(1, 1, {0, 0, 1, std::numeric_limits<float>::infinity()}); });
    WorldLayout world{};
    world.source_grid.width = 10; world.source_grid.height = 8;
    world.worker_grid.width = 14; world.worker_grid.height = 12;
    world.bounds = {-24, -12, -100, 24, 12, 100, 0, 0, 0};
    auto g = make_terrain_road_grid(world);
    check(g.width == 20 && g.height == 16 && g.cells.size() == 320 && g.span_x == 48 && g.span_y == 24,
          "World road grid must use source dimensions and XY bounds");
    g.segments.push_back({{{-4, -2}, {0, 0}, {4, 2}}, 0, 0});
    g.junctions = {{}};
    TerrainRandom random(1); Observation o;
    const auto initial = snapshot(g, random, o);
    rejects([&] { publish_terrain_road_segment(g, 1, 0, 0, 0); });
    rejects([&] { shift_terrain_road_segment(g, 1, 0); });
    g.junctions[0].push_back({42, true, 0});
    rejects([&] { publish_terrain_road_segment(g, 0, 0, 0, 0); });
    g.junctions[0].clear();
    check(snapshot(g, random, o) == initial, "Invalid publication changed state");
    auto invalid = g;
    invalid.segments[0].points[1].x = 10000;
    const auto before = snapshot(invalid, random, o);
    rejects([&] { publish_terrain_road_segment(invalid, 0, 0, 0, 0); });
    check(snapshot(invalid, random, o) == before, "Out-of-storage midpoint partially published");
    invalid = g; invalid.segments[0].cell = 0;
    rejects([&] { publish_terrain_road_segment(invalid, 0, 0, 0, 0); });
    invalid = g; invalid.bounds[2] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { shift_terrain_road_segment(invalid, 0, 0); });
    // Suppressed links are not dereferenced by the original.
    g.junctions[0].push_back({42, true, 1});
    publish_terrain_road_segment(g, 0, 4, 7, 128);
    const auto cell = g.segments[0].cell;
    std::vector<std::uint8_t> flags(64*64, 0x10);
    const TerrainDecalMapView map{64, 64, flags, 32, 32};
    auto library = palette(0, 1);
    const TerrainDecalRemove remove = [&](auto, auto h) { o.removed.push_back(h); };
    const TerrainDecalSubmit submit = [&](auto i, auto selected, const auto &record, const auto &mesh) {
        check(&record == &library.records[selected], "Palette record lifetime");
        check(g.cells[i].dirty && g.cells[i].handle == absent_terrain_decal, "Pending submission state");
        return o.submit(i, selected, mesh, random);
    };
    rejects([&] { refresh_terrain_road_decals(g, map, library, random, remove, {}); });
    check(g.cells[cell].dirty && random.calls() == 0, "Invalid refresh changed pending state");
    refresh_terrain_road_decals(g, map, library, random, remove, submit);
    check(o.count == 1 && o.removed.empty() && random.calls() == 0 && !g.cells[cell].dirty,
          "Published road did not reach singleton submission");
    refresh_terrain_road_decals(g, map, library, random, remove, submit);
    check(o.count == 1, "Clean road grid refreshed twice");
    publish_terrain_road_segment(g, 0, 9, 2, 3);
    refresh_terrain_road_decals(g, map, library, random, remove, submit);
    check(o.count == 2 && o.removed.size() == 1, "Republished road did not replace its resource");
    const auto clean=snapshot(g,random,o);
    rejects([&]{dirty_terrain_road_region(g,{-100,-100,0,0},{0,0});});
    rejects([&]{dirty_terrain_road_region(g,{0,0,1e20f,1e20f},{0,0});});
    rejects([&]{dirty_terrain_road_region(g,{0,0,1e7f,1e7f},{0,0});});
    check(snapshot(g,random,o)==clean,"Invalid region partially dirtied cells");
    dirty_terrain_road_region(g,{-24,-12,24,12},{24,12});
    check(std::all_of(g.cells.begin(),g.cells.end(),[](const auto &v){return v.dirty==1;}),"Full inclusive region missed cells");
}
void polygon_region_integration() {
    auto g=make_terrain_road_grid(4,3,{-32,-24,32,24});
    g.segments.push_back({{{-6,-5},{0,0},{6,5}},0,0});g.junctions={{}};
    publish_terrain_road_segment(g,0,0,0,0);
    SceneMask mask(128,128);TerrainRandom random(17);const auto library=palette(0,1);
    const Point2 offset{32,24};const auto precision=CoastBoundsPrecision::double_or_extended;
    const auto reciprocal=[](float v){return 1.f/v;};
    const std::array<Point2,4> polygon{{{-12,-12},{12,-12},{12,12},{-12,12}}};
    const std::array<float,4> scene_region{-16,-16,16,16};
    unsigned submitted=0,removed=0;
    const TerrainDecalRemove remove=[&](auto,auto handle){check(handle==123,"Unexpected admitted decal handle");++removed;};
    const TerrainDecalSubmit submit=[&](auto,auto,const auto &,const auto &mesh){
        check(mesh.vertices.size()==6 && mesh.indices.size()==12,"Region did not retain admitted strip");++submitted;return 123u;
    };
    SceneCommandQueue commands;
    commands.register_handler(0x21,[&](std::span<const std::uint8_t> payload){
        dispatch_terrain_road_region_payload(payload,g,mask.decal_view(offset),library,
            random,remove,submit);
    });
    const auto dispatch=[&]{
        commands.enqueue(make_terrain_road_region_command(scene_region));
        check(commands.drain()==1,"Scene queue lost road region command");
    };
    auto wrong=make_terrain_road_region_command(scene_region);wrong[4]=0x20;
    rejects([&]{dispatch_terrain_road_region_command(wrong,g,mask.decal_view(offset),
        library,random,remove,submit);});
    mask.polygon(polygon,offset,true,precision,reciprocal);
    dispatch();
    check(submitted==1 && removed==0 && random.calls()==0,"Polygon-to-region admission failed");
    mask.polygon(polygon,offset,false,precision,reciprocal);
    dispatch();
    check(submitted==1 && removed==1 && g.cells[g.segments[0].cell].handle==absent_terrain_decal,
        "Region retained decal after polygon admission cleared");
    mask.polygon(polygon,offset,true,precision,reciprocal);
    dispatch();
    check(submitted==2 && removed==1 && random.calls()==0,"Restored polygon did not resubmit decal");
}
void polygon_owner_command_integration() {
    auto grid=make_terrain_road_grid(4,3,{-32,-24,32,24});
    grid.segments.push_back({{{-6,-5},{0,0},{6,5}},0,0});grid.junctions={{}};
    publish_terrain_road_segment(grid,0,0,0,0);
    SceneMask mask(128,128);TerrainRandom random(17);const auto library=palette(0,1);
    const Point2 offset{32,24};
    const std::array<Point2,4> polygon{{{-12,-12},{12,-12},{12,12},{-12,12}}};
    std::vector<std::uint16_t> tiles(3*256,0xffffu);tiles[0]=0;
    ScenePolygonOwnerGrid source{};source.width=4;source.height=3;source.tile_ids=tiles;
    for(auto *matrix:{&source.bounds_matrix,&source.tile_matrix})
        for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
    ScenePolygonOwner owner(source);
    ScenePolygonEvent event{};event.position={0,0,0};event.polygon_id=1;
    event.chunk={0,1,4,std::vector<Point2>(16,{0,0})};
    std::copy(polygon.begin(),polygon.end(),event.chunk.points.begin());
    check(owner.accept(event,[&](std::uint16_t tile,std::uint32_t slot,
            std::uint32_t record,std::span<const Point2> points){
        check(tile==0&&slot==0&&record==0&&points.size()==4,
            "Scene polygon event did not reach mask owner");
        mask.polygon(points,offset,true,CoastBoundsPrecision::double_or_extended,
            [](float v){return 1.f/v;});
    })==ScenePolygonEventResult::completed,"Scene polygon owner rejected controlled event");
    unsigned submitted=0,removed=0;
    const TerrainDecalRemove remove=[&](auto,auto handle){
        check(handle==123,"Unexpected scene-command decal handle");++removed;
    };
    const TerrainDecalSubmit submit=[&](auto,auto,const auto &,const auto &mesh){
        check(mesh.vertices.size()==6&&mesh.indices.size()==12,
            "Scene command changed admitted strip");++submitted;return 123u;
    };
    refresh_terrain_road_decals(grid,mask.decal_view(offset),library,random,remove,submit);
    check(submitted==1&&removed==0,"Source polygon did not admit road strip");
    SceneCommandQueue commands;
    commands.register_handler(0x21,[&](std::span<const std::uint8_t> payload){
        dispatch_terrain_road_region_payload(payload,grid,mask.decal_view(offset),
            library,random,remove,submit);
    });
    const ScenePolygonDirtyProjection projection{{0,0},identity_matrix(),{32,24}};
    const auto result=owner.remove_cell({0,0,0},projection,
        [](std::uint32_t){throw std::runtime_error("Unexpected source decal handle");},
        [&](std::uint16_t tile,std::uint32_t slot,std::uint32_t record,
            bool setting,std::span<const Point2> points){
            check(tile==0&&slot==0&&record==0&&!setting&&points.size()==4,
                "Source removal changed polygon mask callback");
            mask.polygon(points,offset,setting,CoastBoundsPrecision::double_or_extended,
                [](float v){return 1.f/v;});
        },[&](std::array<float,4> bounds){
            commands.enqueue(make_terrain_road_region_command(bounds));
        });
    check(result.found&&result.removed==1&&commands.bank_bytes(0)==128&&
        submitted==1&&removed==0,"Polygon removal did not defer road refresh");
    check(commands.drain()==1&&submitted==1&&removed==1&&
        grid.cells[grid.segments[0].cell].handle==absent_terrain_decal,
        "Scene polygon removal command did not clear road decal");
}
void oracle(const char *path) {
    Reader r(path);
    const auto cases = r.word(); check(cases <= 1000, "Excessive road cases");
    for (unsigned ci = 0; ci < cases; ++ci) {
        unsigned step = 0;
        try {
            const auto width = r.word(), height = r.word();
            std::array<float, 4> bounds;
            for (auto &f : bounds) f = r.real();
            auto g = make_terrain_road_grid(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height), bounds);
            const auto first = r.word(), end = r.word(), seed = r.word();
            check(first <= 100 && end <= 100, "Excessive road palette");
            const auto library = palette(first, end);
            auto flags = r.blob(); check(flags.size() == 4096, "Wrong controlled mask");
            const auto ox = r.real(), oy = r.real();
            const TerrainDecalMapView map{64, 64, flags, ox, oy};
            const auto segments = r.word(); check(segments <= 10000, "Excessive segments");
            for (unsigned i = 0; i < segments; ++i) {
                const auto begin = r.word(), last = r.word(), count = r.word();
                check(begin <= 65535 && last <= 65535 && count <= 20000, "Excessive segment points/nodes");
                TerrainRoadSegment s{{}, static_cast<std::uint16_t>(begin), static_cast<std::uint16_t>(last)};
                for (unsigned j = 0; j < count; ++j) {
                    const auto x = r.real(), y = r.real(); s.points.push_back({x, y});
                }
                g.segments.push_back(std::move(s));
            }
            const auto nodes = r.word(); check(nodes <= 65535, "Excessive junctions");
            for (unsigned i = 0; i < nodes; ++i) {
                auto &node = g.junctions.emplace_back();
                const auto count = r.word(); check(count <= 4, "Excessive junction links");
                for (unsigned j = 0; j < count; ++j) {
                    const auto packed = r.word();
                    node.push_back({packed & 0x0fffffffu, (packed & 0x10000000u) != 0,
                        static_cast<std::uint8_t>((packed >> 29) & 3)});
                }
            }
            TerrainRandom random(seed); Observation o;Bytes region_writes;
            const auto compare = [&] {
                const auto expected = r.blob(), actual = snapshot(g, random, o);
                compared_bytes += expected.size(); ++snapshots;
                if (expected != actual) {
                    const auto diff = std::mismatch(expected.begin(), expected.end(), actual.begin(), actual.end());
                    throw std::runtime_error("Snapshot mismatch at byte "+std::to_string(diff.first-expected.begin())+
                        ", expected size "+std::to_string(expected.size())+", actual "+std::to_string(actual.size()));
                }
                check(true, "Native road snapshot");
                if(r.regions) {
                    const auto expected_writes=r.blob();compared_bytes+=expected_writes.size();
                    check(region_writes==expected_writes,"Original dirty write order differs");
                }
            };
            compare();
            const auto actions = r.word(); check(actions <= 10000, "Excessive road actions");
            for (step = 1; step <= actions; ++step) {
                const auto op = r.word(), index = r.word(), a = r.word(), b = r.word(), c = r.word();
                if (op == 0) shift_terrain_road_segment(g, index, std::bit_cast<std::int32_t>(a));
                else if (op == 1) publish_terrain_road_segment(g, index, a, static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(c));
                else if (op == 2) refresh_terrain_road_decals(g, map, library, random,
                    [&](auto, auto handle) { o.removed.push_back(handle); },
                    [&](auto cell, auto selected, const auto &record, const auto &mesh) {
                        check(&record == &library.records[selected], "Wrong road palette record");
                        return o.submit(cell, selected, mesh, random);
                    });
                else if (op == 3) g.segments.at(index).points.at(a) = {std::bit_cast<float>(b), std::bit_cast<float>(c)};
                else if (op == 4 || op == 5) {
                    const std::array<float,4> region{std::bit_cast<float>(index),std::bit_cast<float>(a),std::bit_cast<float>(b),std::bit_cast<float>(c)};
                    const auto observer=[&](std::size_t cell){put(region_writes,static_cast<std::uint32_t>(cell));};
                    if(op==4)dirty_terrain_road_region(g,region,{map.offset_x,map.offset_y},observer);
                    else refresh_terrain_road_region(g,region,map,library,random,
                        [&](auto,auto handle){o.removed.push_back(handle);},
                        [&](auto cell,auto selected,const auto &record,const auto &mesh){
                            check(&record==&library.records[selected],"Wrong region palette record");
                            return o.submit(cell,selected,mesh,random);
                        },observer);
                }
                else throw std::runtime_error("Unknown road operation");
                compare();
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Case "+std::to_string(ci)+", step "+std::to_string(step)+": "+e.what());
        }
    }
    check(r.in.peek() == std::char_traits<char>::eof(), "Trailing road oracle bytes");
    std::cout << cases << " original cases, ";
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_road_grid_tests [oracle.bin]");
        guards();
        polygon_region_integration();
        polygon_owner_command_integration();
        if (argc == 2) oracle(argv[1]);
        std::cout << "PASS road grid: " << checks << " checks, " << snapshots << " snapshots, "
                  << compared_bytes << " original bytes, " << submissions << " submissions\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL road grid: " << e.what() << '\n'; return 1;
    }
}
