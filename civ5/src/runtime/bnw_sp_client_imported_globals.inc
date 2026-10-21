// Actual imported GC child; no fresh-game Impl or observation is a lender.
// Only the cacheGlobals fields/source-return are published to VerifyHandicap.
struct BnwSpClientPregameStartup::CachedGlobals final {
 enum class Phase {constructor,loading,returned,failed};
 BnwSpClientPregameStartup* const owner;
 const content::RulesDatabase* const selected;
 Phase phase=Phase::constructor;
 std::unique_ptr<content::RulesDatabase> database;
 std::optional<content::InfoTypeCatalog> types;
 std::map<std::string,content::GlobalDefineValue,std::less<>> fields;
 std::vector<std::string> stored_order;
 std::uint64_t stores=0;
 // SDK CvGlobals constructor initializers1738/1739/1765. They confer no
 // post-activation cache license; original LoadGlobalDefines must return.
 std::int32_t standard=1,multiplayer=3,ai=8;
 explicit CachedGlobals(BnwSpClientPregameStartup& p,const content::RulesDatabase& db)
  :owner(&p),selected(&db){current();}
 void current()const{
  need(phase!=Phase::failed&&owner&&selected,"SP imported Globals child retired");
  owner->current();need(owner->local_parent_!=nullptr,"SP imported Globals requires SAME actual boot lender");
  owner->local_parent_->current();
 }
 static std::int32_t get_int32(std::int64_t n){
  // Native Database::Results::GetInt uses SQLite int32, not widened host SQL.
  return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(n)));
 }
 void load_before_boot(){
  current();need(phase==Phase::constructor&&!owner->local_parent_->entered,
   "SP imported cacheGlobals cannot replay or load lazily in VerifyHandicap");
  phase=Phase::loading;
  try{
   database=selected->clone_gameplay();current();
   // Genuine own SQL allocation and ordered PostDefines predecessor. No
   // report/DTO is adopted as a token for an unrelated RulesDatabase instance.
   const auto post=database->postprocess_gameplay(content::GameplayPostprocessProfile::bnw,{});current();
   for(const auto& error:post.errors)
    if(error.stage=="prepare_insert"||error.stage=="post_defines")
     throw std::runtime_error("SP imported GC PostDefines predecessor failed");
   for(const auto& row:post.post_defines)need(row.errors.empty(),"SP imported GC actual PostDefines row failed");
   need(post.transaction_released,"SP imported GC PostDefines transaction did not return");
   types.emplace(database->cache_info_types());current();
   need(types->report().all_succeeded(),"SP imported GC actual type catalog failed");
   for(const auto& [name,value]:types->entries())
    need(value.id>=INT32_MIN&&value.id<=INT32_MAX,"SP imported GC Type ID outside native closed source domain");
   // Existing complete BNW cache source spec supplies SDK statement order.
   // Lookup -> native conversion -> field store -> next. Never prefetch all
   // SQL values before stores: late errors retain the earned prefix/history.
   for(const auto& spec:content::global_define_specs(content::GlobalDefineProfile::bnw)){
    current();auto result=database->global_define(spec.name,spec.kind);current();
    need(result.error.empty(),"SP imported GC selected Define lookup failed before field store");
    if(spec.kind==content::GlobalDefineKind::integer){
     auto n=get_int32(std::get<std::int64_t>(result.value));
     if(spec.zero_is_none&&n==0)n=-1;
     result.value=std::int64_t{n};
    }else need(spec.kind==content::GlobalDefineKind::source_float,
     "SP imported GC unexpected native cached Define kind");
    auto [field,inserted]=fields.emplace(std::string(spec.name),result.value);
    need(inserted,"SP imported GC cached field replay/duplicate");increment(stores);
    if(spec.name=="STANDARD_HANDICAP")standard=get_int32(std::get<std::int64_t>(field->second));
    if(spec.name=="MULTIPLAYER_HANDICAP")multiplayer=get_int32(std::get<std::int64_t>(field->second));
    if(spec.name=="AI_HANDICAP")ai=get_int32(std::get<std::int64_t>(field->second));
    stored_order.emplace_back(spec.name);current();
   }
   need(fields.contains("AI_HANDICAP")&&fields.contains("STANDARD_HANDICAP")&&fields.contains("MULTIPLAYER_HANDICAP"),
    "SP imported GC required cache field was not stored by its actual initializer");
   phase=Phase::returned;current();
  }catch(...){phase=Phase::failed;owner->local_parent_->quarantine();throw;}
 }
 std::int32_t actual_ai()const{
  current();need(phase==Phase::returned&&database&&types,
   "SP imported VerifyHandicap requires returned actual cached Globals before read");
  const auto it=fields.find("AI_HANDICAP");need(it!=fields.end(),"SP imported GC cached AI field absent");
  const auto value=get_int32(std::get<std::int64_t>(it->second));need(value==ai,"SP imported GC actual cached AI alias differs");
  current();return value;
 }
};
void BnwSpClientPregameStartup::set_handicap(std::int32_t id,std::int32_t value){
 // Native101A41C0 uses equality-aware SetAt, then rereads slotStatus before
 // optionally caching the human handicap. Equal first store does NOT skip
 // the second status read/setter call.
 set_integer(Vector::handicap,id,value);
 current();const auto status=actual_namespace_.archive.integer_arrays.at("slot_status").at(id);current();
 if(status==3)set_integer(Vector::last_human_handicap,id,value);
}
void BnwSpClientPregameStartup::verify_handicap(std::int32_t id){
 current();need(id>=0&&id<64&&cached_globals_,"SP original VerifyHandicap range/Globals child absent");
 const auto status=actual_namespace_.archive.integer_arrays.at("slot_status").at(id);current();
 if(status==1){set_handicap(id,cached_globals_->actual_ai());return;}
 const auto handicap=actual_namespace_.archive.integer_arrays.at("handicaps").at(id);current();
 const auto actual_ai=cached_globals_->actual_ai();
 if(handicap!=actual_ai)return;
 const auto last=actual_namespace_.archive.integer_arrays.at("last_human_handicaps").at(id);current();
 if(last!=-1){need(last>=INT32_MIN&&last<=INT32_MAX,"SP native last human handicap widened");set_handicap(id,static_cast<std::int32_t>(last));return;}
 // This uncommon variant next calls SAME original Game.isNetworkMultiPlayer.
 // The boot namespace alone cannot impersonate the actual initialized Game.
 phase_=Phase::handicap_cache_required;
 throw std::runtime_error("SP imported VerifyHandicap mandatory Game.isNetworkMultiPlayer source absent after genuine cachedAI/last-human reads");
}
void BnwSpClientPregameStartup::set_active_player(std::int32_t id){
 current();auto& active=actual_namespace_.archive.integers.at("active_player");
 if(active==id){current();return;}
 // Native1019DC0B archive dirty insertion, then scalar store; scalar assignment
 // does NOT append a vector delta. Preserve equality and partial insertion.
 const auto inserted=archive_dirty_.insert(Vector::active).second;if(inserted)increment(archive_insertions_);
 increment(field_stores_);active=id;current();
}
