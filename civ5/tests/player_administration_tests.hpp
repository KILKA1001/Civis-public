#pragma once
void player_administration_tests() {
    constexpr auto vp_layout = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw_layout = saves::GameStateLayout::expansion2_version1;
    static_assert(sizeof(void *) == 8);
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, counts, strings;
        std::vector<std::pair<std::size_t, unsigned>> versions;
    };
    const auto fixture = [](bool vp, bool notifications = true, unsigned notification_version = 2,
                            unsigned treasury_version = 1, bool empty = false, bool negative = false,
                            std::uint32_t history = 3) {
        Fixture f;
        auto &s = f.bytes;
        auto integer = [&](std::uint32_t n) { u32(s, n); };
        auto values = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                integer(i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        auto flag = [&](unsigned n) {
            f.flags.push_back(s.size());
            s += char(n);
        };
        auto version = [&](unsigned n, unsigned max) {
            f.versions.emplace_back(s.size(), max);
            integer(n);
        };
        auto text = [&] {
            f.strings.push_back(s.size());
            integer(3);
            s.append("\xff\0a", 3);
        };
        auto count = [&](std::uint32_t n, bool linked = false, bool stl = false) {
            f.counts.push_back(s.size());
            const auto wire = negative && linked ? 0xfffffff9u : empty ? vp && stl ? UINT32_MAX : 0 : n;
            integer(wire);
            return empty || (negative && linked) ? 0u : n;
        };
        flag(notifications);
        if (notifications) {
            if (!vp)
                version(notification_version, 2);
            integer(0);
            integer(70000);
            integer(4);
            integer(3);
            for (unsigned i = 0; i < (vp ? 150u : 100u); ++i) {
                integer(i == 0 ? 0xBE9725C6 : i == 1 ? UINT32_MAX : i);
                text();
                text();
                values(6);
                flag(i % 2);
                integer(70000);
            }
        }
        if (!vp)
            version(treasury_version, 1);
        values(vp ? 8 : 9);
        values(count(history, false, true));
        values(count(2, false, true));
        values(vp ? 3 : 1);
        if (!vp) {
            values(6 * count(2));
            values(6 * count(3));
            values(count(2, true));
        }
        values(count(3, true));
        if (!vp) {
            flag(1);
            flag(0);
        }
        integer(UINT32_MAX);
        integer(70000);
        if (vp)
            integer(0x80000000);
        const auto n = count(2, true);
        for (unsigned i = 0; i < n; ++i)
            text();
        return f;
    };
    const auto observe = [&](const std::string &bytes, bool vp, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_player_administration(in, vp ? vp_layout : bnw_layout, budget);
    };
    const auto number = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing administration field");
    };
    const auto array = [](const auto &b, std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : b.arrays)
            if (name == a.name)
                return a;
        throw std::runtime_error("Missing administration array");
    };
    const auto overwrite = [](std::string bytes, std::size_t offset, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.at(offset + i) = char(n >> (8 * i));
        return bytes;
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto p = observe(f.bytes + "CITIES", vp);
        check(p.start_offset == 0 && p.end_offset == f.bytes.size() && p.blocks.size() == 3,
              "Administration must stop before city objects");
        for (std::size_t i = 0; i < p.blocks.size(); ++i)
            check(p.blocks[i].start_offset == (i ? p.blocks[i - 1].end_offset : 0),
                  "Administration block gap");
        const auto &n = p.blocks[0], &t = p.blocks[1], &c = p.blocks[2];
        check(number(n, "has_notifications") == 1 && n.children.size() == (vp ? 150 : 100),
              "Notification source slots");
        check(number(n, "m_iNotificationsBeginIndex") == 4 && number(n, "m_iNotificationsEndIndex") == 3,
              "Notification wrapped ring changed");
        check(number(n.children[0], "m_eNotificationType_bits") == 0xBE9725C6ull &&
                  number(n.children[1], "m_eNotificationType_bits") == UINT32_MAX &&
                  number(n.children[2], "m_eNotificationType_bits") == 2,
              "Notification hashes/legacy indices changed");
        for (std::size_t i = 0; i < n.children.size(); ++i) {
            const auto &e = n.children[i];
            check(e.index == i && e.end_offset - e.start_offset == 47, "Notification slot order/width");
            check(number(e, "m_bDismissed") == i % 2 && number(e, "m_ePlayerID") == 70000 &&
                      number(e, "m_iExtraGameData") == 70000,
                  "Notification saved flags or unused fields changed");
            check(e.blobs.size() == 2 && e.blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97} &&
                      e.blobs[1].bytes == e.blobs[0].bytes,
                  "Notification raw text lost");
            rejects([&] { number(e, "m_bNeedsBroadcast"); });
            rejects([&] { number(e, "m_bWaitExtraTurn"); });
        }
        check(number(t, "m_iGold") == 70000 && number(t, "m_iGoldPerTurnFromDiplomacy") == -1 &&
                  number(t, "m_iExpensePerTurnUnitMaintenance") == INT32_MIN,
              "Treasury scalar sign/width");
        check(array(t, "m_GoldBalanceForTurnTimes100").values ==
                      std::vector<std::int64_t>{70000, -1, INT32_MIN} &&
                  array(t, "m_GoldChangeForTurnTimes100").values == std::vector<std::int64_t>{70000, -1},
              "Treasury distinct history lengths/units");
        check(array(c, "m_researchQueue").values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
              "Research queue order");
        check(c.children.size() == 2 && c.children[0].blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97},
              "City name bytes");
        check(number(c, "m_eEndTurnBlockingType") == -1 &&
                  number(c, "m_iEndTurnBlockingNotificationIndex") == 70000,
              "End-turn blockers changed");
        if (vp) {
            rejects([&] { number(t, "m_iExpensePerTurnUnitSupply"); });
            check(number(t, "m_iInternalTradeGoldBonus") == -1 &&
                      number(t, "m_iExpensePerTurnFromVassalTax") == INT32_MIN,
                  "VP treasury extensions missing");
            check(number(c, "m_iEmpireSizeModifierPerCityMod") == INT32_MIN && c.arrays.size() == 1,
                  "VP pre-city order");
        } else {
            check(number(t, "m_iExpensePerTurnUnitSupply") == 70000, "BNW supply expense lost");
            rejects([&] { number(t, "m_iInternalTradeGoldBonus"); });
            check(array(c, "m_ppaaiSpecialistExtraYield").columns == 6 &&
                      array(c, "m_ppaaiSpecialistExtraYield").values.size() == 12 &&
                      array(c, "m_ppaaiImprovementYieldChange").values.size() == 18,
                  "BNW fixed yield rows");
            check(array(c, "m_UnitCycle").values == std::vector<std::int64_t>{70000, -1} &&
                      number(c, "m_bEverPoppedGoody") == 1 && number(c, "m_bEverTrainedBuilder") == 0,
                  "BNW unit cycler/flags");
        }
        const auto absent = fixture(vp, false);
        const auto a = observe(absent.bytes, vp);
        check(a.blocks[0].end_offset == 1 && a.blocks[0].fields.size() == 1 && a.blocks[0].children.empty() &&
                  a.end_offset == absent.bytes.size(),
              "Absent notification object synthesized");
        for (bool negative : {false, true}) {
            const auto e = fixture(vp, false, 2, 1, true, negative);
            const auto q = observe(e.bytes, vp);
            check(q.end_offset == e.bytes.size() && q.blocks[2].children.empty(),
                  "Empty administration containers");
            check(number(q.blocks[2], "m_researchQueue_wire_count") == (negative ? -7 : 0),
                  "Signed linked-list sentinel lost");
            check(number(q.blocks[1], "m_GoldBalanceForTurnTimes100_wire_count") ==
                      (vp ? std::int64_t(UINT32_MAX) : 0),
                  "VP treasury STL sentinel lost");
        }
        for (std::size_t cut = 0; cut < f.bytes.size(); ++cut)
            rejects([&] { observe(f.bytes.substr(0, cut), vp); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = char(2);
            rejects([&] { observe(bad, vp); });
        }
        for (auto offset : f.counts)
            rejects([&] { observe(overwrite(f.bytes, offset, 0x7fffffff), vp); });
        for (auto offset : f.strings)
            rejects([&] { observe(overwrite(f.bytes, offset, UINT32_MAX), vp); });
        for (auto [offset, maximum] : f.versions)
            rejects([&] { observe(overwrite(f.bytes, offset, maximum + 1), vp); });
        for (unsigned kind = 0; kind < 3; ++kind) {
            saves::GameStateBudget budget;
            if (kind == 0)
                budget.elements = 1;
            if (kind == 1)
                budget.string_bytes = 2;
            if (kind == 2)
                budget.total_string_bytes = 5;
            rejects([&] { observe(f.bytes, vp, budget); });
        }
        const auto large = fixture(vp, false, 2, 1, false, false, 65537);
        check(array(observe(large.bytes, vp).blocks[1], "m_GoldBalanceForTurnTimes100").values.size() ==
                  65537,
              "Treasury history truncated to 16-bit count");
    }
    for (unsigned n = 0; n <= 2; ++n)
        for (unsigned t = 0; t <= 1; ++t) {
            const auto f = fixture(false, true, n, t);
            const auto p = observe(f.bytes, false);
            check(p.end_offset == f.bytes.size() && number(p.blocks[0], "version") == n &&
                      number(p.blocks[1], "version") == t,
                  "BNW notification/treasury versions");
            check(number(p.blocks[0].children[2], "m_eNotificationType_bits") == 2,
                  "Legacy notification index remapped");
        }
    const auto bytes = fixture(true).bytes;
    class ShiftedAdministrationStream final : public std::streambuf {
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
        explicit ShiftedAdministrationStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream s(&backing);
    runtime::BinaryReader in(s, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_player_administration(in, vp_layout);
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.blocks[2].start_offset > UINT32_MAX,
          "Administration stream offsets narrowed");
}
