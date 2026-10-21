// PRIVATE after complete mutable XML84, Context constructor and actual Gui.
// Native5D2920 receives a VALUE cursor copied from THIS new XML84 document.
// It never consumes cold source-authored metadata or renews its old read stamp.
struct BnwWorldViewControlTree::ContextRootDefaultFieldsEvidence final {
    using Return=detail::NotificationContextRootDefaultFieldsReturn;
    using Prepared=detail::NotificationContextLuaStatePreparation;
    using State=detail::NotificationLayoutContextOrdinaryState;
    using Known=detail::NotificationLabelColorSetKnownBytes;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Core> core;
    const std::weak_ptr<ContextRootReadFileFrame> frame;
    const std::weak_ptr<ContextRootXml84> document;
    const std::shared_ptr<const Prepared> prepared;
    const std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence;
    const std::shared_ptr<detail::NotificationContextGuiPort> gui;
    const detail::BnwCompletedFontLoan*const font;
    const std::string style;
    const Known colors;
    const std::size_t effects;
    ContextRootDefaultFieldsEvidence(std::shared_ptr<Core> c,std::shared_ptr<ContextRootReadFileFrame> f,
        std::shared_ptr<ContextRootXml84> d,std::shared_ptr<const Prepared> p,
        std::shared_ptr<const Core::LayoutReceiverOccurrence> o,std::shared_ptr<detail::NotificationContextGuiPort> g,
        const detail::BnwCompletedFontLoan*font_value,std::string style_value,Known color_value,std::size_t effect_count)
        :core(std::move(c)),frame(std::move(f)),document(std::move(d)),prepared(std::move(p)),
         occurrence(std::move(o)),gui(std::move(g)),font(font_value),style(std::move(style_value)),colors(color_value),effects(effect_count){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& operation,
        const std::shared_ptr<const Prepared>& actual_prepared)const{
        require(creator==std::this_thread::get_id(),"Root5D fields foreign issuer FIRST");
        const auto read=frame.lock();require(read!=nullptr,"Root5D actual read frame retired FIRST");
        read->first(); // B4 canonical Source pointer FIRST before old raw op.
        const auto actual=core.lock();const auto parsed=document.lock();
        require(actual&&parsed&&read->operation==&operation&&State::Impl::same(read->prepared,prepared)&&
            State::Impl::same(prepared,actual_prepared)&&State::Impl::same(actual->context_lua_preparation,prepared)&&
            State::Impl::same(read->occurrence,occurrence)&&State::Impl::same(read->expected_xml84,parsed)&&
            actual->context_layout_baseline&&actual->context_layout_baseline->base_.receiver_owned==read->receiver&&
            parsed->storage&&parsed->storage->document&&parsed->parser_outcome==true&&
            (parsed->stage==ContextRootXml84::Stage::name_complete||parsed->stage==ContextRootXml84::Stage::defaults_complete),
            "Root5D actual new XML84/read receiver/installed518 changed");
        const auto&defaults=actual->context_default_font_style;
        require(defaults.creator==std::this_thread::get_id()&&defaults.allocation==actual.get()&&!defaults.reading&&
            !defaults.read_source_serial&&defaults.read_occurrence.expired()&&defaults.font_538.get()==font&&
            defaults.value==style&&defaults.colors_53c==colors&&defaults.effect_count==effects&&effects&&effects<=defaults.effects.size()&&
            defaults.effects[effects-1]==Core::ContextDefaultFontStyle::Effect::read_return&&
            State::Impl::same(actual->context_gui_port,gui),
            "Root5D physical fields no longer match the real returned function");
        if(font)font->require_current_without_callbacks();
        if(gui)gui->impl_->first(operation);
        read->first();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation,
        const std::shared_ptr<const Prepared>& actual_prepared)const{
        const auto held_prepared=actual_prepared; // no allocating/callback borrow.
        first(operation,held_prepared);
        const auto read=frame.lock();const auto actual=core.lock();
        require(read&&actual,"Root5D real physical owner retired before Source FULL");
        read->full();first(operation,held_prepared);
        if(font){font->require_source_owned(operation);first(operation,held_prepared);read->full();first(operation,held_prepared);}
    }
};

struct detail::NotificationContextRootDefaultFieldsReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const std::shared_ptr<BnwWorldViewControlTree::ContextRootDefaultFieldsEvidence> evidence;
    explicit Impl(std::shared_ptr<BnwWorldViewControlTree::ContextRootDefaultFieldsEvidence> value):evidence(std::move(value)){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& operation,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&evidence,"Root5D return facade retired/foreign FIRST");
        evidence->first(operation,prepared);
        const auto document=evidence->document.lock();
        require(document&&document->stage==BnwWorldViewControlTree::ContextRootXml84::Stage::defaults_complete&&
            document->defaults_return&&document->defaults_return->impl_.get()==this,
            "Root5D actual document returned-fields control was replaced/unpublished");
    }
};
detail::NotificationContextRootDefaultFieldsReturn::NotificationContextRootDefaultFieldsReturn(std::shared_ptr<Impl> value)
    :impl_(std::move(value)){}
detail::NotificationContextRootDefaultFieldsReturn::~NotificationContextRootDefaultFieldsReturn(){impl_->alive.store(false);}
void detail::NotificationContextRootDefaultFieldsReturn::Guard::require_current_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{impl_->first(operation,prepared);}
void detail::NotificationContextRootDefaultFieldsReturn::Guard::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
    const auto saved=impl_;const auto held_prepared=prepared;
    saved->first(operation,held_prepared);saved->evidence->full(operation,held_prepared);saved->first(operation,held_prepared);
}
detail::NotificationContextRootDefaultFieldsReturn::Guard
detail::NotificationContextRootDefaultFieldsReturn::capture_guard_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
    const auto saved=impl_;saved->first(operation,prepared);return Guard(saved);
}

std::shared_ptr<const detail::NotificationContextRootDefaultFieldsReturn>
BnwWorldViewControlTree::read_context_root_default_fields_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::shared_ptr<ContextRootReadFileFrame> frame){
    using State=detail::NotificationLayoutContextOrdinaryState;
    using Effect=Core::ContextDefaultFontStyle::Effect;using Return=detail::NotificationContextRootDefaultFieldsReturn;
    const auto actual=core_; // retain physical Core before the first callback boundary.
    require(frame!=nullptr,"Root5D requires actual entered read frame");frame->full();
    const auto state=frame->state.lock();const auto document=frame->expected_xml84;
    require(actual&&state&&document&&document->storage&&document->storage->document&&
        document->stage==ContextRootXml84::Stage::name_complete&&document->parser_outcome==true&&
        frame->operation==&operation&&State::Impl::same(state->impl_->xml84_current,document)&&
        !document->defaults_return,"Root5D requires newly parsed XML84+actual Name branch exactly once");
    const auto prepared=frame->prepared;const auto occurrence=frame->occurrence;
    const auto gui=actual->context_gui_port;
    // Old cold id0 metadata is never a fresh document/field receipt. Bootstrap
    // must defer that old hook, not accept/relabel/replay its successful stamp.
    auto&defaults=actual->context_default_font_style;
    require(defaults.creator==std::this_thread::get_id()&&defaults.allocation==actual.get()&&!defaults.reading&&
        !defaults.read_source_serial&&defaults.read_occurrence.expired()&&defaults.effect_count==3&&
        defaults.value.empty()&&!defaults.font_538,"required bootstrap deferred cold-root default read; no old style receipt reuse");
    state->impl_->native_constructor_return->impl_->port->require_completed();
    enum class Phase {inside_function,temps_retired};Phase phase=Phase::inside_function;
    auto expected_colors=defaults.colors_53c;
    const detail::BnwCompletedFontLoan*expected_font=nullptr;
    std::string expected_style;
    const auto first=[&]{
        frame->first();require(frame->operation==&operation&&State::Impl::same(frame->expected_xml84,document)&&
            State::Impl::same(state->impl_->xml84_current,document)&&document->storage&&document->storage->document&&
            document->stage==ContextRootXml84::Stage::name_complete&&!document->defaults_return&&
            document->storage->document->name=="Context"&&
            defaults.reading==(phase==Phase::inside_function)&&!defaults.read_source_serial&&defaults.read_occurrence.expired()&&
            defaults.font_538.get()==expected_font&&defaults.value==expected_style&&defaults.colors_53c==expected_colors&&
            State::Impl::same(actual->context_gui_port,gui)&&State::Impl::same(actual->context_lua_preparation,prepared),
            "Root5D fresh actual cursor/fields/GUI/installed518 changed");
        if(expected_font)expected_font->require_current_without_callbacks();
        frame->first();
    };
    const auto full=[&]{first();frame->full();first();};
    const auto attribute=[&](std::string_view name)->const std::string*{
        const auto&attrs=document->storage->document->attributes;
        const auto found=std::find_if(attrs.begin(),attrs.end(),[&](const auto&item){return item.first==name;});
        return found==attrs.end()?nullptr:&found->second;
    };
    const auto conversion=[&](const std::string& text){
        require(std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<128;}),
            "required Root5D nonASCII/embeddedNUL string conversion owner");
        std::string result=text;full();return result;
    };
    defaults.reading=true;
    {
        // Native835550 copied cursor and temporary FString own-x64 lifetime.
        // Their data comes exclusively from the new mutable XML84 arena.
        struct Temps final {
            std::shared_ptr<ContextRootXml84> cursor;
            ~Temps()=default;
        }temps{document};
        struct Exit final {Core::ContextDefaultFontStyle&value;~Exit(){value.reading=false;}}exit{defaults};
        full();
        const auto&children=temps.cursor->storage->document->children;
        require(std::none_of(children.begin(),children.end(),[](const auto&child){return child.name=="ColorSet";}),
            "required Root5D local ColorSet map registration from actual parsed cursor");
        // Native ctor map57C/580 is empty and this fresh5D has no declarations.
        // No selected_context_color_sets metadata registry grants local lookup.
        const auto&map=*state->impl_->native_constructor_return->impl_->port->map57c;
        require(map.count==0&&map.sentinel&&map.sentinel->left==map.sentinel.get()&&
            map.sentinel->parent==map.sentinel.get()&&map.sentinel->right==map.sentinel.get(),
            "Root5D actual initial local palette map/sentinel changed");
        defaults.record(Effect::local_declarations_absent);full();
        if(const auto value=attribute("ColorSet")){
            const auto request=conversion(*value);full();
            require(gui&&gui->impl_,"Root5D ColorSet actual canonical Gui missing");
            const auto storage=gui->impl_;const auto registry=storage->selected_color_sets;
            const auto activation=storage->selected_provider_activation;
            require(registry&&activation,"Root5D genuine selected palette registry/activation missing");
            const auto pure=[&]{
                first();storage->first(operation);first();registry->first();first();activation->first(storage);first();
                require(State::Impl::same(actual->context_gui_port,gui)&&State::Impl::same(gui->impl_,storage)&&
                    State::Impl::same(storage->selected_color_sets,registry)&&State::Impl::same(storage->selected_provider_activation,activation),
                    "Root5D canonical palette owners changed");
            };
            pure();activation->require_current_owned(operation,storage);pure();frame->full();pure();
            const bool enabled=activation->gameplay_enabled_without_callbacks(storage);pure();
            const auto quotes=operation.color_atlas_resources_owned();pure();frame->full();pure();
            for(std::size_t i=0;i<quotes.size();++i){const auto&quote=quotes[i];const auto&saved=registry->atlas[i];
                require(quote.payload_identity.sha256==saved.payload_identity.sha256&&
                    State::Impl::same(quote.bytes,saved.bytes)&&State::Impl::same(quote.xml,saved.xml),
                    "Root5D actual selected Bank ColorAtlas quote changed");}
            const auto key=native_color_load_detail::hash_query(request);pure();
            const detail::NotificationLabelColorSetKnownBytes*found=nullptr;
            if(enabled){const auto it=registry->active_provider.sets.find(key);if(it!=registry->active_provider.sets.end())found=&it->second;}
            if(!found){const auto it=registry->loaded.sets.find(key);if(it!=registry->loaded.sets.end())found=&it->second;}
            pure();if(found){
                const auto copied=*found; // immutable registry retained before effects.
                for(std::size_t word=0;word<16;++word)for(std::size_t byte=0;byte<4;++byte)
                    defaults.colors_53c[word*4+byte]=copied[word*4+byte];
                expected_colors=copied;defaults.record(Effect::root_colorset_copy);
            }else defaults.record(Effect::root_colorset_miss); // true completed global MISS, unchanged field.
            pure();frame->full();pure();
        }else defaults.record(Effect::root_colorset_absent);
        full();
        for(const auto name:std::array<std::string_view,4>{"Color0","Color1","Color2","Color3"})
            require(!attribute(name),"required Root5D direct Color store580780 from actual parsed cursor");
        defaults.record(Effect::direct_colors_absent);full();
        if(const auto value=attribute("Font")){
            const auto request=conversion(*value);full();require(gui!=nullptr,"Root5D Font538 canonical Gui missing");
            gui->require_current_owned(operation);full();
            auto font=gui->begin_font_lookup_owned(operation,std::string_view(request));full();
            require(font!=nullptr,"required Root5D actual null Font lookup outcome mapping");
            font->require_current_without_callbacks();full();font->require_source_owned(operation);full();
            require(!defaults.font_538,"Root5D Font538 changed during genuine completed lookup");
            defaults.record(Effect::font_lookup_return);
            expected_font=font.get();defaults.font_538=std::move(font);defaults.record(Effect::font_store);full();
            defaults.font_538->require_source_owned(operation);full();
        }else defaults.record(Effect::font_absent);
        //409390 reads this cursor's FontStyle; absent keeps ctor value, present
        // ASCII FString assignment owns bytes before publishing534.
        if(const auto value=attribute("FontStyle")){
            auto staged=conversion(*value);full();
            auto expected=staged;full(); // actual534 remains old across allocation.
            defaults.value.swap(staged);expected_style.swap(expected);defaults.record(Effect::style_store);
        }else defaults.record(Effect::style_absent);
        full();
    } //8353F0 + temporary FString DTOR + argument cursor DTOR, no Lua authority.
    phase=Phase::temps_retired;full();defaults.record(Effect::read_return);full();
    // DO NOT publish cold read_occurrence/read_source_serial or parsed_source
    // stamp. A different completed RootRead owner is mandatory for future use.
    auto style=defaults.value;full();
    const auto colors=defaults.colors_53c;const auto effects=defaults.effect_count;
    const auto font=defaults.font_538.get();
    auto evidence=std::make_shared<ContextRootDefaultFieldsEvidence>(actual,frame,document,prepared,occurrence,gui,
        font,std::move(style),colors,effects);full();evidence->full(operation,prepared);
    auto data=std::make_shared<Return::Impl>(evidence);evidence->full(operation,prepared);
    auto returned=std::shared_ptr<const Return>(new Return(data));evidence->full(operation,prepared);
    require(!document->defaults_return&&document->stage==ContextRootXml84::Stage::name_complete,
        "Root5D return field changed during facade allocation");
    document->defaults_return=returned;document->stage=ContextRootXml84::Stage::defaults_complete;
    const auto guard=returned->capture_guard_without_callbacks(operation,prepared);
    guard.require_current_owned(operation,prepared);guard.require_current_without_callbacks(operation,prepared);
    return returned; // actual5D only; not ControlsRef/wholeRootRead/attach completion.
}
