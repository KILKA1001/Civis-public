#include "civ5/graphics/bnw_extended_label_glyph_quote.hpp"
#include "extended_label_geometry_source_pins.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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

static_assert(sizeof(void*)==8,"Cross-architecture numeric gate requires x64");
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


int wmain(int argc,wchar_t**argv)try{
 require(argc==4,"Args: pinned original-produced reference.bin expectedSHA NEW report.json");
 require(!std::filesystem::exists(argv[3]),"Cross-architecture report must be NEW");
 const auto bytes=read(argv[1]);std::string expected_sha;
 for(const wchar_t*at=argv[2];*at;++at){require(*at<128,"Reference SHA must be ASCII");expected_sha.push_back(static_cast<char>(*at));}
 require(expected_sha.size()==64&&sha(bytes)==expected_sha,"Independent original-produced reference SHA mismatch");
 require(bytes.size()>=76&&std::equal(bytes.begin(),bytes.begin()+8,"GLYPH001"),"Original reference version/magic");
 require(std::equal(bytes.begin()+8,bytes.begin()+72,glyph_geometry_source::exe_sha),"Original reference EXE authority changed");
 std::size_t cursor=72;const auto get=[&](unsigned n=4){require(n<=4&&cursor+n<=bytes.size(),"Truncated original reference");std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(bytes[cursor++])<<(8*i);return v;};
 const auto count=get();require(count>0&&bytes.size()==76ull+164ull*count,"Original reference record count/range");
 std::uint32_t compared=0,refused=0;
 for(std::uint32_t row=0;row<count;++row){
  civ5::graphics::BnwExtendedGlyphGeometryInput input;input.caller_declared_numeric_inputs=true;
  auto&d=input.retained_descriptor;d.requested_glyph_index=static_cast<std::uint16_t>(get(2));d.glyph_index=static_cast<std::uint16_t>(get(2));
  for(float*at:{&d.u0,&d.v0,&d.u1,&d.v1,&d.x0,&d.y0,&d.x1,&d.y1,&d.advance_float})*at=std::bit_cast<float>(get());
  input.cursor_x=get();input.line_y=get();input.native_slot=get();input.packed_rgba=get();input.diagnostic_x87_control_word=static_cast<std::uint16_t>(get(2));require(get(2)==0,"Unknown numeric reference extension");input.diagnostic_mxcsr=get();const auto finite_tag=get();require(finite_tag<=1,"Unknown reference outcome");
  std::array<civ5::ui::NativeLabelPackedVertex,4>vertices;std::memcpy(vertices.data(),bytes.data()+cursor,sizeof(vertices));cursor+=sizeof(vertices);
  bool finite=true;for(const auto&v:vertices){for(float f:{v.x,v.y,v.z,v.u,v.v})finite&=std::isfinite(f);require(v.rgba==input.packed_rgba,"Original numeric fixture packed word differs");}
  require(finite==bool(finite_tag),"Original numeric fixture outcome changed");
  // Diagnostic quote data ONLY; no cache owner, upload or currentness receipt
  // is created. Actual production publisher must supply its SAME live quote.
  civ5::ui::BnwSystemFontSlotQuote quote;quote.requested_glyph_index=d.requested_glyph_index;quote.slot_index=37;quote.cache_hit=true;
  auto&slot=quote.slot;slot.glyph_index=d.glyph_index;slot.advance=177;slot.advance_float=d.advance_float;slot.generation=0x123456789abcdef0ull;slot.raster_uploaded=false;
  slot.u0=d.u0;slot.v0=d.v0;slot.u1=d.u1;slot.v1=d.v1;slot.x0=d.x0;slot.y0=d.y0;slot.x1=d.x1;slot.y1=d.y1;
  std::optional<civ5::graphics::BnwExtendedGlyphQuotedGeometry>prepared;
  try{prepared=civ5::graphics::prepare_bnw_extended_glyph_quote(quote,input);}catch(const std::exception&){if(finite)throw;++refused;continue;}
  require(finite,"x64 numeric core accepted original nonfinite output");const auto&actual=prepared->numeric;
  require(prepared->quad.uploaded_slot.slot==quote.slot&&prepared->quad.uploaded_slot.slot_index==quote.slot_index&&prepared->quad.uploaded_slot.cache_hit==quote.cache_hit&&prepared->quad.uploaded_slot.requested_glyph_index==quote.requested_glyph_index,"Thin adapter changed retained quote or minted upload readiness");
  require(std::memcmp(actual.vertices.data(),prepared->quad.vertices.data(),sizeof(vertices))==0,"Thin adapter changed packed quad bytes");
  if(std::memcmp(vertices.data(),actual.vertices.data(),sizeof(vertices))!=0){std::cerr<<"x64 original numeric fixture mismatch row="<<row<<" cursor="<<input.cursor_x<<','<<input.line_y<<" CW="<<std::hex<<input.diagnostic_x87_control_word<<" MXCSR="<<input.diagnostic_mxcsr<<std::dec<<'\n';return 1;}
  require(!actual.native_label_ready&&!actual.production_text_update_observed&&!actual.incoming_production_cw_observed&&!actual.original_whole_run_class_inputs_verified,"Cross fixture minted native Label authority");++compared;
 }
 require(cursor==bytes.size()&&compared+refused==count,"Incomplete cross-architecture gate");
 require(sha(read(argv[1]))==expected_sha,"Original-produced reference changed during gate");
 std::ofstream report(argv[3],std::ios::binary);require(bool(report),"Create NEW cross-architecture report");
 report<<"{\n\"schema\":\"civis-x64-original-glyph-geometry-cross-gate-v1\",\n\"independentOriginalReferenceSha256\":\""<<expected_sha<<"\",\n\"referenceRecords\":"<<count<<",\n\"byteExactFiniteComparisons\":"<<compared<<",\n\"nonfiniteRefusals\":"<<refused<<",\n\"nativeLabelReady\":false,\n\"actualParentObserved\":false,\n\"productionIncomingCwObserved\":false,\n\"gpuExecuted\":false,\n\"pass\":true\n}\n";
 report.flush();require(bool(report),"Flush cross report");report.close();require(!report.fail(),"Close cross report");
 std::cout<<"PASS x64 against independently retained original x86 numeric outputs: "<<compared<<" finite records, "<<refused<<" overflow refusals; nativeLabelReady=false\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
