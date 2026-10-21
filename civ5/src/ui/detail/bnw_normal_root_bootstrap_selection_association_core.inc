// After complete CommonRoot/CURRENT Call definitions. Genuine first selection
// joins the actual initial call; this check does not issue or renew any token.
void BnwWorldViewControlTree::require_normal_root_bootstrap_selection_call_without_callbacks(
    const detail::BnwNormalRootBootstrapSelection& actual_selection,
    const detail::BnwNormalRootBatchHistory::Guard& actual_root,
    const detail::BnwNormalVisibleLabelCall& actual_call)const{
    const auto selection=actual_selection.impl_;const auto root=actual_root;const auto call=actual_call;
    selection->first();root.require_physical_current_without_callbacks();call.require_current_without_callbacks();
    call.require_same_root_without_callbacks(root);
    const auto selected_root=root.impl_;const auto current=call.impl_;const auto common=selected_root->prefix->common_root;
    const auto same=[](const auto&a,const auto&b){return detail::BnwNormalRootBootstrapSelection::Impl::same(a,b);};
    require(selection->tree.get()==this&&same(selected_root->tree,selection->tree)&&
        selected_root->operation==selection->operation&&selected_root->serial==selection->serial&&
        common&&same(common->completion,selection->completion)&&same(common->root,selection->root)&&
        same(common->children,selection->children)&&common->count==selection->count&&
        common->cursor==selection->ordinal&&common->label_returns.empty()&&!common->label_tail&&
        current->ordinal==selection->ordinal&&current->control==selection->control&&
        same(current->receiver,selection->selected)&&selected_root->prefix->control==selection->control,
        "Native prepaint selection does not match genuine first CURRENT bootstrap Root/call");
    for(std::size_t i=0;i<selection->count;++i)
        require(detail::BnwNormalRootBootstrapSelection::Impl::same(common->ordered[i],selection->ordered[i]),
            "Actual bootstrap Common ordered78 differs from original prepaint selection");
    require_normal_root_bootstrap_selection_publication_without_callbacks(*selection,*selected_root->publication);
    selection->first();root.require_physical_current_without_callbacks();call.require_current_without_callbacks();
}
