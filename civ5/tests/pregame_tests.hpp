struct PregameFixture {
    std::string bytes;
    std::map<std::string, std::uint64_t> offsets;
    std::vector<std::uint64_t> boundaries, counts, strings;
};
PregameFixture pregame_fixture(bool vp) {
    PregameFixture f;
    auto &b = f.bytes;
    b = header();
    const auto mark = [&](const std::string &name) { f.offsets[name] = b.size(); f.boundaries.push_back(b.size()); };
    const auto str = [&](const std::string &value) { f.strings.push_back(b.size()); string(b, value); };
    // Version-looking bytes in variable-length fields must not affect framing.
    str(std::string("\xff\0\x03\0\0\0", 6)); str("private fixture"); str("PLAYERCOLOR_TEST");
    mark("gamecore_guid");
    const std::array<std::uint8_t,16> bnw{0xf0,0x4f,0xf3,0x83,0x60,0x3d,0xe7,0x43,0xae,0xdb,0x82,0xc7,0x98,0x67,0xee,0x3f};
    const std::array<std::uint8_t,16> mod{0xf0,0xf7,0x9b,0xbf,0x78,0xe0,0x4e,0x4d,0x8a,0x3e,0x84,0x71,0x2f,0x85,0xaa,0x2b};
    const auto &guid = vp ? mod : bnw;
    b.append(reinterpret_cast<const char *>(guid.data()), guid.size());
    str("1.0.0"); b.append(16, '\x8f'); u32(b, 17);
    mark("slots_version"); u32(b, 3); u32(b, 11); u32(b, 2); str("map");
    const auto arr = [&](char kind, std::uint32_t n) {
        f.counts.push_back(b.size()); u32(b, n);
        for (std::uint32_t i = 0; i < n; ++i) {
            if (kind == 's') str(i % 2 ? std::string("\xff\0",2) : "x");
            else if (kind == 'b') b += static_cast<char>(i % 2);
            else u32(b, i == n - 1 ? 0x80000000U : i);
        }
    };
    arr('i',129); arr('s',129);
    for (unsigned i = 0; i < 4; ++i) arr('i',129);
    arr('s',129); arr('s',129);
    if (vp) { f.counts.push_back(b.size()); u32(b,2); u32(b,1); u32(b,0x80000000); u32(b,0); u32(b,0); }
    mark("archive_version"); u32(b,6);
    const auto base = [&] { u32(b,7); for (unsigned i=0; i<8; ++i) str(i == 5 ? "CUSTOM_TYPE" : ""); };
    // Independent wire fixture: no production serializer or production field
    // descriptors are used to generate input for the reader under test.
    const std::string schema =
        "active_player:i admin_password:s advanced_start_points:i alias:s art_styles:ai autorun:b "
        "autorun_turn_delay:f autorun_turn_limit:i bandwidth:i calendar:i calendar_info:C "
        "civ_adjectives:as civ_descriptions:as civ_passwords:as civ_short_descriptions:as climate:i climate_info:L "
        "era:i email_addresses:as end_turn_timer_length:f flag_decals:as deprecated_force_controls:ab "
        "game_mode:i game_name:s game_speed:i game_started:b game_turn:i game_type:c game_map_type:i "
        "game_update_time:i handicaps:ai last_human_handicaps:ai is_earth_map:b is_internet_game:b "
        "leader_names:as load_file_name:s local_player_email:s map_no_players:b map_random_seed:u "
        "load_wb_scenario:b override_scenario_handicap:b map_script:s max_city_elimination:i max_turns:i "
        "num_minor_civs:i minor_civ_types:as minor_nation_civs:ab dummy_value:b multiplayer_options:ab "
        "net_ids:ai nicknames:as num_victory_infos:i pit_boss_turn_time:i playable_civs:ab player_colors:as "
        "private_game:b quick_combat:b quick_combat_default:b quick_handicap:i quickstart:b random_world_size:b "
        "random_map_script:b ready_players:ab sea_level:i sea_level_info:S dummy_value2:b slot_claims:ai "
        "slot_status:ai smtp_host:s sync_random_seed:u target_score:i team_types:ai transferred_map:b "
        "turn_timer:T turn_timer_type:i city_screen_blocked:b victories:ab white_flags:ab world_info:W "
        "world_size:i game_options:O map_options:O version_string:s turn_notify_steam_invite:ab "
        "turn_notify_email:ab turn_notify_email_address:as";
    std::istringstream fields(schema);
    for (std::string field; fields >> field;) {
        const auto colon = field.find(':');
        const auto name = field.substr(0,colon), type = field.substr(colon+1);
        if (vp && name == "advanced_start_points") continue;
        mark(name);
        if (type[0] == 'a') arr(type[1],129);
        else if (type == "s") str(std::string("raw\0\xff",5));
        else if (type == "b") b += '\1';
        else if (type == "c") b += '\xff';
        else if (type == "f") u32(b,0x7fc12345);
        else if (type == "u") u32(b,0xffffffffU);
        else if (type == "i") u32(b,name == "game_map_type" ? 0x12345678 : 99);
        else if (type == "O") {
            f.counts.push_back(b.size()); u32(b,2);
            str(std::string(4097,'m')); u32(b,1);
            str(std::string(4097,'m')); u32(b,0xffffffffU);
        } else {
            if (type == "W" && !vp) { mark("world_version"); u32(b,2); }
            base();
            const unsigned n = type=="C" ? 0 : type=="L" ? 11 : type=="S" ? 1 : type=="T" ? 4 : vp ? 23 : 19;
            for (unsigned i=0; i<n; ++i) u32(b,1000+i);
        }
    }
    mark("archive_end"); u32(b,2);
    return f;
}
void pregame_tests() {
    for (const bool vp : {false,true}) {
        const auto f = pregame_fixture(vp);
        const auto h = parse(f.bytes);
        const auto layout = vp ? saves::GameStateLayout::vp542_corpus : saves::GameStateLayout::expansion2_version1;
        const auto observe = [&](const std::string &b, saves::PregameBudget budget = {}) {
            return saves::observe_pregame({reinterpret_cast<const std::uint8_t *>(b.data()),b.size()},h,layout,budget);
        };
        const auto p = observe(f.bytes);
        check(p.slots_offset==f.offsets.at("slots_version") && p.archive_offset==f.offsets.at("archive_version") &&
              p.archive_end_offset==f.offsets.at("archive_end") && p.end_offset==f.bytes.size(),
              "Pregame boundary changed or searched for a false version marker");
        check(p.slots.integers.at("game_speed")==11 && p.archive.integers.at("game_speed")==99 &&
              p.archive.integers.at("game_type")==-1 && p.archive.integers.at("game_map_type")==0x12345678,
              "Pregame lost archive precedence evidence or signed-byte game type");
        check(p.archive.integers.at("map_random_seed")==4294967295LL &&
              p.archive.float_bits.at("autorun_turn_delay")==0x7fc12345,
              "Pregame changed unsigned seeds or float bits");
        check(p.archive.integer_arrays.at("team_types").size()==129 &&
              p.archive.integer_arrays.at("team_types").back()==-2147483648LL &&
              p.archive.string_arrays.at("minor_civ_types")[1]==std::string("\xff\0",2),
              "Pregame inherited a 64-slot cap or changed opaque type names");
        check(p.archive.strings.at("admin_password")==std::string("raw\0\xff",5) &&
              p.game_options.size()==2 && p.game_options[0].name.size()==4097 &&
              p.game_options[0].name==p.game_options[1].name && p.game_options[1].value==-1,
              "Pregame dropped raw strings, duplicate options or used a 64-byte option-name buffer");
        check(p.world.values.integers.at("research_percent")== (vp ? 1013 : 1014) &&
              p.world.values.integers.at("num_cities_tech_cost_mod")== (vp ? 1016 : 1018) &&
              p.world.wire_version.has_value()==!vp && p.archive.integers.contains("advanced_start_points")==!vp,
              "BNW/VP WorldInfo or advanced-start layouts were conflated");
        check(p.wire_known_player_masks.has_value()==vp &&
              (!vp || (*p.wire_known_player_masks==std::vector<std::uint64_t>{0x8000000000000001ULL,0})),
              "VP known-player mask width or presence changed");
        auto budget = saves::PregameBudget{};
        budget.total_bytes=f.bytes.size(); check(observe(f.bytes,budget).end_offset==f.bytes.size(),"Exact prefix budget rejected");
        --budget.total_bytes; rejects([&]{ observe(f.bytes,budget); });
        budget={}; budget.elements=0; rejects([&]{ observe(f.bytes,budget); });
        budget={}; budget.string_bytes=4096; rejects([&]{ observe(f.bytes,budget); });
        budget={}; budget.total_string_bytes=8194; rejects([&]{ observe(f.bytes,budget); });
        for (const auto pos : f.boundaries) rejects([&]{ observe(f.bytes.substr(0,pos)); });
        rejects([&]{ observe(f.bytes.substr(0,f.bytes.size()-1)); });
        rejects([&]{ observe(f.bytes+'x'); });
        for (const auto pos : f.counts) {
            auto broken=f.bytes; broken.replace(static_cast<std::size_t>(pos),4,4,'\xff');
            rejects([&]{ observe(broken); });
        }
        for (const auto pos : f.strings) {
            // A sampled set includes header-tail, metadata, archive and option strings.
            if (pos%17!=0) continue;
            auto broken=f.bytes; broken.replace(static_cast<std::size_t>(pos),4,4,'\xff');
            rejects([&]{ observe(broken); });
        }
        for (auto name : {"gamecore_guid","slots_version","archive_version","autorun","archive_end"}) {
            auto broken=f.bytes; broken[static_cast<std::size_t>(f.offsets.at(name))]='\x7f';
            rejects([&]{ observe(broken); });
        }
        if (!vp) {
            auto broken=f.bytes; broken[static_cast<std::size_t>(f.offsets.at("world_version"))]=3;
            rejects([&]{ observe(broken); });
        }
        rejects([&]{ saves::observe_pregame({reinterpret_cast<const std::uint8_t *>(f.bytes.data()),f.bytes.size()},
                    h,vp?saves::GameStateLayout::expansion2_version1:saves::GameStateLayout::vp542_corpus); });
    }
}
