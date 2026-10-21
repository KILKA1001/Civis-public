#pragma once
void player_espionage_tests() {
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, unsigned_counts, signed_counts, zero_versions, spy_versions,
            mod_versions, sentinels, name_sizes;
    };
    // Wire writer follows source stream calls, independently of native records.
    const auto fixture = [](bool vp, unsigned spy_version = 1, std::uint32_t spies = 2, bool negative = false,
                            std::uint32_t max_costs = 3) {
        Fixture f;
        auto &s = f.bytes;
        const auto value = [&](std::uint32_t n = 70000) { u32(s, n); };
        const auto header = [&](bool spy = false) {
            (spy ? f.spy_versions : f.zero_versions).push_back(s.size());
            value(spy ? spy_version : 0);
            if (vp) {
                f.mod_versions.push_back(s.size());
                value(151);
                f.sentinels.push_back(s.size());
                value(0xdeadbeef);
            }
        };
        const auto count = [&](std::uint32_t n, bool signed_count = false) {
            (signed_count ? f.signed_counts : f.unsigned_counts).push_back(s.size());
            value(negative && signed_count ? 0xfffffff9u : n);
            return negative && signed_count ? 0u : n;
        };
        const auto flag = [&](unsigned n) {
            f.flags.push_back(s.size());
            s += char(n);
        };
        const auto name = [&] {
            f.name_sizes.push_back(s.size());
            value(3);
            s.append("\xff\0a", 3);
        };
        const auto values = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                value(i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        header();
        const auto spy_count = count(spies, true);
        for (std::uint32_t i = 0; i < spy_count; ++i) {
            header(true);
            if (vp)
                name();
            values(vp ? 7 : 6);
            if (vp || spy_version >= 1)
                flag(1);
            if (vp)
                value(70000);
            flag(0);
            if (vp) {
                flag(1);
                values(2);
            }
        }
        if (!vp) {
            values(count(3, true));
            value(70000);
        }
        count(2);
        for (unsigned i = 0; i < 2; ++i)
            values(count(i == 0 ? 3 : 0));
        values(count(3));
        if (vp)
            values(count(2));
        else {
            values(count(max_costs, true));
            const auto civs = count(2, true);
            for (unsigned i = 0; i < civs; ++i)
                values(2 * count(i == 0 ? 3 : 0, true));
        }
        const auto notifications = count(2, true);
        for (unsigned i = 0; i < notifications; ++i)
            values(vp ? 8 : 5);
        const auto intrigues = count(2, true);
        for (unsigned i = 0; i < intrigues; ++i) {
            values(vp ? 11 : 9);
            name();
            flag(0);
            if (vp)
                value(70000);
        }
        header();
        for (unsigned i = 0; i < 7; ++i)
            values(count(i + 1));
        flag(1);
        value(UINT32_MAX);
        return f;
    };
    const auto observe = [](const std::string &s, bool vp,
                            saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_player_espionage(
            in, vp ? saves::GameStateLayout::vp542_corpus : saves::GameStateLayout::expansion2_version1,
            budget);
    };
    const auto number = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing espionage field");
    };
    const auto array = [](const auto &b, std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : b.arrays)
            if (name == a.name)
                return a;
        throw std::runtime_error("Missing espionage array");
    };
    const auto overwrite = [](std::string s, std::size_t offset, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            s.at(offset + i) = char(n >> (8 * i));
        return s;
    };
    for (const auto [vp, version] : {std::pair{false, 0u}, {false, 1u}, {true, 1u}}) {
        const auto f = fixture(vp, version);
        const auto p = observe(f.bytes + "PLAYER_TRADE", vp);
        check(p.start_offset == 0 && p.end_offset == f.bytes.size() && p.blocks.size() == 2,
              "Espionage crossed the PlayerTrade boundary");
        const auto &b = p.blocks[0], &ai = p.blocks[1];
        check(b.end_offset == ai.start_offset && ai.end_offset == p.end_offset,
              "Espionage blocks are discontinuous");
        check(b.children.size() == (vp ? 8 : 10) && number(b, "m_aSpyList_wire_count") == 2,
              "Espionage nested collections lost");
        const auto &spy = b.children[0];
        check(number(spy, "version") == version && number(spy, "m_bEvaluateReassignment") == 0,
              "BNW reassignment load side effect applied");
        if (vp || version)
            check(number(spy, "m_bIsDiplomat") == 1, "Spy diplomat flag lost");
        else
            rejects([&] { number(spy, "m_bIsDiplomat"); });
        if (vp) {
            check(number(spy, "m_eVassalDiplomatPlayer") == 70000 &&
                      number(spy, "m_eSpyFocus") == INT32_MIN &&
                      spy.blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97},
                  "VP spy IDs, focus or binary name changed");
            rejects([&] { number(spy, "m_iName"); });
        } else {
            check(spy.blobs.empty() && number(spy, "m_iName") == 70000 && number(spy, "m_eSpyState") == -1,
                  "BNW name index or unknown spy state changed");
            check(array(b, "m_aiMaxTechCost").values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "BNW max tech costs narrowed");
            check(array(b.children[4], "locations").columns == 2 &&
                      array(b.children[4], "locations").values.size() == 6,
                  "BNW heist coordinate pairs lost");
        }
        check(array(b.children[2], "techs").values == std::vector<std::int64_t>{70000, -1, INT32_MIN} &&
                  array(b.children[3], "techs").values.empty(),
              "Stealable tech IDs or empty row changed");
        const auto &intrigue = b.children.back();
        check(intrigue.blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97} &&
                  number(intrigue, "m_bShared") == 0,
              "Intrigue name decoded as UTF8 or shared flag lost");
        check(ai.arrays.size() == 7 && ai.arrays[6].values.size() == 7 &&
                  number(ai, "m_iTurnEspionageStarted") == -1 && number(ai, "m_bUNCountdownStarted") == 1,
              "AI independent list lengths or start turn changed");
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = char(2);
            rejects([&] { observe(bad, vp); });
        }
        for (auto offset : f.zero_versions)
            rejects([&] { observe(overwrite(f.bytes, offset, 1), vp); });
        for (auto offset : f.spy_versions)
            rejects([&] { observe(overwrite(f.bytes, offset, 2), vp); });
        for (auto offset : f.mod_versions)
            rejects([&] { observe(overwrite(f.bytes, offset, 150), vp); });
        for (auto offset : f.sentinels)
            rejects([&] { observe(overwrite(f.bytes, offset, 0), vp); });
        for (auto offset : f.unsigned_counts)
            rejects([&] { observe(overwrite(f.bytes, offset, UINT32_MAX), vp); });
        for (auto offset : f.signed_counts)
            rejects([&] { observe(overwrite(f.bytes, offset, INT32_MAX), vp); });
        for (auto offset : f.name_sizes)
            rejects([&] { observe(overwrite(f.bytes, offset, UINT32_MAX), vp); });
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), vp); });
        saves::GameStateBudget budget;
        budget.elements = 2;
        rejects([&] { observe(f.bytes, vp, budget); });
        budget = {};
        budget.string_bytes = 2;
        rejects([&] { observe(f.bytes, vp, budget); });
        budget = {};
        budget.total_string_bytes = 5;
        rejects([&] { observe(f.bytes, vp, budget); });
        const auto neg = observe(fixture(vp, version, 2, true).bytes, vp);
        check(number(neg.blocks[0], "m_aSpyList_wire_count") == -7 && neg.blocks[0].children.size() == 2 &&
                  number(neg.blocks[0], "m_aSpyNotificationMessages_wire_count") == -7,
              "Negative source loop count became an allocation");
        if (!vp)
            check(number(neg.blocks[0], "m_aHeistLocations_wire_count") == -7 &&
                      array(neg.blocks[0], "m_aiMaxTechCost").values.empty(),
                  "BNW negative fixed-array counts changed");
        auto empty_name = f.bytes;
        const auto offset = f.name_sizes[0];
        empty_name.erase(offset + 4, 3);
        empty_name = overwrite(empty_name, offset, 0);
        const auto empty = observe(empty_name, vp);
        check(empty.end_offset == f.bytes.size() - 3, "Empty spy name consumed missing bytes");
    }
    for (bool vp : {false, true}) {
        saves::GameStateBudget wide;
        wide.elements = 4 * 1024 * 1024;
        const auto many = fixture(vp, 1, 65537, false, 65537);
        const auto p = observe(many.bytes, vp, wide);
        check(number(p.blocks[0], "m_aSpyList_wire_count") == 65537 &&
                  p.blocks[0].children[65536].index == 65536 && p.end_offset == many.bytes.size(),
              "Spy count capped at original player or 16-bit capacity");
        if (!vp)
            check(array(p.blocks[0], "m_aiMaxTechCost").values.size() == 65537,
                  "BNW max tech costs capped at original MAX_MAJOR_CIVS");
    }
    const auto bytes = fixture(true).bytes;
    class ShiftedEspionageStream final : public std::streambuf {
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
        explicit ShiftedEspionageStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_player_espionage(in, saves::GameStateLayout::vp542_corpus);
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.blocks[1].start_offset > UINT32_MAX &&
              shifted.blocks[0].children[0].start_offset > UINT32_MAX,
          "Espionage stream positions narrowed");
}
