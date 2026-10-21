#include "civ5/ui/native_label_color_pack.hpp"
#include "native_label_color_pack_source_pins.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

static_assert(sizeof(void*)==4,"Original instruction slice requires x86 process");
using Bytes=std::vector<unsigned char>;
void require(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
std::string sha(const Bytes&bytes) {
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 require(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"SHA provider");
 DWORD length=0,got=0;BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),4,&got,0);
 Bytes object(length),digest(32);
 const auto created=BCryptCreateHash(alg,&hash,object.data(),length,nullptr,0,0);
 if(created<0){BCryptCloseAlgorithmProvider(alg,0);throw std::runtime_error("SHA create");}
 const auto hashed=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),ULONG(bytes.size()),0);
 const auto finished=hashed>=0?BCryptFinishHash(hash,digest.data(),32,0):hashed;
 BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
 require(finished>=0,"SHA finish");
 std::string text;const char*digits="0123456789abcdef";
 for(auto b:digest){text+=digits[b>>4];text+=digits[b&15];}return text;
}
Bytes read(const std::filesystem::path&path) {
 std::ifstream stream(path,std::ios::binary);require(bool(stream),"Read original executable");
 return Bytes(std::istreambuf_iterator<char>(stream),{});
}
class SourceSlice {
 using Fn=void(__cdecl*)(const float*,std::uint32_t*,const float*,std::uint32_t*);
 void*allocation_=nullptr;float scale_=0;
public:
 explicit SourceSlice(const Bytes&exe) {
  require(sha(exe)==label_pack_source::exe_sha,"Whole original EXE SHA mismatch");
  const auto pos=label_pack_source::slice_raw_offset;
  require(pos+sizeof(label_pack_source::source_slice)<=exe.size(),"Slice range");
  Bytes source(exe.begin()+pos,exe.begin()+pos+sizeof(label_pack_source::source_slice));
  require(sha(source)==label_pack_source::slice_sha,"Original slice SHA mismatch");
  require(std::equal(source.begin(),source.end(),std::begin(label_pack_source::source_slice)),"Original slice bytes mismatch");
  const auto constant=label_pack_source::constant_raw_offset;
  require(constant+4<=exe.size(),"Constant range");
  std::uint32_t scale_bits=0;std::memcpy(&scale_bits,exe.data()+constant,4);
  require(scale_bits==0x437f0000u,"Original 255 constant bits mismatch");
  scale_=std::bit_cast<float>(scale_bits);
  // Own ABI/scratch wrapper. The 209-byte body itself is byte-for-byte original.
  // No calls, absolute data references, or branch relocations in that body.
  // Body ESP+6c/70/74 are outputs; scratch stays inside our 0xa0-byte frame.
  Bytes code={0x53,0x56,0x57,0x55,0x81,0xec,0xa0,0,0,0,
    0x8b,0x94,0x24,0xb4,0,0,0,0x83,0xc2,8,0x33,0xf6,
    0x8b,0x84,0x24,0xbc,0,0,0,0xd9,0x00};
  code.insert(code.end(),source.begin(),source.end());
  const unsigned char tail[]={
    0xd9,0x7c,0x24,0x78,0xdf,0xe0,0x66,0x89,0x44,0x24,0x7a, // CW,SW
    0xdd,0xd8, // source constant ST0 pop, outside original numeric loop
    0x8b,0x84,0x24,0xb8,0,0,0,
    0x8b,0x4c,0x24,0x6c,0x89,0x08,
    0x8b,0x4c,0x24,0x70,0x89,0x48,4,
    0x8b,0x4c,0x24,0x74,0x89,0x48,8,
    0x8b,0x84,0x24,0xc0,0,0,0,
    0x8b,0x4c,0x24,0x78,0x89,0x08,
    0x81,0xc4,0xa0,0,0,0,0x5d,0x5f,0x5e,0x5b,0xc3};
  code.insert(code.end(),std::begin(tail),std::end(tail));
  allocation_=VirtualAlloc(nullptr,code.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
  require(allocation_!=nullptr,"Allocate own instruction wrapper");
  std::memcpy(allocation_,code.data(),code.size());DWORD old=0;
  if(!VirtualProtect(allocation_,code.size(),PAGE_EXECUTE_READ,&old) ||
     !FlushInstructionCache(GetCurrentProcess(),allocation_,code.size())) {
   VirtualFree(allocation_,0,MEM_RELEASE);allocation_=nullptr;
   throw std::runtime_error("Protect own source-slice wrapper RX");
  }
 }
 ~SourceSlice(){if(allocation_)VirtualFree(allocation_,0,MEM_RELEASE);}
 SourceSlice(const SourceSlice&)=delete;SourceSlice&operator=(const SourceSlice&)=delete;
 struct Output {std::array<std::uint32_t,3>words{};std::uint16_t cw=0,sw=0;};
 Output run(const std::array<float,4>&rgba,std::uint16_t cw) const {
  alignas(16) std::array<unsigned char,512>environment{};
  __asm__ volatile("fxsave %0":"=m"(environment));
  std::array<float,12>groups{};
  for(unsigned group=0;group<3;++group)std::copy(rgba.begin(),rgba.end(),groups.begin()+4*group);
  std::uint32_t control_status=0;Output out;
  __asm__ volatile("fninit\n\tfldcw %0"::"m"(cw));
  reinterpret_cast<Fn>(allocation_)(groups.data(),out.words.data(),&scale_,&control_status);
  __asm__ volatile("fxrstor %0"::"m"(environment));
  out.cw=std::uint16_t(control_status);out.sw=std::uint16_t(control_status>>16);return out;
 }
};
struct Difference {std::uint32_t bits,pc24,pc64;};
int wmain(int argc,wchar_t**argv) try {
 require(argc==3,"Args: original CivilizationV.exe, NEW report.json path");
 require(!std::filesystem::exists(argv[2]),"Do not overwrite any previous report");
 const auto original=read(argv[1]);SourceSlice oracle(original);
 std::vector<std::array<float,4>>cases;
 auto from=[](std::uint32_t bits){return std::bit_cast<float>(bits);};
 auto add=[&](std::uint32_t bits) {
  if((bits&0x7f800000u)==0x7f800000u)return;
  const auto magnitude=bits&0x7fffffffu;
  cases.push_back({from(bits),from(bits^0x80000000u),from(magnitude),from(magnitude?magnitude-1:0)});
 };
 for(auto bits:{0u,0x80000000u,1u,0x007fffffu,0x00800000u,0x3f000000u,
      0x3f800000u,0x3f800001u,0xbf800000u,0x40000000u,0x7f7fffffu,0xff7fffffu})add(bits);
 // Neighboring representable source float32 values around all byte boundaries.
 // They are INPUTS only. Expected bytes come from original instructions.
 for(unsigned k=1;k<=256;++k) {
  const auto center=std::bit_cast<std::uint32_t>(float(double(k)/255.0));
  for(int neighbor=-3;neighbor<=3;++neighbor)add(std::uint32_t(std::int64_t(center)+neighbor));
 }
 // A concrete byte-boundary counterexample outside [0,1], which original
 // masks rather than clamps: bitpattern3f810101 times255 is <257 at PC64,
 // but rounds to257 at PC24. Include its neighbors before random inputs.
 for(auto center:{0x3f810101u,0x3f820202u,0x3f830303u})
  for(int neighbor=-2;neighbor<=2;++neighbor)add(std::uint32_t(std::int64_t(center)+neighbor));
 const auto limit=std::bit_cast<std::uint32_t>(float(std::ldexp(1.0,63)/255.0));
 for(int neighbor=-4;neighbor<=4;++neighbor)add(std::uint32_t(std::int64_t(limit)+neighbor));
 std::uint32_t random=0x5077f0u;
 for(unsigned n=0;n<4096;++n){random^=random<<13;random^=random>>17;random^=random<<5;add(random);}
 std::uint64_t compared=0,invalid=0;std::vector<Difference>differences;
 for(const auto&rgba:cases) {
  for(unsigned precision:{0u,2u,3u})for(unsigned rounding=0;rounding<4;++rounding) {
   const std::uint16_t cw=std::uint16_t(0x007fu|(precision<<8)|(rounding<<10));
   const auto candidate=civ5::ui::pack_native_label_color({rgba,cw});
   const auto actual=oracle.run(rgba,cw);
   if(actual.words[0]!=candidate.packed_rgba || actual.words[1]!=candidate.packed_rgba || actual.words[2]!=candidate.packed_rgba) {
    std::cerr<<"Original slice mismatch CW="<<std::hex<<cw<<" input=";
    for(auto bits:candidate.input_float32_bits)std::cerr<<bits<<',';
    std::cerr<<" actual="<<actual.words[0]<<','<<actual.words[1]<<','<<actual.words[2]<<" candidate="<<candidate.packed_rgba<<'\n';
    return 1;
   }
   require(actual.cw==cw,"Original source did not preserve incoming CW");
   require(((actual.sw>>11)&7u)==7u,"Original numeric loop changed retained constant stack depth");
   require(bool(actual.sw&1u)==bool(candidate.masked_invalid_conversion_channels),"Original masked FISTP invalid status mismatch");
   require(!candidate.native_label_ready,"Numeric conversion minted live Label readiness");
   ++compared;invalid+=candidate.masked_invalid_conversion_channels!=0;
  }
  const auto p24=oracle.run(rgba,0x007f),p64=oracle.run(rgba,0x037f);
  if(p24.words[0]!=p64.words[0] && differences.size()<12)
   differences.push_back({std::bit_cast<std::uint32_t>(rgba[0]),p24.words[0],p64.words[0]});
 }
 require(!differences.empty(),"Missing meaningful PC24 rounding boundary cases");
 const std::array<float,4>boundary{from(0x3f810101u),0,0,0};
 const auto actual24=oracle.run(boundary,0x007f),actual64=oracle.run(boundary,0x037f);
 require(actual24.words[0]==1 && actual64.words[0]==0,"Original explicit byte-boundary witness changed");
 unsigned rejected=0;
 auto rejection=[&](const civ5::ui::NativeLabelColorPackRequest&r) {
  bool failed=false;try{(void)civ5::ui::pack_native_label_color(r);}catch(const std::invalid_argument&){failed=true;}
  require(failed,"Invalid source input/profile accepted");++rejected;
 };
 for(auto bits:{0x7f800000u,0xff800000u,0x7fc00000u,0x7f800001u})
  for(unsigned channel=0;channel<4;++channel){auto rgba=cases.front();rgba[channel]=from(bits);rejection({rgba,0x037f});}
 for(unsigned mask=0;mask<6;++mask)rejection({cases.front(),std::uint16_t(0x037fu&~(1u<<mask))});
 rejection({cases.front(),0x017f});
 using Vertex=civ5::ui::NativeLabelPackedVertex;
 static_assert(offsetof(Vertex,x)==0 && offsetof(Vertex,rgba)==12 && offsetof(Vertex,u)==16 && offsetof(Vertex,v)==20);
 const auto rgba=civ5::ui::pack_native_label_color({{0.0f,0.25f,0.5f,1.0f},0x037f});
 const Vertex vertex{1,2,3,rgba.packed_rgba,4,5};
 require(std::memcmp(reinterpret_cast<const unsigned char*>(&vertex)+12,rgba.rgba_bytes.data(),4)==0,"Stride24 color order differs from UBYTE4N source");
 // A real mutation of numeric policy must be exposed by original instruction
 // results: PC24 round-to-nearest differs from exact PC64 before truncation.
 require(sha(read(argv[1]))==label_pack_source::exe_sha,"Original executable changed during numeric gate");
 std::ofstream report(argv[2],std::ios::binary);
 require(bool(report),"Create NEW numeric report");
 report<<"{\n \"schema\":\"civis-original-label-color-pack-gate-v1\",\n"
   <<" \"exeSha256\":\""<<label_pack_source::exe_sha<<"\",\n"
   <<" \"sliceSha256\":\""<<label_pack_source::slice_sha<<"\",\n"
   <<" \"originalSliceBytes\":209,\n \"inputCases\":"<<cases.size()<<",\n"
   <<" \"profileComparisons\":"<<compared<<",\n \"threeOriginalGroupsCompared\":true,\n"
   <<" \"finiteMaskedInvalidCases\":"<<invalid<<",\n \"negativeRejections\":"<<rejected<<",\n"
   <<" \"incomingProductionCwObserved\":false,\n \"nativeLabelReady\":false,\n"
   <<" \"nativeVertexDeclarationExecuted\":false,\n \"gpuExecuted\":false,\n"
   <<" \"boundaryDifferences\":[\n";
 for(std::size_t n=0;n<differences.size();++n) {
  const auto&d=differences[n];report<<"  {\"rInputBits\":"<<d.bits<<",\"pc24Packed\":"<<d.pc24<<",\"pc64Packed\":"<<d.pc64<<"}"<<(n+1<differences.size()?",":"")<<'\n';
 }
 report<<" ],\n \"pass\":true\n}\n";report.flush();require(bool(report),"Flush report");report.close();require(!report.fail(),"Close report");
 std::cout<<"PASS original 209-byte x86 slice: "<<compared<<" profiles, "<<cases.size()<<" cases, "<<rejected<<" negatives; no GPU/live Label claims\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
