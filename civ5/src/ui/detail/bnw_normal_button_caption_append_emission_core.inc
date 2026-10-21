// Genuine positive178 CURRENT projection and actual Ui append binding. The
// original String/Font/ColorSet parser and initialization are never replayed.
struct detail::BnwNormalButtonCaptionPaintEmission::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalButtonCaptionCall caption;
    const std::shared_ptr<const BnwPositionedLabelRunLoan> run;
    const std::shared_ptr<const BnwNormalCaptionFp32RectLoan> rect;
    const std::shared_ptr<const ComposedUiFrame> origin;
    const WorldViewPaint*const item;
    const std::size_t index,count;
    std::weak_ptr<const Impl> canonical_self;
    std::weak_ptr<const BnwNormalButtonCaptionPaintEmission> canonical_facade;
    bool sealed=false;
    Impl(BnwNormalButtonCaptionCall actual_caption,
        std::shared_ptr<const BnwPositionedLabelRunLoan> actual_run,
        std::shared_ptr<const ComposedUiFrame> actual_frame,
        const WorldViewPaint* actual_item,std::size_t actual_index,std::size_t actual_count,
        std::shared_ptr<const BnwNormalCaptionFp32RectLoan> actual_rect)
        :caption(std::move(actual_caption)),run(std::move(actual_run)),rect(std::move(actual_rect)),origin(std::move(actual_frame)),
         item(actual_item),index(actual_index),count(actual_count){}
};

void BnwWorldViewControlTree::require_normal_button_caption_same_common_without_callbacks(
    const detail::BnwNormalButtonCaptionCall& caption,const detail::BnwNormalCommonChildCall& actual_common){
    const auto common=actual_common;
    require_normal_button_caption_call_current_without_callbacks(caption);
    require_normal_common_child_call_current_without_callbacks(common);
    const auto captured=caption.impl_->enclosing_call.impl_;
    const auto supplied=common.impl_;
    require(captured&&supplied&&captured.get()==supplied.get()&&
        !captured.owner_before(supplied)&&!supplied.owner_before(captured),
        "Caption178 callback is not SAME original CURRENT Common call control");
    require_normal_common_child_call_current_without_callbacks(common);
    require_normal_button_caption_call_current_without_callbacks(caption);
}

detail::BnwNormalButtonCaptionAppendProjection
BnwWorldViewControlTree::project_normal_button_caption_for_append_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalButtonCaptionCall& supplied){
    using Caption=detail::BnwNormalButtonCaptionCall;
    const auto caption=supplied;
    const auto first=[&]{require_normal_button_caption_call_current_without_callbacks(caption);};
    const auto full=[&]{first();require_normal_button_caption_call_source_owned(operation,caption);first();};
    first();const auto saved=caption.impl_;
    require(!saved->projecting,"Caption178 actual caller projection reentered");
    saved->projecting=true;
    struct ProjectionExit final {const std::shared_ptr<const Caption::Impl> owner;
        ~ProjectionExit(){owner->projecting=false;}}projection_exit{saved};
    try{
    full();first();
    const auto history=saved->root.impl_;const auto tree=history->tree;
    const auto& baseline=*Core::node(tree->core_->nodes,saved->control.id_).layout_baseline;
    require(baseline.base_.alpha_word_44.has_value(),
        "Caption178 caller has no genuine physical Button44 word");
    const std::array<std::uint32_t,2> button_position{
        std::bit_cast<std::uint32_t>(baseline.base_.offset_28_2c[0]),
        std::bit_cast<std::uint32_t>(baseline.base_.offset_28_2c[1])};
    const auto button_alpha=*baseline.base_.alpha_word_44;
    const auto incoming_position=saved->enclosing_call.actual_child_offset_words_without_callbacks();first();
    const auto incoming_alpha=saved->enclosing_call.actual_child_alpha_word_without_callbacks();first();
    if(!saved->paint_arguments){
        // This is the real6DA5A0/5C5/5D2 caller arithmetic once, after actual
        // Common RETURN and before actual child18. Later consumers use these
        // SAME owned results; they do not execute this caller a second time.
            first();full();first();
            const auto numeric=saved->root.numeric_controls_without_callbacks();first();
            detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(numeric);
            std::array<std::uint32_t,3> out;
            asm volatile("movss %[alpha], %%xmm0\n\t"
                         "mulss %[parentalpha], %%xmm0\n\t"
                         "movss %%xmm0, %[outalpha]\n\t"
                         "movss %[ownx], %%xmm1\n\t"
                         "addss %[parentx], %%xmm1\n\t"
                         "movss %%xmm1, %[outx]\n\t"
                         "movss %[owny], %%xmm2\n\t"
                         "addss %[parenty], %%xmm2\n\t"
                         "movss %%xmm2, %[outy]\n\t"
                :[outx]"=m"(out[0]),[outy]"=m"(out[1]),[outalpha]"=m"(out[2])
                :[alpha]"m"(button_alpha),[parentalpha]"m"(incoming_alpha),
                 [ownx]"m"(button_position[0]),[owny]"m"(button_position[1]),
                 [parentx]"m"(incoming_position[0]),[parenty]"m"(incoming_position[1])
                :"xmm0","xmm1","xmm2","memory");
            first();detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(numeric);first();
            saved->paint_arguments.emplace(Caption::Impl::PaintArguments{
                incoming_position,incoming_alpha,button_position,button_alpha,out,numeric});
            first();full();first();
    }
    first();const auto arguments=*saved->paint_arguments;
    require(arguments.incoming_position==incoming_position&&arguments.incoming_alpha==incoming_alpha&&
        arguments.button_position==button_position&&arguments.button_alpha==button_alpha,
        "Caption178 actual caller arguments changed after their once-only arithmetic");
    detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(arguments.numeric);first();
    const auto run=saved->run->impl_;const auto font=run->font;
    font->require_current_without_callbacks();first();
    const auto allocation=font->impl_->allocation;
    require(allocation&&allocation->name_40c1c&&allocation->styles_40c20,
        "Caption178 real CC font name/style storage unavailable");
    const auto ordinal=std::uint32_t{run->value.style};
    require(ordinal<allocation->styles_40c20->size(),
        "Caption178 actual record style ordinal is outside genuine CC table");
    const auto& name=allocation->styles_40c20->at(ordinal).name;
    const auto style_end=std::find(name.begin(),name.end(),'\0');
    require(style_end!=name.end(),"Caption178 real native64 style name lacks NUL");
    const auto c4_end=std::find(run->value.c4.begin(),run->value.c4.end(),'\0');
    require(c4_end!=run->value.c4.end(),"Caption178 original C4 lacks actual first NUL");
    const auto style_length=static_cast<std::size_t>(style_end-name.begin());
    const auto c4_length=static_cast<std::size_t>(c4_end-run->value.c4.begin());
    // All owners and lengths precede allocator/new-handler/Source callbacks;
    // no reference into a caller DTO/XML/old borrowed font is retained.
    std::string text(run->value.c4.data(),c4_length);first();full();first();
    std::string font_name=*allocation->name_40c1c;first();full();first();
    std::string style_name(name.data(),style_length);first();full();first();
    const auto selectors=font->style_layer_texture_indices_without_callbacks(ordinal);first();
    const auto metrics=font->metrics_without_callbacks();first();
    const auto geometry=run->value.geometry;const auto label_alpha=run->value.alpha_44;
    const auto colors=run->value.colors;const auto raw_colors=run->value.raw_colors;
    first();full();first();
    detail::BnwNormalButtonCaptionAppendProjection result(std::move(text),std::move(font_name),std::move(style_name),font,
        ordinal,selectors,metrics,geometry,label_alpha,colors,raw_colors,arguments.result,
        arguments.incoming_position,arguments.incoming_alpha,arguments.button_position,arguments.button_alpha,
        arguments.numeric);
    first();full();first();return result;
    }catch(...){const auto root=saved->root.impl_;if(root&&root->tree)root->tree->core_->failed.store(true);throw;}
}

void BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(
    const detail::BnwNormalButtonCaptionPaintEmission& offered){
    const auto data=offered.impl_;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(data&&data->creator==std::this_thread::get_id()&&data->sealed&&same(data->canonical_self.lock(),data),
        "Caption178 emission has foreign/unsealed/noncanonical original control");
    require_normal_button_caption_call_current_without_callbacks(data->caption);
    const auto call=data->caption.impl_;
    const auto actual=data->canonical_facade.lock();
    require(actual&&actual.get()==&offered&&same(actual->impl_,data)&&
        call->original_emission_control&&same(call->original_emission_control->lock(),data)&&
        same(call->run,data->run)&&call->paint_arguments&&data->origin&&data->count&&
        data->origin->paints.size()==data->count&&data->index<data->count&&
        &data->origin->paints[data->index]==data->item,
        "Caption178 actual shared postappend frame/index/item or original once journal changed");
    const auto run=call->run->impl_;const auto& paint=*data->item;
    require(data->rect&&paint.normal_caption_fp32_rect&&same(paint.normal_caption_fp32_rect,data->rect),
        "Caption178 indexed immutable item lost SAME original FP32 loan");
    data->rect->require_current_without_callbacks();
    data->rect->require_same_caption_without_callbacks(data->caption);
    data->rect->require_same_initialized_run_without_callbacks(data->run);
    const auto font=run->font;font->require_current_without_callbacks();
    require_normal_button_caption_call_current_without_callbacks(data->caption);
    const auto allocation=font->impl_->allocation;const auto style=std::uint32_t{run->value.style};
    require(allocation&&allocation->name_40c1c&&allocation->styles_40c20&&style<allocation->styles_40c20->size(),
        "Caption178 emission actual CC name/style unavailable");
    const auto& name=allocation->styles_40c20->at(style).name;
    const auto style_end=std::find(name.begin(),name.end(),'\0');
    const auto text_end=std::find(run->value.c4.begin(),run->value.c4.end(),'\0');
    require(style_end!=name.end()&&text_end!=run->value.c4.end()&&paint.kind==WorldViewPaintKind::text&&
        paint.text==std::string_view(run->value.c4.data(),static_cast<std::size_t>(text_end-run->value.c4.begin()))&&
        paint.font_style&&paint.font_style->font==*allocation->name_40c1c&&
        paint.font_style->style==std::string_view(name.data(),static_cast<std::size_t>(style_end-name.begin())),
        "Caption178 emitted text/font/style do not come from SAME physical C4/CC");
    for(std::size_t p=0;p<3;++p)require(paint.font_style->colors[p]&&
        *paint.font_style->colors[p]==run->value.colors[p],
        "Caption178 emitted layer color is not SAME initialized physical148/14C/150");
    detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(call->paint_arguments->numeric);
    require_normal_button_caption_call_current_without_callbacks(data->caption);
}

void BnwWorldViewControlTree::require_normal_button_caption_emission_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const detail::BnwNormalButtonCaptionPaintEmission& emission){
    require_normal_button_caption_emission_current_without_callbacks(emission);
    require_normal_button_caption_call_source_owned(op,emission.impl_->caption);
    require_normal_button_caption_emission_current_without_callbacks(emission);
}

void BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(
    const detail::BnwNormalButtonCaptionPaintEmission& emission,
    const std::shared_ptr<BnwWorldViewControlTree>& supplied_tree,const Control& supplied_control,
    const detail::BnwNormalRootContextEntry& supplied_entry){
    const auto tree=supplied_tree;const auto control=supplied_control;const auto entry=supplied_entry;
    require_normal_button_caption_emission_current_without_callbacks(emission);
    entry.require_current_without_callbacks();
    const auto caption=emission.impl_->caption.impl_;const auto actual=caption->root.impl_->tree;
    require(tree&&tree.get()==actual.get()&&!tree.owner_before(actual)&&!actual.owner_before(tree)&&
        tree->core_==actual->core_&&tree->core_->checked(control)==caption->control.id_,
        "Caption178 original Publication inputs do not name SAME canonical parent Control/Tree");
    caption->entry.require_same_entry_without_callbacks(entry);
    entry.require_same_tree_without_callbacks(*tree);
    require_normal_button_caption_emission_current_without_callbacks(emission);
    entry.require_current_without_callbacks();
}

std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>
BnwWorldViewControlTree::bind_normal_button_caption_emission_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,const detail::BnwNormalButtonCaptionCall& actual_caption,
    std::shared_ptr<const ComposedUiFrame> actual_frame,const WorldViewPaint* actual_item,
    std::size_t index,std::size_t count){
    using Emission=detail::BnwNormalButtonCaptionPaintEmission;
    const auto caption=actual_caption;const auto frame=std::move(actual_frame);
    // Retain the actual immutable indexed field before any Source/allocator cut.
    const auto rect=frame&&index<frame->paints.size()?frame->paints[index].normal_caption_fp32_rect:
        std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>{};
    const auto first=[&]{require_normal_button_caption_call_current_without_callbacks(caption);
        const auto call=caption.impl_;
        require(!call->original_emission_control&&call->paint_arguments&&!call->projecting&&frame&&count&&
            frame->paints.size()==count&&index<count&&&frame->paints[index]==actual_item,
            "Caption178 Ui postappend origin/index/item changed or original emission replayed");
        const auto& item=frame->paints[index];
        require(rect&&item.normal_caption_fp32_rect.get()==rect.get()&&
            !item.normal_caption_fp32_rect.owner_before(rect)&&!rect.owner_before(item.normal_caption_fp32_rect),
            "Caption178 append carrier is not SAME original rect loan");
        rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(caption);
        rect->require_same_initialized_run_without_callbacks(call->run);};
    const auto full=[&]{first();require_normal_button_caption_call_source_owned(op,caption);first();
        rect->require_source_owned(op);first();};
    first();const auto call=caption.impl_;
    try{
    full();first();
    auto data=std::make_shared<Emission::Impl>(caption,call->run,frame,actual_item,index,count,rect);first();full();first();
    auto result=std::shared_ptr<Emission>(new Emission(data));first();full();first();
    require(data->origin.get()==frame.get()&&!data->origin.owner_before(frame)&&!frame.owner_before(data->origin)&&
        data->item==actual_item&&data->index==index&&data->count==count,
        "Caption178 emission allocation POST changed SAME original Ui append backing");
    // No callback/allocation between sealed original-control publication.
    data->canonical_self=data;data->canonical_facade=result;data->sealed=true;
    call->original_emission_control=std::weak_ptr<const Emission::Impl>(data);
    require_normal_button_caption_emission_current_without_callbacks(*result);
    require_normal_button_caption_emission_source_owned(op,*result);
    require_normal_button_caption_emission_current_without_callbacks(*result);return result;
    }catch(...){const auto root=call->root.impl_;if(root&&root->tree)root->tree->core_->failed.store(true);throw;}
}

void BnwWorldViewControlTree::require_normal_button_caption_emission_publication_owned(
    const detail::BnwNormalButtonCaptionPaintEmission& emission,
    const BnwWorldViewControlRenderPublication& publication,
    const runtime::BnwWorldViewSourceOperationLoan& op){
    const auto first=[&]{require_normal_button_caption_emission_current_without_callbacks(emission);
        const auto data=emission.impl_;const auto call=data->caption.impl_;
        const auto root=call->root.impl_;const auto tree=root->tree;
        require(publication.tree_&&publication.tree_.get()==tree.get()&&
            !publication.tree_.owner_before(tree)&&!tree.owner_before(publication.tree_)&&
            publication.control_&&!publication.root_context_entry_&&publication.enclosing_context_entry_&&
            publication.actual_checked_control_id_for_paint_without_callbacks()==call->control.id_&&
            publication.normal_caption_emission_.get()==&emission&&publication.normal_caption_emission_->impl_==data&&
            !publication.normal_label_emission_&&!publication.context_child_body_&&
            publication.paint_indices_.size()==1&&publication.paint_indices_[0]==data->index&&
            publication.frame_.paints.size()==data->count&&
            publication.frame_.paints[data->index].normal_caption_fp32_rect&&
            publication.frame_.paints[data->index].normal_caption_fp32_rect.get()==data->rect.get()&&
            !publication.frame_.paints[data->index].normal_caption_fp32_rect.owner_before(data->rect)&&
            !data->rect.owner_before(publication.frame_.paints[data->index].normal_caption_fp32_rect)&&
            publication.topology_revision_==tree->core_->topology_revision&&
            publication.effect_revision_==tree->core_->effect_revision,
            "Caption178 original Publication Tree/Control/Entry/span association differs");
        call->entry.require_same_entry_without_callbacks(*publication.enclosing_context_entry_);
        if(publication.adjusted_frame_owner_){
            publication.require_adjusted_frame_owner_without_callbacks(data->origin);
            require(publication.adjusted_frame_owner_.get()==data->origin.get()&&
                !publication.adjusted_frame_owner_.owner_before(data->origin)&&
                !data->origin.owner_before(publication.adjusted_frame_owner_),
                "Caption178 Publication lost SAME physical Ui append carrier");}
        require_normal_button_caption_emission_current_without_callbacks(emission);};
    first();require_normal_button_caption_emission_source_owned(op,emission);first();
}

std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
BnwWorldViewControlTree::normal_button_caption_run_for_actual_item_owned(
    const detail::BnwNormalButtonCaptionPaintEmission& supplied,
    const BnwWorldViewControlRenderPublication& publication,std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& op,const detail::BnwLabelActualPaintCallFrame& actual_frame){
    // Capture the genuine ACTIVE Frame by value before publication/source
    // callbacks. Strong originals do not renew the lexical Frame authority.
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    const auto emission=supplied.impl_;
    require_normal_button_caption_emission_current_without_callbacks(supplied);
    const auto caption=emission->caption;const auto call=caption.impl_;
    const auto tree=call->root.impl_->tree;const auto run=call->run;
    const auto rect=emission->rect;
    std::optional<detail::BnwPositionedLabelRunLoan::Guard> entered_guard;
    const auto first=[&]{frame.require_current_without_callbacks();
        require_normal_button_caption_emission_current_without_callbacks(supplied);
        frame.require_actual_paint_item_without_callbacks(publication,index);
        frame.require_actual_control_without_callbacks(*tree,call->control.id_);
        require(index==emission->index&&publication.adjusted_frame_painting_,
            "Caption178 Run requested for another actual emitted item or outside original paint");
        publication.require_adjusted_frame_owner_without_callbacks(emission->origin);
        require(bool(rect),"Caption178 entered Run missing SAME preappend rect owner");
        rect->require_same_initialized_run_without_callbacks(run);
        if(entered_guard){entered_guard->require_current_without_callbacks();
            rect->require_same_entered_run_without_callbacks(*entered_guard);}
        frame.require_current_without_callbacks();};
    const auto full=[&]{first();require_normal_button_caption_emission_publication_owned(supplied,publication,op);first();
        frame.require_source_owned(op);first();};
    first();full();first();
    const auto entered=run->capture_guard_for_paint_owned(op,tree,call->control.id_,frame);
    entered_guard.emplace(entered);first();
    entered.require_current_without_callbacks();entered.require_actual_control_without_callbacks(*tree,call->control.id_);
    require(entered.impl_.get()==run->impl_.get()&&!entered.impl_.owner_before(run->impl_)&&
        !run->impl_.owner_before(entered.impl_),"Caption178 paint Run is not SAME original completed178");
    first();full();first();entered.require_current_without_callbacks();return run;
}

void detail::BnwNormalButtonCaptionCall::require_same_common_call_without_callbacks(
    const BnwNormalCommonChildCall& common)const{
    BnwWorldViewControlTree::require_normal_button_caption_same_common_without_callbacks(*this,common);
}
detail::BnwNormalButtonCaptionAppendProjection
detail::BnwNormalButtonCaptionCall::projection_for_append_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    return BnwWorldViewControlTree::project_normal_button_caption_for_append_owned(op,*this);
}
detail::BnwNormalButtonCaptionCall::EmissionIssuerKey::EmissionIssuerKey(
    std::shared_ptr<const ComposedUiFrame> frame,std::size_t index)
    :frame_(std::move(frame)),item_(frame_&&index<frame_->paints.size()?&frame_->paints[index]:nullptr),
     index_(index),count_(frame_?frame_->paints.size():0){
    require(frame_&&item_&&count_,"Caption178 Ui observed real postappend carrier/index unavailable");
}
std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>
detail::BnwNormalButtonCaptionCall::bind_emitted_index_owned(const EmissionIssuerKey& key,
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto caption=*this;caption.require_current_without_callbacks();
    const auto frame=key.frame_;const auto*const item=key.item_;const auto index=key.index_,count=key.count_;
    caption.require_current_without_callbacks();
    return BnwWorldViewControlTree::bind_normal_button_caption_emission_owned(op,caption,frame,item,index,count);
}
detail::BnwNormalButtonCaptionPaintEmission::BnwNormalButtonCaptionPaintEmission(std::shared_ptr<const Impl> data)
    :impl_(std::move(data)){}
const ComposedUiFrame& detail::BnwNormalButtonCaptionPaintEmission::frame_origin_for_publication_owned()const{
    BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(*this);
    const auto& out=*impl_->origin;
    BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(*this);return out;
}
void detail::BnwNormalButtonCaptionPaintEmission::require_publication_owned(
    const BnwWorldViewControlRenderPublication& pub,const runtime::BnwWorldViewSourceOperationLoan& op)const{
    BnwWorldViewControlTree::require_normal_button_caption_emission_publication_owned(*this,pub,op);
}
std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
detail::BnwNormalButtonCaptionPaintEmission::run_for_actual_item_owned(
    const BnwWorldViewControlRenderPublication& pub,std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& op,const BnwLabelActualPaintCallFrame& frame)const{
    return BnwWorldViewControlTree::normal_button_caption_run_for_actual_item_owned(*this,pub,index,op,frame);
}
