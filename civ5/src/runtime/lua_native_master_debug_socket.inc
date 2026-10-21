// Private original5C2790 ->7EF9F0/7EF4F0 socket prefix. Constructed before
// lua_newstate, in the actual Host constructor. No bank/count, external debug
// registration, default scope, catalog or whole master completion is issued.
class NativeMasterDebugSocketOwner;
struct NativeMasterDebugSocketCtorFrame final {
    Impl& host;const ScriptSourceProvider*const source;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    explicit NativeMasterDebugSocketCtorFrame(Impl&p):host(p),source(&p.sources),creator(std::this_thread::get_id()),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(p.native_master_debug_socket_ctor_frame||p.state||p.calling||p.native_master_debug_socket||
           creator!=p.ui_thread||p.failed.exhausted_)
            throw ScriptError("Native Lua debug socket requires actual first Host constructor before VM creation");
        p.native_master_debug_socket_ctor_frame=this;
    }
    ~NativeMasterDebugSocketCtorFrame(){if(host.native_master_debug_socket_ctor_frame==this)host.native_master_debug_socket_ctor_frame=nullptr;}
    NativeMasterDebugSocketCtorFrame(const NativeMasterDebugSocketCtorFrame&)=delete;
    NativeMasterDebugSocketCtorFrame& operator=(const NativeMasterDebugSocketCtorFrame&)=delete;
    void current()const {
        // Source FIRST for this original pre-VM constructor is its actual
        // retained Host/source fields and lexical constructor frame. A later
        // WorldView selected child/518 loan is not fabricated at startup.
        if(host.native_master_debug_socket_ctor_frame!=this||source!=&host.sources||host.state||host.calling||
           creator!=std::this_thread::get_id()||creator!=host.ui_thread||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("Native Lua debug socket constructor Source/Host/frame/failure changed");
    }
};
class NativeMasterDebugSocketOwner final {
    friend struct Impl;
    Impl& host;const ScriptSourceProvider*const source;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const NativeMasterDebugSocketCtorFrame* constructor;
    HMODULE library=nullptr;
    decltype(&WSAStartup) startup=nullptr;
    decltype(&WSACleanup) cleanup=nullptr;
    decltype(&::socket) open_socket=nullptr;
    decltype(&::setsockopt) set_option=nullptr;
    decltype(&::closesocket) close_socket=nullptr;
    WSADATA actual_startup_data{};
    SOCKET actual_socket=INVALID_SOCKET;
    enum class Phase {created,library_acquired,entries_acquired,wsa_started,socket_created,keepalive_applied,receive_buffer_applied,retired};
    Phase phase=Phase::created;
    std::uint32_t native_socket_state=0; // original receiver+8 BEFORE real socket return
    std::uint8_t native_mode_bits=5; // only original7EF4F0's proven low mode bits
    int startup_result=WSASYSNOTREADY,keepalive_result=SOCKET_ERROR,receive_buffer_result=SOCKET_ERROR;
    std::uint64_t os_calls=0;
    NativeMasterDebugSocketOwner(Impl&p,const NativeMasterDebugSocketCtorFrame&f):host(p),source(&p.sources),creator(f.creator),
        epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty),constructor(&f){}
    NativeMasterDebugSocketOwner(const NativeMasterDebugSocketOwner&)=delete;
    NativeMasterDebugSocketOwner& operator=(const NativeMasterDebugSocketOwner&)=delete;
    void current_constructor()const {
        // Borrowed constructor pointer is read only after actual owning slot
        // and lexical frame equality. No callback-visible caller bool exists.
        if(host.native_master_debug_socket.get()!=this||!constructor||host.native_master_debug_socket_ctor_frame!=constructor||
           phase==Phase::retired||source!=&host.sources||creator!=std::this_thread::get_id())
            throw ScriptError("Native Lua debug socket real constructor owner/frame changed");
        constructor->current();
    }
    void before_os(){current_constructor();if(os_calls==UINT64_MAX)throw ScriptError("Native Lua debug socket call history exhausted");++os_calls;}
    void construct_socket_prefix(){
        before_os();library=LoadLibraryExW(L"ws2_32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);current_constructor();
        if(!library)throw ScriptError("Native Lua debug socket actual Windows system library unavailable");phase=Phase::library_acquired;
        before_os();startup=reinterpret_cast<decltype(startup)>(GetProcAddress(library,"WSAStartup"));current_constructor();
        before_os();cleanup=reinterpret_cast<decltype(cleanup)>(GetProcAddress(library,"WSACleanup"));current_constructor();
        before_os();open_socket=reinterpret_cast<decltype(open_socket)>(GetProcAddress(library,"socket"));current_constructor();
        before_os();set_option=reinterpret_cast<decltype(set_option)>(GetProcAddress(library,"setsockopt"));current_constructor();
        before_os();close_socket=reinterpret_cast<decltype(close_socket)>(GetProcAddress(library,"closesocket"));current_constructor();
        if(!startup||!cleanup||!open_socket||!set_option||!close_socket)
            throw ScriptError("Native Lua debug socket required genuine Windows entrypoint unavailable");phase=Phase::entries_acquired;
        // Genuine independent WSA reference, following the existing Windows
        // client pattern. Another client's storage is never a startup grant.
        before_os();startup_result=startup(MAKEWORD(2,2),&actual_startup_data);
        if(startup_result==0)phase=Phase::wsa_started;current_constructor();
        if(startup_result!=0)throw ScriptError("Native Lua debug socket real WSAStartup failed");
        before_os();actual_socket=open_socket(2,1,0);
        if(actual_socket!=INVALID_SOCKET){phase=Phase::socket_created;native_socket_state=1;}
        current_constructor();if(actual_socket==INVALID_SOCKET)throw ScriptError("Native Lua debug socket original socket(2,1,0) failed");
    }
    void construct_options_suffix(){
        current_constructor();if(phase!=Phase::socket_created||actual_socket==INVALID_SOCKET||native_socket_state!=1)
            throw ScriptError("Native Lua socket options require genuine returned socket prefix");
        const int keepalive=1;
        before_os();keepalive_result=set_option(actual_socket,0xffff,8,reinterpret_cast<const char*>(&keepalive),4);
        if(keepalive_result==0){native_mode_bits=static_cast<std::uint8_t>((native_mode_bits&~2u)|2u);phase=Phase::keepalive_applied;}
        current_constructor();if(keepalive_result!=0)throw ScriptError("Native Lua debug socket original SO_KEEPALIVE=1 failed");
        const int receive_buffer=0x7fff;
        before_os();receive_buffer_result=set_option(actual_socket,0xffff,0x1002,reinterpret_cast<const char*>(&receive_buffer),4);
        if(receive_buffer_result==0)phase=Phase::receive_buffer_applied;current_constructor();
        if(receive_buffer_result!=0)throw ScriptError("Native Lua debug socket original SO_RCVBUF=32767 failed");
        if(native_socket_state!=1||(native_mode_bits&2u)==0||startup_result!=0||keepalive_result!=0||receive_buffer_result!=0)
            throw ScriptError("Native Lua debug socket actual completed constructor effects differ");
    }
    void construct(){construct_socket_prefix();construct_options_suffix();}
    void require_current_without_callbacks()const {
        if(host.native_master_debug_socket.get()!=this||constructor||host.native_master_debug_socket_ctor_frame||
           phase!=Phase::receive_buffer_applied||source!=&host.sources||creator!=std::this_thread::get_id()||creator!=host.ui_thread||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_||
           !library||!startup||!cleanup||!open_socket||!set_option||!close_socket||actual_socket==INVALID_SOCKET||
           native_socket_state!=1||startup_result!=0||keepalive_result!=0||receive_buffer_result!=0||(native_mode_bits&2u)==0)
            throw ScriptError("Native Lua debug socket has no genuine current completed constructor");
    }
    void retire()noexcept{
        if(phase==Phase::retired)return;
        // Teardown owns the actual handles even after a failed Source POST.
        // No permission is renewed by cleanup, and failures poison the Host.
        bool failed_release=false;
        if(actual_socket!=INVALID_SOCKET){if(!close_socket||close_socket(actual_socket)!=0)failed_release=true;actual_socket=INVALID_SOCKET;}
        if(startup_result==0){if(!cleanup||cleanup()!=0)failed_release=true;startup_result=WSASYSNOTREADY;}
        if(library){if(!FreeLibrary(library))failed_release=true;library=nullptr;}
        native_socket_state=0;constructor=nullptr;startup=nullptr;cleanup=nullptr;open_socket=nullptr;set_option=nullptr;close_socket=nullptr;phase=Phase::retired;
        if(failed_release)host.record_failure();
    }
public:~NativeMasterDebugSocketOwner(){retire();}
};
void initialize_native_master_debug_socket_before_vm_owned(){
    {
        NativeMasterDebugSocketCtorFrame frame(*this);frame.current();
        native_master_debug_socket.reset(new NativeMasterDebugSocketOwner(*this,frame));frame.current();
        try{native_master_debug_socket->construct();frame.current();native_master_debug_socket->constructor=nullptr;}
        catch(...){native_master_debug_socket->retire();throw;}
        // Retire the lexical constructor loan before accepting its actual return.
    }
    native_master_debug_socket->require_current_without_callbacks();
    // The real socket remains in Host custody for later actual bank initialization.
}
void native_master_debug_socket_retire()noexcept{
    native_master_debug_socket_ctor_frame=nullptr;
    if(native_master_debug_socket)native_master_debug_socket->retire();
}
