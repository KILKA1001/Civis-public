// SAME Membership State and SAME native HP alias; retained unit Lua tables
// expire at the original member facade boundary. No ScriptRead.current loop.
namespace civ5::runtime {
std::pair<std::int32_t,std::int32_t>
BnwUnitFlagHealthInstanceLoan::owner_full_id_from_storage_without_callbacks(const std::shared_ptr<State>&s){
 require_current_from_storage_owned(s);return {s->owner,s->id};
}
class BnwUnitFlagHealthInstanceLoan::LivingUnitReference final:public ScriptUnitReadReference {
 const std::shared_ptr<State>state_;
 public:
 explicit LivingUnitReference(std::shared_ptr<State>s):state_(std::move(s)){}
 void require_current()const override{
  BnwUnitFlagHealthInstanceLoan::require_current_from_storage_owned(state_);
 }
 ScriptValue read(std::string_view method)const override{
  require_current();
  try{
   const auto&health=*state_->health;
   const auto damage=health.damage_without_callbacks();const auto maximum=health.max_hit_points_without_callbacks();
   ScriptValue result;
   if(method=="IsDead")result=damage>=maximum;
   else if(method=="GetDamage")result=std::int64_t(damage);
   else if(method=="GetMaxHitPoints")result=std::int64_t(maximum);
   else if(method=="GetCurrHitPoints"){
    const auto remaining=std::int64_t(maximum)-std::int64_t(damage);
    if(remaining<INT32_MIN||remaining>INT32_MAX)throw ScriptError("Original Unit health signed subtraction overflow");
    result=remaining;
   }else if(method=="GetID")result=std::int64_t(state_->id);
   else if(method=="GetOwner")result=std::int64_t(state_->owner);
   else throw ScriptError("Living health Unit getter has no earned native supplier");
   require_current();return result;
  }catch(...){state_->failed=true;throw;}
 }
};
std::shared_ptr<const ScriptUnitReadReference>
BnwUnitFlagHealthInstanceLoan::retain_living_unit_reference_from_storage_without_callbacks(const std::shared_ptr<State>&s){
 require_current_from_storage_owned(s);
 try{auto result=std::make_shared<const LivingUnitReference>(s);result->require_current();return result;}
 catch(...){s->failed=true;throw;}
}
}
