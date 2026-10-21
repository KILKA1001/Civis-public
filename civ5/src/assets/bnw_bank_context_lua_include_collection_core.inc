// Inside civ5::assets after complete CivisResourceBankState.
// This Bank retains its own native source controls; CivisHost stays disjoint.
std::vector<std::string> CivisResourceBankOwner::context_lua_include_names_owned()const{
    const auto saved=state_;saved->pure();
    const auto actual=saved->context_native_include_source;
    require_boundary();saved->pure();
    need(saved->published&&saved->input&&saved->memory&&!saved->physical&&actual,
        "Lua collector requires SAME published Bank and authentic native source");
    const auto current=[&]{
        saved->pure();need(saved->context_native_include_source==actual,
            "Lua collector native source control changed");
        require_boundary();saved->pure();
    };
    current();actual->require_current();current();
    std::vector<std::string> result;
    actual->with_actual_completed_inputs([&](const auto& package,const auto& resource,
            const auto&,const auto&){
        current();need(package&&resource,"Lua collector native completed inputs absent");
        const auto input=package->early_input_lease();current();
        need(input.get()==saved->input.get()&&!input.owner_before(saved->input)&&
             !saved->input.owner_before(input),"Lua collector borrowed another Bank input");
        result=package->collect_bank_context_lua_include_names_owned(resource,current);current();
    });
    current();actual->require_current();current();return result;
}
std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner>
CivisResourceBankOwner::context_selected_user_options_owned()const{
    const auto saved=state_;saved->pure();const auto actual=saved->context_native_include_source;
    const auto current=[&]{
        saved->pure();need(saved->published&&actual&&saved->context_native_include_source==actual,
            "Bank UserOptions actual native source control absent/changed");
        require_boundary();saved->pure();
    };
    current();actual->require_current();current();
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> result;
    actual->with_actual_completed_inputs([&](const auto& package,const auto&,const auto&,const auto&){
        current();const auto input=package->early_input_lease();current();
        need(input.get()==saved->input.get()&&!input.owner_before(saved->input)&&
             !saved->input.owner_before(input),"Bank UserOptions borrowed another selected input");
        result=package->borrow_selected_user_options_for_source();current();
    });
    current();actual->require_current();current();
    need(bool(result),"Bank genuine UserOptions producer returned no owner");return result;
}
