#include "civ5/graphics/gl_land_material.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_cache_geometry.hpp"
#include "civ5/materials/material_cache_rebuild.hpp"
#include "civ5/materials/material_cache_free_lists.hpp"
#include "land_material_native.hpp"
#include "material_generation_fixture.hpp"
#include <bit>
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
using namespace land_material_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace io = generation_validation;
#include "land_material_validation_inputs.hpp"
namespace {
using Kind = MaterialGeometryBufferKind;
struct Source {
    unsigned width, height;
    io::Bytes bytes;
    std::uint16_t at(int x, int y) const {
        require(x >= 0 && y >= 0 && unsigned(x) < width && unsigned(y) < height, "GPU geometry source address");
        std::uint16_t value; std::memcpy(&value, bytes.data() + (std::size_t(y) * width + x) * 2, 2); return value;
    }
};
struct Pipeline final : MaterialCacheStagingBackend, MaterialCacheRebuildBackend {
    MaterialCacheLifecycleState state;
    MaterialCacheGeometryJob *job = nullptr;
    std::shared_ptr<GlLandGeometryPool> pool;
    MaterialCacheRebuildSettings settings;
    GlLandGeometryLease lease;
    std::unique_ptr<LandMaterialMesh> mesh;
    std::map<std::uintptr_t, io::Bytes> staging;
    std::map<unsigned, MaterialCachePacket> packets;
    unsigned next_packet = 1;
    explicit Pipeline(std::shared_ptr<GlLandGeometryPool> p, unsigned cols, unsigned rows)
        : pool(std::move(p)), settings{cols, rows, 0, 0, 0.f, 6, false} {
        state.requested_state = 4; state.completed_state = 1; state.pending_jobs = 2;
    }
    std::uintptr_t allocate_staging(std::size_t bytes) override {
        io::Bytes data(bytes, 0xa5); const auto token = reinterpret_cast<std::uintptr_t>(data.data());
        require(token > UINT32_MAX, "Geometry pointer width"); staging.emplace(token, std::move(data)); return token;
    }
    std::span<std::uint8_t> staging_memory(std::uintptr_t token) override { return staging.at(token); }
    void free_staging(std::uintptr_t token) override { if (token) require(staging.erase(token) == 1, "Geometry staging ownership"); }
    void submit_geometry(MaterialCacheGeometryJob &j, std::uintptr_t) override { require(!job && j.owner == &state, "Geometry job identity"); job = &j; }
    void prepare_material(MaterialCacheLifecycleState &, std::uintptr_t) override { throw std::logic_error("Unexpected material job"); }
    void rebuild_geometry(MaterialCacheLifecycleState &s) override { material_cache_rebuild_geometry(s, settings, {}, *this); }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override { throw std::logic_error("Unexpected material generation"); }
    void upload_geometry_pair(std::uintptr_t v, unsigned nv, std::uintptr_t i, unsigned ni, unsigned &vb, unsigned &ib) override {
        require(staging.at(v).size() >= std::size_t(nv) * 24 && staging.at(i).size() >= std::size_t(ni) * 2, "Prepared geometry capacity");
        std::vector<civ5::terrain::LandGridVertex> vertices(nv); std::vector<std::uint16_t> indices(ni);
        std::memcpy(vertices.data(), staging.at(v).data(), nv * 24);
        std::memcpy(indices.data(), staging.at(i).data(), ni * 2);
        mesh = std::make_unique<LandMaterialMesh>(LandVertexInput::source_positioned, vertices, indices);
        lease = pool->upload(*mesh); vb = lease.vertex_buffer(); ib = lease.index_buffer();
    }
    void shared_geometry(unsigned, unsigned, unsigned &, unsigned &, unsigned &, unsigned &) override { throw std::logic_error("Unexpected shared mesh"); }
    void retain_vertex_buffer(unsigned) override { throw std::logic_error("Unexpected retain"); }
    void retain_index_buffer(unsigned) override { throw std::logic_error("Unexpected retain"); }
    void release_geometry_pair(unsigned vb, unsigned ib) override {
        require(vb == lease.vertex_buffer() && ib == lease.index_buffer(), "Returned geometry pair"); pool->release(lease);
    }
    void release_vertex_buffer(unsigned h) override { require(!h, "Unexpected vertex release"); }
    void release_index_buffer(unsigned h) override { require(!h, "Unexpected secondary geometry"); }
    MaterialCachePacket &allocate_land_packet(unsigned &h) override { h = next_packet++; return packets[h]; }
    void release_packet(unsigned h) override { if (h) require(packets.erase(h) == 1, "Packet release ownership"); }
};
LandMaterialConstants frame(const LandMaterialMesh &mesh, unsigned variant) {
    auto p = constants(65, 65, variant);
    const auto a = mesh.vertices().front().position, b = mesh.vertices().back().position;
    p.world = identity(); p.world[3] = {-a[0], -a[1], 0, 1};
    p.view_projection = {{{1.95f / (b[0] - a[0]), 0, 0, 0}, {0, -1.95f / (b[1] - a[1]), 0, 0},
                          {0, 0, .01f, 0}, {-.975f, .975f, 0, 1}}};
    return p;
}
std::vector<float> draw(Control &c, GlLandMaterialRenderer &r, const LandMaterialConstants &p, const GlLandMaterialInputs &in) {
    c.attach({c.targets[6]}, false);
    const float clear[]{187.f/255, 51.f/255, 119.f/255, 1}; c.gl.ClearNamedFramebufferfv(c.fbo, GL_COLOR, 0, clear);
    r.draw(p, in, 64, 64); return c.pixels(c.targets[6]);
}
}
int main(int argc, char **argv) {
    try {
        require(argc == 4, "Usage: geometry-input original-shaders fresh-output");
        const fs::path output(argv[3]); require(!fs::exists(output), "Fresh geometry GPU output required");
        const auto data = io::read(argv[1]); io::Reader in{data};
        const auto magic = in.take(8); require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5GJOBI1", "Geometry source schema");
        std::vector<Source> sources; const auto source_count = in.word(); require(source_count <= 8, "Source count");
        for (unsigned i = 0; i < source_count; ++i) {
            const auto width = in.word(), height = in.word(); require(std::uint64_t(width) * height <= 32000000, "Source extent");
            const auto bytes = in.take(std::size_t(width) * height * 2); sources.push_back({width, height, {bytes.begin(), bytes.end()}});
        }
        auto context = std::make_shared<GlContext>(); Control c(context); NativeLand native(argv[2]);
        auto pool = std::make_shared<GlLandGeometryPool>(context);
        GlLandMaterialInputs resources; std::vector<Com<IDirect3DBaseTexture9>> native_images;
        std::array<IDirect3DBaseTexture9 *, 10> pointers{}; const auto images = fixtures();
        for (unsigned i = 0; i < 10; ++i) { resources.pixel[i] = c.image(images[i]); native_images.push_back(native.image(images[i])); pointers[i] = native_images.back().value; }
        std::array<std::unique_ptr<GlLandMaterialRenderer>, 2> renderers;
        std::map<unsigned, io::Bytes> previous_buffers;
        unsigned jobs = 0, draws = 0, guards = 0; double maximum_error = 0;
        std::uint64_t compared_bytes = 0;
        fs::create_directories(output);
        const auto count = in.word();
        for (unsigned n = 0; n < count; ++n) {
            const auto &source = sources.at(in.word()); const auto step = in.word(), pc = in.word(), available = in.word(), wrap = in.word();
            civ5::terrain::Vector3 scales; for (auto &v : scales) v = std::bit_cast<float>(in.word());
            const auto before = in.take(1024);
            const auto word = [&](unsigned offset) { io::Reader r{before.subspan(offset, 4)}; return r.word(); };
            if (available != 3 || step == 1) continue;
            const MaterialCacheGeometryDomain domain{{static_cast<int>(word(0xd4)), static_cast<int>(word(0xd8)), static_cast<int>(step)},
                {std::bit_cast<std::int32_t>(word(0xdc)), std::bit_cast<std::int32_t>(word(0xe0))},
                static_cast<int>(source.width), static_cast<int>(source.height), scales,
                pc == 24 ? civ5::terrain::X87Precision::single : civ5::terrain::X87Precision::double_precision, wrap != 0};
            Pipeline pipeline(pool, domain.patch.columns, domain.patch.rows);
            material_cache_prepare(pipeline.state, domain.patch, pipeline, false, 0);
            require(pipeline.job && pipeline.state.prepared, "Actual geometry job not submitted");
            material_cache_execute_geometry_job(*pipeline.job, domain, pipeline, [&](int x, int y) { return source.at(x, y); });
            MaterialCacheFreeLists texture_pool;
            require(material_cache_dispatch(pipeline.state, texture_pool, pipeline, false, n + 1, 0) && pipeline.staging.empty() &&
                        pipeline.state.pending_jobs == 1 && pipeline.state.completed_state == 4 && pipeline.packets.size() == 2,
                    "Geometry worker/upload/rebuild/completion connection");
            const auto &mesh = *pipeline.mesh;
            const auto key = "job-" + std::to_string(n);
            for (auto kind : {Kind::vertex, Kind::index}) {
                const auto handle = kind == Kind::vertex ? pipeline.lease.vertex_buffer() : pipeline.lease.index_buffer();
                const auto raw = kind == Kind::vertex ? std::span(reinterpret_cast<const std::uint8_t *>(mesh.vertices().data()), mesh.vertices().size_bytes())
                                                     : std::span(reinterpret_cast<const std::uint8_t *>(mesh.indices().data()), mesh.indices().size_bytes());
                auto gpu = pool->read_buffer(pipeline.lease, kind);
                require(gpu.size() >= raw.size() && std::equal(raw.begin(), raw.end(), gpu.begin()), "GPU geometry upload differs");
                if (previous_buffers.contains(handle)) {
                    const auto &old = previous_buffers.at(handle); require(old.size() == gpu.size(), "Reused GPU storage resized");
                    require(std::equal(gpu.begin() + raw.size(), gpu.end(), old.begin() + raw.size()), "Geometry reuse overwrote untouched capacity");
                }
                previous_buffers[handle] = gpu; compared_bytes += raw.size();
                io::write(output / (key + (kind == Kind::vertex ? "-vb.bin" : "-ib.bin")), gpu);
            }
            for (unsigned slot = 0; slot < 2; ++slot) {
                const unsigned selector = slot + 3;
                if (!renderers[slot]) renderers[slot] = std::make_unique<GlLandMaterialRenderer>(context, pool, pipeline.lease, selector, LandShadowArithmetic::measured_fma_t_x_z_y);
                else renderers[slot]->set_geometry(pipeline.lease);
                GlLandMaterialRenderer legacy(context, mesh, selector, LandShadowArithmetic::measured_fma_t_x_z_y);
                for (unsigned variant = 0; variant < 6; ++variant) {
                    const auto parameters = frame(mesh, variant);
                    const auto actual = draw(c, *renderers[slot], parameters, resources);
                    const auto old = draw(c, legacy, parameters, resources);
                    require(actual == old, "Pooled LAND differs from direct geometry renderer");
                    const auto expected = native.draw(mesh, selector, parameters, pointers, nullptr);
                    for (unsigned i = 0; i < actual.size(); ++i) {
                        require(std::isfinite(expected[i]), "Nonfinite original LAND pixel");
                        const double error = std::abs(double(actual[i]) - expected[i]); maximum_error = std::max(maximum_error, error);
                        require(error <= 1e-5, "Pooled LAND differs from complete original shader");
                    }
                    const auto name = key + "-s" + std::to_string(selector) + "-v" + std::to_string(variant);
                    write(output / (name + "-gl.bin"), actual); write(output / (name + "-d3d9.bin"), expected); ++draws;
                    if (!variant) for (bool indexed : {false, true}) {
                        const auto tf = renderers[slot]->capture(parameters, 0, indexed);
                        require(tf == legacy.capture(parameters, 0, indexed), "Pooled geometry vertex outputs changed");
                        write(output / (name + (indexed ? "-indexed.bin" : "-tf.bin")), tf);
                    }
                }
            }
            material_cache_invalidate_geometry(pipeline.state, pipeline, false);
            require(pipeline.packets.empty(), "Geometry packet invalidation leak");
            for (auto &renderer : renderers) {
                bool rejected = false;
                try { renderer->capture(frame(mesh, 0), 0, false); } catch (const std::invalid_argument &) { rejected = true; }
                require(rejected, "Returned geometry lease remained drawable"); ++guards;
            }
            ++jobs;
        }
        in.end(); require(jobs == 16 && draws == 192 && guards == 32, "Incomplete geometry GPU corpus");
        require(pool->entries(Kind::vertex).size() < jobs && pool->entries(Kind::index).size() < jobs, "Geometry did not reuse GPU storage");
        std::ostringstream report; report.precision(17);
        report << context->description() << "\nDONE jobs=" << jobs << " draws=" << draws << " guards=" << guards
               << " uploadedBytes=" << compared_bytes << " maximumError=" << maximum_error
               << " vertexBuffers=" << pool->entries(Kind::vertex).size() << " indexBuffers=" << pool->entries(Kind::index).size()
               << " actualWorkers=true actualRebuild=true originalFullShaders=true liveWorld=false\n";
        text(output / "execution.txt", report.str()); std::cout << report.str();
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
