#pragma once
#include "cities_fixture.hpp"
void cities_tests() {
    static_assert(sizeof(void *) == 8);
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    const auto header = [](std::int32_t count) {
        std::string s;
        for (std::uint32_t n :
             {3u, UINT32_MAX, UINT32_MAX, 0u, 0x7fffffffu, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX})
            u32(s, n);
        u32(s, static_cast<std::uint32_t>(count));
        return s;
    };
    saves::CitiesContext context;
    context.bnw_projects = 6;
    const auto observe = [&](const std::string &bytes, saves::GameStateLayout layout,
                             saves::CitiesContext c = {}, saves::CitiesBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_cities(in, layout, c, budget);
    };
    const auto field = [](const auto &city, std::string_view name) -> const saves::CityCoreField & {
        for (const auto &f : city.core.fields)
            if (f.name == name)
                return f;
        throw std::runtime_error("Missing city field");
    };
    const auto overwrite = [](std::string bytes, std::size_t at, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.at(at + i) = char(value >> (8 * i));
        return bytes;
    };
    auto bytes = header(2) + cities_fixture_v0 + cities_fixture_v6;
    std::string next;
    u32(next, 50);
    const auto p = observe(bytes + next, bnw, context);
    check(p.start_offset == 0 && p.end_offset == bytes.size() && p.cities.size() == 2,
          "Complete cities boundary/count");
    check(!p.context.profile_extensions.find<saves::VpCitiesWireDimensions>(),
          "BNW city collection acquired VP wire dimensions");
    check(p.header.source_slots == 4 && p.header.next_free_indices.size() == 4 &&
              p.header.wire_city_count == 2,
          "Source freelist metadata retained");
    check(p.cities[0].index == 0 && p.cities[1].index == 1 &&
              p.cities[0].core.start_offset == p.header.end_offset &&
              p.cities[0].tail.end_offset == p.cities[1].core.start_offset,
          "City order or boundary lost");
    check(p.cities[0].core.version == 0 && p.cities[1].core.version == 6 &&
              p.cities[0].production.context.bnw_city_version == 0 &&
              p.cities[1].production.context.bnw_city_version == 6 &&
              p.cities[0].tail.context.bnw_city_version == 0 &&
              p.cities[1].tail.context.bnw_city_version == 6,
          "Per-city version context leaked across records");
    check(field(p.cities[0], "m_iID").values[0] == 70000 &&
              field(p.cities[1], "m_iID").values[0] == INT32_MAX,
          "City IDs masked or narrowed");
    check(field(p.cities[0], "m_iPopulation").values[0] == 17 &&
              field(p.cities[1], "m_iPopulation").values[0] == 23,
          "Second city not independently imported");
    const auto reversed = observe(header(2) + cities_fixture_v6 + cities_fixture_v0, bnw, context);
    check(reversed.cities[0].core.version == 6 && reversed.cities[1].core.version == 0,
          "Mixed city versions reordered");
    const auto duplicates = observe(header(2) + cities_fixture_v0 + cities_fixture_v0, bnw, context);
    check(field(duplicates.cities[0], "m_iID").values == field(duplicates.cities[1], "m_iID").values,
          "Duplicate source IDs filtered");
    // Source capacity does not control native allocation or source-order admission.
    const auto unusual = observe(overwrite(bytes, 0, 1).erase(24, 12), bnw, context);
    check(unusual.header.source_slots == 1 && unusual.cities.size() == 2,
          "Old source capacity became native limit");
    for (std::int32_t count : {0, -7}) {
        const auto empty = observe(header(count) + next, bnw);
        check(empty.cities.empty() && empty.end_offset == header(count).size() &&
                  empty.header.wire_city_count == count,
              "Empty/signed BNW city count");
        std::string v;
        u32(v, static_cast<std::uint32_t>(count));
        const auto q = observe(v + next, vp);
        check(q.cities.empty() && q.end_offset == 4 && q.header.wire_city_count == count,
              "Empty/signed VP city count");
    }
    std::istringstream stream(bytes + next);
    runtime::BinaryReader input(stream, bytes.size() + next.size());
    check(saves::observe_cities(input, bnw, context).end_offset == bytes.size() && input.u32() == 50,
          "Unit header consumed as city bytes");
    // The existing per-block decoders test every truncation; here cover transitions
    // and failures after one valid city, including the final CityAI byte.
    for (const auto &city : p.cities)
        for (const auto end : {city.core.end_offset, city.production.end_offset, city.management.end_offset,
                               city.tail.end_offset})
            rejects([&] { observe(bytes.substr(0, end - 1), bnw, context); });
    rejects([&] { observe(header(3) + cities_fixture_v0 + cities_fixture_v6, bnw, context); });
    auto bad = bytes;
    for (const auto &f : p.cities[1].core.fields)
        if (f.encoding == saves::CityCoreEncoding::boolean) {
            bad.at(f.start_offset) = char(2);
            break;
        }
    try {
        observe(bad, bnw, context);
        check(false, "Invalid second city accepted");
    } catch (const std::exception &e) {
        check(std::string(e.what()).find("City 1 core:") != std::string::npos,
              "Second city diagnostic index");
    }
    bad = overwrite(bytes, p.cities[1].core.start_offset, 7);
    rejects([&] { observe(bad, bnw, context); });
    bad = overwrite(bytes, p.cities[1].tail.blocks[2].start_offset, 1);
    try {
        observe(bad, bnw, context);
        check(false, "Invalid second city espionage accepted");
    } catch (const std::exception &e) {
        check(std::string(e.what()).find("City 1 tail:") != std::string::npos,
              "Second city diagnostic stage");
    }
    rejects([&] { observe(bytes, bnw); });
    saves::CitiesContext other;
    other.profile_extensions.emplace<saves::VpCitiesWireDimensions>();
    rejects([&] { observe(header(0), bnw, other); });
    rejects([&] { observe(std::string(4, '\0'), vp, context); });
    other.bnw_projects = 6;
    rejects([&] { observe(bytes, bnw, other); });
    auto huge = context;
    huge.bnw_projects = UINT64_MAX;
    rejects([&] { observe(bytes, bnw, huge); });
    saves::CitiesBudget exact;
    exact.retained_elements = p.retained_elements;
    exact.retained_string_bytes = p.retained_string_bytes;
    check(observe(bytes, bnw, context, exact).cities.size() == 2, "Exact aggregate budget rejected");
    --exact.retained_elements;
    rejects([&] { observe(bytes, bnw, context, exact); });
    exact.retained_elements = p.retained_elements;
    --exact.retained_string_bytes;
    rejects([&] { observe(bytes, bnw, context, exact); });
    exact = {};
    exact.per_block.elements = 1;
    rejects([&] { observe(bytes, bnw, context, exact); });
    const auto first = observe(header(1) + cities_fixture_v0, bnw, context);
    exact = {};
    exact.retained_elements = first.retained_elements;
    rejects([&] { observe(bytes, bnw, context, exact); });
    exact = {};
    exact.retained_string_bytes = first.retained_string_bytes;
    rejects([&] { observe(bytes, bnw, context, exact); });
    class ShiftedCitiesStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize n) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(n) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, n, out);
            position_ += n;
            return n;
        }

      public:
        explicit ShiftedCitiesStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream shifted(&backing);
    runtime::BinaryReader in(shifted, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto wide = saves::observe_cities(in, bnw, context);
    check(wide.start_offset == 0x100000123ull && wide.end_offset == 0x100000123ull + bytes.size() &&
              wide.cities[1].core.start_offset > UINT32_MAX && wide.cities[1].tail.end_offset > UINT32_MAX,
          "Full city collection offsets narrowed");
    check(wide.retained_elements == p.retained_elements &&
              wide.retained_string_bytes == p.retained_string_bytes,
          "Retained budget depends on absolute offsets");
}
