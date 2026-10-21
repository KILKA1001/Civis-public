#include "civ5/runtime/bnw_selected_map_init.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool b,const char* why){if(!b)throw ScriptError(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Map init DB value exceeds native int32");return static_cast<std::int32_t>(n);}
bool same(const assets::AssetCandidate& a,const assets::AssetCandidate& b){
 return a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&a.payload_size==b.payload_size&&
  a.provenance.kind==b.provenance.kind&&a.provenance.source_id==b.provenance.source_id&&
  a.provenance.source_relative_path==b.provenance.source_relative_path&&
  a.provenance.archive_relative_path==b.provenance.archive_relative_path;
}
bool same(const assets::MountedResource& a,const assets::MountedResource& b){
 if(a.mount_id!=b.mount_id||a.asset.index()!=b.asset.index())return false;
 if(const auto* h=std::get_if<assets::AssetHandle>(&a.asset))return same(h->candidate(),std::get<assets::AssetHandle>(b.asset).candidate());
 if(const auto* x=std::get_if<assets::AmbiguousAsset>(&a.asset)){
  const auto& y=std::get<assets::AmbiguousAsset>(b.asset);
  if(x->requested_key.normalized_name()!=y.requested_key.normalized_name()||x->matches.size()!=y.matches.size())return false;
  for(std::size_t i=0;i<x->matches.size();++i)if(!same(x->matches[i],y.matches[i]))return false;
 }
 return true;
}
}
BnwSelectedMapSourceOwner::BnwSelectedMapSourceOwner(
 std::shared_ptr<const BnwFreshGameContentResult> parent,assets::AssetHandle root,
 std::vector<std::pair<std::string,assets::AssetHandle>> includes,
 const assets::ResourceMountTable& resources,const BnwMapSetupContext& setup,const SimulationRandom& random,std::uint64_t limit,
 std::shared_ptr<BnwFreshGameContentResult::ContentEpoch> epoch)
 :parent_(std::move(parent)),epoch_(std::move(epoch)),root_{std::move(root),{}},resources_(&resources),setup_(&setup),random_(&random),
  setup_before_(setup),source_limit_(limit){
 need(bool(epoch_),"Selected map requires actual content epoch");epoch_->require_current();
 need(parent_&&parent_->content_ready()&&parent_->rules,"Selected map content rules unavailable");
 database_=parent_->rules.get();
 if(!database_->immutable_preview_fingerprints_current())database_->seal_immutable_preview_fingerprints();
 rules_current_=database_->immutable_preview_fingerprint_currentness();
 need(rules_current_&&rules_current_(),"Selected map immutable rules control unavailable");
 rules_sha_=database_->gameplay_fingerprint_sha256();
 root_.sha=identify_bytes(root_.handle.read_bytes(source_limit_)).sha256;
 root_selection_=resources.resolve_all(root_.handle.requested_key());
 std::size_t root_matches=0;
 for(const auto& hit:root_selection_)if(const auto* h=std::get_if<assets::AssetHandle>(&hit.asset))
  if(same(h->candidate(),root_.handle.candidate()))++root_matches;
 need(root_matches==1,"Selected map root is not uniquely present in actual VFS selection");
 for(auto& [name,h]:includes){include_selection_.push_back(resources.resolve(assets::AssetKey(name)));include_names_.push_back(std::move(name));includes_.push_back({std::move(h),{}});}
 require_current("map",nullptr);
}
BnwSelectedMapSourceOwner::~BnwSelectedMapSourceOwner(){retire();}
void BnwSelectedMapSourceOwner::retire()noexcept{retired_.store(true);}
void BnwSelectedMapSourceOwner::require_current(std::string_view context,const ScriptUiChildSource* child)const{
 need(std::this_thread::get_id()==thread_,"Selected map source requires constructor thread");
 need(!retired_.load()&&context=="map"&&!child,"Selected map source retired or foreign context");
 epoch_->require_current();
 need(!checking_,"Selected map source validation is reentrant");checking_=true;
 struct Reset{bool& value;~Reset(){value=false;}} reset{checking_};
 need(setup_&&random_&&setup_->selected_rules==parent_.get()&&
  setup_->world_size==setup_before_.world_size&&setup_->custom_options==setup_before_.custom_options&&
  setup_->climate==setup_before_.climate&&setup_->graphics_initialized==setup_before_.graphics_initialized&&
  setup_->runtime_defines.empty()&&random_->algorithm()==RandomAlgorithm::bnw,
  "Selected map provider job setup or RNG binding changed");
 need(parent_&&parent_->rules.get()==database_&&parent_->content_ready()&&
  rules_current_&&rules_current_()&&database_->immutable_preview_fingerprints_current()&&database_->gameplay_fingerprint_sha256()==rules_sha_,
  "Selected map rules owner changed");
 need(resources_!=nullptr,"Selected map VFS job retired");
 const auto root_now=resources_->resolve_all(root_.handle.requested_key());
 need(root_now.size()==root_selection_.size(),"Selected map VFS root selection changed");
 for(std::size_t i=0;i<root_now.size();++i){
  need(same(root_now[i],root_selection_[i]),"Selected map VFS root mount or candidate changed");
  if(const auto* h=std::get_if<assets::AssetHandle>(&root_now[i].asset))
   if(same(h->candidate(),root_.handle.candidate()))
    need(identify_bytes(h->read_bytes(source_limit_)).sha256==root_.sha,"Selected map current VFS root bytes changed");
 }
 for(std::size_t i=0;i<include_names_.size();++i){
  const auto now=resources_->resolve(assets::AssetKey(include_names_[i]));
  need(same(now,include_selection_[i]),"Selected map VFS include selection changed");
  const auto* h=std::get_if<assets::AssetHandle>(&now.asset);
  need(h&&same(h->candidate(),includes_[i].handle.candidate()),"Selected map VFS include descriptor differs");
  if(!includes_[i].sha.empty())need(identify_bytes(h->read_bytes(source_limit_)).sha256==includes_[i].sha,"Selected map current VFS include bytes changed");
 }
 const auto check=[&](const Pin& p){if(!p.sha.empty())need(identify_bytes(p.handle.read_bytes(source_limit_)).sha256==p.sha,"Selected map VFS source bytes changed");};
 check(root_);for(const auto& p:includes_)check(p);
 // VFS lookup observers may revoke/replace the public parent while resolving.
 // Reconcile the facade BEFORE borrowing it again; retained control owns the
 // original SQLite connection and cannot revive a different immutable scope.
 try{epoch_->require_current();}catch(const std::exception& error){throw ScriptError(std::string("Selected map rules parent changed during VFS lookup: ")+error.what());}
 need(!retired_.load()&&parent_&&parent_->rules.get()==database_&&parent_->content_ready()&&
  rules_current_&&rules_current_(),"Selected map rules parent changed during VFS lookup");
 need(database_->immutable_preview_fingerprints_current()&&database_->gameplay_fingerprint_sha256()==rules_sha_,
  "Selected map source changed during read");
}
std::shared_ptr<const ScriptUiChildSource> BnwSelectedMapSourceOwner::resolve_child(
 std::string_view parent,const ScriptUiChildSource* source,std::string_view){
 require_current(parent,source);
 throw ScriptError("Selected map LoadNewContext has no issued UI child; include uses the existing map source loader");
}
void BnwSelectedMapSourceOwner::require_same_provider_operands(
 const assets::ResourceMountTable& resources,const BnwMapSetupContext& setup,const SimulationRandom& random)const{
 require_current("map",nullptr);
 need(resources_==&resources&&setup_==&setup&&random_==&random,
  "Selected map retained provider operands substituted");
}
void BnwSelectedMapSourceOwner::begin_load(std::string_view bytes){
 require_current("map",nullptr);need(!attempted_,"Selected map constructor already attempted");
 need(identify_bytes(std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size())).sha256==root_.sha,
  "Selected map captured root buffer differs from VFS source");attempted_=true;
}
void BnwSelectedMapSourceOwner::require_include_bytes(std::string_view name,std::span<const std::uint8_t> bytes){
 require_current("map",nullptr);
 for(std::size_t i=0;i<include_names_.size();++i)if(include_names_[i]==name){
  const auto captured=identify_bytes(bytes).sha256;
  need(identify_bytes(includes_[i].handle.read_bytes(source_limit_)).sha256==captured,
   "Selected map captured include buffer differs from actual VFS source");
  if(includes_[i].sha.empty())includes_[i].sha=captured;
  else need(captured==includes_[i].sha,"Selected map captured include buffer differs from pinned source");
  require_current("map",nullptr);return;
 }
 throw ScriptError("Selected map include has no actual source descriptor");
}
void BnwSelectedMapSourceOwner::loaded(std::string_view bytes){
 require_current("map",nullptr);need(attempted_&&!loaded_,"Selected map constructor completion is out of order");
 need(identify_bytes(std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size())).sha256==root_.sha,
  "Selected map executed buffer differs from actual VFS source");loaded_=true;
}
ScriptMapInitQueryResult BnwSelectedMapSourceOwner::query(LuaScriptHost& host,std::int32_t world){
 require_current("map",nullptr);need(loaded_&&!queried_,"Selected map init query lacks an unused actual constructor");
 queried_=true;
 try{
  ScriptMapInitFields initial;initial.wrap_x=true;initial.wrap_y=false;
  initial.top_latitude=90;initial.bottom_latitude=-90;
  bool found=false;
  const auto report=database_->query_rows({"SELECT * from Worlds where ID = ? LIMIT 1",{static_cast<std::int64_t>(world)}},[&](const auto& row){
   need(!found,"Native Worlds LIMIT1 returned multiple rows");found=true;
   need(row.integer("ID")==world,"Selected map Worlds ID differs");
   initial.width=native(row.integer("GridWidth"));initial.height=native(row.integer("GridHeight"));
  });
  need(report.all_succeeded()&&found,"Selected map actual WorldInfo default row unavailable");
  require_current("map",nullptr);
  const auto result=host.query_map_init_data("map",*this,world,initial);
  require_current("map",nullptr);return result;
 }catch(...){retire();throw;}
}
}
