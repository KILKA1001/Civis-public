#include "bnw_research_choice_restart_fixture.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/research_turn_checkpoint.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <map>
#include <fstream>
#include <cstdio>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

namespace civ5::tests {
namespace {
using namespace civ5;
void need(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
template<class A,class B>bool same_control(const A& a,const B& b){
    return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
bool same_file(const runtime::FileIdentity& a,const runtime::FileIdentity& b){
    return a.bytes==b.bytes&&a.sha256==b.sha256;
}
bool same_section(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){
    return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;
}
std::map<std::uint32_t,const saves::CivisSaveSection*> sections(const saves::CivisSaveDocument& doc){
    std::map<std::uint32_t,const saves::CivisSaveSection*> result;
    for(const auto& part:doc.sections)need(result.emplace(part.id,&part).second,"Duplicate retained save section");
    return result;
}
void same_document(const saves::CivisSaveDocument& a,const saves::CivisSaveDocument& b){
    need(a.version==b.version&&a.flags==b.flags,"Save container version/flags changed");
    const auto aa=sections(a),bb=sections(b);need(aa.size()==bb.size(),"Retained save section count changed");
    for(const auto& [id,part]:aa){const auto found=bb.find(id);
        need(found!=bb.end()&&same_section(*part,*found->second),"Retained save section bytes/flags changed");}
}
std::shared_ptr<const ui::SelectedBnwPreviewHost> actual_host(const ui::SelectedBnwReadOnlyPreview& preview){
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
    preview.with_owned_host_camera_input([&](auto actual,const auto*){host=std::move(actual);});
    need(bool(host),"Genuine selected preview did not publish its actual Host");return host;
}
std::string_view name(const char* text){return text?std::string_view{text}:std::string_view{};}
struct Bytes {
    std::vector<std::uint8_t> data;
    void integer(std::uint64_t v){for(unsigned shift=0;shift<64;shift+=8)data.push_back(static_cast<std::uint8_t>(v>>shift));}
    void text(std::string_view v){integer(v.size());for(const char ch:v)data.push_back(static_cast<std::uint8_t>(ch));}
    void values(const std::vector<std::int64_t>& v){integer(v.size());for(const auto value:v)integer(static_cast<std::uint64_t>(value));}
};
// Test-only content fingerprint of genuine raw map values and actual post-load
// visibility. Source offsets, object addresses and UI revision are excluded.
std::string map_semantics(const runtime::LiveSessionState& state){
    need(state.world.map_plots.has_value(),"Actual decoded map plots are unavailable");
    const auto& h=state.world.map_header;Bytes b;
    b.integer(h.grid.width);b.integer(h.grid.height);for(auto v:h.guid)b.integer(v);
    b.integer(h.wrap_x);b.integer(h.wrap_y);b.integer(static_cast<std::uint64_t>(h.land_plots));
    b.integer(static_cast<std::uint64_t>(h.owned_plots));b.integer(static_cast<std::uint64_t>(h.natural_wonders));
    b.integer(static_cast<std::uint64_t>(h.top_latitude));b.integer(static_cast<std::uint64_t>(h.bottom_latitude));
    b.integer(h.map_generated.has_value());if(h.map_generated)b.integer(*h.map_generated);
    for(const auto& counts:h.resources){b.integer(counts.size());for(const auto& item:counts){
        b.integer(item.type_hash.has_value());if(item.type_hash)b.integer(*item.type_hash);
        b.integer(item.count.has_value());if(item.count)b.integer(static_cast<std::uint64_t>(*item.count));}}
    const auto& plots=state.world.map_plots->plots;b.integer(plots.size());
    for(const auto& plot:plots){
        b.integer(plot.index);b.integer(plot.x);b.integer(plot.y);b.integer(plot.fields.size());
        for(const auto& field:plot.fields){b.text(name(field.name));b.integer(static_cast<std::uint64_t>(field.value));}
        b.integer(plot.arrays.size());for(const auto& array:plot.arrays){b.text(name(array.name));b.integer(array.columns);b.values(array.values);}
        b.integer(plot.script_data.has_value());if(plot.script_data){b.integer(plot.script_data->size());b.data.insert(b.data.end(),plot.script_data->begin(),plot.script_data->end());}
        b.integer(plot.invisible_visibility.size());for(const auto& row:plot.invisible_visibility){
            b.integer(static_cast<std::uint64_t>(row.key));b.integer(row.wire_count);b.values(row.values);}
        b.integer(plot.legacy_build_progress.size());for(const auto& item:plot.legacy_build_progress){
            b.integer(item.type_hash.has_value());if(item.type_hash)b.integer(*item.type_hash);
            b.integer(item.count.has_value());if(item.count)b.integer(static_cast<std::uint64_t>(*item.count));}
    }
    const auto& visibility=runtime::bnw_live_plot_visibility_cache(state);
    b.integer(visibility.grid.width);b.integer(visibility.grid.height);b.integer(visibility.team_slots);b.values(visibility.counts);
    return runtime::identify_bytes(b.data).sha256;
}
using PlayerIdentity=std::tuple<std::int64_t,std::int64_t,std::int64_t,std::int64_t,bool,bool,std::set<std::int64_t>,bool>;
struct RawResearch {
    std::uint64_t index=0;std::int64_t saved_id=0,count=0,flag_count=0;
    std::vector<std::int64_t> queue;
    std::vector<std::pair<std::uint64_t,std::vector<std::int64_t>>> flags;
    bool operator==(const RawResearch&)const=default;
};
struct Observation {
    runtime::SessionCheckpointIdentity identity;
    std::int64_t active=-1,turn=-1,game_type=0,speed=0,start_era=0,current_era=0,handicap=0;
    std::uint64_t map_seed=0;
    runtime::RandomAlgorithm map_algorithm=runtime::RandomAlgorithm::bnw,game_algorithm=runtime::RandomAlgorithm::bnw;
    runtime::SimulationRandomState map_rng,game_rng;
    std::string map_hash;
    std::map<std::int64_t,PlayerIdentity> players;
    std::map<std::int64_t,std::int64_t> team_members;
    std::map<std::int64_t,std::vector<std::int64_t>> queues;
    std::map<std::int64_t,std::set<std::int64_t>> researching;
    std::vector<RawResearch> raw;
    saves::CivisSaveSection research,research_turn;
};
Observation observe(const ui::SelectedBnwPreviewHost& host){
    const auto state=host.session().read_state_lease();need(bool(state),"Actual live state absent");
    const auto revision=state->ui_source_revision,generation=host.session().state_generation();
    need(state->world.all_players.has_value(),"Actual raw player collection absent");
    Observation out;out.identity=host.identity();out.active=host.active_player_id();
    const auto& setup=state->setup;out.turn=setup.game_turn;out.game_type=setup.game_type;
    out.speed=setup.speed;out.start_era=setup.start_era;out.current_era=setup.current_era;out.handicap=setup.game_handicap;out.map_seed=setup.map_random_seed;
    out.map_algorithm=state->random.map.algorithm();out.game_algorithm=state->random.gameplay.algorithm();
    out.map_rng=state->random.map.state();out.game_rng=state->random.gameplay.state();out.map_hash=map_semantics(*state);
    for(const auto& [id,p]:setup.players)out.players.emplace(id,PlayerIdentity{p.team,p.handicap,p.slot_status,p.slot_claim,p.alive,p.ever_alive,p.policies,p.policies_resolved});
    out.team_members=setup.team_members;
    for(const auto& [id,p]:state->player_research.players)out.queues.emplace(id,p.queue);
    for(const auto& [id,p]:state->research_turn.players)out.researching.emplace(id,p.researching);
    for(const auto& player:state->world.all_players->players){
        RawResearch raw;raw.index=player.index;raw.saved_id=player.saved_player_id;
        const saves::PlayerAdministrationRecord* pre=nullptr;
        for(const auto& block:player.administration.blocks)if(name(block.name)=="pre_city"){need(!pre,"Duplicate raw pre-city research block");pre=&block;}
        need(pre,"Missing raw pre-city research block");
        bool have_queue=false,have_count=false;
        for(const auto& array:pre->arrays)if(name(array.name)=="m_researchQueue"){
            need(!have_queue,"Duplicate raw research queue");have_queue=true;raw.queue=array.values;}
        for(const auto& field:pre->fields)if(name(field.name)=="m_researchQueue_wire_count"){
            need(!have_count,"Duplicate raw research queue count");have_count=true;raw.count=field.value;}
        need(have_queue&&have_count&&raw.count>=0&&static_cast<std::uint64_t>(raw.count)==raw.queue.size(),"Raw queue/count differs");
        const saves::TeamHashedArray* flags=nullptr;
        for(const auto& block:player.knowledge.blocks)if(name(block.name)=="player_techs")
            for(const auto& array:block.hashes)if(name(array.name)=="researching_tech"){
                need(!flags,"Duplicate raw researching flags");flags=&array;}
        need(flags,"Missing raw researching flags");raw.flag_count=flags->wire_count;
        need(raw.flag_count>=0&&static_cast<std::uint64_t>(raw.flag_count)==flags->rows.size(),"Raw researching row count differs");
        for(const auto& row:flags->rows)raw.flags.emplace_back(row.type_hash,row.values);
        out.raw.push_back(std::move(raw));
    }
    auto research=runtime::make_research_checkpoint(state->technologies,state->player_research,setup,host.technologies(),host.identity());
    need(!research.profile,"BNW fixture unexpectedly produced profile research state");out.research=std::move(research.common);
    out.research_turn=runtime::make_research_turn_checkpoint(state->research_turn,setup,host.technologies(),host.identity());
    need(same_control(state,host.session().read_state_lease())&&generation==host.session().state_generation()&&revision==state->ui_source_revision,
        "Actual state changed while capturing restart observation");return out;
}
template<class T>std::string list(const T& values){
    std::string out;for(const auto value:values){if(!out.empty())out+=',';out+=std::to_string(value);}return out;
}
BnwResearchRestartMeasurement projection(const Observation& observed){
    BnwResearchRestartMeasurement out;
    out["identity.payload_sha256"]=observed.identity.payload_sha256;
    out["identity.rules_sha256"]=observed.identity.rules_sha256;
    out["game.active_player"]=std::to_string(observed.active);out["game.turn"]=std::to_string(observed.turn);
    out["game.type"]=std::to_string(observed.game_type);out["game.speed"]=std::to_string(observed.speed);
    out["game.start_era"]=std::to_string(observed.start_era);out["game.current_era"]=std::to_string(observed.current_era);
    out["game.handicap"]=std::to_string(observed.handicap);out["game.map_seed"]=std::to_string(observed.map_seed);
    out["map.semantic_sha256"]=observed.map_hash;
    out["rng.map.algorithm"]=std::to_string(static_cast<int>(observed.map_algorithm));
    out["rng.gameplay.algorithm"]=std::to_string(static_cast<int>(observed.game_algorithm));
    out["rng.map.seed"]=std::to_string(observed.map_rng.seed);out["rng.map.calls"]=std::to_string(observed.map_rng.calls);
    out["rng.map.resets"]=std::to_string(observed.map_rng.resets);out["rng.gameplay.seed"]=std::to_string(observed.game_rng.seed);
    out["rng.gameplay.calls"]=std::to_string(observed.game_rng.calls);out["rng.gameplay.resets"]=std::to_string(observed.game_rng.resets);
    for(const auto& [id,p]:observed.players){const auto prefix="player."+std::to_string(id)+".";
        out[prefix+"team"]=std::to_string(std::get<0>(p));out[prefix+"handicap"]=std::to_string(std::get<1>(p));
        out[prefix+"slot_status"]=std::to_string(std::get<2>(p));out[prefix+"slot_claim"]=std::to_string(std::get<3>(p));
        out[prefix+"alive"]=std::get<4>(p)?"1":"0";out[prefix+"ever_alive"]=std::get<5>(p)?"1":"0";
        out[prefix+"policies"]=list(std::get<6>(p));out[prefix+"policies_resolved"]=std::get<7>(p)?"1":"0";}
    for(const auto& [id,count]:observed.team_members)out["team."+std::to_string(id)+".members"]=std::to_string(count);
    for(const auto& [id,queue]:observed.queues)out["owned.queue."+std::to_string(id)]=list(queue);
    for(const auto& [id,flags]:observed.researching)out["owned.researching."+std::to_string(id)]=list(flags);
    for(const auto& raw:observed.raw){const auto prefix="raw.player."+std::to_string(raw.index)+".";
        out[prefix+"saved_id"]=std::to_string(raw.saved_id);out[prefix+"queue_count"]=std::to_string(raw.count);
        out[prefix+"queue"]=list(raw.queue);out[prefix+"researching_count"]=std::to_string(raw.flag_count);
        for(std::size_t n=0;n<raw.flags.size();++n){const auto key=prefix+"researching."+std::to_string(n)+".";
            out[key+"type_hash"]=std::to_string(raw.flags[n].first);out[key+"values"]=list(raw.flags[n].second);}}
    out["owned.research_section.sha256"]=runtime::identify_bytes(observed.research.bytes).sha256;
    out["owned.research_section.flags"]=std::to_string(observed.research.flags);
    out["owned.research_turn_section.sha256"]=runtime::identify_bytes(observed.research_turn.bytes).sha256;
    out["owned.research_turn_section.flags"]=std::to_string(observed.research_turn.flags);
    return out;
}
void same_unchanged_semantics(const Observation& a,const Observation& b){
    need(a.identity.payload_sha256==b.identity.payload_sha256&&a.identity.rules_sha256==b.identity.rules_sha256&&
        a.active==b.active&&a.turn==b.turn&&a.game_type==b.game_type&&a.speed==b.speed&&a.start_era==b.start_era&&
        a.current_era==b.current_era&&a.handicap==b.handicap&&a.map_seed==b.map_seed&&a.players==b.players&&a.team_members==b.team_members,
        "Turn/map/player/team or source/rules identities changed on restart");
    need(a.map_algorithm==b.map_algorithm&&a.game_algorithm==b.game_algorithm&&a.map_rng==b.map_rng&&a.game_rng==b.game_rng,
        "Actual RNG algorithm/state/calls/resets changed on restart");
    need(a.map_hash==b.map_hash,"Actual raw map/post-load visibility changed on restart");
}
void same_research(const Observation& a,const Observation& b){
    same_unchanged_semantics(a,b);need(a.queues==b.queues&&a.researching==b.researching&&a.raw==b.raw,
        "Restart lost actual owned/raw research queue/count/flags");
    need(same_section(a.research,b.research)&&same_section(a.research_turn,b.research_turn),
        "Restart changed complete owned research companion semantics");
}
void only_research_sections_changed(const saves::CivisSaveDocument& baseline,const saves::CivisSaveDocument& prepared){
    need(baseline.version==prepared.version&&baseline.flags==prepared.flags,"Research export changed container flags/version");
    const auto aa=sections(baseline),bb=sections(prepared);const std::set<std::uint32_t> allowed{8,11,33};
    for(const auto& [id,part]:aa)if(!allowed.contains(id)){
        const auto found=bb.find(id);need(found!=bb.end()&&same_section(*part,*found->second),"Research export changed/dropped another retained section");}
    for(const auto& [id,part]:bb)if(!allowed.contains(id)){
        const auto found=aa.find(id);need(found!=aa.end()&&same_section(*part,*found->second),"Research export introduced another retained section");}
    for(const auto id:allowed)need(bb.contains(id),"Real research export omitted a mandatory companion");
}
void clean_outputs(const std::filesystem::path& dir,const std::set<std::filesystem::path>& expected){
    std::set<std::filesystem::path> actual;
    for(const auto& entry:std::filesystem::directory_iterator(dir)){
        need(entry.is_regular_file(),"Research fixture output contains an unexpected directory");actual.insert(entry.path().filename());}
    need(actual==expected,"Atomic writer left a partial/temp or unexpected output");
}
void require_new_restart_frame(const ui::SelectedBnwReadOnlyPreview& preview,
    const ui::UiContextRegistry& registry,ui::ClientViewport viewport,
    terrain::CameraControllerState camera,std::uint64_t original_owner,std::int64_t player,
    const std::vector<std::int64_t>& queue){
    bool published=false;
    preview.with_owned_host_camera_input([&](auto host,const ui::LiveBnwPreviewInput* input){
        need(host&&input&&&host->session()==&input->session,"Restart frame did not use its genuine selected input");
        auto owner=ui::BnwClientFrameOwner::from_selected_host(host,input->frame_sources);
        auto ui_host=owner->create_selected_ui_host(registry,nullptr);
        const auto frame=owner->capture(viewport,camera);
        need(frame&&owner->is_current(frame)&&frame->stamp.identity.owner!=original_owner&&
            frame->stamp.identity.player==player,"Restart reused an old/unissued client frame");
        const auto published_ui=ui_host->begin_frame(viewport.framebuffer_width,viewport.framebuffer_height);
        const auto* progression=ui::progression_for_active_player(published_ui.snapshot);
        need(ui_host->is_current_frame(published_ui)&&published_ui.snapshot.template find<ui::BnwClientFrameStamp>()==frame->ui.template find<ui::BnwClientFrameStamp>()&&
            progression&&progression->player_id==player&&!queue.empty()&&
            progression->current_research==std::optional<std::int64_t>{queue.front()},"Restart did not freshly publish real research UI");
        for(const auto target:queue)need(std::any_of(progression->technologies.begin(),progression->technologies.end(),
            [&](const auto& row){return row.id==target&&row.queued&&!row.researched;}),"Restart frame lost an actual queued technology");
        published=true;
    });
    need(published,"Actual restarted client frame callback did not execute");
}
}
struct BnwResearchChoiceRestartFixture::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
    std::weak_ptr<const runtime::LiveSessionState> state;
    std::shared_ptr<runtime::BnwResearchQueueSaveOwner> save_owner;
    std::shared_ptr<const runtime::BnwResearchChoiceCommit> first_receipt;
    runtime::FileIdentity input_pin;
    std::filesystem::path input,game_root,output;
    Observation before;saves::CivisSaveDocument original;
    ui::UiContextRegistry registry;
    ui::ClientViewport viewport;
    terrain::CameraControllerState camera;
    std::uint64_t original_frame_owner=0;
    bool exported=false;
    bool installed_base_unit_art=false;
    void check_thread()const{need(thread==std::this_thread::get_id(),"Restart fixture belongs to its actual client thread");}
    std::shared_ptr<const ui::SelectedBnwPreviewHost> require_actual(const ui::SelectedBnwReadOnlyPreview& preview)const{
        check_thread();const auto actual=actual_host(preview),expected=host.lock();
        need(same_control(actual,expected)&&same_control(actual->session().read_state_lease(),state.lock()),"Research fixture substituted Host/live allocation");
        need(same_file(runtime::identify_file(input),input_pin),"Original selected input changed");return actual;
    }
    void require_completed(const ui::SelectedBnwReadOnlyPreview& preview,const ui::BnwAlphaProgressionBackend& backend,
                           const std::shared_ptr<const runtime::BnwResearchChoiceCommit>& receipt)const{
        const auto actual=require_actual(preview);
        need(backend.phase()==ui::BnwAlphaResearchUiPhase::complete&&same_control(backend.last_commit(),receipt),
            "Export requires actual production GUI/source completion, not just completed core stores");
        need(receipt&&receipt->phase()==runtime::BnwResearchChoicePhase::complete,"Actual completed core receipt absent");
        receipt->require_stored_current(*actual);
    }
};
BnwResearchRestartMeasurement measure_actual_bnw_research_restart(const ui::SelectedBnwReadOnlyPreview& preview){
    return projection(observe(*actual_host(preview)));
}
void write_new_bnw_research_measurement(const std::filesystem::path& path,const BnwResearchRestartMeasurement& measurement){
    // Measurement only; create exclusively so caller mistakes cannot truncate
    // original/save/reference files. No directory or file is deleted on failure.
    std::string serialized="civis-research-restart-measurement-v1\n";
    for(const auto& [key,value]:measurement){
        need(!key.empty()&&key.find_first_of("\t\r\n") == std::string::npos&&value.find_first_of("\t\r\n")==std::string::npos,
            "Measurement has invalid unescaped fields");
        serialized+=key;serialized+='\t';serialized+=value;serialized+='\n';
    }
#ifdef _WIN32
    const auto file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    need(file!=INVALID_HANDLE_VALUE,"Cannot exclusively create new measurement output");
    struct Close{HANDLE handle;~Close(){CloseHandle(handle);}} close{file};
    std::size_t offset=0;
    while(offset<serialized.size()){
        const auto count=static_cast<DWORD>(std::min<std::size_t>(serialized.size()-offset,1024U*1024U));
        DWORD written=0;need(WriteFile(file,serialized.data()+offset,count,&written,nullptr)&&written==count,"Measurement output write failed");
        offset+=written;
    }
    need(FlushFileBuffers(file)!=0,"Measurement output flush failed");
#else
    auto file=std::unique_ptr<std::FILE,int(*)(std::FILE*)>(std::fopen(path.c_str(),"wbx"),std::fclose);
    need(bool(file),"Cannot exclusively create new measurement output");
    need(std::fwrite(serialized.data(),1,serialized.size(),file.get())==serialized.size()&&std::fflush(file.get())==0,
        "Measurement output write/flush failed");
#endif
}
BnwResearchRestartMeasurement read_bnw_research_measurement(const std::filesystem::path& path){
    // Finite test artifact; no file path or code is executed from its contents.
    need(std::filesystem::file_size(path)<=8ULL*1024*1024,"Measurement reference exceeds finite size budget");
    std::ifstream input(path,std::ios::binary);need(bool(input),"Cannot read measurement reference");
    std::string line;need(bool(std::getline(input,line))&&line=="civis-research-restart-measurement-v1","Unknown measurement schema");
    BnwResearchRestartMeasurement out;
    while(std::getline(input,line)){
        const auto tab=line.find('\t');need(tab!=std::string::npos&&tab>0&&line.find('\t',tab+1)==std::string::npos&&line.find('\r')==std::string::npos,
            "Invalid measurement field framing");
        need(out.emplace(line.substr(0,tab),line.substr(tab+1)).second,"Duplicate measurement key");
    }
    need(input.eof(),"Measurement read failed");return out;
}
BnwResearchChoiceRestartFixture::BnwResearchChoiceRestartFixture(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
BnwResearchChoiceRestartFixture::~BnwResearchChoiceRestartFixture()=default;
std::unique_ptr<BnwResearchChoiceRestartFixture> BnwResearchChoiceRestartFixture::capture_before_action(
 const ui::SelectedBnwReadOnlyPreview& preview,const std::shared_ptr<const ui::BnwClientFrame>& before_frame,
 const ui::UiContextRegistry& registry,const std::filesystem::path& input,
 const std::filesystem::path& game_root,const std::filesystem::path& output){
    auto impl=std::make_unique<Impl>();const auto host=actual_host(preview);
    impl->input=input;impl->game_root=game_root;impl->output=output;impl->input_pin=runtime::identify_file(input);
    impl->host=host;impl->state=host->session().read_state_lease();impl->before=observe(*host);
    // Pure producer-lane routing. The restart must still earn the exact rules
    // fingerprint by real installed reads/compilation and ordinary live restore.
    impl->installed_base_unit_art=host->uses_civis_resource_bank();
    need(before_frame&&before_frame->stamp.identity.player==host->active_player_id()&&
        before_frame->stamp.identity.turn==host->session().state().setup.game_turn&&
        before_frame->stamp.identity.map_guid==host->session().state().world.map_header.guid&&
        before_frame->stamp.identity.ui_source_revision==host->session().state().ui_source_revision&&
        before_frame->stamp.viewport.drawable(),"Before-action fixture requires actual client frame/camera/viewport");
    need(registry.find("TechTree")&&registry.find("TopPanel")&&registry.find("DiploList"),"Actual research UI registry lacks required contexts");
    impl->registry=registry;impl->viewport=before_frame->stamp.viewport;impl->camera=before_frame->camera;
    impl->original_frame_owner=before_frame->stamp.identity.owner;
    need(host->session().source().source.container==saves::SaveContainerKind::civ5,"First actual action fixture requires genuine original Civ5 input");
    const auto imported=saves::read_loaded_save(input);
    need(imported.container==saves::SaveContainerKind::civ5,"Actual original fixture input is not Civ5Save");
    impl->original=saves::make_civis_import_document(imported,impl->input_pin.sha256);
    impl->save_owner=runtime::BnwResearchQueueSaveOwner::from_selected_preview(preview,input);
    need(same_file(runtime::identify_file(input),impl->input_pin),"Original input changed during fixture capture");
    need(!std::filesystem::exists(output),"Fixture requires a new caller-selected output directory");
    need(std::filesystem::create_directory(output),"Could not create exclusive fixture output directory");
    return std::unique_ptr<BnwResearchChoiceRestartFixture>(new BnwResearchChoiceRestartFixture(std::move(impl)));
}
BnwResearchChoiceRestartReport BnwResearchChoiceRestartFixture::verify_after_presented_choice(
 const ui::SelectedBnwReadOnlyPreview& preview,const ui::BnwAlphaProgressionBackend& backend,
 std::shared_ptr<const runtime::BnwResearchChoiceCommit> receipt){
    auto& p=*impl_;p.require_completed(preview,backend,receipt);need(!p.exported,"Actual fixture already exported its first action");
    const auto host=p.require_actual(preview);const auto expected=observe(*host);same_unchanged_semantics(p.before,expected);
    need(expected.queues.at(receipt->player())==receipt->queue()&&expected.queues.at(receipt->player())!=p.before.queues.at(receipt->player()),
        "Real presented research choice did not change its queue");
    for(const auto& [player,queue]:p.before.queues)if(player!=receipt->player()){
        need(expected.queues.at(player)==queue&&expected.researching.at(player)==p.before.researching.at(player),"Typed research action changed another player");}
    const auto first=p.output/"acted-first.CivisSave",resaved=p.output/"acted-resave.CivisSave";
    clean_outputs(p.output,{});const auto prepared=p.save_owner->prepare(receipt);
    only_research_sections_changed(p.original,prepared);
    p.save_owner->write_new(first,receipt);same_document(prepared,saves::read_civis_save(first));
    clean_outputs(p.output,{first.filename()});
    const auto input_first=runtime::identify_file(first);BnwResearchChoiceRestartReport report;report.atomic_writes=1;
    {
        auto fresh=p.installed_base_unit_art
            ? ui::bootstrap_selected_bnw_preview_with_installed_base_unit_art(first,p.game_root,expected.active)
            : ui::bootstrap_selected_bnw_preview(first,p.game_root,expected.active);
        need(fresh.preview.host&&!fresh.preview.blocker,"Normal first restart loader rejected actual acted checkpoint");
        ui::SelectedBnwReadOnlyPreview restarted(std::move(fresh.preview));const auto next=actual_host(restarted);
        need(!same_control(next,host)&&!same_control(next->session().read_state_lease(),host->session().read_state_lease()),"Restart reused original client Host/state");
        require_new_restart_frame(restarted,p.registry,p.viewport,p.camera,p.original_frame_owner,expected.active,receipt->queue());
        same_research(expected,observe(*next));need(!next->session().readiness().can_continue(),"Partial research checkpoint falsely grants full continuation");
        auto resaver=runtime::BnwResearchQueueSaveOwner::from_selected_preview(restarted,first);
        const auto again=resaver->prepare();same_document(prepared,again);resaver->write_new(resaved);
        same_document(again,saves::read_civis_save(resaved));++report.atomic_writes;++report.normal_loads;
    }
    clean_outputs(p.output,{first.filename(),resaved.filename()});
    {
        auto fresh=p.installed_base_unit_art
            ? ui::bootstrap_selected_bnw_preview_with_installed_base_unit_art(resaved,p.game_root,expected.active)
            : ui::bootstrap_selected_bnw_preview(resaved,p.game_root,expected.active);
        need(fresh.preview.host&&!fresh.preview.blocker,"Normal second restart loader rejected research re-save");
        ui::SelectedBnwReadOnlyPreview restarted(std::move(fresh.preview));const auto next=actual_host(restarted);
        require_new_restart_frame(restarted,p.registry,p.viewport,p.camera,p.original_frame_owner,expected.active,receipt->queue());
        same_research(expected,observe(*next));need(!next->session().readiness().can_continue(),"Re-save falsely grants full continuation");
        auto resaver=runtime::BnwResearchQueueSaveOwner::from_selected_preview(restarted,resaved);
        same_document(prepared,resaver->prepare());++report.normal_loads;
    }
    need(same_file(runtime::identify_file(first),input_first)&&same_file(runtime::identify_file(p.input),p.input_pin),"Original/input checkpoint changed during resave");
    same_research(expected,observe(*host));
    // Produced only after authentic action, presentation, writer and both normal
    // loader checks have completed. It is measured data, not a synthetic input
    // permission. Root pins this producer/process and each reference byte.
    BnwResearchRestartMeasurement reference;
    for(const auto& [key,value]:projection(p.before))reference.emplace("before."+key,value);
    for(const auto& [key,value]:projection(expected))reference.emplace("after."+key,value);
    reference["meta.actor"]=std::to_string(receipt->player());
    reference["meta.bootstrap"]=p.installed_base_unit_art?"installed-base-unit-art-v1":"legacy-plain-v1";
    reference["meta.original.bytes"]=std::to_string(p.input_pin.bytes);reference["meta.original.sha256"]=p.input_pin.sha256;
    const auto first_pin=runtime::identify_file(first),resave_pin=runtime::identify_file(resaved);
    reference["meta.first.bytes"]=std::to_string(first_pin.bytes);reference["meta.first.sha256"]=first_pin.sha256;
    reference["meta.resave.bytes"]=std::to_string(resave_pin.bytes);reference["meta.resave.sha256"]=resave_pin.sha256;
    reference["meta.atomic_writes"]=std::to_string(report.atomic_writes);reference["meta.normal_loads"]=std::to_string(report.normal_loads);
    reference["meta.gui_source_outcome"]="complete";reference["meta.full_continuation"]="false";reference["meta.original_bnw_parity"]="false";
    reference["meta.document.version"]=std::to_string(prepared.version);reference["meta.document.flags"]=std::to_string(prepared.flags);
    for(const auto& section:prepared.sections){const auto prefix="retained."+std::to_string(section.id)+".";
        reference[prefix+"flags"]=std::to_string(section.flags);reference[prefix+"bytes"]=std::to_string(section.bytes.size());
        reference[prefix+"sha256"]=runtime::identify_bytes(section.bytes).sha256;}
    const auto reference_path=p.output/"research-restart-reference.txt";
    write_new_bnw_research_measurement(reference_path,reference);
    clean_outputs(p.output,{first.filename(),resaved.filename(),reference_path.filename()});
    need(same_file(runtime::identify_file(p.input),p.input_pin)&&same_file(runtime::identify_file(first),first_pin)&&
        same_file(runtime::identify_file(resaved),resave_pin),"Source/checkpoint changed while recording real measurements");
    p.first_receipt=std::move(receipt);p.exported=true;
    report.retained_sections=prepared.sections.size();report.partial_research_preserved=true;return report;
}
void BnwResearchChoiceRestartFixture::verify_failed_source_refuses_export(
 const ui::SelectedBnwReadOnlyPreview& preview,const ui::BnwAlphaProgressionBackend& backend){
    auto& p=*impl_;(void)p.require_actual(preview);need(!p.exported&&backend.phase()==ui::BnwAlphaResearchUiPhase::uncertain,
        "Failure fixture needs actual uncertain source/UI outcome before export");
    const auto receipt=backend.last_commit();const auto path=p.output/"failed-source-rejected.CivisSave";
    need(!std::filesystem::exists(path),"Failure refusal path already exists");bool rejected=false;
    try{p.require_completed(preview,backend,receipt);p.save_owner->write_new(path,receipt);}catch(const std::exception&){rejected=true;}
    need(rejected&&!std::filesystem::exists(path),"Uncertain source/UI outcome wrote a checkpoint");
    need(same_file(runtime::identify_file(p.input),p.input_pin),"Source failure changed original save");clean_outputs(p.output,{});
}
void BnwResearchChoiceRestartFixture::verify_stale_owner_refuses_after_next_actual_choice(
 const ui::SelectedBnwReadOnlyPreview& preview,std::shared_ptr<const runtime::BnwResearchChoiceCommit> next){
    auto& p=*impl_;const auto host=p.require_actual(preview);need(p.exported&&p.first_receipt&&next,
        "Stale fixture needs actual first export and second real action");
    need(next->phase()==runtime::BnwResearchChoicePhase::complete&&next->after_revision()>p.first_receipt->after_revision(),"Second actual choice did not advance research source");
    next->require_stored_current(*host);const auto before=observe(*host);const auto path=p.output/"stale-rejected.CivisSave";
    need(!std::filesystem::exists(path),"Stale refusal output already exists");bool rejected=false;
    try{p.save_owner->write_new(path,p.first_receipt);}catch(const std::exception&){rejected=true;}
    need(rejected&&!std::filesystem::exists(path),"Stale SaveOwner/receipt wrote an output");same_research(before,observe(*host));
    need(same_file(runtime::identify_file(p.input),p.input_pin),"Stale refusal changed original save");
    clean_outputs(p.output,{std::filesystem::path{"acted-first.CivisSave"},std::filesystem::path{"acted-resave.CivisSave"},std::filesystem::path{"research-restart-reference.txt"}});
}
}
