#include "bnw_native_archive_basename_cache.hpp"
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void refuses(F&& f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Unsupported cache key accepted");}
template<class T>concept PublicAcquire=requires{T::acquire({},{});};
std::uint32_t raw_crc(std::string_view s){std::uint32_t c=0xffffffffu;for(char b:s){c^=static_cast<unsigned char>(b);for(unsigned n=0;n<8;++n)c=(c>>1)^((c&1u)?0xedb88320u:0u);}return c;}
}
int main(){try{
    static_assert(!PublicAcquire<BnwNativeArchiveBasenameCacheOwner> &&
                  !std::is_default_constructible_v<BnwNativeArchiveCachePublicationOwner> &&
                  !std::is_copy_constructible_v<BnwNativeArchiveCacheLookupOwner>);
    need(archive_cache_basename_ascii("Resource\\DX9\\AmericanWashingtonTextures.fpk")=="americanwashingtontextures.fpk","Source32 basename differs");
    need(archive_cache_basename_ascii("C:First:Second.FPK")=="first:second.fpk","FIRST colon semantics differ");
    need(archive_cache_basename_ascii("C:First\\Last.FPK")=="last.fpk","LAST backslash must win before colon");
    need(archive_cache_basename_ascii("Resource/DX9/Name.FPK")=="resource/dx9/name.fpk","Forward slash became native delimiter");
    refuses([]{archive_cache_basename_ascii(std::string_view("x\0y",3));});
    refuses([]{archive_cache_basename_ascii(std::string_view("\x80",1));});
    BnwArchiveBasenameCacheStoragePolicy<std::int64_t> cache;
    need(cache.lookup("name.fpk")==nullptr,"Fresh namespace cache was not empty");
    need(!archive_cache_positive_loader_diagnostic(0)&&archive_cache_positive_loader_diagnostic(1)&&
         archive_cache_positive_loader_diagnostic(66),"Executed loader return zero branch differs");
    if(archive_cache_positive_loader_diagnostic(0))cache.insert_unique("zero.fpk",123);
    need(cache.size()==0,"Zero loader result inserted cache value");
    const auto key=archive_cache_basename_ascii("Resource\\DX9\\Name.FPK");
    need(cache.insert_unique(key,111)&&!cache.insert_unique(archive_cache_basename_ascii("Different\\NAME.fpk"),222)&&
         *cache.lookup("name.fpk")==111,"82B590 existing node overwritten");
    need(cache.lookup("Name.FPK")==nullptr,"Map applied case folding after caller _strlwr stage");
    const std::string left="explosion200poundwater-002.wav.fpk",right="work_boat_death.dge.fpk";
    need(raw_crc(left)==raw_crc(right)&&left!=right,"Independent collision fixture differs");
    need(cache.insert_unique(left,333)&&cache.insert_unique(right,444)&&*cache.lookup(left)==333&&
         *cache.lookup(right)==444,"Archive cache used resource CRC-only equality");
    // Check map staging/noexcept source store: no mutation until all work done.
    auto staged=cache;staged.insert_unique("new.fpk",555);
    need(cache.lookup("new.fpk")==nullptr,"Cache preparation mutated live store");
    static_assert(noexcept(cache.swap(staged)));cache.swap(staged);
    need(*cache.lookup("new.fpk")==555&&cache.size()==4,"Prepared cache publication differs");
    std::cout<<"PASS cache source basename/full-name equality/CRC collision/first value/zero gate/noexcept store; genuine source hooks/HWND pending Root\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
