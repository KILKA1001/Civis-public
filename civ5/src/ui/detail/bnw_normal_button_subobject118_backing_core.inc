#include "bnw_normal_button118_geometry_x87_core.inc"
#include "bnw_normal_button_state104_x87_sse_core.inc"
// Inside civ5::ui after complete RuntimeDimensions::State and storage guards.
// Own physical backing only: neither NativeRegion nor CURRENT is issued here.
struct detail::BnwNormalButtonSubobject118Backing::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<NotificationLayoutButtonState> button;
    const std::weak_ptr<const NotificationLayoutOrdinaryReceiver> ordinary_receiver;
    const std::uint64_t id,constructor_serial;
    // Default initialization deliberately executes no byte initializer. All
    // reads below test physical write coverage before accessing these bytes.
    std::array<std::byte,0x60> words;
    std::uint32_t written=0;
    std::uint64_t store_count=0;
    bool failed=false,constructor_returned=false;
    std::weak_ptr<const BnwNormalButtonSubobject118Backing> canonical;
    enum class Source44State {unknown,constructed_null,selected_keeper};
    Source44State source44_state=Source44State::unknown;
    std::weak_ptr<const NotificationLayoutTextureKeeper> selected_source44;
    std::uint64_t outer_store_count=0;
    std::optional<std::uint64_t> null_mask_store_sequence;
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
         std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
         std::shared_ptr<NotificationLayoutButtonState> b,
         std::shared_ptr<const NotificationLayoutOrdinaryReceiver> original_receiver,
         std::uint64_t actual_id,std::uint64_t serial)
        :core(std::move(c)),occurrence(std::move(o)),button(std::move(b)),
         ordinary_receiver(std::move(original_receiver)),id(actual_id),constructor_serial(serial){}
    void first_value_without_callbacks(const NotificationLayoutButtonState& actual)const{
        require(creator==std::this_thread::get_id()&&!failed&&constructor_returned,
            "Button118 physical actor foreign/failed/constructor not returned");
        const auto owner=core.lock();const auto place=occurrence.lock();
        require(owner&&place&&owner->creator==creator&&!owner->failed.load(),
            "Button118 physical actor original Core/occurrence retired");
        // Fresh Source FIRST precedes every borrow of canonical Button/Base.
        const auto& operation=owner->source->current_operation_loan();operation.require_origin_without_callbacks();
        const auto original=button.lock();const auto self=canonical.lock();
        require(original&&original.get()==&actual&&self&&self->impl_.get()==this&&actual.backing_118_&&
            self.get()==actual.backing_118_.get()&&!self.owner_before(actual.backing_118_)&&
            !actual.backing_118_.owner_before(self),
            "Button118 physical actor is not SAME actual Button-owned backing");
        const auto& node=BnwWorldViewControlTree::Core::node(owner->nodes,id);
        require(node.alive&&node.layout_baseline&&node.layout_receiver_occurrence==place&&
            !node.layout_receiver_occurrence.owner_before(place)&&
            !place.owner_before(node.layout_receiver_occurrence)&&
            place->id==id&&place->constructor_serial==constructor_serial&&
            node.layout_baseline->button_.get()==original.get()&&
            !node.layout_baseline->button_.owner_before(original)&&
            !original.owner_before(node.layout_baseline->button_),
            "Button118 physical actor canonical receiver allocation changed");
        const auto receiver=ordinary_receiver.lock();
        require(receiver&&receiver->impl_&&node.layout_baseline->base_.receiver_owned.get()==receiver.get()&&
            !node.layout_baseline->base_.receiver_owned.owner_before(receiver)&&
            !receiver.owner_before(node.layout_baseline->base_.receiver_owned),
            "Button118 SAME original ordinary receiver allocation changed");
        const auto native=receiver->impl_;const auto native_canonical=native->canonical.lock();
        const auto native_button=native->button.lock();
        const auto native_core=native->core.lock();const auto native_place=native->occurrence.lock();
        require(native->creator==creator&&native->id==id&&native->serial==constructor_serial&&
            native_canonical.get()==receiver.get()&&!native_canonical.owner_before(receiver)&&
            !receiver.owner_before(native_canonical)&&native_button.get()==original.get()&&
            !native_button.owner_before(original)&&!original.owner_before(native_button)&&
            native_core.get()==owner.get()&&!native_core.owner_before(owner)&&!owner.owner_before(native_core)&&
            native_place.get()==place.get()&&!native_place.owner_before(place)&&!place.owner_before(native_place)&&
            native->button_state104&&native->button_state104_writer&&native->button_state104_store_count&&
            *native->button_state104==actual.state_104&&
            ((native->button_state104_store_count==1&&native->button_state104_writer==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::literal_ctor548513)||
             (native->button_state104_store_count>1&&native->button_state104_writer==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::state_setter6a9610)),
            "Button118 SAME original physical104 receiver/store coverage changed");
        operation.require_origin_without_callbacks();
    }
    void put(std::uint32_t offset,std::uint32_t word){
        require((offset&3u)==0&&offset<=0x5cu&&store_count<UINT64_MAX,
            "Button118 actual store range/journal exhausted");
        // Physical little-endian destination stores, not a DTO serialization.
        for(std::uint32_t lane=0;lane!=4;++lane)words[offset+lane]=std::byte((word>>(8u*lane))&0xffu);
        written|=std::uint32_t{1}<<(offset/4u);++store_count;
    }
    std::uint32_t read(std::uint32_t offset)const{
        require((offset&3u)==0&&offset<=0x5cu,"Button118 physical word offset invalid");
        if((written&(std::uint32_t{1}<<(offset/4u)))==0){
            switch(offset){
            case 0x14:require(false,"Button118+14 current color580780/replay setter store unproduced");break;
            case 0x18:require(false,"Button118+18 current color580780/replay setter store unproduced");break;
            case 0x1c:require(false,"Button118+1C current color580780/replay setter store unproduced");break;
            case 0x20:require(false,"Button118+20 current color580780/replay setter store unproduced");break;
            case 0x24:require(false,"Button118+24 current6F6C10 geometry physical store unproduced");break;
            case 0x28:require(false,"Button118+28 current6F6C10 geometry physical store unproduced");break;
            case 0x2c:require(false,"Button118+2C current6F6C10 geometry physical store unproduced");break;
            case 0x30:require(false,"Button118+30 current6F6C10 geometry physical store unproduced");break;
            case 0x34:require(false,"Button118+34 current geometry/state physical store unproduced");break;
            case 0x38:require(false,"Button118+38 current geometry/state physical store unproduced");break;
            case 0x44:require(false,"Button118+44 current native32 resource source pointer unproduced");break;
            case 0x50:require(false,"Button118+50 actual5C7B50 UV store unproduced");break;
            case 0x54:require(false,"Button118+54 actual5C7B50 UV store unproduced");break;
            case 0x58:require(false,"Button118+58 actual5C7B50 UV store unproduced");break;
            case 0x5c:require(false,"Button118+5C actual5C7B50 UV store unproduced");break;
            default:require(false,"Button118 constructor word write coverage unexpectedly absent");
            }
        }
        std::uint32_t result=0;
        for(std::uint32_t lane=0;lane!=4;++lane)
            result|=std::uint32_t(std::to_integer<unsigned char>(words[offset+lane]))<<(8u*lane);
        return result;
    }

    void require_same_baseline_without_callbacks(const NotificationLayoutButtonState& actual,
        const NotificationLayoutBaseline& baseline)const{
        first_value_without_callbacks(actual);const auto owner=core.lock();
        const auto& node=BnwWorldViewControlTree::Core::node(owner->nodes,id);
        require(&*node.layout_baseline==&baseline&&baseline.button_.get()==&actual,
            "Button118 actual baseline/receiver allocation changed");
        first_value_without_callbacks(actual);
    }
    void full_owned(const NotificationLayoutButtonState& actual)const{
        first_value_without_callbacks(actual);const auto owner=core.lock();
        const auto& operation=owner->source->current_operation_loan();
        operation.require_origin_without_callbacks();operation.require_current();
        first_value_without_callbacks(actual);
    }
    bool read_genuine_source44_null_without_callbacks(const NotificationLayoutButtonState& actual)const{
        first_value_without_callbacks(actual);
        require(source44_state!=Source44State::selected_keeper,
            "Button source44 selected object native32 binder/address store unproduced");
        require(source44_state==Source44State::constructed_null,
            "Button source44 original physical binding unknown");
        const auto word=read(0x44);
        require(word==0&&selected_source44.expired()&&actual.texture_binding_earned&&
            !actual.texture_keeper&&!actual.texture_extent_15c&&actual.texture_key.empty(),
            "Button source44 constructed DWORD0 contradicts actual retained binding");
        first_value_without_callbacks(actual);return true;
    }
    void require_same_selected_keeper_without_callbacks(const NotificationLayoutButtonState& actual,
        const std::shared_ptr<const NotificationLayoutTextureKeeper>& source)const{
        first_value_without_callbacks(actual);NotificationLayoutTextureKeeper::require_canonical(source);
        const auto bound=selected_source44.lock();
        require(source44_state==Source44State::selected_keeper&&bound&&bound.get()==source.get()&&
            !bound.owner_before(source)&&!source.owner_before(bound)&&actual.texture_keeper.get()==source.get()&&
            !actual.texture_keeper.owner_before(source)&&!source.owner_before(actual.texture_keeper)&&
            actual.texture_binding_earned&&actual.texture_extent_15c&&
            *actual.texture_extent_15c==source->native_114_118_&&actual.texture_key==source->key_,
            "Button source44 object is not SAME actual selected keeper/control/114118");
        first_value_without_callbacks(actual);
    }

    void invalidate(std::uint32_t lanes){
        require((lanes&~0x00ffffffu)==0,"Button118 unavailable lane mask exceeds physical object");
        written&=~lanes;
    }
};

detail::BnwNormalButtonSubobject118Backing::BnwNormalButtonSubobject118Backing(std::shared_ptr<Impl> actual)
    :impl_(std::move(actual)){}

void detail::BnwNormalButtonSubobject118Backing::execute_constructor548513_state_owned(
    const NotificationLayoutRuntimeDimensions& runtime,
    const std::shared_ptr<NotificationLayoutButtonState>& button,
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>& receiver){
    using Runtime=NotificationLayoutRuntimeDimensions;
    const auto saved=runtime.state_;Runtime::require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&button&&receiver&&receiver->impl_,
        "Button548513 actual pending constructor controls absent");
    const auto fields=receiver->impl_;
    auto& baseline=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    std::optional<std::uint32_t> expected_word;
    std::uint64_t expected_count=0;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto pure=[&]{Runtime::require_storage_without_callbacks(saved);
        require(saved->id&&baseline.actual_type_=="Button"&&!baseline.button_&&
            !baseline.base_.receiver_owned&&!baseline.parsed_source_serial_&&
            !(baseline.base_.flags_40&0x8000u)&&!button->backing_118_&&
            saved->serial==occurrence->constructor_serial&&saved->id==occurrence->id&&
            same(saved->core.lock(),core)&&same(saved->occurrence.lock(),occurrence)&&
            receiver->impl_.get()==fields.get()&&same(fields->core.lock(),core)&&
            same(fields->occurrence.lock(),occurrence)&&same(fields->button.lock(),button)&&
            fields->creator==std::this_thread::get_id()&&fields->id==saved->id&&fields->serial==saved->serial&&
            fields->canonical.expired()&&fields->button_parser_progress==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::constructor_returned&&
            fields->button_void_f4_f8&&(*fields->button_void_f4_f8)[0]==0u&&
            (*fields->button_void_f4_f8)[1]==0u&&fields->button_void_missing_read_mask==0u&&
            fields->button_state104==expected_word&&fields->button_state104_store_count==expected_count,
            "Button548513 needs SAME genuine pending548490 F4/F8 receiver before54851E");
        if(expected_word)require(fields->button_state104_writer==
            NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::literal_ctor548513&&
            button->state_104==*expected_word,
            "Button548513 actual physical DWORD0/typed view changed across FULL POST");
        else require(!fields->button_state104_writer,
            "Button548513 initial word/writer was already issued");
    };
    const auto full=[&]{pure();saved->operation->require_current();pure();};
    try{
        full();
        //Actual5484C2 XOR ECX,ECX feeds548513 DWORD store. The optional
        //word was UNKNOWN; it is engaged by THIS executed real store only.
        fields->button_state104.emplace(0u);
        button->state_104=0u;
        fields->button_state104_writer=NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::literal_ctor548513;
        ++fields->button_state104_store_count;expected_word.emplace(0u);expected_count=1;
        full();
    }catch(...){Runtime::fail_storage_without_callbacks(saved);throw;}
}

void detail::BnwNormalButtonSubobject118Backing::execute_constructor40c8f0_owned(
    const NotificationLayoutRuntimeDimensions& runtime,const std::shared_ptr<NotificationLayoutButtonState>& button,
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>& receiver){
    using Runtime=NotificationLayoutRuntimeDimensions;
    const auto saved=runtime.state_;Runtime::require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    auto& baseline=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    const auto pure=[&]{Runtime::require_storage_without_callbacks(saved);
        require(core&&occurrence&&saved->id&&button&&!button->backing_118_&&
            baseline.actual_type_=="Button"&&!baseline.button_&&!baseline.base_.receiver_owned&&
            !baseline.parsed_source_serial_&&(baseline.base_.flags_40&0x8000u)&&
            occurrence->id==saved->id&&occurrence->constructor_serial==saved->serial&&
            same(saved->core.lock(),core)&&same(saved->occurrence.lock(),occurrence)&&
            receiver&&receiver->impl_&&receiver->impl_->canonical.expired()&&
            same(receiver->impl_->core.lock(),core)&&same(receiver->impl_->occurrence.lock(),occurrence)&&
            same(receiver->impl_->button.lock(),button)&&receiver->impl_->creator==std::this_thread::get_id()&&
            receiver->impl_->id==saved->id&&receiver->impl_->serial==saved->serial&&
            receiver->impl_->button_state104&&*receiver->impl_->button_state104==0u&&button->state_104==0u&&
            receiver->impl_->button_state104_store_count==1&&receiver->impl_->button_state104_writer==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::literal_ctor548513,
            "Button40C8F0 needs SAME actual pending5AAA20 allocation after genuine548513 and54851E");};
    const auto full=[&]{pure();saved->operation->require_current();pure();};
    std::shared_ptr<Impl> fields;
    try{
        full();
        fields=std::make_shared<Impl>(core,occurrence,button,receiver,saved->id,saved->serial);full();
        auto actual=std::shared_ptr<BnwNormalButtonSubobject118Backing>(new BnwNormalButtonSubobject118Backing(fields));full();
        // Complete111-byte40C8F0: original XORPS and qualified9D144C operand.
        // The concrete raw source is pinned in ctor-scalar-bounded-byte-qualification.json.
        constexpr std::uint32_t actual9d144c=0x3f800000u;
        fields->put(0x00,0x009f3570u); //40C8FD literal vptr, not a CURRENT vcall grant.
        fields->put(0x04,actual9d144c);
        fields->put(0x08,0u);fields->put(0x0c,0u);fields->put(0x10,actual9d144c);
        button->color_12c_138[0]=std::bit_cast<float>(actual9d144c);fields->put(0x14,actual9d144c);
        button->color_12c_138[1]=std::bit_cast<float>(actual9d144c);fields->put(0x18,actual9d144c);
        button->color_12c_138[2]=std::bit_cast<float>(actual9d144c);fields->put(0x1c,actual9d144c);
        button->color_12c_138[3]=std::bit_cast<float>(actual9d144c);fields->put(0x20,actual9d144c);
        button->image_13c_140[0]=std::bit_cast<float>(0u);fields->put(0x24,0u);
        button->image_13c_140[1]=std::bit_cast<float>(0u);fields->put(0x28,0u);
        button->image_144_148[0]=std::bit_cast<float>(0u);fields->put(0x2c,0u);
        button->image_144_148[1]=std::bit_cast<float>(0u);fields->put(0x30,0u);
        button->image_14c_150[0]=std::bit_cast<float>(0u);fields->put(0x34,0u);
        button->image_14c_150[1]=std::bit_cast<float>(0u);fields->put(0x38,0u);
        fields->put(0x3c,10u);fields->put(0x40,0u);
        fields->put(0x44,0u);fields->source44_state=Impl::Source44State::constructed_null;
        fields->put(0x48,0u);fields->put(0x4c,0u);
        require(fields->written==0x000fffffu&&fields->store_count==20,
            "Button40C8F0 complete original store sequence did not execute");
        full();fields->constructor_returned=true;fields->canonical=actual;
        button->backing_118_=std::move(actual);
        // Caller performs its existing FULL POST after this physical store.
        Runtime::require_storage_without_callbacks(saved);
    }catch(...){if(fields)fields->failed=true;Runtime::fail_storage_without_callbacks(saved);throw;}
}

void detail::BnwNormalButtonSubobject118Backing::execute_visual_state6b3610_owned(
    NotificationLayoutButtonState& button,NotificationLayoutBaseline& baseline,std::uint32_t actual_state){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,
        "Button6B3610 SAME original118 actor absent");
    const auto fields=actual->impl_;fields->require_same_baseline_without_callbacks(button,baseline);
    //Actual enclosing Slot10Frame already holds this canonical baseline. The
    //local strong controls additionally retain its SAME receiver across FULL
    //Source callbacks; all persistent backlinks remain weak.
    const auto core=fields->core.lock();const auto place=fields->occurrence.lock();
    const auto original_button=fields->button.lock();const auto receiver=fields->ordinary_receiver.lock();
    require(core&&place&&original_button&&receiver&&receiver->impl_,
        "Button6B3610 original Core/occurrence/receiver retired");
    const auto native=receiver->impl_;
    require(native->button_state104&&native->button_state104_writer&&
        native->button_state104_store_count,
        "Button6B3610 physical104 must have a real returned constructor/writer");
    const auto flags=baseline.base_.flags_40;
    auto expected_word=*native->button_state104;
    auto expected_count=native->button_state104_store_count;
    auto expected_writer=*native->button_state104_writer;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto pure=[&]{fields->require_same_baseline_without_callbacks(button,baseline);
        require(same(fields->core.lock(),core)&&same(fields->occurrence.lock(),place)&&
            same(fields->button.lock(),original_button)&&original_button.get()==&button&&
            same(fields->ordinary_receiver.lock(),receiver)&&receiver->impl_.get()==native.get()&&
            native->button_state104&&*native->button_state104==expected_word&&
            native->button_state104_store_count==expected_count&&native->button_state104_writer==expected_writer&&
            button.state_104==expected_word&&baseline.base_.flags_40==flags,
            "Button6B3610 SAME physical104/Control/flag tuple changed across Source callbacks");};
    const auto full=[&]{pure();fields->full_owned(button);pure();};
    try{
        full();require(expected_count<UINT64_MAX,"Button6A9614 actual104 store journal exhausted");
        //Fresh qualified complete6A9610: MOV actual stack argument -> EAX,
        //MOV [SAME receiver+104],EAX, RET4. The physical store executes
        //unconditionally FIRST, including80000000 and an unchanged value.
        *native->button_state104=actual_state;
        button.state_104=actual_state;
        ++native->button_state104_store_count;
        native->button_state104_writer=NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::state_setter6a9610;
        expected_word=actual_state;++expected_count;
        expected_writer=NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::state_setter6a9610;
        full(); //Actual6A9610 has returned before the caller's6B3621 TEST.
        if(flags&0x80000000u)return; //Real6B3628 branch has no geometry/UV stores.
        store_state_geometry6b368a_after_6a9610_owned(button,baseline,actual_state);
        full();
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::store_state_geometry6b368a_after_6a9610_owned(
    NotificationLayoutButtonState& button,NotificationLayoutBaseline& baseline,std::uint32_t actual_state){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,
        "Button6B368A actual118 backing absent");
    const auto fields=actual->impl_;fields->require_same_baseline_without_callbacks(button,baseline);
    const auto receiver=fields->ordinary_receiver.lock();require(receiver&&receiver->impl_,
        "Button6B368A original ordinary receiver retired");
    const auto native=receiver->impl_;
    //Only the closed complete6B3610 method above supplies the returned6A
    //state. Hold the SAME canonical owner chain throughout both FULL cuts.
    const auto core=fields->core.lock();const auto original_button=fields->button.lock();
    require(core&&original_button&&original_button.get()==&button,
        "Button6B368A original canonical owner retired");
    const auto same_inputs=[&](const std::array<std::uint32_t,4>& input,std::uint64_t state_count){
        fields->require_same_baseline_without_callbacks(button,baseline);
        require(native->button_state104&&*native->button_state104==actual_state&&
            native->button_state104_store_count==state_count&&native->button_state104_writer==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::state_setter6a9610&&
            button.state_104==actual_state&&!(baseline.base_.flags_40&0x80000000u)&&
            std::bit_cast<std::uint32_t>(button.authored_108_10c[0])==input[0]&&
            std::bit_cast<std::uint32_t>(button.authored_108_10c[1])==input[1]&&
            std::bit_cast<std::uint32_t>(button.increment_110_114[0])==input[2]&&
            std::bit_cast<std::uint32_t>(button.increment_110_114[1])==input[3],
            "Button6B368A SAME returned6A9610/outer input/flag tuple changed");
    };
    try{
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        require(native->button_state104_store_count>1&&native->button_state104&&
            *native->button_state104==actual_state&&native->button_state104_writer==
                NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::state_setter6a9610&&
            !(baseline.base_.flags_40&0x80000000u),
            "Button6B368A requires genuine6A9610 return before the admitted non80000000 branch");
        const std::array<std::uint32_t,4> input={
            std::bit_cast<std::uint32_t>(button.authored_108_10c[0]),
            std::bit_cast<std::uint32_t>(button.authored_108_10c[1]),
            std::bit_cast<std::uint32_t>(button.increment_110_114[0]),
            std::bit_cast<std::uint32_t>(button.increment_110_114[1])};
        const auto state_count=native->button_state104_store_count;
        same_inputs(input,state_count);fields->full_owned(button);same_inputs(input,state_count);
        require(fields->store_count<=UINT64_MAX-2,"Button6B368A physical store journal exhausted");
        const auto before=fields->store_count;
        //A constructor word0, copied state or caller bool cannot substitute
        //the physically returned6A9610 store required by SAME checks above.
        bnw_button118_execute_actual_6b363a_state_geometry_owned(fields->words,actual_state,input);
        fields->written|=(std::uint32_t{1}<<(0x38u/4u))|(std::uint32_t{1}<<(0x34u/4u));
        fields->store_count+=2;
        //Typed views follow earned physical stores in original150 then14C order.
        button.image_14c_150[1]=std::bit_cast<float>(fields->read(0x38));
        button.image_14c_150[0]=std::bit_cast<float>(fields->read(0x34));
        fields->full_owned(button);same_inputs(input,state_count);
        require(fields->store_count==before+2&&
            std::bit_cast<std::uint32_t>(button.image_14c_150[1])==fields->read(0x38)&&
            std::bit_cast<std::uint32_t>(button.image_14c_150[0])==fields->read(0x34),
            "Button6B3696 actual ordered stores changed across FULL POST");
        execute_coordinates5c7b50_owned(button);fields->full_owned(button);same_inputs(input,state_count);
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::store_color5cabc0_owned(
    NotificationLayoutButtonState& button,const std::array<float,4>& captured){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button5CABC0 actual118 backing producer missing");
    const auto fields=actual->impl_;fields->first_value_without_callbacks(button);
    const auto values=captured; //Owned real caller operands, not a parsed XML issuer.
    try{
        require(fields->store_count<=UINT64_MAX-4,"Button5CABC0 physical store journal exhausted");
        //Actual5CABC8/5CABD2/5CABDC/5CABE2, preserving Original store order.
        button.color_12c_138[0]=values[0];fields->put(0x14,std::bit_cast<std::uint32_t>(values[0]));
        button.color_12c_138[1]=values[1];fields->put(0x18,std::bit_cast<std::uint32_t>(values[1]));
        button.color_12c_138[2]=values[2];fields->put(0x1c,std::bit_cast<std::uint32_t>(values[2]));
        button.color_12c_138[3]=values[3];fields->put(0x20,std::bit_cast<std::uint32_t>(values[3]));
        fields->first_value_without_callbacks(button);
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::invalidate_direct580780_color_before_store_owned(
    NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button580780 actual118 backing absent");
    actual->impl_->first_value_without_callbacks(button);actual->impl_->invalidate(0x000001e0u);
}
void detail::BnwNormalButtonSubobject118Backing::invalidate_geometry_before_store_owned(NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button6F6C10 actual118 backing absent");
    actual->impl_->first_value_without_callbacks(button);actual->impl_->invalidate(0x00f07e00u);
}
void detail::BnwNormalButtonSubobject118Backing::invalidate_coordinates_before_store_owned(NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button5C7B50 actual118 backing absent");
    actual->impl_->first_value_without_callbacks(button);actual->impl_->invalidate(0x00f00000u);
}
void detail::BnwNormalButtonSubobject118Backing::invalidate_source44_before_store_owned(NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button543A37 actual118 backing absent");
    actual->impl_->first_value_without_callbacks(button);actual->impl_->invalidate(0x00020000u);
    actual->impl_->source44_state=Impl::Source44State::unknown;actual->impl_->selected_source44.reset();
    actual->impl_->null_mask_store_sequence.reset();
}
void detail::BnwNormalButtonSubobject118Backing::require_same_button_storage_without_callbacks(
    const std::shared_ptr<NotificationLayoutButtonState>& button)const{
    require(button&&impl_,"Button118 actual physical storage absent");impl_->first_value_without_callbacks(*button);
}
std::uint32_t detail::BnwNormalButtonSubobject118Backing::require_written_word_without_callbacks(std::uint32_t offset)const{
    require(bool(impl_),"Button118 actual physical storage absent");const auto button=impl_->button.lock();
    require(bool(button),"Button118 actual Button retired");impl_->first_value_without_callbacks(*button);
    const auto value=impl_->read(offset);impl_->first_value_without_callbacks(*button);return value;
}

bool detail::BnwNormalButtonSubobject118Backing::require_current_source44_null_branch_without_callbacks()const{
    require(bool(impl_),"Button118 source44 classifier physical actor absent");
    const auto button=impl_->button.lock();require(bool(button),"Button118 source44 classifier actual Button retired");
    impl_->first_value_without_callbacks(*button);
    const auto owner=impl_->core.lock();const auto& baseline=*BnwWorldViewControlTree::Core::node(owner->nodes,impl_->id).layout_baseline;
    const auto is_null=impl_->read_genuine_source44_null_without_callbacks(*button);
    if(is_null)require(impl_->null_mask_store_sequence&&*impl_->null_mask_store_sequence&&
        (baseline.base_.flags_40&0x10000000u),
        "Button source44 NULL lacks actual returned6F6C1D ORouter40,10000000 effect");
    impl_->first_value_without_callbacks(*button);return is_null;
}

void detail::BnwNormalButtonSubobject118Backing::bind_source44_selected_keeper_owned(
    NotificationLayoutButtonState& button,const std::shared_ptr<const NotificationLayoutTextureKeeper>& keeper){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button selected source44 actual118 actor absent");
    const auto fields=actual->impl_;fields->first_value_without_callbacks(button);
    try{
        NotificationLayoutTextureKeeper::require_canonical(keeper);fields->first_value_without_callbacks(button);
        require(!button.texture_key.empty()&&keeper->key_==button.texture_key&&
            keeper->native_114_118_[0]&&keeper->native_114_118_[1],
            "Button selected source44 needs actual admitted keeper/key114/118 from owning binder");
        //The actual binder has loaded this selected object under its real
        //Source/raw-resource FULL guards. It now publishes ONE SAME pointer
        //control to the typed receiver. Its native32 numeric address remains
        //UNKNOWN, so the old literal NULL word cannot be consumed afterward.
        fields->invalidate(0x00020000u);fields->source44_state=Impl::Source44State::unknown;
        fields->selected_source44.reset();fields->null_mask_store_sequence.reset();
        button.texture_keeper=keeper;button.texture_extent_15c=keeper->native_114_118_;
        button.texture_binding_earned=true;
        fields->selected_source44=keeper;fields->source44_state=Impl::Source44State::selected_keeper;
        fields->first_value_without_callbacks(button);fields->require_same_selected_keeper_without_callbacks(button,keeper);
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::initialize_6f6c13_prefix_owned(
    NotificationLayoutButtonState& button,NotificationLayoutBaseline& baseline){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button6F6C13 actual118 actor absent");
    const auto fields=actual->impl_;fields->require_same_baseline_without_callbacks(button,baseline);
    try{
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        const auto is_null=fields->read_genuine_source44_null_without_callbacks(button);
        require(fields->outer_store_count<UINT64_MAX,"Button6F6C13 outer store journal exhausted");
        //Actual6F6C13 CMP[Button15C],0 then6F6C1D ORouter40,10000000.
        //Unknown nonNULL native32 bindings fail BEFORE this original branch.
        if(is_null){baseline.base_.flags_40|=0x10000000u;
            fields->null_mask_store_sequence=++fields->outer_store_count;}
        if(!(baseline.base_.flags_40&0x20000000u))
            bnw_button118_execute_actual_6f6c2d_increment_owned(baseline.base_.size_30_34[1],button.increment_110_114[1]);
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        if(is_null)require(fields->null_mask_store_sequence&&(baseline.base_.flags_40&0x10000000u),
            "Button6F6C1D actual NULL mask replaced across FULL POST");
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::store_geometry6f6c42_owned(
    NotificationLayoutButtonState& button,NotificationLayoutBaseline& baseline){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button6F6C42 actual118 actor absent");
    const auto fields=actual->impl_;fields->require_same_baseline_without_callbacks(button,baseline);
    try{
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        const auto size=baseline.base_.size_30_34;const auto origin=button.authored_108_10c;
        require(fields->store_count<=UINT64_MAX-6,"Button6F6C42 physical store journal exhausted");
        const auto before=fields->store_count;
        //Native MOV order13C,140,14C,150,144,148: destinations are the
        //same live typed Button and its sole physical subobject118 owner.
        button.image_13c_140[0]=size[0];fields->put(0x24,std::bit_cast<std::uint32_t>(size[0]));
        button.image_13c_140[1]=size[1];fields->put(0x28,std::bit_cast<std::uint32_t>(size[1]));
        button.image_14c_150[0]=origin[0];fields->put(0x34,std::bit_cast<std::uint32_t>(origin[0]));
        button.image_14c_150[1]=origin[1];fields->put(0x38,std::bit_cast<std::uint32_t>(origin[1]));
        button.image_144_148[0]=size[0];fields->put(0x2c,std::bit_cast<std::uint32_t>(size[0]));
        button.image_144_148[1]=size[1];fields->put(0x30,std::bit_cast<std::uint32_t>(size[1]));
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        require(fields->store_count==before+6,"Button6F6C42 actual stores changed during FULL POST");
        for(std::size_t lane=0;lane!=2;++lane)require(
            std::bit_cast<std::uint32_t>(baseline.base_.size_30_34[lane])==std::bit_cast<std::uint32_t>(size[lane])&&
            std::bit_cast<std::uint32_t>(button.authored_108_10c[lane])==std::bit_cast<std::uint32_t>(origin[lane])&&
            fields->read(0x24u+4u*lane)==std::bit_cast<std::uint32_t>(button.image_13c_140[lane])&&
            fields->read(0x34u+4u*lane)==std::bit_cast<std::uint32_t>(button.image_14c_150[lane])&&
            fields->read(0x2cu+4u*lane)==std::bit_cast<std::uint32_t>(button.image_144_148[lane]),
            "Button6F6C42 SAME sources/paired actual destinations changed across FULL POST");
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::restore_origin6f6cd1_owned(
    NotificationLayoutButtonState& button,NotificationLayoutBaseline& baseline){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button6F6CD1 actual118 actor absent");
    const auto fields=actual->impl_;fields->require_same_baseline_without_callbacks(button,baseline);
    try{
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        require(baseline.base_.flags_40&0x80000000u,"Button6F6CD1 conditional native80000000 branch not reached");
        const auto origin=button.authored_108_10c;require(fields->store_count<=UINT64_MAX-2,"Button6F6CD1 store journal exhausted");
        const auto before=fields->store_count;
        button.image_14c_150[0]=origin[0];fields->put(0x34,std::bit_cast<std::uint32_t>(origin[0]));
        button.image_14c_150[1]=origin[1];fields->put(0x38,std::bit_cast<std::uint32_t>(origin[1]));
        fields->full_owned(button);fields->require_same_baseline_without_callbacks(button,baseline);
        require(fields->store_count==before+2&&(baseline.base_.flags_40&0x80000000u),
            "Button6F6CD1 conditional actual stores changed across FULL POST");
        for(std::size_t lane=0;lane!=2;++lane)require(
            std::bit_cast<std::uint32_t>(button.authored_108_10c[lane])==std::bit_cast<std::uint32_t>(origin[lane])&&
            fields->read(0x34u+4u*lane)==std::bit_cast<std::uint32_t>(button.image_14c_150[lane]),
            "Button6F6CD1 SAME origin/destination changed across FULL POST");
    }catch(...){fields->failed=true;throw;}
}

void detail::BnwNormalButtonSubobject118Backing::execute_slot0_4535b0_owned(NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button6F6C7E actual118 actor absent");
    const auto fields=actual->impl_;fields->first_value_without_callbacks(button);
    require(fields->read(0)==0x009f3570u,"Button6F6C7E actual constructor vptr changed");
    //Fresh actual9F3570+0=4535B0 and its first E9->5C7B50 are frozen
    //in slot0-veneer-qualification. No inferred vtable or native PE call.
    execute_coordinates5c7b50_owned(button);fields->first_value_without_callbacks(button);
}

void detail::BnwNormalButtonSubobject118Backing::execute_coordinates5c7b50_owned(NotificationLayoutButtonState& button){
    const auto actual=button.backing_118_;require(actual&&actual->impl_,"Button5C7B50 actual118 actor absent");
    const auto fields=actual->impl_;fields->first_value_without_callbacks(button);
    try{
        if(fields->source44_state==Impl::Source44State::constructed_null){
            require(fields->read_genuine_source44_null_without_callbacks(button),"Button5C7B51 physical NULL changed");
            //Actual5C7B51 JZ5C7BCD: no UV read/write, no arithmetic bank
            //capture, no new coverage and no CURRENT/native address claim.
            fields->first_value_without_callbacks(button);return;
        }
        require(fields->source44_state==Impl::Source44State::selected_keeper,
            "Button5C7B50 source44 actual object binder is unknown");
        const auto source=fields->selected_source44.lock();fields->require_same_selected_keeper_without_callbacks(button,source);
        const auto owner=fields->core.lock();const auto scope=owner->layout_resource_scope.lock();
        require(scope&&owner->layout_resource_bound&&owner->layout_resource_operation&&
            owner->layout_resource_actual_tree,"Button5C7B50 actual selected resource scope retired");
        const auto& operation=owner->source->current_operation_loan();operation.require_origin_without_callbacks();
        require(owner->layout_resource_operation==&operation&&owner->layout_resource_serial==operation.operation_serial(),
            "Button5C7B50 source44 binder is not SAME entered Source/resource operation");
        const auto* const tree=owner->layout_resource_actual_tree;
        const auto check=[&]{fields->first_value_without_callbacks(button);
            require(owner->layout_resource_bound&&owner->layout_resource_operation==&operation&&
                owner->layout_resource_actual_tree==tree&&
                !owner->layout_resource_scope.owner_before(scope)&&!scope.owner_before(owner->layout_resource_scope),
                "Button5C7B50 original selected source scope replaced");
            fields->require_same_selected_keeper_without_callbacks(button,source);};
        check();scope->require_full(operation,tree);check();scope->require_entry(operation,tree,source);check();
        const std::array<std::uint32_t,4> input={fields->read(0x34),fields->read(0x38),fields->read(0x2c),fields->read(0x30)};
        const auto same_inputs=[&]{check();require(
            fields->read(0x34)==input[0]&&fields->read(0x38)==input[1]&&fields->read(0x2c)==input[2]&&fields->read(0x30)==input[3]&&
            std::bit_cast<std::uint32_t>(button.image_14c_150[0])==input[0]&&
            std::bit_cast<std::uint32_t>(button.image_14c_150[1])==input[1]&&
            std::bit_cast<std::uint32_t>(button.image_144_148[0])==input[2]&&
            std::bit_cast<std::uint32_t>(button.image_144_148[1])==input[3],
            "Button5C7B50 canonical physical geometry differs from SAME actual typed receiver");};
        same_inputs();fields->full_owned(button);same_inputs();
        require(fields->store_count<=UINT64_MAX-4,"Button5C7B50 physical store journal exhausted");
        const auto before=fields->store_count;
        //The genuine keeper is the positive source44 OBJECT. This executes
        //the reached arithmetic/stores using its actual114/118, while the
        //separate numeric native32 source44 lane stays UNKNOWN/uncovered.
        bnw_button118_execute_actual_5c7b50_nonnull_owned(fields->words,
            source->native_114_118_[0],source->native_114_118_[1]);
        fields->written|=0x00f00000u;fields->store_count+=4;
        //Typed views are read from newly written physical words; no typed
        //long-double result is serialized to pretend these x87 stores ran.
        button.uv_170_174[0]=std::bit_cast<float>(fields->read(0x58));
        button.uv_170_174[1]=std::bit_cast<float>(fields->read(0x5c));
        button.uv_168_16c[0]=std::bit_cast<float>(fields->read(0x50));
        button.uv_168_16c[1]=std::bit_cast<float>(fields->read(0x54));
        fields->full_owned(button);same_inputs();scope->require_entry(operation,tree,source);same_inputs();
        require(fields->store_count==before+4,"Button5C7B50 actual four-store completion changed across FULL POST");
        for(std::size_t lane=0;lane!=2;++lane)require(
            fields->read(0x58u+4u*lane)==std::bit_cast<std::uint32_t>(button.uv_170_174[lane])&&
            fields->read(0x50u+4u*lane)==std::bit_cast<std::uint32_t>(button.uv_168_16c[lane]),
            "Button5C7B50 actual physical/typed UV association changed across FULL POST");
        //No CURRENT issuance, native32 address store, tail71A000 return or
        //Invocation follows this bounded prefix supplier.
    }catch(...){fields->failed=true;throw;}
}
