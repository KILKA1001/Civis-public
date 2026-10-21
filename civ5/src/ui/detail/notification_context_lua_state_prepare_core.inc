// Inside civ5::ui, after complete Core and Cursor prefix implementation.
std::shared_ptr<const detail::NotificationContextLuaStatePreparation>
BnwWorldViewControlTree::prepare_context_lua_state_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>& cursor){
    actual->layout_origin();
    try{
        const auto prefix=require_context_cursor_load_prefix_owned(actual,operation);actual->layout_origin();
        require(prefix.get()==cursor.get()&&!prefix.owner_before(cursor)&&!cursor.owner_before(prefix)&&
            !actual->context_lua_preparation,"Context518 actual ordered cursor/new state occurrence absent");
        const auto before_fx=actual->effect_revision;const auto before_seq=actual->context_layout_baseline->sequence_;
        const auto descriptor=actual->source->prepared_source(actual->context);actual->layout_origin();
        const auto& host=operation.context_lua_host_owned();actual->layout_origin();
        const auto result=host.prepare_context_lua_state_owned(operation,descriptor,actual,cursor);actual->layout_origin();
        const auto checked=require_context_cursor_load_prefix_owned(actual,operation);actual->layout_origin();
        require(checked.get()==cursor.get()&&!checked.owner_before(cursor)&&!cursor.owner_before(checked)&&
            actual->effect_revision==before_fx&&actual->context_layout_baseline->sequence_==before_seq&&
            !actual->context_lua_preparation,"Context518 callback changed canonical prefix/once state");
        host.require_context_lua_state_owned(operation,result,actual,cursor);actual->layout_origin();
        require(bool(result),"Context518 protected creator returned no real retained state");
        actual->context_lua_preparation=result;return result;
    }catch(...){actual->failed.store(true);throw;}
}
void BnwWorldViewControlTree::require_context_lua_preparation_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const detail::NotificationContextLuaStatePreparation>& prepared){
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    require(prepared&&actual->context_lua_preparation.get()==prepared.get()&&
        !actual->context_lua_preparation.owner_before(prepared)&&!prepared.owner_before(actual->context_lua_preparation)&&
        actual->context_cursor_load_prefix,"Context518 canonical retained preparation replaced/absent");
    const auto& host=operation.context_lua_host_owned();actual->layout_origin();
    host.require_context_lua_state_owned(operation,prepared,actual,actual->context_cursor_load_prefix);actual->layout_origin();
    operation.require_origin_without_callbacks();actual->layout_origin();
}
