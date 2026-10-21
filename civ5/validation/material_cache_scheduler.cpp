// Primitive original-oracle transport. No expected answers are read by this
// runner.
#include "civ5/materials/material_cache_scheduler.hpp"
#include "civ5/materials/material_cache_loading.hpp"
#include "material_cache_lifecycle_validation.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include <bit>
#include <cstring>
#include <optional>
using namespace civ5::terrain;
namespace {
using cache_lifecycle_validation::Case;
struct Runner final : MaterialCacheSchedulerBackend, MaterialCacheLoadBackend,
                      MaterialCacheLifecycleBackend, MaterialCacheTexturePool {
    std::vector<std::unique_ptr<Case>> cases;
    std::vector<MaterialCacheSchedulerPatch> patches;
    MaterialCacheFreeLists ordinary, compressed;
    MaterialCacheSchedulerSettings settings;
    bool separate = true, mutate_refresh = false;
    unsigned current = 0;
    Bytes events;
    unsigned event_count = 0;
    bool loading = false;
    std::vector<std::uint32_t> load_trace;
    Bytes snapshot(unsigned id) const {
        auto raw = cases[id]->snapshot();
        const auto &p = patches[id];
        // The explicit preparation endpoint does not touch readiness. Keep
        // legacy raw fixtures' noncanonical true bytes, including padding.
        raw[0x136] = cases[id]->entering[0x136];
        if (settings.cull)
            raw[0x134] = p.frustum_visible;
        raw[0x135] = p.submitted_view;
        for (unsigned j = 0; j < 6; ++j)
            put(raw, 0x1a0 + j * 4, std::bit_cast<std::uint32_t>(p.plane_distances[j]));
        put(raw, 0x1bc, p.priority);
        put(raw, 0x2fc, p.state->textures.specular);
        return raw;
    }
    void record(unsigned kind, std::initializer_list<std::uint32_t> args = {}) {
        ++event_count;
        word(events, kind);
        word(events, current);
        vector_words(events, std::span(args.begin(), args.size()));
        const auto raw = snapshot(current);
        events.insert(events.end(), raw.begin(), raw.end());
    }
    MaterialCacheLifecycleState &state(unsigned id) {
        current = id;
        return cases[id]->state;
    }
    std::size_t free_count(std::uint32_t width) const override { return ordinary.free_handles(width).size(); }
    std::size_t compressed_base_free_count() const override { return compressed.free_handles(64).size(); }
    bool acquire_base(std::uint32_t id) override {
        if (loading) load_trace.push_back(10);
        auto &s = state(id);
        record(1);
        require(!settings.compressed_base, "Compressed acquisition is outside this oracle transport");
        return material_cache_acquire_base(s.textures, *this, settings.base_size);
    }
    bool acquire_higher(std::uint32_t id) override {
        auto &s = state(id);
        record(2);
        require(!settings.compressed_base, "Compressed acquisition is outside this oracle transport");
        return material_cache_acquire_higher(s.textures, *this, settings.base_size);
    }
    void downgrade(std::uint32_t id) override {
        auto &s = state(id);
        record(3);
        material_cache_downgrade(s, *this, *this, separate);
    }
    void invalidate_geometry(std::uint32_t id) override {
        material_cache_invalidate_geometry(state(id), *this, separate);
    }
    struct CompressedReturn final : MaterialCacheTexturePool {
        Runner &r;
        explicit CompressedReturn(Runner &owner) : r(owner) {}
        std::uint32_t take(std::uint32_t, std::uint32_t &) override {
            throw std::runtime_error("Unexpected compressed take");
        }
        void give(std::uint32_t handle) override {
            r.record(13, {handle});
            r.compressed.give(handle);
        }
    };
    void release_cache(std::uint32_t id, bool geometry, bool final, bool higher) override {
        auto &s = state(id);
        record(5, {geometry, final, higher});
        CompressedReturn target(*this);
        material_cache_release(s, *this, *this, separate, geometry, final, higher,
                               settings.compressed_base ? &target : nullptr);
    }
    void refresh_selection(std::uint32_t id) override {
        auto &s = state(id);
        record(6);
        if (mutate_refresh)
            s.textures.specular = !s.textures.specular;
    }
    void prepare(std::uint32_t id, std::uintptr_t dispatch) override {
        if (loading) load_trace.push_back(36);
        state(id);
        record(7, {static_cast<std::uint32_t>(dispatch)});
    }
    void prepare_frame() override { load_trace.insert(load_trace.end(), {32, 33}); }
    void begin_step() override { load_trace.insert(load_trace.end(), {34, 35}); }
    void wait() override { load_trace.push_back(37); }
    void complete(std::uint32_t id, std::uint32_t stamp, std::uintptr_t argument) override {
        load_trace.push_back(38);
        state(id);
        record(30, {stamp, static_cast<std::uint32_t>(argument)});
    }
    void end_generation() override { load_trace.push_back(39); }
    void end_step() override { load_trace.push_back(41); }
    std::uint32_t take(std::uint32_t size, std::uint32_t &actual) override {
        return ordinary.take(size, actual);
    }
    void give(std::uint32_t handle) override {
        record(12, {handle});
        ordinary.give(handle);
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override {
        throw std::runtime_error("Unexpected rebuild");
    }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {
        throw std::runtime_error("Unexpected generation");
    }
    void release_geometry_pair(std::uint32_t v, std::uint32_t i) override { record(8, {v, i}); }
    void release_vertex_buffer(std::uint32_t h) override { record(9, {h}); }
    void release_index_buffer(std::uint32_t h) override { record(10, {h}); }
    void release_packet(std::uint32_t h) override { record(11, {h}); }
    void free_staging(std::uintptr_t) override { throw std::runtime_error("Unexpected staging free"); }
};
std::vector<std::uint32_t> vector(Reader &in) {
    std::vector<std::uint32_t> out(in.word());
    for (auto &v : out)
        v = in.word();
    return out;
}
void floats(Reader &in, auto &values) {
    for (auto &v : values)
        v = std::bit_cast<float>(in.word());
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Scheduler primitive input and fresh output required");
        Reader in(argv[1]);
        const auto schema = in.bytes(8);
        const bool camera_input = schema == Bytes{'C', '5', 'S', 'C', 'A', 'M', '0', '1'};
        const bool loading_input = schema == Bytes{'C', '5', 'S', 'C', 'L', 'D', '0', '1'};
        require(camera_input || loading_input || schema == Bytes{'C', '5', 'S', 'C', 'H', 'F', 'I', '1'},
                "Full scheduler input schema");
        const auto count = in.word();
        Bytes out{'C', '5', 'S', 'C', 'H', 'F', 'O', '1'};
        word(out, count);
        for (unsigned test = 0; test < count; ++test) {
            Runner r;
            auto &s = r.settings;
            MaterialCacheSchedulerState owner;
            const auto n = in.word(), pc = in.word(), path = in.word();
            require(n <= 2048 && (pc == 24 || pc == 53) && path <= 1, "Scheduler fixture domain");
            s.precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            s.vector_path = path ? Vec4Path::sse : Vec4Path::legacy_x87;
            s.attempt_budget = in.word();
            s.fast_selection_budget = in.word();
            s.fast = in.word();
            s.force_fast = in.word();
            s.viewport_width = in.word();
            s.base_size = in.word();
            s.cull = in.word();
            s.compressed_base = in.word();
            r.separate = in.word();
            owner.background_cursor = in.word();
            r.mutate_refresh = in.word();
            s.dispatch = in.word();
            std::optional<LandLayout> land;
            std::vector<MaterialCacheSchedulerView> views;
            if (camera_input) {
                const auto width = std::bit_cast<std::int32_t>(in.word());
                const auto height = std::bit_cast<std::int32_t>(in.word());
                const auto mode = std::bit_cast<std::int32_t>(in.word());
                PerspectiveCamera camera{};
                camera.field_of_view = std::bit_cast<float>(in.word());
                camera.aspect_ratio = std::bit_cast<float>(in.word());
                camera.near_plane = std::bit_cast<float>(in.word());
                camera.far_plane = std::bit_cast<float>(in.word());
                floats(in, camera.world);
                CameraFrameState prior{};
                static_assert(sizeof(prior) == 1012);
                const auto entering = in.bytes(sizeof(prior));
                std::memcpy(&prior, entering.data(), sizeof(prior));
                std::vector<std::array<std::uint32_t, 2>> inverses(in.word());
                for (auto &v : inverses)
                    for (auto &value : v)
                        value = in.word();
                std::size_t cursor = 0;
                const auto reciprocal = [&](float determinant) {
                    require(cursor < inverses.size() &&
                                std::bit_cast<std::uint32_t>(determinant) == inverses[cursor][0],
                            "Original camera reciprocal input differs");
                    return std::bit_cast<float>(inverses[cursor++][1]);
                };
                const auto world = build_world_layout(width, height, mode, {}, reciprocal);
                const auto frame = camera_frame(world, mode, camera, prior, s.precision, reciprocal);
                require(cursor == inverses.size(), "Unused camera reciprocal observations");
                s.projection = frame.projection;
                views = material_cache_camera_views(frame);
                land.emplace(HeightDomain(world.worker_grid.width * 64, world.worker_grid.height * 64));
                require(n == unsigned(land->columns() * land->rows()), "Camera/LAND world domain differs");
            } else {
                floats(in, s.projection);
                views.resize(in.word());
                for (auto &v : views) {
                    floats(in, v.matrix);
                    for (auto &p : v.frustum)
                        floats(in, p);
                }
            }
            for (unsigned i = 0; i < n; ++i) {
                auto raw_patch = in.bytes(1024);
                if (land) {
                    const auto patch = land->patch(i / land->rows(), i % land->rows());
                    const auto bounds = patch.culling_bounds();
                    const auto sphere = patch.culling_sphere(s.precision);
                    for (unsigned k = 0; k < 6; ++k)
                        put(raw_patch, 0x10c + k * 4, std::bit_cast<std::uint32_t>(bounds[k]));
                    for (unsigned k = 0; k < 4; ++k)
                        put(raw_patch, 0x124 + k * 4, std::bit_cast<std::uint32_t>(sphere[k]));
                }
                auto item = std::make_unique<Case>(std::move(raw_patch));
                const auto &raw = item->entering;
                MaterialCacheSchedulerPatch p;
                p.state = &item->state;
                for (unsigned j = 0; j < 6; ++j) {
                    p.bounds[j] = std::bit_cast<float>(get(raw, 0x10c + j * 4));
                    p.plane_distances[j] = std::bit_cast<float>(get(raw, 0x1a0 + j * 4));
                }
                for (unsigned j = 0; j < 4; ++j)
                    p.sphere[j] = std::bit_cast<float>(get(raw, 0x124 + j * 4));
                p.enabled = get(raw, 0x1c0);
                p.blocked = get(raw, 0x328);
                p.priority = get(raw, 0x1bc);
                p.eviction_stamp = get(raw, 0x1d4);
                p.source_revision = get(raw, 0x1cc);
                p.frustum_visible = raw[0x134];
                p.submitted_view = raw[0x135];
                r.patches.push_back(p);
                r.cases.push_back(std::move(item));
            }
            const auto active = vector(in), background = vector(in);
            owner.pending = vector(in);
            const auto metadata = in.word();
            for (unsigned i = 0; i < metadata; ++i) {
                const auto h = in.word(), w = in.word(), c = in.word();
                (c ? r.compressed : r.ordinary).add_texture(h, w, false);
            }
            for (auto w : widths)
                for (auto h : vector(in)) {
                    require(r.ordinary.texture_width(h) == w, "Fixture width");
                    r.ordinary.give(h);
                }
            for (auto h : vector(in))
                r.compressed.give(h);
            if (loading_input) {
                r.loading = true;
                const auto block = in.word(), cull = in.word(), steps = in.word();
                require(block <= 1 && cull <= 1 && steps <= 1024, "Loading controls domain");
                MaterialCacheLoadState load;
                load.prepared_in_step = in.word();
                load.finished = in.word();
                load.progress = std::bit_cast<float>(in.word());
                material_cache_begin_load(r.patches, active, views, block != 0, cull != 0, load);
                const auto capture = [&] {
                    for (auto value : {load.cursor, load.background_cursor, load.prepared_in_step, load.finished,
                                       std::bit_cast<std::uint32_t>(load.progress), n})
                        word(out, value);
                    for (unsigned i = 0; i < n; ++i) {
                        const auto raw = r.snapshot(i);
                        out.insert(out.end(), raw.begin(), raw.end());
                    }
                    vector_words(out, load.foreground);
                    vector_words(out, load.background);
                    for (auto w : widths)
                        vector_words(out, r.ordinary.free_handles(w));
                    vector_words(out, r.compressed.free_handles(64));
                    word(out, r.event_count);
                    out.insert(out.end(), r.events.begin(), r.events.end());
                    vector_words(out, r.load_trace);
                };
                capture();
                for (unsigned i = 0; i < steps; ++i) {
                    const auto stamp = in.word(), argument = in.word();
                    material_cache_load_step(r.patches, load, s.precision, s.dispatch, stamp, argument, r);
                    capture();
                }
                continue;
            }
            const auto finished = material_cache_schedule(r.patches, active, background, views, s, owner, r);
            word(out, finished);
            word(out, owner.background_cursor);
            word(out, n);
            for (unsigned i = 0; i < n; ++i) {
                const auto raw = r.snapshot(i);
                out.insert(out.end(), raw.begin(), raw.end());
            }
            for (const auto &v : owner.visible)
                vector_words(out, v);
            vector_words(out, owner.eviction_candidates);
            vector_words(out, owner.pending);
            for (auto w : widths)
                vector_words(out, r.ordinary.free_handles(w));
            vector_words(out, r.compressed.free_handles(64));
            word(out, r.event_count);
            out.insert(out.end(), r.events.begin(), r.events.end());
        }
        require(in.at == in.data.size(), "Trailing scheduler input");
        require(!std::filesystem::exists(argv[2]), "Fresh scheduler output required");
        std::ofstream stream(argv[2], std::ios::binary);
        stream.write(reinterpret_cast<const char *>(out.data()), out.size());
        require(bool(stream), "Scheduler output failed");
        std::cout << "PASS full outer scheduler " << count << " cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
