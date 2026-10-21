// Inside civ5::ui AFTER Caption6 Material preparation. This issuer marks only
// the genuine embedded Label540B54 receiver entry, after the executed font
// passes and physical spriteNULL branch. It does not return Common56 or mint
// a public Material, ordinary Button phase, manager prefix, Root or drain.
struct BnwWorldViewControlTree::NormalEmbeddedLabelCommon56Storage final {
    const std::shared_ptr<detail::BnwNormalLabelMaterialLoan::Impl> material;
    const detail::BnwNormalButtonCaptionCall caption;
    const std::shared_ptr<detail::NotificationLayoutEmbeddedLabelState> label;
    const detail::BnwNormalRootBatchHistory::Guard root;
    // Physical Label28/2C/30/34/44/40, not copied Button geometry or a DTO.
    const std::array<std::uint32_t,6> receiver_words;
    // Actual cached Button6DA5A0/5C5/5D2 result becomes this Label's incoming.
    const std::array<std::uint32_t,3> incoming_words;
    const std::array<std::uint32_t,2> root_arg4_arg5,numeric;
    NormalEmbeddedLabelCommon56Storage(
        std::shared_ptr<detail::BnwNormalLabelMaterialLoan::Impl> actual_material,
        detail::BnwNormalButtonCaptionCall actual_caption,
        std::shared_ptr<detail::NotificationLayoutEmbeddedLabelState> actual_label,
        detail::BnwNormalRootBatchHistory::Guard actual_root,
        std::array<std::uint32_t,6> actual_words,std::array<std::uint32_t,3> actual_incoming,
        std::array<std::uint32_t,2> actual_depth,std::array<std::uint32_t,2> actual_numeric)
        :material(std::move(actual_material)),caption(std::move(actual_caption)),label(std::move(actual_label)),
         root(std::move(actual_root)),receiver_words(actual_words),incoming_words(actual_incoming),
         root_arg4_arg5(actual_depth),numeric(actual_numeric){}
};

struct detail::BnwNormalEmbeddedLabelCommon56Call::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const BnwWorldViewControlTree::NormalEmbeddedLabelCommon56Storage> storage;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    explicit Impl(std::shared_ptr<const BnwWorldViewControlTree::NormalEmbeddedLabelCommon56Storage> actual)
        :storage(std::move(actual)){}
};

void BnwWorldViewControlTree::require_normal_embedded_label_common56_current_without_callbacks(
    const detail::BnwNormalEmbeddedLabelCommon56Call& offered){
    const auto actual=offered.impl_;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(actual&&actual->creator==std::this_thread::get_id()&&actual->sealed&&actual->storage&&
        same(actual->canonical_self.lock(),actual),
        "Embedded Label Common56 origin foreign/unsealed/noncanonical FIRST");
    const auto storage=actual->storage;const auto material=storage->material;
    require(material&&material->creator==actual->creator&&material->alive.load()&&material->caption&&
        !material->sealed&&!material->cpu_child_returned&&material->embedded_common56_control&&
        same(material->embedded_common56_control->lock(),actual),
        "Embedded Label Common56 is not SAME original pending Material entry");
    require_normal_caption_material_current_without_callbacks(*material);
    material->record_current();
    const auto caption=storage->caption.impl_;
    require(same(material->canonical_self.lock(),material)&&same(material->caption->impl_,caption)&&
        same(material->root.impl_,storage->root.impl_)&&same(caption->root.impl_,storage->root.impl_)&&
        same(caption->label,storage->label)&&caption->paint_arguments&&!caption->projecting&&
        !material->payload&&material->completed_selector&&*material->completed_selector==1&&
        material->scalars&&*material->scalars==material->completed_scalars_without_callbacks(),
        "Embedded Common56 origin lacks actual pass-loop RETURN/selector1/scalars/Label178");
    const auto fields=storage->label->impl_;
    using Fields=decltype(fields)::element_type;
    fields->first();
    require(std::get_if<Fields::Null>(&fields->fields[0x128/4])&&
        std::get_if<Fields::Null>(&fields->fields[0x12c/4]),
        "Embedded Common56 spriteNULL branch changed; selector2 body is required");
    constexpr std::array<std::size_t,6> offsets{0x28,0x2c,0x30,0x34,0x44,0x40};
    for(std::size_t i=0;i<offsets.size();++i){
        const auto word=std::get_if<std::uint32_t>(&fields->fields[offsets[i]/4]);
        require(word&&*word==storage->receiver_words[i],
            "Embedded Common56 actual Label physical operand changed");
    }
    require(caption->paint_arguments->result==storage->incoming_words&&
        caption->paint_arguments->numeric==storage->numeric&&
        material->batch&&material->batch->batch&&
        material->batch->batch->root_depth==storage->root_arg4_arg5,
        "Embedded Common56 incoming/root arguments no longer SAME earned actual caller");
    detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(storage->numeric);
    // Parent38/Realm3C, actual embedded Frame/Paint/DATA/Binding and Source are
    // checked by the genuine Caption/Material guards, without phase conversion.
    require_normal_button_caption_call_current_without_callbacks(storage->caption);
    storage->root.require_physical_current_without_callbacks();
    material->record_current();
    require_normal_caption_material_current_without_callbacks(*material);
    require(same(material->embedded_common56_control->lock(),actual),
        "Embedded Common56 original once-entry changed at PURE POST");
}

void BnwWorldViewControlTree::require_normal_embedded_label_common56_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalEmbeddedLabelCommon56Call& offered){
    const auto call=offered;
    const auto first=[&]{require_normal_embedded_label_common56_current_without_callbacks(call);};
    first();const auto material=call.impl_->storage->material;
    require(material->operation==&operation,"Embedded Common56 source is not original actual Material operation");
    material->full(operation);first();
    require_normal_button_caption_call_source_owned(operation,call.impl_->storage->caption);first();
    call.impl_->storage->root.require_source_owned(operation);first();
}

detail::BnwNormalEmbeddedLabelCommon56Call
BnwWorldViewControlTree::capture_normal_embedded_label_common56_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<detail::BnwNormalLabelMaterialLoan::Impl> supplied_material)const{
    using Call=detail::BnwNormalEmbeddedLabelCommon56Call;
    const auto material=std::move(supplied_material);
    const auto first=[&]{
        require(material&&material->creator==std::this_thread::get_id()&&material->alive.load()&&
            material->tree.get()==this&&material->operation==&operation&&material->caption&&
            !material->embedded_common56_control&&!material->sealed&&!material->cpu_child_returned,
            "Embedded Common56 issuer requires SAME original live Material, once before child return");
        require_normal_caption_material_current_without_callbacks(*material);
        material->record_current();
        require(!material->payload&&material->completed_selector&&*material->completed_selector==1&&
            material->scalars&&*material->scalars==material->completed_scalars_without_callbacks(),
            "Embedded Common56 receiver cannot be captured before real per-pass RETURN");
    };
    const auto full=[&]{first();material->full(operation);first();};
    first();full();first();
    const auto caption=*material->caption;const auto label=caption.impl_->label;const auto root=material->root;
    const auto fields=label->impl_;
    using Fields=decltype(fields)::element_type;
    const auto read_words=[&]{
        first();fields->first();
        require(std::get_if<Fields::Null>(&fields->fields[0x128/4])&&
            std::get_if<Fields::Null>(&fields->fields[0x12c/4]),
            "Embedded Label sprite selector2 body is mandatory before Common56 entry");
        constexpr std::array<std::size_t,6> offsets{0x28,0x2c,0x30,0x34,0x44,0x40};
        std::array<std::uint32_t,6> result;
        for(std::size_t i=0;i<offsets.size();++i){
            const auto word=std::get_if<std::uint32_t>(&fields->fields[offsets[i]/4]);
            require(word,"Embedded Common56 current physical Label operand missing");result[i]=*word;
        }
        first();return result;
    };
    const auto words=read_words();
    require(caption.impl_->paint_arguments&&!caption.impl_->projecting,
        "Embedded Common56 actual incoming Caption caller arguments unissued");
    const auto incoming=caption.impl_->paint_arguments->result;
    const auto numeric=caption.impl_->paint_arguments->numeric;
    const auto depth=material->batch->batch->root_depth;
    const auto pending=[&]{
        first();require(read_words()==words&&caption.impl_->paint_arguments&&
            caption.impl_->paint_arguments->result==incoming&&caption.impl_->paint_arguments->numeric==numeric&&
            material->batch->batch->root_depth==depth&&
            material->caption->impl_==caption.impl_&&material->root.impl_==root.impl_&&label==caption.impl_->label,
            "Embedded Common56 retained original argument/Label/Root changed across callback or allocator");
        detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(numeric);first();
    };
    pending();full();pending();
    auto storage=std::make_shared<const NormalEmbeddedLabelCommon56Storage>(
        material,caption,label,root,words,incoming,depth,numeric);
    pending();full();pending();
    auto actual=std::make_shared<Call::Impl>(storage);
    pending();full();pending();
    // No Source observation, allocation or callback between once-only stores.
    actual->canonical_self=actual;
    actual->sealed=true;
    material->embedded_common56_control=std::weak_ptr<const void>(actual);
    const Call::IssuerKey key;Call call(actual,key);
    require_normal_embedded_label_common56_current_without_callbacks(call);
    require_normal_embedded_label_common56_source_owned(operation,call);
    require_normal_embedded_label_common56_current_without_callbacks(call);
    return call;
}

detail::BnwNormalEmbeddedLabelCommon56Call::BnwNormalEmbeddedLabelCommon56Call(
    std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){
    BnwWorldViewControlTree::require_normal_embedded_label_common56_current_without_callbacks(*this);
}
void detail::BnwNormalEmbeddedLabelCommon56Call::require_current_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_embedded_label_common56_current_without_callbacks(*this);
}
void detail::BnwNormalEmbeddedLabelCommon56Call::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    BnwWorldViewControlTree::require_normal_embedded_label_common56_source_owned(operation,*this);
}
std::shared_ptr<detail::NotificationLayoutEmbeddedLabelState>
detail::BnwNormalEmbeddedLabelCommon56Call::original_label_without_callbacks()const{
    require_current_without_callbacks();const auto value=impl_->storage->label;require_current_without_callbacks();return value;
}
detail::BnwNormalRootBatchHistory::Guard
detail::BnwNormalEmbeddedLabelCommon56Call::captured_root_without_callbacks()const{
    require_current_without_callbacks();const auto value=impl_->storage->root;require_current_without_callbacks();return value;
}
std::array<std::uint32_t,4>
detail::BnwNormalEmbeddedLabelCommon56Call::actual_receiver_rect_words_without_callbacks()const{
    require_current_without_callbacks();const auto words=impl_->storage->receiver_words;
    const std::array<std::uint32_t,4> value{words[0],words[1],words[2],words[3]};
    require_current_without_callbacks();return value;
}
std::array<std::uint32_t,3>
detail::BnwNormalEmbeddedLabelCommon56Call::actual_incoming_caption_words_without_callbacks()const{
    require_current_without_callbacks();const auto value=impl_->storage->incoming_words;require_current_without_callbacks();return value;
}
std::array<std::uint32_t,2>
detail::BnwNormalEmbeddedLabelCommon56Call::actual_root_arg4_arg5_words_without_callbacks()const{
    require_current_without_callbacks();const auto value=impl_->storage->root_arg4_arg5;require_current_without_callbacks();return value;
}
