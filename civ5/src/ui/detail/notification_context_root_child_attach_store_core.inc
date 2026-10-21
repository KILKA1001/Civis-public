// Inside civ5::ui. Executable known673730 subsequence behind REQUIRED genuine
// RootRead/factory/v64 suppliers. Their missing bodies do not yield an issuer.
// No normalized Contextid0 or common parser receipt converts to this scope.
struct BnwWorldViewControlTree::ContextRootAttachEvidence final {
    using Read=detail::NotificationContextRootReadChildrenLoan;
    using Controls=detail::NotificationLayoutNativeChildControlsReturn;
    using Receiver=detail::NotificationLayoutOrdinaryReceiver;
    using Realm=detail::NotificationLayoutContextRealmRef;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<Core> core;
    const Read::Guard read;
    const Controls::Guard controls;
    const std::shared_ptr<const Receiver> root,child;
    const std::shared_ptr<const Realm> inherited,context_this;
    const std::weak_ptr<detail::NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> completion;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t child_id;
    const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> initial_array;
    const std::vector<std::shared_ptr<const Receiver>> before_members;
    enum class Stage {before38,inherited3c,append78,storage20,context3c,returned};
    Stage stage=Stage::before38;
    std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> ordered;
    std::size_t before_count=0,append_index=0;
    std::shared_ptr<const Realm> before_value;
    ContextRootAttachEvidence(std::shared_ptr<Core> actual,Read::Guard read_guard,Controls::Guard controls_guard,
        std::shared_ptr<const Receiver> actual_root,std::shared_ptr<const Receiver> actual_child,
        std::shared_ptr<const Realm> inherited_value,std::shared_ptr<const Realm> actual_context,
        std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> physical,
        std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> pending,
        const runtime::BnwWorldViewSourceOperationLoan* op,std::uint64_t actual_child_id,
        std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> array,
        std::vector<std::shared_ptr<const Receiver>> members)
        :core(std::move(actual)),read(std::move(read_guard)),controls(std::move(controls_guard)),
         root(std::move(actual_root)),child(std::move(actual_child)),inherited(std::move(inherited_value)),
         context_this(std::move(actual_context)),context(std::move(physical)),completion(std::move(pending)),
         operation(op),child_id(actual_child_id),initial_array(std::move(array)),before_members(std::move(members)),
         ordered(initial_array),before_count(before_members.size()),append_index(before_members.size()){}
    void first()const {
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "RootAttach lexical evidence retired/foreign/failed FIRST");
        read.require_current_without_callbacks();controls.require_current_without_callbacks();
        BnwWorldViewControlTree::require_context_root_attach_returned_factory_identity_without_callbacks(read,controls);
        const auto actual=core.lock();const auto owned=context.lock();
        require(actual&&owned&&root&&child&&context_this&&completion,
            "RootAttach actual constructor/RootRead physical owners retired");
        // Required Read guard validates its actual Source pointer/scope BEFORE
        // any old raw op call. This is a genuine entered-busy attachment lane.
        actual->layout_origin(true);
        require(&actual->source->current_operation_loan()==operation&&
            actual->context_root_attach_evidence.lock().get()==this&&
            actual->context_ordinary_state==owned&&actual->context_layout_baseline&&
            actual->context_layout_baseline->base_.receiver_owned==root&&
            read.actual_root_receiver_without_callbacks()==root&&
            read.actual_context_realm_without_callbacks()==context_this&&
            controls.actual_receiver_without_callbacks()==child,
            "RootAttach SAME read/Context/receiver/current Source changed");
        const auto& node=Core::node(actual->nodes,child_id);
        require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
            node.layout_baseline->base_.receiver_owned==child&&child->impl_&&
            child->impl_->creator==std::this_thread::get_id()&&child->impl_->canonical.lock()==child&&
            child->impl_->core.lock()==actual&&child->impl_->occurrence.lock()==node.layout_receiver_occurrence,
            "RootAttach child physical registered control/constructor changed");
        const auto& base=node.layout_baseline->base_;
        if(stage!=Stage::before38){
            require(base.parent_38.lock()==root,"RootAttach genuine38 store changed");
            if(node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::label){
                const auto label=node.layout_baseline->standalone_label_;
                require(label&&label->impl_,"RootAttach actual standalone native38 allocation missing");
                label->impl_->first();
                const auto* parent=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::ParentReceiverBorrow>(
                    &label->impl_->fields[0x38/4]);
                require(parent&&detail::NotificationLayoutContextOrdinaryState::Impl::same(parent->value.lock(),root),
                    "RootAttach standalone physical38 differs from paired Baseline parent");
            }
        }
        if(stage==Stage::inherited3c||stage==Stage::context3c){
            const auto value=stage==Stage::inherited3c?inherited:context_this;
            require(base.context_realm_3c==before_value||base.context_realm_3c==value,
                "RootAttach3C outside actual before-value/this-stage store");
        }
        if(stage==Stage::append78||stage==Stage::storage20)require(base.context_realm_3c==inherited,
            "RootAttach append before genuine inherited3C completion");
        if(stage==Stage::returned)require(base.context_realm_3c==context_this,
            "RootAttach returned DATA before genuine ContextTHIS3C completion");
        const auto actual_array=actual->context_layout_baseline->base_.children_78;
        const bool appended=stage==Stage::context3c||stage==Stage::returned;
        require(before_count==before_members.size()&&append_index==before_count,
            "RootAttach immutable parent78 original count changed");
        if(initial_array)require(actual_array==initial_array,"RootAttach actual initial parent78 allocation replaced");
        if(actual_array){
            require(actual_array==ordered&&actual_array->impl_&&actual_array->impl_->parent.lock()==root&&
                actual_array->impl_->ordered.size()==before_count+(appended?1u:0u)&&
                std::equal(before_members.begin(),before_members.end(),actual_array->impl_->ordered.begin()),
                "RootAttach original immutable parent78 ordered prefix/count changed");
            if(appended)require(actual_array->impl_->ordered[append_index]==child,
                "RootAttach true78 append membership changed");
            else require(std::find(actual_array->impl_->ordered.begin(),actual_array->impl_->ordered.end(),child)==actual_array->impl_->ordered.end(),
                "RootAttach early/replayed child78 membership");
            if(stage==Stage::storage20||appended)require(actual_array->impl_->native_storage_byte20&&
                *actual_array->impl_->native_storage_byte20==0,"RootAttach actual44BF6C byte20 clear missing");
        }else require(!before_count&&!appended&&!initial_array&&!ordered,
            "RootAttach actual parent78 storage is missing");
    }
    void full()const {first();read.require_source_owned(*operation);first();controls.require_source_owned(*operation);first();}
};

struct detail::NotificationContextRootChildAttachStoreLoan::Impl final {
    const std::shared_ptr<BnwWorldViewControlTree::ContextRootAttachEvidence> evidence;
    explicit Impl(std::shared_ptr<BnwWorldViewControlTree::ContextRootAttachEvidence> actual):evidence(std::move(actual)){}
};
detail::NotificationContextRootChildAttachStoreLoan::NotificationContextRootChildAttachStoreLoan(std::shared_ptr<Impl> data)
    :impl_(std::move(data)){}
detail::NotificationContextRootChildAttachStoreLoan::~NotificationContextRootChildAttachStoreLoan(){impl_->evidence->alive.store(false);}
detail::NotificationContextRootChildAttachStoreLoan::Guard::Guard(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::NotificationContextRootChildAttachStoreLoan::Guard
detail::NotificationContextRootChildAttachStoreLoan::capture_guard_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationLayoutOrdinaryReceiver& receiver)const{
    const auto saved=impl_;Guard result(saved);result.require_current_without_callbacks(operation,receiver);return result;
}
void detail::NotificationContextRootChildAttachStoreLoan::Guard::require_current_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationLayoutOrdinaryReceiver& receiver)const{
    const auto saved=impl_;require(saved&&saved->evidence,"RootAttach StoreGuard physical owner absent");
    const auto evidence=saved->evidence;evidence->first();
    require(evidence->operation==&operation&&evidence->child.get()==&receiver&&
        (evidence->stage==BnwWorldViewControlTree::ContextRootAttachEvidence::Stage::inherited3c||
         evidence->stage==BnwWorldViewControlTree::ContextRootAttachEvidence::Stage::context3c),
        "RootAttach StoreGuard not SAME actual3C call stage/receiver/Source");
}
void detail::NotificationContextRootChildAttachStoreLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationLayoutOrdinaryReceiver& receiver)const{
    const auto saved=impl_;require_current_without_callbacks(operation,receiver);
    saved->evidence->full();require_current_without_callbacks(operation,receiver);
}
std::shared_ptr<const detail::NotificationLayoutContextRealmRef>
detail::NotificationContextRootChildAttachStoreLoan::Guard::value_3c_without_callbacks()const{
    const auto evidence=impl_->evidence;evidence->first();
    require(evidence->stage==BnwWorldViewControlTree::ContextRootAttachEvidence::Stage::inherited3c||
        evidence->stage==BnwWorldViewControlTree::ContextRootAttachEvidence::Stage::context3c,
        "RootAttach3C DATA outside genuine store stage");
    return evidence->stage==BnwWorldViewControlTree::ContextRootAttachEvidence::Stage::inherited3c?
        evidence->inherited:evidence->context_this;
}

std::shared_ptr<const detail::NotificationContextRootChildAttachReturn>
BnwWorldViewControlTree::attach_context_returned_child_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::NotificationContextRootReadChildrenLoan& actual_read,
    const detail::NotificationLayoutNativeChildControlsReturn& actual_controls){
    using Evidence=ContextRootAttachEvidence;using State=detail::NotificationLayoutContextOrdinaryState;
    using Store=detail::NotificationContextRootChildAttachStoreLoan;
    const auto actual=core_;const auto read=actual_read.capture_guard_without_callbacks();
    read.require_current_without_callbacks();
    const auto controls=actual_controls.capture_guard_for_attach_owned(operation,actual_read);
    read.require_current_without_callbacks();controls.require_current_without_callbacks();
    controls.require_same_root_read_without_callbacks(actual_read);
    require_context_root_attach_returned_factory_identity_without_callbacks(read,controls);
    const auto root=read.actual_root_receiver_without_callbacks();const auto child=controls.actual_receiver_without_callbacks();
    const auto realm=read.actual_context_realm_without_callbacks();
    const auto completion=read.capture_completion_slot_without_callbacks();
    require(root&&child&&realm&&completion&&root->impl_&&child->impl_&&actual->context_layout_baseline,
        "RootAttach cannot invent NULL/root/child/Realm/completion operands");
    const auto context=actual->context_ordinary_state;const auto inherited=actual->context_layout_baseline->base_.context_realm_3c;
    const auto child_id=child->impl_->id;
    const auto initial_array=actual->context_layout_baseline->base_.children_78;
    require(!initial_array||(initial_array->impl_&&initial_array->impl_->parent.lock()==root),
        "RootAttach actual parent78 constructor owner changed");
    const auto initial_count=initial_array?initial_array->impl_->ordered.size():0;
    std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> members;
    const auto before=[&]{
        read.require_current_without_callbacks();controls.require_current_without_callbacks();
        actual->layout_origin(true);require(&actual->source->current_operation_loan()==&operation,
            "RootAttach canonical Source replaced before raw operation");
        require(actual->context_root_attach_evidence.expired()&&actual->context_ordinary_state==context&&
            actual->context_layout_baseline&&actual->context_layout_baseline->base_.receiver_owned==root&&
            actual->context_layout_baseline->base_.context_realm_3c==inherited&&
            actual->context_layout_baseline->base_.children_78==initial_array&&
            (!initial_array||(initial_array->impl_&&initial_array->impl_->ordered.size()==initial_count&&
                members.size()<=initial_count&&std::equal(members.begin(),members.end(),initial_array->impl_->ordered.begin()))),
            "RootAttach actual root/pre-append78 preimage changed before entry");
    };
    const auto full_before=[&]{before();read.require_source_owned(operation);before();controls.require_source_owned(operation);before();};
    std::shared_ptr<Evidence> evidence;
    try{
        full_before();members.reserve(initial_count);full_before();
        if(initial_array)for(const auto& member:initial_array->impl_->ordered)members.push_back(member);
        full_before();require(members.size()==initial_count,"RootAttach immutable parent78 prefix incomplete");
        evidence=std::make_shared<Evidence>(actual,read,controls,root,child,inherited,realm,context,completion,&operation,child_id,
            initial_array,members);
        full_before();actual->context_root_attach_evidence=evidence;evidence->full();
        auto data=std::make_shared<Store::Impl>(evidence);evidence->full();
        auto store=std::unique_ptr<Store>(new Store(data));evidence->full();
        const auto before3c=Core::node(actual->nodes,child_id).layout_baseline->base_.context_realm_3c;
        //44BF40 exact first write. No Source callback or allocator here.
        if(Core::node(actual->nodes,child_id).layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::label)
            store_standalone_root_parent_38_owned(operation,actual_read,actual_controls);
        else Core::mutable_node_owned(actual->nodes,child_id).layout_baseline->base_.parent_38=root;
        evidence->before_value=before3c;evidence->stage=Evidence::Stage::inherited3c;evidence->full();
        store_context_root_attach_3c_for_actual_receiver_owned(*store,operation,*child);evidence->full();
        require(Core::node(actual->nodes,child_id).layout_baseline->base_.context_realm_3c==inherited,
            "RootAttach inherited actual3C store not completed");
        evidence->stage=Evidence::Stage::append78;evidence->full();
        auto array=actual->context_layout_baseline->base_.children_78;
        if(!array){
            // Lazy495E10 only after genuine child return. No fabricated empty78
            // on the no-child/all-native-factory-NULL Root path.
            auto fields=std::make_shared<detail::NotificationLayoutOrdinaryChildren78::Impl>(root);evidence->full();
            auto created=std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78>(
                new detail::NotificationLayoutOrdinaryChildren78(fields));evidence->full();
            require(!actual->context_layout_baseline->base_.children_78,"RootAttach78 changed during allocation");
            actual->context_layout_baseline->base_.children_78=created;array=std::move(created);evidence->ordered=array;evidence->full();
        }
        require(array->impl_&&array->impl_->parent.lock()==root,"RootAttach78 foreign allocation");
        require(evidence->ordered==array&&array->impl_->ordered.size()==evidence->before_count,
            "RootAttach78 actual pre-append array changed");
        // Native44BF6C clears byte[actual78+20] BEFORE the capacity helper.
        array->impl_->native_storage_byte20=0;evidence->stage=Evidence::Stage::storage20;evidence->full();
        const auto retained_array=array;const auto retained_child=child;
        std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> growth;
        require(evidence->before_count<growth.max_size(),"RootAttach78 count overflow");
        growth.reserve(evidence->before_count+1);evidence->full();
        // Allocate only LOCAL owned empty storage. The original immutable
        // prefix is copied without allocation, then actual78 swaps storage.
        for(const auto& member:evidence->before_members)growth.push_back(member);
        retained_array->impl_->ordered.swap(growth);evidence->first();
        retained_array->impl_->ordered.push_back(retained_child); // owned capacity, no callbacks
        evidence->before_value=inherited;evidence->stage=Evidence::Stage::context3c;evidence->full();
        //67373E overrides inherited3C ONLY after actual38/78 append returned.
        store_context_root_attach_3c_for_actual_receiver_owned(*store,operation,*child);evidence->full();
        require(Core::node(actual->nodes,child_id).layout_baseline->base_.context_realm_3c==realm,
            "RootAttach ContextTHIS3C override missing");
        evidence->stage=Evidence::Stage::returned;evidence->full();
        auto returned=publish_context_root_child_attach_return_owned(operation,evidence);evidence->full();
        // Publication is DATA. Initializer checks require the separate actual
        // wholeRootRead completion slot AFTER all temporary DTORs/FULL POST.
        evidence->alive.store(false);actual->context_root_attach_evidence.reset();return returned;
    }catch(...){
        if(evidence){evidence->alive.store(false);evidence->failed.store(true);}
        actual->failed.store(true);
        if(evidence&&actual->context_root_attach_evidence.lock()==evidence)actual->context_root_attach_evidence.reset();
        throw; // partial38/3C/78 is retained; no rollback/fake cleanup/return
    }
}
