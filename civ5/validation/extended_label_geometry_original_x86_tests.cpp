#include "civ5/graphics/bnw_extended_label_glyph_geometry.hpp"
#include "extended_label_geometry_source_pins.hpp"
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
#include <optional>
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

class SourceGeometry {
 using Vertex=civ5::ui::NativeLabelPackedVertex;
 using Fn=void(__cdecl*)(const float*,std::uint32_t,std::uint32_t,unsigned,std::uint32_t,Vertex*,std::uint32_t*);
 void*stub_=nullptr;void*constant_=nullptr;void*capacity_=nullptr;
public:
 explicit SourceGeometry(const std::filesystem::path&path){
  try{
  // Unmodified original absolute reads resolve ONLY to owned scratch. Source
  // correction constant is pinned; capacity read is not consumed by this
  // extracted numeric slice. No source Label/manager instance is fabricated.
  constant_=VirtualAlloc(reinterpret_cast<void*>(0x009d0000),0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  if(constant_!=reinterpret_cast<void*>(0x009d0000)){
   const DWORD failure=GetLastError();MEMORY_BASIC_INFORMATION mapping{};
   const auto observed=VirtualQuery(reinterpret_cast<void*>(0x009d0000),&mapping,sizeof(mapping));
   std::cerr<<"Constant allocation diagnostic: actual="<<constant_<<" error="<<failure<<" VirtualQuery="<<observed<<" Base="<<mapping.BaseAddress<<" Allocation="<<mapping.AllocationBase<<" Region="<<mapping.RegionSize<<" State="<<std::hex<<mapping.State<<" Type="<<mapping.Type<<" Protection="<<mapping.Protect<<std::dec<<'\n';
  }
  require(constant_==reinterpret_cast<void*>(0x009d0000),"Original numeric constant address unavailable in independent x86 process");
  capacity_=VirtualAlloc(reinterpret_cast<void*>(0x02dc0000),0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  require(capacity_==reinterpret_cast<void*>(0x02dc0000),"Original unused capacity scratch address unavailable");
  // Reserve unchanged absolute source dependencies BEFORE reading/hash
  // buffers can occupy low VA; never repurpose a pre-existing mapping.
  const auto exe=read(path);
  require(sha(exe)==glyph_geometry_source::exe_sha,"Whole original EXE SHA mismatch");
  require(glyph_geometry_source::unsigned_two32_raw_offset+4<=exe.size(),"Original uint constant range");
  std::uint32_t constant=0;std::memcpy(&constant,exe.data()+glyph_geometry_source::unsigned_two32_raw_offset,4);
  require(constant==0x4f800000u,"Original uint32 correction constant changed");
  std::uint32_t one=0;std::memcpy(&one,exe.data()+glyph_geometry_source::shadow_one_raw_offset,4);
  require(one==0x3f800000u,"Original layer1 offset constant changed");
  std::memcpy(reinterpret_cast<unsigned char*>(constant_)+0xd390,&constant,4);
  DWORD permission=0;require(VirtualProtect(constant_,0x10000,PAGE_READONLY,&permission)&&VirtualProtect(capacity_,0x10000,PAGE_READONLY,&permission),"Protect source-constant/unconsumed scratch reads");
  Bytes code={0x53,0x56,0x57,0x55,0x81,0xec,0,3,0,0};
  const auto integer=[&](std::uint32_t value){for(unsigned n=0;n<4;++n)code.push_back(static_cast<unsigned char>(value>>(8*n)));};
  const auto load=[&](unsigned reg,std::uint32_t offset){code.push_back(0x8b);code.push_back(static_cast<unsigned char>(0x84+(reg<<3)));code.push_back(0x24);integer(offset);};
  const auto store=[&](std::uint32_t offset,std::uint32_t value){code.insert(code.end(),{0xc7,0x84,0x24});integer(offset);integer(value);};
  const auto emit=[&](const unsigned char*bytes,std::size_t count,std::size_t offset){
   require(offset+count<=exe.size()&&std::equal(bytes,bytes+count,exe.data()+offset),"Original geometry slice bytes substituted");
   code.insert(code.end(),bytes,bytes+count);
  };
  load(3,0x314);load(7,0x318);load(2,0x31c); // EBX descriptor, EDI X, EDX Y
  code.insert(code.end(),{0x89,0x54,0x24,0x1c}); // source Y local
  code.insert(code.end(),{0x8d,0x2c,0x24});store(0x170,1); // unconsumed extended selector comparison
  emit(glyph_geometry_source::base_corners,sizeof(glyph_geometry_source::base_corners),glyph_geometry_source::base_corners_raw_offset);
  for(std::uint32_t offset=0x54;offset<=0x74;offset+=4)store(offset,0);
  store(0x60,one);store(0x64,one); // source slot1 XY; all other XYZ offsets0
  load(6,0x320);code.insert(code.end(),{0x6b,0xf6,0x0c}); // ESI slot*12
  load(0,0x324);code.insert(code.end(),{0x89,0x84,0x24,0x40,2,0,0}); // own color input
  code.insert(code.end(),{0x8d,0x8c,0x24,0x40,2,0,0,0x89,0x4c,0x24,0x14});
  emit(glyph_geometry_source::layer_offsets,sizeof(glyph_geometry_source::layer_offsets),glyph_geometry_source::layer_offsets_raw_offset);
  const auto destination=[&](unsigned offset){load(0,0x328);if(offset)code.insert(code.end(),{0x83,0xc0,static_cast<unsigned char>(offset)});};
  destination(0);emit(glyph_geometry_source::store_tl,sizeof(glyph_geometry_source::store_tl),glyph_geometry_source::store_tl_raw_offset);
  destination(24);emit(glyph_geometry_source::store_tr,sizeof(glyph_geometry_source::store_tr),glyph_geometry_source::store_tr_raw_offset);
  destination(48);emit(glyph_geometry_source::store_br,sizeof(glyph_geometry_source::store_br),glyph_geometry_source::store_br_raw_offset);
  destination(72);emit(glyph_geometry_source::store_bl,sizeof(glyph_geometry_source::store_bl),glyph_geometry_source::store_bl_raw_offset);
  code.insert(code.end(),{0xd9,0xbc,0x24,0x44,2,0,0,0xdf,0xe0,0x66,0x89,0x84,0x24,0x46,2,0,0});
  code.insert(code.end(),{0x0f,0xae,0x9c,0x24,0x48,2,0,0}); // STMXCSR
  load(0,0x32c);load(1,0x244);code.insert(code.end(),{0x89,0x08});load(1,0x248);code.insert(code.end(),{0x89,0x48,4});
  code.insert(code.end(),{0x81,0xc4,0,3,0,0,0x5d,0x5f,0x5e,0x5b,0xc3});
  stub_=VirtualAlloc(nullptr,code.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);require(stub_!=nullptr,"Allocate owned original geometry wrapper");
  std::memcpy(stub_,code.data(),code.size());require(VirtualProtect(stub_,code.size(),PAGE_EXECUTE_READ,&permission)&&FlushInstructionCache(GetCurrentProcess(),stub_,code.size()),"Protect original geometry wrapper RX");
  }catch(...){if(stub_)VirtualFree(stub_,0,MEM_RELEASE);if(constant_)VirtualFree(constant_,0,MEM_RELEASE);if(capacity_)VirtualFree(capacity_,0,MEM_RELEASE);throw;}
 }
 ~SourceGeometry(){if(stub_)VirtualFree(stub_,0,MEM_RELEASE);if(constant_)VirtualFree(constant_,0,MEM_RELEASE);if(capacity_)VirtualFree(capacity_,0,MEM_RELEASE);}
 struct Output {std::array<Vertex,4>vertices{};std::uint16_t cw=0,sw=0;std::uint32_t mxcsr=0;};
 Output run(const civ5::graphics::BnwExtendedGlyphGeometryInput&i)const{
  const auto&s=i.retained_descriptor;
  const std::array<float,9>returned_descriptor{s.u0,s.v0,s.u1,s.v1,s.x0,s.y0,s.x1,s.y1,s.advance_float};
  alignas(16) std::array<unsigned char,512>saved{};__asm__ volatile("fxsave %0":"=m"(saved)::"memory");
  const auto cw=i.diagnostic_x87_control_word;const auto mx=i.diagnostic_mxcsr;
  __asm__ volatile("fninit\n\tfldcw %0\n\tldmxcsr %1"::"m"(cw),"m"(mx):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
  Output out;std::array<std::uint32_t,2>status{};
  reinterpret_cast<Fn>(stub_)(returned_descriptor.data(),i.cursor_x,i.line_y,i.native_slot,i.packed_rgba,out.vertices.data(),status.data());
  __asm__ volatile("fxrstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7");
  out.cw=static_cast<std::uint16_t>(status[0]);out.sw=static_cast<std::uint16_t>(status[0]>>16);out.mxcsr=status[1];return out;
 }
};
int wmain(int argc,wchar_t**argv) try{
 require(argc==4,"Args: original CivilizationV.exe NEW report.json NEW reference.bin");require(!std::filesystem::exists(argv[2])&&!std::filesystem::exists(argv[3]),"Previous geometry artifacts must remain immutable");
 SourceGeometry original(argv[1]);using Input=civ5::graphics::BnwExtendedGlyphGeometryInput;
 Input base;base.caller_declared_numeric_inputs=true;base.retained_descriptor.requested_glyph_index=17;base.retained_descriptor.glyph_index=17;
 auto&s=base.retained_descriptor;s.advance_float=6.5f;s.u0=.11f;s.v0=.23f;s.u1=.67f;s.v1=.89f;s.x0=-.374000012874603271f;s.y0=s.x0;s.x1=12+s.x0;s.y1=23+s.y0;base.packed_rgba=0x93712fb5u;
 std::vector<Input>cases;
 const std::array<std::uint32_t,15>cursors{0,1,7,99,1024,65535,0x00ffffffu,0x01000000u,0x01000001u,0x01000003u,0x7fffffffu,0x80000000u,0x80000001u,0xfffffffeu,0xffffffffu};
 for(auto cursor:cursors){auto input=base;input.cursor_x=cursor;input.line_y=cursor;cases.push_back(input);input.line_y=7;cases.push_back(input);}
 auto from=[](std::uint32_t bits){return std::bit_cast<float>(bits);};
 for(auto bits:{0u,0x80000000u,1u,0x007fffffu,0x00800000u,0x3e800000u,0x3f000000u,0x3f7fffffu,0x3f800000u,0x3f800001u,0xbf800000u,0x7f7fffffu,0xff7fffffu}){
  auto input=base;input.retained_descriptor.x0=from(bits);input.retained_descriptor.y0=from(bits);input.retained_descriptor.x1=from(bits);input.retained_descriptor.y1=from(bits);
  for(auto cursor:{0u,7u,0x01000001u,0xffffffffu}){input.cursor_x=cursor;input.line_y=cursor;cases.push_back(input);}
 }
 alignas(16) std::array<unsigned char,512>environment{};__asm__ volatile("fxsave %0":"=m"(environment));
 std::uint32_t supported_mxcsr=0;std::memcpy(&supported_mxcsr,environment.data()+28,4);if(!supported_mxcsr)supported_mxcsr=0xffbfu;
 Bytes reference{'G','L','Y','P','H','0','0','1'};for(char c:std::string(glyph_geometry_source::exe_sha))reference.push_back(static_cast<unsigned char>(c));
 reference.resize(76);std::uint32_t reference_count=0;
 const auto put=[&](std::uint32_t value,unsigned bytes=4){for(unsigned n=0;n<bytes;++n)reference.push_back(static_cast<unsigned char>(value>>(8*n)));};
 std::uint64_t compared=0,overflow=0;unsigned refused=0;
 for(auto input:cases)for(unsigned pc:{0u,2u,3u})for(unsigned rc=0;rc<4;++rc)for(unsigned sse_rc=0;sse_rc<4;++sse_rc)for(unsigned denorm=0;denorm<4;++denorm)for(unsigned slot=0;slot<3;++slot){
  const std::uint32_t denorm_bits=(denorm&1u?0x40u:0u)|(denorm&2u?0x8000u:0u);if(denorm_bits&~supported_mxcsr)continue;
  input.diagnostic_x87_control_word=static_cast<std::uint16_t>(0x007fu|(pc<<8)|(rc<<10));input.diagnostic_mxcsr=0x1f80u|(sse_rc<<13)|denorm_bits;input.native_slot=slot;
  const auto expected=original.run(input);require(expected.cw==input.diagnostic_x87_control_word,"Original geometry did not preserve incoming CW");require(((expected.sw>>11)&7u)==0,"Original geometry left unexpected FPU stack depth");
  bool finite=true;for(const auto&v:expected.vertices)for(float f:{v.x,v.y,v.z,v.u,v.v})finite&=std::isfinite(f);
  const auto&descriptor=input.retained_descriptor;put(descriptor.requested_glyph_index,2);put(descriptor.glyph_index,2);
  for(float f:{descriptor.u0,descriptor.v0,descriptor.u1,descriptor.v1,descriptor.x0,descriptor.y0,descriptor.x1,descriptor.y1,descriptor.advance_float})put(std::bit_cast<std::uint32_t>(f));
  put(input.cursor_x);put(input.line_y);put(input.native_slot);put(input.packed_rgba);put(input.diagnostic_x87_control_word,2);put(0,2);put(input.diagnostic_mxcsr);put(finite?1:0);
  const auto*source_bytes=reinterpret_cast<const unsigned char*>(expected.vertices.data());reference.insert(reference.end(),source_bytes,source_bytes+sizeof(expected.vertices));++reference_count;
  std::optional<civ5::graphics::BnwExtendedGlyphGeometryResult>prepared;
  try{prepared=civ5::graphics::prepare_bnw_extended_glyph_geometry(input);}
  catch(const std::exception&){if(finite)throw;++overflow;continue;}
  const auto&actual=*prepared;
  require(finite,"Candidate accepted original nonfinite geometry");
   if(std::memcmp(expected.vertices.data(),actual.vertices.data(),sizeof(expected.vertices))!=0){
    std::cerr<<"Source geometry mismatch PC="<<pc<<" RC="<<rc<<" SSE_RC="<<sse_rc<<" denorm="<<denorm<<" slot="<<slot<<" cursor="<<input.cursor_x<<','<<input.line_y<<'\n';
    for(unsigned n=0;n<4;++n){const auto&e=expected.vertices[n];const auto&a=actual.vertices[n];std::cerr<<std::hex<<" vertex"<<n<<" XY source="<<std::bit_cast<std::uint32_t>(e.x)<<','<<std::bit_cast<std::uint32_t>(e.y)<<" candidate="<<std::bit_cast<std::uint32_t>(a.x)<<','<<std::bit_cast<std::uint32_t>(a.y)<<std::dec<<'\n';}return 1;
   }
   require(!actual.native_label_ready&&!actual.production_text_update_observed&&!actual.incoming_production_cw_observed&&!actual.original_whole_run_class_inputs_verified,"Numeric geometry minted native Label readiness");++compared;
 }
 Input witness=base;witness.cursor_x=0x01000001u;witness.diagnostic_x87_control_word=0x037f;witness.diagnostic_mxcsr=0x1f80;witness.retained_descriptor.x0=0;witness.retained_descriptor.x1=1;
 const auto shape=original.run(witness);require(shape.vertices[1].x!=shape.vertices[2].x,"No meaningful original mixed x87/SSE integer-cursor witness");
 const auto before=civ5::graphics::prepare_bnw_extended_glyph_geometry(witness);witness.retained_descriptor.advance_float=177.25f;
 const auto after=civ5::graphics::prepare_bnw_extended_glyph_geometry(witness);require(std::memcmp(before.vertices.data(),after.vertices.data(),sizeof(shape.vertices))==0,"GCP float advance was incorrectly substituted for raster quad width");
 const auto rejection=[&](const Input&i){bool blocked=false;try{(void)civ5::graphics::prepare_bnw_extended_glyph_geometry(i);}catch(const std::exception&){blocked=true;}require(blocked,"Geometry accepted invalid diagnostic/source input");++refused;};
 auto altered=witness;altered.caller_declared_numeric_inputs=false;rejection(altered);altered=witness;altered.native_slot=3;rejection(altered);altered=witness;++altered.retained_descriptor.requested_glyph_index;rejection(altered);
 altered=witness;altered.diagnostic_x87_control_word=0x017f;rejection(altered);altered=witness;altered.diagnostic_x87_control_word&=~1u;rejection(altered);altered=witness;altered.diagnostic_mxcsr&=~0x80u;rejection(altered);altered=witness;altered.diagnostic_mxcsr|=0x80000000u;rejection(altered);
 altered=witness;altered.retained_descriptor.x0=std::numeric_limits<float>::quiet_NaN();rejection(altered);altered=witness;altered.retained_descriptor.advance_float=std::numeric_limits<float>::infinity();rejection(altered);
 std::uint16_t caller_cw=0,returned_cw=0;std::uint32_t caller_mx=0,returned_mx=0;
 __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(caller_cw),"=m"(caller_mx));(void)civ5::graphics::prepare_bnw_extended_glyph_geometry(witness);
 __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(returned_cw),"=m"(returned_mx));require(caller_cw==returned_cw&&caller_mx==returned_mx,"Geometry kernel leaked explicit diagnostic floating environment");
 require(sha(read(argv[1]))==glyph_geometry_source::exe_sha,"Original EXE changed during geometry gate");
 for(unsigned n=0;n<4;++n)reference[72+n]=static_cast<unsigned char>(reference_count>>(8*n));
 require(reference.size()==76ull+164ull*reference_count,"Reference input/output record length");
 std::ofstream reference_file(argv[3],std::ios::binary);require(bool(reference_file),"Create NEW raw original geometry reference");reference_file.write(reinterpret_cast<const char*>(reference.data()),static_cast<std::streamsize>(reference.size()));reference_file.flush();require(bool(reference_file),"Flush raw original geometry reference");reference_file.close();require(!reference_file.fail(),"Close raw original geometry reference");
 std::ofstream report(argv[2],std::ios::binary);require(bool(report),"Create NEW geometry report");report<<"{\n\"schema\":\"civis-original-extended-glyph-geometry-gate-v1\",\n\"originalExeSha256\":\""<<glyph_geometry_source::exe_sha<<"\",\n\"originalSlicesExecuted\":6,\n\"originalInstructionBytes\":773,\n\"inputCases\":"<<cases.size()<<",\n\"finiteProfileComparisons\":"<<compared<<",\n\"nonfiniteOutputRefusals\":"<<overflow<<",\n\"negativeRefusals\":"<<refused<<",\n\"mixedPrecisionWitnessCursor\":16777217,\n\"originalUpperRightFloat32Bits\":"<<std::bit_cast<std::uint32_t>(shape.vertices[1].x)<<",\n\"originalLowerRightFloat32Bits\":"<<std::bit_cast<std::uint32_t>(shape.vertices[2].x)<<",\n\"advanceMutationDoesNotResizeQuad\":true,\n\"floatingEnvironmentRestored\":true,\n\"authoredBranchIncluded\":false,\n\"productionIncomingCwObserved\":false,\n\"originalWholeRunClassInputsVerified\":false,\n\"actualLabelOrParentObserved\":false,\n\"nativeLabelReady\":false,\n\"gpuExecuted\":false,\n\"pass\":true\n}\n";report.flush();require(bool(report),"Flush geometry report");report.close();require(!report.fail(),"Close geometry report");
 std::cout<<"PASS original six unmodified numeric slices773 bytes: "<<compared<<" finite profiles, "<<overflow<<" overflow refusals, "<<refused<<" invalid inputs; nativeLabelReady=false\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
