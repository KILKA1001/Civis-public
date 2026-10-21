// Inside actual private Core: real40F9C0 defaults, not caller snapshots.
struct ContextDefaultFontStyle final {
    using Known=detail::NotificationLabelColorSetKnownBytes;
    const std::thread::id creator=std::this_thread::get_id();
    const Core* const allocation;
    std::string value; // ctor534 empty FString40FA71..7F
    std::unique_ptr<detail::BnwCompletedFontLoan> font_538; // ctor538 NULL
    Known colors_53c; // actual540EB0 known writes, byte63 unknown
    std::weak_ptr<const LayoutReceiverOccurrence> read_occurrence;
    std::uint64_t read_source_serial=0;
    bool reading=false;
    enum class Effect {ctor_style_empty,ctor_font_null,ctor_colors,
        local_declarations_absent,root_colorset_absent,root_colorset_copy,root_colorset_miss,
        direct_colors_absent,font_absent,font_lookup_return,font_store,
        style_absent,style_store,read_return};
    std::array<Effect,16> effects{};
    std::size_t effect_count=0;
    static Known construct_colors(){
        native_color_load_detail::ColorSetBytes scratch{};
        native_color_load_detail::default_color_set(scratch);
        Known result{};for(std::size_t i=0;i<63;++i)result[i]=scratch[i];
        return result; // byte63 has NO ctor assignment, no zero guess.
    }
    void record(Effect event){require(effect_count<effects.size(),"Context defaults effect history exhausted");
        effects[effect_count++]=event;}
    explicit ContextDefaultFontStyle(const Core* actual):allocation(actual),value(),font_538(),colors_53c(construct_colors()){
        record(Effect::ctor_style_empty);record(Effect::ctor_font_null);record(Effect::ctor_colors);
    }
};
ContextDefaultFontStyle context_default_font_style;
ContextDefaultFontStyle::Known default_context_colors_owned(const runtime::BnwWorldViewSourceOperationLoan&);
void require_context_default_font_owned();
void read_context_default_font_style_owned(const runtime::BnwWorldViewSourceOperationLoan&);
