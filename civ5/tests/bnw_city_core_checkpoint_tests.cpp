#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW city core checkpoint was accepted");
}
saves::CityCoreField &field(saves::SavedCityObservation &city,
                            std::string_view wanted) {
    for(auto &part:city.core.fields)
        if(part.name && std::string_view(part.name)==wanted) return part;
    throw std::runtime_error("Selected BNW city core field missing");
}
struct Selection {std::size_t player=0,city=0;};
Selection choose(const saves::SavedSessionSnapshot &source) {
    for(std::size_t p=0;p<source.all_players->players.size();++p)
        if(!source.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.all_players &&
          imported.snapshot.all_players->players.size()==64,
          "Selected Gandhi save has no complete city roster");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto source=std::move(imported.snapshot);
    const auto chosen=choose(source);
    saves::CivisSaveSection section;
    std::int64_t food_after=0,population_after=0;
    {
        auto world=source;
        auto &city=world.all_players->players[chosen.player].cities.cities[chosen.city];
        auto &food=field(city,"m_iFood").values.at(0);
        auto &population=field(city,"m_iPopulation").values.at(0);
        ++food;++population;food_after=food;population_after=population;
        section=runtime::make_bnw_city_core_baseline_checkpoint(world,source,identity);
        check(section.id==runtime::bnw_city_core_checkpoint_section_id &&
              section.bytes.at(8)==3,
              "BNW city core section ID/version changed");
        auto wrong=world;
        field(wrong.all_players->players[chosen.player].cities.cities[chosen.city],
              "m_iFood").values.push_back(0);
        rejects([&]{(void)runtime::make_bnw_city_core_checkpoint(
            wrong,source,identity);});
        wrong=world;
        check(!wrong.all_players->players[chosen.player].cities.cities[chosen.city]
                   .management.blocks.empty(),
              "Selected BNW city has no management block for companion guard");
        wrong.all_players->players[chosen.player].cities.cities[chosen.city]
            .management.blocks.pop_back();
        rejects([&]{(void)runtime::make_bnw_city_core_checkpoint(
            wrong,source,identity);});
        wrong=world;
        field(wrong.all_players->players[chosen.player].cities.cities[chosen.city],
              "m_eOwner").values[0]=42;
        rejects([&]{(void)runtime::make_bnw_city_core_checkpoint(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-city-core-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(section);
        saves::write_civis_save(output,document);
    }
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        const auto reopened=saves::read_civis_save(output);
        const saves::CivisSaveSection *read_section=nullptr;
        for(const auto &item:reopened.sections)
            if(item.id==section.id) read_section=&item;
        check(read_section && read_section->bytes==section.bytes,
              "Atomic CivisSave read/write lost BNW city core section");
        auto restored=source;
        runtime::restore_bnw_city_core_checkpoint(
            *read_section,restored,source,identity);
        auto &city=restored.all_players->players[chosen.player].cities.cities[chosen.city];
        check(field(city,"m_iFood").values.at(0)==food_after &&
              field(city,"m_iPopulation").values.at(0)==population_after,
              "BNW city core food/population did not round-trip");
    }
    // The ordinary container/session decoder preserves the strict section;
    // overlay admission stays explicit and does not claim a playable session.
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_city_core_overlay=true;
    const auto normal=saves::read_saved_session(output,context);
    check(normal.source.live_bnw_city_core==section.bytes,"Normal reader lost strict city core section");
    auto normal_world=normal.snapshot;
    runtime::restore_bnw_city_core_checkpoint(section,normal_world,normal.snapshot,identity);
    check(field(normal_world.all_players->players[chosen.player].cities.cities[chosen.city],"m_iFood").values.at(0)==food_after,
          "Normal reader / strict city core restore lost food mutation");

    // Changing the public source DTO and destination together must not defeat
    // the actual old-value binding recorded when this checkpoint was made.
    auto changed_source=source;
    auto &wrong_city=changed_source.all_players->players[chosen.player].cities.cities[chosen.city];
    auto &wrong_food=field(wrong_city,"m_iFood").values.at(0);wrong_food+=7;
    auto wrong_destination=changed_source;
    rejects([&]{runtime::restore_bnw_city_core_checkpoint(section,wrong_destination,changed_source,identity);});
    check(field(wrong_destination.all_players->players[chosen.player].cities.cities[chosen.city],"m_iFood").values.at(0)==wrong_food,
          "Wrong baseline hash partially published city mutation");

    auto strict_unchanged=runtime::make_bnw_city_core_baseline_checkpoint(source,source,identity,true);
    check(strict_unchanged.bytes.at(8)==4,"Strict unchanged city checkpoint schema is not4");
    auto unchanged_world=source;
    runtime::restore_bnw_city_core_checkpoint(strict_unchanged,unchanged_world,source,identity);
    // Preserve actual schema1/2 read compatibility, without claiming the
    // stronger baseline digest for those older wire formats.
    constexpr std::size_t digest_offset=8+4+64*5;
    auto legacy_changed=section;legacy_changed.bytes.erase(legacy_changed.bytes.begin()+digest_offset,
        legacy_changed.bytes.begin()+digest_offset+64);legacy_changed.bytes[8]=1;
    auto legacy_world=source;runtime::restore_bnw_city_core_checkpoint(legacy_changed,legacy_world,source,identity);
    check(field(legacy_world.all_players->players[chosen.player].cities.cities[chosen.city],"m_iPopulation").values.at(0)==population_after,
          "Legacy city core schema1 no longer restores");
    auto legacy_unchanged=strict_unchanged;legacy_unchanged.bytes.erase(legacy_unchanged.bytes.begin()+digest_offset,
        legacy_unchanged.bytes.begin()+digest_offset+64);legacy_unchanged.bytes[8]=2;
    runtime::restore_bnw_city_core_checkpoint(legacy_unchanged,unchanged_world,source,identity);

    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW city core checkpoint changed original Civ5Save");
    auto destination=source;
    auto bad=section;bad.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_city_core_checkpoint(
        bad,destination,source,identity);});
    auto &unchanged=destination.all_players->players[chosen.player].cities.cities[chosen.city];
    auto &original_city=source.all_players->players[chosen.player].cities.cities[chosen.city];
    check(field(unchanged,"m_iFood").values==field(original_city,"m_iFood").values,
          "Truncated BNW city section partially changed destination");
    bad=section;bad.bytes[8]=4;
    rejects([&]{runtime::restore_bnw_city_core_checkpoint(
        bad,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_city_core_checkpoint(
        section,destination,source,foreign);});
    std::cout << "PASS BNW full CityCore checkpoint\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW city core checkpoint: " << error.what() << '\n';
        return 1;
    }
}
