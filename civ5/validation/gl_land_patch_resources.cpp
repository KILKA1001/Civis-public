#include "civ5/graphics/gl_land_patch_resources.hpp"
#include "civ5/graphics/gl_land_material.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_cache_free_lists.hpp"
#include "civ5/materials/material_cache_geometry.hpp"
#include "land_material_native.hpp"
#include "material_generation_fixture.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <sstream>
using namespace land_material_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace io = generation_validation;
#include "land_material_validation_inputs.hpp"
namespace {
using Kind = MaterialGeometryBufferKind;
LandPacketTextures sources() {
  LandPacketTextures t;
  t.shared = {2, 3, 4, 5, 6, 10};
  t.style_diffuse.fill(7);
  t.style_specular.fill(8);
  return t;
}
void configure(GlLandPatchResources &r) {
  r.state().textures.final = {1, 9};
  r.state().textures.higher = {1, 9};
}
struct Source {
  unsigned width, height;
  io::Bytes bytes;
  std::uint16_t at(int x, int y) const {
    require(x >= 0 && y >= 0 && unsigned(x) < width && unsigned(y) < height,
            "Geometry source address");
    std::uint16_t v;
    std::memcpy(&v, bytes.data() + (std::size_t(y) * width + x) * 2, 2);
    return v;
  }
};
struct Pipeline final : MaterialCacheStagingBackend {
  GlLandPatchResources resources;
  MaterialCacheGeometryJob *job = nullptr;
  MaterialCacheRebuildSettings settings;
  std::unique_ptr<LandMaterialMesh> mesh;
  std::map<std::uintptr_t, io::Bytes> staging;
  Pipeline(std::shared_ptr<GlLandGeometryPool> g,
           std::shared_ptr<MaterialPacketPool> p, unsigned cols, unsigned rows)
      : resources(std::move(g), std::move(p)),
        settings{cols, rows, 0, 0, 0.f, 6, false} {
    auto &s = resources.state();
    s.requested_state = 4;
    s.completed_state = 1;
    s.pending_jobs = 2;
    configure(resources);
  }
  std::uintptr_t allocate_staging(std::size_t n) override {
    io::Bytes b(n, 0xa5);
    const auto token = reinterpret_cast<std::uintptr_t>(b.data());
    require(token > UINT32_MAX, "Geometry pointer width");
    staging.emplace(token, std::move(b));
    return token;
  }
  std::span<std::uint8_t> staging_memory(std::uintptr_t token) override {
    return staging.at(token);
  }
  void free_staging(std::uintptr_t token) override {
    if (token)
      require(staging.erase(token) == 1, "Staging ownership");
  }
  void submit_geometry(MaterialCacheGeometryJob &j, std::uintptr_t) override {
    require(!job && j.owner == &resources.state(),
            "Actual resource-owner job identity");
    job = &j;
  }
  void prepare_material(MaterialCacheLifecycleState &,
                        std::uintptr_t) override {
    throw std::logic_error("Unexpected material job");
  }
  void generate_cache(MaterialCacheLifecycleState &, std::uint32_t,
                      std::uintptr_t) override {
    throw std::logic_error("Unexpected cache job");
  }
  void rebuild_geometry(MaterialCacheLifecycleState &s) override {
    require(&s == &resources.state(), "Rebuild used a different patch state");
    std::vector<civ5::terrain::LandGridVertex> vertices(
        s.geometry.vertex_count);
    std::vector<std::uint16_t> indices(s.geometry.triangle_count * 3);
    require(staging.at(s.staging[0]).size() >= vertices.size() * 24 &&
                staging.at(s.staging[1]).size() >= indices.size() * 2,
            "Staging extent");
    std::memcpy(vertices.data(), staging.at(s.staging[0]).data(),
                vertices.size() * 24);
    std::memcpy(indices.data(), staging.at(s.staging[1]).data(),
                indices.size() * 2);
    mesh = std::make_unique<LandMaterialMesh>(
        LandVertexInput::source_positioned, vertices, indices);
    resources.rebuild(settings, sources(), 1, this);
  }
  void release_geometry_pair(unsigned, unsigned) override {
    throw std::logic_error("Invalidation belongs to actual patch owner");
  }
  void release_vertex_buffer(unsigned) override {
    throw std::logic_error("Invalidation belongs to actual patch owner");
  }
  void release_index_buffer(unsigned) override {
    throw std::logic_error("Invalidation belongs to actual patch owner");
  }
  void release_packet(unsigned) override {
    throw std::logic_error("Invalidation belongs to actual patch owner");
  }
};
LandMaterialConstants frame(const LandMaterialMesh &mesh, unsigned variant) {
  auto p = constants(65, 65, variant);
  const auto a = mesh.vertices().front().position,
             b = mesh.vertices().back().position;
  p.world = identity();
  p.world[3] = {-a[0], -a[1], 0, 1};
  p.view_projection = {{{1.95f / (b[0] - a[0]), 0, 0, 0},
                        {0, -1.95f / (b[1] - a[1]), 0, 0},
                        {0, 0, .01f, 0},
                        {-.975f, .975f, 0, 1}}};
  return p;
}
std::vector<float> packet_draw(Control &c, GlLandMaterialRenderer &r,
                               const MaterialCachePacket &packet,
                               const LandMaterialConstants &p,
                               const GlLandMaterialInputs &inputs) {
  c.attach({c.targets[6]}, false);
  const float clear[]{187.f / 255, 51.f / 255, 119.f / 255, 1};
  c.gl.ClearNamedFramebufferfv(c.fbo, GL_COLOR, 0, clear);
  r.draw_packet(
      packet, p,
      [&](std::uint16_t h) {
        require(h >= 1 && h <= 11, "Packet resource lookup");
        return h == 11 ? inputs.height : inputs.pixel[h - 1];
      },
      64, 64);
  return c.pixels(c.targets[6]);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 4, "Usage: geometry-input original-shaders fresh-output");
    const fs::path output(argv[3]);
    require(!fs::exists(output), "Fresh patch GPU output required");
    const auto data = io::read(argv[1]);
    io::Reader in{data};
    const auto magic = in.take(8);
    require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) ==
                "C5GJOBI1",
            "Geometry source schema");
    std::vector<Source> fields;
    const auto source_count = in.word();
    require(source_count <= 8, "Source count");
    for (unsigned n = 0; n < source_count; ++n) {
      const auto w = in.word(), h = in.word();
      require(std::uint64_t(w) * h <= 32000000, "Source extent");
      const auto b = in.take(std::size_t(w) * h * 2);
      fields.push_back({w, h, {b.begin(), b.end()}});
    }
    auto context = std::make_shared<GlContext>();
    Control control(context);
    NativeLand native(argv[2]);
    auto pool = std::make_shared<GlLandGeometryPool>(context);
    auto packets = std::make_shared<MaterialPacketPool>();
    GlLandMaterialInputs images;
    std::vector<Com<IDirect3DBaseTexture9>> native_images;
    std::array<IDirect3DBaseTexture9 *, 10> pointers{};
    const auto fixtures_data = fixtures();
    for (unsigned i = 0; i < 10; ++i) {
      images.pixel[i] = control.image(fixtures_data[i]);
      native_images.push_back(native.image(fixtures_data[i]));
      pointers[i] = native_images.back().value;
    }
    unsigned jobs = 0, paired_draws = 0, shared_draws = 0, guards = 0;
    double maximum = 0;
    std::uint64_t geometry_bytes = 0;
    const auto rejects = [&](auto action) {
      bool failed = false;
      try {
        action();
      } catch (const std::invalid_argument &) {
        failed = true;
      }
      require(failed, "Missing resource guard");
      ++guards;
    };
    const auto compare = [&](const auto &a, const auto &b,
                             const std::string &name) {
      require(a.size() == b.size(), "Image extent");
      for (unsigned i = 0; i < a.size(); ++i) {
        const double error = std::abs(double(a[i]) - b[i]);
        require(std::isfinite(b[i]) && error <= 1e-5,
                "Packet draw differs from original shader");
        maximum = std::max(maximum, error);
      }
      write(output / (name + "-gl.bin"), a);
      write(output / (name + "-d3d9.bin"), b);
    };
    fs::create_directories(output);
    const auto count = in.word();
    for (unsigned n = 0; n < count; ++n) {
      const auto &field = fields.at(in.word());
      const auto step = in.word(), pc = in.word(), available = in.word(),
                 wrap = in.word();
      civ5::terrain::Vector3 scales;
      for (auto &v : scales)
        v = std::bit_cast<float>(in.word());
      const auto before = in.take(1024);
      const auto word = [&](unsigned offset) {
        io::Reader r{before.subspan(offset, 4)};
        return r.word();
      };
      if (available != 3 || step == 1)
        continue;
      const MaterialCacheGeometryDomain domain{
          {static_cast<int>(word(0xd4)), static_cast<int>(word(0xd8)),
           static_cast<int>(step)},
          {std::bit_cast<std::int32_t>(word(0xdc)),
           std::bit_cast<std::int32_t>(word(0xe0))},
          static_cast<int>(field.width),
          static_cast<int>(field.height),
          scales,
          pc == 24 ? civ5::terrain::X87Precision::single
                   : civ5::terrain::X87Precision::double_precision,
          wrap != 0};
      Pipeline pipeline(pool, packets, domain.patch.columns, domain.patch.rows);
      auto &state = pipeline.resources.state();
      material_cache_prepare(state, domain.patch, pipeline, false, 0);
      require(pipeline.job && state.prepared, "Actual worker submission");
      material_cache_execute_geometry_job(
          *pipeline.job, domain, pipeline,
          [&](int x, int y) { return field.at(x, y); });
      MaterialCacheFreeLists textures;
      require(
          material_cache_dispatch(state, textures, pipeline, false, n + 1, 0) &&
              pipeline.staging.empty() && state.completed_state == 4 &&
              state.pending_jobs == 1,
          "Actual worker/resource rebuild/completion");
      const auto lease = pipeline.resources.geometry();
      const auto &mesh = *pipeline.mesh;
      for (auto kind : {Kind::vertex, Kind::index}) {
        const auto raw = kind == Kind::vertex
                             ? std::span(reinterpret_cast<const std::uint8_t *>(
                                             mesh.vertices().data()),
                                         mesh.vertices().size_bytes())
                             : std::span(reinterpret_cast<const std::uint8_t *>(
                                             mesh.indices().data()),
                                         mesh.indices().size_bytes());
        const auto gpu = pool->read_buffer(lease, kind);
        require(gpu.size() >= raw.size() &&
                    std::equal(raw.begin(), raw.end(), gpu.begin()),
                "Actual resource upload");
        geometry_bytes += raw.size();
      }
      for (unsigned selector = 3; selector < 5; ++selector) {
        GlLandMaterialRenderer renderer(
            context, pool, lease, selector,
            LandShadowArithmetic::measured_fma_t_x_z_y);
        for (unsigned variant = 0; variant < 6; ++variant) {
          const auto p = frame(mesh, variant);
          const auto actual = packet_draw(
              control, renderer, pipeline.resources.packet(0), p, images);
          compare(actual, native.draw(mesh, selector, p, pointers, nullptr),
                  "job-" + std::to_string(n) + "-s" + std::to_string(selector) +
                      "-v" + std::to_string(variant));
          ++paired_draws;
        }
      }
      const auto old = state.geometry.packets;
      pipeline.resources.invalidate();
      require(packets->references(old[0]) == 0 &&
                  packets->references(old[1]) == 0,
              "Packet references survived invalidation");
      rejects([&] { pool->buffers(lease, *context); });
      rejects([&] { pipeline.resources.packet(0); });
      packets->advance_frame();
      packets->collect();
      ++jobs;
    }
    in.end();
    require(jobs == 16 && paired_draws == 192, "CPU resource corpus coverage");
    io::Bytes height_bytes(1024 * 1024);
    for (unsigned i = 0; i < height_bytes.size(); ++i)
      height_bytes[i] =
          static_cast<std::uint8_t>(31 + (i * 7 + i / 1024 * 13) % 151);
    auto height = GlTexture::alpha8(*context, 1024, 1024, height_bytes);
    auto native_height = native.height(height_bytes, 1024);
    images.height = height.id();
    unsigned grid_index = 0;
    for (int cap : {6, 10})
      for (const auto dims :
           {std::array{65, 65, 1}, std::array{9, 7, 2}, std::array{3, 3, 1}}) {
        // Capability is not part of the original cache key, so use a fresh
        // cache to exercise both genuinely created optional-control paths.
        auto shared = std::make_shared<GlLandGeometryPool>(context);
        GlLandPatchResources first(shared, packets), second(shared, packets);
        configure(first);
        configure(second);
        const MaterialCacheRebuildSettings settings{
            unsigned(dims[0]), unsigned(dims[1]), 0, 11, 0.f, cap, true};
        first.rebuild(settings, sources(), dims[2]);
        second.rebuild(settings, sources(), dims[2]);
        const auto lease = second.geometry(), old_first = first.geometry();
        for (auto h : {lease.vertex_buffer(), lease.index_buffer(),
                       lease.control_buffer()})
          if (h)
            require(shared->shared_references(h) == 3,
                    "Two patch references plus cache base");
        const auto overlay = first.state().geometry.packets[4];
        first.invalidate();
        for (auto h : {lease.vertex_buffer(), lease.index_buffer(),
                       lease.control_buffer()})
          if (h)
            require(shared->shared_references(h) == 2,
                    "One invalidation damaged neighbour");
        rejects([&] { shared->buffers(old_first, *context); });
        if (cap >= 10) {
          require(overlay && packets->references(overlay) == 1 &&
                      first.state().geometry.packets[4] == overlay,
                  "Original overlay retention");
          const auto &p = first.packet(4);
          require(p.geometry == std::array<std::uint32_t, 5>{} &&
                      p.vertex_samplers[0] == 0,
                  "Additional packet geometry");
        }
        const civ5::terrain::LandGrid grid(dims[0], dims[1], dims[2]);
        const LandMaterialMesh mesh(LandVertexInput::local_grid,
                                    grid.vertices(), grid.indices());
        for (unsigned selector = 0; selector < 3; ++selector) {
          GlLandMaterialRenderer renderer(
              context, shared, lease, selector,
              LandShadowArithmetic::measured_fma_t_x_z_y);
          for (unsigned variant = 0; variant < 6; ++variant) {
            const auto p = constants(dims[0], dims[1], variant);
            compare(
                packet_draw(control, renderer, second.packet(0), p, images),
                native.draw(mesh, selector, p, pointers, native_height.value),
                "grid-" + std::to_string(grid_index) + "-s" +
                    std::to_string(selector) + "-v" + std::to_string(variant));
            ++shared_draws;
          }
          for (unsigned corrupt = 0; corrupt < 6; ++corrupt) {
            auto bad = second.packet(0);
            switch (corrupt) {
            case 0:
              bad.key = 0;
              break;
            case 1:
              bad.geometry[1]++;
              break;
            case 2:
              bad.pixel_count = 9;
              break;
            case 3:
              bad.pixel_samplers[3] = 5;
              break;
            case 4:
              bad.vertex_count = 0;
              break;
            case 5:
              bad.vertex_samplers[0] = 0;
              break;
            }
            unsigned resolved = 0;
            rejects([&] {
              renderer.draw_packet(
                  bad, constants(dims[0], dims[1], 0),
                  [&](std::uint16_t) {
                    ++resolved;
                    return 0u;
                  },
                  64, 64);
            });
            require(!resolved, "Invalid packet reached texture resolver");
          }
        }
        first.rebuild(settings, sources(), dims[2]);
        if (overlay)
          require(packets->references(overlay) == 0,
                  "Rebuild did not return retained overlay");
        second.invalidate();
        first.invalidate();
        for (auto h : {lease.vertex_buffer(), lease.index_buffer(),
                       lease.control_buffer()})
          if (h)
            require(shared->shared_references(h) == 1,
                    "Cache lost its base reference");
        ++grid_index;
      }
    const auto partial = pool->begin_shared_grid(2, 3, 1, 10);
    rejects([&] { pool->buffers(partial, *context); });
    pool->retain_shared_buffer(partial, partial.vertex_buffer());
    rejects([&] { pool->buffers(partial, *context); });
    rejects(
        [&] { pool->retain_shared_buffer(partial, partial.vertex_buffer()); });
    rejects(
        [&] { pool->release_shared_buffer(partial, partial.index_buffer()); });
    pool->release(partial);
    rejects(
        [&] { pool->retain_shared_buffer(partial, partial.index_buffer()); });
    require(pool->shared_references(partial.vertex_buffer()) == 1 &&
                pool->shared_references(partial.index_buffer()) == 1 &&
                pool->shared_references(partial.control_buffer()) == 1,
            "Partial reservation rollback");
    for (unsigned i = 0; i < 4; ++i)
      packets->advance_frame();
    packets->collect();
    require(packets->retired().empty() &&
                packets->available().size() == packets->next_handle() - 1,
            "Patch destruction leaked packets");
    require(shared_draws == 108, "Shared resource draw coverage");
    std::ostringstream report;
    report.precision(17);
    report << context->description() << "\nPASS jobs=" << jobs
           << " pairedDraws=" << paired_draws << " sharedDraws=" << shared_draws
           << " guards=" << guards << " geometryBytes=" << geometry_bytes
           << " maximumError=" << maximum
           << " actualPatchOwner=true actualPackets=true "
              "originalFullShaders=true capability10Drawing=false\n";
    text(output / "execution.txt", report.str());
    std::cout << report.str();
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
