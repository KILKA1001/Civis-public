// Genuine early Context graph prerequisite, AFTER constructor/cursor/VM/dim/5761.
void BnwWorldViewControlTree::require_context_initial_lua_graph_origin_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const runtime::ScriptUiChildSource>& descriptor,
    const runtime::LuaScriptHost& host)const {
    const auto actual=core_;actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    require(descriptor&&actual->context_construction_return&&actual->context_layout_baseline&&
        actual->context_gui_port&&
        actual->layout_initialization_return&&actual->layout_initialization_return->native_return_&&actual->context_lua_preparation,
        "Context native graph requires actual retained constructor/parser/physicalGUI/VM/dim/5761 return");
    const auto initialized=require_context_initialization_return_owned(actual,actual->layout_initialization_return->native_return_);
    actual->layout_origin();require(initialized.get()==actual.get()&&!initialized.owner_before(actual)&&!actual.owner_before(initialized),
        "Context native graph uses another genuine5761 receiver");
    const auto origin=actual->layout_initialization_return->native_return_->impl_->origin;
    if(origin&&origin->direct_read_){
        require(!actual->context_layout_baseline->parsed_source_serial_&&!actual->context_load_origin,
            "Direct5761 graph cannot masquerade as a legacy parser/load origin");
        require_context_direct_initialization_history_owned(operation,actual->layout_initialization_return->native_return_);
    }else require(actual->context_layout_baseline->parsed_source_serial_&&actual->context_load_origin,
        "Context native graph requires genuine legacy parser/load origin");
    operation.require_same_vm(host);actual->layout_origin();operation.require_same_source(*descriptor);actual->layout_origin();
    const auto selected=actual->source->prepared_source(actual->context);actual->layout_origin();
    require(selected.get()==descriptor.get()&&!selected.owner_before(descriptor)&&!descriptor.owner_before(selected),
        "Context native graph descriptor is not actual SAME native factory member");
    require_context_lua_preparation_owned(actual,operation,actual->context_lua_preparation);actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation,"Context native graph Source operation changed");
    operation.require_origin_without_callbacks();actual->layout_origin();
}
