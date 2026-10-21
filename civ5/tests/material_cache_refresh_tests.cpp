#include "civ5/materials/material_cache_refresh.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::materials;
using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks = 0, compared = 0, samples = 0;
void check(bool ok, const char *why) { ++checks; if (!ok) throw std::runtime_error(why); }
void word(Bytes &b, std::uint32_t v, unsigned count = 4) {
    for (unsigned i = 0; i < count; ++i) b.push_back(std::uint8_t(v >> (8*i)));
}
struct Reader {
    Bytes bytes;
    std::size_t at = 0;
    std::uint32_t word(unsigned count = 4) {
        check(at+count <= bytes.size(), "Truncated refresh corpus");
        std::uint32_t value = 0;
        for (unsigned i = 0; i < count; ++i) value |= std::uint32_t(bytes[at++]) << (8*i);
        return value;
    }
    int integer() { return std::bit_cast<std::int32_t>(word()); }
    float scalar() { return std::bit_cast<float>(word()); }
    Bytes blob() {
        const auto size = word(); check(size <= bytes.size()-at, "Oversized refresh corpus blob");
        Bytes out(bytes.begin()+at, bytes.begin()+at+size); at += size; return out;
    }
    X87Precision precision() {
        const auto pc = word(); check(pc == 24 || pc == 53, "Refresh precision");
        return pc == 24 ? X87Precision::single : X87Precision::double_precision;
    }
    void end() { check(at == bytes.size(), "Trailing refresh corpus bytes"); }
};
void same(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b, const char *why) {
    check(std::equal(a.begin(), a.end(), b.begin(), b.end()), why); compared += a.size();
}
struct Backend : MaterialCacheRefreshBackend {
    std::span<MaterialCacheSchedulerPatch> patches;
    std::vector<MaterialSelection> selection;
    std::uint32_t revision, submit_increment, wait_increment;
    Bytes events;
    Backend(std::span<MaterialCacheSchedulerPatch> p, std::uint32_t stamp, std::uint32_t submit, std::uint32_t wait)
        : patches(p), revision(stamp), submit_increment(submit), wait_increment(wait) {
        std::array<std::uint32_t, 64> scores;
        for (unsigned i = 0; i < 64; ++i) scores[i] = 64-i;
        selection.assign(p.size(), MaterialSelection(scores));
    }
    std::uint32_t source_revision() const override { return revision; }
    void clear_selection(std::uint32_t id) override {
        const auto prior = selection[id].owner_words();
        selection[id].clear_sources();
        check(selection[id].owner_words() == prior, "Clearing selection changed retained words");
    }
    void submit_refresh(std::uint32_t id) override {
        word(events, 1); word(events, id); word(events, patches[id].source_revision);
        word(events, std::uint32_t(selection[id].sources().size())); word(events, patches[id].material_parent_flag);
        revision += submit_increment;
    }
    void wait_refresh() override {
        for (auto v : {2u, 0xffffffffu, revision, 0u, 0u}) word(events, v);
        revision += wait_increment;
    }
};
template <class F> void rejects(F f) {
    bool caught = false; try { f(); } catch (const std::exception &) { caught = true; }
    check(caught, "Invalid cache refresh input accepted");
}
void guards() {
    std::vector<MaterialCacheSchedulerPatch> patches(1);
    Backend backend(patches, 19, 0, 0); std::uint32_t stamp = 43;
    rejects([&] { material_cache_refresh_region({2, 1, 1, 1, 64, 64}, {0, 0}, {64, 64}, true,
        X87Precision::double_precision, patches, stamp, backend); });
    check(backend.events.empty() && stamp == 43 && patches[0].source_revision == 0, "Bad lattice changed owner");
    rejects([&] { material_cache_height_metrics(68, 67, {0, 0}, 64, 64, [](auto, auto) { return 0; }, X87Precision::single); });
    rejects([&] { material_cache_height_metrics(67, 67, {INT32_MAX, 0}, 64, 64, [](auto, auto) { return 0; }, X87Precision::single); });
    rejects([&] { material_cache_height_metrics(67, 67, {0, 0}, 64, 64, [](auto, auto) { return 65536; }, X87Precision::single); });
}
void native(const char *path) {
    std::ifstream in(path, std::ios::binary); check(bool(in), "Missing refresh corpus");
    Reader r{Bytes(std::istreambuf_iterator<char>(in), {})};
    check(r.bytes.size() >= 16 && std::string(r.bytes.begin(), r.bytes.begin()+8) == "C5MCR001", "Refresh corpus magic");
    r.at = 8; const auto regions = r.word(), metrics = r.word();
    for (unsigned i = 0; i < regions; ++i) {
        const auto pc = r.precision();
        const auto columns = r.word(), rows = r.word(); const bool update = r.word() != 0;
        const auto revision = r.word(), submit = r.word(), wait = r.word();
        MaterialCacheRefreshDomain domain{columns, rows, r.scalar(), r.scalar(), r.scalar(), r.scalar()};
        const Point2 lower{r.scalar(), r.scalar()}, upper{r.scalar(), r.scalar()};
        Reader expected{r.blob()}; std::uint32_t stamp = 0xaabbccdd;
        std::vector<MaterialCacheSchedulerPatch> patches(std::size_t(columns)*rows);
        for (unsigned j = 0; j < patches.size(); ++j) {
            patches[j].source_revision = 0x11223300+j; patches[j].material_parent_flag = 3+j;
            patches[j].priority = 199; patches[j].enabled = 89;
        }
        Backend backend(patches, revision, submit, wait);
        material_cache_refresh_region(domain, lower, upper, update, pc, patches, stamp, backend);
        check(stamp == expected.word(), "Original owner refresh stamp differs");
        same(backend.events, expected.blob(), "Original refresh submit/barrier order differs");
        Bytes final;
        for (unsigned j = 0; j < patches.size(); ++j) {
            word(final, patches[j].source_revision); word(final, std::uint32_t(backend.selection[j].sources().size()));
            word(final, patches[j].material_parent_flag);
            check(patches[j].priority == 199 && patches[j].enabled == 89, "Refresh changed unrelated scheduler state");
        }
        same(final, expected.blob(), "Original region patch state differs"); expected.end();
    }
    for (unsigned i = 0; i < metrics; ++i) {
        const auto pc = r.precision(); const auto w = r.integer(), h = r.integer(), columns = r.integer(), rows = r.integer();
        const MaterialOrigin origin{r.integer(), r.integer()}; Reader heights{r.blob()};
        std::vector<std::uint16_t> values;
        while (heights.at < heights.bytes.size()) values.push_back(std::uint16_t(heights.word(2)));
        check(values.size() == std::size_t(w)*h, "Wrong metric height count");
        Reader expected{r.blob()}; Bytes reads;
        const auto result = material_cache_height_metrics(columns, rows, origin, w, h, [&](int x, int y) {
            check(x >= 0 && x < w && y >= 0 && y < h, "Metric sample outside owner");
            word(reads, std::uint32_t(y*w+x)); return values[y*w+x];
        }, pc);
        check(std::bit_cast<std::uint32_t>(result.first) == expected.word(), "Original first height metric differs");
        check(std::bit_cast<std::uint32_t>(result.maximum) == expected.word(), "Original maximum height differs");
        check(result.category == expected.word(), "Original height category differs");
        same(reads, expected.blob(), "Original ordered height samples differ"); samples += reads.size()/4; expected.end();
    }
    r.end(); std::cout << "PASS refresh: " << regions << " region cases, " << metrics << " height cases, "
                       << samples << " ordered samples, " << compared << " compared bytes\n";
}
} // namespace
int main(int argc, char **argv) {
    try { guards(); if (argc == 2) native(argv[1]); else check(argc == 1, "Usage: refresh_tests [oracle.bin]");
        std::cout << "PASS refresh " << checks << " checks\n";
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
