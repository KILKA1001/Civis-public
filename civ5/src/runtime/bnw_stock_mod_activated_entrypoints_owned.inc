namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::require_modding_origin_owned(
    const BnwWorldViewSourceOperationLoan&loan,const LuaScriptHost&vm)const{
    const auto&p=*impl_;
    need(p.thread==std::this_thread::get_id(),"Native Modding foreign Bootstrap creator");
    need(!p.uncertain&&p.operation&&!p.diagnostic_world&&p.civis_sources&&p.host&&p.mods&&
        p.host.get()==&vm&&p.host.get()==p.host_owner&&p.worldview_sources&&
        &p.worldview_sources->current_operation_loan()==&loan,"Native Modding requires SAME actual bank/Bootstrap/Host/Source");
    loan.require_same_vm(vm);loan.require_current();p.civis_sources->require_current();
    const auto&recipe=p.civis_sources->recipe();
    need(recipe.canonical_payload_root.lexically_normal()==p.root.lexically_normal()&&
        recipe.profile.enabled_mods==p.selected.enabled_mods&&recipe.profile.enabled_dlc==p.selected.enabled_dlc,
        "Native Modding selected activation differs from actual Resource Bank recipe");
    const auto stamp=vm.stock_worldview_provider_stamp(loan);
    need(stamp&&stamp->vm==p.host_vm&&!stamp->failed&&stamp->failure_epoch==p.host_epoch,
        "Native Modding actual native Host/VM/failure stamp absent");
    const auto world=p.operation->world_lease();
    need(world&&world.get()==p.world.get()&&!world.owner_before(p.world)&&!p.world.owner_before(world),
        "Native Modding actual source world allocation changed");
    loan.require_current();loan.require_same_vm(vm);
}
std::shared_ptr<ScriptQuery> BnwSelectedStockLuaBootstrap::modding_activated_entrypoint_query_owned(
    const BnwWorldViewSourceOperationLoan&loan,const LuaScriptHost&vm,std::string_view type)const{
    try{
        require_modding_origin_owned(loan,vm);auto&p=*impl_;const auto*const registry=p.mods.get();
        auto actual=registry->activated_entrypoint_query_owned(type);
        require_modding_origin_owned(loan,vm);
        need(actual&&p.mods.get()==registry,"Native Modding actual registry SQL cursor preparation failed/replaced");
        auto query=std::make_shared<Impl::OwnedQuery>(&p,std::move(actual),*p.operation);
        std::erase_if(p.modding_entrypoint_queries,[](const auto&r){return r.query.expired();});
        p.modding_entrypoint_queries.push_back({query,registry});
        require_modding_activated_entrypoint_query_owned(loan,vm,query);return query;
    }catch(...){impl_->fail();throw;}
}
void BnwSelectedStockLuaBootstrap::require_modding_activated_entrypoint_query_owned(
    const BnwWorldViewSourceOperationLoan&loan,const LuaScriptHost&vm,const std::shared_ptr<ScriptQuery>&query)const{
    try{
        require_modding_origin_owned(loan,vm);need(bool(query),"Native Modding actual query retired");
        bool issued=false;
        for(const auto&r:impl_->modding_entrypoint_queries){
            const auto actual=r.query.lock();
            if(actual&&actual.get()==query.get()&&!actual.owner_before(query)&&!query.owner_before(actual)&&
                r.registry==impl_->mods.get()){issued=true;break;}
        }
        need(issued,"Native Modding query is not SAME retained selected registry issuance");
        require_modding_origin_owned(loan,vm);
    }catch(...){impl_->fail();throw;}
}
}
