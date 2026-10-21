#include "../src/graphics/bnw_unit_model_manager_source_initializer.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::graphics::model_manager_init_detail;
static void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::ifstream&f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated initializer source fixture");return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
int main(int argc,char**argv){try{
 need(argc==2,"Expected initializer source fixture");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWMGI01","Initializer source fixture schema");need(word(f)==2,"Initializer scenario count");
 for(unsigned row=0;row<2;++row){const auto flags=word(f);auto registry=constructor_fields(row?0x5678:0x1234);
  need(word(f)==registry.source_vtable&&word(f)==registry.constructor_arg,"Original registry ctor labels/argument");for(auto expected:registry.zero_fields)need(word(f)==expected,"Original registry empty counters");for(auto expected:registry.cache_words)need(word(f)==expected,"Original twelve per-kind caches empty words");need(word(f)==registry.timer_callback&&word(f)==registry.initial_thread,"Original registry timer/thread fields");
  register_source_creators(registry);for(unsigned i=0;i<4;++i){auto kind=word(f),creator=word(f);need(registry.creators.at(kind)==creator,"Original registered source creator");}need(word(f)==0x9f4cc0,"Original published adapter source label");
  const auto wrapper=wrapper_fields(0xabc,2);need(word(f)==wrapper.source_vtable&&word(f)==wrapper.refs&&word(f)==wrapper.name_id&&word(f)==wrapper.timestamp_bits&&word(f)==wrapper.marked&&word(f)==wrapper.underlying_resource,"Original kind0 wrapper constructor");need(word(f)==wrapper_fields(0xabc,10).marked,"Original creator flags bit8");
  const auto resource=resource_constructor_fields(flags);
  constexpr std::array offsets={0U,4U,8U,12U,16U,20U,24U,32U,36U,40U,44U,52U,56U,60U,68U,72U,76U,84U,88U,92U,100U,104U,108U,120U,128U,132U,136U,144U,148U,152U,164U,172U,176U,180U,184U,28U,188U,192U};
  for(auto offset:offsets)need(word(f)==resource.words.at(offset),"Original underlying resource ctor scalar fields");need(word(f)==resource.load_byte,"Original resource load byte");
 }
 need(f.peek()==std::ifstream::traits_type::eof(),"Initializer source fixture tail");std::cout<<"PASS source registry init2cases, 12 empty caches, 4 creators, kind0 wrapper/flags, resource ctor scalar fields; pure fixture only\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
