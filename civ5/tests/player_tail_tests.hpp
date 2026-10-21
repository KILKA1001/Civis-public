#pragma once
#include "player_tail_fixture.hpp"
void player_tail_tests() {
    static_assert(sizeof(void *) == 8);
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    const saves::PlayerTailVpContext vc{3, 65, 43, 0};
    const auto observe = [](const std::string &bytes, auto layout,
                            std::optional<saves::PlayerTailVpContext> context,
                            saves::GameStateBudget budget = {}) {
        std::istringstream s(bytes);
        runtime::BinaryReader in(s, bytes.size());
        return saves::observe_player_tail(in, layout, context, budget);
    };
    const auto field = [](const auto &fields, std::string_view name) -> const saves::PlayerTailNode & {
        for (const auto &f : fields)
            if (f.name == name)
                return f;
        throw std::runtime_error("Missing player tail field");
    };
    const auto patch = [](std::string bytes, std::uint64_t at, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.at(at + i) = char(value >> (8 * i));
        return bytes;
    };
    for (auto layout : {bnw, vp}) {
        const bool modded = layout == vp;
        const std::optional<saves::PlayerTailVpContext> ctx = modded ? std::optional(vc) : std::nullopt;
        const auto &bytes = modded ? player_tail_fixture_vp : player_tail_fixture_bnw;
        const auto p = observe(bytes, layout, ctx);
        check(p.start_offset == 0 && p.end_offset == bytes.size() && p.fields.size() == (modded ? 53 : 11),
              "Full player tail size");
        auto pos = std::uint64_t(0);
        for (const auto &f : p.fields) {
            check(f.start_offset == pos && f.end_offset > pos, "Player suffix continuity");
            pos = f.end_offset;
        }
        check(p.base_player_end_offset == bytes.size() - (modded ? 0 : 4), "CvPlayerAI boundary");
        check(field(p.fields, "m_aUnitExtraCosts").values ==
                  std::vector<std::int64_t>({70000, INT32_MIN, 70000, INT32_MIN}),
              "Player duplicate keys and signed values");
        const auto &names = p.fields.front();
        const auto &string = modded ? names.children[0].children[0] : names.children[0];
        check(string.bytes == std::vector<std::uint8_t>({0, 255, 'S', 'A', 'V', 'E', 'D'}),
              "Replay label bytes were normalized");
        const auto &turns = modded ? names.children[0].children[1] : p.fields[1].children[0];
        check(turns.values == std::vector<std::int64_t>({4294967295ll, INT32_MIN, 4294967295ll, INT32_MIN}),
              "Unsigned replay turn keys and duplicates");
        for (std::size_t i = 0; i < bytes.size(); ++i)
            rejects([&] { observe(bytes.substr(0, i), layout, ctx); });
        saves::GameStateBudget budget;
        budget.elements = p.retained_elements;
        budget.total_string_bytes = p.retained_bytes;
        check(observe(bytes, layout, ctx, budget).retained_elements == p.retained_elements,
              "Exact player tail budget");
        --budget.elements;
        rejects([&] { observe(bytes, layout, ctx, budget); });
        ++budget.elements;
        --budget.total_string_bytes;
        rejects([&] { observe(bytes, layout, ctx, budget); });
        budget.elements = UINT64_MAX;
        budget.total_string_bytes = UINT64_MAX;
        check(observe(bytes, layout, ctx, budget).retained_elements == p.retained_elements,
              "Player tail budget overflow");
        budget.string_bytes = 6;
        rejects([&] { observe(bytes, layout, ctx, budget); });
        rejects([&] { observe(patch(bytes, string.start_offset, UINT32_MAX), layout, ctx); });
        rejects([&] { observe(patch(bytes, 0, 0xfffffffe), layout, ctx); });
        std::istringstream s(bytes + "NEXT");
        runtime::BinaryReader in(s, bytes.size() + 4);
        saves::observe_player_tail(in, layout, ctx);
        check(in.u32() == 0x5458454e, "Player tail consumed next player");
        auto bad = bytes;
        const auto &b = field(p.fields, modded ? "m_pabHasGlobalMonopoly" : "m_bfEverConqueredBy");
        bad.at(b.start_offset + 4) = char(2);
        std::optional<saves::PlayerTailObservation> atomic;
        try {
            atomic = observe(bad, layout, ctx);
        } catch (const std::exception &e) {
            check(std::string(e.what()).find(b.name) != std::string::npos, "Tail error omitted field name");
        }
        check(!atomic, "Partial player tail exposed as complete");
    }
    const auto p = observe(player_tail_fixture_vp, vp, vc);
    const auto b = observe(player_tail_fixture_bnw, bnw, {});
    const auto &conquered = field(p.fields, "m_bfEverConqueredBy");
    check(conquered.values == std::vector<std::int64_t>({2147483649ll, 2147483649ll, 2147483649ll}),
          "VP conquered bit words and rounding");
    const auto &events = field(p.fields, "m_vActiveEspionageEventsList");
    check(events.children[0].end_offset - events.children[0].start_offset == 21,
          "Espionage event bool width");
    const auto &requests = field(p.fields, "m_pDiplomacyRequests");
    check(requests.children[1].children.size() == 2 && requests.children[1].children[0].children.size() == 7,
          "Diplomacy request chain");
    auto bad = player_tail_fixture_vp;
    bad.at(events.children[0].children[1].start_offset) = char(255);
    rejects([&] { observe(bad, vp, vc); });
    bad = player_tail_fixture_vp;
    bad.at(p.fields.back().start_offset) = char(2);
    rejects([&] { observe(bad, vp, vc); });
    const auto &achievement = field(b.fields, "m_kPlayerAchievements");
    for (auto at : {achievement.start_offset, b.base_player_end_offset})
        for (auto version : {2u, UINT32_MAX})
            rejects([&] { observe(patch(player_tail_fixture_bnw, at, version), bnw, {}); });
    check(observe(player_tail_fixture_bnw_old, bnw, {}).fields.back().values[0] == 0, "BNW version zero");
    auto minor = vc;
    minor.player_id = 43;
    check(observe(player_tail_fixture_vp_minor, vp, minor).fields.size() == 52,
          "Minor player request branch");
    minor.player_id = -1;
    check(observe(player_tail_fixture_vp, vp, minor).fields.size() == 53,
          "Signed saved player request branch");
    check(observe(player_tail_fixture_vp_empty, vp, vc).fields[0].wire_count == 0, "Empty tail containers");
    const auto sentinel = observe(player_tail_fixture_vp_sentinel, vp, vc);
    check(sentinel.fields[0].wire_count == UINT32_MAX && sentinel.fields[0].children.empty(),
          "VP history sentinel");
    check(field(observe(player_tail_fixture_vp_negative, vp, vc).fields, "m_kUnitCycle").wire_count == -9,
          "Negative unit cycle");
    check(
        field(observe(player_tail_fixture_bnw_negative, bnw, {}).fields, "m_bfEverConqueredBy").wire_count ==
            -9,
        "Negative BNW bool count");
    rejects([&] { observe(player_tail_fixture_vp, vp, {}); });
    rejects([&] { observe(player_tail_fixture_bnw, bnw, vc); });
    for (unsigned which = 0; which < 3; ++which) {
        auto d = vc;
        if (which == 0)
            d.yields = 0;
        if (which == 1)
            d.players = 0;
        if (which == 2)
            d.major = 66;
        rejects([&] { observe(player_tail_fixture_vp, vp, d); });
    }
    for (bool players : {false, true}) {
        auto d = vc;
        if (players)
            d.players = UINT64_MAX;
        else
            d.yields = UINT64_MAX;
        rejects([&] { observe(player_tail_fixture_vp, vp, d); });
    }
    const auto &history = field(p.fields, "m_ppiInstantYieldHistoryValues");
    std::string many = player_tail_fixture_vp.substr(0, history.start_offset);
    u32(many, 100);
    const auto &row = history.children[0];
    for (unsigned i = 0; i < 100; ++i)
        many += player_tail_fixture_vp.substr(row.start_offset, row.end_offset - row.start_offset);
    many += player_tail_fixture_vp.substr(history.end_offset);
    check(field(observe(many, vp, vc).fields, history.name).children.size() == 100,
          "Original 30-turn history cap leaked");
    many = player_tail_fixture_vp.substr(0, conquered.start_offset);
    for (unsigned i = 0; i < 5; ++i)
        u32(many, UINT32_MAX);
    many += player_tail_fixture_vp.substr(conquered.end_offset);
    auto wide = vc;
    wide.players = 129;
    check(field(observe(many, vp, wide).fields, conquered.name).values.size() == 5,
          "Original 64-player bitfield cap leaked");
    class ShiftedTailStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t pos_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            pos_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize n) override {
            const auto local = pos_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(n) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, n, out);
            pos_ += n;
            return n;
        }

      public:
        explicit ShiftedTailStream(const std::string &b) : bytes_(b) {}
    } backing(player_tail_fixture_vp);
    std::istream s(&backing);
    runtime::BinaryReader in(s, 0x100000123ull + player_tail_fixture_vp.size());
    in.seek(0x100000123ull);
    const auto high = saves::observe_player_tail(in, vp, vc);
    check(high.start_offset == 0x100000123ull &&
              high.end_offset == 0x100000123ull + player_tail_fixture_vp.size() &&
              high.fields.back().start_offset > UINT32_MAX,
          "Player tail offsets truncated at 4GB");
}
