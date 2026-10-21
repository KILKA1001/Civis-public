// Validation executable only. Production has no corpus, file, pointer-replay,
// Python, JVM, JSON or original-game dependency.
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/land_membership.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>

using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace {
void check(bool ok, const std::string &label) {
    if (!ok)
        throw std::runtime_error(label);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    std::span<const std::uint8_t> take(std::size_t count) {
        check(count <= bytes.size() - at, "Truncated fixture");
        auto result = bytes.subspan(at, count);
        at += count;
        return result;
    }
    std::uint32_t u32() {
        auto b = take(4);
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t{b[i]} << (i * 8);
        return value;
    }
    std::int32_t i32() { return std::bit_cast<std::int32_t>(u32()); }
    float f32() { return std::bit_cast<float>(u32()); }
    template <std::size_t N> std::array<std::int32_t, N> ints() {
        std::array<std::int32_t, N> result;
        for (auto &v : result)
            v = i32();
        return result;
    }
    template <std::size_t N> std::array<float, N> floats() {
        std::array<float, N> result;
        for (auto &v : result)
            v = f32();
        return result;
    }
    Bytes blob() {
        auto count = u32();
        auto b = take(count);
        return {b.begin(), b.end()};
    }
    void end() const { check(at == bytes.size(), "Trailing fixture data"); }
};
struct Writer {
    Bytes bytes;
    void raw(std::span<const std::uint8_t> b) { bytes.insert(bytes.end(), b.begin(), b.end()); }
    void word(std::uint32_t v) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void scalar(float v) { word(std::bit_cast<std::uint32_t>(v)); }
    template <class T> void words(const T &v) {
        for (auto x : v)
            word(static_cast<std::uint32_t>(x));
    }
    template <class T> void floats(const T &v) {
        for (auto x : v)
            scalar(x);
    }
    void blob(std::span<const std::uint8_t> v) {
        check(v.size() <= 0xffffffffu, "Oversized output");
        word(static_cast<std::uint32_t>(v.size()));
        raw(v);
    }
};
X87Precision precision(Reader &r) {
    auto v = r.i32();
    check(v == 24 || v == 53, "Unknown precision");
    return v == 24 ? X87Precision::single : X87Precision::double_precision;
}
MatrixPath matrix_path(Reader &r) {
    auto v = r.u32();
    check(v <= 2, "Unknown matrix path");
    return static_cast<MatrixPath>(v);
}
WorldLayout world_input(Reader &r) {
    const auto dims = r.ints<3>();
    const auto observed = r.ints<4>();
    std::size_t calls = 0;
    const auto world = build_world_layout(dims[0], dims[1], dims[2], {}, [&](float det) {
        check(calls < 2 && std::bit_cast<std::int32_t>(det) == observed[calls * 2],
              "World determinant mismatch");
        return std::bit_cast<float>(observed[calls++ * 2 + 1]);
    });
    check(calls == 2, "World inverse call count");
    return world;
}
void grid(Writer &out, const WorldGrid &g) {
    out.words(g.first_matrix);
    out.words(g.second_matrix);
    out.words(g.descriptor);
    out.word(g.width);
    out.word(g.height);
}
Bytes patches(const LandMembership &membership) {
    Writer out;
    for (const auto &members : membership.patches) {
        check(members.size() <= 16, "Fixed native patch overflow");
        out.word(static_cast<std::uint32_t>(members.size()));
        for (const auto &h : members)
            out.words(std::array{h.x, h.y, h.z});
        out.bytes.insert(out.bytes.end(), (16 - members.size()) * 12, 0xa5);
    }
    return out.bytes;
}
std::array<std::uint32_t, 3> membership_stats(const LandMembership &result) {
    std::array<std::uint32_t, 3> v{};
    for (const auto &patch : result.patches) {
        v[0] += static_cast<std::uint32_t>(patch.size());
        v[1] = std::max(v[1], static_cast<std::uint32_t>(patch.size()));
        v[2] += patch.empty();
    }
    return v;
}
Matrix4 world_inverse(const WorldLayout &world) {
    Matrix4 result;
    std::copy_n(world.transforms.begin() + 16, 16, result.begin());
    return result;
}
LandTraversalExtent traversal(const WorldLayout &world) {
    const auto &b = world.bounds;
    return land_traversal_extent(b[0], b[1], b[3], b[4], 1, .866f);
}
LandMembership world_membership(const WorldLayout &world, const LandLayout &layout,
                                LandTraversalExtent extent) {
    return build_land_membership(layout, extent.width, extent.height, world.source_grid.second_matrix,
                                 world_inverse(world), {layout.center_x(), layout.center_y()},
                                 X87Precision::double_precision, MatrixPath::sse);
}
void store(Bytes &target, std::size_t offset, const Writer &value) {
    check(offset <= target.size() && value.bytes.size() <= target.size() - offset,
          "Owner serialization bounds");
    std::copy(value.bytes.begin(), value.bytes.end(), target.begin() + offset);
}
void owner_lifecycles(Reader &r, Writer &out) {
    const auto world = world_input(r);
    const auto count = r.u32();
    check(count == 11, "Incomplete owner corpus");
    std::optional<HeightOwnerState> previous;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto v = r.ints<12>();
        const bool fresh = v[0] != 0, native = v[1] != 0;
        if (fresh)
            previous.reset();
        auto cells = r.blob(), before = r.blob();
        const auto pointers = r.ints<4>();
        const auto p = native ? HeightOwnerParameters::from_world(world)
                              : HeightOwnerParameters::native_call(v[5], v[6]);
        const auto sw = native ? world.worker_grid.width : v[7],
                   sh = native ? world.worker_grid.height : v[8];
        const std::array<std::int32_t, 3> t =
            native ? std::array{world.worker_grid.descriptor[0], world.worker_grid.descriptor[1],
                                world.worker_grid.descriptor[2]}
                   : std::array{v[9], v[10], v[11]};
        check(p.width == v[5] && p.height == v[6] && sw == v[7] && sh == v[8] &&
                  t == std::array{v[9], v[10], v[11]},
              "Computed world owner inputs");
        const auto prior_styles =
            previous ? previous->styles
                     : LandStyleGrid(Bytes(LandStyleGrid::size, static_cast<std::uint8_t>(v[2])));
        auto poison = [&](const PageStorage &field) {
            return field.with_pool_bytes(
                Bytes(static_cast<std::size_t>(field.capacity_pages()) * 256 * field.element_bytes(),
                      static_cast<std::uint8_t>(v[4])));
        };
        std::optional<PageStorage> h, m;
        if (previous) {
            h = poison(previous->height);
            m = poison(previous->material);
        }
        std::vector<std::int32_t> allocations;
        auto result = initialize_height_owner(p, sw, sh, cells, t, prior_styles, h ? &*h : nullptr,
                                              m ? &*m : nullptr, [&](std::int32_t size) {
                                                  allocations.push_back(size);
                                                  return Bytes(size, static_cast<std::uint8_t>(v[3]));
                                              });
        out.word(static_cast<std::uint32_t>(allocations.size()));
        out.words(allocations);
        auto owner = before;
        for (int field_index = 0; field_index < 2; ++field_index) {
            const auto &f = field_index == 0 ? result.height : result.material;
            const std::size_t offset = field_index == 0 ? 0xc : 0x4c;
            Writer descriptor;
            // These two 32-bit words are OBSERVED allocator identities in the
            // original serialized record, never addresses in our 64-bit heap.
            descriptor.words(std::array{f.domain().width(), f.domain().height(), f.page_columns(),
                                        f.page_rows(), pointers[field_index * 2], 0xfffffff, f.active_pages(),
                                        pointers[field_index * 2 + 1], f.active_pages(), f.capacity_pages()});
            store(owner, offset, descriptor);
            out.words(
                std::array{f.domain().width(), f.domain().height(), f.active_pages(), f.capacity_pages()});
            out.blob(f.header_bytes());
            out.blob(f.pool_bytes());
            if (previous) {
                Reader old{before};
                old.take(offset + 0x1c);
                const auto old_pointer = old.i32();
                const auto old_capacity = field_index == 0 ? previous->height.capacity_pages()
                                                           : previous->material.capacity_pages();
                check((old_pointer == pointers[field_index * 2 + 1]) == (old_capacity == f.capacity_pages()),
                      "Observed pool pointer reuse");
            }
        }
        Writer factor;
        factor.scalar(p.factor);
        store(owner, 0, factor);
        check(owner.size() > 9, "Owner flags bounds");
        owner[8] = p.enabled;
        owner[9] = 1;
        Writer normalized;
        normalized.floats(std::array{0.0f, 0.0f, static_cast<float>(p.width), static_cast<float>(p.height),
                                     result.normalized_scale_x(), result.normalized_scale_y()});
        store(owner, 0x34, normalized);
        Writer parameters;
        parameters.word(p.width);
        parameters.word(p.height);
        parameters.floats(std::array{p.scale_x, p.scale_y, p.scale_z, p.center_x, p.center_y, p.center_z,
                                     p.world_span_x, p.world_span_y});
        parameters.words(result.first_matrix);
        parameters.words(result.second_matrix);
        parameters.words(result.coordinate_descriptor);
        store(owner, 0xa0, parameters);
        Writer styles;
        styles.raw(result.styles.owner_bytes());
        store(owner, 0x164, styles);
        out.blob(owner);
        if (native) {
            for (const auto range : {std::pair{0x34, 0x4c}, std::pair{0xa0, 0xc8}, std::pair{0xc8, 0x148}})
                out.raw(std::span{owner}.subspan(range.first, range.second - range.first));
            auto field = result.height.height_field();
            LandLayout layout(field.domain());
            check(layout.center_x() == p.center_x && layout.center_y() == p.center_y &&
                      layout.columns() * layout.rows() == 1120,
                  "Owner to LAND frame");
            for (int y = 0; y < p.height; ++y)
                for (int x = 0; x < p.width; ++x)
                    check(field.u16_at(x, y) == 0 && result.material.value_at(x, y) == 0,
                          "Initialized full field value");
            for (int x = 0; x < layout.columns(); ++x)
                for (int y = 0; y < layout.rows(); ++y) {
                    auto patch = layout.patch(x, y);
                    const auto translation = patch.world_translation();
                    check(field.a8_at(patch.source_x(), patch.source_y()) == 0 &&
                              std::bit_cast<std::uint32_t>(translation[0]) ==
                                  std::bit_cast<std::uint32_t>(-p.center_x) &&
                              std::bit_cast<std::uint32_t>(translation[1]) ==
                                  std::bit_cast<std::uint32_t>(-p.center_y) &&
                              std::bit_cast<std::uint32_t>(translation[2]) == 0x80000000u,
                          "Initialized LAND addressing");
                }
        }
        previous = std::move(result);
    }
}
Bytes calculate(std::uint32_t op, std::span<const std::uint8_t> inputs) {
    Reader r{inputs};
    Writer out;
    switch (op) {
    case 1: {
        auto w = world_input(r);
        out.floats(w.transforms);
        out.floats(w.bounds);
        grid(out, w.source_grid);
        grid(out, w.worker_grid);
        break;
    }
    case 2: {
        auto input = r.floats<16>();
        const auto det = r.u32(), seed = r.u32();
        int calls = 0;
        out.floats(inverse_sse(input, [&](float actual) {
            ++calls;
            check(std::bit_cast<std::uint32_t>(actual) == det, "Inverse determinant");
            return std::bit_cast<float>(seed);
        }));
        check(calls == 1, "Inverse call count");
        break;
    }
    case 3: {
        const auto dimensions = r.ints<4>();
        const auto pc = precision(r);
        const auto path = matrix_path(r);
        const bool native = r.u32() != 0;
        const auto supplied_matrix = r.ints<16>();
        const auto world = r.floats<16>();
        const auto center = r.floats<2>();
        const auto matrix = native ? unshifted_global_map_matrix({}) : supplied_matrix;
        LandLayout layout(HeightDomain(dimensions[0], dimensions[1]));
        auto result = build_land_membership(layout, dimensions[2], dimensions[3], matrix, world,
                                            {center[0], center[1]}, pc, path);
        out.word(layout.columns());
        out.word(layout.rows());
        out.floats(std::array{layout.center_x(), layout.center_y(), 0.0f, static_cast<float>(dimensions[0]),
                              static_cast<float>(dimensions[1]) * .866f});
        out.words(matrix);
        out.blob(patches(result));
        Writer trace;
        for (const auto &h : result.hexes) {
            trace.words(std::array{h.map_x, h.map_y, h.logical.x, h.logical.y, h.logical.z});
            trace.scalar(h.uncentered.x);
            trace.scalar(h.uncentered.y);
            for (const auto &c : h.corners) {
                trace.scalar(c.x);
                trace.scalar(c.y);
            }
            trace.word(h.candidate_x);
            trace.word(h.candidate_y);
            for (const auto &c : h.candidates) {
                trace.word(c.patch_index);
                trace.word(c.intersects);
            }
        }
        out.blob(trace.bytes);
        out.words(membership_stats(result));
        break;
    }
    case 4: {
        auto b = r.floats<6>();
        auto v = r.floats<4>();
        out.word(edge_intersects(b, {v[0], v[1]}, {v[2], v[3]}));
        break;
    }
    case 5: {
        const auto pc = precision(r);
        const auto path = matrix_path(r);
        auto a = r.floats<16>(), b = r.floats<16>();
        out.floats(matrix_multiply(a, b, pc, path));
        break;
    }
    case 6: {
        auto w = world_input(r);
        auto extent = traversal(w);
        auto p = HeightOwnerParameters::from_world(w);
        LandLayout layout(HeightDomain(p.width, p.height));
        auto result = world_membership(w, layout, extent);
        out.words(std::array{extent.width, extent.height, p.width, p.height});
        out.blob(patches(result));
        const auto stats = membership_stats(result);
        out.word(stats[0]);
        out.word(stats[1]);
        break;
    }
    case 7: {
        auto w = world_input(r);
        const auto source = r.ints<2>();
        auto cells = r.blob();
        const auto count = r.u32();
        check(count == 4, "Incomplete style corpus");
        const auto p = HeightOwnerParameters::from_world(w);
        LandLayout layout(HeightDomain(p.width, p.height));
        const auto extent = traversal(w);
        const auto membership = world_membership(w, layout, extent);
        out.words(std::array{p.width, p.height, extent.width, extent.height});
        out.words(w.source_grid.second_matrix);
        out.scalar(layout.center_x());
        out.scalar(layout.center_y());
        out.floats(world_inverse(w));
        out.blob(patches(membership));
        for (std::uint32_t i = 0; i < count; ++i) {
            auto prior = r.blob();
            const auto initial = r.u32();
            const auto g = LandStyleGrid(prior).copy_source_cells(source[0], source[1], cells);
            const auto result =
                accumulate_land_styles(membership.patches, w.worker_grid.first_matrix, g,
                                       std::vector<std::uint32_t>(membership.patches.size(), initial));
            out.words(w.worker_grid.first_matrix);
            out.blob(g.owner_bytes());
            out.words(result.masks);
            out.words(result.ordinary_styles);
            Writer trace;
            std::uint32_t outside = 0;
            for (const auto &read : result.reads) {
                trace.words(std::array{read.patch_index, read.member_index, read.transformed.x,
                                       read.transformed.y, read.transformed.z, read.flat_address,
                                       read.style});
                outside += read.flat_address % 255 >= source[0] || read.flat_address / 255 >= source[1];
            }
            out.blob(trace.bytes);
            out.word(outside);
            out.word(static_cast<std::uint32_t>(result.reads.size()));
        }
        break;
    }
    case 8: {
        auto v = r.floats<6>();
        auto extent = land_traversal_extent(v[0], v[1], v[2], v[3], v[4], v[5]);
        out.word(extent.width);
        out.word(extent.height);
        break;
    }
    case 9:
        owner_lifecycles(r, out);
        break;
    default:
        throw std::runtime_error("Unknown fixture operation");
    }
    r.end();
    return out.bytes;
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 3, "Usage: civ5_world_land_oracle fixture.bin fresh-output.bin");
        const std::filesystem::path input = argv[1], output = argv[2];
        check(!std::filesystem::exists(output), "Refusing to overwrite output");
        check(std::filesystem::file_size(input) <= 128u * 1024u * 1024u, "Oversized fixture");
        std::ifstream stream(input, std::ios::binary);
        check(bool(stream), "Cannot open fixture");
        Bytes file{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        Reader r{file};
        auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5WLOR01", "Wrong fixture magic");
        const auto count = r.u32();
        check(count > 0 && count <= 1000, "Invalid record count");
        Writer all;
        all.word(count);
        std::size_t compared = 0;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto op = r.u32();
            auto label = r.blob();
            check(!label.empty() && label.size() <= 1024, "Invalid label");
            std::string name(label.begin(), label.end());
            auto inputs = r.blob();
            auto expected = r.blob();
            Bytes actual;
            try {
                actual = calculate(op, inputs);
            } catch (const std::exception &e) {
                throw std::runtime_error(name + ": " + e.what());
            }
            check(actual.size() == expected.size(),
                  name + ": output size actual=" + std::to_string(actual.size()) +
                      " expected=" + std::to_string(expected.size()));
            const auto mismatch = std::mismatch(actual.begin(), actual.end(), expected.begin());
            if (mismatch.first != actual.end())
                throw std::runtime_error(name + ": byte " + std::to_string(mismatch.first - actual.begin()) +
                                         " actual=" + std::to_string(*mismatch.first) +
                                         " expected=" + std::to_string(*mismatch.second));
            compared += actual.size();
            all.word(op);
            all.blob(label);
            all.blob(actual);
            std::cout << "PASS " << name << " / " << actual.size() << " bytes\n";
        }
        r.end();
        std::ofstream target(output, std::ios::binary);
        check(bool(target), "Cannot create output");
        target.write(reinterpret_cast<const char *>(all.bytes.data()),
                     static_cast<std::streamsize>(all.bytes.size()));
        target.close();
        check(bool(target), "Output write failed");
        std::cout << "PASS original world/LAND corpus: " << count << " records / " << compared
                  << " bytes / 64-bit runtime\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
