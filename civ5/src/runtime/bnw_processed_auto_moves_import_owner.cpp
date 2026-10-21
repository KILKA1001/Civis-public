#include "civ5/runtime/bnw_processed_auto_moves_import_owner.hpp"
#include "civ5/runtime/bnw_end_turn_request.hpp"
#include "civ5/runtime/detail/bnw_dynamic_turn_mode.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <thread>
namespace civ5::runtime {
namespace {void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}}
struct BnwProcessedAutoMovesImportOwner::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const ui::SelectedBnwPreviewHost>host;
 std::weak_ptr<const LiveSessionState>world;
 BnwGameActivationOwner*owner=nullptr;const saves::SavedSession*source=nullptr;
 std::uint64_t generation=0,sequence=0;SessionCheckpointIdentity identity;
 std::array<const bool*,64>nodes{};std::array<bool,64>expected{};
 explicit Impl(std::shared_ptr<const ui::SelectedBnwPreviewHost>h):host(h),world(h->session().read_state_lease()),
  owner(&const_cast<ui::SelectedBnwPreviewHost&>(*h).activation_owner()),source(&h->session().source()),
  generation(h->session().state_generation()),identity(h->identity()) {
  need(source->source.container==saves::SaveContainerKind::civ5,"ProcessedAutoMoves cannot reset a Civis continuation");
  // SDK CvPlayer ctor404/reset953 initializes this plain unsaved member;
  // Read22035 loads AutoMoves but never overwrites ProcessedAutoMoves. The
  // existing original-import binder performs that reset ON THE SAME owner.
  const auto binding=bind_bnw_end_turn_import(*source,*owner);
  const auto w=world.lock();need(w&&binding.map_guid()==w->world.map_header.guid&&
   binding.identity().payload_sha256==identity.payload_sha256&&binding.identity().rules_sha256==identity.rules_sha256,
   "ProcessedAutoMoves genuine original import binding differs");
  for(std::int32_t n=0;n<64;++n){auto&v=owner->processed_auto_moves.at(n);nodes[n]=&v;expected[n]=v;}
 }
 void thread()const{need(creator==std::this_thread::get_id(),"ProcessedAutoMoves foreign immutable creator");}
};
BnwProcessedAutoMovesImportOwner::BnwProcessedAutoMovesImportOwner(std::shared_ptr<const ui::SelectedBnwPreviewHost>h):impl_(std::make_unique<Impl>(std::move(h))){}
BnwProcessedAutoMovesImportOwner::~BnwProcessedAutoMovesImportOwner()=default;
void BnwProcessedAutoMovesImportOwner::require_host_control_without_callbacks(const std::shared_ptr<const ui::SelectedBnwPreviewHost>&supplied)const{
 impl_->thread();const auto actual=impl_->host.lock();need(actual&&supplied&&actual.get()==supplied.get()&&
  !actual.owner_before(supplied)&&!supplied.owner_before(actual),"ProcessedAutoMoves genuine shared Host control differs");
}
void BnwProcessedAutoMovesImportOwner::require_current_without_callbacks(const ui::SelectedBnwPreviewHost&h,const LiveSessionState&s,std::uint64_t generation)const{
 impl_->thread();const auto&r=*impl_;const auto host=r.host.lock();const auto world=r.world.lock();
 need(host&&host.get()==&h&&world&&world.get()==&s&&h.session().read_state_lease().get()==&s&&
  h.session().state_generation()==generation&&r.generation==generation&&&h.session().source()==r.source&&
  r.source->source.container==saves::SaveContainerKind::civ5&&h.bnw_processed_auto_moves_import_.get()==this&&&h.activation_owner()==r.owner&&
  h.identity().payload_sha256==r.identity.payload_sha256&&h.identity().rules_sha256==r.identity.rules_sha256&&
  r.owner->identity.payload_sha256==r.identity.payload_sha256&&r.owner->identity.rules_sha256==r.identity.rules_sha256&&
  r.owner->processed_auto_moves.size()==64&&s.world.all_players&&s.world.all_players->players.size()==64,
  "ProcessedAutoMoves actual Host/world/generation/OriginalRead owner retired");
 for(std::int32_t n=0;n<64;++n){const auto at=r.owner->processed_auto_moves.find(n);
  need(at!=r.owner->processed_auto_moves.end()&&&at->second==r.nodes[n]&&at->second==r.expected[n]&&
   s.world.all_players->players[n].saved_player_id==n&&detail::dynamic_core(s.world.all_players->players[n],"m_eID").values.at(0)==n,
   "ProcessedAutoMoves canonical64 map node/player or sealed mutation history changed");}
}
bool BnwProcessedAutoMovesImportOwner::value_without_callbacks(std::int32_t actor)const{
 impl_->thread();need(actor>=0&&actor<64,"ProcessedAutoMoves actor outside64");return impl_->expected[actor];
}
std::uint64_t BnwProcessedAutoMovesImportOwner::sequence_without_callbacks()const{impl_->thread();return impl_->sequence;}
void BnwProcessedAutoMovesImportOwner::require_pristine_checkpoint_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,std::uint64_t generation)const{
 require_current_without_callbacks(host,state,generation);need(impl_->sequence==0,"ProcessedAutoMoves stores require a durable transient codec");
 for(const auto value:impl_->expected)need(!value,"ProcessedAutoMoves non-pristine native flag lacks checkpoint codec");
 // This only permits omission of unchanged original reset defaults from a
 // partial import checkpoint. A restored Civis Host still has no seal and
 // cannot execute this native continuation without an actual owner codec.
}
void BnwProcessedAutoMovesImportOwner::store_false_without_callbacks(std::int32_t actor)noexcept{
 if(impl_->creator!=std::this_thread::get_id()||actor<0||actor>=64||!impl_->nodes[actor]||impl_->sequence==UINT64_MAX)std::terminate();
 // LAST caller's pure seal proved the exact writable node. Native setAutoMoves
 // changed branch unconditionally assigns ProcessedAutoMoves=false, even when
 // it was already false. No callback/allocation/map insertion in this tail.
 *const_cast<bool*>(impl_->nodes[actor])=false;impl_->expected[actor]=false;++impl_->sequence;
}
}
