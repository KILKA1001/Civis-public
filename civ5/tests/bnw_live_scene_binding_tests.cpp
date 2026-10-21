#include "civ5/ui/bnw_live_scene_binding.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/graphics/bnw_scene_capture_baseline.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
namespace fs = std::filesystem;
using namespace civ5;

void check(bool valid,const char *reason) {
    if(!valid) throw std::runtime_error(reason);
}

struct TemporaryFiles {
    fs::path import,overlay;
    ~TemporaryFiles() {
        std::error_code ignored;
        fs::remove(overlay,ignored);
        fs::remove(import,ignored);
    }
};

void run(const fs::path &source,const fs::path &game_root,
         const fs::path &export_fixture={},bool treasury_only=false) {
    saves::CivisSaveDocument envelope;
    for(const auto id : {saves::CivisSaveSectionId::imported_payload,
                         saves::CivisSaveSectionId::source_metadata,
                         saves::CivisSaveSectionId::source_prefix})
        envelope.sections.push_back({static_cast<std::uint32_t>(id),0,{1}});
    check(!ui::unbound_bnw_scene_section(envelope),
          "Plain immutable import envelope was rejected");
    for(std::uint32_t id=30;id<=40;++id) {
        envelope.sections.push_back({id,0,{1}});
        check(ui::unbound_bnw_scene_section(envelope)==id,
              "BNW section 30-40 escaped the scene revision guard");
        envelope.sections.pop_back();
    }
    for(const std::uint32_t id : {7u,16u,28u,29u,41u}) {
        envelope.sections.push_back({id,0,{1}});
        check(ui::unbound_bnw_scene_section(envelope)==id,
              "Older or future live section escaped the scene revision guard");
        envelope.sections.pop_back();
    }
    envelope.sections.push_back({10,0,{1}});
    check(ui::unbound_bnw_scene_section(envelope,
              ui::BnwSceneOverlayPolicy::treasury_presentation)==10,
          "Unpaired owned treasury escaped the explicit policy");
    envelope.sections.push_back({30,0,{2}});
    check(graphics::bnw_treasury_presentation_digest(envelope).has_value(),
          "Paired treasury did not produce a candidate digest");
    const auto digest=graphics::bnw_treasury_presentation_digest(envelope);
    envelope.sections.back().bytes.front()=3;
    check(graphics::bnw_treasury_presentation_digest(envelope)!=digest,
          "Treasury candidate digest ignored changed raw bytes");
    envelope.sections.back().flags=1;
    check(!graphics::bnw_treasury_presentation_digest(envelope),
          "Flagged treasury accepted");
    envelope.sections.back().flags=0;
    envelope.sections.push_back({30,0,{3}});
    check(!graphics::bnw_treasury_presentation_digest(envelope),
          "Duplicate treasury accepted");
    envelope.sections.back().id=40;
    check(!graphics::bnw_treasury_presentation_digest(envelope),
          "Treasury policy admitted another world section");

    const auto nonce=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto root=fs::temp_directory_path();
    const TemporaryFiles files{root/("bnw-scene-import-"+nonce+".CivisSave"),
                               root/("bnw-scene-overlay-"+nonce+".CivisSave")};
    const auto loaded=saves::read_loaded_save(source);
    const auto document=saves::make_civis_import_document(
        loaded,runtime::identify_file(source).sha256);
    if(!graphics::has_gandhi_bnw_scene_capture_baseline(document))
        for(const auto &section:document.sections)
            std::cerr << "capture source section " << section.id << ' ' <<
                runtime::identify_bytes(section.bytes).sha256 << '\n';
    check(graphics::has_gandhi_bnw_scene_capture_baseline(document),
          "Selected immutable import failed capture baseline identity");
    for(const auto id:{1u,4u,5u}) {
        auto altered=document;
        for(auto &section:altered.sections)
            if(section.id==id) section.bytes.front()^=1;
        check(!graphics::has_gandhi_bnw_scene_capture_baseline(altered),
              "Changed source payload/metadata/setup reused a native capture");
    }
    saves::write_civis_save(files.import,document);
    saves::SavedSessionDecodeContext decode;
    decode.decode_map_regions=true;
    decode.decode_teams=true;
    decode.decode_all_players=true;
    decode.decode_supporting_classes=true;
    decode.all_players=saves::AllPlayersContext{};
    const auto source_scene=saves::read_saved_session(source,decode);
    const auto source_art=saves::observe_saved_map_team_art(
        *source_scene.snapshot.pregame,*source_scene.snapshot.map_plots);
    const auto source_receipt=graphics::capture_bnw_renderer_scene_receipt(
        source,runtime::identify_file(source).sha256,source_scene,source_art);
    check(source_receipt && source_receipt->applied_live_sections.empty(),
          "Selected original renderer scene did not produce a source-only receipt");
    auto diagnostic=ui::bootstrap_selected_bnw_preview(source,game_root,0);
    check(diagnostic.preview.host!=nullptr,
          "Selected live owner unavailable for scene diagnostic");
    const auto &live_cells=diagnostic.preview.host->session().state().world
        .map_projection->cells;
    std::size_t changed_cells=0;
    for(std::size_t i=0;i<source_receipt->cells.size();++i)
        if(graphics::bnw_scene_cell_revision(live_cells[i])!=
           source_receipt->cells[i]) {
            ++changed_cells;
            check(i==632 && !source_receipt->cells[i].is_city &&
                  live_cells[i].is_city,
                  "Unexpected Gandhi scene difference bypassed the city load hook");
        }
    check(changed_cells==1,
          "Gandhi city load hook no longer explains the raw/live scene difference");
    auto unexpected=diagnostic.preview.host->session().state();
    unexpected.world.map_projection->cells[0].terrain_type=123456;
    auto stale_raw_projection=*source_scene.snapshot.map_projection;
    auto stale_raw_receipt=*source_receipt;
    check(!graphics::reconcile_bnw_renderer_scene_projection(
              stale_raw_projection,stale_raw_receipt,unexpected),
          "Unexpected second renderer/live difference was accepted");
    const auto import_scene=saves::read_saved_session(files.import,decode);
    const auto import_art=saves::observe_saved_map_team_art(
        *import_scene.snapshot.pregame,*import_scene.snapshot.map_plots);
    const auto import_receipt=graphics::capture_bnw_renderer_scene_receipt(
        files.import,runtime::identify_file(files.import).sha256,
        import_scene,import_art);
    check(import_receipt && import_receipt->applied_live_sections.empty(),
          "Plain Civis renderer scene did not produce a source-only receipt");
    ui::BnwLivePreviewRequest request{source,game_root,0};
    auto source_projection=*source_scene.snapshot.map_projection;
    const ui::BnwPrepareRendererScene prepare_source=
        [&](const ui::LiveBnwPreviewInput *input,
            graphics::BnwRendererSceneReceipt &receipt) {
            source_projection=*source_scene.snapshot.map_projection;
            return input && graphics::reconcile_bnw_renderer_scene_projection(
                source_projection,receipt,input->session.state());
        };
    auto import_projection=*import_scene.snapshot.map_projection;
    const ui::BnwPrepareRendererScene prepare_import=
        [&](const ui::LiveBnwPreviewInput *input,
            graphics::BnwRendererSceneReceipt &receipt) {
            import_projection=*import_scene.snapshot.map_projection;
            return input && graphics::reconcile_bnw_renderer_scene_projection(
                import_projection,receipt,input->session.state());
        };
    int renders=0;
    const auto render=[&](const ui::LiveBnwPreviewInput *input) {
        check(input && input->active_player_id==0,
              "Selected BNW renderer received another live owner");
        ++renders;
    };
    ui::BnwLivePreviewOutcome outcome;
    if(!treasury_only) {
    auto no_prepare=ui::launch_selected_bnw_camera_preview(
        request,source,render,&*source_receipt);
    check(!no_prepare.rendered && no_prepare.blocker && renders==0 &&
          no_prepare.blocker->reason.find("reconciliation callback")!=
              std::string::npos,
          "Renderer receipt without post-load scene preparation was accepted");
    auto wrong_file_receipt=*source_receipt;
    wrong_file_receipt.scene_file_sha256.assign(64,'0');
    auto wrong_file=ui::launch_selected_bnw_camera_preview(
        request,source,render,&wrong_file_receipt,prepare_source);
    check(!wrong_file.rendered && wrong_file.blocker && renders==0 &&
          wrong_file.blocker->reason.find("scene file changed")!=
              std::string::npos,
          "Renderer receipt from another file was accepted");
    outcome=ui::launch_selected_bnw_camera_preview(
        request,source,render,&*source_receipt,prepare_source);
    check(outcome.rendered && !outcome.blocker && renders==1,
          "Selected original scene and UI did not launch");
    request.save_path=files.import;
    outcome=ui::launch_selected_bnw_camera_preview(
        request,source,render,&*source_receipt,prepare_source);
    check(outcome.rendered && !outcome.blocker && renders==2,
          "Plain Civis import did not pair with original scene");
    outcome=ui::launch_selected_bnw_camera_preview(
        request,files.import,render,&*import_receipt,prepare_import);
    check(outcome.rendered && !outcome.blocker && renders==3,
          "Plain Civis import did not pair with its identical scene file");
    auto stale_receipt=*source_receipt;
    stale_receipt.cells.front().owner=123456;
    request.save_path=source;
    outcome=ui::launch_selected_bnw_camera_preview(
        request,source,render,&stale_receipt,prepare_source);
    check(!outcome.rendered && outcome.blocker && renders==3 &&
          outcome.blocker->reason.find("same restored world revision")!=
              std::string::npos,
          "A stale decoded scene cell passed the renderer/UI revision guard");

    for(const std::uint32_t id : {30u,40u}) {
        auto overlaid=document;
        overlaid.sections.push_back({id,0,{1}});
        saves::write_civis_save(files.overlay,overlaid);
        request.save_path=files.overlay;
        outcome=ui::launch_selected_bnw_camera_preview(
            request,source,render,&*source_receipt,prepare_source);
        check(!outcome.rendered && outcome.blocker && renders==3 &&
              outcome.blocker->reason.find("unbound section "+
                  std::to_string(id))!=std::string::npos,
              "Selected Civis overlay paired with stale original scene");
        outcome=ui::launch_selected_bnw_camera_preview(
            request,files.overlay,render,&*source_receipt,prepare_source);
        check(!outcome.rendered && outcome.blocker && renders==3 &&
              outcome.blocker->reason.find("unbound section "+
                  std::to_string(id))!=std::string::npos,
              "Identical Civis hash was mistaken for restored scene revision");
        request.save_path=source;
        outcome=ui::launch_selected_bnw_camera_preview(
            request,files.overlay,render,&*source_receipt,prepare_source);
        check(!outcome.rendered && outcome.blocker && renders==3 &&
              outcome.blocker->reason.find("unbound section "+
                  std::to_string(id))!=std::string::npos,
              "Overlaid renderer scene accepted a plain UI source");
        fs::remove(files.overlay);
    }
    }
    const auto baseline_renders=renders;
    auto &host=*diagnostic.preview.host;
    auto state=host.session().state();
    const auto baseline=state.treasury;
    const auto old_balance=baseline.players.at(0).balance_times100;
    bool found_gold=false;
    for(auto &player:state.world.all_players->players)
        if(player.saved_player_id==0)
            for(auto &block:player.administration.blocks)
                for(auto &field:block.fields)
                    if(field.name && std::string_view(field.name)=="m_iGold") {
                        field.value+=100;found_gold=true;
                    }
    check(found_gold,"Selected raw gold missing");
    state.treasury.players.at(0).balance_times100+=100;
    const auto pair=runtime::make_bnw_treasury_balance_checkpoint_pair(
        state,host.session().source().snapshot,baseline,host.identity());
    auto treasury_document=document;
    treasury_document.sections.push_back(pair.raw);
    treasury_document.sections.push_back(pair.owned);
    check(graphics::has_gandhi_bnw_scene_capture_baseline(treasury_document),
          "Verified treasury pair lost its immutable scene capture baseline");
    auto unrelated_world=treasury_document;
    unrelated_world.sections.push_back({40,0,{1}});
    check(!graphics::has_gandhi_bnw_scene_capture_baseline(unrelated_world),
          "Another world overlay reused unchanged-map captures");
    saves::write_civis_save(files.overlay,treasury_document);
    auto treasury_decode=decode;
    treasury_decode.permit_live_treasury_overlay=true;
    treasury_decode.permit_live_bnw_treasury_balance_overlay=true;
    const auto treasury_scene=saves::read_saved_session(files.overlay,treasury_decode);
    const auto treasury_art=saves::observe_saved_map_team_art(
        *treasury_scene.snapshot.pregame,*treasury_scene.snapshot.map_plots);
    check(!graphics::capture_bnw_renderer_scene_receipt(
              files.overlay,runtime::identify_file(files.overlay).sha256,
              treasury_scene,treasury_art),
          "Default source-only receipt admitted treasury overlays");
    const auto treasury_receipt=graphics::capture_bnw_renderer_scene_receipt(
        files.overlay,runtime::identify_file(files.overlay).sha256,
        treasury_scene,treasury_art,
        ui::BnwSceneOverlayPolicy::treasury_presentation);
    check(treasury_receipt && !treasury_receipt->treasury_presentation_verified &&
          !treasury_receipt->treasury_presentation_sha256.empty(),
          "Treasury receipt was missing or prematurely verified");
    request.save_path=files.overlay;
    outcome=ui::launch_selected_bnw_camera_preview(
        request,source,render,&*source_receipt,prepare_source);
    check(!outcome.rendered && outcome.blocker && renders==baseline_renders,
          "Treasury UI paired with an original source scene");
    auto wrong_treasury_receipt=*treasury_receipt;
    wrong_treasury_receipt.treasury_presentation_sha256.assign(64,'0');
    outcome=ui::launch_selected_bnw_camera_preview(
        request,files.overlay,render,&wrong_treasury_receipt,prepare_source);
    check(!outcome.rendered && outcome.blocker && renders==baseline_renders,
          "Incorrect treasury digest was accepted");
    auto treasury_projection=*treasury_scene.snapshot.map_projection;
    bool verified=false;
    const ui::BnwPrepareRendererScene prepare_treasury=
        [&](const ui::LiveBnwPreviewInput *input,
            graphics::BnwRendererSceneReceipt &receipt) {
            verified=receipt.treasury_presentation_verified;
            return input && verified &&
                graphics::reconcile_bnw_renderer_scene_projection(
                    treasury_projection,receipt,input->session.state());
        };
    outcome=ui::launch_selected_bnw_camera_preview(
        request,files.overlay,[&](const ui::LiveBnwPreviewInput *input) {
            check(input && verified &&
                  input->session.state().treasury.players.at(0).balance_times100==
                      old_balance+100,
                  "Renderer did not receive the restored live treasury");
            const auto frame=ui::capture_live_session_ui_frame(
                input->session,1,0,input->frame_sources);
            const auto *panel=frame.find<ui::TopPanelLiveFrame>();
            check(panel && panel->players.at(0).gold_balance==
                      (old_balance+100)/100,
                  "Live UI frame still displayed the immutable source balance");
            ++renders;
        },&*treasury_receipt,prepare_treasury);
    check(outcome.rendered && !outcome.blocker && renders==baseline_renders+1 &&
          !outcome.readiness.can_continue(),
          "Paired treasury scene failed or falsely became playable");
    if(!export_fixture.empty())
        saves::write_civis_save(export_fixture,treasury_document);
    std::cout << "PASS BNW UI/scene binding; paired treasury restored; other live sections fail closed\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc>=3 && argc<=5,"Supply selected BNW save, game root, optional fixture output and --treasury-only");
        check(argc!=5 || std::string_view(argv[4])=="--treasury-only",
              "Unknown scene-binding test mode");
        run(argv[1],argv[2],argc>=4?fs::path(argv[3]):fs::path{},argc==5);
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW live scene binding: " << error.what() << '\n';
        return 1;
    }
}
