// Included in the SAME Core TU/runtime namespace after the real wrapper body.
// New data issuer only; never calls Source.current/old Stock/VM/render providers.
struct BnwUnitFlagLivingHealthRead::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeUnitAllocatorCore>core;
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;
 std::weak_ptr<const BnwAttritionDamageDllWrapperPrefix>wrapper;
 std::weak_ptr<ImportedDamageArchive>archive;
 std::weak_ptr<ImportedMissionRuntime>plain;
 const saves::SavedUnitObservation*unit=nullptr;
 const saves::UnitCoreField*field=nullptr;const std::int64_t*value=nullptr;
 const LiveSessionState*world=nullptr;
 std::int32_t owner=-1,id=-1,expected=0,loaded=0,max_hp=0,last_full=-1;
 std::uint64_t generation=0,turn=0,damage_serial=0,wrapper_serial=0,changes=0,last_stores=0;
 bool dirty=false;
 std::weak_ptr<const BnwUnitFlagLivingHealthRead>self;
 mutable std::atomic<bool>failed{false};
 void pure(const BnwUnitFlagLivingHealthRead&actual)const{
  need(creator==std::this_thread::get_id(),"UnitFlag HP read foreign creator before alias access");
  need(!failed.load(),"UnitFlag HP read quarantined");const auto owner=self.lock();
  need(owner&&owner.get()==&actual&&owner->state_.get()==this,"UnitFlag HP read canonical capture substituted");
 }
};
BnwUnitFlagLivingHealthRead::BnwUnitFlagLivingHealthRead(std::shared_ptr<State>s):state_(std::move(s)){}
BnwUnitFlagLivingHealthRead::~BnwUnitFlagLivingHealthRead()=default;
std::shared_ptr<const BnwUnitFlagLivingHealthRead>
BnwAttritionDamageDllWrapperPrefix::capture_unitflag_health_for_actual_append_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;
 need(q.entered&&q.ctor_returned&&q.final_read&&q.final_initialized&&q.getters_returned&&!q.call_returned&&!q.source_adopted&&
  q.wrapper&&q.wrapper.get()==q.allocation&&q.wrapper->native_unit==q.unit&&q.wrapper->references==1,
  "UnitFlag HP custody lacks actual native append ingress");
 const auto parent=q.prefix.lock();need(bool(parent),"UnitFlag HP custody actual parent expired");const auto r=parent->impl_;
 const auto core_owner=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(r->core);
 const auto&core=core_owner->current();const auto p=core.impl_;const auto&damage=*r->damage;
 need(!r->failed&&damage.executed&&!damage.failed&&damage.assignment_invoked&&damage.changed&&
  damage.archive&&damage.plain&&damage.archive->unit==q.unit&&damage.archive->expected==q.new_damage&&
  damage.new_damage==q.new_damage&&damage.max_hp==q.read_max_hp&&q.unit==native_imported_get_at(*p->state,q.owner,q.id),
  "UnitFlag HP custody missing genuine executed Damage archive and current native full-ID");
 auto s=std::make_shared<BnwUnitFlagLivingHealthRead::State>();s->core=r->core;s->prefix=r->self;s->wrapper=r->dll_wrapper;
 s->archive=damage.archive;s->plain=damage.plain;s->unit=q.unit;s->field=damage.archive->field;s->value=damage.archive->value;
 s->world=p->state.get();s->owner=q.owner;s->id=q.id;s->expected=damage.archive->expected;s->loaded=damage.archive->loaded;
 s->max_hp=q.read_max_hp;s->last_full=damage.plain->last_full_health_turn;s->generation=q.generation;s->turn=r->turn;
 s->damage_serial=damage.serial;s->wrapper_serial=q.serial;s->changes=damage.archive->changes;s->dirty=damage.archive->dirty;
 s->last_stores=damage.plain->last_full_health_stores;
 auto result=std::shared_ptr<const BnwUnitFlagLivingHealthRead>(new BnwUnitFlagLivingHealthRead(s));s->self=result;
 s->pure(*result);require_common_without_callbacks();(void)core_owner->current();return result;
}
void BnwUnitFlagLivingHealthRead::require_current_without_callbacks()const{
 state_->pure(*this);
 try{
  const auto&s=*state_;const auto borrow=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(s.core);
  const auto&core=borrow->current();const auto p=core.impl_;const auto prefix=s.prefix.lock();const auto wrapper=s.wrapper.lock();
  const auto registered=p->player_attrition_prefixes.find(s.turn);
  need(registered!=p->player_attrition_prefixes.end(),"UnitFlag HP custody canonical attrition history absent");
  // Hold the actual registered weak allocation for the complete alias/history
  // check. Expired history never becomes a successful canonical match.
  const auto registered_prefix=registered->second.lock();
  need(prefix&&wrapper&&registered_prefix&&p->state.get()==s.world&&p->generation==s.generation&&p->actor==s.owner&&
   registered_prefix.get()==prefix.get()&&!registered_prefix.owner_before(prefix)&&!prefix.owner_before(registered_prefix)&&
   !registered->second.owner_before(s.prefix)&&!s.prefix.owner_before(registered->second),
   "UnitFlag HP custody SAME canonical Core/world/generation/native history changed");
  const auto r=prefix->impl_;r->thread();const auto&q=*wrapper->impl_;
  need(!r->failed&&r->damage&&r->damage->executed&&!r->damage->failed&&r->damage->assignment_invoked&&
   r->damage->serial==s.damage_serial&&r->damage->id==s.id&&r->damage->new_damage==s.expected&&
   r->dll_wrapper.get()==wrapper.get()&&!r->dll_wrapper.owner_before(s.wrapper)&&!s.wrapper.owner_before(r->dll_wrapper)&&
   !q.failed&&q.owner==s.owner&&q.id==s.id&&q.unit==s.unit&&q.serial==s.wrapper_serial&&q.new_damage==s.expected&&
   q.final_initialized&&q.getters_returned&&q.game_released&&q.call_returned&&q.source_adopted&&!q.wrapper,
   "UnitFlag HP custody lacks genuine returned command/adoption/retired original wrapper history");
  wrapper->require_earned_cleanup_without_callbacks();
  const auto archive=s.archive.lock();const auto plain=s.plain.lock();
  const auto registry=p->state->setup.extensions.find<Registry>();
  need(registry&&registry->players.contains(s.owner)&&registry->players.at(s.owner).get()==p->bank.get()&&
   !registry->players.at(s.owner).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(s.owner))&&
   p->bank->source.lock().get()==s.world&&p->bank->generation==s.generation&&
   !p->bank->source.owner_before(p->state)&&!p->state.owner_before(p->bank->source),
   "UnitFlag HP custody actual native Registry/Bank world controls changed");
  need(archive&&plain&&p->bank->imported_damage_fields.contains(s.id)&&p->bank->imported_mission_runtime.contains(s.id)&&
   p->bank->imported_damage_fields.at(s.id).get()==archive.get()&&p->bank->imported_mission_runtime.at(s.id).get()==plain.get()&&
   !p->bank->imported_damage_fields.at(s.id).owner_before(s.archive)&&!s.archive.owner_before(p->bank->imported_damage_fields.at(s.id))&&
   !p->bank->imported_mission_runtime.at(s.id).owner_before(s.plain)&&!s.plain.owner_before(p->bank->imported_mission_runtime.at(s.id))&&
   r->damage->archive.get()==archive.get()&&r->damage->plain.get()==plain.get()&&
   archive->creator==s.creator&&plain->creator==s.creator&&archive->owner.lock().get()==p->bank.get()&&plain->owner.lock().get()==p->bank.get()&&
   !archive->owner.owner_before(p->bank)&&!p->bank.owner_before(archive->owner)&&
   !plain->owner.owner_before(p->bank)&&!p->bank.owner_before(plain->owner)&&
   archive->generation==s.generation&&plain->generation==s.generation&&archive->actor==s.owner&&plain->actor==s.owner&&
   archive->id==s.id&&plain->id==s.id&&archive->unit==s.unit&&plain->unit==s.unit&&
   archive->field==s.field&&archive->value==s.value&&archive->loaded==s.loaded&&archive->expected==s.expected&&
   archive->changes==s.changes&&archive->dirty==s.dirty&&plain->last_full_health_turn==s.last_full&&plain->last_full_health_stores==s.last_stores,
   "UnitFlag HP custody registered live archive/plain allocation or native history changed");
  need(native_imported_get_at(*p->state,s.owner,s.id)==s.unit&&narrow(::civ5::runtime::core(*s.unit,"m_iID"))==s.id&&
   narrow(::civ5::runtime::core(*s.unit,"m_eOwner"))==s.owner&&s.field&&s.field->encoding==saves::UnitCoreEncoding::i32&&
   s.field->values.size()==1&&s.field->bytes.empty()&&&s.field->values.front()==s.value&&*s.value==s.expected&&
   attrition_actual_max_hit_points(*p->host)==s.max_hp&&p->host->identity().rules_sha256==p->fingerprint,
   "UnitFlag HP custody physical native unit/field alias/actual GC source changed");
  state_->pure(*this);(void)borrow->current();
 }catch(...){state_->failed=true;throw;}
}
void BnwUnitFlagLivingHealthRead::require_same_world_without_callbacks(const LiveSessionState*w,std::uint64_t generation)const{
 require_current_without_callbacks();need(w==state_->world&&generation==state_->generation,"UnitFlag HP custody foreign Scene world");
}
std::pair<std::int32_t,std::int32_t>BnwUnitFlagLivingHealthRead::owner_full_id_without_callbacks()const{
 require_current_without_callbacks();return {state_->owner,state_->id};
}
std::int32_t BnwUnitFlagLivingHealthRead::damage_without_callbacks()const{
 require_current_without_callbacks();return narrow(*state_->value);
}
std::int32_t BnwUnitFlagLivingHealthRead::max_hit_points_without_callbacks()const{
 require_current_without_callbacks();return state_->max_hp;
}
