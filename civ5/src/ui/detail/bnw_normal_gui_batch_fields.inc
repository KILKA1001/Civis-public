    // Real OWN normal-render policy; no inferred Original active split mode.
    // Only the canonical normal prefix entry executes these stores.
    struct NormalGuiOuterWindow;
    #include "bnw_normal_font_payload_arena_fields.inc"
    #include "bnw_normal_root_outer_manager_fields.inc"
    struct NormalGuiBatchState final {
        enum class Effect {viewport_zero,root_offset,root_alpha,root_arg4,root_arg5,
            viewport_dimensions,viewport_min_z,viewport_max_z,entered};
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};
        NormalFontPayloadQueue payload_queue;
        std::uint64_t revision=0;
        std::uint32_t viewport_selector=UINT32_MAX;
        std::array<std::uint32_t,2> root_offset{},root_depth{};
        std::uint32_t root_alpha=0;
        std::array<std::int32_t,2> dimensions{};
        std::array<std::uint32_t,2> viewport_depth{};
        std::array<Effect,9> history{};std::size_t count=0;
        void record(Effect e){require(count<history.size(),"Normal GUI batch prefix history exhausted");history[count++]=e;}
        ~NormalGuiBatchState(){alive.store(false);}
    };
    std::uint64_t normal_gui_batch_revision=0;
    bool normal_gui_batch_beginning=false;
    std::weak_ptr<NormalGuiBatchState> normal_gui_batch;
    std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> normal_gui_batch_prefix;
    //One closed historical Root control; read friends cannot issue it.
    std::weak_ptr<const BnwNormalRootBatchHistory::Impl> normal_root_history;
    bool normal_root_history_capturing=false;
