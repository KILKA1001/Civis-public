#include "civ5/ui/bnw_whole_raw_label_input.hpp"
#include "extended_label_cursor_io.hpp"
using namespace cursor_test;
static_assert(sizeof(void*)==8);
int wmain(int argc,wchar_t **argv)try{
    need(argc==4,"Args original-producedReference exactSHA NEWreport");need(!std::filesystem::exists(argv[3]),"Preserve prior cross evidence");
    std::string expected;for(const wchar_t *p=argv[2];*p;++p){need(*p<128,"ASCII SHA required");expected.push_back(static_cast<char>(*p));}
    const auto reference=read(argv[1]);need(expected.size()==64&&sha(reference)==expected,"Original UTF reference SHA differs");
    need(reference.size()>=76&&std::equal(reference.begin(),reference.begin()+8,"RAWUTF01")&&std::equal(reference.begin()+8,reference.begin()+72,cursor_source::exe_sha),"Original UTF source authority differs");
    std::size_t at=72;const auto count=get(reference,at);need(count>0&&count<100000,"UTF reference row bound");unsigned accepted=0,refused=0,diagnostic=0;
    for(std::uint64_t row=0;row<count;++row){
        const auto bytes=get(reference,at),selected=get(reference,at,2),capacity=get(reference,at,2),tag=get(reference,at,1),status=get(reference,at,1),consumed=get(reference,at,2),units=get(reference,at,2);
        need(bytes<=8192&&selected<=bytes&&selected<=4096&&capacity<=4096&&tag<=2&&status<=2&&units<=capacity&&consumed<=selected+1&&at+bytes+units*2<=reference.size(),"Malformed UTF original record");
        const std::string input(reinterpret_cast<const char*>(reference.data()+at),static_cast<std::size_t>(bytes));at+=bytes;
        std::vector<std::uint16_t> original;for(unsigned n=0;n<units;++n)original.push_back(static_cast<std::uint16_t>(get(reference,at,2)));
        if(tag==2){++diagnostic;continue;}
        if(tag==0){bool rejected=false;try{(void)civ5::ui::prepare_bnw_whole_raw_label_input(input);}catch(const std::exception&){rejected=true;}need(rejected,"Strict input accepted native-only malformed/prefix case");++refused;continue;}
        const auto out=civ5::ui::prepare_bnw_whole_raw_label_input(input);
        need(status==0&&consumed==bytes&&out.raw_utf8==input&&out.utf16==original&&out.source_bytes_consumed==bytes&&out.whole_raw_preserved,
             "Fresh x64 raw whole UTF preparation differs from original converter");
        need(!out.original_class_inputs_verified&&!out.native_label_initialized&&!out.gcp_executed,"Pure preparation minted class/GCP/Label readiness");++accepted;
    }
    need(at==reference.size()&&accepted+refused+diagnostic==count,"Incomplete source UTF replay");
    std::string borrowed="[ICON_GOLD]";const auto retained=civ5::ui::prepare_bnw_whole_raw_label_input(borrowed);borrowed.assign(100,'x');
    need(retained.raw_utf8=="[ICON_GOLD]"&&retained.utf16.size()==11&&retained.utf16[0]=='['&&retained.utf16.back()==']',"Raw markup filtered or result borrowed source storage");
    need(sha(read(argv[1]))==expected,"Source reference changed during cross gate");
    const std::string report="{\"schema\":\"civis-x64-raw-whole-label-input-v1\",\"referenceSha256\":\""+expected+"\",\"records\":"+std::to_string(count)+",\"validExactComparisons\":"+std::to_string(accepted)+",\"strictRefusals\":"+std::to_string(refused)+",\"originalOnlyTargetCapacityObservations\":"+std::to_string(diagnostic)+",\"rawMarkupRetained\":true,\"originalClassInputsVerified\":false,\"gcpExecuted\":false,\"nativeLabelReady\":false,\"pass\":true}\n";
    write(argv[3],Bytes(report.begin(),report.end()));std::cout<<"PASS raw wholeLabel x64 exact="<<accepted<<" strict refusals="<<refused<<" original capacity="<<diagnostic<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
