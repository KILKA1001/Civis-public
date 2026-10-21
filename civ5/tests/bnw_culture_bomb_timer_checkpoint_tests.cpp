#include "civ5/runtime/bnw_culture_bomb_timer_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW culture bomb timer checkpoint accepted");
}
saves::SavedSessionDecodeContext context() {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    return result;
}
runtime::BnwCultureBombTimers reset_timers() {
    runtime::BnwCultureBombTimers timers;
    for(std::int64_t id=0;id<64;++id) timers.emplace(id,0);
    return timers;
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &document) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &item:document.sections)
        if(item.id==runtime::bnw_culture_bomb_timer_checkpoint_section_id) {
            check(!found,"Duplicate BNW culture bomb timer section");found=&item;
        }
    check(found,"Missing BNW culture bomb timer section");return *found;
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto timers=reset_timers();
    timers.at(0)=5;timers.at(63)=1;
    const auto first=runtime::make_bnw_culture_bomb_timer_checkpoint(
        timers,imported.snapshot,identity);
    check(first.id==runtime::bnw_culture_bomb_timer_checkpoint_section_id &&
          first.bytes.size()==8+4+64*3+4+64*12,
          "BNW culture bomb timer section shape changed");
    {
        auto bad=timers;
        bad.erase(63);
        rejects([&] {(void)runtime::make_bnw_culture_bomb_timer_checkpoint(
            bad,imported.snapshot,identity);});
        bad=timers;bad.at(0)=-1;
        rejects([&] {(void)runtime::make_bnw_culture_bomb_timer_checkpoint(
            bad,imported.snapshot,identity);});
        auto foreign=imported.snapshot;
        ++foreign.all_players->players[1].saved_player_id;
        rejects([&] {(void)runtime::make_bnw_culture_bomb_timer_checkpoint(
            timers,foreign,identity);});
        --foreign.all_players->players[1].saved_player_id;
        saves::PlayerCoreField spurious{};
        spurious.name="m_iCultureBombTimer";
        spurious.encoding=saves::PlayerCoreEncoding::i32;
        spurious.values={0};
        foreign.all_players->players[1].core.fields.push_back(spurious);
        rejects([&] {(void)runtime::make_bnw_culture_bomb_timer_checkpoint(
            timers,foreign,identity);});
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first_path=std::filesystem::current_path()/
        ("bnw-culture-bomb-"+suffix+"-1.CivisSave");
    const auto second_path=std::filesystem::current_path()/
        ("bnw-culture-bomb-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first_path,second_path};
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(first);
    saves::write_civis_save(first_path,document);
    const auto reopened=saves::read_civis_save(first_path);
    auto restored=reset_timers();
    runtime::restore_bnw_culture_bomb_timer_checkpoint(
        section(reopened),restored,imported.snapshot,identity);
    check(restored==timers && restored.at(0)==5 && restored.at(63)==1,
          "BNW culture bomb timer physical round-trip changed values");
    {
        auto damaged=first;damaged.bytes.pop_back();
        auto untouched=reset_timers();
        rejects([&] {runtime::restore_bnw_culture_bomb_timer_checkpoint(
            damaged,untouched,imported.snapshot,identity);});
        check(untouched==reset_timers(),
              "Truncated BNW culture bomb section partially published");
        auto foreign=identity;foreign.rules_sha256=std::string(64,'b');
        rejects([&] {runtime::restore_bnw_culture_bomb_timer_checkpoint(
            first,untouched,imported.snapshot,foreign);});
        check(untouched==reset_timers(),
              "Foreign BNW culture bomb identity partially published");
        untouched.at(0)=2;
        rejects([&] {runtime::restore_bnw_culture_bomb_timer_checkpoint(
            first,untouched,imported.snapshot,identity);});
        check(untouched.at(0)==2,
              "Changed culture bomb reset baseline partially overwritten");
        auto bad_id=first;
        // Header 8+4+3*64+4, first row is player ID.
        bad_id.bytes.at(208)=3;
        untouched=reset_timers();
        rejects([&] {runtime::restore_bnw_culture_bomb_timer_checkpoint(
            bad_id,untouched,imported.snapshot,identity);});
        check(untouched==reset_timers(),
              "Corrupt culture bomb player ID partially published");
    }
    restored.at(0)=4;
    const auto second=runtime::make_bnw_culture_bomb_timer_checkpoint(
        restored,imported.snapshot,identity);
    auto next=saves::make_civis_import_document(imported.source,original.sha256);
    next.sections.push_back(second);
    saves::write_civis_save(second_path,next);
    const auto reread=saves::read_civis_save(second_path);
    auto final=reset_timers();
    runtime::restore_bnw_culture_bomb_timer_checkpoint(
        section(reread),final,imported.snapshot,identity);
    check(final.at(0)==4 && final.at(63)==1 &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW culture bomb re-save or original Civ5Save changed");
    std::cout << "PASS BNW culture bomb timer checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW culture bomb timer checkpoint: "
                  << error.what() << '\n';return 1;
    }
}
