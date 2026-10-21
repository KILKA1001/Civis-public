#include "extended_label_cursor_io.hpp"

using namespace cursor_test;
static_assert(sizeof(void*)==4,"Original continuation requires independent x86 process");
class OriginalCursor {
    using Fn=void(__cdecl *)(const float*,std::uint32_t,std::uint32_t,std::uint32_t*);
    void *code_=nullptr;
public:
    explicit OriginalCursor(const std::filesystem::path &path){
        const auto exe=read(path);need(sha(exe)==cursor_source::exe_sha,"Whole original EXE changed");
        need(cursor_source::raw_offset+sizeof(cursor_source::slice)<=exe.size()&&
            std::memcmp(exe.data()+cursor_source::raw_offset,cursor_source::slice,sizeof(cursor_source::slice))==0,"Original57-byte continuation changed");
        Bytes b{0x53,0x56,0x57,0x55,0x81,0xec,0x80,0,0,0};
        const auto integer=[&](std::uint32_t x){put(b,x);};
        const auto load=[&](unsigned reg,std::uint32_t offset){b.push_back(0x8b);b.push_back(static_cast<unsigned char>(0x84+(reg<<3)));b.push_back(0x24);integer(offset);};
        load(3,0x94);load(0,0x98);b.insert(b.end(),{0x89,0x44,0x24,0x18});
        load(0,0x9c);b.insert(b.end(),{0x89,0x44,0x24,0x2c});
        b.insert(b.end(),std::begin(cursor_source::slice),std::end(cursor_source::slice));
        // Source JBE targets this exact end of the slice. No next glyph/line
        // instruction is copied; only own output stores and ABI epilogue follow.
        load(6,0xa0);
        for(auto pair:{std::pair{0x18u,0u},std::pair{0x2cu,4u},std::pair{0x30u,8u},std::pair{0x34u,12u}}){
            load(0,pair.first);b.insert(b.end(),{0x89,0x46,static_cast<unsigned char>(pair.second)});
        }
        b.insert(b.end(),{0xd9,0x7e,0x10,0xdd,0x7e,0x14}); // FNSTCW/FNSTSW own output
        b.insert(b.end(),{0x81,0xc4,0x80,0,0,0,0x5d,0x5f,0x5e,0x5b,0xc3});
        code_=VirtualAlloc(nullptr,b.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);need(code_!=nullptr,"Allocate owned continuation wrapper");
        std::memcpy(code_,b.data(),b.size());DWORD old=0;
        if(!VirtualProtect(code_,b.size(),PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),code_,b.size())){
            VirtualFree(code_,0,MEM_RELEASE);code_=nullptr;throw std::runtime_error("Protect owned continuation wrapper RX");
        }
    }
    ~OriginalCursor(){if(code_)VirtualFree(code_,0,MEM_RELEASE);}
    struct Result{std::uint32_t cursor=0,maximum=0;std::int64_t advance=0;std::uint16_t cw=0,status=0;};
    Result run(const std::array<float,9> &descriptor,std::uint32_t cursor,std::uint32_t maximum,std::uint16_t cw)const{
        alignas(16) std::array<unsigned char,512> prior{};
        __asm__ volatile("fxsave %0":"=m"(prior)::"memory");
        __asm__ volatile("fninit\n\tfldcw %0"::"m"(cw):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::uint32_t,6> raw{};
        reinterpret_cast<Fn>(code_)(descriptor.data(),cursor,maximum,raw.data());
        __asm__ volatile("fxrstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7");
        Result r;r.cursor=raw[0];r.maximum=raw[1];
        r.advance=std::bit_cast<std::int64_t>(std::uint64_t(raw[2])|(std::uint64_t(raw[3])<<32));
        r.cw=static_cast<std::uint16_t>(raw[4]);r.status=static_cast<std::uint16_t>(raw[5]);return r;
    }
};
int wmain(int argc,wchar_t **argv)try{
    need(argc==4,"Args: original CivilizationV.exe NEW report.json NEW reference.bin");
    need(!std::filesystem::exists(argv[2])&&!std::filesystem::exists(argv[3]),"Preserve previous cursor evidence");
    OriginalCursor original(argv[1]);
    Bytes reference{'C','U','R','S','O','R','0','1'};
    for(char ch:std::string(cursor_source::exe_sha))reference.push_back(static_cast<unsigned char>(ch));reference.resize(76);
    std::vector<std::uint32_t> advances;
    for(unsigned exponent=0;exponent<256;++exponent)for(auto mantissa:{0u,1u,0x7fffffu})for(auto sign:{0u,0x80000000u})
        advances.push_back(sign|(exponent<<23)|mantissa);
    for(auto bits:{0x3fe00000u,0xbfe00000u,0x4f7fffffu,0x4f800000u,0x4f800001u,0xcf800001u})advances.push_back(bits);
    std::uint32_t rows=0,accepted=0,indefinite=0,negative=0,wrapped=0;
    const auto record=[&](std::uint32_t bits,std::uint32_t cursor,std::uint32_t width,std::uint16_t cw){
        // Raw source descriptor fixture: +0x20 here is raw slot +0x28.
        static_assert(8*sizeof(float)==0x20 && 8+8*sizeof(float)==0x28);
        const std::array<float,9> descriptor{.11f,.23f,.67f,.89f,-.374f,-.374f,12.0f,23.0f,std::bit_cast<float>(bits)};
        const auto expected=original.run(descriptor,cursor,width,cw);
        need(expected.cw==cw&&((expected.status>>11)&7u)==0,"Original slice failed CW/empty-stack continuation contract");
        const bool valid=(expected.status&1u)==0; // ORIGINAL masked invalid conversion, not a duplicated range predicate.
        if(valid){
            ++accepted;if(expected.advance<0)++negative;
            if(expected.cursor<cursor&&static_cast<std::uint32_t>(expected.advance))++wrapped;
            need(expected.cursor==cursor+static_cast<std::uint32_t>(expected.advance),"Original low32 modulo addition differs");
            need(expected.maximum==(expected.cursor>width?expected.cursor:width),"Original unsigned max differs");
        }else{
            need(std::bit_cast<std::uint64_t>(expected.advance)==0x8000000000000000ull&&expected.cursor==cursor,
                "Original invalid conversion did not produce indefinite/low32zero");++indefinite;
        }
        put(reference,cursor);put(reference,width);put(reference,cw,2);put(reference,0,2);
        // Diagnostic fixture metadata is transported for actual x64 quote retention tests;
        // this x86 process owns no Civis cache/slot/currentness API.
        put(reference,17,2);put(reference,17,2);
        for(float f:descriptor)put(reference,std::bit_cast<std::uint32_t>(f));
        put(reference,177);put(reference,0x123456789abcdef0ull,8);put(reference,37,8);
        put(reference,1);put(reference,valid?1u:0u);
        put(reference,std::bit_cast<std::uint64_t>(expected.advance),8);put(reference,expected.cursor);put(reference,expected.maximum);
        put(reference,expected.cw,2);put(reference,expected.status,2);++rows;
    };
    for(auto bits:advances)for(unsigned pc:{0u,2u,3u})for(unsigned rc=0;rc<4;++rc){
        const auto cw=static_cast<std::uint16_t>(0x007fu|(pc<<8)|(rc<<10));
        record(bits,0,0,cw);record(bits,0xffffffffu,0x80000000u,cw);
    }
    for(auto bits:{0x3fe00000u,0xbfe00000u,0x4f800000u,0x4f800001u,0x5effffffu,0xdf000000u})
        for(auto cursor:{0u,1u,0x7ffffffeu,0x80000000u,0xfffffffeu,0xffffffffu})for(auto maximum:{0u,0x7fffffffu,0x80000000u,0xffffffffu})
            for(unsigned pc:{0u,2u,3u})for(unsigned rc=0;rc<4;++rc)record(bits,cursor,maximum,static_cast<std::uint16_t>(0x007fu|(pc<<8)|(rc<<10)));
    std::array<float,9> witness{};witness[8]=std::bit_cast<float>(0xbfe00000u);
    const auto source=original.run(witness,0,0,0x037f);
    need(source.cursor==0xffffffffu&&source.maximum==0xffffffffu,"Negative advance unsigned-max witness missing");
    witness[8]=std::bit_cast<float>(0xdf000000u);
    const auto endpoint=original.run(witness,0,0,0x037f);
    need(endpoint.advance==(-9223372036854775807ll-1)&&!(endpoint.status&1u),"Negative signed64 endpoint rejected by original");
    need(sha(read(argv[1]))==cursor_source::exe_sha,"Original EXE changed during source execution");
    for(unsigned n=0;n<4;++n)reference[72+n]=static_cast<unsigned char>(rows>>(8*n));need(reference.size()==76ull+100ull*rows,"Cursor reference record layout");write(argv[3],reference);
    const std::string report="{\"schema\":\"civis-original-cursor-continuation-gate-v1\",\"originalInstructionBytes\":57,\"records\":"+std::to_string(rows)+
        ",\"originalValidObservations\":"+std::to_string(accepted)+",\"originalMaskedIndefiniteObservations\":"+std::to_string(indefinite)+
        ",\"negativeValidConversions\":"+std::to_string(negative)+",\"wrappedAdditions\":"+std::to_string(wrapped)+
        ",\"nativeLabelReady\":false,\"productionIncomingCwObserved\":false,\"currentParsedLinesObserved\":false,\"gpuExecuted\":false,\"pass\":true}\n";
    write(argv[2],Bytes(report.begin(),report.end()));std::cout<<"PASS original57-byte cursor: "<<accepted<<" valid, "<<indefinite<<" original masked-indefinite observations; no candidate/loop/production cursor\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
