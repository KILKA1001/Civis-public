#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::runtime::detail {
// Pure bounded textual facts only. These functions cannot issue a Boot owner,
// manager state, native setter return or game-turn admission.
inline std::string_view net_debug_trim(std::string_view s){
 while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.remove_prefix(1);
 while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.remove_suffix(1);
 return s;
}
inline bool net_debug_equal(std::string_view a,std::string_view b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i){auto x=a[i],y=b[i];if(x>='A'&&x<='Z')x+=('a'-'A');if(y>='A'&&y<='Z')y+=('a'-'A');if(x!=y)return false;}
 return true;
}
inline std::optional<bool> net_debug_explicit_boolean(std::string_view source){
 const auto need=[](bool ok,const char* why){if(!ok)throw std::invalid_argument(why);};
 need(source.size()<=1024*1024,"Net debug config exceeds bounded input");
 for(char byte:source){const auto c=static_cast<unsigned char>(byte);need((c>=32&&c<=126)||c=='\t'||c=='\r'||c=='\n',"Net debug config outside explicit ASCII subset");}
 std::vector<std::string> sections,keys;std::string section;std::optional<bool> found;
 while(!source.empty()){
  auto end=source.find('\n');auto line=source.substr(0,end);source=end==source.npos?std::string_view{}:source.substr(end+1);
  if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
  need(line.find('\r')==line.npos&&line.size()<1024,"Net debug config malformed line");
  line=net_debug_trim(line);if(line.empty()||line.front()==';')continue;
  if(line.front()=='['){
   need(line.size()>2&&line.back()==']'&&line.find(']',1)==line.size()-1&&line.find('[',1)==line.npos,"Net debug config malformed section");
   section=std::string(line.substr(1,line.size()-2));
   for(const auto& prior:sections)need(!net_debug_equal(prior,section),"Net debug duplicate section unsupported");
   sections.push_back(section);keys.clear();continue;
  }
  auto sep=line.find('=');if(section.empty()||sep==line.npos)continue;
  auto key=net_debug_trim(line.substr(0,sep)),value=net_debug_trim(line.substr(sep+1));
  need(!key.empty(),"Net debug empty key unsupported");
  for(const auto& prior:keys)need(!net_debug_equal(prior,key),"Net debug duplicate key unsupported");
  keys.emplace_back(key);
  if(!net_debug_equal(section,"DEBUG")||!net_debug_equal(key,"LoggingEnabled"))continue;
  need(value=="0"||value=="1","Net debug value outside explicit boolean subset");
  found=value=="1";
 }
 return found;
}
} // namespace civ5::runtime::detail
