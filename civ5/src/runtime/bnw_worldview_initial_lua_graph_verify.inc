namespace civ5::runtime {
void BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(
    const std::shared_ptr<ScriptUiControlGraph>& graph,const std::weak_ptr<const void>& expected,
    const std::shared_ptr<const ScriptUiChildSource>& source,
    const BnwWorldViewSourceOperationLoan& operation,const LuaScriptHost& vm){
    require(graph&&graph->backend_&&!expected.expired()&&source,"Context initialLua real backend/Core/source absent");
    const auto adapter=std::dynamic_pointer_cast<BnwWorldViewScriptUiAdapter>(graph->backend_);
    require(bool(adapter),"Context initialLua graph has no actual native effects adapter");
    const auto saved=adapter->state_;
    saved->guarded_render([&](const State::Frame& frame){
        require(frame.loan==&operation&&saved->vm==&vm,"Context initialLua actual Frame/source operation/VM changed");
        const std::weak_ptr<const void> physical=saved->tree->core_;
        require(!physical.expired()&&!physical.owner_before(expected)&&!expected.owner_before(physical)&&
            saved->source.get()==source.get()&&!saved->source.owner_before(source)&&!source.owner_before(saved->source),
            "Context initialLua registered graph is another physical Core/source");
        const auto context=saved->render_operation_call(frame,[&]{return graph->context();});
        require(context==saved->context,"Context initialLua registered graph context substituted");
        saved->render_full(frame);
        require(!physical.expired()&&!expected.expired(),"Context initialLua physical receiver retired during validation");
        return 0;
    });
}
}
