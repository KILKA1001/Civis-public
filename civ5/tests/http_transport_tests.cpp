#include "civ5/platform/http_download.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs=std::filesystem;
void check(bool okay,const char *message){if(!okay)throw std::runtime_error(message);}
template<class F> void rejects(F action){
    bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}
    check(rejected,"Expected HTTPS transport rejection");
}
void mode(const wchar_t *value){check(SetEnvironmentVariableW(L"CIVIS_FAKE_CURL_MODE",value)!=0,"Set fake curl mode");}
std::string contents(const fs::path &path){
    std::ifstream file(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
void run(const fs::path &helper){
    check(helper.is_absolute(),"HTTPS helper must be absolute");
    check(SetEnvironmentVariableW(L"CIVIS_HTTPS_CURL",helper.c_str())!=0,"Set HTTPS helper");
    SetEnvironmentVariableW(L"CIVIS_HTTPS_CA_BUNDLE",nullptr);
    const auto root=fs::current_path()/("http-transport-test-"+std::to_string(GetCurrentProcessId()));
    fs::create_directories(root);
    mode(L"success");
    std::uint64_t progress=0;
    const auto success=civ5::platform::download_https({"https://example.com/start",{}},root/"success",16,
        [&](std::uint64_t value){progress=value;return true;});
    check(success.bytes==5&&progress==5&&success.final_url=="https://example.com/final"&&
        contents(root/"success")=="CIVIS","Verified HTTPS helper output or final URL changed");
    mode(L"uppercase");
    check(civ5::platform::download_https({"HTTPS://example.com/start",{}},root/"uppercase",16).bytes==5,
        "HTTPS scheme case handling changed");
    mode(L"large");
    rejects([&]{civ5::platform::download_https({"https://example.com/large",{}},root/"large",3);});
    mode(L"success");
    rejects([&]{civ5::platform::download_https({"https://example.com/cancel",{}},root/"cancel",16,
        [](std::uint64_t){return false;});});
    mode(L"status");
    rejects([&]{civ5::platform::download_https({"https://example.com/missing",{}},root/"status",16);});
    mode(L"redirect");
    rejects([&]{civ5::platform::download_https({"https://example.com/redirect",{}},root/"redirect",16);});
    mode(L"error");
    rejects([&]{civ5::platform::download_https({"https://example.com/error",{}},root/"error",16);});
    mode(L"post");
    const auto post=civ5::platform::download_https({"https://example.com/form","a=1&b=2"},root/"post",16);
    check(post.bytes==5&&contents(root/"post")=="CIVIS","Form POST was not streamed to HTTPS helper");
    for(const auto &entry:fs::directory_iterator(root))
        check(entry.path().filename().wstring().find(L".form-")==std::wstring::npos&&
            entry.path().filename().wstring().find(L".status-")==std::wstring::npos,
            "HTTPS helper left a form or status file");
    rejects([&]{civ5::platform::download_https({"http://example.com/",{}},root/"http",16);});
    rejects([&]{civ5::platform::download_https({"https://user:pass@example.com/",{}},root/"credentials",16);});
    check(!fs::exists(root/"http")&&!fs::exists(root/"credentials"),
        "Rejected URL created a destination file");
    check(SetEnvironmentVariableW(L"CIVIS_HTTPS_CURL",(root/"missing.exe").c_str())!=0,
        "Set invalid HTTPS helper");
    rejects([&]{civ5::platform::download_https({"https://example.com/",{}},root/"invalid_helper",16);});
    check(SetEnvironmentVariableW(L"CIVIS_HTTPS_CURL",helper.c_str())!=0,
        "Restore HTTPS helper");
    check(SetEnvironmentVariableW(L"CIVIS_HTTPS_CA_BUNDLE",(root/"missing-ca.pem").c_str())!=0,
        "Set invalid CA bundle");
    rejects([&]{civ5::platform::download_https({"https://example.com/",{}},root/"invalid_ca",16);});
    SetEnvironmentVariableW(L"CIVIS_HTTPS_CA_BUNDLE",nullptr);
    check(!fs::exists(root/"invalid_helper")&&!fs::exists(root/"invalid_ca"),
        "Invalid transport configuration created a destination file");
    fs::remove_all(root);
}
}
int main(int argc,char **argv){
    try{
        if(argc!=2)throw std::runtime_error("Expected fake 64-bit HTTPS helper path");
        run(fs::absolute(argv[1]));
        std::cout<<"PASS HTTPS helper boundaries; runtime_bits="<<sizeof(void*)*8<<'\n';return 0;
    }catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
