// Inside civ5::ui after real Root outer registration and historical Material.
struct detail::BnwNormalQueueDrainPermission::Impl final {
    using Gui=NotificationContextGuiPort;using Binding=graphics::detail::BnwNormalLabelDrawRecordBinding;
    using Entry=Gui::Impl::NormalFontDrawEntry;using Registration=Gui::Impl::NormalGuiManagerRegistration;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};bool sealed=false;
    const BnwNormalRootBatchHistory::Guard root;
    const BnwNormalItemOccurrence::Guard item;
    const std::shared_ptr<Registration> registration;
    const std::shared_ptr<const Entry> entry;
    const std::shared_ptr<BnwNormalLabelMaterialLoan::Impl> material;
    const std::uint32_t inner_index;
    Impl(BnwNormalRootBatchHistory::Guard r,BnwNormalItemOccurrence::Guard i,
        std::shared_ptr<Registration> actual_registration,std::shared_ptr<const Entry> actual_entry,
        std::shared_ptr<BnwNormalLabelMaterialLoan::Impl> actual_material,std::uint32_t cursor)
        :root(std::move(r)),item(std::move(i)),registration(std::move(actual_registration)),entry(std::move(actual_entry)),
         material(std::move(actual_material)),inner_index(cursor){}
    static bool same(const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
    void source_membership_first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&sealed&&registration&&entry&&material,
            "Selected manager entry permission retired/foreign/unsealed/missing original controls");
        root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
        item.require_same_root_without_callbacks(root);
        const auto actual_root=root.impl_;const auto manager=actual_root->gui.lock();
        require(manager&&manager->normal_queue_dispatching&&!manager->normal_queue_registering&&
            !manager->normal_queue_drained&&manager->normal_queue_selected_registration&&
            *manager->normal_queue_selected_registration==registration->table_index&&
            manager->normal_queue_registrations.size()==manager->normal_queue_dispatch_count&&
            manager->normal_queue_current_permission.lock().get()==this&&
            !manager->normal_queue_permission_capturing&&registration->sealed&&!registration->drained&&
            registration->table_index<manager->normal_queue_registrations.size()&&
            manager->normal_queue_registrations[registration->table_index]==registration&&
            registration->root.lock()==actual_root&&registration->prefix.lock()==actual_root->prefix&&
            registration->outer==actual_root->outer_window&&registration->outer_index==registration->outer->index&&
            registration->current_inner&&*registration->current_inner==inner_index&&
            registration->outer->begin_58&&registration->outer->end_5c&&
            inner_index>=*registration->outer->begin_58&&inner_index<*registration->outer->end_5c,
            "Permission is not current original registered outer window/inner cursor/manager occurrence");
        BnwWorldViewControlTree::require_normal_root_queue_registration_metadata_without_callbacks(root);
        const auto prefix=actual_root->prefix;const auto& queue=prefix->batch->payload_queue;
        require(inner_index<queue.entries.size()&&same(queue.entries[inner_index],entry)&&entry->slot_index==inner_index&&
            same(entry->item.lock(),item.impl_)&&same(entry->material_owner.lock(),material)&&
            same(material->canonical_self.lock(),material)&&same(material->batch,prefix),
            "Permission actual Queue14 inner32 entry/Item/Material control association changed");
        require(material->creator==creator&&material->alive.load()&&material->sealed&&material->records_captured&&
            material->scalars&&material->completed_selector&&*material->completed_selector==1&&!material->payload,
            "Selected manager permission original completed Material metadata retired");
        material->item.require_same_item_without_callbacks(item);
        material->root.require_same_root_without_callbacks(root);
        BnwNormalLabelMaterialLoan::Impl::historical_entry_physical_owned(prefix,entry);
        root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
        require(alive.load()&&manager->normal_queue_current_permission.lock().get()==this&&
            registration->current_inner&&*registration->current_inner==inner_index,
            "Selected manager permission changed during final physical cut");
    }
    void first()const{
        source_membership_first();material->historical_first();
        const auto binding=item.captured_normal_draw_records_without_callbacks();
        binding.require_physical_record_without_callbacks();
        entry->originating_record.require_physical_record_without_callbacks();
        entry->originating_record.require_same_binding_physical_without_callbacks(binding);
        entry->descriptor.require_physical_record_without_callbacks();
        entry->descriptor.require_same_binding_physical_without_callbacks(binding);
        entry->descriptor.require_same_record_physical_without_callbacks(entry->originating_record);
        source_membership_first();
    }
    void terminal_first()const{
        // Only a Graph-key completion for a genuine returned native failure
        // consumes this bridge. It does not grant GPU/scalars/stage readiness.
        // CODE poison and positive record gates are never cleared/weakened.
        source_membership_first();
        require(entry->draw_record_0&&entry->material_10&&entry->length_18&&*entry->length_18==0x90&&
            entry->reserved_c&&*entry->reserved_c==0&&entry->selector_8&&*entry->selector_8==1,
            "Returned failed command lacks executed native inner field stores");
        source_membership_first();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        first();root.require_source_owned(op);first();item.require_source_owned(op);first();
        material->historical_full(op);first();
    }
    void terminal_full(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        terminal_first();root.require_source_owned(op);terminal_first();item.require_source_owned(op);terminal_first();
    }
};
detail::BnwNormalQueueDrainPermission::Guard::Guard(std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
void detail::BnwNormalQueueDrainPermission::Guard::require_physical_current_without_callbacks()const{
    const auto saved=impl_;require(bool(saved),"Selected manager permission Guard missing control");saved->first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;require(bool(saved),"Selected manager permission Guard missing FULL control");saved->full(op);saved->first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& root)const{
    const auto saved=impl_;saved->first();saved->root.require_same_root_without_callbacks(root);saved->first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_actual_renderer_without_callbacks(const UiScreenGlRenderer& renderer)const{
    const auto saved=impl_;saved->first();saved->root.require_actual_renderer_without_callbacks(renderer);
    saved->item.require_actual_renderer_without_callbacks(renderer);saved->first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_queued_record_without_callbacks(
    const BnwNormalItemOccurrence::Guard& item,const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity& record,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt& descriptor)const{
    const auto saved=impl_;const auto supplied_item=item;const auto supplied_record=record;const auto supplied_descriptor=descriptor;
    saved->first();saved->item.require_same_item_without_callbacks(supplied_item);
    const auto binding=supplied_item.captured_normal_draw_records_without_callbacks();
    supplied_record.require_physical_record_without_callbacks();supplied_record.require_same_binding_physical_without_callbacks(binding);
    supplied_descriptor.require_physical_record_without_callbacks();supplied_descriptor.require_same_binding_physical_without_callbacks(binding);
    supplied_descriptor.require_same_record_physical_without_callbacks(supplied_record);
    require(Impl::same(supplied_record.impl_,saved->entry->originating_record.impl_)&&
        supplied_record.pass_==saved->entry->originating_record.pass_&&
        Impl::same(supplied_descriptor.impl_,saved->entry->descriptor.impl_)&&
        supplied_descriptor.pass_==saved->entry->descriptor.pass_&&
        supplied_descriptor.write_occurrence_==saved->entry->descriptor.write_occurrence_,
        "Permission operand is not SAME original queued record/descriptor write occurrence");
    saved->first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_terminal_postreturn_without_callbacks()const{
    const auto saved=impl_;require(bool(saved),"Returned command terminal permission missing own control");saved->terminal_first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_terminal_postreturn_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;require(bool(saved),"Returned command terminal FULL permission missing own control");saved->terminal_full(op);saved->terminal_first();
}
void detail::BnwNormalQueueDrainPermission::Guard::require_terminal_queued_control_without_callbacks(
    const BnwNormalItemOccurrence::Guard& item,const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity& record,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt& descriptor)const{
    const auto saved=impl_;saved->terminal_first();saved->item.require_same_item_without_callbacks(item);
    require(Impl::same(record.impl_,saved->entry->originating_record.impl_)&&record.pass_==saved->entry->originating_record.pass_&&
        Impl::same(descriptor.impl_,saved->entry->descriptor.impl_)&&descriptor.pass_==saved->entry->descriptor.pass_&&
        descriptor.write_occurrence_==saved->entry->descriptor.write_occurrence_,
        "Terminal postreturn operand is not SAME selected original opaque command control");saved->terminal_first();
}

detail::BnwNormalQueueRecordCompletion::BnwNormalQueueRecordCompletion(
    BnwNormalQueueDrainPermission::Guard permission,BnwNormalItemOccurrence::Guard item,
    graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity record,
    graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt descriptor,
    Outcome actual_outcome,const ConstructionKey&)
    :permission_(std::move(permission)),item_(std::move(item)),record_(std::move(record)),
     descriptor_(std::move(descriptor)),outcome_(actual_outcome){require_current_without_callbacks();}
void detail::BnwNormalQueueRecordCompletion::require_current_without_callbacks()const{
    require(outcome_==Outcome::actual_draw_issued||outcome_==Outcome::native_failure,
        "Graph returned invalid outcome or reserved zero outcome for genuine nonempty record");
    if(outcome_==Outcome::native_failure){
        permission_.require_terminal_postreturn_without_callbacks();
        permission_.require_terminal_queued_control_without_callbacks(item_,record_,descriptor_);
        permission_.require_terminal_postreturn_without_callbacks();
    }else{
        permission_.require_physical_current_without_callbacks();permission_.require_queued_record_without_callbacks(item_,record_,descriptor_);
        permission_.require_physical_current_without_callbacks();
    }
}
void detail::BnwNormalQueueRecordCompletion::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_current_without_callbacks();
    if(outcome_==Outcome::native_failure)permission_.require_terminal_postreturn_source_owned(op);
    else permission_.require_source_owned(op);
    require_current_without_callbacks();
}
void detail::BnwNormalQueueRecordCompletion::require_same_permission_without_callbacks(const BnwNormalQueueDrainPermission::Guard& other)const{
    require_current_without_callbacks();
    if(outcome_==Outcome::native_failure)other.require_terminal_postreturn_without_callbacks();
    else other.require_physical_current_without_callbacks();
    require(permission_.impl_==other.impl_&&!permission_.impl_.owner_before(other.impl_)&&
        !other.impl_.owner_before(permission_.impl_),"Graph postreturn completion has another actual selected permission");
    require_current_without_callbacks();
}
detail::BnwNormalQueueRecordCompletion::Outcome detail::BnwNormalQueueRecordCompletion::actual_outcome_without_callbacks()const{
    require_current_without_callbacks();const auto result=outcome_;require_current_without_callbacks();return result;
}

void BnwWorldViewControlTree::drain_normal_root_batch_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchExecution& actual_execution,std::shared_ptr<const NormalQueueConsumer> consumer)const{
    using Permission=detail::BnwNormalQueueDrainPermission;using Item=detail::BnwNormalItemOccurrence;
    using Completion=detail::BnwNormalQueueRecordCompletion;
    // Capture original Root execution control before the first Source FULL.
    const auto execution=actual_execution.impl_;execution->first();const auto root=execution->root;
    const auto actual_consumer=std::move(consumer); // OWN callable storage before FULL, never borrowed lexical function lifetime.
    const auto saved=root.impl_;const auto manager=saved->gui.lock();
    require(saved->tree.get()==this&&manager&&actual_consumer&&bool(*actual_consumer),"Actual manager drain has another Tree/owner or no real consumer");
    const auto first=[&]{execution->first();root.require_physical_current_without_callbacks();
        require_normal_root_queue_registration_metadata_without_callbacks(root);};
    const auto full=[&]{first();root.require_source_owned(operation);first();};
    first();full();first();require(!manager->normal_queue_dispatching&&!manager->normal_queue_registering&&
        saved->completed_registrations.empty()&&
        !saved->queue_drained&&!manager->normal_queue_drained&&!manager->normal_queue_permission_capturing&&
        manager->normal_queue_current_permission.expired(),"Actual manager drain replay/reentered");
    const auto count=manager->normal_queue_registrations.size();require(count!=0,"Actual manager registration table empty");
    // This K003 boundary owns the SAME actual Root. Other queues need their
    // separately entered owners, never this Root's permission or a FIFO alias.
    for(const auto& registration:manager->normal_queue_registrations)require(registration&&registration->sealed&&
        registration->root.lock()==saved&&!registration->drained,
        "Actual manager table includes another Root/current owner or retired registration");
    manager->normal_queue_dispatching=true;manager->normal_queue_dispatch_count=count;
    struct End{std::shared_ptr<detail::NotificationContextGuiPort::Impl> manager;
        ~End(){manager->normal_queue_dispatching=false;manager->normal_queue_permission_capturing=false;
            manager->normal_queue_current_permission.reset();manager->normal_queue_selected_registration.reset();}}end{manager};
    std::array<std::uint64_t,4> observations{};
    try{
        for(std::size_t table=0;table<count;++table){
            first();full();first();const auto registration=manager->normal_queue_registrations[table];
            const auto outer=registration->outer;
            require(registration->table_index==table&&registration->outer_index==outer->index&&outer->closed&&
                outer->header_0&&((*outer->header_0>>28)==0)&&outer->begin_58&&outer->end_5c,
                "Actual manager outer table/index/header/default window association differs");
            manager->normal_queue_selected_registration=table;registration->current_inner=*outer->begin_58;
            while(*registration->current_inner<*outer->end_5c){
                first();full();first();const auto cursor=*registration->current_inner;
                const auto& queue=saved->prefix->batch->payload_queue;
                require(cursor<queue.entries.size(),"Actual manager Queue14 inner32 window outside backing");
                const auto entry=queue.entries[cursor];require(bool(entry),"Actual selected inner record absent");
                const auto item_owner=entry->item.lock();const auto material=entry->material_owner.lock();
                require(item_owner&&material&&entry->reserved_c&&*entry->reserved_c==0&&entry->selector_8&&
                    *entry->selector_8==1,"Actual manager selected inner command lacks genuine simple Label consumer");
                const Item::IssuerKey item_key;const Item::Guard item(item_owner,item_key);
                item.require_physical_current_without_callbacks();item.require_same_root_without_callbacks(root);
                material->historical_first();item.require_source_owned(operation);material->historical_full(operation);
                first();require(manager->normal_queue_current_permission.expired()&&!manager->normal_queue_permission_capturing,
                    "Actual manager selected permission previous occurrence still live");
                manager->normal_queue_permission_capturing=true;
                const auto pending=[&]{first();item.require_physical_current_without_callbacks();material->historical_first();
                    require(manager->normal_queue_permission_capturing&&manager->normal_queue_current_permission.expired()&&
                        manager->normal_queue_selected_registration==table&&registration->current_inner==cursor&&
                        manager->normal_queue_registrations[table]==registration&&queue.entries[cursor]==entry&&
                        entry->item.lock()==item_owner&&entry->material_owner.lock()==material,
                        "Selected permission allocation POST changed actual manager/outer/inner owners");};
                pending();root.require_source_owned(operation);pending();item.require_source_owned(operation);pending();
                auto permission=std::make_shared<Permission::Impl>(root,item,registration,entry,material,cursor);
                pending();root.require_source_owned(operation);pending();item.require_source_owned(operation);pending();
                manager->normal_queue_current_permission=permission;manager->normal_queue_permission_capturing=false;
                permission->sealed=true;permission->first();permission->full(operation);permission->first();
                const Permission::IssuerKey permission_key;const Permission::Guard guard(permission,permission_key);
                const auto binding=item.captured_normal_draw_records_without_callbacks();
                const auto record=entry->originating_record;const auto descriptor=entry->descriptor;
                const detail::BnwNormalLabelMaterialLoan::ShaderAccess scalar_view(material,guard);
                guard.require_queued_record_without_callbacks(item,record,descriptor);guard.require_source_owned(operation);
                guard.require_queued_record_without_callbacks(item,record,descriptor);
                // Actual owner dispatch, not a local vector completion: only
                // genuine closed Graph result can complete this real call.
                const auto completed=(*actual_consumer)(operation,root,item,guard,scalar_view,binding,record,descriptor);
                completed.require_current_without_callbacks();completed.require_same_permission_without_callbacks(guard);
                completed.require_source_owned(operation);completed.require_same_permission_without_callbacks(guard);
                const auto outcome=completed.actual_outcome_without_callbacks();
                require(observations[0]<UINT64_MAX,"Actual manager returned entry observation space exhausted");
                ++observations[0];
                switch(outcome){
                    case Completion::Outcome::actual_draw_issued:++observations[1];break;
                    case Completion::Outcome::zero_count_no_draw:++observations[2];break;
                    case Completion::Outcome::native_failure:++observations[3];break;
                }
                // Native8E2DFB advances after callee RETURN regardless of the
                // callee's Draw/HRESULT. No successful-Draw-only ACK is claimed.
                permission->alive.store(false);manager->normal_queue_current_permission.reset();
                ++*registration->current_inner;
                first();full();first();
            }
            require(registration->current_inner==outer->end_5c,
                "Actual manager outer half-open inner window did not finish");
            registration->drained=true;manager->normal_queue_selected_registration.reset();
        }
        first();full();first();require(manager->normal_queue_current_permission.expired()&&
            !manager->normal_queue_permission_capturing&&observations[0]==observations[1]+observations[2]+observations[3],
            "Actual manager completion counters/current-entry retirement differ");
        for(const auto& registration:manager->normal_queue_registrations)require(registration->drained&&
            registration->current_inner==registration->outer->end_5c,"Actual manager registered outer command not consumed");
        // Canonical manager count/table retirement follows actual registered
        // traversal. Keep SAME consumed registration/cursor controls by a
        // noexcept standard-allocator swap; no allocator/callback or renewed
        // registration is inserted here. Native Queue.byte50/8ECA40 reset
        // policy is not guessed. ORIGINAL physical CPU custody lasts to UiEnd.
        require(saved->completed_registrations.empty(),"Completed original registration journal already published");
        saved->completed_registrations.swap(manager->normal_queue_registrations);
        manager->normal_queue_registrations.clear();manager->normal_queue_dispatch_count=0;manager->normal_queue_drained=true;
        saved->queue_observations=observations;saved->queue_drained=true;
        execution->first();root.require_source_owned(operation);execution->first();
    }catch(...){manager->failed.store(true);saved->tree->core_->failed.store(true);throw;}
}

std::array<std::uint64_t,4> detail::BnwNormalRootBatchExecution::completed_queue_observations_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto root=saved->root;const auto actual=root.impl_;
    const auto manager=actual->gui.lock();
    require(manager&&actual->queue_drained&&manager->normal_queue_drained&&!manager->normal_queue_dispatching&&
        manager->normal_queue_registrations.empty()&&manager->normal_queue_current_permission.expired()&&
        !manager->normal_queue_selected_registration&&!manager->normal_queue_permission_capturing,
        "Root observations require actual complete manager traversal/count retirement");
    const auto values=actual->queue_observations;saved->first();return values;
}

