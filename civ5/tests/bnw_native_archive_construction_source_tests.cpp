#include "bnw_native_archive_construction.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint32_t u32(std::istream& in){std::array<unsigned char,4>b{};in.read(reinterpret_cast<char*>(b.data()),4);need(bool(in),"Truncated archive source fixture");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
std::int64_t i64(std::istream& in){const auto lo=u32(in),hi=u32(in);return std::bit_cast<std::int64_t>((std::uint64_t(hi)<<32)|lo);}
std::string text(std::istream& in){auto n=u32(in);need(n<4096,"Unsafe source fixture string");std::string out(n,'\0');in.read(out.data(),n);need(bool(in),"Truncated source fixture string");return out;}
template<class F>void refuses(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Unknown archive source accepted");}
}
int main(int argc,char**argv){try{
    static_assert(!std::is_default_constructible_v<BnwNativeArchiveConstruction> && !std::is_copy_constructible_v<BnwNativeArchiveConstruction>);
    need(argc==2,"Original archive source fixture required");std::ifstream in(argv[1],std::ios::binary);std::array<char,8>magic{};in.read(magic.data(),8);need(std::string(magic.data(),8)=="BNWARC01","Archive source fixture schema differs");
    const auto times=u32(in);need(times==6,"Bounded source time fixture count differs");
    for(unsigned j=0;j<times;++j){const auto ok=u32(in);const auto incoming=i64(in),expected=i64(in);const auto pref=u32(in);need(pref==1,"Original archive preference initializer differs");
        BnwNativeOriginalStatFields fields;fields.return_value=ok?0:-1;if(ok)fields.mtime=incoming;
        need(archive_checked_stat_mtime(fields)==expected,"Original checked stat failed-zero/success-time control differs");}
    const auto names=u32(in);need(names==16,"Bounded native key fixture count differs");
    for(unsigned j=0;j<names;++j){const auto input=text(in),expected=text(in);need(archive_cache_basename_ascii(input)==expected,"Original native cache basename differs");}
    need(in.peek()==std::char_traits<char>::eof(),"Trailing archive source fixture");
    refuses([&]{(void)archive_checked_stat_mtime({0,0,{},{}});});
    refuses([&]{(void)archive_cache_basename_ascii(std::string("a\0b",3));});
    refuses([&]{(void)archive_cache_basename_ascii(std::string(1,char(0xff)));});
    std::cout<<"PASS original archive stat kernel="<<times<<" native ANSI basename="<<names<<"; private Construction unmanufactured; mappedstream/prefix/cache remain root pending\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
