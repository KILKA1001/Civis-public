// Appended after complete qualified State. Only the authentic Host/Tree path
// invokes this private factory after genuine perContext preparation and5761.
namespace civ5::runtime {
std::shared_ptr<ScriptUiControlGraph> BnwWorldViewScriptUiAdapter::build_context_initial_lua_owned(
    const std::shared_ptr<ui::BnwWorldViewControlTree>& tree,
    const std::shared_ptr<const ScriptUiChildSource>& source,const LuaScriptHost& vm,
    std::vector<std::int64_t> events,const BnwWorldViewSourceOperationLoan& operation){
    require(tree&&source,"Context original graph actual Tree/source absent");
    operation.require_origin_without_callbacks();
    tree->require_context_initial_lua_graph_origin_owned(operation,source,vm);
    operation.require_origin_without_callbacks();
    const auto source_owner=tree->metadata_source_owner();operation.require_origin_without_callbacks();
    const auto& active=source_owner->current_operation_loan();operation.require_origin_without_callbacks();
    require(&active==&operation,"Context original graph active Source substituted");
    operation.require_render_operation_owned();
    operation.require_same_vm(vm);operation.require_origin_without_callbacks();
    operation.require_same_source(*source);operation.require_origin_without_callbacks();
    const std::string context(source->context());operation.require_origin_without_callbacks();
    require(!context.empty()&&events.size()==2&&events[0]!=events[1],
        "Context original graph needs captured descriptor and genuine selected Mouse enums");
    auto state=std::make_unique<State>(tree,source,vm,source_owner,context,std::move(events));
    state->guarded_render([&](const State::Frame& frame){
        require(frame.loan==&operation,"Context original graph factory uses another actual Frame");
        const auto controls=state->render_operation_call(frame,[&]{return tree->static_controls();});
        for(const auto& item:controls){
            const auto quote=state->render_operation_call(frame,[&]{return tree->inspect_control(item.second);});
            require(!quote.dynamic&&quote.alive&&state->controls.emplace(quote.id,item.second).second,
                "Context original graph static canonical control identity duplicated");
            state->statics.push_back({item.first,quote.id});
        }
        // Preserve the original selected ActionInfoPanel control contract,
        // while every other descriptor is admitted by physical native origin.
        if(context=="ActionInfoPanel"){
            const std::map<std::string,std::string,std::less<>> required{
                {"EndTurnButton","GridButton"},{"EndTurnText","Label"},
                {"EndTurnButtonProductionAlpha","AlphaAnim"},{"EndTurnButtonScienceAlpha","AlphaAnim"},
                {"EndTurnButtonFreeTechAlpha","AlphaAnim"},{"EndTurnButtonEndTurnAlpha","AlphaAnim"},
                {"EndTurnButtonMouseOverAlpha","AlphaAnim"}};
            require(state->statics.size()==required.size(),"ActionInfoPanel actual static control set differs");
            for(const auto& item:state->statics){
                const auto expected=required.find(item.name);
                const auto& control=state->render_control(frame,item.id);
                const auto quote=state->render_operation_call(frame,[&]{return tree->inspect_control(control);});
                require(expected!=required.end()&&quote.authored.name==expected->second,
                    "ActionInfoPanel actual static control kind differs");
            }
        }
        return 0;
    });
    tree->require_context_initial_lua_graph_origin_owned(operation,source,vm);
    operation.require_origin_without_callbacks();
    auto adapter=std::shared_ptr<BnwWorldViewScriptUiAdapter>(new BnwWorldViewScriptUiAdapter(std::move(state)));
    auto graph=std::shared_ptr<ScriptUiControlGraph>(new ScriptUiControlGraph(ScriptUiControlGraph::OwnedBackendTag{},context,adapter));
    graph->unitflag_health_adapter_=adapter;
    adapter->state_->guarded_render([&](const State::Frame& frame){
        require(frame.loan==&operation,"Context original graph publication Source changed");return 0;
    });
    operation.require_origin_without_callbacks();return graph;
}
}
