#include "civ5/assets/asset_source.hpp"
#include "civ5/content/mod_activation.hpp"
#include "civ5/content/rules_bootstrap.hpp"
#include "civ5/content/save_dimensions.hpp"
#include "civ5/content/saved_culture.hpp"
#include "civ5/platform/steam_installation.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/saves/all_players.hpp"
#include "civ5/saves/supporting_classes.hpp"
#include "civ5/saves/army_operations.hpp"
#include "civ5/saves/chunked_stream.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/cities.hpp"
#include "civ5/saves/city_core.hpp"
#include "civ5/saves/city_management.hpp"
#include "civ5/saves/city_production.hpp"
#include "civ5/saves/city_tail.hpp"
#include "civ5/saves/expansion2_prefix.hpp"
#include "civ5/saves/game_history.hpp"
#include "civ5/saves/game_leagues.hpp"
#include "civ5/saves/game_state.hpp"
#include "civ5/saves/game_tail.hpp"
#include "civ5/saves/game_trade.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include "civ5/saves/inflate_stream.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/save_payload.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/saves/map_regions.hpp"
#include "civ5/saves/map_state.hpp"
#include "civ5/saves/player_administration.hpp"
#include "civ5/saves/player_core.hpp"
#include "civ5/saves/player_diplomacy.hpp"
#include "civ5/saves/player_espionage.hpp"
#include "civ5/saves/player_knowledge.hpp"
#include "civ5/saves/player_planning.hpp"
#include "civ5/saves/player_strategy.hpp"
#include "civ5/saves/player_tail.hpp"
#include "civ5/saves/player_trade_culture.hpp"
#include "civ5/saves/player_traits.hpp"
#include "civ5/saves/team_state.hpp"
#include "civ5/saves/unit_core.hpp"
#include "civ5/saves/units.hpp"
#include "civ5/saves/vp_prefix.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_cities_profile.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include "civ5/saves/vp_city_tail_profile.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace civ5;
namespace {
unsigned checks = 0;
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, std::source_location location = std::source_location::current()) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    ++checks;
    if (!rejected)
        throw std::runtime_error("Expected rejection at line " + std::to_string(location.line()));
}
void write(const fs::path &path, const std::string &bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!file)
        throw std::runtime_error("Cannot write fixture");
}
void u32(std::string &bytes, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        bytes += static_cast<char>(value >> (8 * i));
}
void string(std::string &bytes, const std::string &value) {
    u32(bytes, static_cast<std::uint32_t>(value.size()));
    bytes += value;
}
std::string header(std::uint32_t mods = 1, std::uint32_t turn = 266) {
    std::string bytes = "CIV5";
    u32(bytes, 8);
    string(bytes, "1.0.3.279 (403694)");
    string(bytes, "403694");
    u32(bytes, turn);
    bytes += '\0';
    for (const auto *value : {"CIVILIZATION_INCA", "HANDICAP_IMMORTAL", "ERA_ANCIENT", "ERA_MODERN",
                              "GAMESPEED_STANDARD", "WORLDSIZE_HUGE", "Custom/Map.lua"})
        string(bytes, value);
    u32(bytes, 1);
    const unsigned char guid[]{0xe3, 0x1e, 0x3c, 0x29, 0x76, 0x11, 0xf6, 0x44,
                               0xac, 0x1f, 0x59, 0x66, 0x38, 0x26, 0xde, 0x74};
    bytes.append(reinterpret_cast<const char *>(guid), sizeof(guid));
    u32(bytes, 0);
    string(bytes, "Mongolia");
    u32(bytes, mods);
    for (std::uint32_t i = 0; i < mods; ++i) {
        string(bytes, "AUTHORED-NON-UUID-" + std::to_string(i));
        u32(bytes, 151);
        string(bytes, "Synthetic mod");
    }
    return bytes;
}
saves::SaveHeader parse(const std::string &bytes, saves::HeaderBudget budget = {}) {
    std::istringstream stream(bytes);
    runtime::BinaryReader reader(stream, bytes.size());
    return saves::read_header(reader, budget);
}
// Independent virtual stream with a valid value beyond 4 GiB; exercises real
// streamoff/seek and byte counts without allocating a multi-gigabyte fixture.
class LargeStream final : public std::streambuf {
    std::uint64_t position_ = 0;

  protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode) override {
        if (direction != std::ios_base::beg || offset < 0)
            return pos_type(off_type(-1));
        position_ = static_cast<std::uint64_t>(offset);
        return pos_type(offset);
    }
    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override {
        return seekoff(off_type(position), std::ios_base::beg, mode);
    }
    std::streamsize xsgetn(char *destination, std::streamsize count) override {
        for (std::streamsize i = 0; i < count; ++i, ++position_) {
            if (position_ >= 0x100000020ull && position_ < 0x100000024ull)
                destination[i] = static_cast<char>(
                    (0x78563412u >> (8 * static_cast<unsigned>(position_ - 0x100000020ull))) & 255);
            else
                destination[i] = 0;
        }
        return count;
    }
};
void binary_tests() {
    LargeStream buffer;
    std::istream stream(&buffer);
    runtime::BinaryReader large(stream, 0x100000100ull);
    large.seek(0x100000020ull);
    check(large.u32() == 0x78563412u, "Read beyond 4 GiB lost offset");
    check(large.position() == 0x100000024ull, "64-bit cursor truncated");
    rejects([&] { large.seek(0x100000101ull); });
    const auto bytes = header();
    const auto parsed = parse(bytes);
    check(parsed.end_offset == bytes.size() && parsed.turn == 266, "Header boundary/turn mismatch");
    check(parsed.dlcs.at(0).id == "293c1ee3-1176-44f6-ac1f-59663826de74", "Windows GUID byte order");
    check(parsed.mods.at(0).id == "AUTHORED-NON-UUID-0" && parsed.mods.at(0).wire_value == 151,
          "Authored mod identity changed");
    check(parse(header(0, 0xf0000000u)).turn == 0xf0000000ull, "Imported turn narrowed to signed x86 int");
    for (std::size_t n = 0; n < bytes.size(); ++n)
        rejects([&] { parse(bytes.substr(0, n)); });
    auto bad = bytes;
    bad[0] = 'X';
    rejects([&] { parse(bad); });
    bad = bytes;
    bad[4] = 9;
    rejects([&] { parse(bad); });
    bad = bytes;
    bad[44] = 1;
    rejects([&] { parse(bad); });
    rejects([&] { parse(bytes, {1024, 64}); });
    check(runtime::valid_utf8("\xd0\x94\xf0\x9f\x98\x80"), "Valid UTF-8 rejected");
    for (const auto *value : {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82"})
        check(!runtime::valid_utf8(value), "Invalid UTF-8 accepted");
    auto duplicate = header(2);
    const auto second = duplicate.rfind("AUTHORED-NON-UUID-1");
    duplicate.replace(second, std::string("AUTHORED-NON-UUID-1").size(), "AUTHORED-NON-UUID-0");
    rejects([&] { parse(duplicate); });
}
void frame_tests() {
    std::string bytes(19, 'x');
    u32(bytes, 65536);
    bytes += '\x78';
    bytes += '\x9c';
    bytes.append(65534, 'a');
    u32(bytes, 4);
    bytes.append("\0\0\xff\xff", 4);
    std::istringstream stream(bytes);
    runtime::BinaryReader reader(stream, bytes.size());
    const auto frame = saves::find_frame(reader, 3);
    check(frame.offset == 19 && frame.chunks.size() == 2 && frame.end == bytes.size() &&
              frame.sync_flush_suffix,
          "Frame parse mismatch");
    std::uint64_t visited = 0;
    saves::visit_compressed_chunks(reader, frame, [&](auto chunk) { visited += chunk.size(); });
    check(visited == 65540, "Chunk framing leaked into compressed bytes");
    rejects([&] { saves::find_frame(reader, 3, 5); });
    for (auto tail : {bytes.substr(0, bytes.size() - 1), bytes + 'X', bytes.substr(0, 19 + 4 + 65536)}) {
        std::istringstream broken(tail);
        runtime::BinaryReader input(broken, tail.size());
        rejects([&] { saves::read_frame_at(input, 19); });
    }
    // Sync-flush suffix split over two physical chunks.
    bytes[19 + 4 + 65534] = '\0';
    bytes[19 + 4 + 65535] = '\0';
    bytes.resize(19 + 4 + 65536);
    u32(bytes, 2);
    bytes.append("\xff\xff", 2);
    std::istringstream split(bytes);
    runtime::BinaryReader input(split, bytes.size());
    check(saves::read_frame_at(input, 19).sync_flush_suffix, "Cross-chunk suffix failed");
}
std::string frame_bytes(const std::string &payload) {
    std::string result;
    for (std::size_t offset = 0; offset < payload.size(); offset += 65536) {
        const auto size = std::min<std::size_t>(65536, payload.size() - offset);
        u32(result, static_cast<std::uint32_t>(size));
        result.append(payload, offset, size);
    }
    return result;
}
void inflate_tests() {
    // RFC 1950/1951 stored block generated without zlib's compressor. The
    // body crosses the Civ V 64-KiB framing boundary and exercises checksum.
    std::string compressed("\x78\x9c\x01\xff\xff\0\0", 7);
    compressed.append(65535, 'A');
    std::uint32_t a = 1, b = 0;
    for (unsigned i = 0; i < 65535; ++i) {
        a = (a + 'A') % 65521;
        b = (b + a) % 65521;
    }
    const auto checksum = (b << 16) | a;
    for (unsigned i = 0; i < 4; ++i)
        compressed += static_cast<char>(checksum >> (24 - 8 * i));
    const auto observe = [](const std::string &joined, std::uint64_t budget) {
        const auto physical = frame_bytes(joined);
        std::istringstream stream(physical);
        runtime::BinaryReader reader(stream, physical.size());
        const auto frame = saves::read_frame_at(reader, 0);
        std::uint64_t emitted = 0;
        const auto result = saves::observe_inflated(
            reader, frame,
            [&](auto bytes) {
                for (auto byte : bytes)
                    if (byte != 'A')
                        throw std::runtime_error("Wrong inflated data");
                emitted += bytes.size();
            },
            budget);
        check(emitted == 65535 && result.available_bytes == emitted &&
                  result.compressed_bytes == joined.size(),
              "Inflate counts mismatch");
        return result;
    };
    check(observe(compressed, 0x100000000ull).stream_end, "Complete zlib stream end missing");
    rejects([&] { observe(compressed, 65534); });
    auto corrupted = compressed;
    corrupted.back() ^= 1;
    rejects([&] { observe(corrupted, 65535); });
    rejects([&] { observe(compressed + 'X', 65535); });
    compressed.resize(compressed.size() - 4);
    compressed[2] = '\0';
    compressed.append("\0\0\0\xff\xff", 5);
    const auto unfinished = observe(compressed, 65535);
    check(!unfinished.stream_end && unfinished.sync_flush_suffix,
          "Sync flush falsely accepted as zlib stream end");
}
void save_payload_tests(const fs::path &root) {
    std::string compressed("\x78\x9c\x01\xff\xff\0\0", 7);
    compressed.append(65535, 'A');
    std::uint32_t a = 1, b = 0;
    for (unsigned i = 0; i < 65535; ++i) {
        a = (a + 'A') % 65521;
        b = (b + a) % 65521;
    }
    const auto checksum = (b << 16) | a;
    for (unsigned i = 0; i < 4; ++i)
        compressed += static_cast<char>(checksum >> (24 - 8 * i));
    const auto path = root / "payload.Civ5Save";
    write(path, header(0) + frame_bytes(compressed));
    const auto payload = saves::read_save_payload(path);
    check(payload.header.mods.empty() && payload.frame.offset == payload.header.end_offset &&
              payload.inflate.stream_end && payload.bytes.size() == 65535 &&
              std::all_of(payload.bytes.begin(), payload.bytes.end(), [](std::uint8_t byte) {
                  return byte == static_cast<std::uint8_t>('A');
              }),
          "Save payload loader did not preserve the decompressed state");
    const auto loaded = saves::read_loaded_save(path);
    check(loaded.container == saves::SaveContainerKind::civ5 && loaded.header.turn == payload.header.turn &&
              loaded.frame.end == payload.frame.end && loaded.bytes == payload.bytes,
          "Unified loaded-save reader changed the original payload");
    const auto header_bytes = header(0);
    check(payload.source_prefix == std::vector<std::uint8_t>(header_bytes.begin(),header_bytes.end()) &&
          loaded.source_prefix && *loaded.source_prefix == payload.source_prefix,
          "Original prefix was dropped when setup gap was empty");
    rejects([&] { saves::read_save_payload(path, {{}, 64 * 1024 * 1024, 65534}); });
    constexpr char setup_bytes[]="\0\xff\xfe\x80\0unparsed setup\0\0\x01\0\x78\x9cgarbage";
    const std::string setup(setup_bytes,sizeof(setup_bytes)-1);
    const auto prefixed = root / "setup-prefix.Civ5Save";
    const auto original_prefix = header_bytes + setup;
    write(prefixed, original_prefix + frame_bytes(compressed));
    const auto with_setup = saves::read_save_payload(prefixed);
    check(with_setup.source_prefix == std::vector<std::uint8_t>(original_prefix.begin(),original_prefix.end()) &&
          with_setup.frame.offset == original_prefix.size() && with_setup.bytes == payload.bytes &&
          with_setup.header == payload.header, "Non-UTF8 opaque setup or false frame candidate changed imported bytes");
    auto limits=saves::SavePayloadBudget{};
    limits.maximum_source_prefix_bytes=original_prefix.size()-1;
    rejects([&] { saves::read_save_payload(prefixed,limits); });
    limits.maximum_source_prefix_bytes=original_prefix.size();
    check(saves::read_save_payload(prefixed,limits).source_prefix == with_setup.source_prefix,
          "Prefix budget rejected its exact boundary");
    auto wrong_header=payload.header; ++wrong_header.turn;
    rejects([&] { saves::validate_source_prefix(with_setup.source_prefix,wrong_header); });
    wrong_header=payload.header; ++wrong_header.end_offset;
    rejects([&] { saves::validate_source_prefix(with_setup.source_prefix,wrong_header); });
    wrong_header=payload.header; wrong_header.end_offset=0;
    rejects([&] { saves::validate_source_prefix(with_setup.source_prefix,wrong_header); });
    rejects([&] { saves::validate_source_prefix({},payload.header); });
    rejects([&] { saves::validate_source_prefix(with_setup.source_prefix,payload.header,{.total_bytes=1}); });
    check(runtime::identify_bytes({}).sha256 == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" &&
          runtime::identify_bytes(std::array<std::uint8_t,3>{'a','b','c'}).sha256 ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "Memory SHA-256 does not match independent standard vectors");
    std::string large(131079,'x'); large[65536]='\0'; large[131072]='\xff';
    write(root/"digest-bytes.bin",large);
    const auto bytes_digest=runtime::identify_bytes({reinterpret_cast<const std::uint8_t *>(large.data()),large.size()});
    check(bytes_digest.bytes==large.size() && bytes_digest.sha256==runtime::identify_file(root/"digest-bytes.bin").sha256,
          "Multi-block file/memory digest mismatch");
    write(root/"digest-abc.bin","abc");
    check(runtime::file_md5(root/"digest-abc.bin")=="900150983cd24fb0d6963f7d28e17f72", "MD5 regression in shared digest path");
}
void civis_save_tests(const fs::path &root) {
    check(saves::civis_save_extension == ".CivisSave", "Civis save extension changed");
    const auto path = root / "civis-save.CivisSave";
    saves::CivisSaveDocument document;
    document.flags = 7;
    document.sections = {{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload), 3,
                          {0, 1, 2, 255}},
                         {static_cast<std::uint32_t>(saves::CivisSaveSectionId::session_state), 0,
                          {9, 8, 7}}};
    saves::write_civis_save(path, document);
    const auto round_trip = saves::read_civis_save(path);
    check(round_trip.version == 1 && round_trip.flags == document.flags &&
              round_trip.sections.size() == document.sections.size() &&
              round_trip.sections[0].bytes == document.sections[0].bytes &&
              round_trip.sections[1].flags == 0, "Civis save round-trip changed sections");
    rejects([&] { saves::write_civis_save(path, document); });
    check(saves::read_civis_save(path).sections[0].bytes == document.sections[0].bytes,
          "Rejected save publication changed the existing file");
    for (const auto &entry : fs::directory_iterator(root)) {
        check(entry.path().filename().string().find("civis-save.CivisSave.civis-tmp-") != 0,
              "Civis save publication left a temporary sibling");
    }
    const auto metadata = saves::CivisSaveMetadata{std::string(64, 'a'), parse(header(1, 42))};
    const auto metadata_section = saves::make_save_metadata_section(metadata);
    const auto decoded_metadata = saves::read_save_metadata_section(metadata_section);
    check(decoded_metadata.source_sha256 == metadata.source_sha256 &&
              decoded_metadata.header.turn == metadata.header.turn &&
              decoded_metadata.header.dlcs.size() == 1 && decoded_metadata.header.mods.size() == 1,
          "Civis save metadata round-trip changed content profile");
    saves::CivisSaveDocument imported_document;
    imported_document.flags = 1;
    imported_document.sections = {
        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload), 0, {4, 5, 6}},
        metadata_section};
    const auto imported_path = root / "imported.CivisSave";
    saves::write_civis_save(imported_path, imported_document);
    const auto imported = saves::read_imported_civis_save(imported_path);
    check(imported.metadata.header.mods.size() == 1 && imported.payload == std::vector<std::uint8_t>{4, 5, 6},
          "Imported Civis save did not restore metadata and payload");
    const auto loaded = saves::read_loaded_save(imported_path);
    check(loaded.container == saves::SaveContainerKind::civis &&
              loaded.source_sha256 == metadata.source_sha256 && loaded.header.turn == metadata.header.turn &&
              loaded.bytes == imported.payload && loaded.inflate.available_bytes == loaded.bytes.size(),
          "Unified loaded-save reader changed the Civis payload or provenance");
    check(!imported.source_prefix && !loaded.source_prefix,"Legacy missing setup was silently represented as an empty setup");
    const auto original_header=header(1,42);
    const std::string opaque("\0\xff\xfe\x80not decoded\0",16);
    const auto prefix_bytes=original_header+opaque;
    saves::CivisSaveSection prefix{static_cast<std::uint32_t>(saves::CivisSaveSectionId::source_prefix),0,
        {prefix_bytes.begin(),prefix_bytes.end()}};
    auto complete=imported_document; complete.sections.push_back(prefix);
    const auto complete_path=root/"complete-import.CivisSave";
    saves::write_civis_save(complete_path,complete);
    const auto restored=saves::read_imported_civis_save(complete_path);
    const auto restored_loaded=saves::read_loaded_save(complete_path);
    check(restored.source_prefix && *restored.source_prefix==prefix.bytes &&
          restored_loaded.source_prefix==restored.source_prefix && restored_loaded.bytes==imported.payload &&
          restored.metadata.header==metadata.header,
          "Imported Civis container dropped original prefix or changed the state/header");
    const runtime::SessionRandomStreams initial_random{
        runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0x100000000ULL,2}),
        runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{11,23,5})};
    const runtime::SessionCheckpointIdentity random_identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto random_section=runtime::make_simulation_random_checkpoint(initial_random,random_identity);
    auto random_document=complete;random_document.sections.push_back(random_section);
    const auto random_path=root/"random-import.CivisSave";
    saves::write_civis_save(random_path,random_document);
    const auto random_loaded=saves::read_loaded_save(random_path);
    check(random_loaded.live_simulation_random==std::optional{random_section.bytes},
          "Imported Civis container dropped the common RNG section");
    const auto random_reimport=saves::make_civis_import_document(random_loaded,std::string(64,'c'));
    check(std::any_of(random_reimport.sections.begin(),random_reimport.sections.end(),[&](const auto &section){
        return section.id==random_section.id&&section.bytes==random_section.bytes;
    }),"Reimport discarded the live RNG checkpoint");
    auto clock_document=complete;
    const saves::CivisSaveSection clock_section{
        static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_clock),0,{1,2,3}};
    clock_document.sections.push_back(clock_section);
    const auto clock_path=root/"clock-import.CivisSave";
    saves::write_civis_save(clock_path,clock_document);
    const auto clock_loaded=saves::read_loaded_save(clock_path);
    check(clock_loaded.live_bnw_game_clock==std::optional{clock_section.bytes},
          "Imported Civis container dropped the BNW clock section");
    const auto clock_reimport=saves::make_civis_import_document(clock_loaded,std::string(64,'c'));
    check(std::any_of(clock_reimport.sections.begin(),clock_reimport.sections.end(),[&](const auto &section){
        return section.id==clock_section.id&&section.bytes==clock_section.bytes;
    }),"Reimport discarded the BNW clock checkpoint");
    auto reverse=complete; std::reverse(reverse.sections.begin(),reverse.sections.end());
    saves::write_civis_save(root/"reverse-import.CivisSave",reverse);
    check(saves::read_imported_civis_save(root/"reverse-import.CivisSave").source_prefix==restored.source_prefix,
          "Prefix loader relies on section ordering");
    unsigned invalid_case=0;
    const auto invalid=[&](saves::CivisSaveDocument candidate) {
        const auto bad_path=root/("bad-prefix-"+std::to_string(invalid_case++)+".CivisSave");
        rejects([&]{saves::write_civis_save(bad_path,candidate);});
        check(!fs::exists(bad_path),"Malformed import was published before refusal");
        // Deliberately corrupt a generic container's flags to preserve the
        // independent loader negative test despite stricter production writes.
        candidate.flags=0;saves::write_civis_save(bad_path,candidate);
        {std::fstream wire(bad_path,std::ios::binary|std::ios::in|std::ios::out);
         wire.seekp(12);const char import_flags[4]={1,0,0,0};
         wire.write(import_flags,4);wire.flush();check(static_cast<bool>(wire),"Cannot corrupt fixture import flags");}
        rejects([&]{saves::read_imported_civis_save(bad_path);});
    };
    auto bad=complete; bad.sections.back().flags=1; invalid(bad);
    bad=complete; bad.sections.back().bytes.clear(); invalid(bad);
    bad=complete; bad.sections.back().bytes.resize(original_header.size()-1); invalid(bad);
    bad=complete; bad.sections.back().bytes[0]='X'; invalid(bad);
    auto other_header=metadata; ++other_header.header.turn;
    bad=complete; bad.sections[1]=saves::make_save_metadata_section(other_header); invalid(bad);
    other_header=metadata; other_header.header.dlcs[0].wire_value++;
    bad=complete; bad.sections[1]=saves::make_save_metadata_section(other_header); invalid(bad);
    other_header=metadata; other_header.header.mods[0].name="Different mod";
    bad=complete; bad.sections[1]=saves::make_save_metadata_section(other_header); invalid(bad);
    bad=complete; bad.sections.back().id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::session_state); invalid(bad);
    bad=complete; bad.sections.back().id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::rules_snapshot); invalid(bad);
    bad=complete; bad.sections.back().id=77; invalid(bad);
    bad=complete; bad.sections.erase(bad.sections.begin()+1); invalid(bad);
    const auto selection = runtime::select_save(imported_path);
    check(selection.selected && selection.candidates[*selection.selected].header.turn == metadata.header.turn,
          "Save selection did not accept a Civis container");
    auto duplicate = document;
    duplicate.sections.push_back(document.sections.front());
    rejects([&] { saves::write_civis_save(root / "duplicate.CivisSave", duplicate); });
    rejects([&] { saves::read_civis_save(path, {.maximum_file_bytes = 8}); });
    auto trailing = std::ifstream(path, std::ios::binary);
    std::string raw((std::istreambuf_iterator<char>(trailing)), std::istreambuf_iterator<char>());
    raw.push_back('\0');
    write(root / "trailing.CivisSave", raw);
    rejects([&] { saves::read_civis_save(root / "trailing.CivisSave"); });
}
void session_context_tests() {
    // Check the actual diagnostic, so truncated input cannot accidentally
    // satisfy a test that was supposed to reject incompatible decode flags.
    const auto invalid_context = [](const saves::SavedSessionDecodeContext &context,
                                     std::string_view diagnostic) {
        try {
            (void)saves::observe_saved_session({}, context);
        } catch (const std::runtime_error &error) {
            check(error.what() == diagnostic, "Session rejected the payload before validating its context");
            return;
        }
        check(false, "Invalid session decode context was accepted");
    };
    saves::SavedSessionDecodeContext context;
    context.decode_map_plots = false;
    context.decode_map_regions = true;
    invalid_context(context, "Map regions require map plots");
    context = {};
    context.decode_teams = true;
    invalid_context(context, "Teams require map regions");
    context = {};
    context.decode_map_regions = true;
    context.decode_teams = true;
    context.decode_all_players = true;
    invalid_context(context, "All-player snapshot requires map regions, teams and player context");
    context.all_players = saves::AllPlayersContext{};
    context.decode_teams = false;
    invalid_context(context, "All-player snapshot requires map regions, teams and player context");
    context = {};
    context.decode_supporting_classes = true;
    invalid_context(context, "Supporting classes require map regions, teams and all players");
}
void vp_prefix_tests() {
    std::string bytes;
    u32(bytes, 0);
    for (unsigned i = 0; i < 4; ++i)
        u32(bytes, i);
    string(bytes, "Release-5.4.2-0-g9f0904a01 Clean");
    for (unsigned i = 0; i < 33; ++i)
        u32(bytes, i == 4 ? 0xfffff060u : i);
    const auto observe = [](const std::string &value) {
        std::istringstream stream(value);
        runtime::BinaryReader reader(stream, value.size());
        return saves::observe_vp_prefix(reader);
    };
    const auto result = observe(bytes);
    check(result.end_offset == bytes.size() && result.scalars[4] == -4000 && result.scalars[32] == 32,
          "VP prefix wire order/sign mismatch");
    rejects([&] { observe(bytes.substr(0, bytes.size() - 1)); });
    bytes[0] = 1;
    rejects([&] { observe(bytes); });
    bytes[0] = 0;
    bytes[24] = 'X';
    rejects([&] { observe(bytes); });
}
void expansion2_prefix_tests() {
    std::string bytes;
    u32(bytes, 1);
    for (unsigned i = 0; i < 29; ++i)
        u32(bytes, i == 4 ? 0xfffff060u : i);
    const auto observe = [](const std::string &value) {
        std::istringstream stream(value);
        runtime::BinaryReader reader(stream, value.size());
        return saves::observe_expansion2_prefix(reader);
    };
    const auto result = observe(bytes);
    check(result.end_offset == 120 && result.version == 1 && result.scalars[4] == -4000,
          "Vanilla Expansion2 prefix mismatch");
    check(result.scalars[28] == 28, "Vanilla scalar layout contaminated by VP fields");
    rejects([&] { observe(bytes.substr(0, 119)); });
    bytes[0] = 0;
    rejects([&] { observe(bytes); });
}
void game_state_tests() {
    using saves::GameStateLayout;
    using saves::RosterWireDimensions;
    const auto fixture = [](bool vp, RosterWireDimensions sizes) {
        std::string bytes;
        u32(bytes, vp ? 0 : 1);
        if (vp) {
            for (int i = 0; i < 4; ++i)
                u32(bytes, i);
            string(bytes, "Release-5.4.2-0-g9f0904a01 Clean");
        }
        for (int i = 0; i < (vp ? 33 : 29); ++i)
            u32(bytes, 42);
        bytes.append("\0\1\1\0\0\1", 6);
        if (vp)
            u32(bytes, 0xffffffffu);
        bytes += '\0';
        if (vp)
            bytes += '\1';
        bytes.append("\1\0\1", 3);
        u32(bytes, 2);
        if (vp) {
            u32(bytes, 0x80000000u);
            u32(bytes, 0xffffffffu);
        } else {
            string(bytes, std::string("\0\xffx", 3));
            string(bytes, "");
        }
        for (int i = 0; i < 12; ++i)
            u32(bytes, i == 6 ? 2 : 0xffffffffu);
        if (vp) {
            bytes += '\xff';
            bytes += '\1';
            for (int i = 0; i < 7; ++i)
                u32(bytes, i);
            for (std::uint64_t i = 0; i < saves::vp_roster_dimensions(sizes)->resource_slots; ++i)
                u32(bytes, 0xffffffffu);
        }
        string(bytes, std::string("\0\xff"
                                  "ab",
                                  4));
        for (std::uint64_t i = 0; i < 7; ++i)
            for (std::uint64_t j = 0; j < (i < 4 ? sizes.player_slots : sizes.team_slots); ++j)
                u32(bytes, j ? static_cast<std::uint32_t>(100 * i + j) : 0x80000000u);
        return bytes;
    };
    const auto observe = [](const std::string &bytes, GameStateLayout layout,
                            std::optional<RosterWireDimensions> sizes, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_state(reader, layout, sizes, budget);
    };
    for (const bool vp : {false, true}) {
        const auto layout = vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1;
        RosterWireDimensions sizes{3, 2};
        if (vp)
            sizes.profile_extensions.emplace<saves::VpRosterWireDimensions>(saves::VpRosterWireDimensions{3});
        else
            check(!saves::vp_roster_dimensions(sizes), "BNW requires VP roster dimensions");
        const auto bytes = fixture(vp, sizes);
        const auto state = observe(bytes, layout, sizes);
        check(state.end_offset == (vp ? 385u : 273u) && state.end_offset == bytes.size(),
              "CvGame control/roster wire boundary changed");
        check(state.controls_end_offset == (vp ? 293u : 193u) && state.roster.has_value(),
              "Content-dependent boundary missing");
        check(state.roster->script_data == std::vector<std::uint8_t>{0, 255, 'a', 'b'},
              "Opaque script data was normalized or decoded as UTF-8");
        for (const auto &value : state.controls)
            if (std::string_view(value.name) == "game_state")
                check(value.value == 2, "Decode applied native load normalization");
        for (std::size_t i = 0; i < 7; ++i) {
            const auto &array = state.roster->arrays[i];
            check(array.size() == (i < 4 ? 3u : 2u) && array[0] == -2147483648ll &&
                      array[1] == static_cast<std::int64_t>(100 * i + 1),
                  "Player/team arrays lost order, sign or dimensions");
        }
        if (vp) {
            check(state.advisor_hashes == std::vector<std::uint64_t>{0x80000000ull, 0xffffffffull},
                  "VP hashes were narrowed or treated as strings");
            check(state.controls[13].value == -1, "Signed one-byte circumnavigation team lost sentinel");
            check(std::get<saves::VpPrefixObservation>(state.prefix).scalars[23] == 42,
                  "Decode applied native autoplay normalization");
        } else
            check(state.advisor_messages.size() == 2 &&
                      state.advisor_messages[0] == std::vector<std::uint8_t>{0, 255, 'x'} &&
                      state.advisor_messages[1].empty(),
                  "Vanilla advisor strings were treated as VP hashes");
        const auto prefix = observe(bytes, layout, {});
        check(!prefix.roster && prefix.end_offset == prefix.controls_end_offset,
              "Decoder invented missing wire dimensions");
        auto invalid_script=bytes;
        const auto script_length_offset=state.controls_end_offset+
            (vp?saves::vp_roster_dimensions(sizes)->resource_slots*4:0);
        invalid_script[script_length_offset]=static_cast<char>(0xff);
        try { observe(invalid_script,layout,sizes);check(false,"Oversized script data was accepted"); }
        catch(const std::runtime_error &e){
            const std::string message=e.what();
            check(message.find("script_data length")!=std::string::npos&&
                  message.find("at byte "+std::to_string(script_length_offset))!=std::string::npos,
                  "Game-state string error must identify the field and source byte");
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), layout, sizes); });
        rejects([&] { observe(bytes, layout, sizes, {2, 100, 100}); });
        rejects([&] { observe(bytes, layout, sizes, {100, 2, 100}); });
        rejects([&] { observe(bytes, layout, sizes, {100, 100, 2}); });
        auto wrong = sizes;
        wrong.player_slots = 0;
        rejects([&] { observe(bytes, layout, wrong); });
        wrong = sizes;
        wrong.player_slots = UINT64_MAX;
        rejects([&] { observe(bytes, layout, wrong, {UINT64_MAX, 100, 100}); });
        wrong = sizes;
        if (vp)
            wrong.profile_extensions = {};
        else
            wrong.profile_extensions.emplace<saves::VpRosterWireDimensions>(saves::VpRosterWireDimensions{0});
        rejects([&] { observe(bytes, layout, wrong); });
        auto bad = bytes;
        bad[vp ? 188 : 120] = 2;
        rejects([&] { observe(bad, layout, sizes); });
    }
    RosterWireDimensions wide{4097, 8193};
    wide.profile_extensions.emplace<saves::VpRosterWireDimensions>(saves::VpRosterWireDimensions{1025});
    const auto wide_bytes = fixture(true, wide);
    const auto state = observe(wide_bytes, GameStateLayout::vp542_corpus, wide);
    check(state.roster->arrays[0].size() == 4097 && state.roster->arrays[6].size() == 8193 &&
              state.roster->resource_monopoly_players.size() == 1025,
          "Original slot/resource capacities leaked into host vectors");
    // Place the same fixture past 4 GiB without allocating the intervening bytes.
    class ShiftedStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(pos));
            return pos;
        }
        std::streamsize xsgetn(char *out, std::streamsize count) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, out);
            position_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedStream(const std::string &bytes) : bytes_(bytes) {}
    } shifted(wide_bytes);
    std::istream stream(&shifted);
    runtime::BinaryReader reader(stream, 0x100000123ull + wide_bytes.size());
    reader.seek(0x100000123ull);
    const auto offset_state = saves::observe_game_state(reader, GameStateLayout::vp542_corpus, wide);
    check(offset_state.end_offset == 0x100000123ull + wide_bytes.size() &&
              offset_state.roster->rankings_offset > 0xffffffffull,
          "CvGame observation truncated a 64-bit offset");
}
#include "all_players_tests.hpp"
#include "pregame_tests.hpp"
#include "supporting_classes_tests.hpp"
#include "army_operations_tests.hpp"
#include "cities_tests.hpp"
#include "city_core_tests.hpp"
#include "city_management_tests.hpp"
#include "city_production_tests.hpp"
#include "city_tail_tests.hpp"
#include "game_history_tests.hpp"
#include "game_leagues_tests.hpp"
#include "game_progression_tests.hpp"
#include "session_dimensions_tests.hpp"
#include "simulation_random_tests.hpp"
#include "game_religion_culture_tests.hpp"
#include "game_tail_tests.hpp"
#include "civis_saved_database_tests.hpp"
#include "game_trade_tests.hpp"
#include "map_regions_tests.hpp"
#include "map_state_tests.hpp"
#include "player_administration_tests.hpp"
#include "player_core_tests.hpp"
#include "player_diplomacy_tests.hpp"
#include "player_espionage_tests.hpp"
#include "player_knowledge_tests.hpp"
#include "player_planning_tests.hpp"
#include "player_strategy_tests.hpp"
#include "player_tail_tests.hpp"
#include "player_trade_culture_tests.hpp"
#include "player_traits_tests.hpp"
#include "team_state_tests.hpp"
#include "unit_core_tests.hpp"
#include "units_tests.hpp"
#include "mod_action_order_tests.hpp"
void mod_sql_action_result_tests(const fs::path &root) {
    struct Case {
        const char *name;
        const char *first_sql;
        bool first_applied;
        bool release_diagnostic;
        bool trigger_error;
        std::vector<std::int64_t> expected_ids;
    };
    const std::array<Case, 5> cases{{
        {"ordinary", "INSERT INTO F29Rows VALUES(1,'first');", true, false, false, {1,2}},
        {"commit", "INSERT INTO F29Rows VALUES(1,'first'); COMMIT;", true, true, false, {1,2}},
        {"rollback", "INSERT INTO F29Rows VALUES(1,'first'); ROLLBACK;", true, true, false, {2}},
        {"syntax", "INSERT INTO F29Rows VALUES(1,'first'); F29_NOT_SQL;", false, false, false, {1,2}},
        {"trigger_rollback",
         "CREATE TRIGGER F29Rollback BEFORE INSERT ON F29Rows WHEN NEW.ID=3 "
         "BEGIN SELECT RAISE(ROLLBACK,'f29 forced rollback'); END; "
         "INSERT INTO F29Rows VALUES(1,'first'); INSERT INTO F29Rows VALUES(3,'rollback');",
         false, true, true, {2}}
    }};
    for (const auto &test : cases) {
        // Only owned synthetic SQL/manifest bytes; no original assets are copied.
        const auto directory = root / "f29-sql-action" / test.name;
        write(directory / "first.sql", test.first_sql);
        write(directory / "next.sql", "INSERT INTO F29Rows VALUES(2,'next');");
        write(directory / "fixture.modinfo",
              "<Mod id='f29-sql-action' version='1'><Files><File md5='" +
              runtime::file_md5(directory / "first.sql") + "'>first.sql</File><File md5='" +
              runtime::file_md5(directory / "next.sql") + "'>next.sql</File></Files>"
              "<Actions><OnModActivated><UpdateDatabase>first.sql</UpdateDatabase>"
              "<UpdateDatabase>next.sql</UpdateDatabase></OnModActivated></Actions></Mod>");
        content::Catalog catalog;
        catalog.packages.push_back(content::read_manifest(directory / "fixture.modinfo"));
        const std::array<std::size_t,1> selected{0};
        const auto profile = runtime::resolve_selected_content_profile({}, selected, catalog);
        check(profile.can_activate && profile.all_requirements_located,
              "F29 generated mod fixture is not a verified selected profile");
        const auto plan = content::plan_saved_database_actions(profile, catalog);
        check(plan.all_sources_present && plan.ordered_mods == std::vector<std::size_t>{0} &&
                  plan.actions.size() == 2 && plan.actions[0].requested_path == "first.sql" &&
                  plan.actions[1].requested_path == "next.sql",
              "F29 actual planner changed authored SQL action order");
        content::RulesDatabase database;
        database.apply_sql("CREATE TABLE F29Rows(ID INTEGER PRIMARY KEY, Value TEXT);");
        // Test the real public saved-mod dispatcher, not a copy of its result rule
        // or just the lower apply_mod_sql API which already has separate diagnostics.
        const auto report = content::apply_saved_database_actions(database, plan, catalog);
        check(report.actions.size() == 2 && report.actions[0].action.requested_path == "first.sql" &&
                  report.actions[1].action.requested_path == "next.sql" &&
                  report.actions[0].manifest_md5_match && report.actions[1].manifest_md5_match,
              "F29 real dispatcher lost a verified action/source identity");
        check(report.actions[0].applied == test.first_applied,
              "F29 RELEASE diagnostic replaced ExecuteMultiple's actual action bool");
        check(report.actions[1].applied && report.actions[1].error.empty(),
              "F29 following valid action was skipped or inherited an earlier SQL error");
        check(report.all_succeeded == test.first_applied,
              "F29 aggregate classified a diagnostic as failure or erased a true SQL failure");
        if (test.first_applied) {
            check(report.actions[0].error.empty() != test.release_diagnostic,
                  "F29 successful SQL lost its separate RELEASE diagnostic");
            if (test.release_diagnostic)
                check(report.actions[0].error.find("savepoint") != std::string::npos,
                      "F29 successful COMMIT/ROLLBACK diagnostic is not the actual release error");
        } else {
            check(!report.actions[0].error.empty(), "F29 SQL failure diagnostic was discarded");
            if (test.trigger_error)
                check(report.actions[0].error.find("f29 forced rollback") != std::string::npos,
                      "F29 release error masked the actual trigger execution error");
        }
        std::vector<std::int64_t> ids;
        const auto rows = database.query({"SELECT ID,Value FROM F29Rows ORDER BY ID", {}},
            [&](std::span<const content::DatabaseValue> row) {
                const auto id = std::get<std::int64_t>(row[0]);
                ids.push_back(id);
                check(std::get<std::string>(row[1]) == (id == 1 ? "first" : "next"),
                      "F29 wrong action bytes produced the visible row");
            });
        check(ids == test.expected_ids && rows == ids.size(),
              "F29 COMMIT/partial prefix/ROLLBACK/following mutation or once count differs");
        // Activation uses an ordinary gameplay DB. Its next real clone must
        // reject a leaked transaction without COMMIT/ROLLBACK cleanup here.
        const auto clone = database.clone_gameplay();
        std::vector<std::int64_t> cloned_ids;
        clone->query({"SELECT ID FROM F29Rows ORDER BY ID", {}},
            [&](std::span<const content::DatabaseValue> row) {
                cloned_ids.push_back(std::get<std::int64_t>(row[0]));
            });
        check(cloned_ids == ids, "F29 activated gameplay clone lost committed rows");
    }
}
void content_tests(const fs::path &root) {
    const auto mod = root / "mods/test.modinfo";
    const std::string xml =
        "\xef\xbb\xbf<?xml version=\"1.0\"?><Mod id=\"Authored-Non-Uuid-0\" version=\"151\">"
        "<Properties><Name>Fixture &amp; mod</Name></Properties><Dependencies><Game "
        "minversion=\"1.0.3.124\"/></Dependencies>"
        "<Files><File import=\"1\">data\\rows.sql</File></Files>"
        "<Actions><OnModActivated><UpdateDatabase>data\\rows.sql</UpdateDatabase><UpdateDatabase>second.sql</"
        "UpdateDatabase></OnModActivated></Actions></Mod>";
    write(mod, xml);
    write(root / "mods/data/rows.sql", "select 1;");
    const auto package = content::read_manifest(mod);
    check(package.id == "authored-non-uuid-0" && package.name == "Fixture & mod" && package.version == 151,
          "Manifest identity or entities");
    check(package.document.child("Dependencies")->children.front().attribute("minversion") == "1.0.3.124",
          "Game requirement lost");
    check(package.document.child("Actions")->children.front().children[1].text == "second.sql",
          "Database action order changed");
    auto catalog = content::discover_packages({root / "mods", root / "mods"});
    check(catalog.packages.size() == 1 && catalog.diagnostics.empty(),
          "Overlapping discovery duplicated a manifest");
    write(root / "mirror/test.modinfo", xml);
    write(root / "mirror/data/rows.sql", "select 1;");
    const auto mirrored = content::discover_packages({root / "mods", root / "mirror"});
    const auto reversed = content::discover_packages({root / "mirror", root / "mods"});
    check(mirrored.packages.size() == 1 && reversed.packages.size() == 1 &&
              mirrored.packages[0].path == reversed.packages[0].path &&
              mirrored.diagnostics.empty() && reversed.diagnostics.empty(),
          "Byte-identical local/cache mod mirrors must select one stable source");
    write(root / "mirror/data/rows.sql", "select 2;");
    auto divergent = content::discover_packages({root / "mods", root / "mirror"});
    check(divergent.packages.size() == 2,
          "Same manifest with different mod data must remain ambiguous");
    write(root / "mirror/data/rows.sql", "select 1;");
    write(root / "mirror/extra.lua", "return true");
    divergent = content::discover_packages({root / "mods", root / "mirror"});
    check(divergent.packages.size() == 2,
          "Undeclared mod script differences must remain ambiguous");
    const auto inspection = runtime::inspect_mod_files(catalog, 0);
    check(inspection.present_files == 1 && inspection.diagnostics.empty(), "Declared mod file unavailable");
    auto saved = parse(header());
    check(runtime::locate_saved_content(saved, divergent).back().status ==
              runtime::RequirementStatus::ambiguous,
          "Differing same-version mirrors must block saved content selection");
    auto pinned_source=divergent;
    const std::array<fs::path,1> chosen_source{root / "mirror/test.modinfo"};
    runtime::select_saved_mod_sources(pinned_source,saved,chosen_source);
    check(pinned_source.packages.size()==1&&pinned_source.packages[0].path==fs::canonical(chosen_source[0])&&
          runtime::locate_saved_content(saved,pinned_source).back().status==runtime::RequirementStatus::located,
          "Explicit source must resolve only the saved mod's differing same-version copies");
    const std::array<fs::path,2> duplicated_source{chosen_source[0],root / "mods/test.modinfo"};
    rejects([&]{auto copy=divergent;runtime::select_saved_mod_sources(copy,saved,duplicated_source);});
    const std::array<fs::path,1> absent_source{root / "mods/data/rows.sql"};
    rejects([&]{auto copy=divergent;runtime::select_saved_mod_sources(copy,saved,absent_source);});
    const auto pinned_xml = "<Mod id='mirror-pin' version='6'><Files><File md5='" +
        runtime::file_md5(root / "mods/data/rows.sql") + "'>data/rows.sql</File></Files></Mod>";
    write(root / "verified-cache/pinned.modinfo", pinned_xml);
    write(root / "verified-cache/data/rows.sql", "select 1;");
    write(root / "local-copy/pinned.modinfo", pinned_xml);
    write(root / "local-copy/data/rows.sql", "select 2;");
    auto pinned = content::discover_packages({root / "local-copy", root / "verified-cache"});
    check(pinned.packages.size() == 2 &&
              content::prefer_verified_cached_mods(pinned, root / "verified-cache") == 0 &&
              pinned.packages.size() == 2,
          "Changed local SQL must remain ambiguous despite a manifest-verified cache");
    write(root / "local-copy/extra.lua", "return true");
    pinned = content::discover_packages({root / "local-copy", root / "verified-cache"});
    check(content::prefer_verified_cached_mods(pinned, root / "verified-cache") == 0 &&
              pinned.packages.size() == 2,
          "Unknown extra local script must prevent automatic cache selection");
    const std::array<fs::path,1> unrelated_source{root / "verified-cache/pinned.modinfo"};
    rejects([&]{runtime::select_saved_mod_sources(pinned,saved,unrelated_source);});
    check(pinned.packages.size()==2,"Rejected source choice must preserve the discovered catalog");
    const auto saved_dlc = saved.dlcs.front();
    saved.dlcs.clear();
    auto vanilla_saved = saved;
    vanilla_saved.mods.clear();
    const auto vanilla_profile = runtime::resolve_saved_content_profile(vanilla_saved, catalog);
    check(vanilla_profile.kind == runtime::ContentProfileKind::vanilla &&
              vanilla_profile.all_requirements_located && vanilla_profile.can_activate &&
              vanilla_profile.enabled_dlc.empty() && vanilla_profile.enabled_mods.empty(),
          "Vanilla profile selected an installed mod");
    auto dlc_catalog = catalog;
    content::PackageManifest dlc{};
    dlc.kind = content::PackageKind::dlc;
    dlc.id = saved_dlc.id;
    dlc.name = saved_dlc.name;
    dlc_catalog.packages.push_back(dlc);
    auto dlc_saved = vanilla_saved;
    dlc_saved.dlcs.push_back(saved_dlc);
    const auto dlc_profile = runtime::resolve_saved_content_profile(dlc_saved, dlc_catalog);
    check(dlc_profile.kind == runtime::ContentProfileKind::dlc && dlc_profile.can_activate &&
              dlc_profile.enabled_dlc.size() == 1 && dlc_profile.enabled_mods.empty(),
          "DLC profile did not select only saved DLC");
    auto mod_saved = vanilla_saved;
    mod_saved.mods.push_back(saved.mods.front());
    const auto mod_profile = runtime::resolve_saved_content_profile(mod_saved, catalog);
    check(mod_profile.kind == runtime::ContentProfileKind::modded && mod_profile.can_activate &&
              mod_profile.enabled_mods.size() == 1 && mod_profile.enabled_dlc.empty(),
          "Arbitrary script mod profile was not activatable");
    write(root / "omission/present.xml", "<GameData/>");
    write(root / "omission/absent.lua", "return true");
    const auto present_md5 = runtime::file_md5(root / "omission/present.xml");
    const auto absent_md5 = runtime::file_md5(root / "omission/absent.lua");
    fs::remove(root / "omission/absent.lua");
    write(root / "omission/omission.modinfo",
          "<Mod id='omission-mod' version='3'><Files><File md5='" + present_md5 +
          "'>present.xml</File><File md5='" + absent_md5 + "'>absent.lua</File></Files></Mod>");
    content::Catalog omission_catalog;
    omission_catalog.packages.push_back(content::read_manifest(root / "omission/omission.modinfo"));
    auto omission_saved = vanilla_saved;
    omission_saved.mods.push_back({"omission-mod", {}, 3});
    check(!runtime::resolve_saved_content_profile(omission_saved, omission_catalog).files_complete,
          "Missing mod file passed ordinary content preflight");
    const std::array<runtime::ManifestFileOmission, 1> exact_omission{{
        {"omission-mod", "absent.lua", absent_md5, 3}}};
    const auto omitted_inspection = runtime::inspect_mod_files(omission_catalog, 0, exact_omission);
    check(omitted_inspection.present_files == 1 && omitted_inspection.profile_omitted_files == 1 &&
              omitted_inspection.diagnostics.empty(),
          "Exact profile omission did not account for one absent declared file");
    const auto omitted_profile = runtime::resolve_saved_content_profile(
        omission_saved, omission_catalog, exact_omission);
    check(omitted_profile.files_complete && omitted_profile.can_activate,
          "Explicit file omission did not reach the saved content profile");
    auto wrong_omission = exact_omission;
    wrong_omission[0].md5 = "wrong";
    rejects([&] { (void)runtime::resolve_saved_content_profile(
        omission_saved, omission_catalog, wrong_omission); });
    rejects([&] { (void)runtime::resolve_saved_content_profile(
        vanilla_saved, omission_catalog, exact_omission); });
    auto native_catalog = omission_catalog;
    native_catalog.packages[0].files.push_back({"native.dll", "ef56", false});
    const std::array<runtime::ManifestFileOmission, 1> native_omission{{
        {"omission-mod", "native.dll", "ef56", 3}}};
    rejects([&] { (void)runtime::inspect_mod_files(native_catalog, 0, native_omission); });
    const std::array<std::size_t,1> native_selection{0};
    const auto unavailable_native=runtime::resolve_selected_content_profile(
        {},native_selection,native_catalog);
    check(unavailable_native.native_mods_require_port&&!unavailable_native.can_activate,
          "Missing native DLL lost the x64 GameCore port requirement");
    write(root / "omission/absent.lua", "return true");
    const auto now_present = runtime::inspect_mod_files(omission_catalog, 0, exact_omission);
    check(now_present.present_files == 2 && now_present.profile_omitted_files == 0,
          "Profile omission hid a file that is actually present");
    fs::remove(root / "omission/absent.lua");
    fs::create_directory(root / "omission/absent.lua");
    const auto directory_instead = runtime::inspect_mod_files(omission_catalog, 0, exact_omission);
    check(directory_instead.profile_omitted_files == 0 && directory_instead.diagnostics.size() == 1,
          "Profile omission hid a directory in place of a declared file");
    write(root / "omission/present.xml", "<Changed/>");
    const auto altered = runtime::inspect_mod_files(omission_catalog, 0, exact_omission);
    check(std::any_of(altered.diagnostics.begin(),altered.diagnostics.end(),
                      [](const std::string &value){return value.find("MD5 mismatch")!=value.npos;}),
          "Modified manifest file passed content preflight");
    const auto database_plan = content::plan_saved_database_actions(mod_profile, catalog);
    check(database_plan.actions.size() == 2 && !database_plan.all_sources_present &&
              database_plan.actions[0].status == content::DatabaseActivationStatus::ready &&
              database_plan.actions[1].status == content::DatabaseActivationStatus::missing_source,
          "Saved mod database action order or source status changed");
    content::RulesDatabase database;
    const auto database_report =
        content::apply_saved_database_actions(database, database_plan, catalog);
    check(!database_report.all_succeeded && database_report.actions.size() == 2 &&
              database_report.actions[0].applied && !database_report.actions[1].applied,
          "Saved mod database actions were not applied independently");
    write(root / "hash-rules/rules.sql", "CREATE TABLE VerifiedRule(ID INTEGER);");
    const auto rules_md5=runtime::file_md5(root / "hash-rules/rules.sql");
    write(root / "hash-rules/hash.modinfo",
          "<Mod id='hash-rules' version='5'><Files><File md5='"+rules_md5+
          "'>rules.sql</File></Files><Actions><OnModActivated><UpdateDatabase>rules.sql"
          "</UpdateDatabase></OnModActivated></Actions></Mod>");
    content::Catalog hash_catalog;
    hash_catalog.packages.push_back(content::read_manifest(root / "hash-rules/hash.modinfo"));
    const std::array<std::size_t,1> hash_selection{0};
    const auto hash_profile=runtime::resolve_selected_content_profile({},hash_selection,hash_catalog);
    check(hash_profile.files_complete&&hash_profile.can_activate,
          "Unchanged SQL with manifest MD5 failed content preflight");
    const auto hash_plan=content::plan_saved_database_actions(hash_profile,hash_catalog);
    content::RulesDatabase verified_database;
    const auto verified_report=content::apply_saved_database_actions(
        verified_database,hash_plan,hash_catalog);
    check(verified_report.all_succeeded&&verified_report.actions.size()==1&&
              verified_report.actions[0].applied,
          "Unchanged manifest-pinned SQL action did not execute");
    write(root / "hash-rules/rules.sql", "CREATE TABLE TamperedRule(ID INTEGER);");
    check(!runtime::resolve_selected_content_profile({},hash_selection,hash_catalog).files_complete,
          "Modified SQL passed manifest MD5 preflight");
    content::RulesDatabase tampered_database;
    const auto tampered_report=content::apply_saved_database_actions(
        tampered_database,hash_plan,hash_catalog);
    check(!tampered_report.all_succeeded&&!tampered_report.actions[0].applied&&
              tampered_report.actions[0].error=="manifest MD5 mismatch"&&
              tampered_database.apply_mod_sql("CREATE TABLE TamperedRule(ID INTEGER);").success,
          "Modified SQL was applied to the saved rules database");
    content::RulesDatabase installed_database;
    const auto installed_report=content::apply_saved_database_actions(
        installed_database,hash_plan,hash_catalog,64ULL*1024*1024,hash_selection);
    check(installed_report.all_succeeded&&installed_report.actions[0].applied&&
              !installed_report.actions[0].manifest_md5_match&&
              !installed_database.apply_mod_sql("CREATE TABLE TamperedRule(ID INTEGER);").success,
          "Explicit installed-byte replay lost the modified SQL or hid its manifest mismatch");
    auto selected_catalog = catalog;
    content::PackageManifest selected_dlc_manifest{};
    selected_dlc_manifest.kind = content::PackageKind::dlc;
    selected_dlc_manifest.id = saved_dlc.id;
    selected_dlc_manifest.name = saved_dlc.name;
    selected_dlc_manifest.version = saved_dlc.wire_value;
    selected_catalog.packages.push_back(selected_dlc_manifest);
    content::PackageManifest dependent{};
    dependent.kind = content::PackageKind::mod;
    dependent.path = root / "mods/dependent.modinfo";
    dependent.id = "dependent";
    dependent.version = 2;
    dependent.document = content::read_xml_metadata(
        "<Mod id='dependent' version='2'><Dependencies><Mod id='authored-non-uuid-0' "
        "minversion='151' maxversion='151'/><Dlc id='" + saved_dlc.id +
        "' minversion='0' maxversion='999'/></Dependencies></Mod>");
    selected_catalog.packages.push_back(dependent);
    const std::array<std::size_t, 1> selected_dlc_index{1};
    const std::array<std::size_t, 1> selected_mod_index{2};
    const auto selected_profile = runtime::resolve_selected_content_profile(
        selected_dlc_index, selected_mod_index, selected_catalog);
    check(selected_profile.kind == runtime::ContentProfileKind::modded &&
              selected_profile.can_activate && selected_profile.enabled_dlc.size() == 1 &&
              selected_profile.enabled_mods.size() == 2 && selected_profile.enabled_mods[0] == 0 &&
              selected_profile.enabled_mods[1] == 2,
          "Explicit mod profile did not include dependency-first DLC/mod closure");
    auto dependent_saved = vanilla_saved;
    dependent_saved.mods.push_back({"dependent", {}, 2});
    const auto saved_dependency_profile =
        runtime::resolve_saved_content_profile(dependent_saved, selected_catalog);
    check(saved_dependency_profile.can_activate && saved_dependency_profile.enabled_dlc.size() == 1 &&
              saved_dependency_profile.enabled_mods.size() == 2 &&
              saved_dependency_profile.enabled_mods[0] == 0 &&
              saved_dependency_profile.enabled_mods[1] == 2,
          "Saved mod profile did not resolve manifest dependency closure");
    const auto explicit_vanilla = runtime::resolve_selected_content_profile({}, {}, catalog);
    check(explicit_vanilla.kind == runtime::ContentProfileKind::vanilla &&
              explicit_vanilla.can_activate && explicit_vanilla.enabled_dlc.empty() &&
              explicit_vanilla.enabled_mods.empty(),
          "Explicit vanilla profile selected installed content");
    content::PackageManifest missing_dependency = dependent;
    missing_dependency.id = "missing-dependency";
    missing_dependency.document = content::read_xml_metadata(
        "<Mod id='missing-dependency' version='1'><Dependencies><Mod id='absent' "
        "minversion='0' maxversion='1'/></Dependencies></Mod>");
    selected_catalog.packages.push_back(missing_dependency);
    const std::array<std::size_t, 1> missing_mod_index{3};
    const auto incomplete_profile = runtime::resolve_selected_content_profile(
        {}, missing_mod_index, selected_catalog);
    check(!incomplete_profile.can_activate && !incomplete_profile.all_requirements_located,
          "Missing explicit mod dependency was silently activated");
    auto missing_saved = vanilla_saved;
    missing_saved.mods.push_back({"missing-dependency", {}, 1});
    const auto missing_saved_profile =
        runtime::resolve_saved_content_profile(missing_saved, selected_catalog);
    check(!missing_saved_profile.can_activate && !missing_saved_profile.all_requirements_located,
          "Missing saved mod dependency was silently activated");
    write(root / "dlc/DlcFixture.xml",
          "<GameData><Table name='DlcFixture'><Column name='ID' type='integer' primarykey='true'/></Table>"
          "<DlcFixture><Row ID='1'/></DlcFixture></GameData>");
    auto dlc_source = std::make_shared<assets::LooseResourceIndex>(root / "dlc");
    dlc_source->add_file("DlcFixture.xml");
    assets::ResourceMountTable dlc_resources(assets::resource_lookup(dlc_source));
    auto game_dlc = dlc;
    content::XmlNode package_document;
    package_document.name = "Civ5Package";
    content::XmlNode game_data;
    game_data.name = "GameData";
    game_data.text = "DlcFixture.xml";
    package_document.children.push_back(std::move(game_data));
    game_dlc.document = std::move(package_document);
    content::Catalog game_dlc_catalog;
    game_dlc_catalog.packages.push_back(std::move(game_dlc));
    const std::array<std::size_t, 1> selected_dlc{0};
    const auto dlc_report = content::apply_saved_dlc_gameplay(
        database, selected_dlc, game_dlc_catalog, dlc_resources);
    check(dlc_report.packages.size() == 1 && dlc_report.packages[0].requested == 1 &&
              dlc_report.packages[0].resolved == 1 && dlc_report.packages[0].applied == 1 &&
              dlc_report.packages[0].failed == 0,
          "Selected DLC GameData was not applied through the shared resource table");
    auto missing_catalog = dlc_catalog;
    missing_catalog.packages.erase(missing_catalog.packages.begin());
    const auto missing_profile = runtime::resolve_saved_content_profile(mod_saved, missing_catalog);
    check(!missing_profile.all_requirements_located && !missing_profile.can_activate,
          "Missing mod was allowed into a profile");
    auto located = runtime::locate_saved_content(saved, catalog);
    check(located[0].package == 0, "Exact ID/version not located");
    catalog.packages[0].name = "Another name";
    check(runtime::locate_saved_content(saved, catalog)[0].package == 0, "Display name used as identity");
    catalog.packages[0].version = 152;
    check(runtime::locate_saved_content(saved, catalog)[0].status ==
              runtime::RequirementStatus::version_mismatch,
          "Version silently substituted");
    catalog.packages.push_back(package);
    check(runtime::locate_saved_content(saved, catalog)[0].package == 1,
          "Exact version must beat newer version");
    catalog.packages.push_back(package);
    check(runtime::locate_saved_content(saved, catalog)[0].status == runtime::RequirementStatus::ambiguous,
          "Duplicate exact versions silently selected");
    catalog.packages.clear();
    check(runtime::locate_saved_content(saved, catalog)[0].status == runtime::RequirementStatus::missing,
          "Missing mod not reported");
    for (const auto *relative : {"../outside", "data/../../outside", "C:/outside", "/absolute",
                                 "data/a:stream", "\\\\server\\share", ".. /outside", "data./rows.sql"})
        rejects([&] { content::package_file(root / "mods", relative); });
    rejects([&] { content::package_file(root / "mods", "missing.sql"); });
    write(root / "bad.xml", "<!DOCTYPE Mod [<!ENTITY attack SYSTEM 'file:///absent'>]><Mod>&attack;</Mod>");
    rejects([&] { content::read_xml(root / "bad.xml"); });
    write(root / "bad.xml", "<Mod id='a' id='b'/>");
    rejects([&] { content::read_xml(root / "bad.xml"); });
    write(root / "bad.xml", "<Mod>");
    rejects([&] { content::read_xml(root / "bad.xml"); });
    write(root / "bad.xml", "<Mod id='x' version='1'><Files><File>../outside</File></Files></Mod>");
    rejects([&] { content::read_manifest(root / "bad.xml"); });
    write(root / "bad.xml", "<Mod id='x' version='1'><Files/><Files/></Mod>");
    rejects([&] { content::read_manifest(root / "bad.xml"); });
    write(root / "bad.xml", "<Mod id='x' version='1'><Files><File import='1'>same.dds</File><File "
                            "import='1'>same.dds</File></Files></Mod>");
    check(content::read_manifest(root / "bad.xml").files.size() == 2,
          "Repeated real-world file declarations lost");
    check(!content::discover_packages({root / "missing"}).diagnostics.empty(),
          "Missing root silently ignored");
    write(root / "hash.txt", "abc");
    const auto identity = runtime::identify_file(root / "hash.txt");
    check(identity.bytes == 3 &&
              identity.sha256 == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "Known SHA-256 vector mismatch");
    write(root / "saves/fewer.Civ5Save", header(0, 900));
    write(root / "saves/early.Civ5Save", header(1, 100));
    write(root / "saves/latest.Civ5Save", header(1, 266));
    write(root / "saves/bad.Civ5Save", "C");
    const auto selection = runtime::select_save(root / "saves");
    check(selection.selected &&
              selection.candidates[*selection.selected].path.filename() == "latest.Civ5Save",
          "Save selection priority mismatch");
    check(selection.diagnostics.size() == 1, "Invalid save silently ignored");
}
std::string vdf_escape(std::string text) {
    std::string result;
    for (char c : text) {
        if (c == '\\' || c == '"')
            result += '\\';
        result += c;
    }
    return result;
}
void package_resource_tests() {
    using namespace content;
    PackageManifest package{};
    package.kind = PackageKind::dlc;
    package.document = read_xml_metadata(R"(<Civ5Package>
      <Common><GameData>common-first.xml</GameData></Common>
      <UISkin><GameData>ui-first.xml</GameData><Skin><Directory>first-skin</Directory>
        <Directory><File>skin-list-a</File><Ignore>ignored</Ignore><File>skin-list-b</File></Directory>
        </Skin><Skin><Directory>second-skin-ignored</Directory></Skin></UISkin>
      <GameData>root-a.xml</GameData><GameData><File>root-b.xml</File><file>case-ignored.xml</file>
        <File>root-b.xml</File></GameData><GameData><Other>ignored.xml</Other></GameData>
      <Gameplay><GameData>game-first.xml</GameData><gamedata>ignored.xml</gamedata></Gameplay>
      <Common><GameData>common-second.xml</GameData></Common>
      <Gameplay><GameData>game-second.xml</GameData></Gameplay>
      <UISkin><GameData>ui-second.xml</GameData></UISkin>
    </Civ5Package>)");
    PackageResourceQuery query{"GameData", {true, true, true}, {}, 0};
    const auto values = [&](const PackageResourceQuery &request) {
        std::vector<std::string> result;
        for (const auto &resource : package_resources(package, request))
            result.push_back(resource.value);
        return result;
    };
    check(values(query) == std::vector<std::string>{"root-a.xml", "root-b.xml", "root-b.xml",
                                                    "game-first.xml", "ui-first.xml", "common-first.xml"},
          "Native root/Gameplay/UISkin/Common order or duplicate File entries lost");
    auto resources = package_resources(package, query);
    check(!resources[0].category && !resources[0].file_list && resources[1].file_list,
          "Scalar/File-list resource provenance lost");
    check(resources[3].category == PackageCategory::gameplay &&
              resources[4].category == PackageCategory::ui_skin &&
              resources[5].category == PackageCategory::common,
          "Resource scope lost");
    query.enabled_categories = {};
    check(values(query) == std::vector<std::string>{"root-a.xml", "root-b.xml", "root-b.xml"},
          "All-scope query lost unconditional root");
    query.category = PackageCategory::common;
    check(values(query).empty(), "Disabled Common included root resources");
    query.enabled_categories[0] = true;
    check(values(query) ==
              std::vector<std::string>{"common-first.xml", "root-a.xml", "root-b.xml", "root-b.xml"},
          "Specific Common must visit category before root");
    query.category_occurrence = 1;
    check(values(query).front() == "common-second.xml", "Second Common occurrence not selected");
    query.category_occurrence = 4294967297ULL;
    check(values(query) == std::vector<std::string>{"root-a.xml", "root-b.xml", "root-b.xml"},
          "Missing Common occurrence lost root or wrapped to 32 bits");
    query.category = PackageCategory::gameplay;
    query.enabled_categories[1] = true;
    check(values(query).empty(), "Out-of-range Gameplay occurrence wrapped");
    query.category_occurrence = 1;
    check(values(query) == std::vector<std::string>{"game-second.xml"},
          "Gameplay included root or wrong occurrence");
    query.category = PackageCategory::ui_skin;
    query.enabled_categories[2] = true;
    check(values(query) == std::vector<std::string>{"ui-second.xml"}, "UISkin occurrence selection failed");
    query.category_occurrence = 0;
    query.selector = "Skin/Directory";
    check(values(query) == std::vector<std::string>{"first-skin", "skin-list-a", "skin-list-b"},
          "XML selector traversal incorrectly recursed or treated selector as filename");
    query.selector = "Skin/directory";
    check(values(query).empty(), "Selector node comparison must be case-sensitive");
    for (const auto *bad : {"", "/GameData", "GameData/", "Skin//Directory"}) {
        query.selector = bad;
        rejects([&] { (void)values(query); });
    }
    query.selector = "GameData";
    query.category = static_cast<PackageCategory>(3);
    rejects([&] { (void)values(query); });
    query.category = {};
    query.category_occurrence = 1;
    rejects([&] { (void)values(query); });
    query.category_occurrence = 0;
    package.kind = PackageKind::mod;
    rejects([&] { (void)values(query); });
    package.kind = PackageKind::dlc;
    package.document = read_xml_metadata("<Civ5Package><GameData/></Civ5Package>");
    resources = package_resources(package, query);
    check(resources.size() == 1 && resources[0].value.empty(),
          "Empty declaration must remain distinguishable from absent declaration");
    const std::string long_name(8193, 'x');
    package.document = read_xml_metadata("<Civ5Package><GameData>" + long_name + "</GameData></Civ5Package>");
    check(values(query) == std::vector<std::string>{long_name},
          "Resource text inherited a fixed native buffer limit");
}
void steam_tests(const fs::path &root) {
    const auto library = root / "library";
    fs::create_directories(library / "steamapps/common/CivFixture/Assets");
    write(library / "steamapps/common/CivFixture/CivilizationV.exe", "fixture");
    const auto manifest = library / "steamapps/appmanifest_8930.acf";
    write(manifest, "\"AppState\" { \"appid\" \"8930\" \"installdir\" \"CivFixture\" \"buildid\" \"123\" }");
    const auto vdf = root / "steam/steamapps/libraryfolders.vdf";
    const auto library_text = vdf_escape(assets::path_utf8(fs::absolute(library)));
    write(vdf, "// fixture\n\"libraryfolders\" { \"0\" { \"path\" \"" + library_text +
                   "\" \"apps\" { \"8930\" \"100\" } } }");
    const auto result = platform::discover_civ5(root / "steam");
    check(result.installations.size() == 1 && result.diagnostics.empty(),
          "Steam installation discovery failed");
    check(result.installations[0].build_id == "123", "Steam build metadata lost");
    write(manifest, "\"AppState\" { \"appid\" \"8930\" \"installdir\" \"../outside\" \"buildid\" \"123\" }");
    check(platform::discover_civ5(root / "steam").installations.empty(),
          "Unsafe Steam installation accepted");
    write(vdf, "\"libraryfolders\" { \"0\" \"a\" \"0\" \"b\" }");
    check(!platform::discover_civ5(root / "steam").diagnostics.empty(), "Duplicate VDF key accepted");
    write(vdf, "\"libraryfolders\" {");
    check(!platform::discover_civ5(root / "steam").diagnostics.empty(), "Truncated VDF accepted");
}
} // namespace
int main() {
    try {
        const auto root = fs::temp_directory_path() /
                          ("civis-runtime-fixture-" +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        binary_tests();
        frame_tests();
        inflate_tests();
        save_payload_tests(root);
        civis_save_tests(root);
        session_context_tests();
        pregame_tests();
        vp_prefix_tests();
        expansion2_prefix_tests();
        game_state_tests();
        game_progression_tests();
        session_dimensions_tests();
        simulation_random_tests();
        game_history_tests();
        game_leagues_tests();
        game_religion_culture_tests();
        game_trade_tests();
        game_tail_tests();
        civis_saved_database_tests(root);
        map_state_tests();
        map_regions_tests();
        team_state_tests();
        player_core_tests();
        player_strategy_tests();
        player_diplomacy_tests();
        player_knowledge_tests();
        player_planning_tests();
        player_traits_tests();
        player_espionage_tests();
        player_trade_culture_tests();
        player_administration_tests();
        city_core_tests();
        city_production_tests();
        city_management_tests();
        city_tail_tests();
        cities_tests();
        unit_core_tests();
        units_tests();
        army_operations_tests();
        all_players_tests();
        supporting_classes_tests();
        player_tail_tests();
        content_tests(root);
        mod_action_order_tests();
        mod_sql_action_result_tests(root);
        package_resource_tests();
        steam_tests(root);
        // Keep small fixtures for failed-build diagnosis; no recursive deletion.
        std::cout << "PASS " << checks << " checks; runtime_bits=" << sizeof(void *) * 8 << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
