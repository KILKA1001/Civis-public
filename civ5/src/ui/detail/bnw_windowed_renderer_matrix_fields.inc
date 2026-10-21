    // Actual owned port state is disengaged until the real windowed supplier
    // completes. No identity/default matrix or immutable Original state.
    struct WindowedMatrixState final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};
        std::array<std::uint32_t,16> words;
        std::array<std::int32_t,2> dimensions;
        std::array<std::uint32_t,2> arithmetic_words,numeric_before,numeric_after;
        std::uint64_t gui_revision,source_serial,update;
        bool completed=false;
        ~WindowedMatrixState(){alive.store(false);}
    };
    std::shared_ptr<WindowedMatrixState> windowed_matrix;
    bool windowed_matrix_busy=false;
    std::uint64_t windowed_matrix_updates=0;
