    void historical_cpu_first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&tree&&run&&batch&&
            records_captured&&scalars&&first_entry_slot&&completed_selector&&*completed_selector==1&&!payload&&
            captured_viewport_dimensions&&captured_viewport_depth,
            "Historical Material original facade/control or completed CPU stores absent");
        root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
        item.require_same_root_without_callbacks(root);item.require_actual_positioned_data_without_callbacks(data);
        const auto original_item=item.impl_;const auto actual_root=root.impl_;
        require(actual_root&&original_item&&same(actual_root->prefix,batch)&&same(actual_root->tree,tree)&&
            same(original_item->tree,tree)&&original_item->publication.get()==publication&&original_item->index==index&&
            original_item->operation==operation&&original_item->serial==serial&&
            same(original_item->paint.standalone_,paint.standalone_)&&!paint.impl_&&
            same(original_item->binding.impl_,binding.impl_)&&same(original_item->paint_storage->run_facade,run)&&
            same(actual_root->matrix_facade->impl_,matrix)&&
            *captured_viewport_dimensions==actual_root->dimensions&&
            batch->batch->dimensions==*captured_viewport_dimensions&&
            batch->batch->viewport_depth==*captured_viewport_depth,
            "Historical Material not SAME actual Root/Item/Paint/DATA/Binding/Matrix ownership");
        // Real Graph physical records preserve immutable CPU/CODE identity.
        // ACTIVE Frame/Paint/DATA/Matrix/current atlas never appears here.
        binding.require_physical_record_without_callbacks();
        std::optional<Scalars> captured;std::size_t descending=0;
        for(int p=2;p>=0;--p){const auto pass=static_cast<unsigned>(p);
            require(bool(entries[pass])==pass_present[pass]&&bool(records[pass])==pass_present[pass],
                "Historical Material completed pass membership changed");
            if(!entries[pass])continue;
            const auto entry=entries[pass];historical_entry_physical_owned(batch,entry);
            require(entry->slot_index==static_cast<std::size_t>(*first_entry_slot)+descending&&
                same(entry->item.lock(),original_item)&&same(entry->material_owner.lock(),canonical_self.lock())&&
                same(entry->originating_record.impl_,records[pass]->impl_)&&entry->originating_record.pass_==pass,
                "Historical Material immutable entry is not its genuine descending Item/Material occurrence");
            entry->originating_record.require_physical_record_without_callbacks();
            entry->originating_record.require_same_binding_physical_without_callbacks(binding);
            entry->descriptor.require_physical_record_without_callbacks();
            entry->descriptor.require_same_binding_physical_without_callbacks(binding);
            entry->descriptor.require_same_record_physical_without_callbacks(*entry->draw_record_0);
            require(same(entry->material_10->impl_,entry->descriptor.impl_),
                "Historical Material original opaque material/descriptor controls differ");
            const auto& w=entry->words;const Scalars value{{w[0],w[3]},{w[6],w[5]},w[4],w[9]};
            if(captured)require(*captured==value,"Historical Material per-pass fourteen-word values differ");
            else captured=value;
            ++descending;
        }
        require(captured&&*captured==*scalars&&descending==completed_entries,
            "Historical Material completed entry count/scalars changed");
        item.require_physical_current_without_callbacks();root.require_physical_current_without_callbacks();
    }
    void historical_first()const{
        require(sealed,"Historical Material whole Root/registration seal unissued");
        historical_cpu_first();
        BnwWorldViewControlTree::require_normal_root18_return_without_callbacks(*batch);
        BnwWorldViewControlTree::require_normal_root_queue_registration_without_callbacks(root);
        historical_cpu_first();
    }
    void child_return_first()const{
        require(cpu_child_returned&&child_tail,"Material genuine Label child return unissued");
        historical_cpu_first();
        const auto common=batch->common_root;
        require(common&&child_tail->creator==creator&&child_tail->serial==serial&&
            child_tail->control==item.impl_->control&&same(child_tail->item.lock(),item.impl_)&&
            child_tail->ordinal<common->count&&
            same(common->ordered[child_tail->ordinal],child_tail->receiver),
            "Material child return is not SAME real Item/receiver/ordered78 occurrence");
        bool registered=same(common->label_tail,child_tail);
        for(const auto& returned:common->label_returns)if(same(returned,child_tail)){
            require(!registered,"Material child return appears twice in Root journal");registered=true;
        }
        require(registered,"Material actual Label tail not in SAME Root pending/completed journal");
        BnwWorldViewControlTree::require_normal_label_tail_return_without_callbacks(*batch);
        historical_cpu_first();
    }
    void child_return_full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        child_return_first();require(&supplied==operation,"Material child return another Source operand");
        root.require_source_owned(supplied);child_return_first();
        item.require_source_owned(supplied);child_return_first();
        supplied.require_operation(serial);child_return_first();
    }

    void historical_full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        historical_first();require(&supplied==operation,"Historical Material another Source operand");
        root.require_source_owned(supplied);historical_first();
        item.require_source_owned(supplied);historical_first();
        supplied.require_operation(serial);historical_first();
    }

