#include "civ5/runtime/bnw_research_queue_save_owner.hpp"
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <stdexcept>
#include <set>
#include <thread>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool same(const FileIdentity& a,const FileIdentity& b){return a.bytes==b.bytes&&a.sha256==b.sha256;}
bool same_part(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){
 return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;
}
// Order can differ in a container, but IDs must be unique and EVERY byte/flag
// must be represented by the selected immutable input. Never discard unknowns.
void same_document(const saves::CivisSaveDocument& a,const saves::CivisSaveDocument& b){
 need(a.version==b.version&&a.flags==b.flags&&a.sections.size()==b.sections.size(),
      "BNW research save input has unsupported or unrepresented sections");
 std::set<std::uint32_t> seen;
 for(const auto& part:a.sections){
  need(seen.insert(part.id).second,"BNW research save duplicate input section");
  const auto it=std::find_if(b.sections.begin(),b.sections.end(),[&](const auto& p){return p.id==part.id;});
  need(it!=b.sections.end()&&same_part(part,*it),"BNW research save input differs from selected source");
 }
}
void no_overlap(const saves::LoadedSave& source){
 need(!source.live_bnw_team_progress&&!source.live_research_selection&&
      !source.live_bnw_player_knowledge&&!source.live_bnw_player_administration&&
      !source.live_bnw_team_turn&&source.profile_sections.empty(),
      "BNW research save requires unavailable overlapping owner reconciliation");
 need(source.live_bnw_research_queue.has_value() ||
      (!source.live_team_research&&!source.live_research_turn),
      "BNW research save cannot replace another research owner section");
}
std::string extension(const std::filesystem::path& p){
 auto s=p.extension().string();std::transform(s.begin(),s.end(),s.begin(),
  [](unsigned char c){return static_cast<char>(std::tolower(c));});return s;
}
}
struct BnwResearchQueueSaveOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false},busy{false};
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 std::unique_ptr<const LiveSessionState> baseline;
 const saves::SavedSession* source=nullptr;
 SessionCheckpointIdentity identity;std::uint64_t generation=0;
 std::filesystem::path input;FileIdentity input_pin;
 saves::CivisSaveDocument original_document;
 void check()const{
  need(thread==std::this_thread::get_id(),"BNW research save belongs to another thread");
  need(!retired.load()&&host&&state&&baseline,"BNW research save owner is retired");
  const auto& s=host->session();
  need(&s.source()==source&&s.state_generation()==generation&&s.read_state_lease()==state,
       "BNW research save source or live allocation is stale");
  need(host->identity().payload_sha256==identity.payload_sha256&&
       host->identity().rules_sha256==identity.rules_sha256&&
       host->database().gameplay_fingerprint_sha256()==identity.rules_sha256&&
       identify_bytes(source->source.bytes).sha256==identity.payload_sha256,
       "BNW research save payload or rules identity changed");
  need(same(identify_file(input),input_pin),"BNW research save input file identity changed");
  (void)s.bnw_game_debug_mode();
  need(!s.has_bnw_operation_checkpoint_history()&&!state->player_gold_publication&&
       !state->player_research_publication&&!state->player_early_publication&&
       !state->game_clock.pending&&state->city_dirty_events.empty()&&
       state->plot_gameplay_events.empty()&&state->city_strategy_runtime.empty(),
       "BNW research save lacks pending effect or once-operation journal codecs");
  // This component preserves the input RNG section, not a new RNG state.
  const auto same_rng=[](const auto& a,const auto& b){
   return a.algorithm()==b.algorithm()&&a.state().seed==b.state().seed&&
          a.state().calls==b.state().calls&&a.state().resets==b.state().resets;
  };
  need(same_rng(state->random.map,baseline->random.map)&&
       same_rng(state->random.gameplay,baseline->random.gameplay)&&
       state->game_clock.sent_auto_moves==baseline->game_clock.sent_auto_moves&&
       state->city_dirty_next_occurrence==baseline->city_dirty_next_occurrence&&
       state->plot_gameplay_next_occurrence==baseline->plot_gameplay_next_occurrence,
       "BNW research save cannot preserve changed RNG/clock/event sections");
  need(s.state_generation()==generation&&s.read_state_lease()==state&&!retired.load(),
       "BNW research save changed during validation");
 }
 struct Guard{
  std::shared_ptr<Impl> p;
  explicit Guard(std::shared_ptr<Impl> q):p(std::move(q)){
   need(p&&p->thread==std::this_thread::get_id(),"BNW research save foreign thread");
   if(p->busy.exchange(true)){p->retired.store(true);throw std::runtime_error("BNW research save reentry");}
  }
  ~Guard(){p->busy.store(false);}
 };
};
BnwResearchQueueSaveOwner::BnwResearchQueueSaveOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwResearchQueueSaveOwner::~BnwResearchQueueSaveOwner()=default;
std::shared_ptr<BnwResearchQueueSaveOwner> BnwResearchQueueSaveOwner::from_selected_preview(
 const ui::SelectedBnwReadOnlyPreview& preview,const std::filesystem::path& actual_input){
 std::shared_ptr<BnwResearchQueueSaveOwner> result;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  need(static_cast<bool>(host),"BNW research save genuine selected Host is missing");
  auto p=std::make_shared<Impl>();p->host=std::move(host);p->input=actual_input;
  const auto& s=p->host->session();p->source=&s.source();p->state=s.read_state_lease();
  p->generation=s.state_generation();p->identity=p->host->identity();
  // Root-owned Host friend seam reads the actual factory-retained identity.
  // Original Civ5: measured compressed file SHA. Reopened Civis: validated
  // inherited source metadata; this does not prove the external original exists.
  const auto original_sha=p->host->original_source_sha256_;
  need(original_sha.size()==64&&std::all_of(original_sha.begin(),original_sha.end(),[](char v){
       return (v>='0'&&v<='9')||(v>='a'&&v<='f');}),
       "BNW research save lacks the genuine retained source identity");
  if(p->source->source.container==saves::SaveContainerKind::civis)
   need(p->source->source.source_sha256&&*p->source->source.source_sha256==original_sha,
        "BNW research save original source metadata differs from retained Host pin");
  no_overlap(p->source->source);
  p->input_pin=identify_file(actual_input);
  if(p->source->source.container==saves::SaveContainerKind::civ5)
   need(p->input_pin.sha256==original_sha,
        "BNW research save original compressed input differs from measured Host pin");
  const auto actual=saves::read_loaded_save(actual_input);
  // First prove every known imported byte, prefix, metadata and override is the
  // SAME selected source; then admit only complete reconstructible documents.
  auto represented=saves::make_civis_import_document(actual,p->input_pin.sha256);
  const auto selected=saves::make_civis_import_document(p->source->source,original_sha);
  same_document(represented,selected);
  if(actual.container==saves::SaveContainerKind::civis){
   p->original_document=saves::read_civis_save(actual_input);
   same_document(p->original_document,represented);
  }else p->original_document=std::move(represented);
  need(same(identify_file(actual_input),p->input_pin),"BNW research save input changed while loading");
  // One authentic comparison snapshot. No snapshot reconstruction at export.
  p->baseline=std::make_unique<const LiveSessionState>(*p->state);
  p->check();result=std::shared_ptr<BnwResearchQueueSaveOwner>(new BnwResearchQueueSaveOwner(std::move(p)));
 });
 need(static_cast<bool>(result),"BNW research save genuine source callback did not run");return result;
}
saves::CivisSaveDocument BnwResearchQueueSaveOwner::prepare(
 std::shared_ptr<const BnwResearchChoiceCommit> commit)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  p->check();
  if(commit){
   need(commit->phase()==BnwResearchChoicePhase::complete&&
        commit->before_revision()==p->baseline->ui_source_revision&&
        commit->after_revision()==p->state->ui_source_revision,
        "BNW research save requires the genuine completed SAME-state research tail");
   commit->require_stored_current(*p->host);
  }else need(p->state->ui_source_revision==p->baseline->ui_source_revision,
             "BNW research save changed revision needs actual completed research commit");
  auto bundle=detail::prepare_bnw_research_queue_bundle(*p->state,*p->baseline,
      p->source->snapshot,p->host->technologies(),p->host->types(),p->identity);
  auto document=p->original_document;
  if(bundle){
   auto& parts=document.sections;
   std::erase_if(parts,[](const auto& v){return v.id==33||v.id==8||v.id==11;});
   parts.push_back(std::move(bundle->raw));parts.push_back(std::move(bundle->research));
   parts.push_back(std::move(bundle->research_turn));
  }else need(!p->source->source.live_bnw_research_queue,
       "BNW research save zero-net ID33 removal needs companion-owner reconciliation");
  p->check();if(commit)commit->require_stored_current(*p->host);
  return document;
 }catch(...){p->retired.store(true);throw;}
}
void BnwResearchQueueSaveOwner::write_new(const std::filesystem::path& path,
 std::shared_ptr<const BnwResearchChoiceCommit> commit)const{
 // An exclusive sibling temporary is atomically published by the existing
 // writer. Original Civ5Save and preexisting output files are never overwritten.
 need(extension(path)==".civissave","BNW research checkpoint requires a new .CivisSave path");
 auto document=prepare(std::move(commit));
 const auto p=impl_;Impl::Guard guard(p);
 try{p->check();saves::write_civis_save(path,document);}catch(...){p->retired.store(true);throw;}
}
}
