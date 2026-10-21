// After complete Source7 Setup/Create bodies. This is a real Civis declaration
// subset transition. It does not issue whole8E3A90/manager/sampler/Draw readiness.
namespace detail {
namespace {
template<class A,class B> bool same_vertex_owner(const A& a,const B& b){
    return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
bool same_vertex_element(const BnwOriginalVertexDeclarationElement& a,
    const BnwOriginalVertexDeclarationElement& b){
    return a.stream==b.stream&&a.offset==b.offset&&a.type==b.type&&
        a.method==b.method&&a.usage==b.usage&&a.usage_index==b.usage_index;
}
bool same_vertex_schema(std::span<const BnwOriginalVertexDeclarationElement> a,
    std::span<const BnwOriginalVertexDeclarationElement> b){
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),same_vertex_element);
}
}

struct BnwOriginalNonFontDeviceInitializer::VertexDeclarationFixedTable final {
    struct Row final {
        std::array<BnwOriginalVertexDeclarationElement,10> elements{};
        std::uint32_t count=0,stride=0;
        std::span<const BnwOriginalVertexDeclarationElement> schema()const{return {elements.data(),count};}
    };
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const BnwOriginalNonFontDeviceInitializer> initializer;
    const std::shared_ptr<const assets::CivisResourceBankOwner> bank;
    const std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
    const std::uint64_t epoch,publication_serial;
    const std::array<Row,29> rows;
    const std::uint32_t positive_rows;
    std::weak_ptr<const VertexDeclarationFixedTable> canonical;

    static std::array<Row,29> build_rows(){
        std::array<Row,29> out{};
        for(std::size_t slot=0;slot<original8e1060_rows.size();++slot){
            const auto native=original8e1060_rows[slot];
            if(!native.count)continue; // Fourteen actual qualified zero rows.
            require(native.count<out[slot].elements.size()&&
                native.first<=original8e1060_records.size()&&
                native.count<=original8e1060_records.size()-native.first,
                "Vertex fixed-table exact native record span unavailable");
            auto& row=out[slot];
            for(std::uint32_t i=0;i<native.count;++i){
                const auto record=original8e1060_records[native.first+i];
                require(record.type_key<original8e1060_types.size()&&
                    record.usage_key<original8e1060_usages.size(),
                    "Vertex fixed-table exact native map key unavailable");
                const auto type=original8e1060_types[record.type_key];
                const auto usage=original8e1060_usages[record.usage_key];
                row.elements[i]={0,record.offset,type.type,0,usage.usage,usage.index};
                row.stride+=type.size;
            }
            row.elements[native.count]={255,0,17,0,0,0};
            row.count=native.count+1;
        }
        return out;
    }
    static std::uint32_t count_positive(const std::array<Row,29>& rows){
        std::uint32_t total=0;for(const auto& row:rows)if(row.count)++total;return total;
    }
    VertexDeclarationFixedTable(const std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>& actual,
        std::uint64_t serial)
        :initializer(actual),bank(actual->impl_->selected_bank),pin(actual->impl_->pin),
         epoch(actual->impl_->epoch),publication_serial(serial),rows(build_rows()),
         positive_rows(count_positive(rows)){}
    void first(const BnwOriginalNonFontDeviceInitializer& receiver)const{
        receiver.require_physical_without_callbacks();
        const auto actual=initializer.lock();const auto self=canonical.lock();
        require(creator==std::this_thread::get_id()&&actual&&actual.get()==&receiver&&
            self&&self.get()==this&&same_vertex_owner(receiver.impl_->vertex_table,self)&&
            epoch==receiver.impl_->epoch&&publication_serial&&
            same_vertex_owner(bank,receiver.impl_->selected_bank)&&same_vertex_owner(pin,receiver.impl_->pin),
            "Vertex current fixed-table canonical Init/Bank/Window publication differs");
        pin->first_without_callbacks();receiver.require_physical_without_callbacks();
    }
};

struct BnwOriginalNonFontDeviceInitializer::VertexDeclarationCreationPhase final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const BnwOriginalNonFontDeviceInitializer> initializer;
    const std::shared_ptr<const VertexDeclarationFixedTable> table;
    const runtime::BnwWorldViewSourceOperationLoan* const operation;
    const std::uint64_t serial,epoch;
    std::weak_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl> actor;
    std::weak_ptr<const VertexDeclarationCreationPhase> canonical;
    std::array<std::optional<std::weak_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>>,29> results;
    std::uint32_t actual_invocations=0;
    std::optional<std::uint32_t> in_flight_slot;
    bool active=true;
    VertexDeclarationCreationPhase(const std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>& actual,
        std::shared_ptr<const VertexDeclarationFixedTable> selected,
        const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t actual_serial)
        :initializer(actual),table(std::move(selected)),operation(&op),serial(actual_serial),epoch(actual->impl_->epoch){}
};

void BnwOriginalNonFontDeviceInitializer::require_vertex_declaration_source_identity_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_physical_without_callbacks();op.require_origin_without_callbacks();
    op.require_render_operation_owned();
    // Existing GUI FIRST is callback-free and checks SAME Source/Host/World/
    // Scene/Window/generation/native occurrence before any old lexical pointer.
    impl_->gui->impl_->first(op);require_physical_without_callbacks();
    graphics::BnwOwnedCameraRendererWindowLease::require_retained_context_pin_origin_without_callbacks(
        impl_->window,impl_->pin);
    impl_->pin->first_without_callbacks();op.require_origin_without_callbacks();
}

void BnwOriginalNonFontDeviceInitializer::bind_vertex_declaration_actor_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl>& actual)const{
    require_vertex_declaration_source_identity_without_callbacks(op);
    const auto phase=impl_->vertex_phase.lock();
    require(impl_->vertex_stage==Impl::VertexStage::creating&&phase&&phase->active&&
        phase->creator==std::this_thread::get_id()&&phase->operation==&op&&
        phase->serial==op.operation_serial()&&phase->epoch==impl_->epoch&&
        same_vertex_owner(phase->canonical.lock(),phase)&&phase->actor.expired()&&
        phase->actual_invocations==0&&!phase->in_flight_slot&&actual&&
        actual->creator==phase->creator&&!actual->failed.load()&&!actual->checking.load()&&
        actual->next_slot==0&&actual->current_create.expired()&&actual->canonical.expired()&&
        same_vertex_owner(actual->initializer,retain_owner_owned_without_callbacks())&&
        actual->epoch==phase->epoch&&actual->creation_serial==phase->serial,
        "Vertex Source7 bind is not the fresh actual lexical actor");
    phase->table->first(*this);phase->actor=actual;
    require_vertex_declaration_setup_creation_owned(op);
}

void BnwOriginalNonFontDeviceInitializer::require_vertex_declaration_setup_creation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_vertex_declaration_source_identity_without_callbacks(op);
    const auto phase=impl_->vertex_phase.lock();
    require(impl_->vertex_stage==Impl::VertexStage::creating&&phase&&phase->active&&
        phase->creator==std::this_thread::get_id()&&same_vertex_owner(phase->canonical.lock(),phase)&&
        phase->operation==&op&&phase->serial==op.operation_serial()&&phase->epoch==impl_->epoch&&
        same_vertex_owner(phase->initializer.lock(),retain_owner_owned_without_callbacks())&&
        same_vertex_owner(phase->table,impl_->vertex_table),
        "Vertex creation outside actual member-owned subset phase");
    phase->table->first(*this);
    const auto actor=phase->actor.lock();
    require(actor&&actor->creator==phase->creator&&!actor->failed.load()&&actor->next_slot<=29&&
        actor->epoch==phase->epoch&&actor->creation_serial==phase->serial&&
        same_vertex_owner(actor->initializer,retain_owner_owned_without_callbacks())&&
        phase->actual_invocations<=phase->table->positive_rows,
        "Vertex creation actual Source7 actor/row cursor retired");
    if(phase->in_flight_slot)
        require(actor->next_slot==*phase->in_flight_slot,
            "Vertex genuine backend invocation moved actual row cursor");
    require_vertex_declaration_source_identity_without_callbacks(op);
}

std::span<const BnwOriginalVertexDeclarationElement>
BnwOriginalNonFontDeviceInitializer::vertex_declaration_elements_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const BnwOriginalVertexDeclarationCreateLoan& offered)const{
    require_vertex_declaration_setup_creation_owned(op);
    offered.require_physical_without_callbacks();
    const auto phase=impl_->vertex_phase.lock();const auto actor=phase->actor.lock();
    const auto create=offered.impl_;const auto canonical=create->canonical.lock();
    require(create->active&&canonical&&canonical.get()==&offered&&
        same_vertex_owner(actor,create->actor)&&same_vertex_owner(actor->current_create.lock(),canonical)&&
        actor->next_slot==create->slot&&create->slot<phase->table->rows.size(),
        "Vertex schema supplier needs SAME sealed current Source7 Create");
    const auto& actual_row=phase->table->rows[create->slot];
    require(actual_row.count&&same_vertex_schema(create->elements,actual_row.schema()),
        "Vertex actual lexical schema differs from current canonical fixed table");
    offered.require_physical_without_callbacks();
    require_vertex_declaration_setup_creation_owned(op);
    return create->elements;
}
std::uint32_t BnwOriginalNonFontDeviceInitializer::vertex_declaration_slot_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const BnwOriginalVertexDeclarationCreateLoan& actual)const{
    vertex_declaration_elements_without_callbacks(op,actual);return actual.impl_->slot;
}
std::uint32_t BnwOriginalNonFontDeviceInitializer::vertex_declaration_stride_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const BnwOriginalVertexDeclarationCreateLoan& actual)const{
    const auto slot=vertex_declaration_slot_without_callbacks(op,actual);
    return impl_->vertex_table->rows[slot].stride;
}

std::shared_ptr<const graphics::detail::BnwRendererContextPin>
BnwOriginalNonFontDeviceInitializer::vertex_declaration_context_pin_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const BnwOriginalVertexDeclarationCreateLoan& offered)const{
    const auto owner=retain_owner_owned_without_callbacks();
    const auto actual=offered.retain_owner_owned_without_callbacks();
    actual->require_source_owned(op); // Genuine fresh FULL, no GPU-only fence.
    owner->vertex_declaration_elements_without_callbacks(op,*actual);
    const auto pin=owner->impl_->pin;
    pin->first_without_callbacks();owner->require_vertex_declaration_setup_creation_owned(op);
    return pin;
}

std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
BnwOriginalNonFontDeviceInitializer::create_original_vertex_declaration_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const BnwOriginalVertexDeclarationCreateLoan& offered)const{
    const auto owner=retain_owner_owned_without_callbacks();
    const auto actual=offered.retain_owner_owned_without_callbacks();
    const auto phase=impl_->vertex_phase.lock();
    try{
        actual->require_source_owned(op);
        const auto slot=owner->vertex_declaration_slot_without_callbacks(op,*actual);
        require(phase&&same_vertex_owner(impl_->vertex_phase.lock(),phase)&&!phase->in_flight_slot&&
            !phase->results[slot]&&phase->actual_invocations<phase->table->positive_rows,
            "Vertex native backend call reentered/replayed actual row");
        phase->in_flight_slot=slot;
        // SAME retained renderer, closed Graph-owned Ui facade and actual
        // DSA VAO creation. Root constructs no Result/GL-name/HRESULT/boolean.
        const auto result=impl_->renderer->create_original_vertex_declaration_owned(owner,op,*actual);
        actual->require_source_owned(op);
        require(bool(result),"Vertex genuine backend returned no canonical Result");
        // This new Result is not in the phase journal yet. Native POST
        // follows its last Create FULL before any pure matcher/publication.
        impl_->renderer->require_original_vertex_declaration_device_current_owned(result,op);
        require(phase->active&&same_vertex_owner(impl_->vertex_phase.lock(),phase)&&
            phase->in_flight_slot&&*phase->in_flight_slot==slot&&result,
            "Vertex genuine backend returned outside current invocation");
        const auto schema=owner->vertex_declaration_elements_without_callbacks(op,*actual);
        BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_creation_without_callbacks(
            *result,*owner,impl_->epoch,*actual,slot,schema);
        owner->require_vertex_declaration_setup_creation_owned(op);
        // Nonallocating weak journal update follows genuine invocation + POST.
        phase->results[slot]=std::weak_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>(result);
        ++phase->actual_invocations;phase->in_flight_slot.reset();
        return result;
    }catch(...){
        if(phase){phase->active=false;phase->in_flight_slot.reset();}
        impl_->vertex_stage=Impl::VertexStage::failed;impl_->stage=Impl::Stage::failed;
        throw;
    }
}

void BnwOriginalNonFontDeviceInitializer::require_original_vertex_declaration_result_creation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t epoch,
    const BnwOriginalVertexDeclarationCreateLoan& actual,
    const BnwOriginalVertexDeclarationDeviceResultLoan& result)const{
    const auto schema=vertex_declaration_elements_without_callbacks(op,actual);
    const auto slot=actual.impl_->slot;const auto phase=impl_->vertex_phase.lock();
    const auto returned=phase->results[slot]?phase->results[slot]->lock():nullptr;
    require(epoch==impl_->epoch&&returned&&returned.get()==&result&&!phase->in_flight_slot,
        "Vertex creation result is not actual completed Ui invocation journal");
    BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_creation_without_callbacks(
        result,*this,epoch,actual,slot,schema);
    require_vertex_declaration_setup_creation_owned(op);
}

void BnwOriginalNonFontDeviceInitializer::require_vertex_declaration_setup_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t epoch,
    const BnwOriginalVertexDeclarationSetupLoan& actual)const{
    require_vertex_declaration_source_identity_without_callbacks(op);
    const auto published=impl_->vertex_setup.lock();const auto actor=actual.impl_;
    require(impl_->vertex_stage==Impl::VertexStage::completed&&impl_->vertex_phase.expired()&&
        epoch==impl_->epoch&&published&&published.get()==&actual&&
        same_vertex_owner(published->impl_,actor)&&same_vertex_owner(actor->canonical.lock(),published)&&
        actor->creator==std::this_thread::get_id()&&!actor->failed.load()&&actor->next_slot==29&&
        actor->current_create.expired()&&actor->epoch==epoch&&
        same_vertex_owner(actor->initializer,retain_owner_owned_without_callbacks())&&impl_->vertex_table,
        "Vertex retained getter needs SAME actually completed/published Setup");
    impl_->vertex_table->first(*this);
    // Immutable actual schema/strides/results are checked from completed
    // actor storage; no expired Create or allocating FULL getter is called.
    for(std::size_t slot=0;slot<impl_->vertex_table->rows.size();++slot){
        const auto& row=impl_->vertex_table->rows[slot];
        require(actor->strides[slot]==row.stride&&bool(actor->results[slot])==bool(row.count),
            "Vertex completed actor differs from current fixed-table row");
    }
    require_vertex_declaration_source_identity_without_callbacks(op);
}

void BnwOriginalNonFontDeviceInitializer::require_original_vertex_declaration_result_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t epoch,
    const BnwOriginalVertexDeclarationSetupLoan& actual,std::uint32_t slot,
    const BnwOriginalVertexDeclarationDeviceResultLoan& result)const{
    require_vertex_declaration_setup_current_owned(op,epoch,actual);
    require(slot<impl_->vertex_table->rows.size()&&impl_->vertex_table->rows[slot].count&&
        actual.impl_->results[slot]&&actual.impl_->results[slot].get()==&result,
        "Vertex retained result differs from completed SAME Setup slot");
    const auto schema=impl_->vertex_table->rows[slot].schema();
    BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_current_without_callbacks(
        result,*this,epoch,slot,schema);
    require_vertex_declaration_setup_current_owned(op,epoch,actual);
}

std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
BnwOriginalNonFontDeviceInitializer::initialize_vertex_declarations_subset_owned(
    UiScreenGlRenderer& renderer,const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& offered_cpu,
    const std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>& offered_gpu)const{
    const auto owner=retain_owner_owned_without_callbacks();
    const auto cpu=offered_cpu;const auto gpu=offered_gpu;
    std::shared_ptr<VertexDeclarationCreationPhase> phase;
    const auto pre=[&]{owner->require_vertex_declaration_source_identity_without_callbacks(op);
        require(cpu&&gpu&&impl_->renderer==&renderer&&impl_->stage==Impl::Stage::gpu_complete&&
            impl_->vertex_stage==Impl::VertexStage::unstarted&&impl_->vertex_phase.expired()&&
            impl_->vertex_setup.expired()&&!impl_->vertex_table&&
            same_vertex_owner(impl_->cpu.lock(),cpu)&&same_vertex_owner(impl_->gpu.lock(),gpu),
            "Vertex subset entry needs actual completed resource0 storage and fresh Init");};
    const auto pre_full=[&]{pre();impl_->full(op);pre();cpu->require_source_owned(op);pre();
        renderer.require_original_resource0_storage_current_owned(owner,op,cpu,gpu);pre();};
    try{
        pre_full();
        // Genuine immutable Civis publication from the admitted fixed BNW
        // definitions under actual Init/Bank/Window ownership, not a PE read
        // receipt or a caller DTO declared current. Original dynamic/whole
        // manager storage and intervening writers remain separate STOPs.
        const auto table=std::make_shared<VertexDeclarationFixedTable>(owner,op.operation_serial());
        pre_full();table->canonical=table;
        phase=std::make_shared<VertexDeclarationCreationPhase>(owner,table,op,op.operation_serial());
        pre_full();phase->canonical=phase;
        impl_->vertex_table=table;impl_->vertex_phase=phase;
        impl_->vertex_stage=Impl::VertexStage::creating;
        // Source7 now binds its actual allocated Impl BEFORE first FULL.
        const auto setup=BnwOriginalVertexDeclarationSetupLoan::execute_manager8e1060_owned(*owner,op);
        owner->require_vertex_declaration_setup_creation_owned(op);
        impl_->full(op);
        owner->require_vertex_declaration_devices_after_source_full_owned(op);
        owner->require_vertex_declaration_setup_creation_owned(op);
        const auto actor=phase->actor.lock();
        require(setup&&actor&&same_vertex_owner(setup->impl_,actor)&&
            same_vertex_owner(actor->canonical.lock(),setup)&&actor->next_slot==29&&
            actor->current_create.expired()&&!actor->failed.load()&&!actor->checking.load()&&
            phase->actual_invocations==table->positive_rows&&!phase->in_flight_slot,
            "Vertex subset did not execute whole29 cursors and genuine positive backend calls");
        for(std::uint32_t slot=0;slot<table->rows.size();++slot){
            const auto& row=table->rows[slot];
            const auto journal=phase->results[slot]?phase->results[slot]->lock():nullptr;
            require(actor->strides[slot]==row.stride&&
                bool(actor->results[slot])==bool(row.count)&&same_vertex_owner(actor->results[slot],journal),
                "Vertex subset actual stride/result journal differs from executed row");
            if(row.count)BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_current_without_callbacks(
                *actor->results[slot],*owner,impl_->epoch,slot,row.schema());
        }
        owner->require_vertex_declaration_setup_creation_owned(op);
        // Permanent completed stage + weak Setup publication: no Init/Setup
        // cycle and no replay if this actual returned Setup later expires.
        impl_->vertex_setup=setup;impl_->vertex_stage=Impl::VertexStage::completed;
        phase->active=false;impl_->vertex_phase.reset();
        setup->require_source_owned(op);
        owner->require_vertex_declaration_setup_current_owned(op,impl_->epoch,*setup);
        return setup;
    }catch(...){
        if(phase){phase->active=false;phase->in_flight_slot.reset();}
        impl_->vertex_stage=Impl::VertexStage::failed;impl_->stage=Impl::Stage::failed;
        impl_->vertex_phase.reset();impl_->vertex_setup.reset();throw;
    }
}

void BnwOriginalNonFontDeviceInitializer::require_vertex_declaration_devices_after_source_full_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    // The caller has just executed its actual Source FULL. This routine never
    // repeats FULL, invokes a Root five-gate method or renews an old Create.
    // Hold the entire genuine current result set before any native boundary.
    const auto owner=retain_owner_owned_without_callbacks();
    std::shared_ptr<VertexDeclarationCreationPhase> phase;
    try{
        owner->require_vertex_declaration_source_identity_without_callbacks(op);
        const auto stage=impl_->vertex_stage;
        const auto table=impl_->vertex_table;
        const auto epoch=impl_->epoch;
        std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan> setup;
        std::shared_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl> actor;
        std::array<std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>,29> keepers{};
        std::array<bool,29> engaged{};
        std::uint32_t invocations=0;
        std::optional<std::uint32_t> in_flight;
        require(table&&(stage==Impl::VertexStage::creating||stage==Impl::VertexStage::completed),
            "Vertex native POST has no genuine current subset/table");
        if(stage==Impl::VertexStage::creating){
            phase=impl_->vertex_phase.lock();
            require(phase&&phase->active&&phase->operation==&op&&
                phase->serial==op.operation_serial()&&phase->epoch==epoch&&
                phase->creator==std::this_thread::get_id()&&
                same_vertex_owner(phase->canonical.lock(),phase)&&
                same_vertex_owner(phase->initializer.lock(),owner)&&same_vertex_owner(phase->table,table),
                "Vertex native POST lost the actual lexical phase");
            actor=phase->actor.lock();
            require(bool(actor),"Vertex native POST lost the actual Source7 actor");
            invocations=phase->actual_invocations;in_flight=phase->in_flight_slot;
            for(std::size_t slot=0;slot<keepers.size();++slot){
                engaged[slot]=phase->results[slot].has_value();
                if(engaged[slot]){
                    keepers[slot]=phase->results[slot]->lock();
                    require(bool(keepers[slot]),"Vertex native POST engaged result has permanently expired");
                }
            }
        }else{
            setup=impl_->vertex_setup.lock();
            require(setup&&impl_->vertex_phase.expired(),
                "Vertex native POST lost the genuine completed Setup");
            actor=setup->impl_;
            require(bool(actor),"Vertex native POST completed actor is absent");
            keepers=actor->results;
        }
        const auto cursor=actor->next_slot;
        const auto pure_snapshot=[&]{
            owner->require_vertex_declaration_source_identity_without_callbacks(op);
            require(impl_->vertex_stage==stage&&impl_->epoch==epoch&&
                same_vertex_owner(impl_->vertex_table,table)&&actor&&
                actor->creator==std::this_thread::get_id()&&!actor->failed.load()&&
                actor->next_slot==cursor&&cursor<=table->rows.size()&&
                actor->epoch==epoch&&actor->creation_serial==table->publication_serial&&
                same_vertex_owner(actor->initializer,owner),
                "Vertex native POST actual actor/table/epoch changed");
            table->first(*owner);actor->first_creation();
            if(stage==Impl::VertexStage::creating){
                require(phase&&phase->active&&same_vertex_owner(impl_->vertex_phase.lock(),phase)&&
                    same_vertex_owner(phase->canonical.lock(),phase)&&
                    same_vertex_owner(phase->initializer.lock(),owner)&&same_vertex_owner(phase->table,table)&&
                    same_vertex_owner(phase->actor.lock(),actor)&&phase->operation==&op&&
                    phase->serial==op.operation_serial()&&phase->epoch==epoch&&
                    phase->actual_invocations==invocations&&invocations<=table->positive_rows&&
                    phase->in_flight_slot==in_flight,
                    "Vertex native POST lexical invocation journal changed");
                if(in_flight)require(*in_flight==cursor&&cursor<table->rows.size()&&
                    table->rows[cursor].count&&!engaged[cursor],
                    "Vertex native POST in-flight invocation is not the actual next row");
                std::uint32_t actual_count=0;
                for(std::size_t slot=0;slot<keepers.size();++slot){
                    require(phase->results[slot].has_value()==engaged[slot],
                        "Vertex native POST permanent journal engagement changed");
                    if(engaged[slot]){
                        const auto actual=phase->results[slot]->lock();
                        require(actual&&same_vertex_owner(actual,keepers[slot])&&
                            table->rows[slot].count&&slot<=cursor,
                            "Vertex native POST actual result keeper/slot differs");
                        ++actual_count;
                    }
                    // Before the first actual memset/cursor advance, future
                    // strides are indeterminate. Inspect only completed rows.
                    if(slot<cursor)require(
                        bool(actor->results[slot])==bool(table->rows[slot].count)&&
                        same_vertex_owner(actor->results[slot],keepers[slot])&&
                        actor->strides[slot]==table->rows[slot].stride,
                        "Vertex native POST completed cursor storage differs");
                    if(slot==cursor&&actor->results[slot])require(
                        same_vertex_owner(actor->results[slot],keepers[slot]),
                        "Vertex native POST pending actual actor store differs");
                }
                require(actual_count==invocations,
                    "Vertex native POST count differs from actual invoked results");
            }else{
                const auto actual=impl_->vertex_setup.lock();
                require(setup&&actual&&same_vertex_owner(actual,setup)&&
                    same_vertex_owner(setup->impl_,actor)&&same_vertex_owner(actor->canonical.lock(),setup)&&
                    impl_->vertex_phase.expired()&&cursor==table->rows.size()&&actor->current_create.expired(),
                    "Vertex native POST completed canonical Setup changed");
                for(std::size_t slot=0;slot<keepers.size();++slot)require(
                    same_vertex_owner(actor->results[slot],keepers[slot])&&
                    bool(keepers[slot])==bool(table->rows[slot].count)&&
                    actor->strides[slot]==table->rows[slot].stride,
                    "Vertex native POST completed result/schema slot changed");
            }
            owner->require_vertex_declaration_source_identity_without_callbacks(op);
        };
        pure_snapshot();
        for(std::uint32_t slot=0;slot<keepers.size();++slot){
            if(!keepers[slot])continue; // No created object in this authentic slot.
            // SAME stored Ui observes its original creating-device functions,
            // independent Window-issued Pin and typed actual VAO. It performs
            // no Source FULL/Root gate/context switch or old Create lookup.
            impl_->renderer->require_original_vertex_declaration_device_current_owned(keepers[slot],op);
            pure_snapshot();
            BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_current_without_callbacks(
                *keepers[slot],*owner,epoch,slot,table->rows[slot].schema());
            pure_snapshot();
        }
        pure_snapshot();
    }catch(...){
        if(phase){phase->active=false;phase->in_flight_slot.reset();}
        impl_->vertex_stage=Impl::VertexStage::failed;impl_->stage=Impl::Stage::failed;
        throw;
    }
}

void BnwOriginalNonFontDeviceInitializer::retire_vertex_declarations_owned()const noexcept{
    if(!impl_)return;
    if(const auto phase=impl_->vertex_phase.lock()){phase->active=false;phase->in_flight_slot.reset();}
    impl_->vertex_stage=Impl::VertexStage::retired;
    impl_->vertex_phase.reset();impl_->vertex_setup.reset();impl_->vertex_table.reset();
}

void BnwOriginalNonFontDeviceInitializer::require_manager20da8_sampler_creation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_vertex_declaration_source_identity_without_callbacks(op);
    throw std::runtime_error("Whole8E3A90 manager NativeStorage/ordered intervening actors unissued; declaration subset cannot admit sampler creation");
}
void BnwOriginalNonFontDeviceInitializer::require_manager20da8_sampler_descriptor_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t,
    const BnwOriginalResourceSamplerDescriptorLoan&)const{
    require_vertex_declaration_source_identity_without_callbacks(op);
    throw std::runtime_error("Whole8E3A90 manager/sampler publication unissued; declaration subset is not sampler CURRENT");
}
} // namespace detail
