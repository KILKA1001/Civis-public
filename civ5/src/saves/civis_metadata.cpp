#include "civ5/saves/civis_save.hpp"
#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/saves/game_tail.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace civ5::saves {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void validate(const CivisSaveMetadata &metadata) {
    require(metadata.source_sha256.size() == 64 &&
                std::all_of(metadata.source_sha256.begin(), metadata.source_sha256.end(), [](char c) {
                    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                }), "Invalid source SHA-256 in Civis metadata");
    std::set<std::string> ids;
    for (const auto &dlc : metadata.header.dlcs) {
        require(dlc.id == windows_guid(dlc.wire_id), "DLC GUID and identity disagree in Civis metadata");
        require(ids.insert(dlc.id).second, "Duplicate DLC identity in Civis metadata");
    }
    ids.clear();
    for (const auto &mod : metadata.header.mods)
        require(!mod.id.empty() && ids.insert(runtime::ascii_lower(mod.id)).second,
                "Empty or duplicate mod identity in Civis metadata");
}
class Encoder {
  public:
    explicit Encoder(HeaderBudget budget) : budget_(budget) {}
    std::vector<std::uint8_t> bytes;
    void integer(std::uint64_t value, unsigned width) {
        require(bytes.size() <= budget_.total_bytes && width <= budget_.total_bytes - bytes.size(),
                "Civis metadata exceeds byte budget");
        for (unsigned i = 0; i < width; ++i)
            bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    }
    void string(const std::string &value) {
        require(value.size() <= budget_.string_bytes && runtime::valid_utf8(value) &&
                    value.find('\0') == std::string::npos, "Invalid Civis metadata string");
        integer(value.size(), 8);
        require(bytes.size() <= budget_.total_bytes && value.size() <= budget_.total_bytes - bytes.size(),
                "Civis metadata exceeds byte budget");
        bytes.insert(bytes.end(), value.begin(), value.end());
    }
  private:
    HeaderBudget budget_;
};
class Decoder {
  public:
    Decoder(std::span<const std::uint8_t> bytes, HeaderBudget budget) : bytes_(bytes), budget_(budget) {
        require(bytes.size() <= budget.total_bytes, "Civis metadata exceeds byte budget");
    }
    std::uint64_t integer(unsigned width) {
        require(width <= bytes_.size(), "Truncated Civis metadata");
        std::uint64_t result = 0;
        for (unsigned i = 0; i < width; ++i)
            result |= std::uint64_t(bytes_[i]) << (8 * i);
        bytes_ = bytes_.subspan(width);
        return result;
    }
    std::string string() {
        const auto count = integer(8);
        require(count <= budget_.string_bytes && count <= bytes_.size(), "Invalid Civis metadata string length");
        std::string result(bytes_.begin(), bytes_.begin() + static_cast<std::size_t>(count));
        bytes_ = bytes_.subspan(static_cast<std::size_t>(count));
        require(runtime::valid_utf8(result) && result.find('\0') == std::string::npos,
                "Invalid Civis metadata string");
        return result;
    }
    std::uint64_t count(std::uint64_t minimum_record_bytes) {
        const auto result = integer(8);
        require(result <= bytes_.size() / minimum_record_bytes, "Invalid Civis metadata record count");
        return result;
    }
    bool empty() const { return bytes_.empty(); }
  private:
    std::span<const std::uint8_t> bytes_;
    HeaderBudget budget_;
};
} // namespace
void require_supported_standalone_services(const StandaloneServicesConfig &config) {
    if(config.schema != 1 ||
       config.steam_achievements != StandaloneServiceBackend::backend_disabled ||
       config.culture_diagnostics != StandaloneServiceBackend::backend_disabled)
        throw std::invalid_argument("Unsupported standalone services configuration");
}
CivisSaveSection make_standalone_services_section(const StandaloneServicesConfig &config) {
    require_supported_standalone_services(config);
    Encoder out(HeaderBudget{});
    out.integer(config.schema,4);
    out.integer(static_cast<std::uint32_t>(config.steam_achievements),4);
    out.integer(static_cast<std::uint32_t>(config.culture_diagnostics),4);
    return {static_cast<std::uint32_t>(CivisSaveSectionId::standalone_services),0,std::move(out.bytes)};
}
StandaloneServicesConfig read_standalone_services_section(const CivisSaveSection &section) {
    require(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::standalone_services) &&
            section.flags==0 && section.bytes.size()==12,"Unsupported standalone services section");
    Decoder in(section.bytes,HeaderBudget{});
    StandaloneServicesConfig result;
    result.schema=static_cast<std::uint32_t>(in.integer(4));
    result.steam_achievements=static_cast<StandaloneServiceBackend>(in.integer(4));
    result.culture_diagnostics=static_cast<StandaloneServiceBackend>(in.integer(4));
    require(in.empty(),"Trailing standalone services data");
    require_supported_standalone_services(result);
    return result;
}
CivisSaveSection make_save_metadata_section(const CivisSaveMetadata &metadata, HeaderBudget budget) {
    validate(metadata);
    Encoder out(budget);
    out.integer(1, 4); // Metadata schema, independent of the container version.
    out.string(metadata.source_sha256);
    const auto &h = metadata.header;
    out.integer(h.revision, 4);
    out.string(h.build);
    out.string(h.build_id);
    out.integer(h.turn, 8);
    out.integer(h.end_offset, 8); // Original header offset, never a Civis file offset.
    out.integer(h.mode, 1);
    for (const auto *s : {&h.civilization, &h.handicap, &h.starting_era, &h.current_era, &h.speed,
                          &h.world_size, &h.map_source})
        out.string(*s);
    out.integer(h.dlcs.size(), 8);
    for (const auto &dlc : h.dlcs) {
        for (const auto byte : dlc.wire_id)
            out.integer(byte, 1);
        out.string(dlc.id);
        out.integer(dlc.wire_value, 4); // Retain the imported wire field exactly.
        out.string(dlc.name);
    }
    out.integer(h.mods.size(), 8);
    for (const auto &mod : h.mods) {
        out.string(mod.id);
        out.integer(mod.wire_value, 4);
        out.string(mod.name);
    }
    return {static_cast<std::uint32_t>(CivisSaveSectionId::source_metadata), 0, std::move(out.bytes)};
}
CivisSaveMetadata read_save_metadata_section(const CivisSaveSection &section, HeaderBudget budget) {
    require(section.id == static_cast<std::uint32_t>(CivisSaveSectionId::source_metadata) && section.flags == 0,
            "Unsupported Civis metadata section");
    Decoder in(section.bytes, budget);
    require(in.integer(4) == 1, "Unsupported Civis metadata schema");
    CivisSaveMetadata result;
    result.source_sha256 = in.string();
    auto &h = result.header;
    h.revision = static_cast<std::uint32_t>(in.integer(4));
    h.build = in.string();
    h.build_id = in.string();
    h.turn = in.integer(8);
    h.end_offset = in.integer(8);
    h.mode = static_cast<std::uint8_t>(in.integer(1));
    for (auto *s : {&h.civilization, &h.handicap, &h.starting_era, &h.current_era, &h.speed,
                    &h.world_size, &h.map_source})
        *s = in.string();
    const auto dlcs = in.count(36);
    for (std::uint64_t i = 0; i < dlcs; ++i) {
        DlcRequirement dlc;
        for (auto &byte : dlc.wire_id)
            byte = static_cast<std::uint8_t>(in.integer(1));
        dlc.id = in.string();
        dlc.wire_value = static_cast<std::uint32_t>(in.integer(4));
        dlc.name = in.string();
        h.dlcs.push_back(std::move(dlc));
    }
    const auto mods = in.count(20);
    for (std::uint64_t i = 0; i < mods; ++i) {
        ModRequirement mod;
        mod.id = in.string();
        mod.wire_value = static_cast<std::uint32_t>(in.integer(4));
        mod.name = in.string();
        h.mods.push_back(std::move(mod));
    }
    require(in.empty(), "Trailing bytes in Civis metadata");
    validate(result);
    return result;
}
namespace {
// Both entry points run this one parser. Validation borrows the document and
// skips ownership transfers; it does not copy the payload or live sections.
template<bool Materialize, class Document>
CivisImportedSave parse_imported_civis_document(Document &document,
                                               HeaderBudget metadata_budget) {
    require(document.flags == 1 && document.sections.size() >= 2,
            "Expected Civis import sections with optional prefix, database and live state");
    const auto metadata = std::find_if(document.sections.begin(), document.sections.end(), [](const auto &s) {
        return s.id == static_cast<std::uint32_t>(CivisSaveSectionId::source_metadata);
    });
    const auto payload = std::find_if(document.sections.begin(), document.sections.end(), [](const auto &s) {
        return s.id == static_cast<std::uint32_t>(CivisSaveSectionId::imported_payload);
    });
    require(metadata != document.sections.end() && payload != document.sections.end() && payload->flags == 0,
            "Missing or unsupported Civis import sections");
    require(!payload->bytes.empty(), "Empty Civis imported payload");
    // Decode and reject unknown required service values before retaining live overlays.
    const auto services=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::standalone_services);
    });
    std::optional<StandaloneServicesConfig> standalone_services;
    if(services!=document.sections.end()) standalone_services=read_standalone_services_section(*services);
    auto decoded = read_save_metadata_section(*metadata, metadata_budget);
    const auto prefix = std::find_if(document.sections.begin(), document.sections.end(), [](const auto &s) {
        return s.id == static_cast<std::uint32_t>(CivisSaveSectionId::source_prefix);
    });
    const auto database=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::saved_database);
    });
    const auto production=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_city_production);
    });
    const auto research=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_team_research);
    });
    const auto vp_research=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::vp_research_profile);
    });
    const auto treasury=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_treasury);
    });
    const auto research_turn=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_research_turn);
    });
    const auto research_selection=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_research_selection);
    });
    const auto random=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_simulation_random);
    });
    const auto game_clock=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_clock);
    });
    const auto map_plots=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_map_plots);
    });
    const auto units=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_units);
    });
    const auto area_units=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_area_units);
    });
    const auto player_proximity=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_proximity);
    });
    const auto team_meet=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_meet);
    });
    const auto exploration_dirty=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_exploration_dirty);
    });
    const auto player_strategy=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_strategy);
    });
    const auto player_core=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_core);
    });
    const auto city_core=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_core);
    });
    const auto city_production=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_production);
    });
    const auto city_management=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_management);
    });
    const auto city_tail=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_tail);
    });
    const auto treasury_balance=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_treasury_balance);
    });
    const auto team_progress=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_progress);
    });
    const auto culture_bomb_timer=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_culture_bomb_timer);
    });
    const auto research_queue=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_research_queue);
    });
    const auto maintenance_expense=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_maintenance_expense);
    });
    const auto player_knowledge=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_knowledge);
    });
    const auto player_administration=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_administration);
    });
    const auto game_supporting=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_supporting);
    });
    const auto game_score_replay=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_score_replay);
    });
    const auto game_deals=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_deals);
    });
    const auto team_turn=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_turn);
    });
    const auto ui_transients=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_ui_transients);
    });
    const auto mission_continuation=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_mission_continuation);
    });
    const auto food_growth=std::find_if(document.sections.begin(),document.sections.end(),[](const auto&s){return s.id==47;});
    const auto population_totals=std::find_if(document.sections.begin(),document.sections.end(),[](const auto&s){return s.id==46;});
    const auto unit_serial=std::find_if(document.sections.begin(),document.sections.end(),[](const auto&s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_serial);
    });
    const auto military_requests=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_military_requests);
    });
    const auto science_profile=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::vp_science_profile);
    });
    const auto espionage_profile=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &s){
        return s.id==static_cast<std::uint32_t>(CivisSaveSectionId::vp_espionage_profile);
    });
    require(player_administration==document.sections.end() ||
                (research==document.sections.end() &&
                 treasury==document.sections.end() &&
                 research_turn==document.sections.end() &&
                 research_selection==document.sections.end() &&
                 treasury_balance==document.sections.end() &&
                 research_queue==document.sections.end() &&
                 maintenance_expense==document.sections.end()),
            "BNW full player administration overlaps research, treasury or maintenance sections");
    require(document.sections.size() == 2+(prefix!=document.sections.end())+(database!=document.sections.end())+
                (production!=document.sections.end())+(research!=document.sections.end())+
                (vp_research!=document.sections.end())+(treasury!=document.sections.end())+
                (research_turn!=document.sections.end())+(science_profile!=document.sections.end())+
                (research_selection!=document.sections.end())+
                (espionage_profile!=document.sections.end())+
                (random!=document.sections.end())+(game_clock!=document.sections.end())+
                (map_plots!=document.sections.end())+(units!=document.sections.end())+
                (area_units!=document.sections.end())+
                (player_proximity!=document.sections.end())+
                (team_meet!=document.sections.end())+
                (exploration_dirty!=document.sections.end())+
                (player_strategy!=document.sections.end())+
                (player_core!=document.sections.end())+
                (city_core!=document.sections.end())+
                (city_production!=document.sections.end())+
                (city_management!=document.sections.end())+
                (city_tail!=document.sections.end())+
                (treasury_balance!=document.sections.end())+
                (team_progress!=document.sections.end())+
                (culture_bomb_timer!=document.sections.end())+
                (research_queue!=document.sections.end())+
                (maintenance_expense!=document.sections.end())+
                (player_knowledge!=document.sections.end())+
                (player_administration!=document.sections.end())+
                (game_supporting!=document.sections.end())+
                (game_score_replay!=document.sections.end())+
                (game_deals!=document.sections.end())+
                (team_turn!=document.sections.end())+(ui_transients!=document.sections.end())+
                (services!=document.sections.end())+
                (mission_continuation!=document.sections.end())+
                (military_requests!=document.sections.end())+(unit_serial!=document.sections.end())+
                (population_totals!=document.sections.end())+(food_growth!=document.sections.end()) &&
                (military_requests==document.sections.end()||player_strategy!=document.sections.end()) &&
                (treasury_balance==document.sections.end()||treasury!=document.sections.end()) &&
                (team_progress==document.sections.end()||research!=document.sections.end()) &&
                (team_turn==document.sections.end()||research!=document.sections.end()) &&
                (research_queue==document.sections.end()||
                    (research!=document.sections.end()&&
                     research_turn!=document.sections.end()&&
                     team_progress==document.sections.end()&&
                     research_selection==document.sections.end())) &&
                (player_knowledge==document.sections.end()||
                    research_queue==document.sections.end()) &&
                (units==document.sections.end()||map_plots!=document.sections.end()) &&
                (area_units==document.sections.end()||
                    (map_plots!=document.sections.end()&&units!=document.sections.end())) &&
                (vp_research==document.sections.end()||research!=document.sections.end()),
            "Unsupported Civis import section; live session/rules sections cannot fall back to source payload");
    require(exploration_dirty==document.sections.end()||player_strategy==document.sections.end(),
            "BNW exploration dirty and full player strategy sections cannot coexist");
    require(team_turn==document.sections.end()||team_progress==document.sections.end(),
            "BNW team turn and raw research progress need a combined checkpoint coordinator");
    require(team_turn==document.sections.end()||research_queue==document.sections.end(),
            "BNW team turn and research queue need a combined checkpoint coordinator");
    require(player_proximity==document.sections.end()||player_core==document.sections.end(),
            "BNW player proximity and full player core sections cannot coexist");
    if(population_totals!=document.sections.end())runtime::validate_bnw_population_totals_checkpoint_companions(document);
    if(food_growth!=document.sections.end())runtime::validate_bnw_food_growth_journal_companions(document);
    std::optional<std::vector<std::uint8_t>> retained_prefix;
    if (prefix != document.sections.end()) {
        require(prefix->flags == 0, "Unsupported original prefix section flags");
        validate_source_prefix(prefix->bytes, decoded.header, metadata_budget);
        if constexpr(Materialize) retained_prefix = std::move(prefix->bytes);
    }
    std::optional<std::vector<std::uint8_t>> saved_database;
    if(database!=document.sections.end()){
        require(database->flags==0,"Unsupported saved database section flags");
        validate_saved_database_image(database->bytes);
        if constexpr(Materialize) saved_database=std::move(database->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_production;
    if(production!=document.sections.end()){
        require(production->flags==0&&!production->bytes.empty(),"Unsupported live production section");
        if constexpr(Materialize) live_production=std::move(production->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_research;
    ProfileSaveSections profile_sections;
    if(research!=document.sections.end()){
        require(research->flags==0&&!research->bytes.empty(),"Unsupported live research section");
        if constexpr(Materialize) live_research=std::move(research->bytes);
    }
    if(vp_research!=document.sections.end()){
        require(vp_research->flags==0&&!vp_research->bytes.empty(),"Unsupported VP research section");
        if constexpr(Materialize) profile_sections.emplace(vp_research->id,std::move(vp_research->bytes));
    }
    std::optional<std::vector<std::uint8_t>> live_treasury;
    if(treasury!=document.sections.end()){
        require(treasury->flags==0&&!treasury->bytes.empty(),"Unsupported live treasury section");
        if constexpr(Materialize) live_treasury=std::move(treasury->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_research_turn;
    if(research_turn!=document.sections.end()){
        require(research_turn->flags==0&&!research_turn->bytes.empty(),"Unsupported live research-turn section");
        if constexpr(Materialize) live_research_turn=std::move(research_turn->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_research_selection;
    if(research_selection!=document.sections.end()){
        require(research_selection->flags==0&&!research_selection->bytes.empty(),
                "Unsupported live research-selection section");
        if constexpr(Materialize) live_research_selection=std::move(research_selection->bytes);
    }
    if(science_profile!=document.sections.end()){
        require(science_profile->flags==0&&!science_profile->bytes.empty(),"Unsupported VP science section");
        if constexpr(Materialize) profile_sections.emplace(science_profile->id,std::move(science_profile->bytes));
    }
    if(espionage_profile!=document.sections.end()){
        require(espionage_profile->flags==0&&!espionage_profile->bytes.empty(),
            "Unsupported VP espionage section");
        if constexpr(Materialize) profile_sections.emplace(espionage_profile->id,std::move(espionage_profile->bytes));
    }
    std::optional<std::vector<std::uint8_t>> live_random;
    if(random!=document.sections.end()){
        require(random->flags==0&&!random->bytes.empty(),"Unsupported live simulation RNG section");
        if constexpr(Materialize) live_random=std::move(random->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_game_clock;
    if(game_clock!=document.sections.end()){
        require(game_clock->flags==0&&!game_clock->bytes.empty(),"Unsupported live BNW game clock section");
        if constexpr(Materialize) live_game_clock=std::move(game_clock->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_map_plots;
    if(map_plots!=document.sections.end()){
        require(map_plots->flags==0&&!map_plots->bytes.empty(),"Unsupported live BNW map plots section");
        if constexpr(Materialize) live_map_plots=std::move(map_plots->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_units;
    if(units!=document.sections.end()){
        require(units->flags==0&&!units->bytes.empty(),"Unsupported live BNW units section");
        if constexpr(Materialize) live_units=std::move(units->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_area_units;
    if(area_units!=document.sections.end()){
        require(area_units->flags==0&&!area_units->bytes.empty(),
                "Unsupported live BNW area units section");
        if constexpr(Materialize) live_area_units=std::move(area_units->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_player_proximity;
    if(player_proximity!=document.sections.end()){
        require(player_proximity->flags==0&&!player_proximity->bytes.empty(),
                "Unsupported live BNW player proximity section");
        if constexpr(Materialize) live_player_proximity=std::move(player_proximity->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_team_meet;
    if(team_meet!=document.sections.end()){
        require(team_meet->flags==0&&!team_meet->bytes.empty(),
                "Unsupported live BNW team meet section");
        if constexpr(Materialize) live_team_meet=std::move(team_meet->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_exploration_dirty;
    if(exploration_dirty!=document.sections.end()){
        require(exploration_dirty->flags==0&&!exploration_dirty->bytes.empty(),
                "Unsupported live BNW exploration dirty section");
        if constexpr(Materialize) live_exploration_dirty=std::move(exploration_dirty->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_player_strategy;
    if(player_strategy!=document.sections.end()){
        require(player_strategy->flags==0&&!player_strategy->bytes.empty(),
                "Unsupported live BNW player strategy section");
        if constexpr(Materialize) live_player_strategy=std::move(player_strategy->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_player_core;
    if(player_core!=document.sections.end()){
        require(player_core->flags==0&&!player_core->bytes.empty(),
                "Unsupported live BNW player core section");
        if constexpr(Materialize) live_player_core=std::move(player_core->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_city_core;
    if(city_core!=document.sections.end()){
        require(city_core->flags==0&&!city_core->bytes.empty(),
                "Unsupported live BNW city core section");
        if constexpr(Materialize) live_city_core=std::move(city_core->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_city_production;
    if(city_production!=document.sections.end()){
        require(city_production->flags==0&&!city_production->bytes.empty(),
                "Unsupported live BNW city production section");
        if constexpr(Materialize) live_city_production=std::move(city_production->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_city_management;
    if(city_management!=document.sections.end()){
        require(city_management->flags==0&&!city_management->bytes.empty(),
                "Unsupported live BNW city management section");
        if constexpr(Materialize) live_city_management=std::move(city_management->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_city_tail;
    if(city_tail!=document.sections.end()){
        require(city_tail->flags==0&&!city_tail->bytes.empty(),
                "Unsupported live BNW city tail section");
        if constexpr(Materialize) live_city_tail=std::move(city_tail->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_treasury_balance;
    if(treasury_balance!=document.sections.end()){
        require(treasury_balance->flags==0&&!treasury_balance->bytes.empty(),
                "Unsupported live BNW treasury balance section");
        if constexpr(Materialize) live_treasury_balance=std::move(treasury_balance->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_team_progress;
    if(team_progress!=document.sections.end()){
        require(team_progress->flags==0&&!team_progress->bytes.empty(),
                "Unsupported live BNW team progress section");
        if constexpr(Materialize) live_team_progress=std::move(team_progress->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_culture_bomb_timer;
    if(culture_bomb_timer!=document.sections.end()){
        require(culture_bomb_timer->flags==0&&!culture_bomb_timer->bytes.empty(),
                "Unsupported live BNW culture bomb timer section");
        if constexpr(Materialize) live_culture_bomb_timer=std::move(culture_bomb_timer->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_research_queue;
    if(research_queue!=document.sections.end()){
        require(research_queue->flags==0&&!research_queue->bytes.empty(),
                "Unsupported live BNW research queue section");
        if constexpr(Materialize) live_research_queue=std::move(research_queue->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_maintenance_expense;
    if(maintenance_expense!=document.sections.end()){
        require(maintenance_expense->flags==0&&!maintenance_expense->bytes.empty(),
                "Unsupported live BNW maintenance expense section");
        if constexpr(Materialize) live_maintenance_expense=std::move(maintenance_expense->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_player_knowledge;
    if(player_knowledge!=document.sections.end()){
        require(player_knowledge->flags==0&&!player_knowledge->bytes.empty(),
                "Unsupported live BNW player knowledge section");
        if constexpr(Materialize) live_player_knowledge=std::move(player_knowledge->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_player_administration;
    if(player_administration!=document.sections.end()){
        require(player_administration->flags==0&&!player_administration->bytes.empty(),
                "Unsupported live BNW player administration section");
        if constexpr(Materialize) live_player_administration=std::move(player_administration->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_game_supporting;
    if(game_supporting!=document.sections.end()){
        require(game_supporting->flags==0&&!game_supporting->bytes.empty(),
                "Unsupported live BNW game supporting section");
        if constexpr(Materialize) live_game_supporting=std::move(game_supporting->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_game_score_replay;
    if(game_score_replay!=document.sections.end()){
        require(game_score_replay->flags==0&&!game_score_replay->bytes.empty(),
                "Unsupported live BNW game score and replay section");
        if constexpr(Materialize) live_game_score_replay=std::move(game_score_replay->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_game_deals;
    if(game_deals!=document.sections.end()){
        require(game_deals->flags==0&&!game_deals->bytes.empty(),
                "Unsupported live BNW game deals section");
        if constexpr(Materialize) live_game_deals=std::move(game_deals->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_team_turn;
    if(team_turn!=document.sections.end()){
        require(team_turn->flags==0&&!team_turn->bytes.empty(),
                "Unsupported live BNW team turn section");
        if constexpr(Materialize) live_team_turn=std::move(team_turn->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_ui_transients;
    if(ui_transients!=document.sections.end()){
        require(ui_transients->flags==0&&!ui_transients->bytes.empty(),
                "Unsupported live BNW UI transients section");
        if constexpr(Materialize) live_ui_transients=std::move(ui_transients->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_mission_continuation;
    if(mission_continuation!=document.sections.end()) {
        require(mission_continuation->flags==0&&!mission_continuation->bytes.empty(),
                "Unsupported live BNW unit mission continuation section");
        // The shared codec validates the whole canonical record accepted by the genuine Unit codec,
        // including known phases. No second container-side wire parser exists.
        runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(*mission_continuation);
        if constexpr(Materialize) live_mission_continuation=std::move(mission_continuation->bytes);
    }
    // ID45 currently supports serial-only unit domain. Mixed ID18/19/43
    // requires a closed ordered companion exporter and cannot guess a baseline.
    require(unit_serial==document.sections.end()||
        (units==document.sections.end()&&area_units==document.sections.end()&&
         mission_continuation==document.sections.end()),
        "Unit serial checkpoint requires a combined unit/history coordinator");
    std::optional<std::vector<std::uint8_t>> live_unit_serial;
    if(unit_serial!=document.sections.end()){
        runtime::validate_bnw_unit_serial_checkpoint_shape(*unit_serial);
        if constexpr(Materialize) live_unit_serial=std::move(unit_serial->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_military_requests;
    if(military_requests!=document.sections.end()){
        runtime::validate_bnw_military_request_checkpoint_shape(*military_requests);
        if constexpr(Materialize) live_military_requests=std::move(military_requests->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_population_totals;
    if(population_totals!=document.sections.end()){

        if constexpr(Materialize) live_population_totals=std::move(population_totals->bytes);
    }
    std::optional<std::vector<std::uint8_t>> live_food_growth;
    if(food_growth!=document.sections.end()){

        if constexpr(Materialize) live_food_growth=std::move(food_growth->bytes);
    }
    std::vector<std::uint8_t> retained_payload;
    if constexpr(Materialize) retained_payload=std::move(payload->bytes);
    return {std::move(decoded), std::move(retained_payload), std::move(retained_prefix),
            std::move(saved_database),std::move(live_production),
            std::move(live_research),std::move(live_treasury),
            std::move(live_research_turn),std::move(live_research_selection),
            std::move(live_random),std::move(live_game_clock),std::move(live_map_plots),
            std::move(live_units),
            std::move(live_area_units),
            std::move(live_player_proximity),
            std::move(live_team_meet),std::move(live_exploration_dirty),
            std::move(live_player_strategy),std::move(live_player_core),
            std::move(live_city_core),std::move(live_city_production),
            std::move(live_city_management),std::move(live_city_tail),
            std::move(live_treasury_balance),
            std::move(live_team_progress),
            std::move(live_culture_bomb_timer),
            std::move(live_research_queue),
            std::move(live_maintenance_expense),
            std::move(live_player_knowledge),
            std::move(live_player_administration),
            std::move(live_game_supporting),
            std::move(live_game_score_replay),
            std::move(live_game_deals),
            std::move(live_team_turn),
            std::move(live_ui_transients),
            std::move(profile_sections),std::move(standalone_services),
            std::move(live_mission_continuation),std::move(live_military_requests),std::move(live_unit_serial),std::move(live_population_totals),std::move(live_food_growth)};
}
} // namespace
void validate_civis_import_document(const CivisSaveDocument &document,
                                   HeaderBudget metadata_budget) {
    (void)parse_imported_civis_document<false>(document,metadata_budget);
}
CivisImportedSave read_imported_civis_save(const std::filesystem::path &path,
                                         CivisSaveBudget budget, HeaderBudget metadata_budget) {
    auto document=read_civis_save(path,budget);
    return parse_imported_civis_document<true>(document,metadata_budget);
}
} // namespace civ5::saves
