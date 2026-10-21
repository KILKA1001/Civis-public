#include "civ5/runtime/bnw_player_administration_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
template<class F> void rejects(F &&operation) {
    bool rejected=false;
    try {operation();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW player administration checkpoint accepted");
}
std::string_view name(const char *text) {
    return text?std::string_view(text):std::string_view();
}
saves::PlayerAdministrationRecord &block(saves::SavedPlayerObservation &player,
                                         std::string_view wanted) {
    for(auto &entry:player.administration.blocks)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW administration block unavailable");
}
std::int64_t &field(saves::PlayerAdministrationRecord &record,
                    std::string_view wanted) {
    for(auto &entry:record.fields)
        if(name(entry.name)==wanted) return entry.value;
    throw std::runtime_error("Selected BNW administration field unavailable");
}
bool same_record(const saves::PlayerAdministrationRecord &a,
                 const saves::PlayerAdministrationRecord &b) {
    if(name(a.name)!=name(b.name) || a.index!=b.index ||
       a.start_offset!=b.start_offset || a.end_offset!=b.end_offset ||
       a.fields.size()!=b.fields.size() || a.arrays.size()!=b.arrays.size() ||
       a.blobs.size()!=b.blobs.size() || a.children.size()!=b.children.size())
        return false;
    for(std::size_t i=0;i<a.fields.size();++i)
        if(name(a.fields[i].name)!=name(b.fields[i].name) ||
           a.fields[i].value!=b.fields[i].value) return false;
    for(std::size_t i=0;i<a.arrays.size();++i)
        if(name(a.arrays[i].name)!=name(b.arrays[i].name) ||
           a.arrays[i].columns!=b.arrays[i].columns ||
           a.arrays[i].values!=b.arrays[i].values) return false;
    for(std::size_t i=0;i<a.blobs.size();++i)
        if(name(a.blobs[i].name)!=name(b.blobs[i].name) ||
           a.blobs[i].bytes!=b.blobs[i].bytes) return false;
    for(std::size_t i=0;i<a.children.size();++i)
        if(!same_record(a.children[i],b.children[i])) return false;
    return true;
}
bool same_administration(const saves::SavedSessionSnapshot &a,
                         const saves::SavedSessionSnapshot &b) {
    if(!a.all_players || !b.all_players ||
       a.all_players->players.size()!=b.all_players->players.size()) return false;
    for(std::size_t i=0;i<a.all_players->players.size();++i) {
        const auto &x=a.all_players->players[i].administration;
        const auto &y=b.all_players->players[i].administration;
        if(x.layout!=y.layout || x.start_offset!=y.start_offset ||
           x.end_offset!=y.end_offset || x.blocks.size()!=y.blocks.size()) return false;
        for(std::size_t j=0;j<x.blocks.size();++j)
            if(!same_record(x.blocks[j],y.blocks[j])) return false;
    }
    return true;
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,
                                       std::uint32_t id) {
    for(const auto &entry:doc.sections) if(entry.id==id) return entry;
    throw std::runtime_error("Missing BNW player administration section");
}
struct Cleanup {
    std::filesystem::path first,second;
    ~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }
};
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source().snapshot;
    auto changed=source;
    auto &player=changed.all_players->players.at(0);
    auto &treasury=block(player,"treasury");
    auto &gold=field(treasury,"m_iGold");
    check(gold<INT32_MAX,"Selected BNW gold at i32 limit");
    ++gold;
    auto &pre_city=block(player,"pre_city");
    auto &popped=field(pre_city,"m_bEverPoppedGoody");
    check(popped==0 || popped==1,"Selected BNW goody flag invalid");
    popped^=1;
    bool changed_slot=false,changed_blob=false;
    for(auto &owner:changed.all_players->players) {
        auto &notifications=block(owner,"notifications");
        if(!notifications.children.empty() && !changed_slot) {
            auto &x=field(notifications.children.front(),"m_iX");
            check(x<INT32_MAX,"Selected BNW notification X at i32 limit");
            ++x;changed_slot=true;
        }
        for(auto &entry:notifications.children)
            for(auto &blob:entry.blobs)
                if(!blob.bytes.empty() && !changed_blob) {
                    blob.bytes.front()^=1;changed_blob=true;
                }
        auto &names=block(owner,"pre_city");
        for(auto &entry:names.children)
            for(auto &blob:entry.blobs)
                if(!blob.bytes.empty() && !changed_blob) {
                    blob.bytes.front()^=1;changed_blob=true;
                }
    }
    const auto checkpoint=runtime::make_bnw_player_administration_checkpoint(
        changed,source,host.identity());
    check(checkpoint.id==runtime::bnw_player_administration_checkpoint_section_id,
          "BNW player administration section ID changed");
    {
        auto bad=changed;
        ++field(block(bad.all_players->players.at(0),"treasury"),"version");
        rejects([&] {(void)runtime::make_bnw_player_administration_checkpoint(
            bad,source,host.identity());});
        bad=changed;
        bad.all_players->players.at(0).saved_player_id=1;
        rejects([&] {(void)runtime::make_bnw_player_administration_checkpoint(
            bad,source,host.identity());});
        bad=changed;
        block(bad.all_players->players.at(0),"treasury").arrays.front().values.push_back(1);
        rejects([&] {(void)runtime::make_bnw_player_administration_checkpoint(
            bad,source,host.identity());});
        if(changed_slot) {
            bad=changed;
            auto &notification=block(bad.all_players->players.at(0),"notifications");
            if(!notification.children.empty()) {
                notification.children.front().blobs.front().bytes.push_back(1);
                rejects([&] {(void)runtime::make_bnw_player_administration_checkpoint(
                    bad,source,host.identity());});
            }
        }
    }
    runtime::validate_bnw_player_administration_composition(
        std::array{checkpoint});
    for(const std::uint32_t overlapping:{8u,10u,11u,15u,30u,33u,34u})
        rejects([&] {runtime::validate_bnw_player_administration_composition(
            std::array{checkpoint,saves::CivisSaveSection{overlapping,0,{1}}});});
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    Cleanup cleanup{
        std::filesystem::current_path()/("bnw-full-administration-"+suffix+"-1.CivisSave"),
        std::filesystem::current_path()/("bnw-full-administration-"+suffix+"-2.CivisSave")};
    auto document=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    document.sections.push_back(checkpoint);
    saves::write_civis_save(cleanup.first,document);
    const auto loaded=saves::read_civis_save(cleanup.first);
    runtime::validate_bnw_player_administration_composition(loaded.sections);
    {
        auto ambiguous=document;
        ambiguous.sections.push_back({8,0,{1}});
        saves::write_civis_save(cleanup.second,ambiguous);
        rejects([&] {(void)saves::read_loaded_save(cleanup.second);});
        check(std::filesystem::remove(cleanup.second),
              "Cannot retire rejected BNW administration import fixture");
        const auto imported=saves::read_loaded_save(cleanup.first);
        check(imported.live_bnw_player_administration &&
              *imported.live_bnw_player_administration==checkpoint.bytes,
              "Normal Civis importer lost ID36");
        const auto reexport=saves::make_civis_import_document(imported,original.sha256);
        check(section(reexport,checkpoint.id).bytes==checkpoint.bytes,
              "Normal Civis re-export lost ID36");
        for(const std::uint32_t overlapping:{8u,10u,11u,15u,30u,33u,34u}) {
            auto conflict=imported;
            if(overlapping==8) conflict.live_team_research=std::vector<std::uint8_t>{1};
            if(overlapping==10) conflict.live_treasury=std::vector<std::uint8_t>{1};
            if(overlapping==11) conflict.live_research_turn=std::vector<std::uint8_t>{1};
            if(overlapping==15) conflict.live_research_selection=std::vector<std::uint8_t>{1};
            if(overlapping==30) {
                conflict.live_treasury=std::vector<std::uint8_t>{1};
                conflict.live_bnw_treasury_balance=std::vector<std::uint8_t>{1};
            }
            if(overlapping==33) {
                conflict.live_team_research=std::vector<std::uint8_t>{1};
                conflict.live_research_turn=std::vector<std::uint8_t>{1};
                conflict.live_bnw_research_queue=std::vector<std::uint8_t>{1};
            }
            if(overlapping==34) conflict.live_bnw_maintenance_expense=std::vector<std::uint8_t>{1};
            rejects([&] {(void)saves::make_civis_import_document(
                conflict,original.sha256);});
        }
        saves::SavedSessionDecodeContext decode;
        decode.decode_map_plots=false;
        rejects([&] {(void)saves::read_saved_session(cleanup.first,decode);});
        decode.permit_live_bnw_player_administration_overlay=true;
        const auto opt_in=saves::read_saved_session(cleanup.first,decode);
        check(opt_in.source.live_bnw_player_administration &&
              *opt_in.source.live_bnw_player_administration==checkpoint.bytes,
              "Explicit BNW ID36 decode opt-in lost the section");
        auto normal=ui::bootstrap_selected_bnw_preview(cleanup.first,game_root,0);
        check(!normal.preview.host && normal.preview.blocker &&
              normal.preview.blocker->stage==ui::SelectedBnwPreviewStage::live_restore,
              "Normal BNW LiveSession published unsynchronized ID36 raw state");
    }
    auto restored=runtime::restore_bnw_player_administration_checkpoint(
        section(loaded,checkpoint.id),source,source,host.identity());
    check(same_administration(restored,changed),
          "BNW full administration round-trip omitted a field, array or string byte");
    {
        auto corrupt=section(loaded,checkpoint.id);
        corrupt.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_player_administration_checkpoint(
            corrupt,source,source,host.identity());});
        auto foreign=host.identity();foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_player_administration_checkpoint(
            section(loaded,checkpoint.id),source,source,foreign);});
        rejects([&] {(void)runtime::restore_bnw_player_administration_checkpoint(
            section(loaded,checkpoint.id),changed,source,host.identity());});
        check(same_administration(source,host.session().source().snapshot),
              "Rejected BNW administration checkpoint changed immutable source");
    }
    const auto again=runtime::make_bnw_player_administration_checkpoint(
        restored,source,host.identity());
    auto second=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    second.sections.push_back(again);
    saves::write_civis_save(cleanup.second,second);
    const auto reread=saves::read_civis_save(cleanup.second);
    const auto final=runtime::restore_bnw_player_administration_checkpoint(
        section(reread,again.id),source,source,host.identity());
    check(same_administration(final,changed) &&
          runtime::identify_file(path).sha256==original.sha256 &&
          !host.session().readiness().can_continue(),
          "BNW administration re-save changed state, source or readiness");
    std::cout << "PASS BNW full PlayerAdministration raw checkpoint"
              << " (notification slot=" << changed_slot
              << ", string byte=" << changed_blob << ")\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW full PlayerAdministration checkpoint: "
                  << error.what() << '\n';return 1;
    }
}
