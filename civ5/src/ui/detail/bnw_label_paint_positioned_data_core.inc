// PRIVATE Source10 successor. Included after actual Run/Paint and numeric/text
// leaves. Graphics owns the real early410900→5A7250 body and actual submit.
struct detail::BnwLabelPaintTextLoan::PositionedState {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const Guard paint;
    const BnwLabelPaintPreparedFlushLoan::Guard early;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    const std::unique_ptr<BnwLabelPaintPreparedFlushLoan> early_facade;
    const std::array<std::uint32_t,2> numeric;
    const std::shared_ptr<const BnwNormalCaptionFp32RectLoan> caption_rect;
    std::vector<BnwLabelPaintPositionedData::RecordData> records;
    std::vector<BnwLabelPaintPositionedData::GlyphData> glyphs;
    std::vector<BnwLabelPaintPositionedData::PassData> passes;
    PositionedState(Guard actual_paint,BnwLabelPaintPreparedFlushLoan::Guard actual_early,
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system,
        std::unique_ptr<BnwLabelPaintPreparedFlushLoan> actual_early_facade,
        std::array<std::uint32_t,2> actual_numeric,
        std::shared_ptr<const BnwNormalCaptionFp32RectLoan> actual_rect)
        :paint(std::move(actual_paint)),early(std::move(actual_early)),system(std::move(actual_system)),
         early_facade(std::move(actual_early_facade)),numeric(actual_numeric),caption_rect(std::move(actual_rect)){first();}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"591020 CPU DATA lexical lease retired/foreign");
        paint.require_current_without_callbacks();early.require_current_without_callbacks();
        const auto actual_rect=paint.actual_caption_fp32_rect_without_callbacks();
        require(actual_rect.get()==caption_rect.get()&&!actual_rect.owner_before(caption_rect)&&!caption_rect.owner_before(actual_rect),
            "CPU DATA lost SAME original Paint FP32 loan control");
        early.require_same_paint_guard_without_callbacks(paint);
        require(bool(system),"591020 CPU real170 descriptor absent");
        paint.require_system_font_borrower_without_callbacks(*system);
        require(paint.actual_numeric_profile_without_callbacks()==numeric,
            "591020 CPU actual CW/MXCSR authority changed");
        early.require_current_without_callbacks();paint.require_current_without_callbacks();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();paint.require_source_owned(operation);first();
        early.require_source_owned(operation);first();
        paint.require_source_owned(operation);first();
    }
};
struct detail::BnwLabelPaintPositionedData::Impl {
    // Canonical weak control only: Binding owns Use/Vertices/Planes/DATA.
    // A strong OpaqueRecordIdentity here would create a retention cycle.
    std::optional<graphics::detail::BnwNormalLabelDrawRecordBinding::WeakGuard> normal_draw_records;
    std::uint64_t normal_draw_records_revision=0;
    bool normal_draw_records_installing=false,normal_draw_records_installed=false;
    //One genuine history issuance per actual DATA allocation. Weak
    //control avoids a DATA/Item cycle; retirement never reissues it.
    std::weak_ptr<const BnwNormalItemOccurrence::Impl> normal_item_occurrence;
    bool normal_item_occurrence_issued=false;
    const std::shared_ptr<BnwLabelPaintTextLoan::PositionedState> state;
    explicit Impl(std::shared_ptr<BnwLabelPaintTextLoan::PositionedState> actual):state(std::move(actual)){
        state->first();
    }
};
detail::BnwLabelPaintPositionedData::BnwLabelPaintPositionedData(std::shared_ptr<Impl> actual):impl_(std::move(actual)){}
detail::BnwLabelPaintPositionedData::~BnwLabelPaintPositionedData(){impl_->state->alive.store(false);}
detail::BnwLabelPaintPositionedData::Guard::Guard(std::shared_ptr<Impl> actual):impl_(std::move(actual)){}
detail::BnwLabelPaintPositionedData::Guard detail::BnwLabelPaintPositionedData::capture_guard_without_callbacks()const{
    const auto actual=impl_;actual->state->first();return Guard(actual);
}
void detail::BnwLabelPaintPositionedData::Guard::require_current_without_callbacks()const{impl_->state->first();}
void detail::BnwLabelPaintPositionedData::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    impl_->state->full(operation);
}
void detail::BnwLabelPaintPositionedData::Guard::require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower& actual)const{
    impl_->state->first();require(impl_->state->system.get()==&actual,"591020 CPU DATA another170");
    impl_->state->paint.require_system_font_borrower_without_callbacks(actual);impl_->state->first();
}
void detail::BnwLabelPaintPositionedData::Guard::require_actual_renderer_without_callbacks(const UiScreenGlRenderer& actual)const{
    impl_->state->first();impl_->state->paint.require_actual_renderer_without_callbacks(actual);impl_->state->first();
}
std::span<const detail::BnwLabelPaintPositionedData::GlyphData>
detail::BnwLabelPaintPositionedData::Guard::glyphs_without_callbacks()const{impl_->state->first();return impl_->state->glyphs;}
std::span<const detail::BnwLabelPaintPositionedData::RecordData>
detail::BnwLabelPaintPositionedData::Guard::records_without_callbacks()const{impl_->state->first();return impl_->state->records;}
std::span<const detail::BnwLabelPaintPositionedData::PassData>
detail::BnwLabelPaintPositionedData::Guard::passes_without_callbacks()const{impl_->state->first();return impl_->state->passes;}
std::array<std::uint32_t,2>
detail::BnwLabelPaintPositionedData::Guard::actual_numeric_profile_without_callbacks()const{
    impl_->state->first();return impl_->state->numeric;
}

detail::BnwLabelPaintTextLoan::Guard
detail::BnwLabelPaintPositionedData::Guard::captured_paint_guard_without_callbacks()const{
    const auto actual=impl_->state;actual->first();
    const auto result=actual->paint;actual->first();
    actual->early.require_same_paint_guard_without_callbacks(result);actual->first();
    return result;
}
detail::BnwLabelPaintPreparedFlushLoan::Guard
detail::BnwLabelPaintPositionedData::Guard::captured_early_flush_guard_without_callbacks()const{
    const auto actual=impl_->state;actual->first();
    const auto result=actual->early;actual->first();
    result.require_same_paint_guard_without_callbacks(actual->paint);actual->first();
    return result;
}
void detail::BnwLabelPaintPositionedData::Guard::require_same_paint_guard_without_callbacks(
    const BnwLabelPaintTextLoan::Guard& actual_paint)const{
    const auto actual=impl_->state;const auto expected=actual_paint;
    actual->first();expected.require_current_without_callbacks();actual->first();
    actual->paint.require_same_paint_guard_without_callbacks(expected);actual->first();
    actual->early.require_same_paint_guard_without_callbacks(expected);actual->first();
    expected.require_current_without_callbacks();actual->first();
}

graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
detail::BnwLabelPaintPositionedData::Guard::capture_normal_draw_records_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto actual=impl_;const auto data=Guard(actual);
    data.require_current_without_callbacks();
    require(actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
        actual->normal_draw_records_revision==1&&bool(actual->normal_draw_records),
        "Item capture DATA has no genuine one-time normal CPU Binding association");
    const auto binding=actual->normal_draw_records->lock_current_without_callbacks();
    data.require_current_without_callbacks();
    data.require_same_normal_draw_records_source_owned(operation,binding);
    data.require_current_without_callbacks();return binding;
}
void detail::BnwLabelPaintPositionedData::Guard::require_same_normal_draw_records_without_callbacks(
    const graphics::detail::BnwNormalLabelDrawRecordBinding::Guard& supplied)const{
    const auto actual=impl_;const auto data=Guard(actual);const auto binding=supplied;
    data.require_current_without_callbacks();
    require(actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
        actual->normal_draw_records_revision==1&&bool(actual->normal_draw_records),
        "Item DATA original Binding association changed at PURE cut");
    binding.require_current_without_callbacks();data.require_current_without_callbacks();
    const auto canonical=actual->normal_draw_records->impl_.lock();
    require(canonical&&binding.impl_&&canonical.get()==binding.impl_.get()&&
        !canonical.owner_before(binding.impl_)&&!binding.impl_.owner_before(canonical),
        "Item DATA PURE cut is not SAME genuine Graph Binding control");
    actual->normal_draw_records->require_current_without_callbacks();
    binding.require_current_without_callbacks();data.require_current_without_callbacks();
}
void detail::BnwLabelPaintPositionedData::Guard::require_same_normal_draw_records_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::Guard& supplied)const{
    const auto actual=impl_;const auto data=Guard(actual);const auto binding=supplied;
    const auto active=[&]{
        data.require_current_without_callbacks();
        require(actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
            actual->normal_draw_records_revision==1&&bool(actual->normal_draw_records),
            "Item capture DATA original Binding association changed");
        binding.require_current_without_callbacks();data.require_current_without_callbacks();
        const auto canonical=actual->normal_draw_records->impl_.lock();
        require(canonical&&binding.impl_&&canonical.get()==binding.impl_.get()&&
            !canonical.owner_before(binding.impl_)&&!binding.impl_.owner_before(canonical),
            "Item capture DATA is not SAME genuine Graph Binding control");
        actual->normal_draw_records->require_current_without_callbacks();data.require_current_without_callbacks();
    };
    active();data.require_source_owned(operation);active();binding.require_source_owned(operation);active();
    data.require_source_owned(operation);active();
}

void detail::BnwLabelPaintPositionedData::attach_normal_draw_records_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::Guard& incoming)const{
    // Own every operand before FULL. Raw this is used only in the initial
    // PURE association check; callbacks may destroy the original DATA facade.
    const auto actual=impl_;
    const auto data=Guard(actual);
    const auto records=incoming;
    const auto first=[&]{
        data.require_current_without_callbacks();
        records.require_current_without_callbacks();
        data.require_current_without_callbacks();
    };
    const auto full=[&]{
        first();data.require_source_owned(operation);first();
        records.require_source_owned(operation);first();
        data.require_source_owned(operation);first();
    };
    try{
        // Capture-first matcher stays inside original retirement boundary.
        // Lambda definitions above execute no checks/callbacks/WGL.
        records.require_same_positioned_data_without_callbacks(*this);
        first();
        require(!actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
            !actual->normal_draw_records&&actual->normal_draw_records_revision==0,
            "Normal draw-record DATA association already installed/entered");
        const auto weak=records.capture_weak_without_callbacks();
        first();weak.require_current_without_callbacks();first();
        actual->normal_draw_records_installing=true;
        full();
        require(actual->normal_draw_records_installing&&!actual->normal_draw_records_installed&&
            !actual->normal_draw_records&&actual->normal_draw_records_revision==0,
            "Normal draw-record DATA association changed during FULL");
        // Inline weak copy only: no strong Binding/record identity is retained
        // in DATA. The immutable WeakGuard always denotes this same control.
        actual->normal_draw_records.emplace(weak);
        actual->normal_draw_records_revision=1;
        actual->normal_draw_records_installed=true;
        actual->normal_draw_records_installing=false;
        const auto published=[&]{
            first();
            require(actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
                actual->normal_draw_records_revision==1&&bool(actual->normal_draw_records),
                "Normal draw-record DATA publication journal changed");
            actual->normal_draw_records->require_current_without_callbacks();first();
        };
        published();full();published();
    }catch(...){
        // Retire DATA authority FIRST. Weak ownership cleanup cannot revive a
        // Binding facade or execute Graphics/Source/world cleanup callbacks.
        actual->state->alive.store(false);
        actual->normal_draw_records.reset();
        actual->normal_draw_records_installing=false;
        throw;
    }
}
graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
detail::BnwLabelPaintPositionedData::originating_normal_draw_records_without_callbacks()const{
    const auto actual=impl_;
    const auto data=Guard(actual);
    const auto first=[&]{
        data.require_current_without_callbacks();
        require(actual->normal_draw_records_installed&&!actual->normal_draw_records_installing&&
            actual->normal_draw_records_revision==1&&bool(actual->normal_draw_records),
            "Normal draw-record DATA has no completed one-time association");
        actual->normal_draw_records->require_current_without_callbacks();
        data.require_current_without_callbacks();
    };
    first();
    const auto result=actual->normal_draw_records->lock_current_without_callbacks();
    first();result.require_current_without_callbacks();first();
    // This lock retains storage, never renews retired Binding/transport/DATA.
    return result;
}

std::unique_ptr<detail::BnwLabelPaintPositionedData>
detail::BnwLabelPaintTextLoan::positioned_data_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<graphics::GlBnwSystemFontAtlas> actual_atlas)const{
    //ALL operands and closed Guards byVALUE BEFORE any callback/allocation.
    //Do not reread this/rawPaint facade or caller-owned Atlas entries later.
    const auto actual=impl_;const auto standalone=standalone_;const auto paint=capture_guard_without_callbacks();
    const auto caption_rect=paint.actual_caption_fp32_rect_without_callbacks();
    const auto system=standalone?standalone->system:actual->system;const auto atlas=std::move(actual_atlas);
    const auto first=[&]{paint.require_current_without_callbacks();
        require(bool(system)&&bool(atlas),"591020 actual170/Atlas mandatory owner absent");
        paint.require_system_font_borrower_without_callbacks(*system);paint.require_current_without_callbacks();};
    const auto full=[&]{first();paint.require_source_owned(operation);first();};
    first();const auto descriptor=paint.actual_positioned_record_without_callbacks();first();
    const auto numeric=paint.actual_numeric_profile_without_callbacks();first();
    BnwLabelPaintCpuNumeric::profile(numeric);first();full();
    //59104F6C5FB0 even empty text. Native+170753CF0 is literalTRUE; all
    //three real4E1860 calls precede preparation/flush, not a measure shortcut.
    std::uint32_t ellipsis_width=0;
    const std::uint32_t dot_lookup=(descriptor.flags&0x80000000u)?0x2au:0x2eu;
    for(unsigned i=0;i<3;++i){
        require(system->has_glyph_owned(operation,paint,dot_lookup),"591020 actual753CF0 did not returnTRUE");full();
        const auto bits=atlas->glyph_bits_native_owned(operation,*system,paint,dot_lookup);full();
        ellipsis_width+=BnwLabelPaintCpuNumeric::advance_low32(bits[8],numeric);first();
    }
    (void)ellipsis_width; //Computed realNative prefix; positive truncation is below STOP.
    full();auto flush=atlas->prepare_and_flush_native_owned(operation,*system,paint);full();
    require(bool(flush),"591020 genuine completed410900/5A7250 receipt missing");
    const auto early=flush->capture_guard_without_callbacks();first();
    early.require_current_without_callbacks();early.require_same_paint_guard_without_callbacks(paint);
    early.require_source_owned(operation);full();early.require_current_without_callbacks();
    auto state=std::make_shared<PositionedState>(paint,early,system,std::move(flush),numeric,caption_rect);
    state->full(operation);state->first();
    //Actual C4 is physically retained by captured Run; this allocating copy
    //cannot destroy its input. Authority still must pass fullPOST afterwards.
    const auto bytes_view=paint.actual_c4_bytes_without_callbacks();state->first();
    const std::string bytes(bytes_view);state->full(operation);state->first();
    require(descriptor.count<=1,"591020 multiple actual records require true array supplier");
    std::uint32_t cursor=0,line_y=0,peak=0;
    if(descriptor.count){
        //Actual621320 lookup is BEFORE actualnewline; raw three FontStyle
        //keys retain signedDWORD eligibility and do not select shader/program.
        const auto style=paint.actual_positioned_style_passes_without_callbacks();state->first();
        if(descriptor.newline){
            const auto metrics=paint.actual_positioned_font_metrics_without_callbacks();state->first();
            cursor=0;
            line_y+=BnwLabelPaintCpuNumeric::newline_height_low32(std::bit_cast<std::uint32_t>(metrics[2]),numeric);
            line_y+=static_cast<std::uint32_t>(static_cast<std::int32_t>(descriptor.leading));state->first();
        }
        BnwLabelPaintCpuTextRecords::require_text_record_known(bytes,descriptor.begin,
            descriptor.byte_count,descriptor.source_discriminator,descriptor.truncate);state->first();
        state->records.push_back({descriptor.begin,descriptor.byte_count,descriptor.style,
            descriptor.source_discriminator,descriptor.inline_payload,descriptor.packed_colors,style});state->full(operation);
        const auto end=descriptor.begin+descriptor.byte_count;
        for(std::uint32_t at=descriptor.begin;at<end;){
            state->first();const auto decoded=BnwLabelPaintCpuTextRecords::decode_486330_known(bytes,at,end,descriptor.flags);
            require(system->has_glyph_owned(operation,paint,decoded.lookup),"591020 actual753CF0 did not returnTRUE");state->full(operation);
            const auto bits=atlas->glyph_bits_native_owned(operation,*system,paint,decoded.lookup);state->full(operation);
            require(state->glyphs.size()<UINT32_MAX,"591020 CPU glyph ordinal exhausted");
            const auto glyph_id=static_cast<std::uint32_t>(state->glyphs.size());
            state->glyphs.push_back({0,at,decoded.byte_count,decoded.scalar,decoded.lookup,cursor,line_y,bits});state->full(operation);
            //Native corner math occurs before signed gates; this CPU lender
            //records scalar/pass DATA only. Graphics MUST run real quad math
            //under numericProfile before treating these as vertex/draw input.
            for(std::uint32_t pass=0;pass<3;++pass){
                if(BnwLabelPaintCpuTextRecords::actual_style_pass_admitted(style[pass])){
                    state->passes.push_back({glyph_id,pass,descriptor.packed_colors[pass]});state->full(operation);
                }
            }
            cursor=BnwLabelPaintCpuTextRecords::next_cursor(cursor,
                BnwLabelPaintCpuNumeric::advance_low32(bits[8],numeric));state->first();
            peak=BnwLabelPaintCpuTextRecords::next_peak(peak,cursor);at+=decoded.byte_count;
        }
    }
    (void)peak;state->full(operation);state->first();
    auto data=std::make_shared<BnwLabelPaintPositionedData::Impl>(state);state->full(operation);state->first();
    auto result=std::unique_ptr<BnwLabelPaintPositionedData>(new BnwLabelPaintPositionedData(data));
    state->full(operation);state->first();
    //DATA stops BEFORE591E26 real688B00. No fake submit/flags40clear,
    //lateMISS autoFlush/Shader/vertex-ready/full591020 completion is issued.
    return result;
}
