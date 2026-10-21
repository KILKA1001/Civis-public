// PRIVATE field implementation; caller must FULL-check its captured RootRead
// guard after EACH allocation/native effect. No callback-injected authority.
// Windows worker touches ONLY this immutable-path/FILETIME/dirty capsule.
// It never dereferences a Tree, SourceOp, Context frame, Lua or renderer.
#if defined(_WIN32)
struct detail::NotificationContextRootFileWatcher::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    enum class Stage {copied_path,attributes_captured,change_opened,wait_attempted};
    Stage stage=Stage::copied_path;
    const std::array<wchar_t,260> path;
    std::array<wchar_t,256> directory{};
    std::mutex worker_mutex;
    FILETIME last_write{};
    std::atomic<bool> dirty{false};
    HANDLE change=INVALID_HANDLE_VALUE;
    HANDLE wait=INVALID_HANDLE_VALUE;
    // Intrusive quarantine has no allocations. A failed wait-drain cannot free
    // a payload still held by KERNEL32 and cannot claim successful retirement.
    Impl* quarantine_next=nullptr;
    static inline thread_local Impl* quarantine_head=nullptr;
    static std::array<wchar_t,260> copied_path(std::array<char16_t,260> actual){
        static_assert(sizeof(wchar_t)==sizeof(char16_t));
        const auto nul=std::find(actual.begin(),actual.end(),u'\0');
        require(actual[0]!=u'\0'&&nul!=actual.end()&&static_cast<std::size_t>(nul-actual.begin())<256,
            "Root watcher fixed-path >=256/noNUL branch requires exact helper successor");
        require(actual[255]!=u'/'&&actual[255]!=u'\\',
            "Root watcher slot255 separator/native wcsncpy_s failure branch unsupported");
        std::array<wchar_t,260> result{};
        // Actual6B8B60 copies all520 bytes. Preserve every actual fixed-buffer
        // codeunit, including tail DATA; do not rederive it from a basename.
        for(std::size_t i=0;i<260;++i)result[i]=static_cast<wchar_t>(actual[i]);
        return result;
    }
    explicit Impl(std::array<char16_t,260> actual):path(copied_path(std::move(actual))){
        // Actual530320→4FE020 scans first256 slots and returns prefix count at
        // last '/' OR '\\'; count excludes separator. No separator =>empty.
        std::size_t prefix=0;
        for(std::size_t i=0;i<256;++i)if(path[i]==L'/'||path[i]==L'\\')prefix=i;
        for(std::size_t i=0;i<prefix;++i)directory[i]=path[i];
        directory[prefix]=L'\0';
    }
    void first()const {
        require(creator==std::this_thread::get_id()&&alive.load(),
            "Root watcher creator/alive FIRST failed");
    }
    static VOID CALLBACK changed_callback(PVOID raw,BOOLEAN timed_out)noexcept{
        auto*const self=static_cast<Impl*>(raw);
        if(timed_out||!self->alive.load())return; // actual750FC3 timer branch
        WIN32_FILE_ATTRIBUTE_DATA now{};
        if(GetFileAttributesExW(self->path.data(),GetFileExInfoStandard,&now)){
            std::lock_guard<std::mutex> lock(self->worker_mutex);
            // Actual CompareFileTime(stored,current)==-1, not any inequality.
            if(self->alive.load()&&CompareFileTime(&self->last_write,&now.ftLastWriteTime)==-1){
                self->dirty.store(true);self->last_write=now.ftLastWriteTime;
            }
        }
        if(self->alive.load())FindNextChangeNotification(self->change);
    }
    static bool retire_kernel_owned(Impl* self)noexcept{
        self->alive.store(false); // FIRST before any native retirement effect
        // Normalized x64 cancellation waits for callbacks before freeing their
        // exact payload. Original456C70 uses UnregisterWait then closes change;
        // Ex(INVALID_HANDLE_VALUE) preserves that order and drains the callback.
        // Failure retains all storage/handles in quarantine, with no success.
        if(self->wait!=INVALID_HANDLE_VALUE){
            if(!UnregisterWaitEx(self->wait,INVALID_HANDLE_VALUE))return false;
            self->wait=INVALID_HANDLE_VALUE;
        }
        if(self->change!=INVALID_HANDLE_VALUE){
            if(!FindCloseChangeNotification(self->change))return false;
            self->change=INVALID_HANDLE_VALUE;
        }
        return true;
    }
    static void release_owned(Impl* self)noexcept{
        if(retire_kernel_owned(self)){delete self;return;}
        self->quarantine_next=quarantine_head;quarantine_head=self;
    }
};
detail::NotificationContextRootFileWatcher::NotificationContextRootFileWatcher(std::shared_ptr<Impl> data)
    :impl_(std::move(data)){}
std::shared_ptr<detail::NotificationContextRootFileWatcher>
detail::NotificationContextRootFileWatcher::allocate_first_owned(std::array<char16_t,260> actual){
    auto data=std::shared_ptr<Impl>(new Impl(std::move(actual)),&Impl::release_owned);
    return std::shared_ptr<NotificationContextRootFileWatcher>(new NotificationContextRootFileWatcher(std::move(data)));
}
void detail::NotificationContextRootFileWatcher::capture_file_attributes_owned()const{
    const auto saved=impl_;saved->first();require(saved->stage==Impl::Stage::copied_path,"Root watcher attrs replay");
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if(GetFileAttributesExW(saved->path.data(),GetFileExInfoStandard,&attributes)){
        std::lock_guard<std::mutex> lock(saved->worker_mutex);saved->last_write=attributes.ftLastWriteTime;
    }
    saved->first();saved->stage=Impl::Stage::attributes_captured;
}
void detail::NotificationContextRootFileWatcher::open_change_notification_owned()const{
    const auto saved=impl_;saved->first();require(saved->stage==Impl::Stage::attributes_captured,"Root watcher open order");
    const auto handle=FindFirstChangeNotificationW(saved->directory.data(),FALSE,0x10);
    saved->first();saved->change=handle;saved->stage=Impl::Stage::change_opened;
    // OriginalRoot ignores71C790 return and still registers an invalid handle.
    // Native MISS/failure is retained as actual DATA, not an invented success.
}
void detail::NotificationContextRootFileWatcher::register_wait_owned()const{
    const auto saved=impl_;saved->first();require(saved->stage==Impl::Stage::change_opened,"Root watcher wait order");
    RegisterWaitForSingleObject(&saved->wait,saved->change,&Impl::changed_callback,saved.get(),INFINITE,0);
    saved->first();saved->stage=Impl::Stage::wait_attempted;
}
void detail::NotificationContextRootFileWatcher::require_reached_without_callbacks()const{
    const auto saved=impl_;saved->first();require(saved->stage==Impl::Stage::wait_attempted,
        "Root watcher mandatory attribute/open/register effects not returned");
}
bool detail::NotificationContextRootFileWatcher::changed_without_callbacks()const{
    require_reached_without_callbacks();return impl_->dirty.load();
}
detail::NotificationContextRootFileWatcher::~NotificationContextRootFileWatcher(){impl_->alive.store(false);}
#else
struct detail::NotificationContextRootFileWatcher::Impl final {};
detail::NotificationContextRootFileWatcher::NotificationContextRootFileWatcher(std::shared_ptr<Impl> data):impl_(std::move(data)){}
std::shared_ptr<detail::NotificationContextRootFileWatcher>
detail::NotificationContextRootFileWatcher::allocate_first_owned(std::array<char16_t,260>){
    throw std::runtime_error("required real Win32 Root file watcher owner unsupported on this platform");
}
void detail::NotificationContextRootFileWatcher::capture_file_attributes_owned()const{throw std::runtime_error("required Win32 watcher");}
void detail::NotificationContextRootFileWatcher::open_change_notification_owned()const{throw std::runtime_error("required Win32 watcher");}
void detail::NotificationContextRootFileWatcher::register_wait_owned()const{throw std::runtime_error("required Win32 watcher");}
void detail::NotificationContextRootFileWatcher::require_reached_without_callbacks()const{throw std::runtime_error("required Win32 watcher");}
bool detail::NotificationContextRootFileWatcher::changed_without_callbacks()const{throw std::runtime_error("required Win32 watcher");}
detail::NotificationContextRootFileWatcher::~NotificationContextRootFileWatcher()=default;
#endif
