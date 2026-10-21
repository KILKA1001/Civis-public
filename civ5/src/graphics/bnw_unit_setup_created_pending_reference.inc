#include "civ5/runtime/lua_script_host.hpp"
namespace civ5::graphics {
class BnwUnitSetupGraphicalQueue::PendingReadReference final:public runtime::ScriptUnitReadReference {
 friend class CreatedOccurrenceLoan;
 const std::shared_ptr<PendingReadLifetime>life_;
 explicit PendingReadReference(std::shared_ptr<PendingReadLifetime>l):life_(std::move(l)){}
 public:
 void require_current()const override{life_->current();}
 runtime::ScriptValue read(std::string_view method)const override{
  life_->current();
  if(method=="IsDead"||method=="IsCombatUnit"||method=="IsEmbarked"||method=="IsTrade"||method=="IsGarrisoned"||method=="IsCargo"){
   const bool result=life_->loan->read_pending_boolean_owned(method);life_->current();return result;
  }
  const auto result=life_->loan->read_pending_integer_owned(method);life_->current();return std::int64_t(result);
 }
};
std::shared_ptr<const runtime::ScriptUnitReadReference>
BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::retain_pending_unit_reference_owned()const{
 require_current();
 try{
  setup_need(impl_->reference_life&&impl_->reference_life->alive.load()&&impl_->reference_life->loan==this,
   "Created Unit reference SAME lifetime issuer missing");
  auto result=std::shared_ptr<const PendingReadReference>(new PendingReadReference(impl_->reference_life));
  result->require_current();require_origin_without_callbacks();require_current();return result;
 }catch(...){impl_->poison();throw;}
}
}
