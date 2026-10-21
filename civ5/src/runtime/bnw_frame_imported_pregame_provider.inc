namespace civ5::runtime {
bool BnwImportedPregameReadCache::frame_observer_participant(const BnwStockLuaAttemptLease::ScopeIdentity& scope,std::int32_t actor)const{
 require_actual(scope);
 try{const bool value=actual_sp_startup_before_read_->frame_observer_participant(actor);require_actual(scope);return value;}
 catch(...){poisoned_.store(true);throw;}
}
bool BnwImportedPregameReadCache::frame_is_host(const BnwStockLuaAttemptLease::ScopeIdentity& scope)const{
 require_actual(scope);
 try{const bool value=actual_sp_startup_before_read_->frame_is_host();require_actual(scope);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}
