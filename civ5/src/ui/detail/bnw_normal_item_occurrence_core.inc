// Inside civ5::ui after complete Root/Paint/DATA definitions. CPU association
// only; genuine Graph historical Atlas/planes health is a separate supplier.
struct detail::BnwNormalItemOccurrence::Impl final {
    using Root=BnwNormalRootBatchHistory;using Data=BnwLabelPaintPositionedData;using Paint=BnwLabelPaintTextLoan;
    using Binding=graphics::detail::BnwNormalLabelDrawRecordBinding;
    const std::thread::id creator=std::this_thread::get_id();
    const Root::Guard root;
    const BnwLabelActualPaintCallFrame::RootScopeHealthGuard scope;
    const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
    const std::shared_ptr<const ComposedUiFrame> adjusted_frame;
    const std::shared_ptr<BnwWorldViewControlTree> tree;
    const UiScreenGlRenderer*const renderer;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,control,occurrence;
    const std::size_t index;
    const Paint::Guard paint;
    const Data::Guard data;
    const Binding::Guard binding;
    const std::shared_ptr<Paint::StandaloneImpl> paint_storage;
    const std::shared_ptr<Paint::Impl> embedded_paint_storage{};
    const std::optional<BnwNormalButtonCaptionCall> caption_history{};
    const std::shared_ptr<const BnwNormalButtonCaptionPaintEmission> caption_emission{};
    const std::shared_ptr<Paint::PositionedState> data_storage;
    const std::array<std::uint32_t,2> numeric;
    const std::vector<Data::RecordData> records;
    const std::vector<Data::GlyphData> glyphs;
    const std::vector<Data::PassData> passes;
    mutable std::optional<graphics::GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard> historical_planes;
    mutable std::shared_ptr<graphics::GlBnwSystemFontAtlas> actual_atlas;
    mutable bool planes_installing=false,planes_installed=false;
    bool sealed=false;
    Impl(Root::Guard actual_root,BnwLabelActualPaintCallFrame::RootScopeHealthGuard actual_scope,
        std::shared_ptr<const BnwWorldViewControlRenderPublication> actual_publication,
        std::shared_ptr<const ComposedUiFrame> actual_frame,std::shared_ptr<BnwWorldViewControlTree> actual_tree,
        const UiScreenGlRenderer* actual_renderer,const runtime::BnwWorldViewSourceOperationLoan* actual_operation,
        std::uint64_t actual_serial,std::uint64_t actual_control,std::uint64_t actual_occurrence,std::size_t actual_index,
        Paint::Guard actual_paint,Data::Guard actual_data,Binding::Guard actual_binding,
        std::shared_ptr<Paint::StandaloneImpl> actual_paint_storage,
        std::shared_ptr<Paint::PositionedState> actual_data_storage,std::array<std::uint32_t,2> actual_numeric,
        std::vector<Data::RecordData> actual_records,std::vector<Data::GlyphData> actual_glyphs,
        std::vector<Data::PassData> actual_passes)
        :root(std::move(actual_root)),scope(std::move(actual_scope)),publication(std::move(actual_publication)),
         adjusted_frame(std::move(actual_frame)),tree(std::move(actual_tree)),renderer(actual_renderer),
         operation(actual_operation),serial(actual_serial),control(actual_control),occurrence(actual_occurrence),index(actual_index),
         paint(std::move(actual_paint)),data(std::move(actual_data)),binding(std::move(actual_binding)),
         paint_storage(std::move(actual_paint_storage)),
         data_storage(std::move(actual_data_storage)),numeric(actual_numeric),records(std::move(actual_records)),
         glyphs(std::move(actual_glyphs)),passes(std::move(actual_passes)){}
    Impl(Root::Guard,BnwLabelActualPaintCallFrame::RootScopeHealthGuard,
        std::shared_ptr<const BnwWorldViewControlRenderPublication>,std::shared_ptr<const ComposedUiFrame>,
        std::shared_ptr<BnwWorldViewControlTree>,const UiScreenGlRenderer*,const runtime::BnwWorldViewSourceOperationLoan*,
        std::uint64_t,std::uint64_t,std::uint64_t,std::size_t,Paint::Guard,Data::Guard,Binding::Guard,
        std::shared_ptr<Paint::Impl>,BnwNormalButtonCaptionCall,std::shared_ptr<const BnwNormalButtonCaptionPaintEmission>,
        std::shared_ptr<Paint::PositionedState>,std::array<std::uint32_t,2>,
        std::vector<Data::RecordData>,std::vector<Data::GlyphData>,std::vector<Data::PassData>);
    static bool same(const auto& a,const auto& b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    }
    static bool same_records(const std::vector<Data::RecordData>& a,const std::vector<Data::RecordData>& b){
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i){const auto& x=a[i];const auto& y=b[i];
            if(x.begin!=y.begin||x.byte_count!=y.byte_count||x.style_ordinal!=y.style_ordinal||
                x.source_discriminator!=y.source_discriminator||x.inline_payload!=y.inline_payload||
                x.packed_colors!=y.packed_colors||x.style_pass_selectors!=y.style_pass_selectors)return false;
        }return true;
    }
    static bool same_glyphs(const std::vector<Data::GlyphData>& a,const std::vector<Data::GlyphData>& b){
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i){const auto& x=a[i];const auto& y=b[i];
            if(x.record_ordinal!=y.record_ordinal||x.byte_begin!=y.byte_begin||x.byte_count!=y.byte_count||
                x.codepoint!=y.codepoint||x.lookup_codepoint!=y.lookup_codepoint||x.cursor_x!=y.cursor_x||
                x.line_y!=y.line_y||x.native_bits!=y.native_bits)return false;
        }return true;
    }
    static bool same_passes(const std::vector<Data::PassData>& a,const std::vector<Data::PassData>& b){
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i){const auto& x=a[i];const auto& y=b[i];
            if(x.glyph_ordinal!=y.glyph_ordinal||x.pass_ordinal!=y.pass_ordinal||x.packed_color!=y.packed_color)return false;
        }return true;
    }
    void first()const{
        if(embedded_paint_storage||caption_history||caption_emission){
            BnwWorldViewControlTree::require_normal_caption_item_physical_without_callbacks(*this);return;}
        require(creator==std::this_thread::get_id()&&sealed,"Item history foreign creator/unsealed");
        root.require_physical_current_without_callbacks();scope.require_physical_current_without_callbacks();
        const auto actual_root=root.impl_;
        require(actual_root&&publication&&adjusted_frame&&tree&&renderer&&paint_storage&&data_storage&&
            actual_root->operation==operation&&actual_root->serial==serial&&actual_root->renderer==renderer&&
            same(actual_root->tree,tree)&&occurrence&&occurrence<=actual_root->item_occurrences.size(),
            "Item history physical Root/item owners absent or different");
        const auto registered=actual_root->item_occurrences[occurrence-1].lock();
        require(registered.get()==this&&!actual_root->item_capturing,
            "Item history not the genuine registered distinct Root occurrence");
        const auto* actual_root_scope=std::get_if<Root::Impl::RootScope::Label>(&actual_root->root_scope.value);
        require(actual_root_scope,"Item history requires genuine captured Label Root health");
        scope.require_same_root_scope_without_callbacks(*actual_root_scope);
        scope.require_actual_renderer_without_callbacks(*renderer);
        scope.require_actual_publication_without_callbacks(*publication);
        scope.require_actual_frame_owner_without_callbacks(adjusted_frame);
        const auto original=publication->original_owner_without_callbacks();
        require(same(original,publication)&&same(publication->tree_,tree),
            "Item history original Publication allocation/Tree changed");
        publication->require_adjusted_frame_owner_without_callbacks(adjusted_frame);
        require(publication->actual_checked_control_id_for_paint_without_callbacks()==control&&
            index<adjusted_frame->paints.size()&&
            std::find(publication->paint_indices_.begin(),publication->paint_indices_.end(),index)!=publication->paint_indices_.end(),
            "Item history captured original index/control association changed");
        // These are OWN immutable control associations, not ACTIVE Paint/DATA
        // first or alive bits. Their facades may retire after genuine capture.
        require(!paint.impl_&&same(paint.standalone_,paint_storage)&&paint_storage->creator==creator&&
            same(paint_storage->tree,tree)&&paint_storage->publication==publication.get()&&
            paint_storage->index==index&&paint_storage->control==control&&paint_storage->run_facade&&
            paint_storage->system&&data.impl_&&same(data.impl_->state,data_storage)&&
            data_storage->creator==creator&&same(data_storage->paint.standalone_,paint_storage)&&
            !data_storage->paint.impl_&&same(data_storage->system,paint_storage->system)&&
            data_storage->numeric==numeric&&data.impl_->normal_draw_records_installed&&
            !data.impl_->normal_draw_records_installing&&data.impl_->normal_draw_records_revision==1&&
            bool(data.impl_->normal_draw_records)&&data.impl_->normal_item_occurrence_issued&&
            data.impl_->normal_item_occurrence.lock().get()==this&&same_records(data_storage->records,records)&&
            same_glyphs(data_storage->glyphs,glyphs)&&same_passes(data_storage->passes,passes),
            "Item history genuine original Paint/DATA controls or immutable CPU payload changed");
        require(bool(historical_planes)==planes_installed&&bool(actual_atlas)==planes_installed&&
            (!planes_installing||(!planes_installed&&!historical_planes&&!actual_atlas)),
            "Item history once-only historical Atlas attachment journal differs");
        if(planes_installed){
            historical_planes->require_physical_current_without_callbacks();
            historical_planes->require_actual_atlas_without_callbacks(*actual_atlas);
            historical_planes->require_actual_positioned_data_without_callbacks(data);
        }
        const auto actual_numeric=scope.numeric_controls_without_callbacks();
        require(actual_numeric[0]==numeric[0]&&
            (actual_numeric[1]&~std::uint32_t{0x3f})==(numeric[1]&~std::uint32_t{0x3f}),
            "Item history current root numeric controls differ from captured CPU DATA");
        root.require_physical_current_without_callbacks();scope.require_physical_current_without_callbacks();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Item history another Source operand");
        root.require_source_owned(supplied);first();scope.require_source_owned(supplied);first();
        supplied.require_operation(serial);first();
    }
};

detail::BnwNormalItemOccurrence::Guard::Guard(std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
void detail::BnwNormalItemOccurrence::Guard::require_physical_current_without_callbacks()const{
    const auto saved=impl_;require(bool(saved),"Item history Guard missing own control");saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;require(bool(saved),"Item history Guard missing own control FULL");saved->full(op);saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& root)const{
    const auto saved=impl_;saved->first();saved->root.require_same_root_without_callbacks(root);saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_same_item_without_callbacks(const Guard& item)const{
    const auto saved=impl_;const auto other=item.impl_;saved->first();other->first();
    require(Impl::same(saved,other),"Item history another earned occurrence");saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_actual_renderer_without_callbacks(const UiScreenGlRenderer& renderer)const{
    const auto saved=impl_;saved->first();require(saved->renderer==&renderer,"Item history another actual renderer");saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_actual_publication_without_callbacks(const BnwWorldViewControlRenderPublication& publication)const{
    const auto saved=impl_;saved->first();require(saved->publication.get()==&publication,"Item history another original Publication");saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_actual_control_without_callbacks(const BnwWorldViewControlTree& tree,std::uint64_t control)const{
    const auto saved=impl_;saved->first();require(saved->tree.get()==&tree&&saved->control==control,"Item history another actual Tree/control");saved->first();
}
void detail::BnwNormalItemOccurrence::Guard::require_actual_positioned_data_without_callbacks(const BnwLabelPaintPositionedData::Guard& data)const{
    const auto saved=impl_;saved->first();
    require(Impl::same(saved->data.impl_,data.impl_)&&data.impl_&&Impl::same(data.impl_->state,saved->data_storage),
        "Item history another genuine positioned DATA control");saved->first();
}
graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
detail::BnwNormalItemOccurrence::Guard::captured_normal_draw_records_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto binding=saved->binding;saved->first();return binding;
}

detail::BnwNormalItemOccurrence::Guard BnwWorldViewControlTree::capture_normal_item_occurrence_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalRootBatchHistory::Guard& actual_root,
    const BnwWorldViewControlRenderPublication& actual_publication,std::size_t index,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,const detail::BnwLabelPaintTextLoan& actual_paint,
    const detail::BnwLabelPaintPositionedData& actual_data)const{
    using Item=detail::BnwNormalItemOccurrence;
    const auto root=actual_root;root.require_physical_current_without_callbacks();
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    frame.require_actual_paint_item_without_callbacks(actual_publication,index);
    const auto publication=actual_publication.original_owner_without_callbacks();
    const auto tree=publication->tree_;const auto adjusted=publication->adjusted_frame_owner_;
    const auto paint=actual_paint.capture_guard_without_callbacks();paint.require_current_without_callbacks();
    const auto data=actual_data.capture_guard_without_callbacks();data.require_current_without_callbacks();
    data.require_same_paint_guard_without_callbacks(paint);
    const auto binding=data.capture_normal_draw_records_owned(operation);
    root.require_physical_current_without_callbacks();frame.require_current_without_callbacks();
    const auto captured_root=root.impl_;const auto p=paint.standalone_;const auto physical=data.impl_->state;
    require(captured_root&&tree&&tree.get()==this&&p&&!paint.impl_&&adjusted&&publication->renderer_,
        "Item capture requires genuine supported standalone Paint/Tree/original frame");
    const auto*const renderer=publication->renderer_;const auto control=p->control;
    const auto active_first=[&]{
        root.require_physical_current_without_callbacks();frame.require_current_without_callbacks();
        paint.require_current_without_callbacks();data.require_current_without_callbacks();
        data.require_same_paint_guard_without_callbacks(paint);
        frame.require_actual_paint_item_without_callbacks(*publication,index);
        frame.require_actual_control_without_callbacks(*tree,control);frame.require_actual_renderer_without_callbacks(*renderer);
        const auto& fresh=tree->core_->source->current_operation_loan();
        require(&fresh==&operation,"Item capture fresh Source differs before supplied operand");
        fresh.require_origin_without_callbacks();
        require(captured_root->operation==&operation&&captured_root->serial==fresh.operation_serial()&&
            captured_root->renderer==renderer&&Item::Impl::same(captured_root->tree,tree)&&
            Item::Impl::same(paint.standalone_,p)&&!paint.impl_&&Item::Impl::same(p->tree,tree)&&
            p->publication==publication.get()&&p->index==index&&p->control==control&&
            p->frame.impl_==frame.impl_&&data.impl_&&Item::Impl::same(data.impl_->state,physical)&&
            Item::Impl::same(physical->paint.standalone_,p)&&!physical->paint.impl_&&
            data.impl_->normal_draw_records_installed&&!data.impl_->normal_draw_records_installing&&
            data.impl_->normal_draw_records_revision==1&&bool(data.impl_->normal_draw_records),
            "Item capture requires SAME actual Root/Frame/Paint/DATA with original CPU Binding attached");
        publication->require_adjusted_frame_owner_without_callbacks(adjusted);
        root.require_physical_current_without_callbacks();frame.require_current_without_callbacks();
    };
    const auto active_full=[&]{active_first();root.require_source_owned(operation);active_first();
        frame.require_source_owned(operation);active_first();paint.require_source_owned(operation);active_first();
        data.require_source_owned(operation);active_first();
        data.require_same_normal_draw_records_source_owned(operation,binding);active_first();};
    active_first();active_full();active_first();
    require(!captured_root->item_capturing&&captured_root->item_occurrences.size()<UINT64_MAX&&
        !data.impl_->normal_item_occurrence_issued&&data.impl_->normal_item_occurrence.expired(),
        "Item capture recursive or distinct occurrence space exhausted");
    for(const auto& weak:captured_root->item_occurrences){const auto previous=weak.lock();
        require(!previous||!(previous->publication.get()==publication.get()&&previous->index==index&&
            previous->control==control&&Item::Impl::same(previous->data.impl_,data.impl_)),
            "The exact actual Root item/DATA occurrence cannot be issued twice");
    }
    const auto serial=operation.operation_serial();const auto count=captured_root->item_occurrences.size();
    const auto numeric=physical->numeric;
    captured_root->item_capturing=true;
    struct End{std::shared_ptr<const detail::BnwNormalRootBatchHistory::Impl> root;~End(){root->item_capturing=false;}}end{captured_root};
    try{
    const auto scope=frame.capture_root_scope_health_owned(operation);active_first();active_full();active_first();
    const auto* actual_root_scope=std::get_if<detail::BnwNormalRootBatchHistory::Impl::RootScope::Label>(
        &captured_root->root_scope.value);
    require(actual_root_scope,"Item capture requires genuine captured Label Root health");
    scope.require_same_root_scope_without_callbacks(*actual_root_scope);
    scope.require_actual_frame_owner_without_callbacks(adjusted);
    // Only immutable OWN CPU payload is copied. Retaining original DATA's
    // physical state keeps each allocator input valid; ACTIVE POST can reject
    // facade retirement and never upgrades it to a historical successful item.
    auto records=physical->records;active_first();active_full();active_first();
    auto glyphs=physical->glyphs;active_first();active_full();active_first();
    auto passes=physical->passes;active_first();active_full();active_first();
    const auto pending=[&]{active_first();scope.require_physical_current_without_callbacks();
        require(captured_root->item_capturing&&captured_root->item_occurrences.size()==count&&
            !data.impl_->normal_item_occurrence_issued&&data.impl_->normal_item_occurrence.expired()&&
            operation.operation_serial()==serial&&physical->numeric==numeric,
            "Item capture allocation/FULL POST changed actual Root/item state");
    };
        pending();active_full();pending();
        captured_root->item_occurrences.reserve(count+1);pending();active_full();pending();
        // Every constructor operand is an OWN VALUE before allocator/new-handler.
        auto owned=std::make_shared<Item::Impl>(root,scope,publication,adjusted,tree,renderer,&operation,serial,
            control,count+1,index,paint,data,binding,p,physical,numeric,std::move(records),std::move(glyphs),std::move(passes));
        pending();active_full();pending();
        require(Item::Impl::same_records(owned->records,physical->records)&&
            Item::Impl::same_glyphs(owned->glyphs,physical->glyphs)&&Item::Impl::same_passes(owned->passes,physical->passes),
            "Item capture immutable CPU payload allocation POST differs");
        // Reserved weak journal append and seal execute without allocation/FULL.
        captured_root->item_occurrences.emplace_back(owned);data.impl_->normal_item_occurrence=owned;
        data.impl_->normal_item_occurrence_issued=true;captured_root->item_capturing=false;owned->sealed=true;
        owned->first();owned->full(operation);owned->first();
        const Item::IssuerKey key;return Item::Guard(owned,key);
    }catch(...){const auto state=captured_root->gui.lock();if(state)state->failed.store(true);throw;}
}
