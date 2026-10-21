#include "civ5/runtime/bnw_treasury_same_vm_income_loan.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/live_session.hpp"
#include <stdexcept>
#include <utility>
namespace civ5::runtime {
namespace {void income_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
BnwTreasurySameVmIncomeLoan::BnwTreasurySameVmIncomeLoan(
 BnwSelectedStockLuaBootstrap& vm,BnwStockLuaAttemptLease lease,
 std::shared_ptr<const LiveSessionState> world,std::int64_t actor)
 :vm_(&vm),lease_(std::move(lease)),world_(std::move(world)),actor_(actor),
  serial_(lease_.operation_serial()),revision_(world_?world_->ui_source_revision:0){
 income_need(world_&&actor_>=0&&actor_<64,"Treasury actual income issuer/world/actor missing");
 require_current(*world_);
}
void BnwTreasurySameVmIncomeLoan::require_current(const LiveSessionState& supplied)const{
 // Thread first, then genuine active Source currentness before raw VM borrow.
 income_need(creator_==std::this_thread::get_id(),"Treasury income foreign thread");
 income_need(!failed_,"Treasury income query quarantined");
 lease_.require_operation(serial_);const auto actual=lease_.world_lease();
 income_need(actual&&actual.get()==world_.get()&&!actual.owner_before(world_)&&
  !world_.owner_before(actual)&& &supplied==world_.get(),
  "Treasury income query requires SAME canonical world; detached state refused");
 income_need(world_->ui_source_revision==revision_,"Treasury income world revision changed");
 vm_->require_prepared_owned();vm_->require_healthy_owned();
 lease_.require_operation(serial_);
 income_need(world_->ui_source_revision==revision_,"Treasury income preparation changed world");
}
std::optional<std::int64_t> BnwTreasurySameVmIncomeLoan::founder_benefits_read_only(
 std::int64_t actor,const LiveSessionState& world)const{
 income_need(creator_==std::this_thread::get_id(),"Treasury income foreign thread");
 income_need(!querying_,"Treasury income query reentry");
 income_need(actor==actor_,"Treasury income actor substituted");
 try{
  require_current(world);
  // Reserve before real VM callbacks. Error never becomes native fallback.
  querying_=true;
  struct Reset{bool& value;~Reset(){value=false;}} reset{querying_};
  const auto map_before=world_->random.map.state(),game_before=world_->random.gameplay.state();
  const auto turn_before=world_->setup.game_turn;
  const auto actual=vm_->accumulate_engine_founder_owned(static_cast<std::int32_t>(actor_),lease_);
  income_need(actual.errors.empty(),"Treasury actual founder accumulator failed");
  require_current(world);
  income_need(world_->random.map.state()==map_before&&world_->random.gameplay.state()==game_before&&
   world_->setup.game_turn==turn_before,"Treasury observational income query changed RNG/clock");
  // NONE is the actual CallAccumulator no-override result, not a made-up ACK.
  return actual.value;
 }catch(...){failed_=true;throw;}
}
}
