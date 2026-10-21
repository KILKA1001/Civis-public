namespace civ5::runtime {
void BnwWorldViewScriptUiAdapter::register_context_showhide_owned(
    const std::shared_ptr<ScriptUiControlGraph>& graph,const BnwWorldViewSourceOperationLoan& operation,const LuaScriptHost& vm){
    require(graph&&graph->backend_,"Context showhide registration actual backend missing");
    const auto adapter=std::dynamic_pointer_cast<BnwWorldViewScriptUiAdapter>(graph->backend_);
    require(bool(adapter),"Context showhide registration backend owner differs");
    const auto saved=adapter->state_;
    saved->guarded_render([&](auto frame){
        saved->render_loan(frame).require_same_vm(vm);saved->render_full(frame);
        require(saved->vm==&vm&&&saved->render_loan(frame)==&operation,"Context showhide registration actual VM/operation changed");
        saved->render_operation_call(frame,[&]{saved->tree->register_context_showhide_flag_owned(operation);return 0;});
        saved->render_full(frame);return 0;
    });
}
}
