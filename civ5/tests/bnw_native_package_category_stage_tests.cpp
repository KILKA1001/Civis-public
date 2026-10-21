#include "bnw_native_package_category_stage.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <cstring>
#include <bit>
using namespace civ5::ui::detail;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::uint32_t u32(std::istream& in){std::array<unsigned char,4>b{};in.read(reinterpret_cast<char*>(b.data()),4);need(bool(in),"Truncated category CPU fixture");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
std::int64_t i64(std::istream& in){const auto lo=u32(in),hi=u32(in);return std::bit_cast<std::int64_t>((std::uint64_t(hi)<<32)|lo);}
template<class F>void refuses(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Missing category mandatory source unexpectedly accepted");}
}
int main(int argc,char**argv){try{
    need(argc==2,"Original category CPU fixture required");std::ifstream in(argv[1],std::ios::binary);std::array<char,8>magic{};in.read(magic.data(),magic.size());need(std::string(magic.data(),magic.size())=="BNWCAT01","Category fixture schema differs");
    const auto decisions=u32(in);need(decisions==720,"Bounded original mode decision count differs");
    for(unsigned i=0;i<decisions;++i){
        const auto flags=u32(in),archived=u32(in),cache=u32(in),found=u32(in),borrowed=u32(in),expected=u32(in),deleted=u32(in),recycled=u32(in);const auto incoming=i64(in),cached=i64(in);
        const auto actual=category_record_replacement(flags,{bool(archived),bool(cache),bool(found),cached},incoming);
        need(actual!=CategoryReplaceDecision::unknown_mandatory_input && unsigned(actual==CategoryReplaceDecision::replace)==expected && deleted==expected && recycled==(expected&&!borrowed),"Original83B130 mode7/cache/time decision differs");
    }
    const auto loose=u32(in);need(loose==18,"Original unchecked loose boundary count differs");
    for(unsigned i=0;i<loose;++i){
        const auto flags=u32(in),ok=u32(in),override=u32(in),actualflags=u32(in),observedSize=u32(in),observedLo=u32(in),observedHi=u32(in);const auto size=std::bit_cast<std::int32_t>(u32(in));const auto time=i64(in);
        need(category_loose_flags(flags,bool(override))==actualflags,"Original directory XML/ggxml flag transformation differs");
        if(ok){const auto known=require_category_loose_fields({0,size,time});need(std::bit_cast<std::uint32_t>(known.size)==observedSize && std::bit_cast<std::uint64_t>(known.mtime)==((std::uint64_t(observedHi)<<32)|observedLo),"Original unchecked successful stat fields differ");}
        else {need(observedSize==0xa5a5a5a5 && observedLo==0xa5a5a5a5 && observedHi==0xa5a5a5a5,"Original failed stat fixture poison changed");refuses([&]{(void)require_category_loose_fields({-1,{}, {}});});}
    }
    need(in.peek()==std::char_traits<char>::eof(),"Trailing category CPU fixture");
    need(category_record_replacement(7,{true,true,true,{}},1)==CategoryReplaceDecision::unknown_mandatory_input &&
         category_record_replacement(7,{true,true,true,1},{})==CategoryReplaceDecision::unknown_mandatory_input,"Missing ORIGINAL cache/incoming time guessed");
    need(category_record_replacement(7,{},{})==CategoryReplaceDecision::unknown_mandatory_input &&
         category_record_replacement(7,{true,{}, {},1},1)==CategoryReplaceDecision::unknown_mandatory_input,
         "Unowned cache presence incorrectly treated as proved native absence");
    refuses([&]{(void)require_category_loose_fields({0,{},1});});
    const auto common=category_source_plan({0,false,false,false,0});
    need(common.front().phase==CategorySourcePhase::create_primary_even_empty_paks && common[3].phase==CategorySourcePhase::scan_package_root_loose_then_top_level_fpks_for_empty_common && common[3].flags==7,"Empty Common skipped mandatory constructor/root scan");
    const auto gameplay=category_source_plan({1,false,false,false,0});
    need(gameplay.front().phase==CategorySourcePhase::create_primary_even_empty_paks && gameplay[3].phase==CategorySourcePhase::scan_explicit_files,"Empty Gameplay fabricated default directory scan");
    need(category_source_plan({0,true,false,false,0}).empty() && category_source_plan({1,false,true,false,0}).front().phase==CategorySourcePhase::enable_existing_pair,"Invalid/existing native category branch differs");
    refuses([&]{(void)category_source_plan({0,false,false,true,0});});
    std::cout<<"PASS original category numeric decisions="<<decisions<<" unchecked stat boundaries="<<loose<<"; missing fields refuse; live category/cache/publication remain pending\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
