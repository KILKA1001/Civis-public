#include "../src/graphics/bnw_unit_model_source_request.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
using namespace civ5::graphics::model_request_detail;
static void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::ifstream& f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated source model fixture");return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
int main(int argc,char**argv){try{
 need(argc==2,"Expected original model request fixture");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWMRQ01","Source model fixture schema");const auto count=word(f);need(count==24,"Source model scenario cardinality");
 for(unsigned i=0;i<count;++i){std::uint32_t row[13];for(auto& v:row)v=word(f);const auto [scale,manager,hit,success,modifier,modifier_scale,flags,filter,filter_count,load_flags,refs,result_scale,result_flags]=std::to_array(row);const auto args=prepare(scale);ResourceFields resource{manager?0x3f000000U:0x3f800000U,0x10203040};
  need(success==(!manager||hit),"Native cache refusal result");
  if(manager){need(load_flags==args.load_flags&&modifier==args.modifier_required,"Native manager source branch/load flags");need(modifier_scale==args.modifier_scale_bits&&flags==args.flags&&filter==args.filter&&filter_count==args.filter_count,"Native modifier source fields");need(refs==unsigned(args.modifier_required),"Native modifier initial reference count");if(hit&&args.modifier_required)resource=apply_modifier(resource,args);}
  else{need(load_flags==0&&modifier==0,"Native fallback has no manager request");need(apply_fallback(resource,scale)==0x3f800000,"Native fallback old scale return");}
  need(resource.scale_bits==result_scale&&resource.flags==result_flags,"Native conditional resource field mutation");
 }
 need(f.peek()==std::ifstream::traits_type::eof(),"Unexpected source fixture tail");std::cout<<"PASS 24 original conditional request/modifier/fallback/cache-refusal cases; pure source fixture, no production cache/resource receipt\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
