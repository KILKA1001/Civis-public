// Inside runtime namespace, after complete Source::Impl. Private Host-only relay.
std::shared_ptr<ScriptQuery> BnwWorldViewSourceOperationLoan::context_modding_activated_entrypoint_query_owned(
    std::string_view type)const{
    try{
        require_origin_without_callbacks();require_current();const auto&p=*impl_;
        need(p.bootstrap&&p.vm,"Native Modding current actual Bootstrap/VM absent");
        auto query=p.bootstrap->modding_activated_entrypoint_query_owned(*this,*p.vm,type);
        require_origin_without_callbacks();require_current();
        p.bootstrap->require_modding_activated_entrypoint_query_owned(*this,*p.vm,query);
        require_origin_without_callbacks();return query;
    }catch(...){impl_->failed.store(true);impl_->lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
void BnwWorldViewSourceOperationLoan::require_modding_activated_entrypoint_query_owned(
    const std::shared_ptr<ScriptQuery>&query)const{
    try{
        require_origin_without_callbacks();require_current();const auto&p=*impl_;
        need(p.bootstrap&&p.vm,"Native Modding current actual Bootstrap/VM absent");
        p.bootstrap->require_modding_activated_entrypoint_query_owned(*this,*p.vm,query);
        require_origin_without_callbacks();require_current();
    }catch(...){impl_->failed.store(true);impl_->lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
