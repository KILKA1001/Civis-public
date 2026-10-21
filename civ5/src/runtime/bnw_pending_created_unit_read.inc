// Same Core translation unit; no new Bank, archive, or original Plan lease.
namespace civ5::runtime {
struct BnwPendingCreatedUnitRead::Storage {
 const std::thread::id creator=std::this_thread::get_id();
 const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::Binding binding;
 const std::weak_ptr<const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan>plan;
 const std::weak_ptr<const PendingUnit>node;
 std::weak_ptr<const BnwPendingCreatedUnitRead>self;
 mutable std::atomic<bool>failed{false};
 Storage(const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::Binding&b,
  const std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan>&p,
  const std::shared_ptr<PendingUnit>&n):binding(b),plan(p),node(n){}
 void pure(const BnwPendingCreatedUnitRead&actual)const{
  need(creator==std::this_thread::get_id(),"Pending created read foreign creator before mutable source");
  need(!failed.load(),"Pending created read quarantined");
  const auto own=self.lock();need(own&&own.get()==&actual&&own->storage_.get()==this,
   "Pending created read is not canonical issued binding");
 }
};
BnwPendingCreatedUnitRead::BnwPendingCreatedUnitRead(std::shared_ptr<Storage>s):storage_(std::move(s)){}
BnwPendingCreatedUnitRead::~BnwPendingCreatedUnitRead()=default;
std::shared_ptr<const BnwPendingCreatedUnitRead>
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::capture_created_pending_read_without_callbacks()const{
 require_setup_graphical_enqueue_dispatch_without_callbacks();
 need(!storage_->created_pending_read_issued,"Pending created read factory replay");
 auto data=std::make_shared<BnwPendingCreatedUnitRead::Storage>(storage_->binding,retain_factory_owner(),storage_->node);
 auto result=std::shared_ptr<const BnwPendingCreatedUnitRead>(new BnwPendingCreatedUnitRead(data));data->self=result;
 result->require_issuing_plan_without_callbacks();require_setup_graphical_enqueue_dispatch_without_callbacks();
 storage_->created_pending_read=result;storage_->created_pending_read_issued=true;return result;
}
void BnwPendingCreatedUnitRead::require_issuing_plan_without_callbacks()const{
 storage_->pure(*this);
 try{
  const auto owner=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
  const auto&core=owner->current();const auto p=core.impl_;
  const auto plan=storage_->plan.lock();need(plan&&plan.get()==p->initial_setup_graphical_plan.get(),
   "Pending created read actual Plan substituted");
  core.require_initial_setup_graphical_plan_without_callbacks(*plan);
  const auto node=storage_->node.lock();need(node&&node.get()==storage_->binding.node&&
   !node.owner_before(storage_->binding.node_control)&&!storage_->binding.node_control.owner_before(node)&&
   node.get()==p->initial_minor_quest->node.get(),"Pending created read SAME native node unavailable");
  storage_->pure(*this);(void)owner->current();
 }catch(...){storage_->failed=true;throw;}
}
void BnwPendingCreatedUnitRead::require_returned_without_callbacks()const{
 storage_->pure(*this);
 try{
  const auto owner=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
  const auto&core=owner->current();const auto p=core.impl_;const auto&b=storage_->binding;
  need(&core==b.core&&p->host.get()==b.host.get()&&!p->host.owner_before(b.host)&&!b.host.owner_before(p->host)&&
   p->state.get()==b.state.get()&&!p->state.owner_before(b.state)&&!b.state.owner_before(p->state)&&
   p->source==b.source&&&p->host->session()==b.session&&p->generation==b.generation&&
   p->state->ui_source_revision==b.world_revision&&p->host->identity().rules_sha256==p->fingerprint&&p->bank.get()==b.bank&&
   !p->bank.owner_before(b.bank_control)&&!b.bank_control.owner_before(p->bank)&&p->bank->pending_slot==b.slot,
   "Pending created returned Core/Host/world/Bank/control/generation changed");
  need(p->initial_set_xy&&p->initial_set_xy.get()==b.pending.get()&&
   !p->initial_set_xy.owner_before(b.pending)&&!b.pending.owner_before(p->initial_set_xy),
   "Pending created returned original pending parent substituted");
  // Existing earned original return checks the complete live archive and
  // input revision. It explicitly does NOT restore the retired old Plan scope.
  core.require_initial_setup_graphical_return_without_callbacks(*p->initial_set_xy);
  const auto plan=storage_->plan.lock();const auto node=storage_->node.lock();
  need(plan&&plan.get()==p->initial_setup_graphical_plan.get()&&plan->storage_->created_pending_read_issued&&
   !plan->storage_->created_pending_read.owner_before(storage_->self)&&
   !storage_->self.owner_before(plan->storage_->created_pending_read)&&node&&node.get()==b.node&&
   !node.owner_before(b.node_control)&&!b.node_control.owner_before(node)&&
   node.get()==p->initial_minor_quest->node.get(),"Pending created returned canonical issuer/node changed");
  check_registered(*p->bank,*node,p->initial_minor_quest->node,b.slot);
  need(node->scalars.at("m_eOwner")==b.owner&&node->scalars.at("m_iID")==b.full_id&&
   node->scalars.at("m_iX")==b.x&&node->scalars.at("m_iY")==b.y,
   "Pending created returned native full-ID/coordinates changed");
  storage_->pure(*this);(void)owner->current();
 }catch(...){storage_->failed=true;throw;}
}
void BnwPendingCreatedUnitRead::require_same_world_without_callbacks(const LiveSessionState*w,std::uint64_t generation)const{
 require_returned_without_callbacks();need(storage_->binding.state.get()==w&&storage_->binding.generation==generation,
  "Pending created read belongs to another Scene world/generation");require_returned_without_callbacks();
}
std::pair<std::int32_t,std::int32_t>BnwPendingCreatedUnitRead::issued_owner_full_id_without_callbacks()const{
 require_issuing_plan_without_callbacks();return {storage_->binding.owner,storage_->binding.full_id};
}
std::int32_t BnwPendingCreatedUnitRead::read_integer_without_callbacks(std::string_view method)const{
 require_returned_without_callbacks();
 try{
  const auto node=storage_->node.lock();need(bool(node),"Pending created live node expired");
  std::int32_t result=0;
  const auto max_hp=[&]{
   const auto&cache=storage_->binding.host->defines();const auto*d=cache.find("MAX_HIT_POINTS");
   need(cache.profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&
    d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),
    "Pending created actual BNW GC.MAX_HIT_POINTS unavailable");
   const auto value=narrow(std::get<std::int64_t>(d->lookup.value));need(value>0,"Pending created actual maximum HP invalid");return value;
  };
  if(method=="GetID")result=node->scalars.at("m_iID");
  else if(method=="GetOwner")result=node->scalars.at("m_eOwner");
  else if(method=="GetX")result=node->scalars.at("m_iX");
  else if(method=="GetY")result=node->scalars.at("m_iY");
  else if(method=="MovesLeft")result=node->scalars.at("m_iMoves");
  else if(method=="GetDamage")result=node->scalars.at("m_iDamage");
  else if(method=="GetMaxHitPoints")result=max_hp();
  else if(method=="GetCurrHitPoints")result=narrow(std::int64_t(max_hp())-node->scalars.at("m_iDamage"));
  else if(method=="CargoSpace")result=node->scalars.at("m_iCargoCapacity");
  else if(method=="GetUnitType")result=node->scalars.at("m_eUnitType");
  else if(method=="GetDomainType")result=read_domain_without_callbacks();
  else throw BnwNativeUnitAllocatorUnavailable("Pending created required integer Unit API unsupported");
  require_returned_without_callbacks();return result;
 }catch(...){storage_->failed=true;throw;}
}
bool BnwPendingCreatedUnitRead::read_boolean_without_callbacks(std::string_view method)const{
 require_returned_without_callbacks();
 try{
  const auto node=storage_->node.lock();need(bool(node),"Pending created live boolean node expired");bool result=false;
  if(method=="IsTrade")result=read_trade_without_callbacks();
  else if(method=="IsGarrisoned"){
   const auto plan=storage_->plan.lock();need(plan&&plan->storage_->readers_prepared,"Pending Unit actual garrison getter not prepared");
   result=plan->storage_->garrison&&plan->storage_->garrison->selected==std::optional{storage_->binding.full_id};
  }else if(method=="IsCargo"){
   const auto owner=node->scalars.at("m_transportUnit.owner");
   need(owner==-1,"Pending actual cargo transport resolution requires genuine typed transport Unit reader");
   result=false; // Native NO_PLAYER handle branch, proven actual member; no missing-provider fallback.
  }else if(method=="IsDead")result=node->scalars.at("m_iDamage")>=read_integer_without_callbacks("GetMaxHitPoints");
  else if(method=="IsCombatUnit")result=node->scalars.at("m_iBaseCombat")>0;
  else if(method=="IsEmbarked"){
   const auto v=node->scalars.at("m_bEmbarked");need(v==0||v==1,"Pending created actual embark boolean malformed");result=v!=0;
  }else throw BnwNativeUnitAllocatorUnavailable("Pending created required boolean Unit API unsupported");
  require_returned_without_callbacks();return result;
 }catch(...){storage_->failed=true;throw;}
}
}
