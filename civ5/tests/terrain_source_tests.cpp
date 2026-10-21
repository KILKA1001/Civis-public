#include "civ5/terrain/terrain_occurrences.hpp"
#include <algorithm>
#include <future>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace {
int checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Terrain source check failed at line " + std::to_string(where.line()));
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected);
}
void put(Bytes &b, std::size_t at, std::uint32_t value, unsigned width = 4) {
    for (unsigned i = 0; i < width; ++i)
        b.at(at + i) = static_cast<std::uint8_t>(value >> (i * 8));
}
void append(Bytes &b, std::uint32_t value, unsigned width = 4) {
    const auto at = b.size();
    b.resize(at + width);
    put(b, at, value, width);
}
struct Fixture { Bytes bytes; std::size_t starts, byte_pool, u16_pool, records; };
Fixture fixture() {
    Bytes starts, bytes{0x77}, shorts{0x77, 0x88}, records;
    for (unsigned ordinal = 0; ordinal < 2; ++ordinal) {
        const unsigned size = ordinal ? 512 : 128;
        const auto first = starts.size() / 4;
        starts.resize(starts.size() + size * 4 * 4);
        for (unsigned grid = 0; grid < 4; ++grid)
            // Reverse physical storage: the next logical row is not a delimiter.
            for (unsigned y = size; y-- > 0;) {
                put(starts, (first + size * grid + y) * 4,
                    static_cast<std::uint32_t>(grid == 0 ? shorts.size() / 2 : bytes.size()));
                if (grid == 0) {
                    append(shorts, size + 3, 2);
                    append(shorts, 0xe000 + y + ordinal, 2);
                } else {
                    for (unsigned x = 0; x < size;) {
                        const auto length = std::min(255u, size - x + 3);
                        bytes.push_back(static_cast<std::uint8_t>(length));
                        bytes.push_back(static_cast<std::uint8_t>(grid * 67 + y + ordinal));
                        x += length;
                    }
                }
            }
        Bytes record(0x56c, 0);
        for (auto offset : {0x94, 0x98, 0x3c0, 0x3c4}) put(record, offset, 10);
        put(record, 0x3bc, ordinal + 23); // Retain serialized identity, never infer vector position from it.
        put(record, 0x3c8, ordinal + 1);
        put(record, 0x3d0, ordinal ? 0xbaadf00c : 0xbaadf00d);
        for (unsigned i = 0; i < 4; ++i)
            put(record, 0x3d4 + i * 4, static_cast<std::uint32_t>(first + size * i));
        put(record, 0x3e4, 0x1234, 2);
        put(record, 0x3e6, 0x5678, 2);
        put(record, 0x3e8, 0x9abc, 2);
        record[0x3ea] = 'A' + ordinal;
        for (unsigned y = 0; y < 10; ++y)
            for (unsigned x = 0; x < 10; ++x)
                put(record, 0x9c + (x + y * 10) * 8, 0xffffffffu);
        put(record, 0x9c, 1);
        put(record, 0x9c + 2 * 8, 1);
        put(record, 0x9c + 20 * 8, 1);
        record[0x9c + 5] = 1;
        record[0x9c + 11 * 8 + 5] = 255; // Sentinel mask with a claim is active.
        records.insert(records.end(), record.begin(), record.end());
    }
    Fixture f;
    append(f.bytes, 0x14);
    const auto add_pool = [&](const Bytes &b, unsigned unit) {
        append(f.bytes, static_cast<std::uint32_t>(b.size() / unit));
        const auto at = f.bytes.size();
        f.bytes.insert(f.bytes.end(), b.begin(), b.end());
        return at;
    };
    f.starts = add_pool(starts, 4);
    f.byte_pool = add_pool(bytes, 1);
    f.u16_pool = add_pool(shorts, 2);
    f.records = add_pool(records, 0x56c);
    return f;
}
void library() {
    auto f = fixture();
    const auto lib = TerrainPieceLibrary::parse(f.bytes);
    check(!lib.asset().has_value());
    check(std::equal(f.bytes.begin(), f.bytes.end(), lib.serialized_bytes().begin()));
    f.bytes.assign(f.bytes.size(), 0); // The caller's source buffer is not borrowed.
    check(lib.serialized_bytes()[0] == 0x14 && lib.records().size() == 2);
    for (unsigned i = 0; i < 2; ++i) {
        const auto &r = lib.record(i);
        check(r.ordinal == i && r.serialized_ordinal == i + 23 && r.generation_pass == i + 1);
        check(r.raw_state == (i ? 0xbaadf00c : 0xbaadf00d));
        check(r.result_code == 0x1234 && r.selection_key_a == 0x5678 && r.selection_key_b == 0x9abc);
        check(r.active_coordinates == std::vector<MarkupCoordinate>{{0, 0}, {0, 2}, {1, 1}, {2, 0}});
        check(r.markup_at({1, 1}).mask == 0xffffffffu && r.markup_at({1, 1}).claim == 255);
        check(lib.serialized_record(i).size() == 0x56c);
        rejects([&] { r.markup_at({10, 0}); });
        const auto grids = lib.grids(i);
        check(grids->size() == (i ? 512 : 128));
        const auto width = grids->size();
        for (int y = 0; y < width; ++y)
            for (int x = 0; x < width; ++x) {
                const auto n = y * width + x;
                check(grids->heights()[n] == 0xe000 + y + i);
                check(grids->materials()[n] == std::uint8_t(67 + y + i));
                check(grids->weights()[n] == std::uint8_t(134 + y + i));
                check(grids->priorities()[n] == std::uint8_t(201 + y + i));
            }
        check(lib.grids(i) == grids);
        auto copied = lib;
        check(copied.grids(i) == grids);
    }
    // Concurrent first decode must publish one immutable owned grid.
    const auto fresh = TerrainPieceLibrary::parse(fixture().bytes);
    std::vector<std::future<std::shared_ptr<const TerrainPieceGrids>>> reads;
    for (unsigned i = 0; i < 8; ++i)
        reads.push_back(std::async(std::launch::async, [&] { return fresh.grids(1); }));
    const auto grid = reads[0].get();
    for (unsigned i = 1; i < reads.size(); ++i)
        check(reads[i].get() == grid);
    rejects([&] { lib.record(2); });
    rejects([&] { lib.grids(2); });
    rejects([&] { lib.serialized_record(2); });
}
void malformed() {
    const auto f = fixture();
    for (auto n : {std::size_t(0), std::size_t(3), f.starts - 1, f.byte_pool - 1, f.u16_pool - 1,
                   f.records - 1, f.bytes.size() - 1})
        rejects([&] { TerrainPieceLibrary::parse(std::span(f.bytes).first(n)); });
    for (auto [offset, value] : std::vector<std::pair<std::size_t, std::uint32_t>>{
             {0, 0x13}, {4, 0xffffffff}, {f.byte_pool - 4, 0xffffffff}, {f.u16_pool - 4, 0xffffffff},
             {f.records - 4, 0xffffffff}, {f.records + 0x3d4, 0xffffffff}, {f.records + 0x3d8, 0},
             {f.records + 0x3dc, 255}, {f.records + 0x3e0, 0xffffffff}, {f.starts, 0xffffffff},
             {f.starts + 128 * 4, 0xffffffff}, {f.records + 0x3d0, 0}}) {
        auto b = f.bytes;
        put(b, offset, value);
        rejects([&] { TerrainPieceLibrary::parse(b); });
    }
    auto tail = f.bytes;
    tail.push_back(0);
    rejects([&] { TerrainPieceLibrary::parse(tail); });
    for (auto offset : {0x3ea, 0x42a, 0x46a, 0x4aa, 0x4ea, 0x52a}) {
        auto name = f.bytes;
        std::fill_n(name.begin() + f.records + offset, 64, 'x');
        rejects([&] { TerrainPieceLibrary::parse(name); });
        name[f.records + offset + 1] = 0;
        name[f.records + offset] = 128;
        rejects([&] { TerrainPieceLibrary::parse(name); });
    }
    // In-pool pointers alone do not prove valid RLE; malformed rows fail before publication.
    for (bool height : {false, true}) {
        auto zero = f.bytes;
        const auto table = f.starts + (height ? 0 : 128 * 4);
        put(zero, table, 1);
        put(zero, height ? f.u16_pool + 2 : f.byte_pool + 1, 0, height ? 2 : 1);
        const auto invalid = TerrainPieceLibrary::parse(zero);
        rejects([&] { invalid.grids(0); });
        rejects([&] { invalid.grids(0); });
        auto truncated = f.bytes;
        const auto remaining = height ? (f.records - 4 - f.u16_pool) / 2 : f.u16_pool - 4 - f.byte_pool;
        put(truncated, table, static_cast<std::uint32_t>(remaining - 1));
        const auto short_row = TerrainPieceLibrary::parse(truncated);
        rejects([&] { short_row.grids(0); });
    }
    Bytes empty;
    for (auto value : {0x14u, 0u, 0u, 0u, 0u}) append(empty, value);
    rejects([&] { TerrainPieceLibrary::parse(empty); });
}
IntMatrix4 identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }
TerrainPlacement placement(float translation = 0) {
    const Matrix4 cache{64, 0, 0, 0, 32, 64, 0, 0, 0, 0, 0, 0, translation, translation, 0, 1};
    const std::array<Vector4, 4> corners{{{0, 0, 0, 1}, {1, 2, -3, 1}, {0, 0, 0, 1}, {4, 8, -12, 1}}};
    return TerrainPlacement({identity(), identity(), identity(), identity()}, identity(), identity(), cache, corners);
}
void math() {
    const auto p = placement();
    const TerrainOccurrence o{0, 0, {-2, 3, -1}};
    auto r = p.rectangle(o, 128);
    check(r.x0 == -32 && r.y0 == 192 && r.x1 == 96 && r.y1 == 320);
    r = p.rectangle(o, 512);
    check(r.x1 == 480 && r.y1 == 704);
    for (float value : {-.5f, .5f}) {
        const auto rect = placement(value).rectangle({0, 0, {0, 0, 0}}, 128);
        check(rect.x0 == (value < 0 ? -1 : 1) && rect.y0 == rect.x0);
    }
    check(p.logical_cell({0, 0, {3, -2, -1}}, {0, 0}) == LogicalCellCoordinate{2, -2});
    check(p.logical_cell({0, 0, {3, -2, -1}}, {0, 1}) == LogicalCellCoordinate{2, -1});
    const auto candidates = p.candidate_positions(2, 3);
    check(candidates.size() == 42 && candidates.front() == LogicalHex{-1, -2, 3} &&
          candidates.back() == LogicalHex{1, 4, -5});
    for (auto c : candidates) check(wrap_add(wrap_add(c.x, c.y), c.z) == 0);
    // IMUL/ADD are deliberately modulo-32-bit, including the translation.
    check(p.logical_cell({0, 0, {std::numeric_limits<std::int32_t>::max(), 0, 0}}, {1, 0}) ==
          LogicalCellCoordinate{std::numeric_limits<std::int32_t>::min(), 0});
    rejects([&] { p.rectangle(o, 256); });
    rejects([&] { p.rectangle({0, 4, {0, 0, 0}}, 128); });
    rejects([&] { p.logical_cell({0, 4, {0, 0, 0}}, {0, 0}); });
    rejects([&] { p.logical_cell(o, {0, 10}); });
    rejects([&] { p.rectangle({0, 0, {std::numeric_limits<std::int32_t>::max(), 0, 0}}, 128); });
    for (auto [w, h] : {std::pair{0, 1}, {-1, 1}, {1, 0}, {1, -1}, {10000, 10000}, {2147483647, 1}})
        rejects([&] { p.candidate_positions(w, h); });
    rejects([] { placement(std::numeric_limits<float>::quiet_NaN()); });
    rejects([] { placement(std::numeric_limits<float>::infinity()); });
    auto bad_corners = std::array<Vector4, 4>{};
    rejects([&] { TerrainPlacement({identity(), identity(), identity(), identity()}, identity(), identity(),
                                  identity_matrix(), bad_corners); });
}
void descriptors_and_claims() {
    const auto lib = TerrainPieceLibrary::parse(fixture().bytes);
    const auto p = placement();
    const std::array<std::uint32_t, 2> mapping{1, 0};
    const std::array<TerrainOccurrence, 3> occurrences{{{0, 0, {0, 0, 0}}, {1, 1, {-2, 3, -1}}, {0, 2, {2, 0, -2}}}};
    const auto ds = build_terrain_descriptors(lib, p, occurrences, mapping);
    check(ds.size() == 3 && ds[0].grids->size() == 512 && ds[1].grids->size() == 128);
    check(ds[0].grids == ds[2].grids && ds[0].grids != ds[1].grids);
    check(ds[0].cell_gate == 0x1234 && ds[0].record_state_flag == 0 && ds[1].record_state_flag == 1);
    rejects([&] { build_terrain_descriptors(lib, p, occurrences, {}); });
    const std::array<std::uint32_t, 2> invalid{2, 0};
    rejects([&] { build_terrain_descriptors(lib, p, occurrences, invalid); });
    check(terrain_markup_matches({0, 4, 1}, 8, 4));
    check(terrain_markup_matches({0, 4, 1}, 8, 5));
    check(terrain_markup_matches({8, 4, 1}, 0, 5));
    check(terrain_markup_matches({1, 0, 1}, 0, 2));
    check(terrain_markup_matches({1, 4, 1}, 3, 0));
    check(!terrain_markup_matches({1, 4, 1}, 2, 4));
    check(!terrain_markup_matches({1, 4, 1}, 1, 5));
    Bytes cells(4 * 3 * 8, 0);
    for (std::size_t i = 0; i < cells.size(); i += 8) {
        put(cells, i, 1);
        cells[i + 6] = 0x98;
        cells[i + 7] = 0x76;
    }
    TerrainLogicalClaims claims(4, 3, cells);
    check(claims.try_claim(lib.record(0), {0, 0, {0, 0, 0}}, p));
    check(claims.claim_count() == 2 && claims.cells()[5] == 1 && claims.cells()[(1 * 3 + 1) * 8 + 5] == 1);
    const Bytes accepted(claims.cells().begin(), claims.cells().end());
    check(!claims.try_claim(lib.record(0), {0, 0, {0, 0, 0}}, p));
    check(!claims.try_claim(lib.record(0), {0, 0, {-100, -100, 200}}, p));
    check(std::equal(accepted.begin(), accepted.end(), claims.cells().begin()) && claims.claim_count() == 2);
    auto mismatched = lib.record(0);
    mismatched.markup[11 * 8] = 2;
    for (unsigned i = 1; i < 4; ++i) mismatched.markup[11 * 8 + i] = 0;
    TerrainLogicalClaims rejected(4, 3, cells);
    check(!rejected.try_claim(mismatched, {0, 0, {0, 0, 0}}, p));
    check(std::equal(cells.begin(), cells.end(), rejected.cells().begin()) && rejected.claim_count() == 0);
    // Clipped outside markup is ignored while an inside claim still succeeds.
    check(rejected.try_claim(lib.record(0), {0, 0, {3, 0, -3}}, p));
    check(rejected.claim_count() == 1);
    for (std::size_t i = 0; i < cells.size(); ++i)
        if (i % 8 != 5) check(rejected.cells()[i] == cells[i]);
    rejects([&] { TerrainLogicalClaims(0, 3, cells); });
    rejects([&] { TerrainLogicalClaims(4, 4, cells); });
    rejects([&] { TerrainLogicalClaims(2147483647, 2147483647, cells); });
}
class MissingSource final : public civ5::assets::AssetSource {
  public:
    explicit MissingSource(bool ambiguous) : ambiguous_(ambiguous) {}
    civ5::assets::AssetLookup find(const civ5::assets::AssetKey &key) const override {
        if (ambiguous_) return civ5::assets::AmbiguousAsset{key, provenance(), {}};
        return {};
    }
    civ5::assets::Provenance provenance() const override { return {civ5::assets::SourceKind::loose, "test", ".", {}}; }
  private:
    bool ambiguous_;
};
void sources() {
    for (bool ambiguous : {false, true}) {
        MissingSource source(ambiguous);
        rejects([&] { TerrainPieceLibrary::load(source, civ5::assets::AssetKey("TerrainPiecesLibrary.fxsb")); });
        rejects([&] { TerrainPieceLibrary::load(source, civ5::assets::AssetKey("../outside.fxsb")); });
    }
}
void generation() {
    const auto lib = TerrainPieceLibrary::parse(fixture().bytes);
    const auto p = placement();
    const std::array<std::uint32_t, 2> mapping{1, 0};
    const Bytes input(4 * 3 * 8, 0);
    TerrainRandom random(1);
    int passes = 0;
    const auto result = generate_terrain_occurrences(lib, p, mapping, 4, 3, input, 2, random,
        [&](auto pass, auto occurrences, const auto &claims, const auto &rng) {
            check(pass == 2 - passes++ && rng.calls() == std::uint64_t(passes * 279));
            for (auto o : occurrences) check(lib.record(mapping[o.cooked_index]).generation_pass >= std::uint32_t(pass));
            check(claims.cells().size() == input.size());
        });
    check(passes == 3 && random.calls() == 837 && !result.occurrences.empty());
    check(std::all_of(input.begin(), input.end(), [](auto v) { return v == 0; }));
    check(result.claims.claim_count() > 0 && result.claims.claim_count() <= 12);
    check(build_terrain_descriptors(lib, p, result.occurrences, mapping).size() == result.occurrences.size());
    // A shared generator continues its stream even when every attempted cell
    // is already claimed. Invalid requests must not consume that stream.
    Bytes occupied = input;
    for (std::size_t i = 5; i < occupied.size(); i += 8) occupied[i] = 7;
    const auto rejected = generate_terrain_occurrences(lib, p, mapping, 4, 3, occupied, 2, random);
    check(random.calls() == 1674 && rejected.occurrences.empty() && rejected.claims.claim_count() == 0);
    check(std::equal(occupied.begin(), occupied.end(), rejected.claims.cells().begin()));
    const auto state = random.state();
    const auto skipped = generate_terrain_occurrences(lib, p, mapping, 4, 3, input, -1, random);
    check(skipped.occurrences.empty() && random.state() == state && random.calls() == 1674);
    for (auto bad : {std::array<std::uint32_t, 2>{0, 2}, {3, 0}})
        rejects([&] { generate_terrain_occurrences(lib, p, bad, 4, 3, input, 2, random); });
    rejects([&] { generate_terrain_occurrences(lib, p, mapping, 0, 3, input, 2, random); });
    rejects([&] { generate_terrain_occurrences(lib, p, mapping, 4, 3, input, 256, random); });
    rejects([&] { random.index(0); });
    check(random.state() == state && random.calls() == 1674);
    const auto empty = generate_terrain_occurrences(lib, p, {}, 4, 3, input, 0, random);
    check(empty.occurrences.empty() && random.calls() == 1674 + 223);
}
} // namespace
int main() {
    try {
        library(); malformed(); math(); descriptors_and_claims(); sources(); generation();
        std::cout << "PASS " << checks << " terrain source/placement checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
