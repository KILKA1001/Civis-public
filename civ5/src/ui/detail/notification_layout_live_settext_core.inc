// Inside civ5::ui AFTER complete genuine InitLoan/Run/Parent Binding.
// Native Button55F140 в†’ existing178 Label611930 в†’ initialized6A2EA0.
// Sole issuer is the real Tree::set_text branch with checked actual Control.
void BnwWorldViewControlTree::set_embedded_label_text_live_owned(const Control& control,std::string input){
    using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
    using Child=detail::NotificationLayoutEmbeddedLabelState;using F=Child::Impl;
    using Run=detail::BnwPositionedLabelRunLoan;using Loan=Child::InitializationLoan;
    const auto core=core_;core->layout_origin();
    const auto actual_control=control;
    const auto& operation=core->source->current_operation_loan();core->layout_origin();
    const auto serial=operation.operation_serial();
    const auto id=core->checked(actual_control);
    std::shared_ptr<View::Binding> binding;
    std::shared_ptr<Child> child;
    std::shared_ptr<Run> pending;
    std::shared_ptr<const Run> retired_run;
    std::vector<Parent::FourCReturn> anchoring_returns;
    const auto entry=[&](bool busy){core->layout_origin(busy);
        const auto& current=core->source->current_operation_loan();core->layout_origin(busy);
        require(&current==&operation&&serial&&current.operation_serial()==serial,
            "Live SetText canonical fresh Source was replaced before borrowed operation access");
        current.require_origin_without_callbacks();core->layout_origin(busy);
        require(
            core->checked(actual_control)==id,"Live SetText actual Control/Source expired");
        const auto& n=Core::node(core->nodes,id);
        require(n.layout_baseline&&n.layout_receiver_occurrence&&n.layout_baseline->parsed_source_serial_&&
            n.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button&&
            n.layout_baseline->actual_type_=="Button"&&n.layout_baseline->button_,
            "Live SetText needs genuine parsed allocated Button, never flat tag/ID");};
    try{
        entry(false);
        //Before entering any busy receiver, validate any genuine old Run in
        //its existing cold guard scope. Later busy checks use exact child DATA
        //and lexical setter membership, never relax that cold guard predicate.
        {const auto& n=Core::node(core->nodes,id);const auto button=n.layout_baseline->button_;
         if(button->label_178_&&button->label_178_->impl_->completed_run){
            retired_run=button->label_178_->impl_->completed_run;
            retired_run->impl_->first(operation);entry(false);
            //End of pure admission. Retire this exact old DATA artifact before
            //any fresh setter Source callback, Binding/View allocation or copy.
            retired_run->impl_->alive.store(false);
            button->label_178_->impl_->completed_run.reset();entry(false);}}
        core->layout_current_owned();entry(false);
        core->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{core->busy};entry(true);
        binding=std::make_shared<View::Binding>(core,operation);entry(true);
        {
            View view(binding,binding->captured);view.require_pure();
            const Parent::NodeKey key{0,id};auto& parent=*Parent::node(view,key).state;
            require(parent.history_.size()<parent.history_.max_size()&&parent.sequence_<UINT64_MAX,
                "Button55F140 parent journal exhausted");
            parent.history_.reserve(parent.history_.size()+1);view.require_full();view.require_pure();
            //55F143 ALWAYS precedes the178 test, including actual NULL.
            view.preflight_store_owned(key);parent.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            parent.base_.flags_40|=0x08000000u;view.record_store_owned(key,Parent::Store::Kind::button);view.require_pure();
            const auto button=parent.button_;require(button->child_178_observation.has_value(),
                "Button55F14A actual178 observation is unknown");
            if(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null){
                require(!button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_,
                    "Button55F14A actual NULL178 contradicted by physical child");
                throw std::runtime_error("Button55F153 dynamic703120/38/3C/slot10 factory supplier required after real OR40 prefix");
            }
            require(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::factory_child&&
                button->label_178_&&button->label_178_factory_return_&&!button->pending_label_178_,
                "Button55F190 actual existing178 allocation absent");
            child=button->label_178_;const auto fields=child->impl_;fields->first();
            const auto* flag=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
            require(flag,"Label6A actual initialized flag unknown");const bool initialized=bool(*flag&1u);
            //A current initialized receiver must already have a closed old Run.
            //It is checked before retirement; never republished or renewed.
            if(initialized){require(retired_run&&fields->init_stage==F::InitStage::bounds_returned,
                "Live SetText initialized child lacks genuine old completed Run");
                require(!fields->completed_run&&!retired_run->impl_->alive.load(),
                    "Live SetText old Run withdrawal changed after pure admission");
                //Old alive=false stays false. Validate physical child VALUE
                //only; no old Guard/cold Source currency is renewed.
                require(retired_run->impl_->child.lock().get()==child.get(),
                    "Live SetText withdrawn Run belongs to another actual child");
                const auto admitted_child=retired_run->impl_->child.lock();
                require(admitted_child&&!admitted_child.owner_before(child)&&!child.owner_before(admitted_child)&&
                    fields->sequence==retired_run->impl_->child_sequence,
                    "Live SetText actual child journal changed after old Run admission");
                fields->first();view.require_pure();
                require(retired_run->impl_->published||retired_run->impl_->live_published,
                    "Live SetText old Run was never genuinely published");
            }else require(!retired_run,"Live SetText uninitialized bit contradicts old completed Run");
            //Pure entry already withdrew initialized old Run. No second
            //withdrawal, new old-Run grant or pending pointer reset occurs here.
            require(!fields->completed_run,"Live SetText completed slot reappeared after early withdrawal");
            view.require_pure();fields->first();
            auto loan=std::shared_ptr<Loan>(new Loan(view,key,*this,actual_control));loan->full();
            require(fields->history.max_size()-fields->history.size()>=256&&fields->sequence<=UINT64_MAX-256,
                "Live SetText child journal exhausted before611930");
            fields->history.reserve(fields->history.size()+256);loan->full();
            //Existing public host owns a separate std::string input allocation.
            //Its buffer cannot be the private current TextObject byte buffer,
            //even if bytes compare equal. Never invent a content-equality skip.
            const auto bound=std::min<std::size_t>(input.size(),0x4000);
            const auto stop=std::find(input.begin(),input.begin()+bound,'\0');
            const auto length=static_cast<std::size_t>(stop-input.begin());
            std::string command_mirror(input.data(),length);loan->full();
            std::string render_mirror=command_mirror;loan->full();
            const auto* old=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);
            if(old){
                const auto object=old->object;require(object&&object->creator==std::this_thread::get_id()&&!object->retired.load(),
                    "Label61196F actual old C4 already retired");
                const auto allocation=object->allocation_child.lock();
                require(allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
                    !fields.owner_before(allocation)&&object->source_serial==fields->source_serial,
                    "Label61196F old C4 belongs to a different child allocation");
                //Genuine OWN x64 analogue of pool vslot4: independent retirement
                //of THIS object, with no claim to native pool addresses/counters.
                loan->child_store(0xc4);object->retired.store(true);loan->first();
            }else require(std::holds_alternative<F::Null>(fields->fields[0xc4/4]),
                "Label611960 unknown C4 cannot substitute native NULL");
            auto object=std::make_shared<F::TextObject>();loan->full();
            object->allocation_child=fields;object->source_serial=fields->source_serial;
            object->bytes.resize(length+1);loan->full();
            //611984 stores actual allocation BEFORE61198F copies length+1 bytes.
            loan->child_store(0xc4);fields->fields[0xc4/4]=F::TextBorrow{object};loan->first();
            std::copy_n(input.data(),length,object->bytes.data());object->bytes[length]='\0';loan->first();
            require(object->bytes.size()==length+1&&
                std::equal(input.begin(),input.begin()+length,object->bytes.begin())&&object->bytes.back()=='\0',
                "Label61198F real C4 copy changed");loan->full();
            //6A2EA3 always executes, including same-content or empty input.
            const auto* current_flag=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
            require(current_flag&&bool(*current_flag&1u)==initialized,
                "Live SetText actual initialized bit changed across611930");
            if(initialized){
                fields->init_stage=F::InitStage::live_reinit_entered;loan->first();
                child->rebuild_initialized_layout_owned(loan);loan->full();
                require(fields->init_stage==F::InitStage::bounds_returned,
                    "Live SetText6A/5A/7505/5077/4A actual return incomplete");
                pending=child->make_positioned_run_pending_owned(loan);loan->full();
                require(pending&&pending->impl_->origin==Run::Impl::Origin::live_set_text&&
                    !pending->impl_->published&&!pending->impl_->live_published,
                    "Live SetText pending Run has wrong issuer or premature return");
            } //Actual6A2F0E uninitialized return has NO Run/paint grant.
            //Derived host command/text caches follow the real C4/reinit return.
            //They issue no native mutation or readiness; both buffers were
            //allocated under the fresh lexical guard before old C4 retirement.
            loan->first();parent.actual_text_command_=std::move(command_mirror);
            core->nodes.at(id-1).text=std::move(render_mirror);loan->first();
            view.require_full();loan->first();anchoring_returns=view.four_c_returns_;loan->first();
        } //Actual lexical loan retirement and View DTOR, BEFORE publication.
        binding->pure(false);require(binding->destruction_passed&&!binding->graph_active,
            "Live SetText true reinit View destruction did not pass");
        //Preserve genuine4C return observers for ALL reached domains.
        for(const auto& stamp:anchoring_returns){const auto& d=binding->domains.at(stamp.key.domain);
            const Core::LayoutFourCReturn result{stamp.actual_offset,stamp.baseline_sequence,serial,d.topology,stamp.offset_store_sequence};
            if(stamp.key.id==0)d.core->context_layout_four_c_return=result;
            else d.core->nodes.at(stamp.key.id-1).layout_four_c_return=result;}
        //Save POD post-DTOR epoch only; destroy Binding BEFORE full Source POST.
        const auto topology=binding->domains.front().topology,effects=binding->domains.front().effects;
        const auto reached_domains=binding->domains;binding->pure(false);
        binding->pure(false);binding->finished=true;binding.reset();
        //Cold Run/Realm checks retain their separate busy=false contract.
        //No historical busy=true parser/init authority is extended past DTOR.
        core->busy=false;
        const auto post=[&]{entry(false);require(core->topology_revision==topology&&core->effect_revision==effects&&
            child&&child->impl_->alive.load()&&!child->impl_->failed.load(),
            "Live SetText post-DTOR canonical epoch/child changed");
            for(const auto& d:reached_domains){d.core->layout_origin();
                require(d.core->topology_revision==d.topology&&d.core->effect_revision==d.effects&&
                    d.core->nodes.size()==d.node_count,"Live SetText reached ancestor domain changed after actual DTOR");}
            child->impl_->first();const auto& n=Core::node(core->nodes,id);
            require(n.layout_baseline->button_->label_178_.get()==child.get()&&
                !n.layout_baseline->button_->label_178_.owner_before(child)&&
                !child.owner_before(n.layout_baseline->button_->label_178_),
                "Live SetText post-DTOR actual178 changed");};
        post();operation.require_current();post();
        if(pending){
            const auto data=pending->impl_;data->validate_target_without_callbacks(operation);post();
            require(!child->impl_->completed_run&&!data->published&&!data->live_published&&
                data->origin==Run::Impl::Origin::live_set_text,
                "Live Run current publication slot/issuer changed");
            //First store is pending custody ONLY. No typed live return yet, so
            //every Guard still refuses during the full post-store callbacks.
            child->impl_->completed_run=pending;
            operation.require_current();post();data->validate_target_without_callbacks(operation);post();
            require(child->impl_->completed_run.get()==pending.get()&&
                !child->impl_->completed_run.owner_before(pending)&&!pending.owner_before(child->impl_->completed_run),
                "Live Run completed pointer/control changed across FULL POST");
            //Final POD seal AFTER actual complete return and both FULL bookends.
            //No callback or allocation follows this store; only pure validation.
            data->live_published=Run::Impl::PublishedSetTextReturn{
                serial,data->child_sequence,topology,effects,data->text.get()};
            data->first(operation);entry(false);
        }else {entry(false);} //Earned uninitialized no-rebuild path only.
    }catch(...){if(binding)binding->poison();if(child)child->impl_->failed.store(true);core->failed.store(true);throw;}
}
