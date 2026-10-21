#include "civ5/runtime/bnw_culture_bomb_timer_checkpoint.hpp"
#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {std::error_code error;std::filesystem::remove(path,error);}
};
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and installed BNW root");
        const std::filesystem::path save(argv[1]),root(argv[2]);
        const auto original_hash=civ5::runtime::identify_file(save).sha256;
        auto original=civ5::ui::bootstrap_selected_bnw_preview(save,root,0);
        if(original.preview.blocker)
            throw std::runtime_error(original.preview.blocker->reason);
        check(original.preview.host!=nullptr,"Selected BNW live host unavailable");
        auto &host=*original.preview.host;
        const auto player=host.active_player_id();
        auto timers=host.session().state().player_culture_bomb_timers;
        check(timers.size()==64 && timers.at(player)==0,
              "BNW CvPlayer::Read timer baseline unavailable");
        timers.at(player)=5;
        const auto &source=host.session().source();
        const auto checkpoint=civ5::runtime::make_bnw_culture_bomb_timer_checkpoint(
            timers,source.snapshot,host.identity());
        const auto suffix=std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        Cleanup cleanup{std::filesystem::current_path()/
            ("bnw-culture-bomb-live-import-"+suffix+".CivisSave")};
        auto document=civ5::saves::make_civis_import_document(
            source.source,original_hash);
        document.sections.push_back(checkpoint);
        civ5::saves::write_civis_save(cleanup.path,document);
        auto resumed=civ5::ui::bootstrap_selected_bnw_preview(
            cleanup.path,root,0);
        if(resumed.preview.blocker)
            throw std::runtime_error(resumed.preview.blocker->reason);
        check(resumed.preview.host!=nullptr,"BNW timer import host unavailable");
        const auto &session=resumed.preview.host->session();
        check(session.state().player_culture_bomb_timers.at(player)==5 &&
              host.session().state().player_culture_bomb_timers.at(player)==0 &&
              !session.readiness().can_continue(),
              "BNW timer checkpoint lost live value or readiness guard");
        auto draft=session.state();
        const auto input=resumed.preview.host->preview_input();
        check(input.frame_sources.database!=nullptr,
              "Selected BNW rules unavailable after timer import");
        const auto step=civ5::runtime::apply_bnw_human_player_early_turn(
            draft,player,player,*input.frame_sources.database,
            resumed.preview.host->identity());
        check(step.initial.scalars.culture_bomb.before==5 &&
              step.initial.scalars.culture_bomb.after==4 &&
              draft.player_culture_bomb_timers.at(player)==4 &&
              civ5::runtime::identify_file(save).sha256==original_hash,
              "BNW imported timer did not feed the next early player segment");
        std::cout << "BNW culture bomb timer live import PASS\n";
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
