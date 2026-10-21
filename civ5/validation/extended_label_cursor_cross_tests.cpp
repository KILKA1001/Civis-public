#include "extended_label_cursor_test_support.hpp"
#include <optional>
using namespace cursor_test;
static_assert(sizeof(void*)==8,"Cross gate requires fresh x64 process");
int wmain(int argc,wchar_t **argv)try{
    need(argc==4,"Args: independently original-produced reference.bin expectedSHA NEW report.json");
    need(!std::filesystem::exists(argv[3]),"Preserve previous cursor cross evidence");
    std::string expected_sha;for(const wchar_t *p=argv[2];*p;++p){need(*p<128,"Reference SHA ASCII");expected_sha.push_back(static_cast<char>(*p));}
    const auto reference=read(argv[1]);need(expected_sha.size()==64&&sha(reference)==expected_sha,"Original reference SHA differs");
    need(reference.size()>=76&&std::equal(reference.begin(),reference.begin()+8,"CURSOR01"),"Cursor reference magic/version");
    need(std::equal(reference.begin()+8,reference.begin()+72,cursor_source::exe_sha),"Original EXE authority differs");
    std::size_t at=72;const auto count=get(reference,at);need(count>0&&reference.size()==76ull+100ull*count,"Cursor reference record bounds");
    std::uint32_t accepted=0,refused=0;
    for(std::uint64_t n=0;n<count;++n){
        civ5::graphics::BnwExtendedGlyphCursorControls c;
        c.cursor_x=static_cast<std::uint32_t>(get(reference,at));c.maximum_width=static_cast<std::uint32_t>(get(reference,at));
        c.diagnostic_incoming_x87_control_word=static_cast<std::uint16_t>(get(reference,at,2));need(get(reference,at,2)==0,"Unknown reference controls extension");c.caller_declared_numeric_inputs=true;
        civ5::ui::BnwSystemFontSlotQuote q;q.requested_glyph_index=static_cast<std::uint16_t>(get(reference,at,2));q.slot.glyph_index=static_cast<std::uint16_t>(get(reference,at,2));
        auto &s=q.slot;for(float *p:{&s.u0,&s.v0,&s.u1,&s.v1,&s.x0,&s.y0,&s.x1,&s.y1,&s.advance_float})*p=std::bit_cast<float>(static_cast<std::uint32_t>(get(reference,at)));
        s.advance=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(get(reference,at)));s.generation=get(reference,at,8);q.slot_index=get(reference,at,8);
        const auto flags=get(reference,at);need(flags<=3,"Unknown quote flags");q.cache_hit=(flags&1)!=0;s.raster_uploaded=(flags&2)!=0;
        const auto valid=get(reference,at);need(valid<=1,"Unknown original admission tag");
        const auto advance=std::bit_cast<std::int64_t>(get(reference,at,8));
        const auto cursor=static_cast<std::uint32_t>(get(reference,at));const auto maximum=static_cast<std::uint32_t>(get(reference,at));
        const auto cw=static_cast<std::uint16_t>(get(reference,at,2)),status=static_cast<std::uint16_t>(get(reference,at,2));
        need(cw==c.diagnostic_incoming_x87_control_word&&((status>>11)&7u)==0&&bool(valid)==((status&1u)==0),"Original status/CW/admission authority differs");
        std::optional<civ5::graphics::BnwExtendedGlyphCursorResult> result;
        try{result=civ5::graphics::continue_bnw_extended_glyph_cursor(q,c);}catch(const std::exception&){need(!valid,"x64 refused original valid cursor");}
        if(valid){
            need(bool(result),"x64 result absent on valid original row");const auto &r=*result;
            same_quote(q,r.retained_quote);diagnostic_only(r);
            need(r.truncated_advance==advance&&r.advance_low32==static_cast<std::uint32_t>(advance)&&r.cursor_x==cursor&&r.maximum_width==maximum&&
                r.diagnostic_restored_x87_control_word==cw&&r.diagnostic_x87_status==status,"x64 continuation differs from original qword/low32/unsignedmax/CW/status");++accepted;
        }else{
            need(!result,"x64 accepted original masked indefinite");need(std::bit_cast<std::uint64_t>(advance)==0x8000000000000000ull&&cursor==c.cursor_x,"Malformed native indefinite observation");++refused;
        }
    }
    need(at==reference.size()&&accepted+refused==count,"Incomplete cursor reference replay");
    auto q=quote(0x3fe00000u);civ5::graphics::BnwExtendedGlyphCursorControls controls{0xffffffffu,0x80000000u,0x037f,true};
    const auto witness=civ5::graphics::continue_bnw_extended_glyph_cursor(q,controls);need(witness.cursor_x==0&&witness.maximum_width==0x80000000u,"Unsigned overflow/max witness missing");
    std::uint16_t before_cw=0,after_cw=0;std::uint32_t before_mx=0,after_mx=0;
    __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(before_cw),"=m"(before_mx));
    (void)civ5::graphics::continue_bnw_extended_glyph_cursor(q,controls);
    __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(after_cw),"=m"(after_mx));need(before_cw==after_cw&&before_mx==after_mx,"x64 diagnostic FP environment leaked");
    auto negative_quote=quote(0xbfe00000u);
    auto negative_controls=controls;negative_controls.cursor_x=0;negative_controls.maximum_width=0;
    const auto before=civ5::graphics::continue_bnw_extended_glyph_cursor(negative_quote,negative_controls);
    negative_quote.slot.advance=0;negative_quote.slot.x1=999;negative_quote.slot.u1=0;
    const auto after=civ5::graphics::continue_bnw_extended_glyph_cursor(negative_quote,negative_controls);
    need(before.cursor_x==0xffffffffu&&before.maximum_width==0xffffffffu&&before.cursor_x==after.cursor_x&&before.maximum_width==after.maximum_width,
        "Raster bounds/advance substituted for returned +0x20/rawslot +0x28 advance_float");
    unsigned invalid=0;
    const auto reject=[&](auto input,auto control){
        bool refused_input=false;
        try{(void)civ5::graphics::continue_bnw_extended_glyph_cursor(input,control);}catch(const std::exception&){refused_input=true;}
        need(refused_input,"Malformed diagnostic input accepted");++invalid;
    };
    auto altered=controls;altered.caller_declared_numeric_inputs=false;reject(q,altered);
    auto wrong=q;++wrong.requested_glyph_index;reject(wrong,controls);
    for(auto profile:{0x017fu,0x037eu,0x03ffu,0x137fu,0x837fu}){
        altered=controls;altered.diagnostic_incoming_x87_control_word=static_cast<std::uint16_t>(profile);reject(q,altered);
    }
    need(sha(read(argv[1]))==expected_sha,"Original reference changed during replay");
    const std::string report="{\"schema\":\"civis-x64-original-cursor-cross-gate-v1\",\"independentOriginalReferenceSha256\":\""+expected_sha+"\",\"records\":"+std::to_string(count)+
        ",\"validComparisons\":"+std::to_string(accepted)+",\"nativeMaskedIndefiniteTypedRefusals\":"+std::to_string(refused)+
        ",\"malformedRefusals\":"+std::to_string(invalid)+",\"nativeLabelReady\":false,\"productionIncomingCwObserved\":false,\"currentParsedLinesObserved\":false,\"gpuExecuted\":false,\"pass\":true}\n";
    write(argv[3],Bytes(report.begin(),report.end()));std::cout<<"PASS x64 own original cursor reference: "<<accepted<<" valid, "<<refused<<" native masked-indefinite typed refusals; nativeLabelReady=false\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
