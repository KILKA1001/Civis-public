#pragma once
#include "civ5/assets/asset_source.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
namespace civ5::graphics::gsd_detail {
// Private byte codec. No native enumeration, allocator, manager or bootstrap authority.
inline void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
inline std::uint32_t u32(std::span<const std::uint8_t>b,std::size_t p){need(p<=b.size()&&b.size()-p>=4,"GSD word outside payload");return b[p]|(std::uint32_t(b[p+1])<<8)|(std::uint32_t(b[p+2])<<16)|(std::uint32_t(b[p+3])<<24);}
inline void range(std::span<const std::uint8_t>b,std::uint64_t p,std::uint64_t n){need(p<=b.size()&&n<=b.size()-p,"GSD range outside payload");}
struct Reference {std::uint32_t section,offset;};
struct Section {std::uint32_t offset,size;std::map<std::uint32_t,Reference> references;};
struct Descriptor {std::vector<std::string> names;std::uint32_t sections=0,relocations=0;};
inline std::uint32_t name_crc(std::string_view name){std::uint32_t crc=0xffffffff;for(unsigned char c:name){need(c!=0,"GSD pool name embeds NUL");crc^=c;for(unsigned i=0;i<8;++i)crc=(crc>>1)^((crc&1)?0xedb88320U:0U);}return crc;}
inline Descriptor decode(std::span<const std::uint8_t>b){
 need(b.size()>=0x68&&b.size()<=16*1024*1024,"GSD bounded payload extent");
 constexpr std::array<std::uint8_t,16> magic={0x29,0xde,0x6c,0xc0,0xba,0xa4,0x53,0x2b,0x25,0xf5,0xb7,0xa5,0xf6,0x66,0xe2,0xee};
 need(std::equal(magic.begin(),magic.end(),b.begin()),"GSD little-endian32 magic unsupported");
 need(u32(b,0x14)==0&&u32(b,0x18)==0&&u32(b,0x1c)==0,"GSD extended magic unsupported");
 need(u32(b,0x20)==7&&u32(b,0x24)==b.size(),"GSD version/file extent unsupported");
 need(u32(b,0x44)==0x80000031,"GSD descriptor schema conversion required");
 const auto count=u32(b,0x30),start=u32(b,0x2c);need(count>0&&count<=32&&start>=0x48,"GSD section directory extent");range(b,std::uint64_t(0x20)+start,std::uint64_t(count)*0x2c);
 const auto header_end=std::uint64_t(0x20)+start+std::uint64_t(count)*0x2c;need(u32(b,0x10)==header_end,"GSD magic header length mismatch");
 std::vector<Section> sections;Descriptor out;out.sections=count;
 for(std::uint32_t i=0;i<count;++i){auto s=std::size_t(0x20)+start+std::size_t(i)*0x2c;
  need(u32(b,s)==0&&u32(b,s+8)==u32(b,s+12),"GSD compressed section unsupported");
  need(u32(b,s+16)==4,"GSD section alignment unsupported");
  need(u32(b,s+40)==0,"GSD mixed-width section conversion required");
  const auto p=u32(b,s+4),size=u32(b,s+12);range(b,p,size);need(p>=header_end,"GSD section overlaps header");
  const auto rel=u32(b,s+28),n=u32(b,s+32);need(n<=1000000,"GSD relocation count exceeds budget");range(b,rel,std::uint64_t(n)*12);
  Section section{p,size,{}};
  for(std::uint32_t j=0;j<n;++j){auto at=std::size_t(rel)+std::size_t(j)*12;auto field=u32(b,at),target=u32(b,at+4),offset=u32(b,at+8);need(field<=size&&size-field>=4&&field%4==0,"GSD relocation field outside section");need(target<count,"GSD relocation target section outside directory");need(section.references.emplace(field,Reference{target,offset}).second,"GSD duplicate relocation field");}
  need(out.relocations<=1000000-n,"GSD aggregate relocation budget");out.relocations+=n;sections.push_back(std::move(section));
 }
 for(const auto&s:sections)for(const auto&[field,ref]:s.references){(void)field;need(ref.offset<sections[ref.section].size,"GSD relocation target outside section");}
 const auto rootsection=u32(b,0x3c),root=u32(b,0x40),typesection=u32(b,0x34),type=u32(b,0x38);need(rootsection<count&&typesection<count,"GSD root section outside directory");need(type<sections[typesection].size,"GSD root type outside section");const auto&s=sections[rootsection];need(root<=s.size&&s.size-root>=8,"GSD descriptor outside section");
 const auto names=u32(b,std::size_t(s.offset)+root);need(names<=1000000,"GSD descriptor count exceeds budget");
 auto ref=s.references.find(root+4);need(ref!=s.references.end(),"GSD descriptor name array missing relocation");const auto&a=sections[ref->second.section];const auto first=ref->second.offset;need(first<=a.size&&std::uint64_t(names)*4<=a.size-first,"GSD descriptor array outside section");
 out.names.reserve(names);std::uint64_t total=0;
 for(std::uint32_t i=0;i<names;++i){auto stringref=a.references.find(first+4*i);need(stringref!=a.references.end(),"GSD name pointer missing relocation");const auto&t=sections[stringref->second.section];auto off=stringref->second.offset;std::string text;
  while(off<t.size&&b[std::size_t(t.offset)+off]!=0){need(text.size()<4096,"GSD string exceeds budget");text.push_back(char(b[std::size_t(t.offset)+off++]));}
  need(off<t.size,"GSD name lacks terminating NUL");need(total+text.size()<=16*1024*1024,"GSD aggregate string budget");total+=text.size();out.names.push_back(std::move(text));
 }return out;
}
// Existing typed reader only; caller must establish SAME native namespace/resource lineage.
inline Descriptor read(const assets::AssetHandle& handle){return decode(handle.read_bytes(16*1024*1024));}
class StringPool {
 std::map<std::uint32_t,std::string> first_;
 public:
 void retain(const Descriptor& d){for(const auto& name:d.names)first_.try_emplace(name_crc(name),name);}
 const auto& names()const{return first_;}
};
} // namespace civ5::graphics::gsd_detail
