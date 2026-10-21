// Included inside civ5::runtime after the existing imported native Core
// services. Bank and original private field bindings remain in that TU.
std::optional<BnwNativeImportedDamageArchiveObservation>
BnwNativeUnitAllocatorCore::bind_native_existing_damage_archive(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);
 const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())
  for(const auto&[actor,bank]:r->players){(void)actor;
   need(bank&&!bank->retired.load()&&bank->pending_units.empty(),
        "Imported damage registration requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);
 if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto id=narrow(core(*unit,"m_iID"));
 const auto plain_entry=p->bank->imported_mission_runtime.find(id);
 need(plain_entry!=p->bank->imported_mission_runtime.end()&&plain_entry->second,
      "Imported damage original plain runtime absent");
 const auto plain=plain_entry->second;
 const auto require_plain=[&]{
  const auto actual=p->bank->imported_mission_runtime.find(id);
  const auto owner=plain->owner.lock();
  need(actual!=p->bank->imported_mission_runtime.end()&&actual->second.get()==plain.get()&&
       !actual->second.owner_before(plain)&&!plain.owner_before(actual->second)&&
       plain->creator==std::this_thread::get_id()&&owner.get()==p->bank.get()&&
       !owner.owner_before(p->bank)&&!p->bank.owner_before(owner)&&plain->unit==unit&&
       plain->actor==p->actor&&plain->id==id&&plain->generation==p->generation,
       "Imported damage SAME OriginalRead plain node changed");
 };
 require_plain();
 auto&fields=p->bank->imported_damage_fields;
 auto existing=fields.find(id);std::shared_ptr<ImportedDamageArchive>binding;
 if(existing==fields.end()){
  require_imported_mission_payload_lineage(*p->source,p->host->identity());
  need(!p->source->source.live_bnw_units&&!p->source->source.live_bnw_area_units&&
       !p->source->source.live_bnw_unit_mission_continuation,
       "Imported damage requires closed unit/plain/effect history before overlay restore");
  need(p->source->snapshot.all_players&&p->source->snapshot.all_players->layout==bnw,
       "Imported damage original Read lineage unavailable");
  const auto&original=p->source->snapshot.all_players->players.at(p->actor);
  need(original.saved_player_id==p->actor,"Imported damage source player identity differs");
  const saves::SavedUnitObservation*loaded=nullptr;
  for(const auto&u:original.units.units)if(narrow(core(u,"m_iID"))==id){
   need(!loaded,"Duplicate original imported damage full ID");loaded=&u;}
  need(loaded&&core(*loaded,"m_eOwner")==p->actor&&core(*unit,"m_eOwner")==p->actor&&
       loaded->core.start_offset==unit->core.start_offset&&loaded->core.end_offset==unit->core.end_offset&&
       loaded->tail.start_offset==unit->tail.start_offset&&loaded->tail.end_offset==unit->tail.end_offset,
       "Imported damage original physical object lineage differs");
  const auto value=narrow(core(*unit,"m_iDamage"));
  need(value==narrow(core(*loaded,"m_iDamage"))&&plain->last_full_health_turn==-1&&
       plain->last_full_health_stores==0,
       "Imported damage cannot invent original Load after unregistered stores");
  // This is the canonical mutable SAME-session object; never a source copy.
  saves::SavedUnitObservation*mutable_unit=nullptr;
  for(auto&u:p->state->world.all_players->players.at(p->actor).units.units)
   if(&u==unit)mutable_unit=&u;
  need(mutable_unit,"Imported damage mutable canonical object absent");
  saves::UnitCoreField*field=nullptr;
  for(auto&f:mutable_unit->core.fields)if(name(f.name)=="m_iDamage"){
   need(!field,"Duplicate imported damage field");field=&f;}
  need(field&&field->encoding==saves::UnitCoreEncoding::i32&&field->values.size()==1&&field->bytes.empty(),
       "Imported damage actual typed i32 storage unavailable");
  std::optional<std::size_t>native_index;
  for(std::size_t i=0;i<std::size(archive_specs);++i)if(name(archive_specs[i].member)=="m_iDamage"){
   need(!native_index&&name(archive_specs[i].name)=="CvUnit::m_iDamage"&&
        archive_specs[i].kind==ArchiveKind::scalar,"Imported damage native constructor binding differs");
   native_index=i;}
  need(native_index.has_value(),"Imported damage native archive slot unavailable");
  binding=std::make_shared<ImportedDamageArchive>();binding->owner=p->bank;binding->unit=unit;
  binding->field=field;binding->value=&field->values.front();binding->actor=p->actor;binding->id=id;
  binding->loaded=value;binding->expected=value;binding->generation=p->generation;binding->native_index=*native_index;
  // reset()/Read Load wrote this archive scalar without touch. Its clean
  // baseline cannot certify any other field, retained effect or HP setter.
  p->check();(void)issuer->current();require_plain();
  need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&
       narrow(core(*unit,"m_iDamage"))==value&&plain->last_full_health_turn==-1&&
       plain->last_full_health_stores==0,"Imported damage source changed before real registration");
  need(fields.emplace(id,binding).second,"Imported damage canonical registration replay changed");
 }else binding=existing->second;
 const auto require_binding=[&]{
  const auto registered=fields.find(id);
  need(binding&&registered!=fields.end()&&registered->second.get()==binding.get()&&
       !registered->second.owner_before(binding)&&!binding.owner_before(registered->second)&&
       binding->creator==std::this_thread::get_id(),"Imported damage foreign/replaced scalar owner");
  const auto owner=binding->owner.lock();
  need(owner.get()==p->bank.get()&&!owner.owner_before(p->bank)&&!p->bank.owner_before(owner)&&
       binding->unit==unit&&binding->actor==p->actor&&binding->id==id&&binding->generation==p->generation,
       "Imported damage registered source/Bank/fullID changed");
  const saves::UnitCoreField*field=nullptr;
  for(const auto&f:unit->core.fields)if(name(f.name)=="m_iDamage"){
   need(!field,"Duplicate current imported damage scalar");field=&f;}
  need(field==binding->field&&field&&field->encoding==saves::UnitCoreEncoding::i32&&
       field->values.size()==1&&field->bytes.empty()&&&field->values.front()==binding->value&&
       narrow(field->values.front())==binding->expected,
       "Imported damage physical field alias/earned scalar lineage changed");
  require_plain();
 };
 require_binding();
 BnwNativeImportedDamageArchiveObservation result;
 result.owner=p->actor;result.full_id=id;result.value=binding->expected;result.loaded_value=binding->loaded;
 result.native_archive_index=binding->native_index;result.binding_name="CvUnit::m_iDamage";
 result.binding_member="m_iDamage";result.dirty=binding->dirty;result.changes=binding->changes;
 result.last_full_health_turn=plain->last_full_health_turn;
 result.last_full_health_stores=plain->last_full_health_stores;
 result.registered_lifetime=binding;result.plain_runtime_lifetime=plain;
 p->check();(void)issuer->current();require_binding();
 need(native_imported_get_at(*p->state,p->actor,query_id)==unit,
      "Imported damage actual registration return changed");return result;
}
