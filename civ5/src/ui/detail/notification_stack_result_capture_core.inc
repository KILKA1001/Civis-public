//Inside ui after complete Core and Parent::Binding; no new solver/DTO authority.
std::vector<BnwWorldViewControlTree::Core::StackPending>
BnwWorldViewControlTree::Core::capture_stack_view_owned(detail::NotificationLayoutParentReceiver::TreeView& view){
    using Parent=detail::NotificationLayoutParentReceiver;using Kind=Parent::Store::Kind;
    view.require_pure();std::vector<StackPending> result;
    const auto stores=view.stores_; //successful real target/FULL already returned.
    for(const auto& store:stores){
        if(store.kind!=Kind::stack_size&&store.kind!=Kind::stack_anchor)continue;
        const auto c=view.binding_->domains.at(store.key.domain).core;view.require_pure();
        require(store.key.id!=0,"Stack result cannot be a Context metadata root");
        const auto& n=node(c->nodes,store.key.id);const auto& b=*n.layout_baseline;
        require(b.kind_==detail::NotificationLayoutBaseline::Kind::stack&&b.stack_&&b.parsed_source_serial_&&
            n.layout_receiver_occurrence,"Stack real returned store lacks canonical parser/allocation");
        auto old=std::find_if(result.begin(),result.end(),[&](const auto& p){return p.core.lock()==c&&p.id==store.key.id;});
        if(old!=result.end()){old->size_returned|=store.kind==Kind::stack_size;old->anchor_returned|=store.kind==Kind::stack_anchor;continue;}
        StackPending p;p.core=c;p.id=store.key.id;p.parser_serial=*b.parsed_source_serial_;
        p.occurrence=n.layout_receiver_occurrence;p.constructor_serial=n.layout_receiver_occurrence->constructor_serial;
        p.baseline_sequence=b.sequence_;p.size=b.base_.size_30_34;p.extent=b.base_.extent_20_24;
        p.offset=b.base_.offset_28_2c;p.bounds=b.base_.bounds_10_1c;p.anchor={b.stack_->field_90,b.stack_->field_94};
        p.size_returned=std::any_of(stores.begin(),stores.end(),[&](const auto& s){return s.key==store.key&&s.kind==Kind::stack_size;});
        p.anchor_returned=std::any_of(stores.begin(),stores.end(),[&](const auto& s){return s.key==store.key&&s.kind==Kind::stack_anchor;});
        for(const auto child:Parent::children(view,store.key)){
            view.require_pure();const auto cc=view.binding_->domains.at(child.domain).core;
            const auto& childview=Parent::node(view,child);const auto& cb=*childview.state;
            const auto occurrence=child.id?node(cc->nodes,child.id).layout_receiver_occurrence:cc->context_receiver_occurrence;
            require(occurrence&&cb.parsed_source_serial_&&childview.parent&&*childview.parent==store.key,
                "Stack derived child is not a real canonical attached member");
            StackPending::Child row{cc,child.id,childview.attached_order,store.key.id,*cb.parsed_source_serial_,
                occurrence->constructor_serial,occurrence,{},cb.base_.offset_28_2c,cb.base_.size_30_34,false};
            if(p.size_returned){const auto actual=std::find_if(view.child_positions_.rbegin(),view.child_positions_.rend(),
                [&](const auto& x){return x.stack==store.key&&x.child==child;});
                require(actual!=view.child_positions_.rend(),"Stack size return lacks real child cursor output");
                row.derived_offset=actual->position;row.derived_returned=true;}
            p.children.push_back(std::move(row));
        }
        view.require_pure();result.push_back(std::move(p));
    }
    view.require_pure();return result;
}
void BnwWorldViewControlTree::Core::merge_stack_results_owned(std::vector<StackPending>& into,std::vector<StackPending> next){
    for(auto& p:next){const auto c=p.core.lock();require(bool(c),"Stack result merge owner retired");
        auto old=std::find_if(into.begin(),into.end(),[&](const auto& q){return q.core.lock()==c&&q.id==p.id;});
        if(old==into.end()){into.push_back(std::move(p));continue;}
        require(old->occurrence.lock()==p.occurrence.lock()&&old->constructor_serial==p.constructor_serial&&
            old->parser_serial==p.parser_serial,"Stack merge replaced canonical occurrence");
        if(!p.size_returned&&old->size_returned){
            require(p.children.size()==old->children.size(),"Stack anchor lost actual derived child membership");
            for(std::size_t i=0;i<p.children.size();++i){const auto& a=old->children[i];auto& z=p.children[i];
                require(a.core.lock()==z.core.lock()&&a.id==z.id&&a.order==z.order&&a.occurrence.lock()==z.occurrence.lock(),
                    "Stack anchor derived member changed after size return");
                z.derived_offset=a.derived_offset;z.derived_returned=a.derived_returned;}
        }
        p.size_returned|=old->size_returned;p.anchor_returned|=old->anchor_returned;
        p.empty_initializer|=old->empty_initializer;p.member_finished|=old->member_finished;*old=std::move(p);
    }
}
void BnwWorldViewControlTree::Core::require_stack_pending_owned(const StackPending& p,bool expected_busy){
    const auto c=p.core.lock();require(bool(c),"Stack result actual Core retired");c->layout_origin(expected_busy);
    const auto& n=node(c->nodes,p.id);const auto occurrence=p.occurrence.lock();
    require(n.alive&&n.layout_baseline&&n.layout_baseline->stack_&&n.layout_receiver_occurrence==occurrence&&occurrence&&
        occurrence->constructor_serial==p.constructor_serial&&n.layout_baseline->parsed_source_serial_==p.parser_serial&&
        n.layout_baseline->sequence_==p.baseline_sequence,"Stack result stale allocation/parser/store history");
    c->require_native_context_child_vector_history_owned(expected_busy);
    struct ActualChild {std::shared_ptr<Core> core;std::uint64_t id,order;};std::vector<ActualChild> actual_children;
    for(const auto& child:c->nodes)if(child.alive&&child.parent==p.id)actual_children.push_back({c,child.id,child.attached_order});
    require(c->context_layout_children.has_value(),"Stack result current genuine Context child vector absent");
    for(const auto& child:*c->context_layout_children)if(child.parent_control_id==p.id){const auto cc=child.core.lock();
        require(bool(cc),"Stack current attached Context child retired");actual_children.push_back({cc,0,child.attached_order});}
    std::sort(actual_children.begin(),actual_children.end(),[](const auto& a,const auto& b){return a.order<b.order;});
    require(actual_children.size()==p.children.size(),"Stack result complete actual child membership changed");
    for(std::size_t i=0;i<p.children.size();++i)require(actual_children[i].core==p.children[i].core.lock()&&
        actual_children[i].id==p.children[i].id&&actual_children[i].order==p.children[i].order,
        "Stack result current actual child order differs");
    const auto& b=*n.layout_baseline;
    require(b.base_.size_30_34==p.size&&b.base_.extent_20_24==p.extent&&b.base_.bounds_10_1c==p.bounds&&
        b.base_.offset_28_2c==p.offset&&std::array<float,2>{b.stack_->field_90,b.stack_->field_94}==p.anchor,
        "Stack result real canonical fields changed after returned View");
    for(const auto& row:p.children){const auto cc=row.core.lock();require(bool(cc),"Stack actual child Core retired");cc->layout_origin(expected_busy);
        const auto& cb=row.id?*node(cc->nodes,row.id).layout_baseline:*cc->context_layout_baseline;
        const auto actual=row.id?node(cc->nodes,row.id).layout_receiver_occurrence:cc->context_receiver_occurrence;
        require(actual&&actual==row.occurrence.lock()&&actual->constructor_serial==row.constructor_serial&&
            cb.parsed_source_serial_==row.parser_serial&&cb.base_.offset_28_2c==row.native_offset&&cb.base_.size_30_34==row.native_size,
            "Stack child current allocation/parser/native offset differs");
        if(row.id)require(cc==c&&node(cc->nodes,row.id).parent==p.id&&node(cc->nodes,row.id).attached_order==row.order,
            "Stack ordinary child actual parent/order changed");
        else require(c->context_layout_children&&cc->context_parent_core.lock()==c&&cc->context_parent_control_id==p.id&&
            std::any_of(c->context_layout_children->begin(),c->context_layout_children->end(),[&](const auto& x){return x.core.lock()==cc&&x.parent_control_id==p.id&&x.attached_order==row.order;}),
            "Stack attached Context child actual parent/order changed");
    }
}
