#include "civ5/runtime/bnw_team_tech_suffix_receipt_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
template<class Action> void rejected(Action &&action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Invalid BNW suffix checkpoint was accepted");
}
template<class T> T &named(std::vector<T> &items,std::string_view name) {
    for(auto &item:items)
        if(item.name && std::string_view(item.name)==name) return item;
    throw std::runtime_error("Missing selected BNW raw team array");
}
void prepare(LiveSessionState &state,const BnwTeamTechLuaAttempt &quote,
             const ui::SelectedBnwPreviewHost &host,
             const content::InfoTypeCatalog &types) {
    state.technologies[63].set_has_tech(0,true);
    auto &team=state.world.teams->teams[63];
    auto &hashes=named(team.arrays,"tech_hashes").values;
    auto &flags=named(team.arrays,"has_tech").values;
    std::size_t found=0;
    for(std::size_t i=0;i<hashes.size();++i)
        if(hashes[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            flags[i]=1;++found;
        }
    check(found==1,"Selected Agriculture raw hash changed");
    // Fixture predecessor only: a real Lua gameplay epoch is not claimed.
    state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,0,true,false,true});
    check(run_bnw_team_tech_best_route(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Selected best-route predecessor missing");
    check(run_bnw_team_tech_player_research(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Selected player-loop predecessor missing");
    check(run_bnw_team_tech_share_culture(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Selected share/culture predecessor missing");
}
struct TemporaryFile {
    std::filesystem::path path;
    ~TemporaryFile() {std::error_code ignored;std::filesystem::remove(path,ignored);}
};
TemporaryFile name(std::string_view label) {
    const auto ticks=std::chrono::steady_clock::now().time_since_epoch().count();
    return {std::filesystem::temp_directory_path()/
        ("civis-bnw-tech-receipt-"+std::string(label)+"-"+
         std::to_string(ticks)+".CivisSave")};
}
void round_trip(const BnwTeamTechSuffixCallbackMarker &marker,
                const LiveSessionState &prepared,
                const BnwTeamTechLuaAttempt &quote,
                const ui::SelectedBnwPreviewHost &host,
                const content::InfoTypeCatalog &types,
                std::string_view label) {
    TemporaryFile file=name(label);
    saves::CivisSaveDocument document;
    // Test-only dependency sections. The real full-world/VM/escrow section
    // IDs are assigned by their owners before ID44 enters the common loader.
    document.sections.push_back({100,0,{1,2,3}});
    document.sections.push_back({101,0,{4,5,6}});
    document.sections.push_back({102,0,{7,8,9}});
    const auto binding=bind_bnw_team_tech_suffix_document(
        document,host.identity(),17,{100},101,102);
    document.sections.push_back(
        make_bnw_team_tech_suffix_receipt_checkpoint(marker,binding));
    saves::write_civis_save(file.path,document);
    const auto reloaded=saves::read_civis_save(file.path);
    check(reloaded.sections.size()==4,"BNW receipt physical save lost sections");
    rejected([&]{saves::write_civis_save(file.path,document);});
    check(saves::read_civis_save(file.path).sections[3].bytes==
              reloaded.sections[3].bytes,
          "Exclusive atomic save overwrote existing receipt");

    const auto physical=bind_bnw_team_tech_suffix_document(
        reloaded,host.identity(),17,{100},101,102);
    check(physical.world_sha256==binding.world_sha256 &&
          physical.lua_vm_sha256==binding.lua_vm_sha256 &&
          physical.external_escrow_sha256==binding.external_escrow_sha256,
          "Physical dependency sections changed after atomic write");
    auto tampered=reloaded;
    tampered.sections[0].bytes[0]^=1;
    const auto altered=bind_bnw_team_tech_suffix_document(
        tampered,host.identity(),17,{100},101,102);
    auto untouched=prepared;
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        tampered.sections[3],untouched,altered);});
    check(!untouched.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
          "Tampered physical world section restored callback receipt");

    auto restored=prepared;
    auto wrong=binding;wrong.revision++;
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,wrong);});
    check(!restored.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
          "Revision mismatch mutated detached state");
    wrong=binding;wrong.world_sha256=std::string(64,'c');
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,wrong);});
    wrong=binding;wrong.lua_vm_sha256=std::string(64,'d');
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,wrong);});
    wrong=binding;wrong.external_escrow_sha256=std::string(64,'e');
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,wrong);});
    wrong=binding;wrong.source.rules_sha256=std::string(64,'f');
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,wrong);});
    check(!restored.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
          "Mismatched checkpoint identity mutated detached state");
    auto truncated=reloaded.sections[3];truncated.bytes.pop_back();
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        truncated,restored,binding);});
    check(!restored.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
          "Truncated callback receipt mutated detached state");

    restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,physical);
    const auto *actual=restored.setup.extensions.find<
        BnwTeamTechSuffixCallbackMarker>();
    check(actual && actual->team==marker.team &&
          actual->technology==marker.technology &&
          actual->ui_query_attempted==marker.ui_query_attempted &&
          actual->ui_query_confirmed==marker.ui_query_confirmed &&
          actual->no_reward_popups==marker.no_reward_popups &&
          actual->engine_attempted==marker.engine_attempted &&
          actual->engine_confirmed==marker.engine_confirmed &&
          actual->uncertain==marker.uncertain,
          "BNW suffix one-shot receipt changed after reload");
    rejected([&]{restore_bnw_team_tech_suffix_receipt_checkpoint(
        reloaded.sections[3],restored,physical);});

    std::size_t callbacks=0;
    BnwTeamTechSuffixCallbackOwner owner;
    owner.getter_world=&restored;
    owner.identity=host.identity();
    owner.selected_ui_option_owner=true;
    owner.selected_engine_owner=true;
    owner.external_writes_escrowed=true;
    owner.query_no_reward_popups=[&](const LiveSessionState &) {
        ++callbacks;return std::optional<bool>{false};
    };
    owner.gameplay_tech_acquired=[&](auto,auto,const auto &) {
        ++callbacks;return true;
    };
    const auto retry=quote_bnw_team_tech_suffix_callbacks(
        restored,quote,host.technologies(),host.database(),types,
        binding.source,&owner,63,0);
    check(retry.status==BnwTeamTechSuffixCallbackStatus::blocked &&
          callbacks==0 && !retry.grant_complete,
          "Reloaded suffix receipt replayed an external callback");
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi BNW profile changed");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    auto prepared=host.session().state();
    prepare(prepared,quote,host,types);
    // Physical fixture sections exercise digest binding; they are not claims
    // that the selected gameplay VM/escrow can already be serialized.
    round_trip({63,0,true,true,false,true,true,false},
               prepared,quote,host,types,"confirmed");
    round_trip({63,0,true,true,true,true,false,true},
               prepared,quote,host,types,"engine-uncertain");
    round_trip({63,0,true,false,std::nullopt,false,false,true},
               prepared,quote,host,types,"ui-uncertain");
    saves::CivisSaveDocument incomplete;
    incomplete.sections={{100,0,{1}}};
    rejected([&]{bind_bnw_team_tech_suffix_document(
        incomplete,host.identity(),17,{100},101,102);});
    auto binding=bind_bnw_team_tech_suffix_document(
        saves::CivisSaveDocument{1,0,{{100,0,{1}},{101,0,{2}},
                                      {102,0,{3}}}},
        host.identity(),17,{100},101,102);
    rejected([&]{make_bnw_team_tech_suffix_receipt_checkpoint(
        {63,0,false,false,std::nullopt,false,false,false},binding);});
}
