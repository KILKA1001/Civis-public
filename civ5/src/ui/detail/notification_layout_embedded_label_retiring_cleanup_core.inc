// Inside civ5::ui AFTER complete State/Realm/Child/Factory/child5E.
// Retiring access is separate from all ordinary parser/paint borrowers.
void detail::NotificationLayoutRuntimeDimensions::retire_label_parent_owned(
    const std::shared_ptr<NotificationLayoutButtonState>& parent,
    const std::shared_ptr<NotificationContextGuiPort>& gui)noexcept{
    if(!parent||parent->label_retirement_)return;
    const auto returned=parent->label_178_factory_return_;
    const auto child=parent->label_178_?parent->label_178_:parent->pending_label_178_;
    if(!child)return;
    const auto fields=child->impl_;
    // FIRST, also when last physical owner retires on a foreign thread. No
    // ordinary getter may succeed while destructor stores/callbacks execute.
    if(returned)returned->impl_->alive.store(false);
    fields->alive.store(false);fields->failed.store(true);
    if(fields->creator!=std::this_thread::get_id())return;
    using F=NotificationLayoutEmbeddedLabelState::Impl;
    using Disposition=NotificationLayoutButtonState::LabelRetirement;
    parent->label_retirement_=Disposition::quarantined;
    if(fields->retirement!=F::Retirement::none)return;
    fields->retirement=F::Retirement::quarantined;
    try{
        require(returned&&returned->impl_->creator==std::this_thread::get_id(),
            "Label cleanup has no genuine full constructor return");
        const auto stamp=returned->impl_;const auto origin=stamp->parent.lock();
        require(origin&&origin.get()==parent.get()&&!origin.owner_before(parent)&&
            !parent.owner_before(origin)&&stamp->child.get()==child.get()&&
            !stamp->child.owner_before(child)&&!child.owner_before(stamp->child)&&
            stamp->published_sequence&&stamp->module.get()==fields->module.get()&&
            !stamp->module.owner_before(fields->module)&&!fields->module.owner_before(stamp->module)&&
            stamp->source_serial==fields->source_serial&&fields->entered&&!fields->busy,
            "Label cleanup actual published allocation/constructor changed");
        const auto parser=stamp->parser;const auto core=parser->core.lock();
        require(core&&core->creator==std::this_thread::get_id()&&parser->creator==core->creator&&
            parser->id&&parser->id<=core->nodes.size(),
            "Label cleanup canonical parent allocation missing");
        const auto& node=core->nodes[parser->id-1];
        require(node.layout_baseline&&&*node.layout_baseline==parser->baseline&&
            node.layout_baseline->button_&&node.layout_baseline->button_.get()==parent.get()&&
            !node.layout_baseline->button_.owner_before(parent)&&!parent.owner_before(node.layout_baseline->button_)&&
            (!core->facade_alive.load()||core->failed.load()),
            "Label cleanup must belong to actual retiring/failed canonical parent");
        stamp->module->begin_label_cleanup_owned(stamp->allocation,child);
        fields->retirement=F::Retirement::entered;
        // A fixed bounded journal avoids allocations/reentry while real
        // destructor effects change the retired allocation. Unknown branches
        // stop at their reached boundary; no unknown value becomes NULL.
        const auto record=[&](std::uint32_t offset){
            require(fields->cleanup_count<fields->cleanup_offsets.size(),
                "Label cleanup bounded history exhausted");
            fields->cleanup_offsets[fields->cleanup_count++]=offset;
        };
        const auto word=[&](std::uint32_t offset,std::uint32_t value){
            record(offset);fields->fields[offset/4]=value;
        };
        const auto nul=[&](std::uint32_t offset){record(offset);fields->fields[offset/4]=F::Null{};};
        const auto is_null=[&](std::uint32_t offset){return std::holds_alternative<F::Null>(fields->fields[offset/4]);};
        const auto zero=[&](std::uint32_t offset){
            const auto* value=std::get_if<std::uint32_t>(&fields->fields[offset/4]);return value&&!*value;
        };
        // 754540(deleteflag0) -> 5126B0 -> 64C8C0. No allocator delete occurs
        // inside native slot0 when called by7A0630. OWN reclamation follows.
        record(0);fields->fields[0]=F::Vptr::label_a48dd0; //5126B3
        const auto* before=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(before,"Label64C8C0 flags are unissued");
        word(0x40,(*before&0xff7fffffu)|0x09000000u);
        record(0x168);fields->wrap_168=-1;
        record(0x16c);fields->truncate_16c=-1;
        record(0x16a);fields->leading_16a=0;
        // CC is borrowed registry Font, distinct from C4 text/string object.
        // Its real native cleanup is a scalar NULL store, not Font::DTOR.
        const auto* font=std::get_if<F::FontBorrow>(&fields->fields[0xcc/4]);
        require(is_null(0xcc)||(font&&font->loan),
            "Label64C8F2 borrowed Font slot is unknown");
        nul(0xcc);
        //64C8F8 is a scalar NULL store. The genuine private50E500 issuer
        //owns the whole Gui cache; a Label170 descriptor never owns/retires it.
        if(!is_null(0x170)){
            const auto* system=std::get_if<F::SystemFontBorrow>(&fields->fields[0x170/4]);
            require(system&&system->loan,"Label64C8F8 unknown/empty system-font borrower");
            //No live ordinary getter during retiring cleanup. Actual GUI may
            //already be retired. Dropping this typed weak borrower has no
            //Source/GL/cache/Font callback and earns no global Font destruction.
        }
        nul(0x170);
        for(std::uint32_t offset=0xd0;offset<=0x120;offset+=4){
            require(is_null(offset),"Label64C910/64C940 nonNULL graphics handle requires actual8E4D80 release");
            nul(offset);
        }
        // 64C960...64C9CB: the actual constructor has both begin/end NULL;
        // no generated glyph table exists, so the pointer-equality branch is
        // earned. NonNULL table cleanup must release each real entry first.
        require(is_null(0x128)&&is_null(0x12c),
            "Label64C960 generated glyph table needs real texture/image release");
        if(!is_null(0xc4)){
            const auto* text=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);
            require(text&&text->object,"Label64C9E2 unknown/empty actual text object");
            const auto& object=text->object;
            const auto allocation_child=object->allocation_child.lock();
            require(object->creator==std::this_thread::get_id()&&!object->retired.load()&&
                allocation_child&&allocation_child.get()==fields.get()&&
                !allocation_child.owner_before(fields)&&!fields.owner_before(allocation_child)&&
                object->source_serial==fields->source_serial&&object->source_serial&&
                !object->bytes.empty()&&object->bytes.size()<=0x4000&&object->bytes.back()=='\0'&&
                std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1,
                "Label64C9E2 requires this exact fresh OWN UTF8+NUL text allocation");
            require(fields->cleanup_count<fields->cleanup_offsets.size(),
                "Label64C9E2 cleanup history exhausted before text retirement");
            //All ten native pool vslot4 bodies append the SAME object to their
            //memory free-vector and decrement its active count. Fresh OWN x64
            //storage retires this exact allocation and drops its real vector;
            //native pool addresses/counters/reuse are not executed or claimed.
            //Retire FIRST so no physical surviving descriptor may read text.
            object->retired.store(true);
        }
        nul(0xc4); //64C9E4, strictly AFTER supported actual text retirement.
        // 64C8C0 returned. 5126BE owns the inline-vs-heap cleanup decision.
        const auto* inline_cell=std::get_if<F::SelfInline140>(&fields->fields[0x134/4]);
        require(inline_cell&&inline_cell->actual_inline_cell==&fields->fields[0x140/4],
            "Label5126CF noninline text storage needs real allocator discharge");
        require(is_null(0x128),"Label5126E2 glyph-array allocation is unissued");
        nul(0x128);nul(0x12c);nul(0x130);
        // Literal5BC660 Base teardown. This does not perform the enclosing
        // Button's embedded Image or Button Base destructors.
        record(0);fields->fields[0]=F::Vptr::base_a40cb8;
        const auto* base_flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(base_flags&& !(*base_flags&0x80000u),
            "Label5BC680 registered control requires actual6BF8A0 removal");
        require(is_null(0x54),"Label5BC691 attached reference needs actual vslot4 release");
        // The current canonical GUI is still strongly retained. Its constructor
        // fields5A0/5A4 are actual observed NULL and therefore cannot equal this
        // distinct actual child. This executes the supported676800 nonmatch
        // branch; a future nonNULL input writer is never silently skipped.
        require(gui&&gui->impl_->creator==std::this_thread::get_id(),
            "Label5BC6A0 canonical GUI retiring identity missing");
        const auto manager=gui->impl_;const auto tooltip=manager->tooltip_manager;
        const auto retained_gui_source=manager->source.lock();
        require(retained_gui_source&&retained_gui_source.get()==core->source.get()&&
            !retained_gui_source.owner_before(core->source)&&!core->source.owner_before(retained_gui_source)&&
            tooltip&&tooltip->life->creator==std::this_thread::get_id()&&
            tooltip->constructor_source_serial&&tooltip->history.size()>=9&&
            tooltip->fields_5a0_5bc[0]&&tooltip->fields_5a0_5bc[1]&&
            std::holds_alternative<NotificationContextGuiPort::Impl::TooltipKnownFields::Null>(*tooltip->fields_5a0_5bc[0])&&
            std::holds_alternative<NotificationContextGuiPort::Impl::TooltipKnownFields::Null>(*tooltip->fields_5a0_5bc[1]),
            "Label676800 actual GUI hover/focus nonNULL/unknown needs its retiring resolver");
        record(0x676800); //real supported branch entry/return, no equality stores
        require(!(*base_flags&0x300000u),
            "Label5BC6BF resize registration needs actual86A6E0 removal");
        word(0x40,0);
        require(is_null(0x78)&&is_null(0x7c)&&is_null(0x80),
            "Label5BC6D3 owned ordinary children/list cleanup requires actual destructors");
        require(zero(0x58),"Label5BC73D base58 reference requires its actual release");
        // Real empty flags-reference6029E0: vptr store precedes the two tests.
        record(0x5c);fields->fields[0x5c/4]=F::Vptr::reference_a67fa8;
        require(zero(0x60)&&zero(0x64),"Label6029F4 flags reference callback must execute");
        // Base4C empty Lua/String handle. Native8F8820 does not call the Lua
        // unref when the actual constructor pointer is zero; it still resets
        // both fields. No parser-created handle is converted into emptiness.
        require(zero(0x4c),"Label8F8838 base String handle needs real Lua unref");
        word(0x4c,0);word(0x50,0xfffffffeu);
        record(0);fields->fields[0]=F::Vptr::reference_a67fa8;
        require(zero(4)&&zero(8),"Label6029F4 root reference callback must execute");
        fields->retirement=F::Retirement::subtype_returned;
        stamp->module->finish_label_subtype_cleanup_owned(stamp->allocation,child);
        // Selected OWN x64 backend uses fresh objects and exact retired
        // allocation reclamation. No original x86 pool/address/global total
        // equality or pooled reuse is claimed. Active decrement is earned only
        // AFTER the whole supported subtype destructor returns.
        stamp->module->finish_label_reclamation_owned(stamp->allocation,child);
        fields->retirement=F::Retirement::reclaimed;
        parent->label_retirement_=Disposition::reclaimed;
        parent->label_178_factory_return_.reset();
        parent->label_178_.reset();parent->pending_label_178_.reset();
        // Keep native178 observation intact: reclaiming OWN references is not
        // a fabricated native store178=NULL or a reusable child constructor.
    }catch(...){
        fields->retirement=F::Retirement::quarantined;
        parent->label_retirement_=Disposition::quarantined;
        if(gui)gui->impl_->failed.store(true);
        if(returned)if(const auto core=returned->impl_->parser->core.lock())core->failed.store(true);
        // Keep actual earned cells and owning parent pointers. C++ final storage
        // deletion is not a subtype/pool release receipt. No counter rollback,
        // active decrement or reuse follows an unknown mandatory effect.
    }
}
void detail::NotificationLayoutRuntimeDimensions::retire_failed_label_owned(
    const std::shared_ptr<State>& saved)noexcept{
    if(!saved||saved->creator!=std::this_thread::get_id())return;
    const auto actual=saved->core.lock();
    if(!actual||actual->creator!=std::this_thread::get_id()||!actual->failed.load()||
       !saved->id||saved->id>actual->nodes.size())return;
    const auto& node=actual->nodes[saved->id-1];
    if(!node.layout_baseline||&*node.layout_baseline!=saved->baseline)return;
    retire_label_parent_owned(node.layout_baseline->button_,actual->context_gui_port);
}
void detail::NotificationLayoutRuntimeDimensions::retire_tree_labels_owned(
    BnwWorldViewControlTree& tree)noexcept{
    const auto actual=tree.core_;
    if(actual->creator!=std::this_thread::get_id()){actual->failed.store(true);return;}
    if(actual->facade_alive.load())return; //only real destructor-FIRST callsite
    for(const auto& node:actual->nodes)if(node.layout_baseline)
        retire_label_parent_owned(node.layout_baseline->button_,actual->context_gui_port);
}
