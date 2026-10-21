#include "extended_label_cursor_io.hpp"
#include "whole_raw_source_pins.hpp"
using namespace cursor_test;
static_assert(sizeof(void*)==4);
class OriginalConverter {
    void *code_=nullptr,*tables_=nullptr;
public:
    OriginalConverter(const std::filesystem::path &path){
        for(auto address:{0x830000u,0xc90000u}){
            MEMORY_BASIC_INFORMATION info{};
            const auto queried=VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info));
            std::cerr<<"PRECHECK address="<<std::hex<<address<<" state="<<info.State<<" base="<<reinterpret_cast<std::uintptr_t>(info.BaseAddress)
                     <<" region="<<info.RegionSize<<" type="<<info.Type<<" ownStackAddress="<<reinterpret_cast<std::uintptr_t>(&info)<<std::dec<<"\n";
            need(queried==sizeof(info)&&info.State==MEM_FREE&&std::uint64_t(reinterpret_cast<std::uintptr_t>(info.BaseAddress))+info.RegionSize>=std::uint64_t(address)+65536u,
                 "FixedVA collision refused BEFORE original converter execution");
        }
        code_=VirtualAlloc(reinterpret_cast<void*>(0x830000u),65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        tables_=VirtualAlloc(reinterpret_cast<void*>(0xc90000u),65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(code_!=reinterpret_cast<void*>(0x830000u)||tables_!=reinterpret_cast<void*>(0xc90000u)){
            if(code_)VirtualFree(code_,0,MEM_RELEASE);if(tables_)VirtualFree(tables_,0,MEM_RELEASE);
            code_=tables_=nullptr;throw std::runtime_error("Own fixedVA allocation refused");
        }
        try {
        const auto exe=read(path);need(sha(exe)==cursor_source::exe_sha,"Original whole EXE changed");
        need(std::memcmp(exe.data()+4434816,whole_raw_source::converter,332)==0&&
             std::memcmp(exe.data()+9033360,whole_raw_source::converter_tables,280)==0,"Original converter/table bytes changed");
        std::memcpy(static_cast<unsigned char*>(code_)+0xb780,whole_raw_source::converter,332);
        std::memcpy(static_cast<unsigned char*>(tables_)+0xf490,whole_raw_source::converter_tables,280);
        DWORD old=0;
        need(VirtualProtect(code_,65536,PAGE_EXECUTE_READ,&old)&&VirtualProtect(tables_,65536,PAGE_READONLY,&old)&&
             FlushInstructionCache(GetCurrentProcess(),code_,65536),"Own original converter protection");
        } catch(...) {
            VirtualFree(code_,0,MEM_RELEASE);VirtualFree(tables_,0,MEM_RELEASE);
            code_=tables_=nullptr;throw;
        }
    }
    ~OriginalConverter(){if(code_)VirtualFree(code_,0,MEM_RELEASE);if(tables_)VirtualFree(tables_,0,MEM_RELEASE);}
    struct Result{unsigned status=0,consumed=0;std::vector<std::uint16_t> utf16;};
    Result run(const Bytes &text,unsigned selected,unsigned capacity)const{
        need(selected<=text.size()&&capacity<=4096,"Bounded diagnostic conversion pointers");
        // Original410900 receives owned NULterminated bytes. Guarded padding
        // also owns the one excluded byte permitted by converter's JA test.
        Bytes storage_input=text;storage_input.resize(text.size()+8,0);
        const char *begin=reinterpret_cast<const char*>(storage_input.data()),*cursor=begin;
        std::array<std::uint16_t,4097> storage{};storage.fill(0xbeef);auto *target=storage.data();
        using Fn=unsigned(__cdecl *)(const char**,const char*,std::uint16_t**,std::uint16_t*);
        Result out;out.status=reinterpret_cast<Fn>(0x83b780u)(&cursor,begin+selected,&target,storage.data()+capacity);
        need(cursor>=begin&&cursor<=begin+selected+1&&target>=storage.data()&&target<=storage.data()+capacity&&storage[capacity]==0xbeef,"Original converter exceeded own buffers");
        out.consumed=static_cast<unsigned>(cursor-begin);out.utf16.assign(storage.data(),target);return out;
    }
};
Bytes encode(std::uint32_t cp){
    Bytes b;
    if(cp<0x80)b.push_back(static_cast<unsigned char>(cp));
    else if(cp<0x800){b={static_cast<unsigned char>(0xc0u|(cp>>6)),static_cast<unsigned char>(0x80u|(cp&63))};}
    else if(cp<0x10000){b={static_cast<unsigned char>(0xe0u|(cp>>12)),static_cast<unsigned char>(0x80u|((cp>>6)&63)),static_cast<unsigned char>(0x80u|(cp&63))};}
    else{b={static_cast<unsigned char>(0xf0u|(cp>>18)),static_cast<unsigned char>(0x80u|((cp>>12)&63)),static_cast<unsigned char>(0x80u|((cp>>6)&63)),static_cast<unsigned char>(0x80u|(cp&63))};}
    return b;
}
int wmain(int argc,wchar_t **argv)try{
    need(argc==4,"Args originalEXE NEWreference NEWreport");
    need(!std::filesystem::exists(argv[2])&&!std::filesystem::exists(argv[3]),"Preserve prior source evidence");
    OriginalConverter converter(argv[1]);Bytes reference{'R','A','W','U','T','F','0','1'};
    for(char ch:std::string(cursor_source::exe_sha))reference.push_back(static_cast<unsigned char>(ch));reference.resize(76);
    unsigned rows=0,admitted=0,refused=0,diagnostic=0;
    const auto record=[&](Bytes text,unsigned tag,unsigned capacity){
        need(text.size()<=8192&&tag<=2,"Diagnostic reference bound");
        const auto first_zero=std::find(text.begin(),text.end(),0);
        const auto selected=std::min(4096u,static_cast<unsigned>(first_zero-text.begin()));
        const auto result=converter.run(text,selected,capacity);
        if(tag==1){need(result.status==0&&result.consumed==text.size(),"Original rejected declared strict fixture");++admitted;}
        else if(tag==0)++refused;else ++diagnostic;
        put(reference,text.size());put(reference,selected,2);put(reference,capacity,2);put(reference,tag,1);put(reference,result.status,1);
        put(reference,result.consumed,2);put(reference,result.utf16.size(),2);reference.insert(reference.end(),text.begin(),text.end());
        for(auto unit:result.utf16)put(reference,unit,2);++rows;
    };
    record({},1,0);
    for(std::uint32_t cp=1;cp<=0xffff;++cp)if(cp<0xd800||cp>0xdfff){auto b=encode(cp);record(b,1,static_cast<unsigned>(b.size()));}
    for(std::uint32_t cp=0x10000;cp<=0x10ffff;cp+=0x401){auto b=encode(cp);record(b,1,static_cast<unsigned>(b.size()));}
    for(auto cp:{0x10ffffu,0x10000u,0x1f600u,0x10437u}){auto b=encode(cp);record(b,1,static_cast<unsigned>(b.size()));}
    for(const std::string text:{"[ICON_GOLD][COLOR:255:235:0:255]0 (+3)[/COLOR]","[NEWLINE]A\tB\r\n[[","\xd0\x94\xd0\xb5\xd0\xbb\xd0\xb8"})record(Bytes(text.begin(),text.end()),1,static_cast<unsigned>(text.size()));
    record(Bytes(4096,'A'),1,4096);
    for(Bytes malformed:{Bytes{0x80},Bytes{0xc0,0x80},Bytes{0xe0,0x80,0x80},Bytes{0xed,0xa0,0x80},Bytes{0xf4,0x90,0x80,0x80},Bytes{0xf8,0x88,0x80,0x80,0x80},Bytes{0xfc,0x84,0x80,0x80,0x80,0x80},Bytes{0xc2},Bytes{0xe2},Bytes{0xf0,0x9f},Bytes{0xe2,0x82},Bytes{0xc2,'A'},Bytes{'A',0,'B'}})
        record(malformed,0,static_cast<unsigned>(malformed.size()));
    record(Bytes(4097,'A'),0,4096);
    Bytes cutoff(4095,'A');cutoff.push_back(0xd0);cutoff.push_back(0x94);record(cutoff,0,4096);
    for(auto cp:{0x41u,0x4ffu,0x1f600u})for(unsigned capacity=0;capacity<=2;++capacity)record(encode(cp),2,capacity);
    for(unsigned n=0;n<4;++n)reference[72+n]=static_cast<unsigned char>(rows>>(8*n));
    need(sha(read(argv[1]))==cursor_source::exe_sha,"Original EXE changed during execution");write(argv[2],reference);
    const std::string report="{\"schema\":\"civis-original-raw-utf-preparation-v1\",\"rows\":"+std::to_string(rows)+",\"strictValidOriginalObservations\":"+std::to_string(admitted)+",\"outsideStrictScopeOriginalObservations\":"+std::to_string(refused)+",\"targetExhaustionDiagnosticObservations\":"+std::to_string(diagnostic)+",\"classBytesObserved\":false,\"gcpExecuted\":false,\"nativeLabelReady\":false,\"pass\":true}\n";
    write(argv[3],Bytes(report.begin(),report.end()));std::cout<<"PASS original raw converter rows="<<rows<<" strict="<<admitted<<" outside="<<refused<<" capacity="<<diagnostic<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
