#include "../src/ui/bnw_native_loose_walk.hpp"
#include "../src/ui/bnw_native_resource_stamp.hpp"
#include <chrono>
#include <cstring>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>
#include <optional>
#include <cerrno>

namespace loose = civ5::ui::native_loose_detail;
namespace {
void check(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
namespace stamps = civ5::ui::native_resource_stamp_detail;
struct StampBackend {
    std::string fail;unsigned fail_pass=0,opens=0,closes=0;
    DWORD failure=ERROR_ACCESS_DENIED;
    DWORD attributes=FILE_ATTRIBUTE_NORMAL,tag_value=IO_REPARSE_TAG_MOUNT_POINT;
    LONGLONG size=3,time=116444736000000007LL;
    std::vector<std::string> calls;std::vector<std::filesystem::path> paths;
    std::function<void()> after_first_close,during_standard;
    bool broken(const char* phase)const{return fail==phase&&opens==fail_pass;}
    HANDLE open(const std::filesystem::path& path){++opens;paths.push_back(path);calls.push_back("open");return broken("open")?INVALID_HANDLE_VALUE:reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(opens));}
    bool basic(HANDLE,FILE_BASIC_INFO& out){calls.push_back("basic");out.FileAttributes=attributes;out.LastWriteTime.QuadPart=time;return !broken("basic");}
    bool tag(HANDLE,FILE_ATTRIBUTE_TAG_INFO& out){calls.push_back("tag");out.ReparseTag=tag_value;return !broken("tag");}
    bool standard(HANDLE,FILE_STANDARD_INFO& out){calls.push_back("standard");if(during_standard)during_standard();out.EndOfFile.QuadPart=size;return !broken("standard");}
    bool identity(HANDLE,BY_HANDLE_FILE_INFORMATION&){calls.push_back("identity");return !broken("identity");}
    DWORD error()const{return failure;}
    void close(HANDLE){++closes;failure=ERROR_INVALID_HANDLE;if(closes==1&&after_first_close)after_first_close();}
};
void stamp_scripted_cases() {
    const std::filesystem::path path=L"Synthetic/Original.xml";
    StampBackend normal;const auto first=stamps::two_target_stamp(path,normal);
    check(normal.calls==std::vector<std::string>{"open","basic","standard","identity","open","basic","standard","identity"}&&normal.closes==2,
          "Stamp must retain both complete stat/open/close passes in order");
    check(normal.paths==std::vector<std::filesystem::path>{path,path}&&first.path==path&&first.size==3&&first.written.time_since_epoch().count()==700,
          "Both opens and returned witness must retain exact original path, first size and second time");
    for(const unsigned pass:{1u,2u})for(const auto* phase:{"open","basic","tag","standard","identity"}) {
        StampBackend broken;broken.fail=phase;broken.fail_pass=pass;
        if(broken.fail=="tag")broken.attributes|=FILE_ATTRIBUTE_REPARSE_POINT;
        if(pass==2)broken.after_first_close=[&]{broken.failure=ERROR_ACCESS_DENIED;};
        bool refused=false;
        try{(void)stamps::two_target_stamp(path,broken);}
        catch(const std::filesystem::filesystem_error& error){
            const char* operation=pass==1?"in file_size":"in last_write_time";
            refused=error.path1()==path&&error.path2().empty()&&
                error.code()==std::error_code(ERROR_ACCESS_DENIED,std::system_category())&&
                std::string(error.what())==std::filesystem::filesystem_error(operation,path,error.code()).what();
        }
        check(refused&&broken.opens==pass&&broken.closes==(broken.fail=="open"?pass-1:pass),
              "Each first/second query failure must refuse with its own operation label and exact close count");
        check(broken.calls.back()==phase,"Backend failure must stop before later queries or fallback");
    }
    for(const bool symbolic:{false,true}) {
        StampBackend nonregular;nonregular.attributes=FILE_ATTRIBUTE_DIRECTORY;
        if(symbolic){nonregular.attributes|=FILE_ATTRIBUTE_REPARSE_POINT;nonregular.tag_value=IO_REPARSE_TAG_SYMLINK;}
        bool refused=false;
        try{(void)stamps::two_target_stamp(path,nonregular);}
        catch(const std::filesystem::filesystem_error& error){refused=error.code()==std::make_error_code(
            symbolic?std::errc::not_supported:std::errc::is_a_directory)&&error.path1()==path;}
        check(refused&&nonregular.calls.back()=="identity"&&nonregular.opens==1&&nonregular.closes==1,
              "First nonregular refusal must follow its full closed stat and precede second open");
    }
    StampBackend mounted;mounted.attributes|=FILE_ATTRIBUTE_REPARSE_POINT;
    check(stamps::two_target_stamp(path,mounted)==first&&mounted.calls[2]=="tag"&&mounted.calls[7]=="tag"&&mounted.closes==2,
          "Both stat passes must retain their mandatory non-symlink reparse query");
    StampBackend changed;changed.after_first_close=[&]{changed.size=9;changed.time+=10;};
    const auto mixed=stamps::two_target_stamp(path,changed);
    check(mixed.size==3&&mixed.written==stamps::filetime_clock(116444736000000017ULL),
          "Two-pass witness must preserve first size and differently observed second target time");
    changed.after_first_close={};const auto after=stamps::two_target_stamp(path,changed);
    check(after.size==9&&after.written==mixed.written&&after!=mixed,"Next witness must freshly observe both passes, without cache");
    changed.size=normal.size;changed.time=normal.time;
    check(stamps::two_target_stamp(path,changed)==first,"Restoration must remain observable");
    for(const bool symbolic:{false,true}) {
        StampBackend rebound;rebound.after_first_close=[&]{
            rebound.attributes=FILE_ATTRIBUTE_DIRECTORY;rebound.time+=20;
            if(symbolic){rebound.attributes|=FILE_ATTRIBUTE_REPARSE_POINT;rebound.tag_value=IO_REPARSE_TAG_SYMLINK;}
        };
        const auto value=stamps::two_target_stamp(path,rebound);
        check(value.size==3&&value.written==stamps::filetime_clock(116444736000000027ULL)&&rebound.closes==2,
              "Second last_write_time pass must not incorrectly require a regular rebound target");
    }
    StampBackend unlinked;unlinked.after_first_close=[&]{unlinked.fail="open";unlinked.fail_pass=2;unlinked.failure=ERROR_FILE_NOT_FOUND;};
    bool refused=false;
    try{(void)stamps::two_target_stamp(path,unlinked);}catch(const std::filesystem::filesystem_error& error){
        refused=error.code()==std::error_code(ERROR_FILE_NOT_FOUND,std::system_category())&&
            std::string(error.what())==std::filesystem::filesystem_error("in last_write_time",path,error.code()).what();
    }
    check(refused&&unlinked.opens==2&&unlinked.closes==1,"Unlink between passes must reject in the SAME stamp; no later bookend required");
    check(stamps::filetime_clock(116444736000000000ULL).time_since_epoch().count()==0&&
          stamps::filetime_clock(116444735999999999ULL).time_since_epoch().count()==-100&&
          stamps::filetime_clock(0).time_since_epoch().count()==
              -static_cast<std::filesystem::file_time_type::duration::rep>(116444736000000000ULL)*100,
          "FILETIME epoch/subsecond/1601 conversion must retain int128 mapping");
}
namespace stamp_diag = civ5::ui::native_stamp_diagnostic;
struct StampClockProbe {
    unsigned calls=0,fail_at=0;
    bool tick(std::int64_t& value)noexcept {
        value=static_cast<std::int64_t>(++calls)*10;
        SetLastError(0xdead);errno=EDOM;return calls!=fail_at;
    }
};
struct StampErrorProbe {
    unsigned gets=0,sets=0;
    DWORD get()noexcept{++gets;return GetLastError();}
    void set(DWORD value)noexcept{++sets;SetLastError(value);}
};
struct StampStatusBackend : StampBackend {
    bool close_ok=true;
    std::vector<DWORD> entry_errors;std::vector<int> entry_errno;
    void entry(){entry_errors.push_back(GetLastError());entry_errno.push_back(errno);}
    bool observed(bool ok,unsigned code)noexcept {
        SetLastError(ok?code:ERROR_ACCESS_DENIED);errno=ok?ERANGE:EACCES;return ok;
    }
    HANDLE open(const std::filesystem::path& path) {
        entry();const auto result=StampBackend::open(path);
        observed(result!=INVALID_HANDLE_VALUE,101);return result;
    }
    bool basic(HANDLE file,FILE_BASIC_INFO& out){entry();return observed(StampBackend::basic(file,out),102);}
    bool tag(HANDLE file,FILE_ATTRIBUTE_TAG_INFO& out){entry();return observed(StampBackend::tag(file,out),103);}
    bool standard(HANDLE file,FILE_STANDARD_INFO& out){entry();return observed(StampBackend::standard(file,out),104);}
    bool identity(HANDLE file,BY_HANDLE_FILE_INFORMATION& out){entry();return observed(StampBackend::identity(file,out),105);}
    DWORD error()const{return GetLastError();}
    bool close(HANDLE file)noexcept {
        // Fixed fixture vectors are pre-reserved before this noexcept close.
        entry();StampBackend::close(file);return observed(close_ok,106);
    }
    void reserve(){entry_errors.reserve(16);entry_errno.reserve(16);calls.reserve(16);paths.reserve(2);}
};
struct StampProfileResult {
    std::optional<stamps::Stamp> value;std::optional<std::filesystem::filesystem_error> error;
    DWORD error_state=0;int errno_state=0;
};
StampProfileResult exercise_stamp_profile(StampStatusBackend& backend,stamp_diag::Collector* collector,
        StampClockProbe& clock,StampErrorProbe& errors) {
    backend.reserve();SetLastError(77);errno=E2BIG;StampProfileResult result;
    try{result.value=stamps::observe_stamp(L"Synthetic/Profile.xml",backend,collector,clock,errors);}
    catch(const std::filesystem::filesystem_error& error){result.error=error;}
    result.error_state=GetLastError();result.errno_state=errno;return result;
}
void stamp_diagnostic_cases() {
    // SAME dispatcher/adapter as production. Timer corruption must not change
    // original API entry/exit error state, error label, call order or closes.
    for(const unsigned pass:{1u,2u})for(const auto* phase:{"", "open","basic","tag","standard","identity"})
        for(const unsigned clock_failure:{0u,1u,2u}) {
            StampStatusBackend baseline,profiled;baseline.fail=profiled.fail=phase;
            baseline.fail_pass=profiled.fail_pass=pass;
            if(std::string(phase)=="tag")baseline.attributes=profiled.attributes=FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_REPARSE_POINT;
            StampClockProbe quiet,clock;clock.fail_at=clock_failure;StampErrorProbe quiet_errors,errors;
            const auto expected=exercise_stamp_profile(baseline,nullptr,quiet,quiet_errors);
            stamp_diag::Collector collector;collector.frequency=1000;
            const auto actual=exercise_stamp_profile(profiled,&collector,clock,errors);
            check(quiet.calls==0&&quiet_errors.gets==0&&quiet_errors.sets==0,
                  "Disabled profiling performed clock/error-state work");
            check(actual.value==expected.value&&bool(actual.error)==bool(expected.error)&&
                actual.error_state==expected.error_state&&actual.errno_state==expected.errno_state&&
                profiled.calls==baseline.calls&&profiled.paths==baseline.paths&&profiled.closes==baseline.closes&&
                profiled.entry_errors==baseline.entry_errors&&profiled.entry_errno==baseline.entry_errno,
                "Profiling/timer failure changed original return/entry/exit error state, order/path or close lifetime");
            if(actual.error)check(actual.error->code()==expected.error->code()&&actual.error->path1()==expected.error->path1()&&
                std::string(actual.error->what())==expected.error->what(),"Timer corruption replaced original first/second failure");
            std::uint64_t calls=0,samples=0,start_failures=0,end_failures=0,native_failures=0;
            for(const auto& cell:collector.cells){calls+=cell.calls;samples+=cell.samples;start_failures+=cell.start_failures;end_failures+=cell.end_failures;native_failures+=cell.failure;}
            check(calls==profiled.calls.size()+profiled.closes&&native_failures==(std::string(phase).empty()?0u:1u),
                "Native success/failure observations changed calls or confused timer failure with native failure");
            check(clock_failure==0?(samples==calls&&start_failures==0&&end_failures==0):
                (samples+1==calls&&start_failures+end_failures==1),"Failed timer became a valid duration sample");
            check(collector.cell(stamp_diag::Pass::size,stamp_diag::Step::open).calls==1&&
                collector.cell(stamp_diag::Pass::time,stamp_diag::Step::open).calls==(profiled.opens==2?1u:0u),
                "Independent first and second opens lost their diagnostic pass identity");
        }
    // CloseHandle failure is ignored by the existing stamp contract, but its
    // returned bool and actual post-call error state must be truthfully observed.
    StampStatusBackend close_base,close_profile;close_base.close_ok=close_profile.close_ok=false;
    StampClockProbe quiet,clock;StampErrorProbe quiet_errors,errors;stamp_diag::Collector collector;collector.frequency=1000;
    const auto expected=exercise_stamp_profile(close_base,nullptr,quiet,quiet_errors);
    const auto actual=exercise_stamp_profile(close_profile,&collector,clock,errors);
    check(actual.value==expected.value&&!actual.error&&actual.error_state==expected.error_state&&
        actual.errno_state==expected.errno_state&&close_profile.closes==2&&
        collector.cell(stamp_diag::Pass::size,stamp_diag::Step::close).failure==1&&
        collector.cell(stamp_diag::Pass::time,stamp_diag::Step::close).failure==1,
        "Close failure observation changed stamp behavior or manufactured close success");
    // Unavailable frequency counts native outcomes without querying the timer.
    StampStatusBackend unavailable;StampClockProbe unavailable_clock;StampErrorProbe unavailable_errors;
    stamp_diag::Collector unavailable_collector;
    const auto value=exercise_stamp_profile(unavailable,&unavailable_collector,unavailable_clock,unavailable_errors);
    check(value.value&&!value.error&&unavailable_clock.calls==0&&
        unavailable_collector.cell(stamp_diag::Pass::size,stamp_diag::Step::open).unavailable==1&&
        unavailable_collector.cell(stamp_diag::Pass::size,stamp_diag::Step::open).samples==0,
        "Unavailable timer fabricated samples or blocked original stamp");
}

struct BoundaryNativeBackend : stamps::NativeBackend {
    unsigned closes=0;std::function<void()> after_first_close;
    void close(HANDLE file){stamps::NativeBackend::close(file);if(++closes==1&&after_first_close)after_first_close();}
};
void stamp_native_cases() {
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-opened-stamp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(root),"Stamp temp root must be fresh and owned");
    std::filesystem::create_directory(root/"Target");const auto path=root/"Target/Value.xml";
    const auto put=[&](std::string_view bytes){std::ofstream out(path,std::ios::binary);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));check(bool(out),"Stamp fixture write failed");};
    put("abc");const auto before=stamps::stamp(path);
    check(before==stamps::legacy_stamp(path),"Actual regular stamp must equal unchanged std filesystem values");
    const auto relative_spelling=root/"Target/../Target//Value.xml";
    check(stamps::stamp(relative_spelling)==stamps::legacy_stamp(relative_spelling),
          "Dot segments and forward separators must retain the original std/Win32 path relation");
    const auto unicode_path=root/"Target"/L"\u0414\u0438\u043c\u0430.xml";
    {std::ofstream out(unicode_path,std::ios::binary);out<<"unicode";check(bool(out),"Unicode fixture write failed");}
    check(stamps::stamp(unicode_path)==stamps::legacy_stamp(unicode_path),"Native Unicode original path must equal unchanged std parsing");
    // A real private junction requires no invented resource handle and verifies
    // CreateFile follows the original target rather than query the link itself.
    const auto link=root/"Link";std::filesystem::create_directory(link);
    const auto target=std::filesystem::absolute(root/"Target").wstring();const auto substitute=L"\\??\\"+target;
    struct MountHeader{DWORD tag;WORD length,reserved,substitute_offset,substitute_length,print_offset,print_length;};
    static_assert(sizeof(MountHeader)==16);
    const auto paths_bytes=(substitute.size()+target.size()+2)*sizeof(wchar_t);
    std::vector<std::uint8_t> buffer(sizeof(MountHeader)+paths_bytes,0);
    MountHeader header{IO_REPARSE_TAG_MOUNT_POINT,static_cast<WORD>(8+paths_bytes),0,0,
        static_cast<WORD>(substitute.size()*sizeof(wchar_t)),static_cast<WORD>((substitute.size()+1)*sizeof(wchar_t)),
        static_cast<WORD>(target.size()*sizeof(wchar_t))};
    std::memcpy(buffer.data(),&header,sizeof(header));
    std::memcpy(buffer.data()+sizeof(header),substitute.data(),header.substitute_length);
    std::memcpy(buffer.data()+sizeof(header)+header.print_offset,target.data(),header.print_length);
    HANDLE junction=CreateFileW(link.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    check(junction!=INVALID_HANDLE_VALUE,"Stamp owned junction open failed");DWORD returned=0;
    const bool installed=DeviceIoControl(junction,FSCTL_SET_REPARSE_POINT,buffer.data(),static_cast<DWORD>(buffer.size()),nullptr,0,&returned,nullptr)!=0;
    CloseHandle(junction);check(installed,"Stamp real junction setup failed; reparse test cannot be fabricated/skipped");
    const auto alias=link/"Value.xml";const auto via_link=stamps::stamp(alias);
    check(via_link==stamps::legacy_stamp(alias)&&via_link.size==before.size&&via_link.written==before.written,
          "Actual junction target stamp must equal legacy followed-target size/time");
    const auto equivalent_error=[&](const std::filesystem::path& item){
        std::optional<std::filesystem::filesystem_error> old_error,new_error;
        try{(void)stamps::legacy_stamp(item);}catch(const std::filesystem::filesystem_error& error){old_error=error;}
        try{(void)stamps::stamp(item);}catch(const std::filesystem::filesystem_error& error){new_error=error;}
        check(old_error&&new_error&&old_error->code()==new_error->code()&&old_error->path1()==new_error->path1()&&
              std::string(old_error->what())==new_error->what(),"Actual missing/directory target refusal mapping must equal legacy filesystem_error");
    };
    equivalent_error(root/"Missing.xml");equivalent_error(link);
    put("abcdef");const auto larger=stamps::stamp(path);
    check(larger.size==6&&larger!=before&&larger==stamps::legacy_stamp(path),"Actual size mutation must be observed on each stamp");
    HANDLE file=CreateFileW(path.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    check(file!=INVALID_HANDLE_VALUE,"Stamp mtime fixture open failed");
    struct WriteCloser{HANDLE file;~WriteCloser(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);}} write_closer{file};
    bool time_ok=true;
    for(const std::uint64_t ticks:{116444735999999999ULL,116444736000000000ULL,116444736000000001ULL}) {
        FILETIME time{static_cast<DWORD>(ticks),static_cast<DWORD>(ticks>>32)};
        if(!SetFileTime(file,nullptr,nullptr,&time)){time_ok=false;break;}
        const auto changed=stamps::stamp(path);
        check(changed==stamps::legacy_stamp(path)&&changed.written==stamps::filetime_clock(ticks),"Actual epoch/subsecond mtime mapping must equal legacy clock values");
    }
    CloseHandle(file);write_closer.file=INVALID_HANDLE_VALUE;check(time_ok,"Stamp actual mtime fixture failed");
    put("abc");std::filesystem::last_write_time(path,before.written);
    check(stamps::stamp(path)==before,"Actual restored metadata must not remain cached as changed");
    // Verify sharing compatibility after return. FILE_READ_ATTRIBUTES sharing
    // is special on Windows; exact handle closure is proved by scripted cases.
    HANDLE exclusive=CreateFileW(path.c_str(),DELETE,0,nullptr,OPEN_EXISTING,0,nullptr);
    check(exclusive!=INVALID_HANDLE_VALUE,"Independent sharing-compatible open after stamp failed");CloseHandle(exclusive);
    check(RemoveDirectoryW(link.c_str())!=0,"Owned junction cleanup must not recurse through target");
    std::filesystem::remove_all(root);
}
void stamp_native_between_passes() {
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-two-pass-stamp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(root),"Two-pass root must be fresh and owned");
    const auto path=root/"Input.xml",replacement=root/"Replacement.xml";
    const auto put=[](const std::filesystem::path& item,const char* bytes){
        std::ofstream out(item,std::ios::binary);out<<bytes;check(bool(out),"Two-pass fixture write failed");
    };
    const auto missing_error=[&](auto&& action){
        std::optional<std::filesystem::filesystem_error> error;
        try{action();}catch(const std::filesystem::filesystem_error& value){error=value;}
        return error;
    };
    put(path,"abc");
    const auto reference_first_size=std::filesystem::file_size(path);
    check(DeleteFileW(path.c_str())!=0,"Reference unlink failed");
    const auto old_missing=missing_error([&]{(void)std::filesystem::last_write_time(path);});
    check(reference_first_size==3&&old_missing,"Old second observation must reject the removed pathname");
    put(path,"abc");bool unlinked=false;
    BoundaryNativeBackend remove_backend;remove_backend.after_first_close=[&]{unlinked=DeleteFileW(path.c_str())!=0;};
    const auto new_missing=missing_error([&]{(void)stamps::two_target_stamp(path,remove_backend);});
    check(unlinked&&new_missing&&new_missing->code()==old_missing->code()&&
          new_missing->path1()==old_missing->path1()&&std::string(new_missing->what())==old_missing->what()&&remove_backend.closes==1,
          "Actual unlink between stat passes must reject immediately with the old second-operation error");
    put(path,"abc");put(replacement,"replacement");
    std::filesystem::last_write_time(replacement,std::filesystem::last_write_time(path)+std::chrono::hours(1));
    const auto first_size=std::filesystem::file_size(path);
    const auto second_time=std::filesystem::last_write_time(replacement);
    bool rebound=false;BoundaryNativeBackend replace_backend;
    replace_backend.after_first_close=[&]{rebound=MoveFileExW(replacement.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)!=0;};
    const auto mixed=stamps::two_target_stamp(path,replace_backend);
    check(rebound&&replace_backend.closes==2&&mixed.path==path&&mixed.size==first_size&&mixed.size==3&&mixed.written==second_time&&
          std::filesystem::file_size(path)==11,
          "Actual file rebind must preserve first file size and different second file mtime, not pin the first handle");
    check(DeleteFileW(path.c_str())!=0,"Rebound fixture removal failed");put(path,"abc");
    const auto old_size=std::filesystem::file_size(path);
    check(DeleteFileW(path.c_str())&&CreateDirectoryW(path.c_str(),nullptr),"Reference directory rebind failed");
    const auto old_directory_time=std::filesystem::last_write_time(path);
    check(old_size==3&&old_directory_time==std::filesystem::last_write_time(path),
          "Old second last_write_time supports a directory rebound");
    check(RemoveDirectoryW(path.c_str())!=0,"Reference directory cleanup failed");put(path,"abc");
    bool directory_rebound=false;BoundaryNativeBackend directory_backend;
    directory_backend.after_first_close=[&]{directory_rebound=DeleteFileW(path.c_str())&&CreateDirectoryW(path.c_str(),nullptr);};
    const auto directory_mixed=stamps::two_target_stamp(path,directory_backend);
    check(directory_rebound&&directory_backend.closes==2&&directory_mixed.size==3&&
          directory_mixed.written==std::filesystem::last_write_time(path),
          "Actual second-pass directory must retain last_write_time behavior without added regular-file refusal");
    std::filesystem::remove_all(root);
}
#endif
template<class F> void rejects(F&& action, const char* reason) {
    bool rejected = false;
    try { action(); }
    catch (const std::runtime_error& error) { rejected = std::string(error.what()) == reason; }
    check(rejected, "Expected exact traversal/currentness refusal");
}
WIN32_FIND_DATAA row(const std::string& name, DWORD attrs = FILE_ATTRIBUTE_NORMAL,
                     std::uint64_t bytes = 3, std::uint64_t written = 7) {
    check(name.size() < MAX_PATH, "Fixture name too long");
    WIN32_FIND_DATAA out{};
    std::copy(name.begin(), name.end(), out.cFileName);
    out.dwFileAttributes = attrs;
    out.nFileSizeLow = static_cast<DWORD>(bytes);
    out.nFileSizeHigh = static_cast<DWORD>(bytes >> 32);
    out.ftLastWriteTime.dwLowDateTime = static_cast<DWORD>(written);
    out.ftLastWriteTime.dwHighDateTime = static_cast<DWORD>(written >> 32);
    return out;
}
// Script only the OS records/errors. The comparison, filters, refusal order,
// recursion and capture conversion are the exact production private helper.
struct ScriptEnumeration {
    struct Directory { std::vector<WIN32_FIND_DATAA> rows; DWORD final_error = ERROR_NO_MORE_FILES; };
    std::map<std::string, Directory> directories;
    std::vector<std::string> opened;
    std::size_t records = 0;
    struct Cursor {
        WIN32_FIND_DATAA data{};
        const Directory* directory = nullptr;
        ScriptEnumeration* owner = nullptr;
        std::size_t index = 0;
        bool valid() const { return directory && !directory->rows.empty(); }
        bool next() {
            if (++index == directory->rows.size()) return false;
            data = directory->rows[index]; ++owner->records; return true;
        }
        DWORD error() const { return directory ? directory->final_error : ERROR_PATH_NOT_FOUND; }
    };
    Cursor open(const std::string& search) {
        opened.push_back(search);
        auto found = directories.find(search);
        Cursor cursor{{}, found == directories.end() ? nullptr : &found->second, this, 0};
        if (cursor.valid()) { cursor.data = cursor.directory->rows.front(); ++records; }
        return cursor;
    }
};
ScriptEnumeration script() {
    ScriptEnumeration out;
    out.directories["root\\*"] = {{row("."), row(".."), row("Alpha.xml"),
        row("Nested", FILE_ATTRIBUTE_DIRECTORY), row("dLc", FILE_ATTRIBUTE_DIRECTORY),
        row("Scenarios", FILE_ATTRIBUTE_DIRECTORY), row("Sounds", FILE_ATTRIBUTE_DIRECTORY),
        row("hidden", FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_REPARSE_POINT),
        row("system", FILE_ATTRIBUTE_SYSTEM)}};
    out.directories["root\\Nested\\*"] = {{row("Leaf.xml", FILE_ATTRIBUTE_READONLY,
        (std::uint64_t(2) << 32) | 3, (std::uint64_t(4) << 32) | 5)}};
    out.directories["root\\Sounds\\*"] = {{row("Speech", FILE_ATTRIBUTE_DIRECTORY), row("Music.xml")}};
    // Filtered directories intentionally have no script: opening one fails.
    return out;
}
loose::LooseWalk captured(ScriptEnumeration source) {
    return {"root", true, true, loose::capture_loose_entries("root", true, source), GetACP()};
}
void scripted_cases() {
    const auto baseline = captured(script());
    check(baseline.entries.size() == 8, "Native directory-before-child/filter membership changed");
    for (const auto& entry : baseline.entries)
        check(entry.relative.native() == loose::acp_wide(entry.raw_relative), "Capture lost exact ACP path relation");
    auto unchanged = script();
    check(loose::compare_loose_entries(baseline, "root", unchanged), "Unchanged stream rejected");
    check(unchanged.opened == std::vector<std::string>{"root\\*", "root\\Nested\\*", "root\\Sounds\\*"},
          "Filter recursion or native preorder changed");
    for (int mutation = 0; mutation < 7; ++mutation) {
        auto changed = script();
        auto& child = changed.directories.at("root\\Nested\\*").rows.front();
        if (mutation == 0) child = row("Renamed.xml", child.dwFileAttributes,
            (std::uint64_t(2) << 32) | 3, (std::uint64_t(4) << 32) | 5);
        if (mutation == 1) changed.directories.at("root\\Nested\\*").rows.push_back(row("Added.xml"));
        if (mutation == 2) child = row("."); // Remove the only ordinary child, keep valid native enumeration.
        if (mutation == 3) child.dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
        if (mutation == 4) ++child.nFileSizeHigh;
        if (mutation == 5) ++child.ftLastWriteTime.dwHighDateTime;
        if (mutation == 6) std::swap(changed.directories.at("root\\*").rows[2],
                                    changed.directories.at("root\\*").rows[3]);
        loose::LooseWalkDifference difference;
        check(!loose::compare_loose_entries(baseline, "root", changed, &difference), "Mutation/order accepted");
        check(difference.kind != loose::LooseWalkDifference::Kind::none &&
                  difference.expected && difference.observed,
              "Rejected stream lost its first actual expected/observed record");
        if (mutation == 3 || mutation == 4 || mutation == 5) {
            const unsigned field = mutation == 3 ? 4u : mutation == 4 ? 8u : 16u;
            check(difference.index == 2 && difference.fields == field,
                  "Native mismatch diagnosis misidentified attribute/size/time refusal");
        }
        check(difference.observed_count == baseline.entries.size() + (mutation == 1 ? 1 : 0) - (mutation == 2 ? 1 : 0) && changed.opened.size() == 3,
              "Diagnosis stopped traversal or counted skipped dot records");
    }
    // Same basename and metadata in another parent must fail full raw-relative binding.
    auto forged_parent = baseline;
    forged_parent.entries[2].raw_relative = "Other\\Leaf.xml";
    auto same_observed = script();
    check(!loose::compare_loose_entries(forged_parent, "root", same_observed), "Only basename compared");
    auto ascii_failure = script();
    ascii_failure.directories.at("root\\*").rows[2].nFileSizeLow++; // Earlier mismatch.
    ascii_failure.directories.at("root\\Nested\\*").rows.front() = row(std::string(1, char(0x80)) + ".xml");
    rejects([&] { loose::compare_loose_entries(baseline, "root", ascii_failure); },
            "Native loose non-ASCII record bytes require CRT/CRC source owner");
    auto reparse_failure = script();
    reparse_failure.directories.at("root\\*").rows[2].nFileSizeLow++;
    reparse_failure.directories.at("root\\Nested\\*").rows.front().dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
    rejects([&] { loose::compare_loose_entries(baseline, "root", reparse_failure); },
            "Native loose reparse source requires target owner");
    auto incomplete = script();
    incomplete.directories.at("root\\*").rows[2].nFileSizeLow++;
    incomplete.directories.at("root\\Sounds\\*").final_error = ERROR_ACCESS_DENIED;
    rejects([&] { loose::compare_loose_entries(baseline, "root", incomplete); }, "Native loose traversal incomplete");
    auto missing_dir = script(); missing_dir.directories.erase("root\\Nested\\*");
    rejects([&] { loose::compare_loose_entries(baseline, "root", missing_dir); }, "Native loose directory enumeration failed");
    auto unfiltered = baseline; unfiltered.base_filters = false; auto missing_filtered = script();
    rejects([&] { loose::compare_loose_entries(unfiltered, "root", missing_filtered); }, "Native loose directory enumeration failed");
    auto excluded_ascii = script();
    excluded_ascii.directories.at("root\\*").rows.push_back(row(std::string(1, char(0x80)), FILE_ATTRIBUTE_HIDDEN));
    check(loose::compare_loose_entries(baseline, "root", excluded_ascii), "Hidden/system skip moved after ASCII refusal");
}
// Regression coverage for borrowed cursor names and literal parent/name
// comparison. Existing scripted/native cases continue to exercise every field,
// full traversal after mismatch, late refusals, root state and actual FindA.
void segmented_comparison_cases() {
    check(loose::raw_relative_equals_segments("Leaf.xml","","Leaf.xml"),"Root filename segment changed");
    check(loose::raw_relative_equals_segments("A\\Leaf.xml","A","Leaf.xml"),"Nested filename segment changed");
    check(!loose::raw_relative_equals_segments("AB\\Leaf.xml","A","Leaf.xml"),"Parent prefix boundary weakened");
    check(!loose::raw_relative_equals_segments("A/Leaf.xml","A","Leaf.xml"),"Literal separator was normalized");
    check(!loose::raw_relative_equals_segments("A\\Leaf.xml.extra","A","Leaf.xml"),"Filename suffix was ignored");
    check(!loose::raw_relative_equals_segments("A","A",""),"Short raw prefix was indexed or accepted");
    check(!loose::raw_relative_equals_segments("a\\Leaf.xml","A","Leaf.xml"),"Raw case comparison changed");
    check(loose::raw_relative_equals_segments("A\\\\Leaf.xml","A\\","Leaf.xml"),"Literal doubled separator changed");
    const auto baseline=captured(script());
    auto changed=script();
    const std::string changed_name="Alpha-with-long-owning-diagnostic-name.xml";
    changed.directories.at("root\\*").rows[2]=row(changed_name);
    loose::LooseWalkDifference difference;
    check(!loose::compare_loose_entries(baseline,"root",changed,&difference),"Changed root filename accepted");
    check(difference.kind==loose::LooseWalkDifference::Kind::record&&difference.index==0&&
          difference.fields==3&&difference.observed_count==baseline.entries.size(),"First-difference fields/count changed");
    changed.directories.clear(); // All cursor backing records have now retired.
    check(difference.expected&&difference.expected->raw_relative=="Alpha.xml"&&difference.observed&&
          difference.observed->raw_relative==changed_name&&difference.observed->native_name==changed_name&&
          difference.observed->bytes==3&&difference.observed->written==7,"Diagnostic retained a borrowed cursor view");
    auto star_script=script();
    std::map<std::string,ScriptEnumeration::Directory> star_directories;
    for(auto& [search,directory]:star_script.directories)star_directories.emplace(search+".*",std::move(directory));
    star_script.directories=std::move(star_directories);
    auto star_expected=baseline;star_expected.original_star_dot_star=true;
    check(loose::compare_loose_entries(star_expected,"root",star_script),"Original star-dot-star search changed");
    check(star_script.opened==std::vector<std::string>{"root\\*.*","root\\Nested\\*.*","root\\Sounds\\*.*"},
          "Comparator changed actual per-folder search suffix");
    struct RecordingEnumeration:ScriptEnumeration {
        std::vector<std::string> events;
        Cursor open(const std::string& search){events.push_back("open:"+search);return ScriptEnumeration::open(search);}
    } recorded;
    recorded.directories=script().directories;
    std::vector<std::string> names;
    loose::visit_loose_entries_with_folder("root",true,recorded,
        [&](const std::string& path,const std::string& name,DWORD,std::uint64_t,std::uint64_t){
            recorded.events.push_back("emit:"+path);names.push_back(name);
        },[&](const std::string& folder){recorded.events.push_back("enter:"+folder);});
    check(recorded.events==std::vector<std::string>{"enter:","open:root\\*","emit:Alpha.xml","emit:Nested",
          "enter:Nested","open:root\\Nested\\*","emit:Nested\\Leaf.xml","emit:dLc","emit:Scenarios",
          "emit:Sounds","enter:Sounds","open:root\\Sounds\\*","emit:Sounds\\Speech","emit:Sounds\\Music.xml"},
          "Folder registration, owning capture emission or recursion order changed");
    check(names==std::vector<std::string>{"Alpha.xml","Nested","Leaf.xml","dLc","Scenarios","Sounds","Speech","Music.xml"},
          "Owning capture/admission callback names changed");
}
struct TemporaryDirectory {
    std::filesystem::path root;
    TemporaryDirectory() {
        wchar_t temp[MAX_PATH + 1]{}, name[MAX_PATH + 1]{};
        const auto count = GetTempPathW(MAX_PATH, temp);
        check(count > 0 && count < MAX_PATH, "Temporary path unavailable");
        check(GetTempFileNameW(temp, L"lwj", 0, name) != 0, "Unique fixture name unavailable");
        root = name;
        check(DeleteFileW(name) && CreateDirectoryW(name, nullptr), "Unique fixture directory unavailable");
    }
    ~TemporaryDirectory() { std::error_code error; std::filesystem::remove_all(root, error); }
};
void write(const std::filesystem::path& path, const char* bytes) {
    std::ofstream file(path, std::ios::binary); file << bytes;
    check(static_cast<bool>(file), "Fixture write failed");
    file.flush();
    check(static_cast<bool>(file), "Fixture flush failed");
    file.close();
    check(!file.fail(), "Fixture close failed");
}
// Install fixture-owned directory times only after every setup writer has
// closed. This is setup, not a retry/relaxation of the native comparison.
// A cache which still exposes a different time fails setup explicitly.
constexpr std::uint64_t fixture_directory_time = 132537600000000000ull;
void set_directory_time(const std::filesystem::path& path, std::uint64_t written) {
    const HANDLE directory = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    check(directory != INVALID_HANDLE_VALUE, "Directory timestamp fixture open failed");
    const FILETIME requested{static_cast<DWORD>(written), static_cast<DWORD>(written >> 32)};
    const bool set = SetFileTime(directory, nullptr, nullptr, &requested) != 0;
    FILETIME observed{};
    const bool read = GetFileTime(directory, nullptr, nullptr, &observed) != 0;
    const bool closed = CloseHandle(directory) != 0;
    check(set && read && closed, "Directory timestamp fixture finalize failed");
    check(observed.dwLowDateTime == requested.dwLowDateTime &&
          observed.dwHighDateTime == requested.dwHighDateTime,
          "Directory timestamp fixture did not retain requested value");
}
loose::LooseWalk fixture_baseline(const std::filesystem::path& root, bool filters = false) {
    std::vector<std::filesystem::path> directories{root};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
        if (entry.is_directory()) directories.push_back(entry.path());
    for (auto entry = directories.rbegin(); entry != directories.rend(); ++entry)
        set_directory_time(*entry, fixture_directory_time);
    auto baseline = loose::loose_walk(root, filters);
    for (const auto& entry : baseline.entries)
        if ((entry.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            check(entry.written == fixture_directory_time,
                  "Fixture directory time not settled in native enumeration");
    return baseline;
}
void native_cases() {
    TemporaryDirectory fixture;
    const auto root = fixture.root;
    std::filesystem::create_directories(root / "Nested");
    write(root / "Nested/Leaf.xml", "first");
    write(root / "Alpha.xml", "alpha");
    const auto baseline = fixture_baseline(root);
    loose::LooseWalkDifference difference;
    const bool unchanged = loose::loose_walk_matches(baseline, &difference);
    if (!unchanged) {
        // Print evidence from that very comparison, never another filesystem walk.
        std::cerr << "FIRST_NATIVE_WALK_MISMATCH kind=" << static_cast<int>(difference.kind)
                  << " index=" << difference.index << " fields=" << difference.fields
                  << " expected_count=" << baseline.entries.size()
                  << " observed_count=" << difference.observed_count
                  << " expected_root_exists=" << baseline.exists
                  << " observed_root_exists=" << difference.observed_root_exists
                  << " code_page=" << baseline.code_page << std::endl;
        if (difference.expected) {
            const auto& e = *difference.expected;
            std::cerr << "EXPECTED raw=" << e.raw_relative << " name=" << e.native_name
                      << " attrs=" << e.attributes << " bytes=" << e.bytes << " written=" << e.written << std::endl;
        }
        if (difference.observed) {
            const auto& e = *difference.observed;
            std::cerr << "OBSERVED raw=" << e.raw_relative << " name=" << e.native_name
                      << " attrs=" << e.attributes << " bytes=" << e.bytes << " written=" << e.written << std::endl;
        }
    }
    check(unchanged, "Unchanged actual native walk rejected");
    set_directory_time(root / "Nested", fixture_directory_time + 10000000ull);
    loose::LooseWalkDifference directory_time_difference;
    check(!loose::loose_walk_matches(baseline, &directory_time_difference),
          "Actual directory timestamp-only change accepted");
    check(directory_time_difference.kind == loose::LooseWalkDifference::Kind::record &&
          directory_time_difference.fields == 16u && directory_time_difference.expected &&
          directory_time_difference.observed &&
          directory_time_difference.expected->raw_relative == "Nested" &&
          directory_time_difference.observed->raw_relative == "Nested",
          "Actual directory timestamp mutation lost exact first difference");
    const auto before_rename = fixture_baseline(root);
    std::filesystem::rename(root / "Nested/Leaf.xml", root / "Nested/Renamed.xml");
    check(!loose::loose_walk_matches(before_rename), "Actual nested rename accepted");
    std::filesystem::rename(root / "Nested/Renamed.xml", root / "Nested/Leaf.xml");
    const auto after_rename = fixture_baseline(root);
    write(root / "Nested/Added.xml", "added");
    check(!loose::loose_walk_matches(after_rename), "Actual nested add accepted");
    std::filesystem::remove(root / "Nested/Added.xml");
    const auto after_add = fixture_baseline(root);
    std::filesystem::remove(root / "Nested/Leaf.xml");
    check(!loose::loose_walk_matches(after_add), "Actual nested remove accepted");
    write(root / "Nested/Leaf.xml", "first");
    const auto metadata = fixture_baseline(root);
    write(root / "Nested/Leaf.xml", "different-size");
    check(!loose::loose_walk_matches(metadata), "Actual payload-size metadata mutation accepted");
    const auto attrs = fixture_baseline(root);
    check(SetFileAttributesW((root / "Alpha.xml").c_str(), FILE_ATTRIBUTE_READONLY) != 0, "Attribute fixture failed");
    check(!loose::loose_walk_matches(attrs), "Actual attribute change accepted");
    check(SetFileAttributesW((root / "Alpha.xml").c_str(), FILE_ATTRIBUTE_NORMAL) != 0, "Attribute restore failed");
    const auto timestamp = fixture_baseline(root);
    const auto leaf = root / "Nested/Leaf.xml";
    const HANDLE file = CreateFileW(leaf.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(file != INVALID_HANDLE_VALUE, "Timestamp fixture open failed");
    FILETIME changed_time{};
    GetSystemTimeAsFileTime(&changed_time);
    changed_time.dwHighDateTime += 1;
    const bool time_set = SetFileTime(file, nullptr, nullptr, &changed_time) != 0;
    CloseHandle(file);
    check(time_set && !loose::loose_walk_matches(timestamp), "Actual timestamp-only change accepted");
    auto foreign_acp = fixture_baseline(root); foreign_acp.code_page ^= 1u;
    rejects([&] { loose::loose_walk_matches(foreign_acp); }, "Native loose CP_ACP changed");
    const auto absent = loose::loose_walk(root / "Absent", false);
    check(loose::loose_walk_matches(absent), "Still-absent root rejected");
    std::filesystem::create_directory(root / "Absent");
    check(!loose::loose_walk_matches(absent), "Appeared empty root accepted");
    const auto existing = fixture_baseline(root / "Absent");
    std::filesystem::remove(root / "Absent");
    check(!loose::loose_walk_matches(existing), "Disappeared root accepted");
    write(root / "NotDirectory", "file");
    rejects([&] { loose::loose_walk(root / "NotDirectory", false); }, "Native loose source is not a directory");
    std::filesystem::create_directories(root / "dLc/Excluded");
    write(root / "dLc/Excluded/Filtered.xml", "before");
    const auto filtered = fixture_baseline(root, true);
    const auto unfiltered = fixture_baseline(root);
    write(root / "dLc/Excluded/Filtered.xml", "after-with-new-size");
    check(loose::loose_walk_matches(filtered) && !loose::loose_walk_matches(unfiltered),
          "Actual base-filter boundary changed or unfiltered scan omitted child");
    // Non-ASCII root keeps the original full ACP round-trip contract. Accept or
    // reject based on actual CP_ACP conversion, never by direct ASCII widening.
    const auto unicode_root = root / L"\u0414\u0438\u043c\u0430";
    std::filesystem::create_directory(unicode_root); write(unicode_root / "Ascii.xml", "source");
    bool round_trip = false;
    try { round_trip = loose::acp_wide(loose::acp_path(unicode_root)) == unicode_root.native(); }
    catch (const std::runtime_error&) {}
    if (round_trip) check(loose::loose_walk_matches(fixture_baseline(unicode_root)), "Representable non-ASCII root rejected");
    else rejects([&] { loose::loose_walk(unicode_root, false); }, "Native loose source root cannot round-trip current CP_ACP");
}
}
int main() {
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
    stamp_scripted_cases();
    stamp_diagnostic_cases();
    stamp_native_cases();
    stamp_native_between_passes();
#endif
    scripted_cases();
    segmented_comparison_cases();
    native_cases();
    std::cout << "PASS: actual private loose streaming comparison, complete traversal/refusals and mutations" << std::endl;
}
