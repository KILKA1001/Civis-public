// Inside civ5::ui after complete genuine standalone state/Run and Core.
struct detail::BnwNormalLabelPaintInvocation::Impl {
    using Label=NotificationLayoutStandaloneLabelState;using F=Label::Impl;
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::weak_ptr<const BnwWorldViewControlTree> tree;
    std::weak_ptr<const BnwNormalLabelPaintInvocation> self;
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::optional<BnwNormalRootContextEntry> enclosing_context;
    const std::weak_ptr<Label> child;
    const std::shared_ptr<BnwStandalonePositionedLabelRunLoan> run;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t id,serial,sequence,base_sequence,topology,effects,constructor_serial;
    bool normal_returned=false,emitted=false;
    // This fixed route is selected only by the real private normal dispatcher
    // after actual vptrA48DD0 is authenticated. It is not a caller enum.
    static constexpr std::uint32_t selector=1;
    Impl(std::shared_ptr<const BnwWorldViewControlTree> t,std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<Label> label,std::shared_ptr<BnwStandalonePositionedLabelRunLoan> data,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual,const runtime::BnwWorldViewSourceOperationLoan& op,
        std::uint64_t value,std::uint64_t op_serial,std::uint64_t s,std::uint64_t b,std::uint64_t topo,std::uint64_t e,std::uint64_t ctor,
        std::optional<BnwNormalRootContextEntry> entry)
        :tree(t),core(c),enclosing_context(std::move(entry)),child(label),run(std::move(data)),receiver(std::move(actual)),operation(&op),id(value),serial(op_serial),
         sequence(s),base_sequence(b),topology(topo),effects(e),constructor_serial(ctor){}
    const runtime::BnwWorldViewSourceOperationLoan& first_native_entry(bool paint_entered)const{
        if(enclosing_context)enclosing_context->require_current_without_callbacks();
        const bool entered=paint_entered||enclosing_context.has_value();
        require(creator==std::this_thread::get_id()&&alive.load(),"Normal Label invocation retired/foreign before weak borrow");
        const auto actual_tree=tree.lock();const auto owner=core.lock();
        require(actual_tree&&owner&&actual_tree->core_.get()==owner.get()&&!actual_tree->core_.owner_before(owner)&&
            !owner.owner_before(actual_tree->core_),"Normal Label physical Tree/Core retired or differs");
        if(enclosing_context)enclosing_context->require_same_tree_without_callbacks(*actual_tree);
        owner->layout_origin(entered);
        const auto& current=owner->source->current_operation_loan();owner->layout_origin(entered);
        require(&current==operation,"Normal Label canonical Source replaced before saved pointer");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,"Normal Label Source serial changed");
        const auto label=child.lock();require(bool(label),"Normal Label actual standalone allocation retired");
        const auto fields=label->impl_;fields->first();const auto& node=BnwWorldViewControlTree::Core::node(owner->nodes,id);
        const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        require(node.layout_baseline&&node.layout_baseline->kind_==NotificationLayoutBaseline::Kind::label&&
            same(node.layout_baseline->standalone_label_,label)&&!node.layout_baseline->button_&&
            node.layout_baseline->parsed_source_serial_&&*node.layout_baseline->parsed_source_serial_&&
            node.layout_receiver_occurrence&&same(node.layout_receiver_occurrence,fields->occurrence.lock())&&
            node.full_xml_occurrence&&same(node.full_xml_occurrence,fields->selected.lock())&&
            same(node.layout_baseline->base_.receiver_owned,receiver)&&receiver&&
            fields->returned&&fields->wrapper_returned&&fields->fields_returned&&fields->common_prefix_returned&&
            fields->returned->source_serial==constructor_serial&&fields->source_serial==constructor_serial&&
            same(fields->returned->core.lock(),owner)&&same(fields->returned->occurrence.lock(),node.layout_receiver_occurrence)&&
            same(fields->wrapper_returned->core.lock(),owner)&&same(fields->wrapper_returned->receiver.lock(),receiver)&&
            fields->sequence==sequence&&node.layout_baseline->sequence_==base_sequence&&
            owner->topology_revision==topology&&owner->effect_revision==effects,
            "Normal Label genuine constructor/common/receiver/current journals absent or changed");
        const auto* vptr=std::get_if<F::Vptr>(&fields->fields[0]);
        require(vptr&&*vptr==F::Vptr::label_a48dd0,"Normal Label actual physical vptr18 is not standalone A48DD0");
        fields->module->require_standalone_label_live_allocation_without_callbacks(current,fields->returned->ticket,label,constructor_serial);
        require(fields->init_stage==F::InitStage::bounds_returned&&run&&run->impl_&&run->impl_->alive.load()&&
            same(fields->completed_standalone_run,run)&&run->impl_->published&&run->impl_->published->source_serial&&
            run->impl_->published->receiver_return_sequence&&run->impl_->published->topology==topology&&
            run->impl_->id==id&&same(run->impl_->child.lock(),label)&&same(run->impl_->receiver,receiver)&&
            run->impl_->sequence==sequence&&run->impl_->base_sequence==base_sequence&&run->impl_->constructor_serial==constructor_serial,
            "Normal Label missing actual initialized and post-DTOR published standalone Run");
        const auto* cache=std::get_if<F::SystemFontBorrow>(&fields->fields[0x170/4]);
        if(std::holds_alternative<F::Null>(fields->fields[0x170/4]))
            throw std::runtime_error("Normal Label genuine bitmapNULL170 route requires distinct bitmap producer");
        require(cache&&cache->loan&&run->impl_->system&&same(cache->loan,run->impl_->system),
            "Normal Label actual positive system170 missing/foreign");
        const auto* cc=std::get_if<F::FontBorrow>(&fields->fields[0xcc/4]);
        require(cc&&cc->loan&&run->impl_->font&&same(cc->loan,run->impl_->font),"Normal Label actual physical CC differs");
        cc->loan->require_current_without_callbacks();cache->loan->require_current_without_callbacks();
        if(enclosing_context){enclosing_context->require_current_without_callbacks();
            enclosing_context->require_same_tree_without_callbacks(*actual_tree);}
        owner->layout_origin(entered);current.require_origin_without_callbacks();return current;
    }
    const runtime::BnwWorldViewSourceOperationLoan& first(bool entered)const{
        require(normal_returned,"Normal Label genuine normal dispatcher return missing");
        const auto actual=self.lock();require(actual&&actual->impl_.get()==this,"Normal Label actual invocation facade retired");
        return first_native_entry(entered);
    }
    void full(bool entered)const{
        const auto& current=first(entered);current.require_current();first(entered);
    }
    void require_same_enclosing_context_without_callbacks(
        const std::optional<BnwNormalRootContextEntry>& actual)const{
        first(false);
        require(enclosing_context.has_value()==actual.has_value(),
            "Normal Label emission/publication enclosing Context mode differs");
        if(enclosing_context)enclosing_context->require_same_entry_without_callbacks(*actual);
        first(false);
    }
};
struct detail::BnwNormalLabelPaintEmission::Impl {
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::shared_ptr<BnwNormalLabelPaintInvocation::Impl> invocation;
    const std::shared_ptr<const BnwNormalLabelPaintInvocation> invocation_facade;
    const std::shared_ptr<const ComposedUiFrame> origin;
    const std::size_t index,count;
    bool returned=false;
    Impl(std::shared_ptr<BnwNormalLabelPaintInvocation::Impl> call,std::shared_ptr<const BnwNormalLabelPaintInvocation> facade,
        std::shared_ptr<const ComposedUiFrame> frame,std::size_t i)
        :invocation(std::move(call)),invocation_facade(std::move(facade)),origin(std::move(frame)),index(i),count(origin->paints.size()){}
    void first(bool entered)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&returned&&invocation&&origin,
            "Normal Label emission retired/foreign/unreturned before origin borrow");
        invocation->first(entered);require(invocation_facade&&invocation_facade->impl_.get()==invocation.get()&&
            invocation->emitted&&count==origin->paints.size()&&index<count,
            "Normal Label actual emitted index/owned producer frame changed");invocation->first(entered);
    }
};
detail::BnwNormalLabelPaintInvocation::BnwNormalLabelPaintInvocation(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwNormalLabelPaintInvocation::~BnwNormalLabelPaintInvocation(){impl_->alive.store(false);}
detail::BnwNormalLabelPaintEmission::BnwNormalLabelPaintEmission(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwNormalLabelPaintEmission::~BnwNormalLabelPaintEmission(){impl_->alive.store(false);}
detail::BnwNormalLabelPaintInvocation::EmissionIssuerKey::EmissionIssuerKey(const ComposedUiFrame& frame,std::size_t index)
    :frame_(&frame),item_(index<frame.paints.size()?&frame.paints[index]:nullptr),index_(index),count_(frame.paints.size()){
    require(item_&&count_,"Normal Label Ui actual append index absent");
}
std::shared_ptr<const detail::BnwNormalLabelPaintEmission>
detail::BnwNormalLabelPaintInvocation::bind_emitted_index_owned(const EmissionIssuerKey& key,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto call=impl_;const auto*const frame=key.frame_;const auto*const item=key.item_;
    const auto index=key.index_,count=key.count_;
    const auto first=[&]{const auto& current=call->first(false);require(&current==&operation,
        "Normal Label emission requires actual SAME Source before caller operand");
        require(!call->emitted&&frame&&index<count&&frame->paints.size()==count&&&frame->paints[index]==item,
            "Normal Label Ui actual append frame/index/item changed or replayed");call->first(false);};
    first();call->full(false);first();
    const auto facade=call->self.lock();require(facade&&facade.get()==this,"Normal Label emission actual invocation owner differs");first();
    // Own immutable frame clone is copied ONLY from the Ui-only observed
    // actual post-append frame, not a public scene/paint DTO role issuer.
    auto origin=std::make_shared<const ComposedUiFrame>(*frame);first();call->full(false);first();
    auto data=std::make_shared<BnwNormalLabelPaintEmission::Impl>(call,facade,origin,index);first();call->full(false);first();
    auto result=std::shared_ptr<BnwNormalLabelPaintEmission>(new BnwNormalLabelPaintEmission(data));
    first();call->full(false);first();require(data->origin==origin&&data->index==index&&data->count==count,
        "Normal Label own emission allocating POST changed origin/index");
    call->emitted=true;data->returned=true;data->first(false);call->full(false);data->first(false);return result;
}
const ComposedUiFrame& detail::BnwNormalLabelPaintEmission::frame_origin_for_publication_owned()const{
    const auto data=impl_;data->first(false);const auto& out=*data->origin;data->first(false);return out;
}
void detail::BnwNormalLabelPaintEmission::require_publication_owned(const BnwWorldViewControlRenderPublication& publication,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto data=impl_;data->first(false);const auto& current=data->invocation->first(false);
    require(&current==&operation,"Normal Label publication another Source operand");
    data->invocation->require_same_enclosing_context_without_callbacks(publication.enclosing_context_entry_);
    const auto tree=data->invocation->tree.lock();require(tree&&publication.tree_.get()==tree.get()&&
        !publication.tree_.owner_before(tree)&&!tree.owner_before(publication.tree_)&&
        publication.control_&&!publication.root_context_entry_&&
        publication.tree_->core_->checked(*publication.control_)==data->invocation->id&&
        publication.topology_revision_==data->invocation->topology&&publication.effect_revision_==data->invocation->effects&&
        publication.frame_.paints.size()==data->count&&publication.normal_label_emission_.get()==this&&
        std::find(publication.paint_indices_.begin(),publication.paint_indices_.end(),data->index)!=publication.paint_indices_.end(),
        "Normal Label publication actual Tree/control/origin/index association differs");
    data->invocation->full(false);data->first(false);
}
std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>
detail::BnwNormalLabelPaintEmission::run_for_actual_item_owned(const BnwWorldViewControlRenderPublication& publication,
    std::size_t index,const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwLabelActualPaintCallFrame& actual_frame)const{
    // Actual Frame9 lexical FIRST precedes every publication/adjusted item read.
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    frame.require_actual_paint_item_without_callbacks(publication,index);
    const auto data=impl_;data->first(true);const auto call=data->invocation;
    const auto& current=call->first(true);require(&current==&operation,"Normal Label current paint Source differs before saved operand");
    const auto tree=call->tree.lock();require(bool(tree),"Normal Label actual Tree retired");
    frame.require_actual_control_without_callbacks(*tree,call->id);
    require(publication.normal_label_emission_.get()==this&&publication.tree_.get()==tree.get()&&
        !publication.tree_.owner_before(tree)&&!tree.owner_before(publication.tree_)&&
        publication.actual_checked_control_id_for_paint_without_callbacks()==call->id&&
        publication.topology_revision_==call->topology&&publication.effect_revision_==call->effects&&
        publication.frame_.paints.size()==data->count&&data->index<data->count,
        "Normal Label actual publication/frame/control/index association changed");
    frame.require_current_without_callbacks();data->first(true);
    if(index!=data->index)return nullptr; //Known other emitted index; no Label invocation for it.
    require(call->selector==1,"Normal Label closed native route is not proven selector1");
    auto run=tree->standalone_positioned_label_run_for_paint_owned(tree,*publication.control_,current,actual_frame);
    frame.require_actual_paint_item_without_callbacks(publication,index);data->first(true);
    require(run&&run.get()==call->run.get()&&!run.owner_before(call->run)&&!call->run.owner_before(run),
        "Normal Label actual item has no SAME genuine Source8 standalone Run");
    const auto guard=run->capture_guard_for_paint_owned(current,tree,call->id,actual_frame);
    guard.require_current_without_callbacks();require(bool(guard.actual_system_font_without_callbacks()),
        "Normal Label matching invocation requires actual positive170; bitmapNULL remains unsupported");
    guard.require_source_owned(current);data->first(true);frame.require_source_owned(current);
    frame.require_actual_paint_item_without_callbacks(publication,index);data->first(true);return run;
}
BnwWorldViewControlTree::NormalGuiLabelDispatchOwned BnwWorldViewControlTree::dispatch_normal_gui_label_paint_owned(
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,Control actual_control,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    return dispatch_normal_gui_label_paint_impl_owned(std::move(actual_tree),std::move(actual_control),operation,std::nullopt);
}
BnwWorldViewControlTree::NormalGuiLabelDispatchOwned BnwWorldViewControlTree::dispatch_normal_gui_label_paint_owned(
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,Control actual_control,
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalRootContextEntry& entry)const{
    entry.require_current_without_callbacks();
    require(bool(actual_tree),"Context Label dispatcher requires actual Tree");
    entry.require_same_tree_without_callbacks(*actual_tree);
    return dispatch_normal_gui_label_paint_impl_owned(std::move(actual_tree),std::move(actual_control),operation,entry);
}
BnwWorldViewControlTree::NormalGuiLabelDispatchOwned BnwWorldViewControlTree::dispatch_normal_gui_label_paint_impl_owned(
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,Control actual_control,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::optional<detail::BnwNormalRootContextEntry> enclosing)const{
    if(enclosing)enclosing->require_current_without_callbacks();
    require(actual_tree&&actual_tree.get()==this,"Normal GUI dispatcher requires SAME actual Tree");
    if(enclosing)enclosing->require_same_tree_without_callbacks(*actual_tree);
    const bool entered=enclosing.has_value();
    const auto owner=actual_tree->core_;owner->layout_origin(entered);const auto id=owner->checked(actual_control);
    const auto initial_topology=owner->topology_revision,initial_effects=owner->effect_revision;
    const auto canonical=[&]() -> const runtime::BnwWorldViewSourceOperationLoan&{
        if(enclosing){enclosing->require_current_without_callbacks();
            enclosing->require_same_tree_without_callbacks(*actual_tree);}
        owner->layout_origin(entered);require(owner->checked(actual_control)==id&&owner->topology_revision==initial_topology&&
            owner->effect_revision==initial_effects,"Normal GUI actual checked Control/epochs changed");
        const auto& current=owner->source->current_operation_loan();owner->layout_origin(entered);
        require(&current==&operation,"Normal GUI actual current Source before supplied raw operand");
        current.require_origin_without_callbacks();return current;
    };canonical();const auto& node=Core::node(owner->nodes,id);
    if(!node.layout_baseline)throw std::runtime_error("Normal GUI dispatcher real parsed receiver absent");
    const auto& base=*node.layout_baseline;
    if(base.kind_==detail::NotificationLayoutBaseline::Kind::button){
        require(base.button_&&base.button_->child_178_observation,"Normal GUI Button actual178 outcome missing");
        if(*base.button_->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null){
            require(!base.button_->label_178_&&!base.actual_text_command_,"Normal GUI earnedNULL178 has positive caption/text command");
            canonical().require_current();canonical();return NormalGuiLabelDispatchOwned(NormalGuiLabelDispatchOwned::Outcome::button_no_caption,nullptr);
        }
        throw std::runtime_error("Normal GUI positive Button178 caption/+20/selector4 producer required");
    }
    if(base.kind_!=detail::NotificationLayoutBaseline::Kind::label){
        canonical().require_current();canonical();return NormalGuiLabelDispatchOwned(NormalGuiLabelDispatchOwned::Outcome::other_control,nullptr);
    }
    using Loan=detail::BnwNormalLabelPaintInvocation;
    require(base.standalone_label_&&!base.button_&&base.base_.receiver_owned,
        "Normal GUI Label lacks genuine standalone constructor/common receiver");
    const auto label=base.standalone_label_;label->impl_->first();const auto fields=label->impl_;
    const auto run=fields->completed_standalone_run;require(bool(run),"Normal GUI Label actual whole initializer/Run producer missing");
    const auto receiver=base.base_.receiver_owned;const auto sequence=fields->sequence,base_sequence=base.sequence_;
    const auto topology=owner->topology_revision,effects=owner->effect_revision,ctor=fields->source_serial;
    const auto serial=canonical().operation_serial();
    auto data=std::make_shared<Loan::Impl>(actual_tree,owner,label,run,receiver,canonical(),id,serial,sequence,base_sequence,topology,effects,ctor,enclosing);
    canonical();data->first_native_entry(false);canonical().require_current();canonical();data->first_native_entry(false);
    auto result=std::shared_ptr<Loan>(new Loan(data));canonical();data->first_native_entry(false);data->self=result;
    canonical().require_current();canonical();data->first_native_entry(false);
    // Completed port normal dispatch selects native A48DD0+18=5406F0 ->1.
    // All batch/depth/ancestor/AllowColor/clip material operands remain held.
    data->normal_returned=true;data->full(false);canonical();data->first(false);
    return NormalGuiLabelDispatchOwned(NormalGuiLabelDispatchOwned::Outcome::standalone_label,result);
}
std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>
BnwWorldViewControlRenderPublication::normal_label_run_for_actual_item_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelActualPaintCallFrame& actual_frame)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    frame.require_actual_paint_item_without_callbacks(*this,index);
    const auto emission=normal_label_emission_;
    if(!emission){
        // A missing association on a real Label is a required supplier error,
        // never a successful primitive-text fallback.
        require(control_&&!root_context_entry_,"Indexed Label query cannot use Context entry");
        tree_->require_normal_paint_no_label_invocation_owned(*control_,operation);
        frame.require_actual_paint_item_without_callbacks(*this,index);frame.require_current_without_callbacks();return nullptr;
    }
    auto run=emission->run_for_actual_item_owned(*this,index,operation,actual_frame);
    frame.require_actual_paint_item_without_callbacks(*this,index);frame.require_current_without_callbacks();return run;
}
void BnwWorldViewControlTree::require_normal_paint_no_label_invocation_owned(const Control& control,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto owner=core_;owner->layout_origin(true);
    const auto& current=owner->source->current_operation_loan();owner->layout_origin(true);
    require(&current==&operation,"Normal paint no-role query needs fresh current Source before caller operand");
    current.require_origin_without_callbacks();const auto id=owner->checked(control);
    const auto& node=Core::node(owner->nodes,id);require(bool(node.layout_baseline),"Normal paint actual parsed receiver absent");
    if(node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::label)
        throw std::runtime_error("Normal Label actual emitted-index association producer missing");
    if(node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button){
        const auto button=node.layout_baseline->button_;
        require(button&&button->child_178_observation&&
            *button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !button->label_178_&&!node.layout_baseline->actual_text_command_,
            "Normal Button positive/unknown178 caption producer required");
    }
    owner->layout_origin(true);current.require_origin_without_callbacks();
}
