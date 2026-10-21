#include "civ5/terrain/coast_curves.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Coast curve check failed at line " + std::to_string(where.line()));
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
bool bits(float a, float b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
bool same(CoastControlPoint a, CoastControlPoint b) {
    return bits(a.x, b.x) && bits(a.y, b.y) && bits(a.transition, b.transition) && a.style_a == b.style_a &&
           a.style_b == b.style_b;
}
CoastProfileRecord profile(int key, float offset = 0.f, int character = -1) {
    CoastProfileRecord p{key, {}, {-1, -1, -1, -1}};
    p.nodes.fill({offset, 1.f, .5f, character});
    return p;
}
CoastCurveState state(X87Precision pc = X87Precision::double_precision) {
    return {false, Point2(8.f, 0.f), 0x12345678, {}, 0x110000, 0x100000, pc};
}
std::vector<CoastControlPoint> straight(std::size_t count) {
    std::vector<CoastControlPoint> result;
    for (std::size_t i = 0; i < count; ++i)
        result.push_back({float(i) * 4.f, 0.f, .25f, 1, 2});
    return result;
}
void profiles() {
    const std::array<std::string_view, 13> names{
        "Default", "Grass",      "GrassHill", "Plains",     "PlainsHill", "Snow",           "SnowHill",
        "Desert",  "DesertHill", "Tundra",    "TundraHill", "Mountain",   "DefaultShallows"};
    for (int i = 0; i < 13; ++i)
        check(coast_style_key(names[i]) == i);
    check(coast_style_key("grass") == -1 && coast_style_key("unknown") == -1);
    CoastStyleSource source{"Grass", 0.f, 7.f, {.7f, -0.f, 65535, 65535}, {-.2f, 3.f, 2, 5}, {.1f, 4.f, 1}};
    const auto compiled = compile_coast_profile(source);
    check(compiled.key == 1 && compiled.materials == std::array<std::int32_t, 4>{-1, -1, 5, -1});
    check(std::bit_cast<std::uint32_t>(compiled.nodes[0].offset) == 0x80000000u);
    check(std::bit_cast<std::uint32_t>(compiled.nodes[1].offset) == 0u);
    check(compiled.nodes[0].character == -1 && compiled.nodes[1].character == -1 &&
          compiled.nodes[4].character == -1);
    check(compiled.nodes[0].height == .7f && compiled.nodes[4].height == -.2f);
    check(compiled.nodes[2].offset == 4.f && compiled.nodes[2].height == .1f &&
          compiled.nodes[2].character == 1);
    for (int i = 0; i < 5; ++i)
        check(compiled.nodes[i].opacity == (i == 0 || i == 4 ? 0.f : 1.f));
    auto unknown = source;
    unknown.type = "not-a-style";
    check(compile_coast_profile(unknown).key == -1);
    for (float value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        auto invalid = source;
        invalid.high.offset = value;
        rejects([&] { compile_coast_profile(invalid); });
        invalid = source;
        invalid.smoothing_low = value;
        rejects([&] { compile_coast_profile(invalid); });
        invalid = source;
        invalid.water.height = value;
        rejects([&] { compile_coast_profile(invalid); });
    }
    auto first = profile(1, 2), last = profile(1, 3);
    last.materials = {-1, 5, 35, 0};
    std::vector<CoastProfileRecord> records{first, last};
    const CoastProfiles styles(records);
    records[1].nodes[0].offset = 100;
    check(styles.records()[1].nodes[0].offset == 3);
    std::vector<std::uint32_t> palette(512, 0xffffffffu);
    palette[255] = 0x110005;
    palette[4] = 0x100005;
    palette[511] = 0x110023;
    palette[9] = 0x100023;
    palette[256] = 0x110000;
    auto resolved = styles.resolve(1, palette, 0x110000, 0x100000);
    check(resolved.nodes[0].offset == 3 && resolved.materials == std::array<std::int32_t, 4>{-1, 4, 9, 0});
    palette[0] = 0x110005;
    check(styles.resolve(1, palette, 0x110000, 0x100000).materials[1] == 0);
    resolved = styles.resolve(250, {}, 0x110000, 0x100000);
    check(resolved.key == 0 && resolved.materials == std::array<std::int32_t, 4>{255, 255, 255, 255});
    for (auto node : resolved.nodes)
        check(node.offset == 0 && node.opacity == 0 && node.height == 0 && node.character == 0);
    records[0].nodes[0].height = std::numeric_limits<float>::infinity();
    rejects([&] { CoastProfiles invalid(records); });
    records.assign(4097, profile(0));
    rejects([&] { CoastProfiles invalid(records); });
    palette.assign(65537, 0);
    rejects([&] { styles.resolve(0, palette, 0, 0); });
}
void characters() {
    std::vector<CoastCharacterSourcePoint> source;
    for (int i = 0; i < 100; ++i)
        source.push_back({float(i), 0.f, float(i) * 3.f});
    for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
        const auto table = compile_coast_character(source, pc);
        check(table.size() == 98);
        for (auto p : table)
            check(p.tangent == 0.f && p.normal == 0.f);
        auto changed = source;
        for (int i = 0; i < 98; ++i)
            changed[i].z = -float(i) * 109.f;
        changed[98] = {std::numeric_limits<float>::quiet_NaN(), 0.f, 0.f};
        changed[99] = {0.f, std::numeric_limits<float>::infinity(), 0.f};
        const auto ignored = compile_coast_character(changed, pc);
        for (std::size_t i = 0; i < table.size(); ++i)
            check(bits(table[i].tangent, ignored[i].tangent) && bits(table[i].normal, ignored[i].normal));
        changed = source;
        for (auto &p : changed)
            p.x = -p.x;
        for (auto p : compile_coast_character(changed, pc))
            check(p.tangent == 0.f && p.normal == 0.f);
    }
    rejects([&] { compile_coast_character(std::span(source).first(97), X87Precision::single); });
    rejects([&] { compile_coast_character(source, static_cast<X87Precision>(2)); });
    auto changed = source;
    changed[0].z = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { compile_coast_character(changed, X87Precision::single); });
    changed = source;
    changed[2].x = std::numeric_limits<float>::infinity();
    rejects([&] { compile_coast_character(changed, X87Precision::double_precision); });
    changed.assign(98, {0.f, 0.f, 0.f});
    rejects([&] { compile_coast_character(changed, X87Precision::double_precision); });
}
void curves() {
    std::vector<CoastControlPoint> sample{
        {0, 0, 0, 7, 3}, {2, 0, .25f, 9, 4}, {8, 0, .75f, 1, 2}, {10, 0, 1, 3, 4}};
    const auto midpoint = sample_coast_curve(sample, .5f);
    check(midpoint.x == 2.5f && midpoint.style_a == 7 && midpoint.style_b == 3);
    check(sample_coast_curve(sample, .5f, true).x == 4.f);
    const auto terminal = sample_coast_curve(sample, 2.5f);
    check(terminal.x == 9.f && terminal.style_a == 9 && terminal.style_b == 4);
    check(same(terminal, sample_coast_curve(sample, 2.001f)));
    for (auto value :
         {-1.f, 3.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
        rejects([&] { sample_coast_curve(sample, value); });
    rejects([&] { sample_coast_curve(std::span(sample).first(2), 0.f); });
    sample[1].transition = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { sample_coast_curve(sample, 0.f); });
    const std::array records{profile(1), profile(2)};
    const CoastProfiles styles(records);
    for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
        const auto points = straight(4);
        const auto built = build_coast_curves(points, styles, {}, state(pc));
        check(built.control_count() == 18 && built.owner_flag() == 0x12345678);
        for (const auto &curve : built.curves())
            for (std::size_t i = 0; i < curve.size(); ++i) {
                check(bits(curve[i].x, 1.75f + float(i) * .5f));
                check(curve[i].y == 0 && curve[i].transition == .25f && curve[i].style_a == 1 &&
                      curve[i].style_b == 2);
            }
        const std::array shifted_records{profile(1, 2), profile(2, 2)};
        const CoastProfiles shifted(shifted_records);
        const auto offset = build_coast_curves(points, shifted, {}, state(pc));
        for (const auto &curve : offset.curves())
            for (auto p : curve)
                check(p.y == -2.f && p.transition == .25f);
        auto closed_state = state(pc);
        closed_state.closed = true;
        const auto closed = build_coast_curves(points, styles, {}, closed_state);
        check(closed.control_count() == 18);
        auto saved = closed.curves();
        const CoastCurveSet owned(saved, 1);
        saved[2].clear();
        check(owned.curves()[2].size() == 18);
        for (int count : {514, 830, 2048, 4310}) {
            const auto large = build_coast_curves(straight(count), styles, {}, state(pc));
            check(large.control_count() == std::size_t(8 * count - 14));
            for (const auto &curve : large.curves())
                for (auto p : curve)
                    check(std::isfinite(p.x) && p.y == 0.f);
        }
    }
    const std::array char_records{profile(1, 0, 0), profile(2, 0, 0)};
    const CoastProfiles char_styles(char_records);
    const auto points = straight(4);
    rejects([&] { build_coast_curves(points, char_styles, {}, state()); });
    const auto displaced = build_coast_curves(points, char_styles, {{{0.f, 1.f}}}, state());
    bool moved = false;
    for (const auto &curve : displaced.curves())
        for (auto p : curve) {
            moved = moved || p.y != 0.f;
            check(p.transition == .25f);
        }
    check(moved);
    auto invalid = straight(3);
    rejects([&] { build_coast_curves(invalid, styles, {}, state()); });
    invalid = straight(max_coast_contour_controls + 1);
    rejects([&] { build_coast_curves(invalid, styles, {}, state()); });
    invalid.assign(4, {0, 0, 0, 1, 2});
    rejects([&] { build_coast_curves(invalid, styles, {}, state()); });
    invalid = points;
    invalid[1].x = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { build_coast_curves(invalid, styles, {}, state()); });
    auto bad_state = state();
    bad_state.precision = static_cast<X87Precision>(2);
    rejects([&] { build_coast_curves(points, styles, {}, bad_state); });
    bad_state = state();
    bad_state.wrap_offset.y = std::numeric_limits<float>::infinity();
    rejects([&] { build_coast_curves(points, styles, {}, bad_state); });
    CoastCharacterTables bad_tables{{{0, std::numeric_limits<float>::infinity()}}};
    rejects([&] { build_coast_curves(points, char_styles, bad_tables, state()); });
    bad_tables.resize(4097);
    rejects([&] { build_coast_curves(points, styles, bad_tables, state()); });
    bad_tables = {{}};
    bad_tables[0].resize(1048577, CoastCharacterPoint{});
    rejects([&] { build_coast_curves(points, styles, bad_tables, state()); });
    CoastCurves wrong;
    for (auto &c : wrong)
        c = straight(3);
    wrong[3].pop_back();
    rejects([&] { CoastCurveSet bad(wrong, 0); });
    for (auto &c : wrong)
        c = straight(2);
    rejects([&] { CoastCurveSet bad(wrong, 0); });
}
} // namespace
int main() {
    try {
        profiles();
        characters();
        curves();
        std::cout << "PASS " << checks << " coast profile/Character/curve checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
