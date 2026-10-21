//Late inside civ5::ui, after complete Common3/Payload5/Paint11/DATA6→8→10.
//Concrete PORT counterpart to native8EC210's 32-byte entry. Native IDs
//are closed opaque controls, and base24+offset is retained OWN arena storage.
//Only seven proved fields are written in the real instruction order below;
//padding A/1A/1C and other unwritten lanes have no value/getter here.
struct detail::NotificationContextGuiPort::Impl::NormalFontDrawEntry final {
    using Binding=graphics::detail::BnwNormalLabelDrawRecordBinding;
    using Prefix=detail::BnwNormalGuiBatchPrefix::Impl;
    static constexpr std::array<std::size_t,14> word_offsets{
        8,0,4,0xc,0x1c,0x20,0x24,0x70,0x74,0x40,0x50,0x60,0x80,0x84};
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Prefix> batch; //No Queue->entry->Batch ownership cycle.
    const std::weak_ptr<const detail::BnwNormalItemOccurrence::Impl> item;
    const std::weak_ptr<detail::BnwNormalLabelMaterialLoan::Impl> material_owner;
    const std::shared_ptr<NormalFontPayload144> storage;
    const Binding::OpaqueRecordIdentity originating_record;
    const Binding::RendererDescriptorReceipt descriptor;
    const std::uint64_t allocation_occurrence,append_occurrence;
    const std::size_t arena_index,slot_index;
    const std::uint32_t actual_offset;
    const std::array<std::uint32_t,14> words;
    //Presence denotes the executed physical store, not constructor defaults.
    std::optional<std::uint32_t> previous_4;
    std::optional<Binding::OpaqueRecordIdentity> draw_record_0;
    std::optional<std::uint16_t> selector_8;
    std::optional<std::uint32_t> reserved_c;
    std::shared_ptr<NormalFontPayload144> source_14;
    std::optional<std::uint16_t> length_18;
    std::optional<Binding::OpaqueMaterialIdentity> material_10;
    bool sealed=false;
    NormalFontDrawEntry(std::weak_ptr<Prefix> b,
        std::weak_ptr<const detail::BnwNormalItemOccurrence::Impl> actual_item,
        std::weak_ptr<detail::BnwNormalLabelMaterialLoan::Impl> actual_material,
        std::shared_ptr<NormalFontPayload144> v,
        Binding::OpaqueRecordIdentity r,Binding::RendererDescriptorReceipt d,
        std::uint64_t allocation,std::uint64_t append,std::size_t arena,std::size_t slot,
        std::uint32_t offset,std::array<std::uint32_t,14> captured_words)
        :batch(std::move(b)),item(std::move(actual_item)),material_owner(std::move(actual_material)),
         storage(std::move(v)),originating_record(std::move(r)),descriptor(std::move(d)),
         allocation_occurrence(allocation),append_occurrence(append),arena_index(arena),slot_index(slot),
         actual_offset(offset),words(captured_words){}
};

//This implements the genuine dependency/checker body. Positive Material mint
//still requires the unissued normal root18/visiblechild18/descriptor finalizer.
struct detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot::LabelTailReturn final {
    using Binding=graphics::detail::BnwNormalLabelDrawRecordBinding;
    using Entry=detail::NotificationContextGuiPort::Impl::NormalFontDrawEntry;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> batch;
    const std::weak_ptr<CommonRoot> root;
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<detail::NotificationLayoutStandaloneLabelState> label;
    const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
    const Binding::Guard binding;
    const std::array<std::shared_ptr<const Entry>,3> entries;
    const std::array<bool,3> pass_present;
    const std::array<std::uint32_t,4> words;
    const std::uint64_t serial,control;const std::size_t ordinal;
    const std::weak_ptr<const BnwNormalItemOccurrence::Impl> item; //No Root->Tail->Item->Root cycle.
    LabelTailReturn(std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> b,std::weak_ptr<CommonRoot> r,
        std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> v,
        std::shared_ptr<detail::NotificationLayoutStandaloneLabelState> l,
        std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> c,Binding::Guard records,
        std::array<std::shared_ptr<const Entry>,3> e,std::array<bool,3> passes,std::array<std::uint32_t,4> physical,
        std::uint64_t s,std::uint64_t id,std::size_t i,std::weak_ptr<const BnwNormalItemOccurrence::Impl> actual_item)
        :batch(std::move(b)),root(std::move(r)),receiver(std::move(v)),
         label(std::move(l)),children(std::move(c)),binding(std::move(records)),entries(std::move(e)),
         pass_present(passes),words(physical),serial(s),control(id),ordinal(i),item(std::move(actual_item)){}
};

struct detail::BnwNormalLabelMaterialLoan::Impl final {
    using Batch=BnwNormalGuiBatchPrefix::Impl;
    using Payload=BnwNormalFontPayloadPrefix::Impl;
    using Binding=graphics::detail::BnwNormalLabelDrawRecordBinding;
    using Entry=detail::NotificationContextGuiPort::Impl::NormalFontDrawEntry;
    struct Scalars final {
        std::array<std::uint32_t,2> translation,depth;
        std::uint32_t alpha,allow_color;
        bool operator==(const Scalars&)const=default;
    };
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::shared_ptr<const BnwWorldViewControlTree> tree;
    const BnwWorldViewControlRenderPublication*const publication;
    const std::size_t index;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    const BnwLabelActualPaintCallFrame::Guard frame;
    const std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> run;
    const std::shared_ptr<const BnwPositionedLabelRunLoan> embedded_run{};
    const std::optional<BnwNormalButtonCaptionCall> caption{};
    const BnwLabelPaintTextLoan::Guard paint;
    const BnwLabelPaintPositionedData::Guard data;
    const std::shared_ptr<BnwWindowedRendererMatrixDataLoan::Impl> matrix; //SAME original Root-bound storage.
    const Binding::Guard binding;
    const BnwNormalRootBatchHistory::Guard root;
    const BnwNormalItemOccurrence::Guard item;
    std::weak_ptr<Impl> canonical_self;
    std::optional<std::array<std::uint32_t,2>> captured_viewport_depth;
    std::optional<std::array<std::int32_t,2>> captured_viewport_dimensions;
    std::optional<BnwWindowedRendererSharedDescriptorIdentity> shared_descriptor;
    std::shared_ptr<Batch> batch;
    std::shared_ptr<Payload> payload;
    std::array<std::optional<Binding::OpaqueRecordIdentity>,3> records;
    std::array<bool,3> pass_present{};
    std::array<std::shared_ptr<const Entry>,3> entries;
    std::optional<std::uint32_t> first_entry_slot;std::size_t completed_entries=0;
    std::optional<Scalars> scalars;
    std::optional<std::uint32_t> completed_selector;
    std::weak_ptr<const BnwNormalVisibleLabelCall::Impl> selected_call;
    std::shared_ptr<const Batch::CommonRoot::LabelTailReturn> child_tail;
    std::optional<std::weak_ptr<const void>> embedded_common56_control; //Once; no Material->Call cycle.
    bool records_captured=false,cpu_child_returned=false,sealed=false;
    Impl(std::shared_ptr<const BnwWorldViewControlTree> t,
        const BnwWorldViewControlRenderPublication* p,std::size_t i,
        const runtime::BnwWorldViewSourceOperationLoan* op,std::uint64_t s,
        BnwLabelActualPaintCallFrame::Guard f,
        std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> r,
        BnwLabelPaintTextLoan::Guard text,BnwLabelPaintPositionedData::Guard d,
        std::shared_ptr<BnwWindowedRendererMatrixDataLoan::Impl> m,Binding::Guard b,
        BnwNormalRootBatchHistory::Guard actual_root,BnwNormalItemOccurrence::Guard actual_item)
        :tree(std::move(t)),publication(p),index(i),operation(op),serial(s),frame(std::move(f)),
         run(std::move(r)),paint(std::move(text)),data(std::move(d)),matrix(std::move(m)),binding(std::move(b)),
         root(std::move(actual_root)),item(std::move(actual_item)){}
    Impl(std::shared_ptr<const BnwWorldViewControlTree> t,
        const BnwWorldViewControlRenderPublication* p,std::size_t i,
        const runtime::BnwWorldViewSourceOperationLoan* op,std::uint64_t s,
        BnwLabelActualPaintCallFrame::Guard f,std::shared_ptr<const BnwPositionedLabelRunLoan> r,
        BnwLabelPaintTextLoan::Guard text,BnwLabelPaintPositionedData::Guard d,
        std::shared_ptr<BnwWindowedRendererMatrixDataLoan::Impl> m,Binding::Guard b,
        BnwNormalRootBatchHistory::Guard actual_root,BnwNormalItemOccurrence::Guard actual_item,
        BnwNormalButtonCaptionCall actual_caption)
        :tree(std::move(t)),publication(p),index(i),operation(op),serial(s),frame(std::move(f)),run(),
         embedded_run(std::move(r)),caption(std::move(actual_caption)),paint(std::move(text)),data(std::move(d)),
         matrix(std::move(m)),binding(std::move(b)),root(std::move(actual_root)),item(std::move(actual_item)){}
    static bool same(const auto& a,const auto& b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    }
    #include "bnw_normal_label_material_historical_core.inc"
    void matrix_physical_first()const{
        root.require_physical_current_without_callbacks();const auto captured=root.impl_;
        require(matrix&&captured&&same(matrix,captured->matrix_facade->impl_)&&
            matrix->matrix==captured->matrix_storage&&matrix->operation==operation&&matrix->serial==serial,
            "Normal Material lacks SAME earned immutable Root Matrix control");
        root.require_physical_current_without_callbacks();
    }
    void origin()const{
        if(sealed){historical_first();return;}
        if(cpu_child_returned){child_return_first();return;}
        if(caption){BnwWorldViewControlTree::require_normal_caption_material_current_without_callbacks(*this);return;}
        require(creator==std::this_thread::get_id()&&alive.load()&&tree&&run,
            "Normal Material retired/foreign/missing genuine owners FIRST");
        //Frame FIRST before any borrowed Publication/Tree or saved Source.
        frame.require_current_without_callbacks();
        const auto owner=tree->core_;owner->layout_origin(true);
        const auto& fresh=owner->source->current_operation_loan();
        require(&fresh==operation,"Normal Material old Source replaced before raw operand");
        fresh.require_origin_without_callbacks();require(fresh.operation_serial()==serial,
            "Normal Material immutable Source serial changed");
        data.require_current_without_callbacks();paint.require_current_without_callbacks();
        data.require_same_paint_guard_without_callbacks(paint);
        const auto p=paint.standalone_;
        require(p&&!paint.impl_&&same(p->tree,tree)&&same(p->run_facade,run)&&
            p->publication==publication&&p->index==index,
            "Normal Material requires SAME distinct standalone Run/Paint/DATA item");
        frame.require_actual_paint_item_without_callbacks(*publication,index);
        frame.require_actual_control_without_callbacks(*tree,p->control);
        matrix_physical_first();binding.require_current_without_callbacks();
        //DATA normal Binding weakslot was earned once by its actual attach;
        //binding's saved DATA is immutable and validated by its own FIRST.
        data.require_current_without_callbacks();paint.require_current_without_callbacks();
        frame.require_current_without_callbacks();matrix_physical_first();
    }
    void capture_runtime_owned(){
        origin();require(!batch&&!payload,"Normal Material batch captured twice");
        const auto gui=tree->core_->context_gui_port;require(bool(gui),"Normal Material canonical Gui absent");
        const auto manager=gui->impl_;const auto b=manager->normal_gui_batch_prefix.lock();
        require(bool(b),"Normal Material actual normal-zero prefix producer not executed/retired");
        BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*b);origin();
        require(same(b->tree,tree)&&b->operation==operation&&b->serial==serial&&
            same(b->gui.lock(),manager)&&same(b->gui_facade.lock(),gui)&&same(b->matrix->impl_,matrix),
            "Normal Material another actual batch/Gui/Matrix/Source");
        if(caption)BnwWorldViewControlTree::require_normal_payload_pending_caption_without_callbacks(*b,data,paint,*caption,item);
        else BnwWorldViewControlTree::require_normal_pending_label_item_without_callbacks(*b,item);
        const auto actual_root=root.impl_;
        if(actual_root->label_traversal_active){selected_call=actual_root->current_visible_label;
            require(!selected_call.expired(),"Normal Material actual pending child call unissued");}
        frame.require_actual_control_without_callbacks(*tree,item.impl_->control);
        batch=b;runtime();
    }
    void runtime()const{
        origin();require(bool(batch),"Normal Material runtime batch predecessor absent");
        if(cpu_child_returned){child_return_first();return;}
        BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);
        if(shared_descriptor)shared_descriptor->require_current_without_callbacks();
        require(same(batch->matrix->impl_,matrix)&&same(batch->tree,tree)&&
            batch->operation==operation&&batch->serial==serial,
            "Normal Material runtime batch association changed");
        if(caption)BnwWorldViewControlTree::require_normal_payload_pending_caption_without_callbacks(*batch,data,paint,*caption,item);
        else BnwWorldViewControlTree::require_normal_pending_label_item_without_callbacks(*batch,item);
        const auto manager=batch->gui.lock();require(manager&&same(manager->normal_gui_batch_prefix.lock(),batch),
            "Normal Material canonical batch producer replaced");
        if(payload){payload->first();require(same(payload->batch,batch)&&same(payload->data.impl_,data.impl_)&&
            same(batch->batch->payload_queue.current_loan.lock(),payload),
            "Normal Material current allocation association replaced");
            payload->paint.require_same_paint_guard_without_callbacks(paint);}
        origin();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        if(sealed){historical_full(supplied);return;}
        if(cpu_child_returned){child_return_full(supplied);return;}
        origin();require(&supplied==operation,"Normal Material another supplied Source");
        frame.require_source_owned(supplied);origin();
        data.require_source_owned(supplied);origin();paint.require_source_owned(supplied);origin();
        root.require_source_owned(supplied);matrix_physical_first();origin();binding.require_source_owned(supplied);origin();
        if(batch){runtime();BnwWorldViewControlTree::require_normal_root_epoch_prefix_source_owned(*batch,supplied);runtime();if(payload){payload->full(supplied);runtime();}}
        frame.require_source_owned(supplied);origin();
    }
    void capture_records_owned(){
        runtime();require(!records_captured,"Normal Material pass records captured twice");
        //Snapshot real DATA membership before Binding's WGL observations. DATA
        //is immutable OWN storage retained by its Guard; no borrowed span crosses
        //callbacks or a store into published Material state.
        std::array<bool,3> present{};
        for(const auto& p:data.passes_without_callbacks()){
            require(p.pass_ordinal<3,"Normal Material DATA pass outside actual3");
            present[p.pass_ordinal]=true;
        }
        runtime();require(present[0],"Normal Material actual591020 positive pass0 prerequisite absent");
        for(unsigned i=0;i<3;++i){if(present[i]){
            auto actual=binding.originating_record_without_callbacks(i);runtime();
            actual.require_current_without_callbacks();actual.require_same_binding_without_callbacks(binding);runtime();
            records[i].emplace(actual);runtime();
        }}
        pass_present=present;records_captured=true;record_current();
    }
    void record_current()const{
        if(cpu_child_returned){child_return_first();return;}
        runtime();require(records_captured&&pass_present[0],"Normal Material actual draw records not captured");
        std::array<bool,3> now{};
        for(const auto& p:data.passes_without_callbacks()){
            require(p.pass_ordinal<3,"Normal Material current DATA pass outside actual3");now[p.pass_ordinal]=true;
        }
        runtime();require(now==pass_present,"Normal Material actual pass membership changed");
        for(unsigned i=0;i<3;++i){
            require(bool(records[i])==pass_present[i],"Normal Material absent pass synthesized/lost");
            if(records[i]){records[i]->require_current_without_callbacks();
                records[i]->require_same_binding_without_callbacks(binding);runtime();}
        }
    }
    Scalars actual_scalars_without_callbacks()const{
        runtime();require(payload&&payload->payload->scalar_stores,"Normal Material genuine scalar writer not completed");
        const auto word=[&](std::size_t at){
            require(at%4==0&&at+4<=payload->payload->storage.size()&&payload->payload->written[at/4],
                "Normal Material attempted unwritten payload word");
            std::array<std::byte,4> bytes;
            for(std::size_t i=0;i<4;++i)bytes[i]=payload->payload->storage[at+i];
            return std::bit_cast<std::uint32_t>(bytes);
        };
        const Scalars result{{word(8),word(0xc)},{word(0x24),word(0x20)},word(0x1c),word(0x40)};
        runtime();return result;
    }
    std::array<std::uint32_t,14> payload_words_physical_without_callbacks()const{
        require(payload&&payload->creator==creator&&payload->alive.load()&&payload->sealed&&
            same(payload->batch,batch)&&payload->payload&&payload->payload->scalar_stores,
            "Normal append requires the real current scalar writer");
        const auto storage=payload->payload;
        const auto& queue=batch->batch->payload_queue;
        require(queue.creator==creator&&queue.alive.load()&&!queue.allocating&&
            queue.allocation_revision==payload->revision&&payload->revision&&queue.used_40==payload->used&&
            queue.length_4c==0x90&&same(queue.current_loan.lock(),payload)&&same(queue.current_30,storage)&&
            !queue.arena.empty()&&same(queue.arena.back(),storage)&&storage->creator==creator&&
            storage->alive.load()&&storage->offset+0x90u==payload->used,
            "Normal physical current allocation control changed");
        std::array<std::uint32_t,14> result;
        std::array<bool,0x90/4> written{};
        for(std::size_t i=0;i<Entry::word_offsets.size();++i){
            const auto at=Entry::word_offsets[i];written[at/4]=true;
            require(storage->written[at/4],"Normal append physical payload word was not written");
            std::array<std::byte,4> bytes;
            for(std::size_t k=0;k<4;++k)bytes[k]=storage->storage[at+k];
            result[i]=std::bit_cast<std::uint32_t>(bytes);
        }
        require(storage->written==written,"Normal append claimed an extra unproved payload lane");
        return result;
    }
    std::array<std::uint32_t,14> actual_payload_words_without_callbacks()const{
        runtime();const auto words=payload_words_physical_without_callbacks();runtime();
        require(words==payload_words_physical_without_callbacks(),
            "Normal physical payload words changed after observing FIRST");return words;
    }
    static void historical_entry_physical_owned(const std::shared_ptr<Batch>& batch,
        const std::shared_ptr<const Entry>& entry){
        const auto creator=batch->creator;
        require(entry&&entry->creator==creator&&entry->sealed&&
            same(entry->batch.lock(),batch),"Normal historical entry retired/foreign/another batch");
        const auto& queue=batch->batch->payload_queue;
        require(queue.creator==creator&&queue.alive.load()&&queue.entry_count_c==queue.entries.size()&&
            queue.append_sequence==queue.entry_count_c&&entry->append_occurrence==entry->slot_index+1&&
            entry->append_occurrence<=queue.append_sequence&&entry->slot_index<queue.entries.size()&&
            same(queue.entries[entry->slot_index],entry)&&entry->allocation_occurrence&&
            entry->allocation_occurrence<=queue.allocation_revision&&entry->arena_index<queue.arena.size()&&
            same(queue.arena[entry->arena_index],entry->storage)&&entry->storage&&
            entry->storage->creator==creator&&entry->storage->alive.load()&&entry->storage->scalar_stores&&
            entry->storage->offset==entry->actual_offset,
            "Normal historical slot/occurrence/storage changed; a current Loan cannot renew it");
        require(entry->previous_4&&*entry->previous_4==UINT32_MAX&&entry->draw_record_0&&
            entry->selector_8&&*entry->selector_8==1&&entry->reserved_c&&*entry->reserved_c==0&&
            same(entry->source_14,entry->storage)&&entry->length_18&&*entry->length_18==0x90&&entry->material_10,
            "Normal historical entry lacks the executed 8EC210 field stores");
        require(same(entry->draw_record_0->impl_,entry->originating_record.impl_)&&
            entry->draw_record_0->pass_==entry->originating_record.pass_,
            "Normal historical physical draw record replaced");
        std::array<bool,0x90/4> written{};
        for(std::size_t i=0;i<Entry::word_offsets.size();++i){
            const auto at=Entry::word_offsets[i];written[at/4]=true;
            require(entry->storage->written[at/4],"Normal historical physical word lost its store");
            std::array<std::byte,4> bytes;
            for(std::size_t k=0;k<4;++k)bytes[k]=entry->storage->storage[at+k];
            require(std::bit_cast<std::uint32_t>(bytes)==entry->words[i],
                "Normal historical payload marked word changed");
        }
        require(entry->storage->written==written,"Normal historical entry read unproved payload lanes");
        //No current30/current_loan/used40/last-arena equality appears here.
        //Earlier completed payloads survive the next real allocation.
    }
    void historical_entry_physical_without_callbacks(const std::shared_ptr<const Entry>& entry)const{
        historical_entry_physical_owned(batch,entry);
    }
    void historical_entry_current(const std::shared_ptr<const Entry>& entry)const{
        runtime();historical_entry_physical_without_callbacks(entry);
        //These are SAME authentic opaque record/descriptor/material controls;
        //all private Graph checks remain inside its actual Material friend.
        entry->originating_record.require_current_without_callbacks();
        entry->originating_record.require_same_binding_without_callbacks(binding);runtime();
        entry->draw_record_0->require_current_without_callbacks();
        entry->draw_record_0->require_same_binding_without_callbacks(binding);runtime();
        entry->descriptor.require_current_without_callbacks();
        entry->descriptor.require_same_binding_without_callbacks(binding);
        entry->descriptor.require_same_record_without_callbacks(*entry->draw_record_0);
        entry->material_10->require_current_without_callbacks();
        entry->material_10->require_same_descriptor_without_callbacks(entry->descriptor);runtime();
        //Physical POST follows the LAST possibly observing current check.
        //The sealed receipt/material checks below are Graph's purely physical
        //historical controls, with no Source/WGL/GL/project allocator calls.
        historical_entry_physical_without_callbacks(entry);
        entry->descriptor.require_current_without_callbacks();
        entry->descriptor.require_same_record_without_callbacks(*entry->draw_record_0);
        entry->material_10->require_current_without_callbacks();
        entry->material_10->require_same_descriptor_without_callbacks(entry->descriptor);
    }
    Scalars completed_scalars_without_callbacks()const{
        runtime();require(bool(first_entry_slot),"Normal Material has no completed historical entries");
        std::optional<Scalars> result;std::size_t ordered=0;
        for(int p=2;p>=0;--p){
            const auto i=static_cast<unsigned>(p);
            require(bool(entries[i])==pass_present[i],"Normal completed entry membership changed");
            if(!entries[i])continue;
            const auto entry=entries[i];historical_entry_current(entry);
            require(entry->slot_index==static_cast<std::size_t>(*first_entry_slot)+ordered&&
                entry->originating_record.pass_==i&&records[i]&&
                same(entry->originating_record.impl_,records[i]->impl_),
                "Normal completed entries are not the genuine descending 2->1->0 sequence");
            const auto& w=entry->words;
            const Scalars value{{w[0],w[3]},{w[6],w[5]},w[4],w[9]};
            if(result)require(*result==value,"Normal completed per-pass scalar values differ");
            else result=value;
            ++ordered;
        }
        require(result&&ordered&&ordered==completed_entries,
            "Normal Material genuine nonempty historical entry count missing");
        runtime();
        for(const auto& entry:entries)if(entry){
            historical_entry_physical_without_callbacks(entry);
            entry->descriptor.require_current_without_callbacks();
            entry->descriptor.require_same_record_without_callbacks(*entry->draw_record_0);
            entry->material_10->require_current_without_callbacks();
            entry->material_10->require_same_descriptor_without_callbacks(entry->descriptor);
        }
        return *result;
    }
    static void completed_label_entries_physical_owned(const Batch& actual){
        const auto root=actual.common_root;require(root&&(root->label_tail||!root->label_returns.empty()),
            "Normal Label540B5F return is not issued");
        const auto validate=[&](const auto& tail){require(bool(tail),"Normal Label returned journal has NULL tail");
        const auto batch=tail->batch.lock();
        require(batch&&batch.get()==&actual&&same(tail->root.lock(),root)&&
            tail->creator==actual.creator&&tail->serial==actual.serial&&tail->ordinal<root->count&&
            same(root->ordered[tail->ordinal],tail->receiver),
            "Normal Label tail has another real call/batch/root/ordinal");
        //Only owned physical entry/receipt controls are checked here. Source,
        //Frame and Matrix FIRST were performed independently by Batch.first.
        require(tail->pass_present[0],"Normal Label actual positive pass0 return absent");
        for(unsigned i=0;i<3;++i){
          const auto& entry=tail->entries[i];
          require(bool(entry)==tail->pass_present[i],"Normal Label actual returned pass membership changed");
          if(entry){
            historical_entry_physical_owned(batch,entry);
            require(entry->originating_record.pass_==i&&entry->draw_record_0->pass_==i,
                "Normal Label actual returned record pass differs");
            entry->descriptor.require_physical_record_without_callbacks();
            entry->descriptor.require_same_binding_physical_without_callbacks(tail->binding);
            entry->descriptor.require_same_record_physical_without_callbacks(*entry->draw_record_0);
            require(same(entry->material_10->impl_,entry->descriptor.impl_),
                "Normal Label physical material has another genuine descriptor control");
          }
        }
        };
        for(const auto& returned:root->label_returns)validate(returned);
        if(root->label_tail)validate(root->label_tail);
    }
    void attach_shared_descriptor_owned(){
        record_current();require(!shared_descriptor,
            "Normal Material canonical shared descriptor attached twice");
        full(*operation);record_current();
        //The getter captures a genuine canonical GUI-owned resource VALUE
        //captured while SAME Matrix was ACTIVE; later reads use earned Root custody.
        const auto identity=root.captured_shared_descriptor_identity_without_callbacks();
        record_current();full(*operation);record_current();
        identity.require_current_without_callbacks();
        //Graph may allocate its closed CODE material association here; it has
        //independent Source/Binding PRE and POST cuts around that operation.
        binding.attach_shared_descriptor_without_callbacks(identity);
        record_current();full(*operation);record_current();
        identity.require_current_without_callbacks();
        const auto current=root.captured_shared_descriptor_identity_without_callbacks();
        record_current();full(*operation);record_current();
        //Pure physical owner POST follows the LAST observing FULL. A stale
        //GUI or a different CODE association cannot be rebound successfully.
        identity.require_current_without_callbacks();
        current.require_current_without_callbacks();
        identity.require_same_owner_without_callbacks(current);
        shared_descriptor.emplace(identity); //OWN VALUE, no facade or reissuer.
    }
    Binding::RendererDescriptorReceipt finalize_current_pass_owned(unsigned pass){
        record_current();require(shared_descriptor&&pass<3&&records[pass]&&payload,
            "Normal descriptor zero requires a genuine nonempty current pass");
        full(*operation);record_current();
        auto descriptor=binding.finalize_renderer_descriptor_without_callbacks(*records[pass]);
        record_current();full(*operation);record_current();return descriptor;
    }
    void append_completed_pass_owned(unsigned pass,Binding::RendererDescriptorReceipt descriptor){
        record_current();require(pass<3&&records[pass]&&!entries[pass]&&payload&&first_entry_slot,
            "Normal append requires a new current allocation and an actual nonempty pass");
        const auto record=*records[pass];const auto current=payload;
        const auto physical=current->payload;const auto actual_batch=batch;
        const auto captured_words=actual_payload_words_without_callbacks();
        descriptor.require_current_without_callbacks();descriptor.require_same_binding_without_callbacks(binding);
        descriptor.require_same_record_without_callbacks(record);record_current();
        const auto material=descriptor.material_identity_without_callbacks();
        material.require_current_without_callbacks();material.require_same_descriptor_without_callbacks(descriptor);
        record_current();full(*operation);record_current();
        auto& queue=actual_batch->batch->payload_queue;
        require(queue.creator==creator&&queue.alive.load()&&!queue.allocating&&!queue.appending&&
            queue.entry_count_c==queue.entries.size()&&queue.append_sequence==queue.entry_count_c&&
            queue.entry_count_c<UINT32_MAX&&queue.append_sequence<UINT64_MAX&&
            queue.entries.size()<queue.entries.max_size()&&
            queue.entry_count_c==static_cast<std::uint64_t>(*first_entry_slot)+completed_entries,
            "Normal actual Queue entry table reentered/overflow/interleaved");
        const auto count=queue.entry_count_c;const auto sequence=queue.append_sequence;
        const auto allocation=queue.allocation_revision;const auto used=queue.used_40;
        const auto arena_count=queue.arena.size();const auto actual_offset=physical->offset;
        const auto actual_length=queue.length_4c;
        require(arena_count&&actual_length==0x90&&current->revision==allocation&&current->used==used&&
            same(queue.current_30,physical)&&same(queue.current_loan.lock(),current)&&
            same(queue.arena.back(),physical),"Normal append current30/4C allocation replaced before commit");
        queue.appending=true;
        struct End{decltype(queue)& q;~End(){q.appending=false;}}end{queue};
        const auto pending_physical=[&]{
            require(queue.creator==creator&&queue.alive.load()&&queue.appending&&!queue.allocating&&
                queue.entry_count_c==count&&queue.entries.size()==count&&queue.append_sequence==sequence&&
                queue.allocation_revision==allocation&&queue.used_40==used&&queue.arena.size()==arena_count&&
                queue.length_4c==actual_length&&same(queue.current_30,physical)&&
                same(queue.current_loan.lock(),current)&&same(payload,current)&&
                same(queue.arena.back(),physical)&&physical->offset==actual_offset&&
                captured_words==payload_words_physical_without_callbacks(),
                "Normal appending OWN entry/current allocation POST changed");
        };
        const auto pending=[&]{
            record_current();pending_physical();
            descriptor.require_current_without_callbacks();descriptor.require_same_binding_without_callbacks(binding);
            descriptor.require_same_record_without_callbacks(record);
            material.require_current_without_callbacks();material.require_same_descriptor_without_callbacks(descriptor);
            record_current();
            for(const auto& previous:entries)if(previous)historical_entry_current(previous);
            //LAST callback has returned: recheck saved physical arguments and
            //pure sealed Graph receipt before any commit/allocator operation.
            pending_physical();
            descriptor.require_current_without_callbacks();descriptor.require_same_record_without_callbacks(record);
            material.require_current_without_callbacks();material.require_same_descriptor_without_callbacks(descriptor);
        };
        try{
            pending();full(*operation);pending();queue.entries.reserve(static_cast<std::size_t>(count)+1);
            pending();full(*operation);pending();
            //All constructor arguments are captured OWN controls/words. No old
            //facade or borrowed Source/entry/payload span is reread by new-handler.
            auto entry=std::make_shared<Entry>(std::weak_ptr<Batch>(actual_batch),item.impl_,canonical_self,
                physical,record,descriptor,
                allocation,sequence+1,arena_count-1,static_cast<std::size_t>(count),actual_offset,captured_words);
            pending();full(*operation);pending();
            //Exact native8EC210 stores:04,00,08,0C,14,18,10,then count0C.
            //Reserved vector and captured shared VALUES keep this commit free
            //of project allocations, Source/WGL/GL calls and external callbacks.
            entry->previous_4.emplace(UINT32_MAX);
            entry->draw_record_0.emplace(record);
            entry->selector_8.emplace(std::uint16_t{1});
            entry->reserved_c.emplace(std::uint32_t{0});
            entry->source_14=physical;
            entry->length_18.emplace(actual_length);
            entry->material_10.emplace(material);
            queue.entries.push_back(entry);
            queue.entry_count_c=count+1;
            queue.append_sequence=sequence+1;entry->sealed=true;
            entries[pass]=entry;++completed_entries;queue.appending=false;
            //Retire only the completed current lease BEFORE the next allocation.
            //Its marked physical storage remains owned by Queue and this entry.
            current->alive.store(false);payload.reset();queue.current_loan.reset();
            record_current();full(*operation);record_current();historical_entry_current(entry);
        }catch(...){
            alive.store(false);current->alive.store(false); //authority FIRST
            queue.alive.store(false);actual_batch->alive.store(false);actual_batch->batch->alive.store(false);
            tree->core_->failed.store(true);throw;
        }
    }
    void require_visible_child_owned()const{
        runtime();
        if(caption)BnwWorldViewControlTree::require_normal_payload_pending_caption_without_callbacks(*batch,data,paint,*caption,item);
        else BnwWorldViewControlTree::require_normal_material_visible_child_without_callbacks(*batch,item);
        runtime();
    }
    void first()const{historical_first();}
};

detail::BnwNormalLabelMaterialLoan::BnwNormalLabelMaterialLoan(std::shared_ptr<Impl> actual)
    :impl_(std::move(actual)){impl_->first();}
detail::BnwNormalLabelMaterialLoan::BnwNormalLabelMaterialLoan(
    std::shared_ptr<Impl> actual,const ChildReturnConstructionKey&):impl_(std::move(actual)){impl_->child_return_first();}
detail::BnwNormalLabelMaterialLoan::~BnwNormalLabelMaterialLoan(){impl_->alive.store(false);}
void detail::BnwNormalLabelMaterialLoan::require_current_without_callbacks()const{
    const auto actual=impl_;actual->first();
}
void detail::BnwNormalLabelMaterialLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto actual=impl_;actual->first();actual->full(op);actual->first();
}
detail::BnwNormalLabelMaterialLoan::ShaderAccess::ShaderAccess(std::shared_ptr<Impl> actual)
    :impl_(std::move(actual)){impl_->first();}
detail::BnwNormalLabelMaterialLoan::ShaderAccess
detail::BnwNormalLabelMaterialLoan::capture_shader_access_without_callbacks()const{
    const auto actual=impl_;actual->first();
    throw runtime::ScriptError("Historical queued Material ShaderAccess requires genuine CURRENT manager-selected entry");
}
void detail::BnwNormalLabelMaterialLoan::ShaderAccess::require_current_without_callbacks()const{
    require(selected_&&selected_record_&&selected_descriptor_,"Queued scalar view lacks actual CURRENT manager permission/record");
    selected_->require_physical_current_without_callbacks();impl_->first();
    selected_->require_same_root_without_callbacks(impl_->root);
    selected_->require_queued_record_without_callbacks(impl_->item,*selected_record_,*selected_descriptor_);
    selected_->require_physical_current_without_callbacks();
}
void detail::BnwNormalLabelMaterialLoan::ShaderAccess::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_current_without_callbacks();selected_->require_source_owned(op);require_current_without_callbacks();
    impl_->full(op);require_current_without_callbacks();
}
void detail::BnwNormalLabelMaterialLoan::ShaderAccess::require_actual_association_without_callbacks(
    const UiScreenGlRenderer& renderer,const BnwWorldViewControlRenderPublication& publication,std::size_t index,
    const BnwWorldViewControlTree& tree,std::uint64_t control,const BnwLabelPaintPositionedData::Guard& data,
    const BnwWindowedRendererMatrixDataLoan::ShaderAccess& matrix)const{
    const auto actual=impl_;const auto input_data=data;
    //Snapshot the matrix control before first Binding/WGL; no borrowed view
    //is accessed again after observations. Storage does not renew its facade.
    const auto input_matrix=matrix.impl_;
    actual->first();input_data.require_current_without_callbacks();input_matrix->first();
    require(actual->tree.get()==&tree&&actual->publication==&publication&&actual->index==index&&
        actual->item.impl_->control==control&&Impl::same(actual->data.impl_,input_data.impl_)&&
        Impl::same(actual->matrix,input_matrix),"Normal Material another actual consumer item/DATA/Matrix");
    actual->frame.require_actual_paint_item_without_callbacks(publication,index);
    actual->frame.require_actual_control_without_callbacks(tree,control);
    actual->frame.require_actual_renderer_without_callbacks(renderer);
    actual->data.require_actual_renderer_without_callbacks(renderer);
    actual->paint.require_actual_renderer_without_callbacks(renderer);
    actual->first();input_data.require_current_without_callbacks();input_matrix->first();
}
using NormalMaterialPair=std::array<std::uint32_t,2>;
using NormalMaterialExtent=std::array<std::int32_t,2>;
#define BNW_NORMAL_MATERIAL_VALUE(method,type,expression) \
type detail::BnwNormalLabelMaterialLoan::ShaderAccess::method()const{ \
    require_current_without_callbacks();const auto result=(expression);require_current_without_callbacks();return result;}
BNW_NORMAL_MATERIAL_VALUE(ordered_translation_words_without_callbacks,NormalMaterialPair,impl_->scalars->translation)
BNW_NORMAL_MATERIAL_VALUE(depth_arg4_arg5_words_without_callbacks,NormalMaterialPair,impl_->scalars->depth)
BNW_NORMAL_MATERIAL_VALUE(composed_alpha_word_without_callbacks,std::uint32_t,impl_->scalars->alpha)
BNW_NORMAL_MATERIAL_VALUE(allow_color_word_without_callbacks,std::uint32_t,impl_->scalars->allow_color)
BNW_NORMAL_MATERIAL_VALUE(native_selector_without_callbacks,std::uint32_t,*impl_->completed_selector)
BNW_NORMAL_MATERIAL_VALUE(viewport_depth_words_without_callbacks,NormalMaterialPair,*impl_->captured_viewport_depth)
BNW_NORMAL_MATERIAL_VALUE(viewport_dimensions_without_callbacks,NormalMaterialExtent,*impl_->captured_viewport_dimensions)
BNW_NORMAL_MATERIAL_VALUE(numeric_controls_without_callbacks,NormalMaterialPair,impl_->item.impl_->numeric)
BNW_NORMAL_MATERIAL_VALUE(normal_batch_revision_without_callbacks,std::uint64_t,impl_->batch->revision)
#undef BNW_NORMAL_MATERIAL_VALUE

void BnwWorldViewControlTree::require_normal_material_visible_child_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& batch,const detail::BnwNormalItemOccurrence::Guard& item){
    require_normal_pending_label_item_without_callbacks(batch,item);
    require_normal_common_root_prefix_without_callbacks(batch);
    const auto root=batch.common_root;
    require(root&&root->stage==detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot::Stage::child18_required&&
        root->children&&root->cursor<root->count,
        "Normal Material genuine root18 visiblechild18 entry unissued; empty prefix/Normal10 not completion");
    //Only actual ordered78 receiver; never generic Node.parent/layout doubles.
    const auto& node=Core::node(batch.tree->core_->nodes,item.impl_->control);
    require(node.layout_baseline&&node.layout_baseline->standalone_label_&&
        root->ordered[root->cursor]==node.layout_baseline->base_.receiver_owned,
        "Normal Material original ordered78 entry is not SAME actual Label");
    require_normal_common_root_prefix_without_callbacks(batch);
}

void BnwWorldViewControlTree::complete_normal_label_tail_owned(
    detail::BnwNormalLabelMaterialLoan::Impl& material)const{
        using Batch=detail::BnwNormalGuiBatchPrefix::Impl;
        const auto batch=material.batch;
        const auto creator=material.creator;const auto serial=material.serial;
        const auto& entries=material.entries;const auto& pass_present=material.pass_present;
        const auto& binding=material.binding;
        const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto record_current=[&]{material.record_current();require(material.tree.get()==this,
            "Normal Label tail producer has another actual Tree");};
        const auto full=[&]{material.full(*material.operation);};
        record_current();full();record_current();
        const auto root=batch->common_root;const auto control=material.item.impl_->control;
        require(root&&root->stage==Batch::CommonRoot::Stage::child18_required&&root->cursor<root->count&&
            !root->label_tail&&!material.payload&&material.completed_selector&&*material.completed_selector==1&&material.scalars&&
            *material.scalars==material.completed_scalars_without_callbacks(),
            "Normal Label tail needs all real per-pass queue entries and its exact pending child call");
        std::shared_ptr<detail::NotificationLayoutStandaloneLabelState> label;
        std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
        std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
        {
            const auto& node=Core::node(core_->nodes,control);
            require(node.layout_baseline&&node.layout_baseline->standalone_label_,
                "Normal Label tail real completed Label receiver absent");
            label=node.layout_baseline->standalone_label_;receiver=node.layout_baseline->base_.receiver_owned;
            children=node.layout_baseline->base_.children_78;
        } //No borrowed Node/Base reference crosses FULL or operator new.
        const auto fields=label->impl_;
        const auto ordinal=root->cursor;
        const auto completed_entries=entries;const auto completed_passes=pass_present;
        const auto completed_binding=binding;
        const auto word=[&](std::size_t at){const auto value=std::get_if<std::uint32_t>(&fields->fields[at/4]);
            require(value,"Normal Label tail physical common word absent");return *value;};
        const std::array<std::uint32_t,4> words{word(0x28),word(0x2c),word(0x44),word(0x40)};
        const auto pending=[&]{
            record_current();
            const auto& node=Core::node(core_->nodes,control);
            require(root==batch->common_root&&root->stage==Batch::CommonRoot::Stage::child18_required&&
                root->cursor==ordinal&&!root->label_tail&&same(root->ordered[ordinal],receiver)&&
                completed_entries==entries&&completed_passes==pass_present&&
                node.layout_baseline&&same(node.layout_baseline->standalone_label_,label)&&
                same(label->impl_,fields)&&fields->creator==creator&&fields->alive.load()&&!fields->failed.load()&&
                same(node.layout_baseline->base_.receiver_owned,receiver)&&same(node.layout_baseline->base_.children_78,children)&&
                same(node.layout_baseline->base_.parent_38.lock(),root->root)&&node.layout_baseline->base_.flags_40==words[3]&&
                std::get_if<decltype(fields)::element_type::Null>(&fields->fields[0x128/4])&&
                std::get_if<decltype(fields)::element_type::Null>(&fields->fields[0x12c/4])&&
                words==std::array<std::uint32_t,4>{word(0x28),word(0x2c),word(0x44),word(0x40)},
                "Normal Label actual tail current128/12C/receiver/words changed");
            //540A08's actual NULL==NULL branch enters540B3E. A genuine
            //nonempty sprite range needs resource getters and selector2 loop.
            //540B54 then really invokes56F630 on this Label's own physical78.
            if(children){
                const auto cell=std::get_if<decltype(fields)::element_type::ChildrenBorrow>(&fields->fields[0x78/4]);
                require(cell&&same(cell->object,children)&&children->impl_&&
                    children->impl_->creator==creator&&
                    same(children->impl_->parent.lock(),receiver)&&children->impl_->ordered.empty(),
                    "Normal Label nonempty actual78 needs the genuine recursive child dispatcher");
            }else require(std::get_if<decltype(fields)::element_type::Null>(&fields->fields[0x78/4]),
                "Normal Label NULL78 current physical branch not earned");
        };
        pending();full();pending();
        auto done=std::make_shared<const Batch::CommonRoot::LabelTailReturn>(std::weak_ptr<Batch>(batch),
            std::weak_ptr<Batch::CommonRoot>(root),receiver,label,children,completed_binding,
            completed_entries,completed_passes,words,serial,control,ordinal,material.item.impl_);
        pending();full();pending();
        root->label_tail=done;material.child_tail=done;
        record_current();full();record_current();require_normal_label_tail_return_without_callbacks(*batch);
}

void BnwWorldViewControlTree::require_normal_label_tail_return_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& actual){
        const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto root=actual.common_root;require(root&&(root->label_tail||!root->label_returns.empty()),
            "Normal Label540B5F physical return is not issued");
        std::optional<std::size_t> previous;
    const auto validate=[&](const auto& tail){require(bool(tail),"Normal Label returned journal has NULL tail");
    const auto batch=tail->batch.lock();
    require(!previous||*previous<tail->ordinal,"Normal Label returned cursor order duplicated/changed");
    previous=tail->ordinal;const auto original_item=tail->item.lock();
    require(bool(original_item),"Normal Label original Item owner retired");
    const detail::BnwNormalItemOccurrence::IssuerKey item_key;
    const detail::BnwNormalItemOccurrence::Guard item(original_item,item_key);
    item.require_physical_current_without_callbacks();
    require(original_item->control==tail->control&&original_item->root.impl_->prefix.get()==&actual,
        "Normal Label returned Item not SAME original Root/control");
        require(batch&&batch.get()==&actual&&same(tail->root.lock(),root)&&
            tail->creator==actual.creator&&tail->serial==actual.serial&&tail->ordinal<root->count&&
            same(root->ordered[tail->ordinal],tail->receiver),
            "Normal Label physical tail has another real call/batch/root/ordinal");
        const auto& node=Core::node(actual.tree->core_->nodes,tail->control);
        require(node.layout_baseline&&same(node.layout_baseline->standalone_label_,tail->label)&&
            same(node.layout_baseline->base_.receiver_owned,tail->receiver)&&
            same(node.layout_baseline->base_.parent_38.lock(),root->root),
            "Normal Label returned receiver no longer matches actual78/38");
        const auto fields=tail->label->impl_;
        require(fields&&fields->creator==actual.creator&&fields->alive.load()&&!fields->failed.load()&&
            std::get_if<typename decltype(fields)::element_type::Null>(&fields->fields[0x128/4])&&
            std::get_if<typename decltype(fields)::element_type::Null>(&fields->fields[0x12c/4]),
            "Normal Label real sprite-empty128/12C return branch changed");
        static constexpr std::array<std::size_t,4> offsets{0x28,0x2c,0x44,0x40};
        for(std::size_t i=0;i<offsets.size();++i){
            const auto word=std::get_if<std::uint32_t>(&fields->fields[offsets[i]/4]);
            require(word&&*word==tail->words[i],"Normal Label returned actual common words changed");
        }
        require(node.layout_baseline->base_.flags_40==tail->words[3]&&
            same(node.layout_baseline->base_.children_78,tail->children),
            "Normal Label returned paired40/78 fields changed");
        if(tail->children){
            const auto cell=std::get_if<typename decltype(fields)::element_type::ChildrenBorrow>(&fields->fields[0x78/4]);
            require(cell&&same(cell->object,tail->children)&&tail->children->impl_&&
                tail->children->impl_->creator==actual.creator&&
                same(tail->children->impl_->parent.lock(),tail->receiver)&&tail->children->impl_->ordered.empty(),
                "Normal Label actual56F630 empty78 return changed");
        }else require(std::get_if<typename decltype(fields)::element_type::Null>(&fields->fields[0x78/4]),
            "Normal Label actual56F630 NULL78 return changed");
        };
    for(const auto& returned:root->label_returns)validate(returned);
    if(root->label_tail)validate(root->label_tail);
    //Graph private controls stay inside their genuine Material friend.
        detail::BnwNormalLabelMaterialLoan::Impl::completed_label_entries_physical_owned(actual);
}

void BnwWorldViewControlTree::enter_normal_selected_label591020_owned(
    detail::BnwNormalLabelMaterialLoan::Impl& material,const detail::BnwNormalGuiBatchPrefix::Impl& actual_batch){
    material.runtime();require(material.batch.get()==&actual_batch,"Normal Label child18 another entered batch");
    require_normal_material_visible_child_without_callbacks(actual_batch,material.item);
    material.full(*material.operation);material.runtime();
    //5406FE calls591020 first. The authentic Graph Binding was already staged
    //from SAME actual DATA/Use/Vertices/accepted textures before Material.
    //Validate its genuine positive170/pass0 and ascending 0,1,2 record members
    //BEFORE Label scalar arithmetic or any144-byte allocation.
    material.capture_records_owned();material.full(*material.operation);material.record_current();
}

std::unique_ptr<detail::BnwNormalLabelMaterialLoan>
BnwWorldViewControlTree::normal_label_material_for_actual_item_owned(
    std::shared_ptr<const BnwWorldViewControlTree> same_tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& publication,std::size_t actual_index,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,
    const std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>& actual_run,
    const detail::BnwLabelPaintTextLoan& actual_paint,const detail::BnwLabelPaintPositionedData& actual_data,
    const detail::BnwWindowedRendererMatrixDataLoan& actual_matrix)const{
    using Loan=detail::BnwNormalLabelMaterialLoan;
    //Capture ALL9 operands before any Guard/Binding WGL/current observation.
    //Guard constructors below copy shared storage only. No facade is cloned.
    const auto tree=std::move(same_tree);const auto owner=core_;const auto run=actual_run;
    const auto frame_control=actual_frame.impl_;const auto paint_control=actual_paint.impl_;
    const auto standalone=actual_paint.standalone_;const auto data_control=actual_data.impl_;
    const auto matrix_control=actual_matrix.impl_;
    const auto actual_root_control=owner->context_gui_port?owner->context_gui_port->impl_->normal_root_history.lock():nullptr;
    const auto actual_item_control=data_control?data_control->normal_item_occurrence.lock():nullptr;
    require(actual_root_control&&actual_item_control,"Fixed9 Material requires earned actual Root/Item history");
    const detail::BnwNormalRootBatchHistory::IssuerKey root_key;
    const detail::BnwNormalItemOccurrence::IssuerKey item_key;
    const detail::BnwNormalRootBatchHistory::Guard root(actual_root_control,root_key);
    const detail::BnwNormalItemOccurrence::Guard item(actual_item_control,item_key);
    const auto frame=detail::BnwLabelActualPaintCallFrame::Guard(frame_control);
    const auto paint=standalone?detail::BnwLabelPaintTextLoan::Guard(standalone):
        detail::BnwLabelPaintTextLoan::Guard(paint_control);
    const auto data=detail::BnwLabelPaintPositionedData::Guard(data_control);
    //Getter internally captures DATA before its Binding/WGL observation.
    const auto binding=data.capture_normal_draw_records_owned(operation);
    //No raw input facade is accessed after the originating getter.
    frame.require_current_without_callbacks();data.require_current_without_callbacks();
    require(tree&&tree.get()==this&&tree->core_==owner&&run,"Normal Material fixed9 actual Tree/Run missing");
    owner->layout_origin(true);const auto& fresh=owner->source->current_operation_loan();
    require(&fresh==&operation,"Normal Material producer requires fresh canonical Source before old operand");
    fresh.require_origin_without_callbacks();const auto serial=fresh.operation_serial();
    root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
    item.require_same_root_without_callbacks(root);item.require_actual_positioned_data_without_callbacks(data);
    item.require_actual_publication_without_callbacks(publication);
    require(actual_item_control->index==actual_index&&actual_root_control->tree.get()==tree.get(),
        "Fixed9 Material another actual Root/Item/index/Tree");
    const auto genuine_planes=item.historical_font_planes_without_callbacks();
    genuine_planes.require_physical_current_without_callbacks();
    root.require_source_owned(operation);item.require_source_owned(operation);
    frame.require_current_without_callbacks();data.require_current_without_callbacks();
    // One ORIGINAL heap Material control exists before entry publication; no stack clone or copied facade.
    auto prepared_owner=[&]{try{return std::make_shared<Loan::Impl>(tree,actual_item_control->publication.get(),actual_index,
        &operation,serial,frame,run,paint,data,matrix_control,binding,root,item);}
        catch(...){owner->failed.store(true);throw;}}();
    auto& prepared=*prepared_owner;prepared.canonical_self=prepared_owner;
    try{
    prepared.origin();prepared.full(operation);prepared.origin();
    prepared.capture_runtime_owned();prepared.full(operation);prepared.runtime();
    if(actual_root_control->label_traversal_active){
        require_normal_pending_label_item_without_callbacks(*prepared.batch,item);
        enter_normal_selected_label591020_owned(prepared,*prepared.batch);
    }else execute_normal_context_selected_label_entry_owned(prepared.batch,&prepared);
    prepared.require_visible_child_owned();prepared.record_current();
    prepared.attach_shared_descriptor_owned();
    prepared.record_current();prepared.full(operation);prepared.record_current();
    //Each actual nonempty descending2->1->0 pass gets a NEW144 allocation,
    //the original fourteen stores, actual descriptor zero, and one selector1
    //entry. Never append three records against one last-current payload.
    auto& queue=prepared.batch->batch->payload_queue;
    require(queue.alive.load()&&!queue.allocating&&!queue.appending&&
        queue.entry_count_c==queue.entries.size()&&queue.append_sequence==queue.entry_count_c,
        "Normal Label entry sequence starts on another/reentered actual Queue");
    prepared.first_entry_slot=queue.entry_count_c;
    for(int p=2;p>=0;--p){
        const auto pass=static_cast<unsigned>(p);
        prepared.record_current();if(!prepared.pass_present[pass])continue;
        require(!prepared.payload,"Normal Label preceding current allocation was not retired");
        prepared.payload=allocate_normal_font_payload_control_owned(prepared.batch,prepared.data,prepared.paint,operation);
        prepared.record_current();prepared.full(operation);prepared.record_current();
        write_normal_font_scalar_control_owned(prepared.payload);
        prepared.record_current();prepared.full(operation);prepared.record_current();
        const auto value=prepared.actual_scalars_without_callbacks();
        if(prepared.scalars)require(*prepared.scalars==value,"Normal Label actual per-pass scalar input changed");
        else prepared.scalars=value;
        prepared.full(operation);prepared.record_current();
        require(*prepared.scalars==prepared.actual_scalars_without_callbacks(),
            "Normal Material pending scalar DATA changed after FULL");
        //PURE Graph operation checks captured physical owners only and executes
        //the genuine descriptor+2C=0 store. Source/WGL/GL/allocating FULL checks
        //are performed independently here before and after that operation.
        prepared.full(operation);prepared.record_current();
        auto descriptor=prepared.finalize_current_pass_owned(pass);
        prepared.record_current();prepared.full(operation);prepared.record_current();
        prepared.append_completed_pass_owned(pass,descriptor);
        prepared.record_current();prepared.full(operation);prepared.record_current();
    }
    require(!prepared.payload&&prepared.scalars&&
        *prepared.scalars==prepared.completed_scalars_without_callbacks(),
        "Normal Label completed historical entry sequence/scalar values missing");
    prepared.completed_selector=1;prepared.full(operation);prepared.record_current();
    complete_normal_label_tail_owned(prepared);
    prepared.captured_viewport_depth=prepared.batch->batch->viewport_depth;
    prepared.captured_viewport_dimensions=prepared.batch->batch->dimensions;
    if(actual_root_control->label_traversal_active){
        require(prepared.selected_call.lock()==actual_root_control->current_visible_label.lock(),
            "Normal Material Label return belongs to another CURRENT actual call");
        prepared.cpu_child_returned=true;prepared.child_return_first();prepared.child_return_full(operation);
        const Loan::ChildReturnConstructionKey key;
        auto result=std::unique_ptr<Loan>(new Loan(prepared_owner,key));
        prepared.child_return_first();prepared.child_return_full(operation);prepared.child_return_first();return result;
    }
    resume_normal_context_common56_after_label_owned(prepared.batch);
    prepared.record_current();prepared.full(operation);prepared.record_current();
    require_normal_root18_return_without_callbacks(*prepared.batch);
    prepared.captured_viewport_depth=prepared.batch->batch->viewport_depth;
    prepared.captured_viewport_dimensions=prepared.batch->batch->dimensions;
    close_normal_root_outer_window_and_register_owned(operation,root);
    prepared.record_current();prepared.full(operation);prepared.record_current();
    require_normal_root_queue_registration_without_callbacks(root);
    // This is queued CPU Material only. Actual Draw remains gated by the independently earned CURRENT manager-entry permission.
    prepared.sealed=true;prepared.first();prepared.full(operation);prepared.first();
    auto result=std::unique_ptr<Loan>(new Loan(prepared_owner));
    prepared.first();prepared.full(operation);prepared.first();return result;
    }catch(...){
        prepared.alive.store(false); //retire authority FIRST, no owner callbacks
        if(prepared.payload){prepared.payload->alive.store(false);prepared.payload->payload->alive.store(false);}
        if(prepared.batch){prepared.batch->alive.store(false);prepared.batch->batch->alive.store(false);}
        owner->failed.store(true);throw;
    }
}
