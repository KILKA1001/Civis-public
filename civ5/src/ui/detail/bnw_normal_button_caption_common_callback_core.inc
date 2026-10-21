// Inside civ5::ui after Common5 recursion and Caption1..7. This executes the
// real positive178 callback AFTER unconditional Common RETURN. The callback
// returns only an original unique Material; no caller can issue its tail.
// The still-missing earned embedded tail remains an explicit terminal cut.
[[noreturn]] void BnwWorldViewControlTree::execute_normal_common_button_caption178_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& supplied_call,
    std::shared_ptr<const NormalCommonChildConsumers> supplied_consumers)const{
    using Call=detail::BnwNormalCommonChildCall;
    // Copy only genuine VALUE/strong original controls before any FULL cut.
    const Call call=supplied_call;const auto consumers=std::move(supplied_consumers);
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{
        call.require_current_without_callbacks();
        const auto original=call.impl_;
        require(original->kind==Call::Kind::button&&original->phase==Call::Phase::button_caption178&&
            original->common_return&&original->original_button&&original->original_button->label_178_,
            "Positive Caption callback requires actual Button178 after unconditional Common return");
        require_normal_common_recursive_return_physical_without_callbacks(*original->common_return);
        call.require_current_without_callbacks();
        require(consumers&&bool(consumers->caption178),
            "Positive Caption178 genuine Ui consumer unissued; empty callback is not a return");
    };
    current();const auto root=call.captured_root_without_callbacks();
    const auto original=call.impl_;const auto traversal=original->traversal.lock();
    require(bool(traversal),"Positive Caption callback original traversal retired");
    const auto execution=traversal->execution.lock();
    require(bool(execution),"Positive Caption callback original Execution retired");
    const auto matrix=execution->original_matrix;
    const auto same_root=[&]{
        current();root.require_physical_current_without_callbacks();
        require(matrix&&same(execution->root.impl_,root.impl_)&&
            same(execution->original_matrix,matrix)&&same(root.impl_->matrix_facade,matrix)&&
            root.impl_->tree.get()==this&&root.impl_->operation==&operation,
            "Positive Caption callback changed original Root/Matrix/Tree/Source association");
        root.require_physical_current_without_callbacks();current();
    };
    const auto full=[&]{same_root();call.require_source_owned(operation);same_root();
        root.require_source_owned(operation);same_root();};
    full();same_root();
    const auto caption=capture_normal_common_button_caption_call_owned(operation,call);
    const auto caption_first=[&]{
        same_root();require_normal_button_caption_call_current_without_callbacks(caption);
        require_normal_button_caption_same_common_without_callbacks(caption,call);
        require(same(caption.impl_->root.impl_,root.impl_),
            "Positive Caption callback not SAME original captured Common/Root");
        same_root();
    };
    const auto caption_full=[&]{caption_first();full();caption_first();
        require_normal_button_caption_call_source_owned(operation,caption);caption_first();};
    std::unique_ptr<detail::BnwNormalLabelMaterialLoan> returned;
    try{
        caption_full();caption_first();
        returned=consumers->caption178(operation,root,matrix,call,caption);
        // Own actual returned control BEFORE post-callback Source/lifetime cuts.
        // Do not reuse old active item Frame/Paint/DATA after its paint returned.
        const auto material=returned?returned->impl_:nullptr;
        const auto returned_first=[&]{
            caption_first();
            require(material&&material->creator==std::this_thread::get_id()&&material->alive.load()&&
                same(material->canonical_self.lock(),material)&&material->caption&&material->embedded_run&&
                !material->run&&same(material->caption->impl_,caption.impl_)&&
                same(material->embedded_run,caption.impl_->run)&&same(material->root.impl_,root.impl_)&&
                material->tree.get()==this&&material->operation==&operation&&material->serial==root.impl_->serial&&
                material->item.impl_->caption_history&&
                same(material->item.impl_->caption_history->impl_,caption.impl_)&&
                material->item.impl_->publication->normal_caption_emission_&&
                same(material->item.impl_->publication->normal_caption_emission_->impl_->caption.impl_,caption.impl_),
                "Positive Caption callback did not return SAME original embedded unique Material/Item/history");
            material->item.require_physical_current_without_callbacks();
            material->item.require_same_root_without_callbacks(root);
            root.require_physical_current_without_callbacks();caption_first();
        };
        returned_first();caption_full();returned_first();
        // Caption6/7 currently refuse before a public Loan. Future completion
        // must produce its own actual embedded Label540B54 tail certificate.
        // A standalone LabelTailReturn, bool or callback return cannot supply it.
        throw runtime::ScriptError("Positive Caption178 actual callback returned; required earned embedded Label Common56 tail RETURN certificate remains unissued");
    }catch(...){
        // Preserve the enclosing Root failure path; no loop cursor/phase/seal
        // advances through failed callbacks, missing tail or wrong Material.
        core_->failed.store(true);throw;
    }
}
