#include "bnw_research_choice_restart_fixture.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include <charconv>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void need(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
bool same_file(const runtime::FileIdentity& a,const runtime::FileIdentity& b){return a.bytes==b.bytes&&a.sha256==b.sha256;}
template<class T>T number(std::string_view text){
    T result{};const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    need(parsed.ec==std::errc{}&&parsed.ptr==text.data()+text.size(),"Invalid integer in measured reference");return result;
}
tests::BnwResearchRestartMeasurement prefix(const tests::BnwResearchRestartMeasurement& all,std::string_view tag){
    tests::BnwResearchRestartMeasurement out;
    for(const auto& [key,value]:all)if(key.starts_with(tag))need(out.emplace(key.substr(tag.size()),value).second,"Duplicate semantic projection");
    need(!out.empty(),"Missing before/after actual observation");return out;
}
bool research_change(std::string_view key,std::string_view actor,std::string_view raw_prefix){
    return key=="owned.queue."+std::string(actor)||key=="owned.researching."+std::string(actor)||
        key=="owned.research_section.sha256"||key=="owned.research_turn_section.sha256"||
        key==std::string(raw_prefix)+"queue"||key==std::string(raw_prefix)+"queue_count"||
        (key.starts_with(std::string(raw_prefix)+"researching.")&&key.ends_with(".values"));
}
void verify(const std::filesystem::path& checkpoint,const std::filesystem::path& game_root,
    const std::filesystem::path& reference_path,const std::filesystem::path& original,
    const std::filesystem::path& measured_output){
    const auto checkpoint_pin=runtime::identify_file(checkpoint),original_pin=runtime::identify_file(original),reference_pin=runtime::identify_file(reference_path);
    const auto reference=tests::read_bnw_research_measurement(reference_path);
    const auto get=[&](const std::string& key)->const std::string&{
        const auto found=reference.find(key);need(found!=reference.end(),"Required genuine reference measurement absent");return found->second;};
    need(get("meta.gui_source_outcome")=="complete"&&get("meta.full_continuation")=="false"&&
        get("meta.original_bnw_parity")=="false"&&get("meta.atomic_writes")=="2"&&get("meta.normal_loads")=="2",
        "Reference was not recorded after complete actual action/save/reload fixture");
    need(original_pin.sha256==get("meta.original.sha256")&&original_pin.bytes==number<std::uint64_t>(get("meta.original.bytes")),
        "Actual original Civ5Save differs from fixture source");
    const bool first=checkpoint_pin.sha256==get("meta.first.sha256")&&checkpoint_pin.bytes==number<std::uint64_t>(get("meta.first.bytes"));
    const bool resaved=checkpoint_pin.sha256==get("meta.resave.sha256")&&checkpoint_pin.bytes==number<std::uint64_t>(get("meta.resave.bytes"));
    need(first||resaved,"Actual checkpoint bytes are not a measured fixture output");
    // Routing text carries no readiness. Both branches perform genuine
    // activation; section fingerprints and full measured state remain required.
    const auto route=reference.find("meta.bootstrap");
    const bool installed_base_unit_art=route!=reference.end()&&route->second=="installed-base-unit-art-v1";
    need(route==reference.end()||installed_base_unit_art||route->second=="legacy-plain-v1",
        "Unknown measured research restart activation mode");
    const auto before=prefix(reference,"before."),after=prefix(reference,"after.");
    need(before.size()==after.size(),"Measured semantic field coverage changed across action");
    const auto actor_text=get("meta.actor");const auto actor=number<std::int64_t>(actor_text);
    need(actor>=0&&actor<64,"Actor is outside selected BNW source roster");
    need(after.at("game.active_player")==actor_text&&before.at("game.active_player")==actor_text,
        "Measured action belongs to another actual client actor");
    need(before.at("owned.queue."+actor_text)!=after.at("owned.queue."+actor_text)&&!after.at("owned.queue."+actor_text).empty(),
        "Reference contains no real changed research choice");
    std::string raw_prefix;
    for(const auto& [key,value]:after)if(key.starts_with("raw.player.")&&key.ends_with(".saved_id")&&value==actor_text){
        need(raw_prefix.empty(),"Measured raw actor identity is duplicated");raw_prefix=key.substr(0,key.size()-std::string_view{"saved_id"}.size());}
    need(!raw_prefix.empty(),"Measured actual raw player identity is missing");
    for(const auto& [key,value]:before){const auto found=after.find(key);need(found!=after.end(),"After observation dropped required semantic field");
        if(value!=found->second)need(research_change(key,actor_text,raw_prefix),"Actual reference changed a non-research semantic field");}

    const auto doc=saves::read_civis_save(checkpoint);const auto imported=saves::read_imported_civis_save(checkpoint);
    need(doc.version==number<std::uint32_t>(get("meta.document.version"))&&
        doc.flags==number<std::uint32_t>(get("meta.document.flags")),"Actual checkpoint container version/flags changed");
    need(imported.metadata.source_sha256==original_pin.sha256&&imported.live_bnw_research_queue&&
        imported.live_team_research&&imported.live_research_turn,"Checkpoint lacks genuine original identity or partial research companions");
    std::set<std::uint32_t> ids;std::size_t measured_sections=0;
    for(const auto& [key,value]:reference){(void)value;if(key.starts_with("retained.")&&key.ends_with(".sha256"))++measured_sections;}
    need(measured_sections==doc.sections.size(),"Checkpoint retained section coverage differs from measured reference");
    for(const auto& section:doc.sections){need(ids.insert(section.id).second,"Duplicate actual checkpoint section");
        const auto tag="retained."+std::to_string(section.id)+".";
        need(section.flags==number<std::uint32_t>(get(tag+"flags"))&&section.bytes.size()==number<std::uint64_t>(get(tag+"bytes"))&&
            runtime::identify_bytes(section.bytes).sha256==get(tag+"sha256"),"Actual retained section bytes/flags differ from completed fixture");}

    // Ordinary fresh loader only. No EARLY reuse, mutable session, action hooks,
    // GUI/native activation or renderer receipt is created by this executable.
    auto loaded=installed_base_unit_art
        ? ui::bootstrap_selected_bnw_preview_with_installed_base_unit_art(checkpoint,game_root,actor)
        : ui::bootstrap_selected_bnw_preview(checkpoint,game_root,actor);
    need(loaded.preview.host&&!loaded.preview.blocker,"Fresh normal loader rejected the actual research checkpoint");
    ui::SelectedBnwReadOnlyPreview preview(std::move(loaded.preview));
    const auto actual=tests::measure_actual_bnw_research_restart(preview);
    need(actual==after,"Fresh process measured research/raw flags/RNG/map/turn/rules/players differently");
    need(!preview.session().readiness().can_continue(),"Partial research load falsely grants full continuation");
    need(same_file(runtime::identify_file(checkpoint),checkpoint_pin)&&same_file(runtime::identify_file(original),original_pin)&&
        same_file(runtime::identify_file(reference_path),reference_pin),"Source/checkpoint/reference changed during fresh normal load");
    auto emitted=actual;
    emitted["measurement.mode"]="fresh-normal-loader-read-only";
    emitted["measurement.full_continuation"]="false";emitted["measurement.original_bnw_parity"]="false";
    emitted["measurement.checkpoint.sha256"]=checkpoint_pin.sha256;emitted["measurement.original.sha256"]=original_pin.sha256;
    emitted["measurement.reference.sha256"]=reference_pin.sha256;
    tests::write_new_bnw_research_measurement(measured_output,emitted);
    need(same_file(runtime::identify_file(checkpoint),checkpoint_pin)&&same_file(runtime::identify_file(original),original_pin)&&
        same_file(runtime::identify_file(reference_path),reference_pin),"Source/checkpoint/reference changed while emitting measured summary");
    std::cout<<"PASS measured partial research checkpoint through ordinary fresh loader; full continuation unavailable\n";
}
}
int main(int argc,char** argv){
    if(argc!=6){std::cout<<"UNAVAILABLE: supply genuine fixture CHECKPOINT GAME_ROOT REFERENCE ORIGINAL_CIV5 NEW_MEASURED_OUTPUT\n";return 2;}
    try{verify(argv[1],argv[2],argv[3],argv[4],argv[5]);return 0;}
    catch(const std::exception& error){std::cerr<<"FAIL fresh research checkpoint verifier: "<<error.what()<<'\n';return 1;}
}
