    // Actual distinct outer0x90 command window over the owned inner0x20 table.
    // Only proved header/begin/end stores are projected here; padding is unread.
    struct NormalGuiOuterWindow final {
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> prefix;
        const std::uint32_t index;
        std::optional<std::uint32_t> header_0,begin_58,end_5c;
        bool opened=false,closed=false;
        NormalGuiOuterWindow(std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> p,std::uint32_t i)
            :prefix(std::move(p)),index(i){}
    };
    struct NormalGuiManagerRegistration final {
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<const BnwNormalRootBatchHistory::Impl> root;
        const std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> prefix;
        const std::shared_ptr<NormalGuiOuterWindow> outer;
        const std::uint32_t outer_index;
        const std::size_t table_index;
        const std::uint64_t serial,batch_revision;
        std::optional<std::uint32_t> current_inner;
        bool drained=false,sealed=false;
        NormalGuiManagerRegistration(std::weak_ptr<const BnwNormalRootBatchHistory::Impl> r,
            std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> p,std::shared_ptr<NormalGuiOuterWindow> o,
            std::uint32_t i,std::size_t t,std::uint64_t s,std::uint64_t b)
            :root(std::move(r)),prefix(std::move(p)),outer(std::move(o)),outer_index(i),
             table_index(t),serial(s),batch_revision(b){}
    };
    std::vector<std::shared_ptr<NormalGuiManagerRegistration>> normal_queue_registrations;
    bool normal_queue_registering=false,normal_queue_dispatching=false,normal_queue_drained=false;
    bool normal_queue_permission_capturing=false;
    std::size_t normal_queue_dispatch_count=0;
    std::optional<std::size_t> normal_queue_selected_registration;
    std::weak_ptr<const BnwNormalQueueDrainPermission::Impl> normal_queue_current_permission;

