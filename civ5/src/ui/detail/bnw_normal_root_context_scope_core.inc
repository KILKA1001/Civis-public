// Included after the complete, Bank-owned Context Entry/Scope definitions.
// Only the actual renderer friend calls this private Tree entry. The lexical
// callback covers Root, its children, the actual manager return and drain.
UiGlPaintReport BnwWorldViewControlTree::with_normal_root_context_entry_owned(
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
    const runtime::BnwWorldViewSourceOperationLoan& supplied,
    std::shared_ptr<const std::function<UiGlPaintReport(const detail::BnwNormalRootContextEntry&)>> body)const{
    using Entry=detail::BnwNormalRootContextEntry;
    const auto tree=std::move(actual_tree);
    const auto callback=std::move(body);
    require(tree&&tree.get()==this&&callback&&bool(*callback),
        "Context Root scope requires SAME owned Tree and actual renderer callback");
    const auto owner=tree->core_;
    owner->layout_origin();
    const auto source=owner->source;
    const auto& operation=source->current_operation_loan();
    operation.require_origin_without_callbacks();
    require(&operation==&supplied,"Context Root scope received another Source operation");
    const auto serial=operation.operation_serial();
    const auto topology=owner->topology_revision,effects=owner->effect_revision;
    require(owner->context_layout_baseline.has_value(),
        "Context Root scope canonical baseline unavailable");
    const auto* const baseline=&*owner->context_layout_baseline;
    const auto layout=baseline->sequence_,sequence=owner->layout_sequence;
    std::shared_ptr<Entry::Scope> scope;

    const auto pure=[&](bool entered){
        require(tree.get()==this&&tree->core_==owner&&owner->source==source,
            "Context Root scope lost SAME Tree/Core/Source physical custody");
        owner->layout_origin(entered);
        const auto& current=source->current_operation_loan();
        require(&current==&operation,"Context Root scope Source replaced before saved operand");
        current.require_origin_without_callbacks();
        require(current.operation_serial()==serial&&owner->topology_revision==topology&&
            owner->effect_revision==effects&&owner->context_layout_baseline&&
            &*owner->context_layout_baseline==baseline&&owner->context_layout_baseline->sequence_==layout&&
            owner->layout_sequence==sequence,
            "Context Root scope immutable Source/layout tuple changed");
        if(entered){
            require(scope&&scope->creator==std::this_thread::get_id()&&scope->alive.load(),
                "Context Root lexical scope retired before borrowed receiver access");
            const auto physical=scope->core.lock();
            const auto canonical=owner->normal_context_root_scope.lock();
            require(physical.get()==owner.get()&&!physical.owner_before(owner)&&!owner.owner_before(physical)&&
                canonical.get()==scope.get()&&!canonical.owner_before(scope)&&!scope.owner_before(canonical),
                "Context Root lexical Scope allocation/control replaced");
            require_normal_root_receiver_before_common_without_callbacks(owner,operation,serial);
        }else{
            const auto previous=owner->normal_context_root_scope.lock();
            require(!previous,"Context Root scope cannot replace another lexical owner");
        }
        owner->layout_origin(entered);
    };
    const auto full=[&](bool entered){
        pure(entered);operation.require_operation(serial);pure(entered);
        source->require_sources_current();pure(entered);
        operation.require_current();pure(entered);
    };

    try{
        pure(false);full(false);pure(false);
        scope=std::make_shared<Entry::Scope>(owner);
        pure(false);full(false);pure(false);
        require(scope&&scope->alive.load()&&scope->creator==std::this_thread::get_id(),
            "Context Root allocating Scope failed its actual physical POST");
        // The same creator owns both writes. No callback/allocation between
        // entering the real Tree exclusion and publishing its unique control.
        owner->busy=true;
        owner->normal_context_root_scope=scope;
        struct Exit final {
            const std::shared_ptr<Entry::Scope> scope;
            const std::shared_ptr<Core> owner;
            ~Exit(){
                scope->alive.store(false); //Retire authority BEFORE borrowed Core use.
                owner->normal_context_root_scope.reset();
                owner->busy=false;
            }
        }exit{scope,owner};
        pure(true);full(true);pure(true);
        const auto entry=capture_normal_root_context_entry_owned(tree,operation);
        pure(true);entry.require_same_tree_without_callbacks(*tree);
        entry.require_source_owned(operation);pure(true);
        auto report=(*callback)(entry);
        pure(true);entry.require_source_owned(operation);pure(true);
        full(true);pure(true);
        return report;
    }catch(...){owner->failed.store(true);throw;}
}
