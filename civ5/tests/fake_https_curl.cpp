#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <string>

int main(int argc,char **argv){
    char mode_buffer[64]{};
    GetEnvironmentVariableA("CIVIS_FAKE_CURL_MODE",mode_buffer,sizeof(mode_buffer));
    const std::string mode=mode_buffer;
    bool disable=false,https=false,redirect_https=false;
    std::string body_file;
    for(int i=1;i<argc;++i){
        const std::string value=argv[i];
        if(value=="--disable")disable=true;
        if(value=="--proto"&&i+1<argc)https=std::string(argv[++i])=="=https";
        if(value=="--proto-redir"&&i+1<argc)redirect_https=std::string(argv[++i])=="=https";
        if(value=="--data-binary"&&i+1<argc)body_file=argv[++i];
    }
    if(!disable||!https||!redirect_https)return 9;
    if(mode=="post"){
        if(body_file.empty()||body_file[0]!='@')return 8;
        std::ifstream stream(body_file.substr(1),std::ios::binary);
        const std::string data((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
        if(data!="a=1&b=2")return 7;
    }
    const std::string body=mode=="large"?std::string(8192,'x'):"CIVIS";
    DWORD written=0;
    if(!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),body.data(),static_cast<DWORD>(body.size()),&written,nullptr)
       ||written!=body.size())return 6;
    const std::string marker=mode=="status"?
        "CIVIS_HTTP_RESULT:404:https://example.com/missing\n":
        mode=="redirect"?"CIVIS_HTTP_RESULT:200:http://example.com/final\n":
        mode=="uppercase"?"CIVIS_HTTP_RESULT:200:HTTPS://example.com/final\n":
        "CIVIS_HTTP_RESULT:200:https://example.com/final\n";
    if(!WriteFile(GetStdHandle(STD_ERROR_HANDLE),marker.data(),static_cast<DWORD>(marker.size()),&written,nullptr)
       ||written!=marker.size())return 5;
    return mode=="error"?4:0;
}
