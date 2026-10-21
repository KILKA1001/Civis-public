#pragma once
void player_diplomacy_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    const saves::PlayerDiplomacyWireDimensions d{2, 3, 4, 5, 2, 2, 3, 2};
    const auto observe = [&](const std::string &s, bool is_vp, saves::PlayerDiplomacyWireDimensions dims,
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_player_diplomacy(in, is_vp ? vp : bnw, dims, budget);
    };
    const auto field = [](const auto &p, std::string_view name) -> const saves::PlayerDiplomacyField & {
        for (const auto &f : p.fields)
            if (f.name == name)
                return f;
        throw std::runtime_error("Missing diplomacy field");
    };
    const auto put = [](std::string &s, std::size_t offset, std::uint32_t v, unsigned width) {
        for (unsigned i = 0; i < width; ++i)
            s.at(offset + i) = char(v >> (i * 8));
    };
    // Sizes and offsets were derived independently from SDK Read/Write and VP
    // declarations (probe_diplomacy), not the runtime field table.
    std::string v(846, 0), b(770, 0);
    b[0] = 4;
    put(v, 0, 255, 1);
    put(v, 110, 0x80, 1);
    put(v, 111, 0xff, 1);
    put(v, 112, 0x8000, 2);
    put(v, 114, 0x7fff, 2);
    put(v, 173, 0x80000000, 4);
    put(v, 177, 0x7fffffff, 4);
    put(v, 193, 255, 1);
    put(v, 215, 65535, 2);
    put(v, 55, 1, 1);
    put(v, 842, 70000, 4);
    put(b, 10, 0x8000, 2);
    put(b, 12, 7, 2);
    put(b, 24, 1234, 2);
    put(b, 26, 0xffff, 2);
    put(b, 8, 1, 1);
    put(b, 23, 1, 1);
    put(b, 692, UINT32_MAX, 4);
    put(b, 696, 70000, 4);
    put(b, 724, 0x80000000, 4);
    put(b, 736, UINT32_MAX, 4);
    put(b, 740, 1, 1);
    put(b, 741, 70000, 4);
    const auto pv = observe(v, true, d), pb = observe(b, false, d);
    check(pv.fields.size() == 216 && pb.fields.size() == 149 && !pv.version && pb.version == 4,
          "Diplomacy field/version count");
    check(pv.end_offset == 846 && pb.end_offset == 770, "Diplomacy boundary");
    check(field(pv, "m_eID").values[0] == 255 && field(pv, "m_aiNumDemandsMade").values[0] == 255,
          "Unsigned diplomacy byte narrowed");
    check(field(pv, "m_aeCivOpinion").values == std::vector<std::int64_t>{-128, -1},
          "Signed diplomacy char changed");
    check(field(pv, "m_aiCachedOpinionWeight").values == std::vector<std::int64_t>{-32768, 32767},
          "Signed diplomacy short changed");
    check(field(pv, "m_aiTradeValue").values[0] == 65535, "Unsigned diplomacy short changed");
    check(field(pv, "m_aiDoFAcceptedTurn").values == std::vector<std::int64_t>{INT32_MIN, INT32_MAX},
          "Diplomacy signed integer changed");
    check(field(pv, "m_aiVassalGoldPerTurnCollectedSinceVassalStarted").values[1] == 70000,
          "Final VP diplomacy counter lost");
    const auto &matrix = field(pb, "m_ppaaiWorkingAgainstPlayerCounter");
    check(matrix.values == std::vector<std::int64_t>{-32768, 7, 1234, -1},
          "BNW interleaved matrix reordered");
    check(matrix.spans.size() == 2 && matrix.spans[0].start == 10 && matrix.spans[0].end == 14 &&
              matrix.spans[1].start == 24 && matrix.spans[1].end == 28,
          "BNW matrix wire spans collapsed");
    check(field(pb, "m_ppaabWorkingAgainstPlayerAccepted").values == std::vector<std::int64_t>{1, 0, 0, 1},
          "BNW interleaved booleans reordered");
    check(field(pb, "m_ppaDiploStatementsLog").values[0] == -1 &&
              field(pb, "m_ppaDiploStatementsLog").values[1] == 70000,
          "Diplomatic log enum/turn changed");
    const auto &decl = field(pb, "m_paDeclarationsLog");
    check(decl.values[0] == INT32_MIN && decl.values[3] == -1 && decl.values[4] == 1 &&
              decl.values[5] == 70000 && decl.spans[0].end == 766,
          "Declaration record padding or order changed");
    for (bool is_vp : {false, true}) {
        const auto &s = is_vp ? v : b;
        for (std::size_t n = 0; n < s.size(); ++n)
            rejects([&] { observe(s.substr(0, n), is_vp, d); });
        auto bad = s;
        bad[is_vp ? 55 : 740] = 2;
        rejects([&] { observe(bad, is_vp, d); });
        saves::GameStateBudget budget;
        budget.elements = 1;
        rejects([&] { observe(s, is_vp, d, budget); });
        auto overflow = d;
        overflow.major = UINT64_MAX;
        rejects([&] { observe(s, is_vp, overflow); });
    }
    const std::size_t sizes[] = {0, 732, 746, 766, 770}, counts[] = {0, 137, 143, 148, 149};
    for (unsigned ver = 1; ver <= 4; ++ver) {
        std::string s(sizes[ver], 0);
        s[0] = char(ver);
        const auto p = observe(s, false, d);
        check(p.version == ver && p.end_offset == s.size() && p.fields.size() == counts[ver],
              "Historical BNW diplomacy version");
        for (const auto &f : p.fields) {
            if (ver < 4)
                check(std::string_view(f.name) != "m_paiNumLandmarksBuiltForMe",
                      "Missing old field replaced with reset default");
            if (ver < 2)
                check(std::string_view(f.name) != "m_pabPlayerMadeNoDiggingPromise",
                      "Old digging flag injected");
            if (ver < 3)
                check(std::string_view(f.name) != "m_paiOtherPlayerTurnsSinceTheySupportedOurHosting",
                      "Old hosting counter injected");
        }
    }
    for (auto ver : {0u, 5u, UINT32_MAX}) {
        auto s = b;
        put(s, 0, ver, 4);
        rejects([&] { observe(s, false, d); });
    }
    const saves::PlayerDiplomacyWireDimensions large_vp{1, 0, 0, 0, 0, 0, 65537, 0},
        large_bnw{0, 0, 0, 0, 0, 0, 0, 65537};
    auto p = observe(std::string(262535, 0), true, large_vp);
    check(field(p, "m_aaiTurnStatementLastSent").values.size() == 65537,
          "VP statement dimension capped at 16 bits");
    std::string wide(1376339, 0);
    wide[0] = 4;
    p = observe(wide, false, large_bnw);
    check(field(p, "m_paDeclarationsLog").values.size() == 65537 * 6ull, "BNW log dimension capped");
    // An unused source dimension does not constrain this layout.
    auto empty = large_bnw;
    empty.log_entries = 0;
    empty.players = UINT64_MAX;
    std::string empty_bytes(62, 0);
    empty_bytes[0] = 4;
    check(observe(empty_bytes, false, empty).end_offset == 62, "Unused wide dimension narrowed");
    class ShiftedDiplomacyStream final : public std::streambuf {
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
        explicit ShiftedDiplomacyStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(v);
    std::istream input(&backing);
    runtime::BinaryReader shifted(input, 0x100000123ull + v.size());
    shifted.seek(0x100000123ull);
    p = saves::observe_player_diplomacy(shifted, vp, d);
    check(p.start_offset == 0x100000123ull && p.end_offset == 0x100000123ull + v.size() &&
              p.fields.back().spans.back().end == p.end_offset,
          "Diplomacy stream offsets narrowed");
}
