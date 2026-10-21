#include "civ5/ui/bnw_owned_camera_ui_source_input.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <stdexcept>
namespace civ5::ui {
struct BnwOwnedCameraUiSourceInput::Impl {
 std::shared_ptr<const SelectedBnwPreviewHost> parent;
 const std::thread::id thread=std::this_thread::get_id();
 const std::shared_ptr<const assets::CivisResourceBankOwner> civis_bank;
 std::shared_ptr<const runtime::LiveSessionState> state;
 std::uint64_t generation=0;
 LiveBnwPreviewInput input;
 std::weak_ptr<BnwOwnedCameraUiSourceInput> facade;
 Impl(std::shared_ptr<const SelectedBnwPreviewHost> owner,LiveBnwPreviewInput&& value)
  :parent(std::move(owner)),civis_bank(parent&&parent->uses_civis_resource_bank()?parent->civis_resource_bank_:nullptr),
   state(parent?parent->session().read_state_lease():nullptr),
   generation(parent?parent->session().state_generation():0),input(std::move(value)){}
 void finality()const{
  if(thread!=std::this_thread::get_id())throw std::runtime_error("Owned camera input foreign thread");
  if(facade.expired()||!parent||(!parent->early_package_&&!civis_bank))
   throw std::runtime_error("Owned camera input facade expired or genuine resource parent absent");
  if(civis_bank){
   if(parent->early_package_||parent->completed_resources_||parent->completed_observation_||
      parent->civis_resource_bank_.get()!=civis_bank.get()||
      parent->civis_resource_bank_.owner_before(civis_bank)||
      civis_bank.owner_before(parent->civis_resource_bank_))
    throw std::runtime_error("Owned camera input genuine Civis bank control changed or mixed native lineage");
   civis_bank->require_current();
  }
  const auto& session=parent->session();
  if(&session!=&input.session||parent->active_player_id()!=input.active_player_id||
     session.read_state_lease()!=state||session.state_generation()!=generation||
     !input.scene_identity||input.scene_identity->payload_sha256!=parent->identity().payload_sha256||
     input.scene_identity->rules_sha256!=parent->identity().rules_sha256||
     input.frame_sources.database!=&parent->database()||
     input.frame_sources.technologies!=&parent->technologies()||
     &input.production_rules!=&parent->production_rules()||
     input.pregame_roster!=&parent->pregame_roster()||input.activation_owner!=&parent->activation_owner())
   throw std::runtime_error("Owned camera input parent/allocation/identity replaced");
 }
 void current()const{
  finality(); // Known expired facade/allocation refuses BEFORE native callbacks.
  if(civis_bank){
   civis_bank->require_boundary();
   finality();
   parent->require_same_civis_resources(*civis_bank);
   civis_bank->require_boundary();
  }else{
   parent->require_same_early_package(*parent->early_package_);
  }
  finality(); // Only retained Impl is read after callback; no facade immortality.
 }
};
BnwOwnedCameraUiSourceInput::BnwOwnedCameraUiSourceInput(
 std::shared_ptr<const SelectedBnwPreviewHost> parent,LiveBnwPreviewInput&& input)
 :impl_(std::make_shared<Impl>(std::move(parent),std::move(input))){}
std::shared_ptr<BnwOwnedCameraUiSourceInput> BnwOwnedCameraUiSourceInput::retain_owned_ui_owner(
 const std::shared_ptr<BnwOwnedCameraUiSourceInput>& supplied)const{
 const auto i=impl_;
 if(i->thread!=std::this_thread::get_id())throw std::runtime_error("Owned camera input owner foreign thread");
 const auto actual=i->facade.lock();
 if(!actual||actual.get()!=this||supplied.get()!=this||actual.owner_before(supplied)||supplied.owner_before(actual))
  throw std::runtime_error("Owned camera input canonical owner differs");
 return actual;
}
void BnwOwnedCameraUiSourceInput::require_current()const{const auto i=impl_;i->current();}
void BnwOwnedCameraUiSourceInput::require_finality()const{const auto i=impl_;i->finality();}
const LiveBnwPreviewInput& BnwOwnedCameraUiSourceInput::final_input()const{
 const auto i=impl_;i->finality();return i->input;
}
const LiveBnwPreviewInput& BnwOwnedCameraUiSourceInput::input()const{
 const auto i=impl_;i->current();return i->input;
}
const std::shared_ptr<const SelectedBnwPreviewHost>& BnwOwnedCameraUiSourceInput::parent()const{
 const auto i=impl_;i->current();return i->parent;
}
void BnwOwnedCameraUiSourceInput::install_checked_launch_input(LiveBnwPreviewInput&& input){
 const auto i=impl_;i->current();
 if(&input.session!=&i->input.session||input.active_player_id!=i->input.active_player_id||
    !input.scene_identity||!i->input.scene_identity||
    input.scene_identity->payload_sha256!=i->input.scene_identity->payload_sha256||
    input.scene_identity->rules_sha256!=i->input.scene_identity->rules_sha256)
  throw std::runtime_error("Owned camera checked launch input substituted");
 i->input.frame_sources=std::move(input.frame_sources);i->current();
}
void SelectedBnwReadOnlyPreview::with_owned_camera_ui_source_input(
 const std::function<void(std::shared_ptr<BnwOwnedCameraUiSourceInput>)>& render)const{
 if(!render)throw std::invalid_argument("Owned camera input renderer absent");
 const auto parent=owner_; // Genuine parent BEFORE callable target copy.
 const auto retained_core=restored_mission_core_;
 if(!parent)throw std::runtime_error("Owned camera preview facade absent");
 // This pure Host route check authenticates its immutable creator before any
 // bank/session copy. The returned bool grants no render or source authority.
 (void)parent->uses_civis_resource_bank();
 const auto state=parent->session().read_state_lease();const auto generation=parent->session().state_generation();
 const auto callback=render;
 if(parent->session().read_state_lease()!=state||parent->session().state_generation()!=generation)
  throw std::runtime_error("Owned camera preview source replaced during callback copy");
 auto input=parent->preview_input();
 if(parent->session().read_state_lease()!=state||parent->session().state_generation()!=generation)
  throw std::runtime_error("Owned camera preview source replaced during input construction");
 const auto source=std::shared_ptr<BnwOwnedCameraUiSourceInput>(
     new BnwOwnedCameraUiSourceInput(parent,std::move(input)));
 // Silent construction cannot query native sources before the actual mint
 // installs its canonical weak-facade record. This is the sole private issuer.
 source->impl_->facade=source;
 source->require_current();callback(source);source->require_current();
 (void)retained_core;
}
}
