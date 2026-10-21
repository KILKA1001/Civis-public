#include "http_curl.hpp"
#include "civ5/content/mod_archive.hpp"
#include "civ5/platform/windows_path.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <limits>
#include <stdexcept>
#include <vector>

namespace civ5::platform {
namespace {
struct Handle {
    HANDLE value=INVALID_HANDLE_VALUE;
    Handle()=default;
    explicit Handle(HANDLE handle):value(handle){}
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    HANDLE release(){const auto result=value;value=INVALID_HANDLE_VALUE;return result;}
};
void require(bool okay,const char *operation){
    if(!okay)throw std::runtime_error(std::string(operation)+" failed (Win32 "+
        std::to_string(GetLastError())+")");
}
std::optional<std::wstring> environment(const wchar_t *name){
    SetLastError(ERROR_SUCCESS);
    const auto size=GetEnvironmentVariableW(name,nullptr,0);
    if(!size){
        if(GetLastError()==ERROR_ENVVAR_NOT_FOUND)return {};
        throw std::runtime_error("Read HTTPS transport environment failed");
    }
    std::wstring value(size,L'\0');
    const auto written=GetEnvironmentVariableW(name,value.data(),size);
    require(written>0&&written<size,"Read HTTPS transport environment");
    value.resize(written);
    return value;
}
std::wstring wide(std::string_view text){
    if(text.size()>INT_MAX)throw std::runtime_error("HTTPS URL exceeds conversion budget");
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),
        static_cast<int>(text.size()),nullptr,0);
    require(count>0,"Convert HTTPS URL");
    std::wstring result(count,L'\0');
    require(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),
        static_cast<int>(text.size()),result.data(),count)==count,"Convert HTTPS URL");
    return result;
}
std::wstring quote(std::wstring_view value){
    std::wstring result=L"\"";
    std::size_t slashes=0;
    for(const auto c:value){
        if(c==L'\\'){++slashes;continue;}
        result.append(slashes*(c==L'"'?2:1),L'\\');slashes=0;
        if(c==L'"')result+=L'\\';
        result+=c;
    }
    result.append(slashes*2,L'\\');return result+L'"';
}
void argument(std::wstring &command,std::wstring_view value){command+=L' ';command+=quote(value);}
std::filesystem::path body_file(const std::filesystem::path &destination,std::string_view body){
    static std::atomic<std::uint64_t> ordinal{0};
    for(unsigned attempt=0;attempt<32;++attempt){
        const auto path=extended_windows_path(destination.wstring()+L".form-"+
            std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++ordinal));
        Handle file(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
        if(file.value==INVALID_HANDLE_VALUE){
            if(GetLastError()==ERROR_FILE_EXISTS)continue;
            require(false,"Create HTTPS form file");
        }
        try{
            DWORD written=0;
            require(WriteFile(file.value,body.data(),static_cast<DWORD>(body.size()),&written,nullptr)&&
                written==body.size(),"Write HTTPS form file");
            require(FlushFileBuffers(file.value),"Flush HTTPS form file");
        }catch(...){CloseHandle(file.release());DeleteFileW(path.c_str());throw;}
        return path;
    }
    throw std::runtime_error("Could not reserve HTTPS form file");
}
std::string read_errors(HANDLE pipe){
    std::string output;std::array<char,4096> buffer;
    for(;;){
        DWORD read=0;
        require(ReadFile(pipe,buffer.data(),static_cast<DWORD>(buffer.size()),&read,nullptr),
            "Read curl status");
        if(!read)break;
        if(output.size()>131072-read)throw std::runtime_error("Curl status exceeds byte budget");
        output.append(buffer.data(),read);
    }
    return output;
}
HttpDownload status(std::string_view error,std::uint64_t bytes){
    constexpr std::string_view marker="CIVIS_HTTP_RESULT:";
    const auto start=error.rfind(marker);
    if(start==std::string_view::npos)throw std::runtime_error("Curl did not report HTTPS status: "+
        std::string(error.substr(0,256)));
    auto value=error.substr(start+marker.size());
    if(value.size()<5||value[3]!=':'||value[0]<'0'||value[0]>'9'||
       value[1]<'0'||value[1]>'9'||value[2]<'0'||value[2]>'9')
        throw std::runtime_error("Invalid curl HTTPS status");
    const auto code=(value[0]-'0')*100+(value[1]-'0')*10+(value[2]-'0');
    value.remove_prefix(4);
    while(!value.empty()&&(value.back()=='\r'||value.back()=='\n'))value.remove_suffix(1);
    const auto secure_scheme=value.size()>=8&&
        std::equal(value.begin(),value.begin()+8,"https://",
            [](char a,char b){
                if(a>='A'&&a<='Z')a=static_cast<char>(a-'A'+'a');
                return a==b;
            });
    if(value.size()>32768||value.find_first_of("\r\n")!=std::string_view::npos||
       value.find('\0')!=std::string_view::npos||
       !secure_scheme)throw std::runtime_error("Invalid final HTTPS URL");
    if(code!=200)throw std::runtime_error("HTTP status "+std::to_string(code));
    return {bytes,std::string(value)};
}
} // namespace
std::optional<std::filesystem::path> configured_https_curl(){
    const auto value=environment(L"CIVIS_HTTPS_CURL");
    std::filesystem::path path;
    if(value)path=*value;
    else{
        std::vector<wchar_t> executable(MAX_PATH);
        for(;;){
            const auto used=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
            require(used>0,"Locate Civis executable");
            if(used<executable.size()-1){
                path=std::filesystem::path(std::wstring(executable.data(),used)).parent_path()/
                    "network"/"curl.exe";
                break;
            }
            if(executable.size()>=32768)throw std::runtime_error("Civis executable path exceeds budget");
            executable.resize(std::min<std::size_t>(executable.size()*2,32768));
        }
        if(!std::filesystem::exists(path))return {};
    }
    DWORD type=0;
    if(!path.is_absolute()||!GetBinaryTypeW(path.c_str(),&type)||type!=SCS_64BIT_BINARY)
        throw std::runtime_error("CIVIS_HTTPS_CURL must name an existing 64-bit executable");
    return path;
}
HttpDownload download_https_curl(const HttpRequest &request,
    const std::filesystem::path &destination,std::uint64_t maximum_bytes,
    DownloadProgress progress,const std::filesystem::path &executable){
    content::require_plain_directory(destination.parent_path());
    std::optional<std::filesystem::path> form;
    if(!request.form_body.empty())form=body_file(destination,request.form_body);
    const auto cleanup_form=[&]{if(form)DeleteFileW(form->c_str());};
    std::optional<std::filesystem::path> status_path;
    Handle status_file;
    const auto cleanup_status=[&]{
        if(status_file.value!=INVALID_HANDLE_VALUE)CloseHandle(status_file.release());
        if(status_path)DeleteFileW(status_path->c_str());
    };
    try{
        std::wstring command=quote(executable.wstring());
        for(const auto option:{L"--disable",L"--silent",L"--show-error",L"--location",L"--proto",L"=https",
            L"--proto-redir",L"=https",L"--max-redirs",L"10",L"--connect-timeout",L"30",
            L"--max-time",L"180",L"--output",L"-",L"--write-out",
            L"%{stderr}CIVIS_HTTP_RESULT:%{http_code}:%{url_effective}\\n"})argument(command,option);
        auto certificate=environment(L"CIVIS_HTTPS_CA_BUNDLE");
        if(!certificate){
            const auto bundled=executable.parent_path()/"ca-bundle.crt";
            if(std::filesystem::exists(bundled))certificate=bundled.wstring();
        }
        if(certificate){
            const std::filesystem::path path(*certificate);
            if(!path.is_absolute()||!std::filesystem::is_regular_file(path))
                throw std::runtime_error("CIVIS_HTTPS_CA_BUNDLE must name a certificate file");
            argument(command,L"--cacert");argument(command,path.wstring());
        }
        if(form){
            argument(command,L"--header");argument(command,L"Content-Type: application/x-www-form-urlencoded");
            argument(command,L"--data-binary");argument(command,L"@"+form->wstring());
        }
        argument(command,L"--url");argument(command,wide(request.url));
        if(command.size()>=32767)throw std::runtime_error("HTTPS helper command exceeds Windows limit");
        SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};
        HANDLE raw_output_read=nullptr,raw_output_write=nullptr;
        require(CreatePipe(&raw_output_read,&raw_output_write,&attributes,0),"Create curl output pipe");
        Handle output_read(raw_output_read),output_write(raw_output_write);
        require(SetHandleInformation(output_read.value,HANDLE_FLAG_INHERIT,0),"Protect curl read pipe");
        static std::atomic<std::uint64_t> status_ordinal{0};
        for(unsigned attempt=0;attempt<32;++attempt){
            const auto name=extended_windows_path(destination.wstring()+L".status-"+
                std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++status_ordinal));
            status_file.value=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(status_file.value!=INVALID_HANDLE_VALUE){status_path=name;break;}
            if(GetLastError()!=ERROR_FILE_EXISTS)require(false,"Create curl status file");
        }
        if(!status_path)throw std::runtime_error("Could not reserve curl status file");
        STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
        startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput=output_write.value;startup.hStdError=status_file.value;
        PROCESS_INFORMATION process{};
        require(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,
            nullptr,nullptr,&startup,&process),"Start 64-bit HTTPS helper");
        Handle process_handle(process.hProcess),thread_handle(process.hThread);
        CloseHandle(output_write.release());
        const auto output_path=extended_windows_path(destination);
        Handle file(CreateFileW(output_path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
        if(file.value==INVALID_HANDLE_VALUE){TerminateProcess(process_handle.value,1);require(false,"Create download file");}
        std::uint64_t bytes=0;
        try{
            std::array<std::uint8_t,65536> buffer;
            for(;;){
                DWORD received=0;
                if(!ReadFile(output_read.value,buffer.data(),static_cast<DWORD>(buffer.size()),
                    &received,nullptr)){
                    if(GetLastError()==ERROR_BROKEN_PIPE)break;
                    require(false,"Read HTTPS body");
                }
                if(!received)break;
                if(received>maximum_bytes-bytes)throw std::runtime_error("Download exceeds configured byte budget");
                DWORD written=0;
                require(WriteFile(file.value,buffer.data(),received,&written,nullptr)&&written==received,
                    "Write download");
                bytes+=received;
                if(progress&&!progress(bytes))throw std::runtime_error("Download cancelled");
            }
            require(FlushFileBuffers(file.value),"Flush download");
        }catch(...){TerminateProcess(process_handle.value,1);WaitForSingleObject(process_handle.value,30000);throw;}
        if(WaitForSingleObject(process_handle.value,30000)!=WAIT_OBJECT_0){
            TerminateProcess(process_handle.value,1);
            throw std::runtime_error("HTTPS helper did not exit after closing its output");
        }
        DWORD exit_code=0;require(GetExitCodeProcess(process_handle.value,&exit_code),"Read HTTPS helper exit code");
        LARGE_INTEGER beginning{};
        require(SetFilePointerEx(status_file.value,beginning,nullptr,FILE_BEGIN),"Rewind curl status file");
        const auto errors=read_errors(status_file.value);
        if(exit_code!=0)throw std::runtime_error("HTTPS helper failed (exit "+std::to_string(exit_code)+"): "+
            errors.substr(0,256));
        auto result=status(errors,bytes);
        cleanup_status();cleanup_form();return result;
    }catch(...){cleanup_status();cleanup_form();throw;}
}
} // namespace civ5::platform
